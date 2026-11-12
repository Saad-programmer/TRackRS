// SPDX-License-Identifier: GPL-2.0-or-later
#include "props_renderer.h"
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
const PropsRenderer *activeProps = nullptr;
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
/// LOD ranges scale with the object's size (radius) and the quality tier.
vegetation::LodRanges ranges(RenderQuality q, float radius) {
  const float s = q == RenderQuality::Low ? .6f : q == RenderQuality::Medium ? .8f : q == RenderQuality::High ? 1.f : 1.3f;
  const float k = s * std::clamp(radius / 1.5f, 1.f, 6.f);
  return {{35 * k, 110 * k, 260 * k}, 380 * k, .15f};
}
float hipHeight(PropKind kind) { return kind == PropKind::Camel ? 1.2f : 0.55f; }
} // namespace

struct PropsRenderer::Impl {
  std::unique_ptr<rhi::ShaderProgram> shader;
  std::unique_ptr<pbr::Ibl> environment;
  std::vector<Prop> props;
  std::function<float(float, float)> height;
  PlacementReport placement;
  struct Range {
    GLint baseVertex = 0;
    std::size_t firstIndex = 0, count = 0;
  };
  std::array<std::array<Range, PropLods>, PropKindCount> ranges;
  std::array<std::array<std::vector<vegetation::Instance>, PropLods>, PropKindCount> buckets;
  std::vector<vegetation::Instance> staging;
  GLuint vao = 0, vbo = 0, ibo = 0, instances = 0, white = 0, flat = 0, orm = 0;
  std::size_t capacity = 0, draws = 0, triangles = 0, visible = 0, meshBytes = 0;
  std::array<std::size_t, PropKindCount> counts{};
  std::array<Range, SignFaces> signRanges;
  std::array<std::vector<vegetation::Instance>, SignFaces> signBuckets;
  std::vector<std::uint8_t> signPixels;
  int signWidth = 0, signHeight = 0;
  GLuint atlas = 0;
  std::size_t signs = 0;
  ~Impl() {
    glDeleteVertexArrays(1, &vao);
    for (auto *b : {&vbo, &ibo, &instances})
      glDeleteBuffers(1, b);
    for (auto *t : {&white, &flat, &orm, &atlas})
      glDeleteTextures(1, t);
  }
};

PropsRenderer::PropsRenderer(const std::filesystem::path &data) : impl_(std::make_unique<Impl>()) {
  auto &p = *impl_;
  p.shader = std::make_unique<rhi::ShaderProgram>(
      data / "shaders/pbr/props.vert", data / "shaders/pbr/pbr.frag",
      [](GLuint id) { return glGetUniformLocation(id, "uViewProjection") >= 0; }, std::vector<std::string>{"VEGETATION"});
  p.environment = std::make_unique<pbr::Ibl>(data / "scenes/material_preview/ibl");
  std::vector<vegetation::Vertex> vertices;
  std::vector<std::uint16_t> indices;
  for (unsigned k = 0; k < PropKindCount; ++k)
    for (unsigned lod = 0; lod < PropLods; ++lod) {
      const auto mesh = buildProp(static_cast<PropKind>(k), lod);
      auto &r = p.ranges[k][lod];
      r.baseVertex = static_cast<GLint>(vertices.size());
      r.firstIndex = indices.size();
      r.count = mesh.indices.size();
      vertices.insert(vertices.end(), mesh.vertices.begin(), mesh.vertices.end());
      indices.insert(indices.end(), mesh.indices.begin(), mesh.indices.end());
    }
  for (unsigned f = 0; f < SignFaces; ++f) {
    const auto mesh = buildSignFace(f);
    auto &r = p.signRanges[f];
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
PropsRenderer::~PropsRenderer() {
  if (activeProps == this)
    activeProps = nullptr;
}
void PropsRenderer::setProps(std::vector<Prop> props, std::function<float(float, float)> height,
                             PlacementReport placement) {
  auto &p = *impl_;
  p.props = std::move(props);
  p.height = std::move(height);
  p.placement = placement;
  p.counts.fill(0);
  for (const auto &prop : p.props)
    ++p.counts[static_cast<std::size_t>(prop.kind)];
  activeProps = this;
}

void PropsRenderer::setSignAtlas(std::vector<std::uint8_t> rgba, int width, int height) {
  if (width <= 0 || height <= 0 || rgba.size() != static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4)
    throw std::invalid_argument("sign atlas must be RGBA8 width x height");
  auto &p = *impl_;
  p.signPixels = std::move(rgba);
  p.signWidth = width;
  p.signHeight = height;
}

std::size_t PropsRenderer::draw(glm::vec3 camera, const glm::mat4 &vp, vegetation::Pass pass, RenderQuality quality,
                                bool linearCapture, float time) {
  auto &p = *impl_;
  if (pass == vegetation::Pass::Probe || p.props.empty())
    return 0;
  if (pass == vegetation::Pass::Main)
    p.draws = p.triangles = p.visible = 0;
  for (auto &kind : p.buckets)
    for (auto &b : kind)
      b.clear();
  for (auto &b : p.signBuckets)
    b.clear();
  if (!p.signPixels.empty() && !p.atlas) {
    GLint previous = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous);
    glGenTextures(1, &p.atlas);
    glBindTexture(GL_TEXTURE_2D, p.atlas);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, p.signWidth, p.signHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 p.signPixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previous));
  }
  std::array<glm::vec4, 6> planes{};
  for (int i = 0; i < 3; ++i)
    for (int s = 0; s < 2; ++s) {
      glm::vec4 plane;
      for (int k = 0; k < 4; ++k)
        plane[k] = vp[k][3] + (s ? -1.f : 1.f) * vp[k][i];
      planes[static_cast<std::size_t>(i) * 2 + static_cast<std::size_t>(s)] = plane / glm::length(glm::vec3(plane));
    }
  const bool shadow = pass == vegetation::Pass::Shadow;
  for (const auto &prop : p.props) {
    const auto pose = animateProp(prop, time, p.height);
    const float radius = propRadius(prop.kind) * prop.scale;
    const glm::vec3 centre = pose.position + glm::vec3(0, 0, 0.5f * radius);
    bool inside = true;
    for (const auto &plane : planes)
      inside = inside && glm::dot(glm::vec3(plane), centre) + plane.w >= -radius;
    const float distance = glm::length(centre - camera);
    if (!inside || (shadow && distance > 250.f))
      continue;
    const auto choice = vegetation::chooseLod(distance, ranges(quality, radius), vegetation::Form::Bush);
    if (!choice.visible)
      continue;
    const auto kind = static_cast<std::size_t>(prop.kind);
    auto push = [&](unsigned lod, float fade) {
      if (lod >= PropLods)
        return; // beyond the last mesh LOD the prop fades out (no impostor)
      if (shadow)
        lod = std::max(lod, 1u);
      p.buckets[kind][lod].push_back({glm::vec4(pose.position, prop.scale),
                                      glm::vec4(pose.yaw, fade, static_cast<float>(prop.tint), pose.stride)});
    };
    // Sign text: full-detail faces only, never in shadow maps (the board casts).
    if (prop.face >= 0 && p.atlas && !shadow && choice.lod < 2)
      p.signBuckets[static_cast<std::size_t>(prop.face)].push_back(
          {glm::vec4(pose.position, prop.scale), glm::vec4(pose.yaw, 1.f, 0.f, 0.f)});
    if (choice.next == choice.lod)
      push(choice.lod, 1.f);
    else {
      push(choice.lod, choice.t + 2.f);
      push(choice.next, choice.t);
    }
  }
  draw::flush();
  p.staging.clear();
  std::array<std::array<std::size_t, PropLods>, PropKindCount> offsets{};
  for (std::size_t k = 0; k < PropKindCount; ++k)
    for (unsigned lod = 0; lod < PropLods; ++lod) {
      offsets[k][lod] = p.staging.size();
      p.staging.insert(p.staging.end(), p.buckets[k][lod].begin(), p.buckets[k][lod].end());
    }
  std::array<std::size_t, SignFaces> signOffsets{};
  for (std::size_t f = 0; f < SignFaces; ++f) {
    signOffsets[f] = p.staging.size();
    p.staging.insert(p.staging.end(), p.signBuckets[f].begin(), p.signBuckets[f].end());
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
  glUniform1f(uniform("uRoughnessFactor"), .9f);
  glUniform1f(uniform("uAOStrength"), 0);
  glUniform1f(uniform("uAlphaCutoff"), 0);
  glUniform1f(uniform("uTranslucency"), 0);
  glUniform1i(uniform("uAlbedoGamma"), 0);
  glUniform1f(uniform("uTime"), time);
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
  glDisable(GL_CULL_FACE); // thin walls, tent panels and boards are two-sided
  glDisable(GL_BLEND);
  std::size_t draws = 0, triangles = 0;
  for (std::size_t k = 0; k < PropKindCount; ++k) {
    const auto kind = static_cast<PropKind>(k);
    glUniform1i(uniform("uAnimal"), animated(kind) ? 1 : 0);
    glUniform1f(uniform("uHip"), hipHeight(kind));
    for (unsigned lod = 0; lod < PropLods; ++lod) {
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
  }
  // Sign faces: the same lit PBR path, albedo from the sign atlas.
  glUniform1i(uniform("uAnimal"), 0);
  glUniform1i(uniform("uSign"), 1);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, p.atlas);
  std::size_t signs = 0;
  for (std::size_t f = 0; f < SignFaces; ++f) {
    const auto count = p.signBuckets[f].size();
    if (!count)
      continue;
    const auto &range = p.signRanges[f];
    const auto offset = signOffsets[f] * sizeof(vegetation::Instance);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(vegetation::Instance),
                          bufferOffset(offset + offsetof(vegetation::Instance, a)));
    glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(vegetation::Instance),
                          bufferOffset(offset + offsetof(vegetation::Instance, b)));
    glDrawElementsInstancedBaseVertex(GL_TRIANGLES, static_cast<GLsizei>(range.count), GL_UNSIGNED_SHORT,
                                      bufferOffset(range.firstIndex * sizeof(std::uint16_t)),
                                      static_cast<GLsizei>(count), range.baseVertex);
    ++draws;
    triangles += range.count / 3 * count;
    signs += count;
  }
  glUniform1i(uniform("uSign"), 0);
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
    p.signs = signs;
    activeProps = this;
  }
  return draws;
}

nlohmann::json PropsRenderer::report() const {
  const auto &p = *impl_;
  nlohmann::json kinds = nlohmann::json::object();
  for (unsigned k = 0; k < PropKindCount; ++k)
    if (p.counts[k])
      kinds[std::string(propName(static_cast<PropKind>(k)))] = p.counts[k];
  return {{"props", p.props.size()},
          {"kinds", kinds},
          {"requested", p.placement.requested},
          {"rejected_road", p.placement.rejectedRoad},
          {"rejected_slope", p.placement.rejectedSlope},
          {"dressing_hash", p.placement.hash},
          {"draw_calls", p.draws},
          {"submitted_triangles", p.triangles},
          {"visible_instances", p.visible},
          {"mesh_bytes", p.meshBytes},
          {"sign_faces_visible", p.signs},
          {"sign_atlas", p.signPixels.empty() ? nlohmann::json(nullptr) : nlohmann::json::array({p.signWidth, p.signHeight})},
          {"instance_stream_bytes", p.capacity}};
}
void writePropsReport(const std::filesystem::path &path) {
  if (activeProps)
    std::ofstream(path) << activeProps->report().dump(2) << '\n';
}
} // namespace trackrs::dressing
