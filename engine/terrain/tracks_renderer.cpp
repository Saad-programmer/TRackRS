// SPDX-License-Identifier: GPL-2.0-or-later
#include "tracks_renderer.h"
#include "engine/rhi/rhi.h"
#include "engine/world/state.h"
#include "validation.h"
#include <glm/gtc/type_ptr.hpp>
#include <vector>
namespace trackrs::terrain {
namespace {
struct Vertex {
  glm::vec3 position;
  glm::vec2 side;
  glm::vec2 uv;      // across (half-widths), along (metres)
  glm::vec3 profile; // depth, lip, darken
  float kind, halfWidth;
};
const void *offset(std::size_t bytes) {
  // NOLINTNEXTLINE(performance-no-int-to-ptr)
  return reinterpret_cast<const void *>(bytes);
}
float luminance(glm::vec3 c) { return glm::dot(c, glm::vec3(.2126f, .7152f, .0722f)); }
} // namespace

struct TracksRenderer::Impl {
  std::unique_ptr<rhi::ShaderProgram> shader;
  std::function<float(float, float)> height;
  GLuint vao = 0, vbo = 0, ibo = 0;
  std::size_t ringCapacity = 0, staticUploaded = 0;
  std::uint64_t uploaded = 0; // ring segments written when last uploaded
  const Tracks *source = nullptr;
  std::vector<Vertex> scratch;
  ~Impl() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ibo);
  }
  void quad(const TrackSegment &s, Vertex *out) const {
    // Tyre ribbons reach past the rut to carry the lips.
    const float extent = s.kind == TrackKind::Tyre ? 1.7f : 1.f;
    const TrackPoint *ends[2] = {&s.a, &s.b};
    for (int e = 0; e < 2; ++e)
      for (int k = 0; k < 2; ++k) {
        const float across = k ? extent : -extent;
        const glm::vec2 xy = glm::vec2(ends[e]->position) + ends[e]->side * (across * s.halfWidth);
        auto &v = out[e * 2 + k];
        v.position = {xy, height(xy.x, xy.y) + .02f};
        v.side = ends[e]->side;
        v.uv = {across, ends[e]->along};
        v.profile = {s.profile.depth, s.profile.lip, s.profile.darken};
        v.kind = static_cast<float>(s.kind);
        v.halfWidth = s.halfWidth;
      }
  }
  void allocate(std::size_t ring, std::size_t statics) {
    ringCapacity = ring;
    const std::size_t total = ring + statics;
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(total * 4 * sizeof(Vertex)), nullptr, GL_DYNAMIC_DRAW);
    std::vector<std::uint32_t> indices(total * 6);
    for (std::size_t q = 0; q < total; ++q) {
      const auto b = static_cast<std::uint32_t>(q * 4);
      const std::uint32_t pattern[6] = {b, b + 1, b + 2, b + 2, b + 1, b + 3};
      std::copy(pattern, pattern + 6, indices.begin() + static_cast<std::ptrdiff_t>(q * 6));
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * 4), indices.data(), GL_STATIC_DRAW);
    uploaded = 0;
    staticUploaded = 0;
  }
  void upload(const Tracks &t) {
    const auto &ring = t.segments();
    const auto &statics = t.staticSegments();
    if (source != &t || ring.size() != ringCapacity || statics.size() != staticUploaded || t.written() < uploaded) {
      source = &t;
      allocate(ring.size(), statics.size());
      scratch.resize(statics.size() * 4);
      for (std::size_t i = 0; i < statics.size(); ++i)
        quad(statics[i], &scratch[i * 4]);
      glBindBuffer(GL_ARRAY_BUFFER, vbo);
      if (!statics.empty())
        glBufferSubData(GL_ARRAY_BUFFER, static_cast<GLintptr>(ring.size() * 4 * sizeof(Vertex)),
                        static_cast<GLsizeiptr>(scratch.size() * sizeof(Vertex)), scratch.data());
      staticUploaded = statics.size();
    }
    if (ring.empty() || t.written() == uploaded)
      return;
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    // Only the newest ringCapacity segments can be live.
    std::uint64_t first = std::max<std::uint64_t>(uploaded, t.written() > ring.size() ? t.written() - ring.size() : 0);
    while (first < t.written()) {
      const std::size_t slot = first % ring.size();
      const std::size_t run =
          static_cast<std::size_t>(std::min<std::uint64_t>(t.written() - first, ring.size() - slot));
      scratch.resize(run * 4);
      for (std::size_t i = 0; i < run; ++i)
        quad(ring[slot + i], &scratch[i * 4]);
      glBufferSubData(GL_ARRAY_BUFFER, static_cast<GLintptr>(slot * 4 * sizeof(Vertex)),
                      static_cast<GLsizeiptr>(run * 4 * sizeof(Vertex)), scratch.data());
      first += run;
    }
    uploaded = t.written();
  }
};

TracksRenderer::TracksRenderer(const std::filesystem::path &data, std::function<float(float, float)> height)
    : impl_(std::make_unique<Impl>()) {
  auto &p = *impl_;
  p.height = std::move(height);
  p.shader = std::make_unique<rhi::ShaderProgram>(
      data / "shaders/world/tracks.vert", data / "shaders/world/tracks.frag",
      [](GLuint id) { return glGetUniformLocation(id, "uViewProjection") >= 0; });
  glGenVertexArrays(1, &p.vao);
  glGenBuffers(1, &p.vbo);
  glGenBuffers(1, &p.ibo);
  glBindVertexArray(p.vao);
  glBindBuffer(GL_ARRAY_BUFFER, p.vbo);
  constexpr auto stride = static_cast<GLsizei>(sizeof(Vertex));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, offset(offsetof(Vertex, position)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, offset(offsetof(Vertex, side)));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, offset(offsetof(Vertex, uv)));
  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, offset(offsetof(Vertex, profile)));
  glEnableVertexAttribArray(4);
  glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, stride, offset(offsetof(Vertex, kind)));
  glBindVertexArray(0);
}
TracksRenderer::~TracksRenderer() = default;

std::size_t TracksRenderer::draw(const Tracks &tracks, glm::vec3 camera, const glm::mat4 &viewProjection) {
  auto &p = *impl_;
  p.shader->poll();
  if (!p.shader->id() || (tracks.size() == 0 && tracks.staticSegments().empty()))
    return 0;
  GLint previousProgram = 0, previousVao = 0, depthFunc = 0, srcRgb = 0, dstRgb = 0, srcA = 0, dstA = 0;
  GLboolean depthMask = GL_TRUE;
  glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
  glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVao);
  glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);
  glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
  glGetIntegerv(GL_BLEND_SRC_RGB, &srcRgb);
  glGetIntegerv(GL_BLEND_DST_RGB, &dstRgb);
  glGetIntegerv(GL_BLEND_SRC_ALPHA, &srcA);
  glGetIntegerv(GL_BLEND_DST_ALPHA, &dstA);
  const GLboolean blend = glIsEnabled(GL_BLEND), cull = glIsEnabled(GL_CULL_FACE), depthTest = glIsEnabled(GL_DEPTH_TEST);
  glBindVertexArray(p.vao);
  p.upload(tracks);
  const GLuint program = p.shader->id();
  glUseProgram(program);
  const auto &ws = world::state();
  world::apply(program, glm::mat4(1), ws.scene);
  glUniformMatrix4fv(glGetUniformLocation(program, "uViewProjection"), 1, GL_FALSE, glm::value_ptr(viewProjection));
  glUniform3fv(glGetUniformLocation(program, "uCamPos"), 1, glm::value_ptr(camera));
  const glm::vec3 sun = ws.settings.enabled ? ws.sun.direction : glm::normalize(glm::vec3(.2f, .5f, 1));
  glUniform3fv(glGetUniformLocation(program, "uSunDir"), 1, glm::value_ptr(sun));
  glUniform1f(glGetUniformLocation(program, "uSunLum"), ws.settings.enabled ? luminance(ws.sun.radiance) : 3.f);
  glUniform1f(glGetUniformLocation(program, "uAmbLum"), ws.settings.enabled ? luminance(ws.sun.ambient) * 3.f : .3f);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glDepthMask(GL_FALSE);
  glDisable(GL_CULL_FACE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_DST_COLOR, GL_ZERO); // multiply the lit terrain by the relief ratio
  glEnable(GL_POLYGON_OFFSET_FILL);
  glPolygonOffset(-1.f, -4.f);
  std::size_t draws = 0;
  if (tracks.size()) {
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(tracks.size() * 6), GL_UNSIGNED_INT, offset(0));
    ++draws;
  }
  if (!tracks.staticSegments().empty()) {
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(tracks.staticSegments().size() * 6), GL_UNSIGNED_INT,
                   offset(p.ringCapacity * 6 * sizeof(std::uint32_t)));
    ++draws;
  }
  glDisable(GL_POLYGON_OFFSET_FILL);
  glBlendFuncSeparate(static_cast<GLenum>(srcRgb), static_cast<GLenum>(dstRgb), static_cast<GLenum>(srcA),
                      static_cast<GLenum>(dstA));
  (blend ? glEnable : glDisable)(GL_BLEND);
  (cull ? glEnable : glDisable)(GL_CULL_FACE);
  (depthTest ? glEnable : glDisable)(GL_DEPTH_TEST);
  glDepthMask(depthMask);
  glDepthFunc(static_cast<GLenum>(depthFunc));
  glBindVertexArray(static_cast<GLuint>(previousVao));
  glUseProgram(static_cast<GLuint>(previousProgram));
  if (validation::run.capture) {
    validation::run.frameDraws += draws;
    validation::run.frameTriangles += (tracks.size() + tracks.staticSegments().size()) * 2;
  }
  return draws;
}
} // namespace trackrs::terrain
