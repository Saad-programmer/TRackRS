// SPDX-License-Identifier: GPL-2.0-or-later
#include "post.h"
#include "exposure.h"
#include "engine/render/basic.h"
#include "engine/rhi/rhi.h"
#include "legacy_metrics.h"
#include "validation.h"
#include <array>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <vector>
namespace trackrs::post {
namespace {
Settings current;
struct Target {
  GLuint fbo = 0, texture = 0;
  glm::ivec2 size{0};
};
GLuint texture2d(GLenum internal, glm::ivec2 size, GLenum format, GLenum type, bool mips = false) {
  GLuint id = 0;
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);
  glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internal), size.x, size.y, 0, format, type, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mips ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  if (mips)
    glGenerateMipmap(GL_TEXTURE_2D);
  return id;
}
Target target(GLenum internal, glm::ivec2 size, GLenum format, GLenum type, bool mips = false) {
  Target t;
  t.size = size;
  t.texture = texture2d(internal, size, format, type, mips);
  glGenFramebuffers(1, &t.fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, t.fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t.texture, 0);
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    throw std::runtime_error("post-processing target incomplete");
  return t;
}
void release(Target &t) {
  glDeleteFramebuffers(1, &t.fbo);
  glDeleteTextures(1, &t.texture);
  t = {};
}
struct Processor {
  rhi::ShaderProgram luminance, adapt, down, up, composite, ssao, ssaoBlur;
  Target ao, aoBlurred;
  std::vector<glm::vec3> kernel = ssaoKernel(12);
  GLuint vao = 0, hdrFbo = 0, hdrColor = 0, hdrDepth = 0;
  glm::ivec2 size{0};
  Target logLuminance;
  std::array<Target, 2> adapted;
  std::vector<Target> bloom;
  unsigned current = 0;
  bool reset = true, bound = false;
  float lastTime = 0;
  GLint previousFbo = 0;
  std::uint64_t resolves = 0, passes = 0;
  explicit Processor(const std::filesystem::path &d)
      : luminance(d / "shaders/post/fullscreen.vert", d / "shaders/post/luminance.frag"),
        adapt(d / "shaders/post/fullscreen.vert", d / "shaders/post/adapt.frag"),
        down(d / "shaders/post/fullscreen.vert", d / "shaders/post/bloom_down.frag"),
        up(d / "shaders/post/fullscreen.vert", d / "shaders/post/bloom_up.frag"),
        composite(d / "shaders/post/fullscreen.vert", d / "shaders/post/composite.frag"),
        ssao(d / "shaders/post/fullscreen.vert", d / "shaders/post/ssao.frag"),
        ssaoBlur(d / "shaders/post/fullscreen.vert", d / "shaders/post/ssao_blur.frag") {
    glGenVertexArrays(1, &vao);
    logLuminance = target(GL_R16F, {64, 64}, GL_RED, GL_FLOAT, true);
    for (auto &a : adapted)
      a = target(GL_R32F, {1, 1}, GL_RED, GL_FLOAT);
  }
  ~Processor() {
    releaseFrame();
    release(logLuminance);
    for (auto &a : adapted)
      release(a);
    glDeleteVertexArrays(1, &vao);
  }
  void releaseFrame() {
    glDeleteFramebuffers(1, &hdrFbo);
    glDeleteTextures(1, &hdrColor);
    glDeleteTextures(1, &hdrDepth);
    hdrFbo = hdrColor = hdrDepth = 0;
    for (auto &b : bloom)
      release(b);
    bloom.clear();
    release(ao);
    release(aoBlurred);
  }
  void resize(glm::ivec2 s) {
    releaseFrame();
    size = s;
    hdrColor = texture2d(GL_RGBA16F, s, GL_RGBA, GL_FLOAT);
    glGenTextures(1, &hdrDepth);
    glBindTexture(GL_TEXTURE_2D, hdrDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, s.x, s.y, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glGenFramebuffers(1, &hdrFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, hdrColor, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, hdrDepth, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
      throw std::runtime_error("HDR scene target incomplete");
    for (auto level : bloomChain(s))
      bloom.push_back(target(GL_R11F_G11F_B10F, level, GL_RGB, GL_FLOAT));
    const glm::ivec2 half = glm::max(s / 2, glm::ivec2(1));
    ao = target(GL_R8, half, GL_RED, GL_UNSIGNED_BYTE);
    aoBlurred = target(GL_R8, half, GL_RED, GL_UNSIGNED_BYTE);
    reset = true;
  }
  void fullscreen(GLuint fbo, glm::ivec2 viewport) {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, viewport.x, viewport.y);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    ++passes;
  }
};
std::unique_ptr<Processor> processor;
enum class Unit : int {}; // distinct type: a texture unit is not a texture name
void bindTexture(GLuint program, const char *name, Unit unit, GLuint texture) {
  glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(unit));
  glBindTexture(GL_TEXTURE_2D, texture);
  glUniform1i(glGetUniformLocation(program, name), static_cast<int>(unit));
}
} // namespace

void configure(Settings settings) { current = settings; }
Settings settings() { return current; }
bool active() { return processor && processor->bound; }

void begin(int width, int height, const std::filesystem::path &data) {
  if (width <= 0 || height <= 0)
    throw std::invalid_argument("invalid post-processing size");
  draw::flush();
  // Capture the frame's framebuffer before any lazy resource creation below
  // (creating targets binds them).
  GLint previousFbo = 0, previousTexture = 0;
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousFbo);
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
  if (!processor)
    processor = std::make_unique<Processor>(data);
  auto &p = *processor;
  p.previousFbo = previousFbo;
  if (p.size != glm::ivec2(width, height))
    p.resize({width, height});
  glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
  glBindFramebuffer(GL_FRAMEBUFFER, p.hdrFbo);
  glViewport(0, 0, width, height);
  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  p.bound = true;
  draw::invalidateGpuState();
}

void end(float time, const glm::mat4 &projection) {
  if (!active())
    return;
  draw::flush();
  validation::PassScope timing("post");
  auto &p = *processor;
  p.bound = false;
  GLint program = 0, vao = 0, active = 0, texture = 0;
  glGetIntegerv(GL_CURRENT_PROGRAM, &program);
  glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
  glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
  glActiveTexture(GL_TEXTURE0);
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
  const bool depth = glIsEnabled(GL_DEPTH_TEST), blend = glIsEnabled(GL_BLEND), cull = glIsEnabled(GL_CULL_FACE),
             scissor = glIsEnabled(GL_SCISSOR_TEST), srgb = glIsEnabled(GL_FRAMEBUFFER_SRGB);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  glDisable(GL_CULL_FACE);
  glDisable(GL_SCISSOR_TEST);
  glDisable(GL_FRAMEBUFFER_SRGB);
  glBindVertexArray(p.vao);
  const ExposureSettings exposure;
  const float dt = p.reset ? 0.f : std::max(time - p.lastTime, 0.f);
  p.lastTime = time;

  // 1. Log luminance (64x64) and its mip chain -> geometric mean.
  p.luminance.poll();
  glUseProgram(p.luminance.id());
  bindTexture(p.luminance.id(), "uHdr", Unit{0}, p.hdrColor);
  p.fullscreen(p.logLuminance.fbo, p.logLuminance.size);
  glBindTexture(GL_TEXTURE_2D, p.logLuminance.texture);
  glGenerateMipmap(GL_TEXTURE_2D);
  // 2. Temporal adaptation into the other 1x1 target.
  p.adapt.poll();
  const unsigned next = p.current ^ 1u;
  const auto a = p.adapt.id();
  glUseProgram(a);
  bindTexture(a, "uLogLuminance", Unit{0}, p.logLuminance.texture);
  bindTexture(a, "uPrevious", Unit{1}, p.adapted[p.current].texture);
  glUniform1f(glGetUniformLocation(a, "uLogLod"), 6.f); // 64 -> 1
  glUniform1f(glGetUniformLocation(a, "uDt"), dt);
  glUniform1f(glGetUniformLocation(a, "uSpeedUp"), exposure.speedUp);
  glUniform1f(glGetUniformLocation(a, "uSpeedDown"), exposure.speedDown);
  glUniform1i(glGetUniformLocation(a, "uReset"), p.reset);
  p.fullscreen(p.adapted[next].fbo, {1, 1});
  p.current = next;
  p.reset = false;
  // 3. Bloom: downsample chain, then additive tent upsample back up.
  const bool bloom = current.bloom && !p.bloom.empty();
  if (bloom) {
    p.down.poll();
    const auto d = p.down.id();
    glUseProgram(d);
    glUniform1f(glGetUniformLocation(d, "uThreshold"), 1.f);
    glUniform1f(glGetUniformLocation(d, "uKnee"), .5f);
    GLuint source = p.hdrColor;
    glm::ivec2 sourceSize = p.size;
    for (std::size_t i = 0; i < p.bloom.size(); ++i) {
      bindTexture(d, "uSource", Unit{0}, source);
      glUniform2f(glGetUniformLocation(d, "uTexel"), 1.f / static_cast<float>(sourceSize.x),
                  1.f / static_cast<float>(sourceSize.y));
      glUniform1i(glGetUniformLocation(d, "uPrefilter"), i == 0);
      p.fullscreen(p.bloom[i].fbo, p.bloom[i].size);
      source = p.bloom[i].texture;
      sourceSize = p.bloom[i].size;
    }
    p.up.poll();
    const auto u = p.up.id();
    glUseProgram(u);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    for (std::size_t i = p.bloom.size() - 1; i > 0; --i) {
      bindTexture(u, "uSource", Unit{0}, p.bloom[i].texture);
      glUniform2f(glGetUniformLocation(u, "uTexel"), 1.f / static_cast<float>(p.bloom[i].size.x),
                  1.f / static_cast<float>(p.bloom[i].size.y));
      glUniform1f(glGetUniformLocation(u, "uWeight"), 1.f);
      p.fullscreen(p.bloom[i - 1].fbo, p.bloom[i - 1].size);
    }
    glDisable(GL_BLEND);
  }
  // 4. SSAO at half resolution, then a depth-aware blur.
  const bool occlusion = current.ambientOcclusion && p.ao.fbo;
  if (occlusion) {
    const glm::mat4 inverse = glm::inverse(projection);
    p.ssao.poll();
    const auto o = p.ssao.id();
    glUseProgram(o);
    bindTexture(o, "uDepth", Unit{0}, p.hdrDepth);
    glUniformMatrix4fv(glGetUniformLocation(o, "uProjection"), 1, GL_FALSE, &projection[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(o, "uInverseProjection"), 1, GL_FALSE, &inverse[0][0]);
    glUniform3fv(glGetUniformLocation(o, "uKernel[0]"), static_cast<GLsizei>(p.kernel.size()), &p.kernel[0][0]);
    glUniform1f(glGetUniformLocation(o, "uRadius"), current.aoRadius);
    p.fullscreen(p.ao.fbo, p.ao.size);
    p.ssaoBlur.poll();
    const auto bl = p.ssaoBlur.id();
    glUseProgram(bl);
    bindTexture(bl, "uAo", Unit{0}, p.ao.texture);
    bindTexture(bl, "uDepth", Unit{1}, p.hdrDepth);
    glUniform2f(glGetUniformLocation(bl, "uTexel"), 1.f / static_cast<float>(p.ao.size.x),
                1.f / static_cast<float>(p.ao.size.y));
    glUniformMatrix4fv(glGetUniformLocation(bl, "uInverseProjection"), 1, GL_FALSE, &inverse[0][0]);
    p.fullscreen(p.aoBlurred.fbo, p.aoBlurred.size);
  }
  // 5. Composite to the frame's original framebuffer.
  p.composite.poll();
  const auto c = p.composite.id();
  glUseProgram(c);
  bindTexture(c, "uHdr", Unit{0}, p.hdrColor);
  bindTexture(c, "uBloom", Unit{1}, bloom ? p.bloom[0].texture : p.adapted[p.current].texture);
  bindTexture(c, "uAdapted", Unit{2}, p.adapted[p.current].texture);
  bindTexture(c, "uAo", Unit{3}, occlusion ? p.aoBlurred.texture : p.adapted[p.current].texture);
  glUniform1f(glGetUniformLocation(c, "uAoStrength"), occlusion ? current.aoStrength : 0.f);
  glUniform1i(glGetUniformLocation(c, "uAutoExposure"), current.autoExposure);
  glUniform1f(glGetUniformLocation(c, "uKey"), exposure.key);
  glUniform1f(glGetUniformLocation(c, "uMinExposure"), exposure.minExposure);
  glUniform1f(glGetUniformLocation(c, "uMaxExposure"), exposure.maxExposure);
  glUniform1f(glGetUniformLocation(c, "uBloomIntensity"), bloom ? current.bloomIntensity : 0.f);
  glUniform1f(glGetUniformLocation(c, "uVignette"), current.vignette);
  // Developer views (documented in docs/reports/PHASE_3D.md): 1 no bloom,
  // 2 bloom only, 3 ambient occlusion.
  static const int debugView = [] {
    const char *v = std::getenv("TRACKRS_POST_DEBUG");
    return v ? std::atoi(v) : 0;
  }();
  glUniform1i(glGetUniformLocation(c, "uDebugView"), debugView);
  p.fullscreen(static_cast<GLuint>(p.previousFbo), p.size);
  ++p.resolves;
  if (validation::run.capture)
    validation::run.frameDraws += 3 + (bloom ? 2 * p.bloom.size() - 1 : 0) + (occlusion ? 2 : 0);
  // Restore state for the HUD.
  for (int unit = 3; unit >= 0; --unit) {
    glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(unit));
    glBindTexture(GL_TEXTURE_2D, unit == 0 ? static_cast<GLuint>(texture) : 0);
  }
  glActiveTexture(static_cast<GLenum>(active));
  glUseProgram(static_cast<GLuint>(program));
  glBindVertexArray(static_cast<GLuint>(vao));
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  for (auto [cap, on] : {std::pair{GL_DEPTH_TEST, depth}, {GL_BLEND, blend}, {GL_CULL_FACE, cull},
                         {GL_SCISSOR_TEST, scissor}, {GL_FRAMEBUFFER_SRGB, srgb}})
    on ? glEnable(cap) : glDisable(cap);
  draw::invalidateGpuState();
}

void shutdown() { processor.reset(); }

nlohmann::json report() {
  if (!processor)
    return nullptr;
  const auto &p = *processor;
  std::size_t bloomBytes = 0;
  for (const auto &b : p.bloom)
    bloomBytes += static_cast<std::size_t>(b.size.x) * static_cast<std::size_t>(b.size.y) * 4;
  return {{"enabled", current.enabled},
          {"auto_exposure", current.autoExposure},
          {"bloom", current.bloom},
          {"bloom_levels", p.bloom.size()},
          {"ambient_occlusion", current.ambientOcclusion},
          {"ao_bytes", static_cast<std::size_t>(p.ao.size.x) * static_cast<std::size_t>(p.ao.size.y) * 2},
          {"size", {p.size.x, p.size.y}},
          {"resolves", p.resolves},
          {"fullscreen_passes", p.passes},
          {"hdr_bytes", static_cast<std::size_t>(p.size.x) * static_cast<std::size_t>(p.size.y) * (8 + 4)},
          {"bloom_bytes", bloomBytes}};
}
void writeReport(const std::filesystem::path &path) {
  if (processor)
    std::ofstream(path) << report().dump(2) << '\n';
}
} // namespace trackrs::post
