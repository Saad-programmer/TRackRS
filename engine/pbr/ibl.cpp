// SPDX-License-Identifier: GPL-2.0-or-later
#include "ibl.h"
#include "engine/render/basic.h"
#include "engine/world/state.h"
#include <algorithm>
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <map>
#include <set>
namespace trackrs::pbr {
namespace {
void check(bool v, const char *text) {
  if (!v)
    throw std::runtime_error(text);
}
double now() {
  return std::chrono::duration<double>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}
struct CaptureState {
  GLint draw = 0, read = 0, rb = 0, vao = 0, program = 0, active = 0, pbo = 0,
        depth = 0, cull = 0, front = 0;
  std::array<GLint, 4> viewport{};
  std::array<GLint, 8> textures{};
  std::array<GLboolean, 6> enables{};
  GLboolean mask = 0;
  std::array<GLboolean, 4> colors{};
  static constexpr std::array<GLenum, 6> caps = {
      GL_DEPTH_TEST,   GL_CULL_FACE,        GL_BLEND,
      GL_SCISSOR_TEST, GL_FRAMEBUFFER_SRGB, GL_SAMPLE_ALPHA_TO_COVERAGE};
  CaptureState() {
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read);
    glGetIntegerv(GL_RENDERBUFFER_BINDING, &rb);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
    glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &pbo);
    glGetIntegerv(GL_DEPTH_FUNC, &depth);
    glGetIntegerv(GL_CULL_FACE_MODE, &cull);
    glGetIntegerv(GL_FRONT_FACE, &front);
    glGetIntegerv(GL_VIEWPORT, viewport.data());
    glGetBooleanv(GL_DEPTH_WRITEMASK, &mask);
    glGetBooleanv(GL_COLOR_WRITEMASK, colors.data());
    for (unsigned i = 0; i < caps.size(); ++i)
      enables[i] = glIsEnabled(caps[i]);
    for (unsigned i = 0; i < textures.size(); ++i) {
      glActiveTexture(GL_TEXTURE0 + i);
      glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &textures[i]);
    }
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  }
  ~CaptureState() {
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(draw));
    glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(read));
    glBindRenderbuffer(GL_RENDERBUFFER, static_cast<GLuint>(rb));
    glBindVertexArray(static_cast<GLuint>(vao));
    glUseProgram(static_cast<GLuint>(program));
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, static_cast<GLuint>(pbo));
    glDepthFunc(static_cast<GLenum>(depth));
    glCullFace(static_cast<GLenum>(cull));
    glFrontFace(static_cast<GLenum>(front));
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glDepthMask(mask);
    glColorMask(colors[0], colors[1], colors[2], colors[3]);
    for (unsigned i = 0; i < caps.size(); ++i)
      enables[i] ? glEnable(caps[i]) : glDisable(caps[i]);
    for (unsigned i = 0; i < textures.size(); ++i) {
      glActiveTexture(GL_TEXTURE0 + i);
      glBindTexture(GL_TEXTURE_CUBE_MAP, static_cast<GLuint>(textures[i]));
    }
    glActiveTexture(static_cast<GLenum>(active));
    draw::invalidateGpuState();
  }
};
struct Timed {
  std::string name;
  GLuint first = 0, last = 0;
};
struct Measurement {
  std::size_t bytes = 0, samples = 0;
  double sum = 0, maximum = 0;
};
bool profiling = false;
std::vector<Timed> pending;
std::map<std::string, Measurement> measurements;
void collect() {
  for (auto it = pending.begin(); it != pending.end();) {
    GLint available = 0;
    glGetQueryObjectiv(it->last, GL_QUERY_RESULT_AVAILABLE, &available);
    if (!available) {
      ++it;
      continue;
    }
    GLuint64 a = 0, b = 0;
    glGetQueryObjectui64v(it->first, GL_QUERY_RESULT, &a);
    glGetQueryObjectui64v(it->last, GL_QUERY_RESULT, &b);
    auto &m = measurements[it->name];
    double ms = double(b - a) / 1e6;
    m.sum += ms;
    m.maximum = std::max(m.maximum, ms);
    ++m.samples;
    glDeleteQueries(1, &it->first);
    glDeleteQueries(1, &it->last);
    it = pending.erase(it);
  }
}
} // namespace
Ibl::Ibl(const std::filesystem::path &directory) : stream_(textureStream()) {
  unsigned i = 0;
  for (const auto *file :
       {"sky.ktx2", "irradiance.ktx2", "prefilter.ktx2", "brdf.ktx2"}) {
    maps_[i] = stream_->request(assets::readFile(directory / file),
                                i == 3 ? assets::Semantic::Brdf
                                       : assets::Semantic::Environment);
    ++i;
  }
}
void Ibl::pump() {
  stream_->pump();
  for (const auto &t : maps_)
    if (!t->error.empty())
      throw std::runtime_error(t->error);
}
bool Ibl::ready() const {
  return std::all_of(maps_.begin(), maps_.end(),
                     [](const auto &t) { return t->ready; });
}
GLuint Ibl::sky() const {
  return world::state().settings.enabled && world::state().sky
             ? world::state().sky
             : maps_[0]->id();
}
GLuint Ibl::irradiance() const {
  return world::state().settings.enabled && world::state().irradiance
             ? world::state().irradiance
             : maps_[1]->id();
}
GLuint Ibl::prefilter() const {
  return world::state().settings.enabled && world::state().prefilter
             ? world::state().prefilter
             : maps_[2]->id();
}
GLuint Ibl::brdf() const { return maps_[3]->id(); }
float Ibl::maxLod() const {
  return world::state().settings.enabled && world::state().prefilter
             ? world::state().environmentLod
             : float(maps_[2]->levels - 1);
}
std::size_t Ibl::bytes() const {
  std::size_t bytes = 0;
  std::set<const StreamedTexture *> seen;
  for (const auto &map : maps_)
    if (seen.insert(map.get()).second)
      bytes += map->residentBytes;
  return bytes;
}

struct ReflectionProbe::Impl {
  std::function<double()> clock;
  unsigned size;
  double interval, last = -1e30;
  GLuint raw = 0, filtered = 0, fbo = 0, depth = 0, vao = 0;
  std::unique_ptr<rhi::ShaderProgram> sky, convolve;
  std::size_t updates = 0;
  std::vector<double> times;
  struct Query {
    GLuint q[3]{};
  };
  std::vector<Query> pending;
  std::vector<double> captureMs, filterMs;
  // Resolution in texels and refresh interval in seconds have distinct units.
  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  Impl(const std::filesystem::path &path, unsigned dimension, double seconds,
       std::function<double()> clockSource)
      : clock(clockSource ? std::move(clockSource)
                          : std::function<double()>(now)),
        size(dimension), interval(seconds) {
    check(size >= 4 && size <= 128 && !(size & (size - 1)) && interval >= .25,
          "probe requires POT <=128 and interval >=250ms");
    CaptureState state;
    sky = std::make_unique<rhi::ShaderProgram>(path / "cube.vert",
                                               path / "probe_sky.frag");
    convolve = std::make_unique<rhi::ShaderProgram>(path / "cube.vert",
                                                    path / "probe_filter.frag");
    glGenVertexArrays(1, &vao);
    glGenFramebuffers(1, &fbo);
    glGenRenderbuffers(1, &depth);
    glBindRenderbuffer(GL_RENDERBUFFER, depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
                          static_cast<GLsizei>(size),
                          static_cast<GLsizei>(size));
    glGenTextures(1, &raw);
    glGenTextures(1, &filtered);
    for (GLuint texture : {raw, filtered}) {
      glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
      for (unsigned l = 0, w = size; l < assets::mipCount(size, size);
           ++l, w = std::max(1u, w / 2))
        for (unsigned face = 0; face < 6; ++face)
          glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                       static_cast<GLint>(l), GL_RGBA16F,
                       static_cast<GLsizei>(w), static_cast<GLsizei>(w), 0,
                       GL_RGBA, GL_HALF_FLOAT, nullptr);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                      GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      for (auto axis :
           {GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_TEXTURE_WRAP_R})
        glTexParameteri(GL_TEXTURE_CUBE_MAP, axis, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL,
                      static_cast<GLint>(assets::mipCount(size, size) - 1));
    }
  }
  ~Impl() {
    for (auto &q : pending)
      glDeleteQueries(3, q.q);
    glDeleteTextures(1, &raw);
    glDeleteTextures(1, &filtered);
    glDeleteFramebuffers(1, &fbo);
    glDeleteRenderbuffers(1, &depth);
    glDeleteVertexArrays(1, &vao);
  }
  void collect() {
    for (auto it = pending.begin(); it != pending.end();) {
      GLint ready = 0;
      glGetQueryObjectiv(it->q[2], GL_QUERY_RESULT_AVAILABLE, &ready);
      if (!ready) {
        ++it;
        continue;
      }
      GLuint64 a = 0, b = 0, c = 0;
      glGetQueryObjectui64v(it->q[0], GL_QUERY_RESULT, &a);
      glGetQueryObjectui64v(it->q[1], GL_QUERY_RESULT, &b);
      glGetQueryObjectui64v(it->q[2], GL_QUERY_RESULT, &c);
      captureMs.push_back(double(b - a) / 1e6);
      filterMs.push_back(double(c - b) / 1e6);
      glDeleteQueries(3, it->q);
      it = pending.erase(it);
    }
  }
};
ReflectionProbe::ReflectionProbe(const std::filesystem::path &shaders,
                                 unsigned size, double interval,
                                 std::function<double()> clock)
    : impl_(std::make_unique<Impl>(shaders, size, interval, std::move(clock))) {
}
ReflectionProbe::~ReflectionProbe() = default;
bool ReflectionProbe::update(const Ibl &env, glm::vec3 center,
                             const SceneCapture &scene, glm::mat3 skyRotation) {
  auto &p = *impl_;
  p.collect();
  double time = p.clock();
  if (!env.ready() || time - p.last < p.interval)
    return false;
  p.sky->poll();
  p.convolve->poll();
  CaptureState saved;
  bool previous = profiling;
  profiling = false;
  struct Restore {
    bool value;
    ~Restore() { profiling = value; }
  } restore{previous};
  p.last = time;
  p.times.push_back(time);
  ++p.updates;
  Impl::Query query;
  glGenQueries(3, query.q);
  glQueryCounter(query.q[0], GL_TIMESTAMP);
  glBindFramebuffer(GL_FRAMEBUFFER, p.fbo);
  glDisable(GL_SCISSOR_TEST);
  glDisable(GL_BLEND);
  glDisable(GL_FRAMEBUFFER_SRGB);
  glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, p.depth);
  glm::vec3 direction[6] = {{1, 0, 0},  {-1, 0, 0}, {0, 1, 0},
                            {0, -1, 0}, {0, 0, 1},  {0, 0, -1}},
            up[6] = {{0, -1, 0}, {0, -1, 0}, {0, 0, 1},
                     {0, 0, -1}, {0, -1, 0}, {0, -1, 0}};
  auto projection = glm::perspective(glm::radians(90.f), 1.f, .05f, 2000.f);
  for (unsigned face = 0; face < 6; ++face) {
    glViewport(0, 0, static_cast<GLsizei>(p.size),
               static_cast<GLsizei>(p.size));
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, p.raw, 0);
    check(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
          "probe framebuffer incomplete");
    glDepthMask(GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glBindVertexArray(p.vao);
    glUseProgram(p.sky->id());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, env.sky());
    glUniform1i(glGetUniformLocation(p.sky->id(), "uSky"), 0);
    glUniform1i(glGetUniformLocation(p.sky->id(), "uFace"),
                static_cast<GLint>(face));
    glUniform1f(glGetUniformLocation(p.sky->id(), "uSize"), float(p.size));
    glUniformMatrix3fv(glGetUniformLocation(p.sky->id(), "uSkyRotation"), 1,
                       GL_FALSE, glm::value_ptr(skyRotation));
    glDrawArrays(GL_TRIANGLES, 0, 3);
    if (scene) {
      glEnable(GL_DEPTH_TEST);
      glDepthFunc(GL_LESS);
      draw::invalidateGpuState();
      scene(projection, glm::lookAt(center, center + direction[face], up[face]),
            center);
    }
  }
  glQueryCounter(query.q[1], GL_TIMESTAMP);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, 0);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glDisable(GL_BLEND);
  glBindVertexArray(p.vao);
  glUseProgram(p.convolve->id());
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, p.raw);
  glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
  glUniform1i(glGetUniformLocation(p.convolve->id(), "uSource"), 0);
  unsigned levels = assets::mipCount(p.size, p.size);
  for (unsigned level = 0, w = p.size; level < levels;
       ++level, w = std::max(1u, w / 2)) {
    glViewport(0, 0, static_cast<GLsizei>(w), static_cast<GLsizei>(w));
    glUniform1f(glGetUniformLocation(p.convolve->id(), "uSize"), float(w));
    glUniform1f(glGetUniformLocation(p.convolve->id(), "uRoughness"),
                float(level) / float(levels - 1));
    for (unsigned face = 0; face < 6; ++face) {
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, p.filtered,
                             static_cast<GLint>(level));
      glUniform1i(glGetUniformLocation(p.convolve->id(), "uFace"),
                  static_cast<GLint>(face));
      glDrawArrays(GL_TRIANGLES, 0, 3);
    }
  }
  glQueryCounter(query.q[2], GL_TIMESTAMP);
  p.pending.push_back(query);
  return true;
}
GLuint ReflectionProbe::texture() const {
  return impl_->updates ? impl_->filtered : 0;
}
float ReflectionProbe::maxLod() const {
  return float(assets::mipCount(impl_->size, impl_->size) - 1);
}
std::size_t ReflectionProbe::updates() const { return impl_->updates; }
nlohmann::json ReflectionProbe::report() {
  auto &p = *impl_;
  p.collect();
  std::size_t texels = 0;
  for (unsigned n = p.size;; n = std::max(1u, n / 2)) {
    texels += std::size_t(n) * n;
    if (n == 1)
      break;
  }
  nlohmann::json j = {{"size", p.size},
                      {"faces", 6},
                      {"interval_seconds", p.interval},
                      {"updates", p.updates},
                      {"update_times", p.times},
                      {"capture_gpu_ms", p.captureMs},
                      {"prefilter_gpu_ms", p.filterMs},
                      {"texture_bytes", texels * 6 * 8 * 2},
                      {"depth_bytes", std::size_t(p.size) * p.size * 4},
                      {"resident_bytes",
                       texels * 6 * 8 * 2 + std::size_t(p.size) * p.size * 4}};
  return j;
}
void profileEnabled(bool value) { profiling = value; }
void materialMeasurement(const std::string &name, std::size_t bytes,
                         const std::function<void()> &draw) {
  if (!profiling || world::state().depthPass) {
    draw();
    return;
  }
  collect();
  auto &m = measurements[name];
  m.bytes = std::max(m.bytes, bytes);
  Timed t;
  t.name = name;
  glGenQueries(1, &t.first);
  glGenQueries(1, &t.last);
  glQueryCounter(t.first, GL_TIMESTAMP);
  try {
    draw();
  } catch (...) {
    glDeleteQueries(1, &t.first);
    glDeleteQueries(1, &t.last);
    throw;
  }
  glQueryCounter(t.last, GL_TIMESTAMP);
  pending.push_back(t);
}
nlohmann::json materialReport() {
  collect();
  nlohmann::json j = nlohmann::json::array();
  for (const auto &entry : measurements) {
    const auto &m = entry.second;
    j.push_back({{"name", entry.first},
                 {"resident_texture_bytes", m.bytes},
                 {"gpu_samples", m.samples},
                 {"gpu_ms_mean", m.samples ? m.sum / double(m.samples) : 0},
                 {"gpu_ms_max", m.maximum}});
  }
  return j;
}
void clearMeasurements() {
  for (auto &q : pending) {
    glDeleteQueries(1, &q.first);
    glDeleteQueries(1, &q.last);
  }
  pending.clear();
  measurements.clear();
}
} // namespace trackrs::pbr
