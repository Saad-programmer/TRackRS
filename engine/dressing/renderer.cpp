// SPDX-License-Identifier: GPL-2.0-or-later
#include "renderer.h"
#include "engine/pbr/ibl.h"
#include "engine/render/basic.h"
#include "engine/rhi/rhi.h"
#include "engine/world/state.h"
#include "validation.h"
#include <array>
#include <fstream>
#include <glm/gtc/type_ptr.hpp>
namespace trackrs::dressing {
namespace {
Settings current;
float frameTime = 0;
std::vector<glm::vec3> frameCars;
const Renderer *activeRenderer = nullptr;
const void *bufferOffset(std::size_t bytes) {
  // GL buffer-object offsets are passed through the pointer parameter.
  // NOLINTNEXTLINE(performance-no-int-to-ptr)
  return reinterpret_cast<const void *>(bytes);
}
GLuint pixel(GLenum internal, std::array<std::uint8_t, 4> rgba) {
  GLuint id = 0;
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internal), 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  return id;
}
// Distances (m): LOD0 / LOD1 / LOD2 end, cross-fade band 15 %.
vegetation::LodRanges ranges(RenderQuality q) {
  const float s = q == RenderQuality::Low ? .6f : q == RenderQuality::Medium ? .8f : q == RenderQuality::High ? 1.f : 1.3f;
  return {{25 * s, 70 * s, 140 * s}, 200 * s, .15f};
}
} // namespace

void configure(Settings settings) { current = settings; }
Settings settings() { return current; }
void setFrame(float time, const std::vector<glm::vec3> &cars) {
  frameTime = time;
  frameCars.assign(cars.begin(), cars.end());
}

struct Renderer::Impl {
  std::unique_ptr<rhi::ShaderProgram> shader;
  std::unique_ptr<pbr::Ibl> environment;
  std::vector<Person> people;
  std::array<std::array<vegetation::Mesh, PersonLods>, KindCount> meshes;
  struct Range {
    GLint baseVertex = 0;
    std::size_t firstIndex = 0, count = 0;
  };
  std::array<std::array<Range, PersonLods>, KindCount> ranges;
  std::array<std::array<std::vector<vegetation::Instance>, PersonLods>, KindCount> buckets;
  std::vector<vegetation::Instance> staging;
  GLuint vao = 0, vbo = 0, ibo = 0, instances = 0, white = 0, flat = 0, orm = 0;
  std::size_t capacity = 0, draws = 0, triangles = 0, visible = 0, meshBytes = 0;
  std::array<std::size_t, KindCount> counts{};
  ~Impl() {
    glDeleteVertexArrays(1, &vao);
    for (auto *b : {&vbo, &ibo, &instances})
      glDeleteBuffers(1, b);
    for (auto *t : {&white, &flat, &orm})
      glDeleteTextures(1, t);
  }
};

Renderer::Renderer(const std::filesystem::path &data) : impl_(std::make_unique<Impl>()) {
  auto &p = *impl_;
  p.shader = std::make_unique<rhi::ShaderProgram>(
      data / "shaders/pbr/people.vert", data / "shaders/pbr/pbr.frag",
      [](GLuint id) { return glGetUniformLocation(id, "uViewProjection") >= 0; }, std::vector<std::string>{"VEGETATION"});
  p.environment = std::make_unique<pbr::Ibl>(data / "scenes/material_preview/ibl");
  std::vector<vegetation::Vertex> vertices;
  std::vector<std::uint16_t> indices;
  for (unsigned k = 0; k < KindCount; ++k)
    for (unsigned lod = 0; lod < PersonLods; ++lod) {
      const auto mesh = buildFigure(static_cast<Kind>(k), lod);
      auto &r = p.ranges[k][lod];
      r.baseVertex = static_cast<GLint>(vertices.size());
      r.firstIndex = indices.size();
      r.count = mesh.indices.size();
      vertices.insert(vertices.end(), mesh.vertices.begin(), mesh.vertices.end());
      indices.insert(indices.end(), mesh.indices.begin(), mesh.indices.end());
    }
  p.meshBytes = vertices.size() * sizeof(vegetation::Vertex) + indices.size() * sizeof(std::uint16_t);
  GLint previousVao = 0, previousBuffer = 0, previousTexture = 0;
  glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVao);
  glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousBuffer);
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
  glGenVertexArrays(1, &p.vao);
  glGenBuffers(1, &p.vbo);
  glGenBuffers(1, &p.ibo);
  glGenBuffers(1, &p.instances);
  glBindVertexArray(p.vao);
  glBindBuffer(GL_ARRAY_BUFFER, p.vbo);
  glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(vegetation::Vertex)), vertices.data(),
               GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, p.ibo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(std::uint16_t)), indices.data(),
               GL_STATIC_DRAW);
  const auto stride = static_cast<GLsizei>(sizeof(vegetation::Vertex));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, bufferOffset(offsetof(vegetation::Vertex, position)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 4, GL_INT_2_10_10_10_REV, GL_TRUE, stride, bufferOffset(offsetof(vegetation::Vertex, normal)));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_UNSIGNED_SHORT, GL_TRUE, stride, bufferOffset(offsetof(vegetation::Vertex, uv)));
  glEnableVertexAttribArray(5);
  glVertexAttribPointer(5, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, bufferOffset(offsetof(vegetation::Vertex, color)));
  glBindBuffer(GL_ARRAY_BUFFER, p.instances);
  for (GLuint location : {6u, 7u}) {
    glEnableVertexAttribArray(location);
    glVertexAttribDivisor(location, 1);
  }
  glBindVertexArray(static_cast<GLuint>(previousVao));
  glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(previousBuffer));
  p.white = pixel(GL_SRGB8_ALPHA8, {255, 255, 255, 255});
  p.flat = pixel(GL_RGBA8, {128, 128, 255, 255});
  p.orm = pixel(GL_RGBA8, {255, 255, 255, 255});
  glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
}
Renderer::~Renderer() {
  if (activeRenderer == this)
    activeRenderer = nullptr;
}
void Renderer::setPeople(std::vector<Person> people) {
  auto &p = *impl_;
  p.people = std::move(people);
  p.counts.fill(0);
  for (const auto &person : p.people)
    ++p.counts[static_cast<std::size_t>(person.kind)];
}

std::size_t Renderer::draw(glm::vec3 camera, const glm::mat4 &vp, vegetation::Pass pass, RenderQuality quality,
                           bool linearCapture) {
  auto &p = *impl_;
  if (pass == vegetation::Pass::Probe || p.people.empty())
    return 0;
  if (pass == vegetation::Pass::Main) { // report even when nobody is in view
    activeRenderer = this;
    p.draws = p.triangles = p.visible = 0;
  }
  for (auto &kind : p.buckets)
    for (auto &b : kind)
      b.clear();
  // Frustum planes (normalised) for sphere culling.
  std::array<glm::vec4, 6> planes{};
  for (int i = 0; i < 3; ++i)
    for (int s = 0; s < 2; ++s) {
      glm::vec4 plane;
      for (int k = 0; k < 4; ++k)
        plane[k] = vp[k][3] + (s ? -1.f : 1.f) * vp[k][i];
      planes[static_cast<std::size_t>(i) * 2 + static_cast<std::size_t>(s)] = plane / glm::length(glm::vec3(plane));
    }
  const auto r = ranges(quality);
  const bool shadow = pass == vegetation::Pass::Shadow;
  for (const auto &person : p.people) {
    glm::vec3 nearest(0);
    float best = 1e30f;
    for (const auto &car : frameCars) {
      const float d = glm::length(glm::vec2(car - person.position));
      if (d < best) {
        best = d;
        nearest = car;
      }
    }
    const auto a = animate(person, nearest, !frameCars.empty());
    const float h = person.height;
    const glm::vec3 centre = a.position + glm::vec3(0, 0, .5f * h);
    bool inside = true;
    for (const auto &plane : planes)
      inside = inside && glm::dot(glm::vec3(plane), centre) + plane.w >= -h;
    const float distance = glm::length(centre - camera);
    if (!inside || (shadow && distance > 120.f))
      continue;
    auto choice = vegetation::chooseLod(distance, r, vegetation::Form::Bush);
    if (!choice.visible)
      continue;
    const auto kind = static_cast<std::size_t>(person.kind);
    const float code = static_cast<float>(person.palette) + 8.f * static_cast<float>(person.pose);
    auto push = [&](unsigned lod, float fade) {
      // LOD3 (impostor) does not exist for people: the far mesh fades out.
      if (lod >= PersonLods)
        return;
      if (shadow)
        lod = std::max(lod, 1u);
      p.buckets[kind][lod].push_back({glm::vec4(a.position, h), glm::vec4(a.yaw, fade, code, a.excitement)});
    };
    if (choice.next == choice.lod)
      push(choice.lod, 1.f);
    else {
      push(choice.lod, choice.t + 2.f);
      push(choice.next, choice.t);
    }
  }
  draw::flush();
  p.staging.clear();
  std::array<std::array<std::size_t, PersonLods>, KindCount> offsets{};
  for (std::size_t k = 0; k < KindCount; ++k)
    for (unsigned lod = 0; lod < PersonLods; ++lod) {
      offsets[k][lod] = p.staging.size();
      p.staging.insert(p.staging.end(), p.buckets[k][lod].begin(), p.buckets[k][lod].end());
    }
  if (p.staging.empty())
    return 0;
  GLint program = 0, vao = 0, buffer = 0, active = 0;
  glGetIntegerv(GL_CURRENT_PROGRAM, &program);
  glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
  glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &buffer);
  glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
  const GLboolean cull = glIsEnabled(GL_CULL_FACE), blend = glIsEnabled(GL_BLEND);
  glBindVertexArray(p.vao);
  glBindBuffer(GL_ARRAY_BUFFER, p.instances);
  const auto bytes = p.staging.size() * sizeof(vegetation::Instance);
  if (bytes > p.capacity)
    p.capacity = std::max<std::size_t>(bytes * 3 / 2, std::size_t{16} * 1024);
  glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(p.capacity), nullptr, GL_STREAM_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(bytes), p.staging.data());
  p.shader->poll();
  p.environment->pump();
  const GLuint id = p.shader->id();
  glUseProgram(id);
  world::apply(id, glm::mat4(1), world::state().scene);
  auto uniform = [&](const char *name) { return glGetUniformLocation(id, name); };
  const auto &ws = world::state();
  const bool lit = ws.settings.enabled;
  glUniformMatrix4fv(uniform("uViewProjection"), 1, GL_FALSE, glm::value_ptr(vp));
  const glm::vec3 eye = lit ? glm::vec3(glm::inverse(ws.view)[3]) : camera;
  glUniform3fv(uniform("uCamPos"), 1, glm::value_ptr(eye));
  const glm::vec3 sun = lit ? ws.sun.direction : glm::normalize(glm::vec3(.2f, .5f, 1));
  const glm::vec3 sunColour = lit ? ws.sun.radiance : glm::vec3(3), ambient = lit ? ws.sun.ambient : glm::vec3(.08f);
  glUniform3fv(uniform("uSunDir"), 1, glm::value_ptr(sun));
  glUniform3fv(uniform("uSunColor"), 1, glm::value_ptr(sunColour));
  glUniform3fv(uniform("uAmbientColor"), 1, glm::value_ptr(ambient));
  glUniform1i(uniform("uPointLightCount"), 0);
  glUniform1i(uniform("uUseLocalProbe"), 0);
  glUniform1i(uniform("uToneMap"), linearCapture ? 0 : 1);
  glUniform1f(uniform("uMetallicFactor"), 0);
  glUniform1f(uniform("uRoughnessFactor"), .85f);
  glUniform1f(uniform("uAOStrength"), 0);
  glUniform1f(uniform("uAlphaCutoff"), 0);
  glUniform1f(uniform("uTranslucency"), 0);
  glUniform1i(uniform("uAlbedoGamma"), 0);
  glUniform1f(uniform("uTime"), frameTime);
  for (auto [name, unit] : {std::pair{"uAlbedoMap", 0}, {"uNormalMap", 1}, {"uMetalRoughMap", 2}, {"uIrradianceMap", 3},
                            {"uPrefilterMap", 4}, {"uBrdfLUT", 5}, {"uLocalProbe", 7}})
    glUniform1i(uniform(name), unit);
  const bool ibl = p.environment->ready();
  glUniform1i(uniform("uHasIBL"), ibl ? 1 : 0);
  if (ibl) {
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_CUBE_MAP, p.environment->irradiance());
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_CUBE_MAP, p.environment->prefilter());
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, p.environment->brdf());
    glUniform1f(uniform("uPrefilterMaxLod"), p.environment->maxLod());
    glUniformMatrix3fv(uniform("uEnvironmentRotation"), 1, GL_FALSE, glm::value_ptr(glm::mat3(1)));
  }
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, p.white);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, p.flat);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, p.orm);
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  glDisable(GL_CULL_FACE);
  glDisable(GL_BLEND);
  std::size_t draws = 0, triangles = 0;
  for (std::size_t k = 0; k < KindCount; ++k)
    for (unsigned lod = 0; lod < PersonLods; ++lod) {
      const auto count = p.buckets[k][lod].size();
      const auto &range = p.ranges[k][lod];
      if (!count || !range.count)
        continue;
      const auto offset = offsets[k][lod] * sizeof(vegetation::Instance);
      glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(vegetation::Instance),
                            bufferOffset(offset + offsetof(vegetation::Instance, a)));
      glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(vegetation::Instance),
                            bufferOffset(offset + offsetof(vegetation::Instance, b)));
      glDrawElementsInstancedBaseVertex(GL_TRIANGLES, static_cast<GLsizei>(range.count), GL_UNSIGNED_SHORT,
                                        bufferOffset(range.firstIndex * sizeof(std::uint16_t)),
                                        static_cast<GLsizei>(count), range.baseVertex);
      ++draws;
      triangles += range.count / 3 * count;
    }
  for (int unit = 5; unit >= 0; --unit) {
    glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(unit));
    glBindTexture(unit >= 3 && unit <= 4 ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, 0);
  }
  glActiveTexture(static_cast<GLenum>(active));
  glUseProgram(static_cast<GLuint>(program));
  glBindVertexArray(static_cast<GLuint>(vao));
  glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(buffer));
  cull ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
  blend ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
  draw::invalidateGpuState();
  if (validation::run.capture) {
    validation::run.frameDraws += draws;
    validation::run.frameTriangles += triangles;
  }
  if (pass == vegetation::Pass::Main) {
    p.draws = draws;
    p.triangles = triangles;
    p.visible = p.staging.size();
    activeRenderer = this;
  }
  return draws;
}

nlohmann::json Renderer::report() const {
  const auto &p = *impl_;
  return {{"people", p.people.size()},
          {"spectators", p.counts[0]},
          {"camera_operators", p.counts[1]},
          {"tripod_cameras", p.counts[2]},
          {"reporters", p.counts[3]},
          {"draw_calls", p.draws},
          {"submitted_triangles", p.triangles},
          {"visible_instances", p.visible},
          {"mesh_bytes", p.meshBytes},
          {"instance_stream_bytes", p.capacity}};
}
void writeReport(const std::filesystem::path &path) {
  if (activeRenderer)
    std::ofstream(path) << activeRenderer->report().dump(2) << '\n';
}
} // namespace trackrs::dressing
