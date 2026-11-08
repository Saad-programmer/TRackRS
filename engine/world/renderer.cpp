// SPDX-License-Identifier: GPL-2.0-or-later
#include "renderer.h"
#include "engine/pbr/ibl.h"
#include "engine/render/basic.h"
#include "engine/rhi/rhi.h"
#include "legacy_metrics.h"
#include "validation.h"
#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <memory>
#include <stdexcept>
namespace trackrs::world {
namespace {
struct Guard {
  GLint draw = 0, read = 0, program = 0, vao = 0, depth = 0, active = 0;
  std::array<GLint, 4> viewport{};
  std::array<GLboolean, 4> colour{};
  GLboolean mask = 0, depthTest = 0, cull = 0, blend = 0, scissor = 0, srgb = 0;
  Guard() {
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read);
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
    glGetIntegerv(GL_VIEWPORT, viewport.data());
    glGetIntegerv(GL_DEPTH_FUNC, &depth);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
    glGetBooleanv(GL_COLOR_WRITEMASK, colour.data());
    glGetBooleanv(GL_DEPTH_WRITEMASK, &mask);
    depthTest = glIsEnabled(GL_DEPTH_TEST);
    cull = glIsEnabled(GL_CULL_FACE);
    blend = glIsEnabled(GL_BLEND);
    scissor = glIsEnabled(GL_SCISSOR_TEST);
    srgb = glIsEnabled(GL_FRAMEBUFFER_SRGB);
  }
  ~Guard() {
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(draw));
    glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(read));
    glUseProgram(static_cast<GLuint>(program));
    glBindVertexArray(static_cast<GLuint>(vao));
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glDepthFunc(static_cast<GLenum>(depth));
    glDepthMask(mask);
    glColorMask(colour[0], colour[1], colour[2], colour[3]);
    if (depthTest)
      glEnable(GL_DEPTH_TEST);
    else
      glDisable(GL_DEPTH_TEST);
    if (cull)
      glEnable(GL_CULL_FACE);
    else
      glDisable(GL_CULL_FACE);
    if (blend)
      glEnable(GL_BLEND);
    else
      glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_FRAMEBUFFER_SRGB);
    if (scissor)
      glEnable(GL_SCISSOR_TEST);
    else
      glDisable(GL_SCISSOR_TEST);
    if (srgb)
      glEnable(GL_FRAMEBUFFER_SRGB);
    else
      glDisable(GL_FRAMEBUFFER_SRGB);
    glActiveTexture(static_cast<GLenum>(active));
    draw::invalidateGpuState();
  }
};
struct Renderer {
  rhi::ShaderProgram sky, irradiance;
  pbr::Ibl base;
  std::unique_ptr<pbr::ReflectionProbe> environment;
  GLuint vao = 0, fbo = 0, diffuse = 0, shadow = 0;
  unsigned shadowSize = 0, shadowCount = 0;
  float bakedHour = -1, bakedOvercast = -1;
  std::filesystem::path shaders;
  explicit Renderer(const std::filesystem::path &data)
      : sky(data / "shaders/world/sky.vert", data / "shaders/world/sky.frag",
            [](GLuint p) {
              return glGetUniformLocation(p, "uSunDirection") >= 0;
            }),
        irradiance(
            data / "shaders/pbr/cube.vert",
            data / "shaders/world/irradiance.frag",
            [](GLuint p) { return glGetUniformLocation(p, "uFace") >= 0; }),
        base(data / "scenes/material_preview/ibl"),
        shaders(data / "shaders/pbr") {
    glGenVertexArrays(1, &vao);
    glGenFramebuffers(1, &fbo);
    glGenTextures(1, &diffuse);
    glBindTexture(GL_TEXTURE_CUBE_MAP, diffuse);
    for (unsigned face = 0; face < 6; ++face)
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGBA16F, 8, 8,
                   0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    for (auto axis : {GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_TEXTURE_WRAP_R})
      glTexParameteri(GL_TEXTURE_CUBE_MAP, static_cast<GLenum>(axis),
                      GL_CLAMP_TO_EDGE);
  }
  ~Renderer() {
    glDeleteTextures(1, &diffuse);
    glDeleteTextures(1, &shadow);
    glDeleteFramebuffers(1, &fbo);
    glDeleteVertexArrays(1, &vao);
  }
};
std::unique_ptr<Renderer> renderer;
void solar(GLuint p) {
  auto &s = state();
  glUniform3fv(glGetUniformLocation(p, "uSunDirection"), 1,
               glm::value_ptr(s.sun.direction));
  glUniform3fv(glGetUniformLocation(p, "uSunRadiance"), 1,
               glm::value_ptr(s.sun.radiance));
  glUniform1f(glGetUniformLocation(p, "uDaylight"), s.sun.daylight);
  glUniform3fv(glGetUniformLocation(p, "uSunSkyRadiance"), 1,
               glm::value_ptr(s.sun.skyRadiance));
  glUniform1f(glGetUniformLocation(p, "uOvercast"), s.settings.weather.overcast);
}
} // namespace
void drawSky(const glm::mat4 &projection, const glm::mat4 &view,
             glm::vec3 camera, bool linear) {
  if (!state().settings.enabled || !renderer)
    return;
  draw::flush();
  Guard guard;
  auto &r = *renderer;
  r.sky.poll();
  auto p = r.sky.id();
  glUseProgram(p);
  glBindVertexArray(r.vao);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glDepthMask(GL_FALSE);
  glDisable(GL_CULL_FACE);
  glDisable(GL_BLEND);
  glDisable(GL_SCISSOR_TEST);
  glDisable(GL_FRAMEBUFFER_SRGB);
  auto inverse = glm::inverse(projection * view);
  glUniformMatrix4fv(glGetUniformLocation(p, "uInverseViewProjection"), 1,
                     GL_FALSE, glm::value_ptr(inverse));
  glUniform3fv(glGetUniformLocation(p, "uCamera"), 1, glm::value_ptr(camera));
  glUniform1i(glGetUniformLocation(p, "uLinear"), linear);
  solar(p);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  if (validation::run.capture) {
    ++validation::run.frameDraws;
    ++validation::run.frameTriangles;
  }
}
void prepare(const glm::mat4 &view, const glm::mat4 &projection,
             RenderQuality quality, const std::filesystem::path &data) {
  auto &s = state();
  s.scene = s.settings.enabled;
  s.blobDraws = 0;
  s.lightCount = 0; // the game adds this frame's lights after prepare()
  s.shadowDraws.fill(0);
  s.shadowTriangles.fill(0);
  s.view = view;
  s.shadowView = view;
  if (!s.settings.enabled)
    return;
  draw::flush();
  Guard guard;
  if (!renderer)
    renderer = std::make_unique<Renderer>(data);
  auto &r = *renderer;
  if (r.sky.poll() | r.irradiance.poll())
    r.bakedHour = -1;
  r.base.pump();
  if (r.base.ready() && (r.bakedHour != s.settings.hour ||
                         r.bakedOvercast != s.settings.weather.overcast)) {
    validation::PassScope timing("sky_ibl_bake");
    s.sky = 0;
    s.irradiance = 0;
    s.prefilter = 0;
    r.environment = std::make_unique<pbr::ReflectionProbe>(r.shaders, 64, .5);
    r.environment->update(r.base, glm::vec3(0),
                          [](const glm::mat4 &p, const glm::mat4 &v,
                             const glm::vec3 &c) { drawSky(p, v, c, true); });
    glBindFramebuffer(GL_FRAMEBUFFER, r.fbo);
    glViewport(0, 0, 8, 8);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_FRAMEBUFFER_SRGB);
    glDisable(GL_CULL_FACE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glBindVertexArray(r.vao);
    glUseProgram(r.irradiance.id());
    solar(r.irradiance.id());
    for (unsigned face = 0; face < 6; ++face) {
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, r.diffuse,
                             0);
      glUniform1i(glGetUniformLocation(r.irradiance.id(), "uFace"),
                  static_cast<int>(face));
      glDrawArrays(GL_TRIANGLES, 0, 3);
      if (validation::run.capture) {
        ++validation::run.frameDraws;
        ++validation::run.frameTriangles;
      }
    }
    s.sky = r.environment->texture();
    s.prefilter = s.sky;
    s.irradiance = r.diffuse;
    s.environmentLod = r.environment->maxLod();
    r.bakedHour = s.settings.hour;
    r.bakedOvercast = s.settings.weather.overcast;
  }
  const bool high =
      quality == RenderQuality::High || quality == RenderQuality::Ultra;
  auto count = high ? 4u : 3u, size = high ? 2048u : 1024u;
  s.cascades = makeCascades({view, projection}, s.sun.direction, count, size,
                            high ? 768.f : 512.f);
  if (s.settings.csm && (r.shadowSize != size || r.shadowCount != count)) {
    glDeleteTextures(1, &r.shadow);
    glGenTextures(1, &r.shadow);
    glBindTexture(GL_TEXTURE_2D_ARRAY, r.shadow);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT24,
                 static_cast<GLsizei>(size), static_cast<GLsizei>(size),
                 static_cast<GLsizei>(count), 0, GL_DEPTH_COMPONENT,
                 GL_UNSIGNED_INT, nullptr);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE,
                    GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float border[] = {1, 1, 1, 1};
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, border);
    r.shadowSize = size;
    r.shadowCount = count;
  }
  s.shadow = s.settings.csm ? r.shadow : 0;
}
void renderShadows(const std::function<void(const glm::mat4 &)> &casters) {
  auto &s = state();
  if (!s.settings.enabled || !s.settings.csm || !renderer)
    return;
  draw::flush();
  Guard guard;
  auto &r = *renderer;
  glActiveTexture(GL_TEXTURE10);
  glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
  s.depthPass = true;
  try {
    glBindFramebuffer(GL_FRAMEBUFFER, r.fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_CUBE_MAP_POSITIVE_X, r.diffuse, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           0, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glViewport(0, 0, static_cast<GLsizei>(r.shadowSize),
               static_cast<GLsizei>(r.shadowSize));
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_FRAMEBUFFER_SRGB);
    for (unsigned i = 0; i < s.cascades.count; ++i) {
      const auto name = "shadow_cascade_" + std::to_string(i);
      validation::PassScope timing(name.c_str());
      glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, r.shadow,
                                0, static_cast<GLint>(i));
      if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("CSM framebuffer incomplete");
      glClear(GL_DEPTH_BUFFER_BIT);
      const auto beforeDraws = validation::run.frameDraws,
                 beforeTriangles = validation::run.frameTriangles;
      casters(s.cascades.matrices[i]);
      draw::flush();
      s.shadowDraws[i] = validation::run.frameDraws - beforeDraws;
      s.shadowTriangles[i] = validation::run.frameTriangles - beforeTriangles;
    }
  } catch (...) {
    s.depthPass = false;
    throw;
  }
  s.depthPass = false;
  // This FBO is shared with diffuse baking; restore its colour routing.
  glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, 0, 0);
  glDrawBuffer(GL_COLOR_ATTACHMENT0);
  glReadBuffer(GL_COLOR_ATTACHMENT0);
}
void shutdown() {
  renderer.reset();
  auto settings = state().settings;
  auto name = state().weatherName;
  auto level = state().levelWeather;
  state() = State{};
  state().weatherName = name;
  state().levelWeather = level;
  configure(settings);
}
void writeReport(const std::filesystem::path &path) {
  if (state().settings.enabled)
    std::ofstream(path) << report().dump(2) << '\n';
}
nlohmann::json report() {
  auto &s = state();
  return {
      {"hour", s.settings.hour},
      {"weather",
       {{"selection", s.weatherName},
        {"wetness", s.settings.weather.wetness},
        {"puddles", s.settings.weather.puddles},
        {"snow", s.settings.weather.snow},
        {"overcast", s.settings.weather.overcast},
        {"fog_density", s.settings.weather.fogDensity}}},
      {"sun_direction",
       {s.sun.direction.x, s.sun.direction.y, s.sun.direction.z}},
      {"sky_source_radiance",
       {s.sun.skyRadiance.x, s.sun.skyRadiance.y, s.sun.skyRadiance.z}},
      {"sun_radiance", {s.sun.radiance.x, s.sun.radiance.y, s.sun.radiance.z}},
      {"ibl_ready", s.sky != 0},
      {"csm", s.settings.csm},
      {"cascade_count", s.cascades.count},
      {"shadow_resolution", s.cascades.resolution},
      {"shadow_payload_bytes", renderer && renderer->shadow
                                   ? std::uint64_t(renderer->shadowCount) *
                                         renderer->shadowSize *
                                         renderer->shadowSize * 3
                                   : 0},
      {"active_cascade_count", s.settings.csm ? s.cascades.count : 0},
      {"scene_lights", s.lightCount},
      {"depth_pass", s.depthPass},
      {"blob_draw_calls", s.blobDraws},
      {"cascade_draw_calls", s.shadowDraws},
      {"cascade_triangles", s.shadowTriangles},
      {"environment_probe", renderer && renderer->environment
                                ? renderer->environment->report()
                                : nlohmann::json(nullptr)}};
}
} // namespace trackrs::world
