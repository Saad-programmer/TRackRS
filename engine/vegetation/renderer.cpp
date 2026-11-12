// SPDX-License-Identifier: GPL-2.0-or-later
#include "renderer.h"
#include "engine/core/pcg32.h"
#include "engine/pbr/ibl.h"
#include "engine/render/basic.h"
#include "engine/rhi/rhi.h"
#include "engine/world/state.h"
#include "validation.h"
#include <array>
#include <fstream>
#include <glm/gtc/type_ptr.hpp>
namespace trackrs::vegetation {
namespace {
Settings current;
float animationTime = 0;
const Renderer *activeRenderer = nullptr;

// Saves and restores exactly the state this pass touches.
class Guard {
  GLint program = 0, vao = 0, buffer = 0, active = 0;
  std::array<GLint, 6> two{}, cube{};
  GLboolean depthMask = 0, cull = 0, blend = 0, coverage = 0, depth = 0, srgb = 0;

public:
  Guard() {
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &buffer);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
    cull = glIsEnabled(GL_CULL_FACE);
    blend = glIsEnabled(GL_BLEND);
    coverage = glIsEnabled(GL_SAMPLE_ALPHA_TO_COVERAGE);
    depth = glIsEnabled(GL_DEPTH_TEST);
    srgb = glIsEnabled(GL_FRAMEBUFFER_SRGB);
    for (unsigned i = 0; i < two.size(); ++i) {
      glActiveTexture(GL_TEXTURE0 + i);
      glGetIntegerv(GL_TEXTURE_BINDING_2D, &two[i]);
      glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &cube[i]);
    }
  }
  ~Guard() {
    for (unsigned i = 0; i < two.size(); ++i) {
      glActiveTexture(GL_TEXTURE0 + i);
      glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(two[i]));
      glBindTexture(GL_TEXTURE_CUBE_MAP, static_cast<GLuint>(cube[i]));
    }
    glActiveTexture(static_cast<GLenum>(active));
    glUseProgram(static_cast<GLuint>(program));
    glBindVertexArray(static_cast<GLuint>(vao));
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(buffer));
    glDepthMask(depthMask);
    for (auto [cap, on] : {std::pair{GL_CULL_FACE, cull}, {GL_BLEND, blend},
                           {GL_SAMPLE_ALPHA_TO_COVERAGE, coverage}, {GL_DEPTH_TEST, depth},
                           {GL_FRAMEBUFFER_SRGB, srgb}})
      on ? glEnable(cap) : glDisable(cap);
    draw::invalidateGpuState();
  }
};
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
} // namespace

void configure(Settings settings) { current = settings; }
Settings settings() { return current; }
void setAnimationTime(float seconds) { animationTime = seconds; }

struct Range {
  GLint baseVertex = 0;
  std::size_t firstIndex = 0, count = 0;
};
struct Band {
  std::string sprite;
  Species species;
  GLuint texture = 0;
  std::array<Range, MeshLods + 1> lods;
};
struct Renderer::Impl {
  Catalog catalog;
  std::unique_ptr<rhi::ShaderProgram> shader, coverageShader;
  std::unique_ptr<pbr::Ibl> environment;
  std::vector<Band> bands;
  std::vector<Vertex> vertices;
  std::vector<std::uint16_t> indices;
  std::vector<Instance> staging;
  std::vector<std::array<std::size_t, MeshLods + 1>> offsets;
  Classifier classifier;
  GLuint vao = 0, vbo = 0, ibo = 0, instances = 0, white = 0, flat = 0, orm = 0;
  std::size_t instanceCapacity = 0;
  bool dirty = false;
  Pass pass = Pass::Main;
  glm::vec3 camera{0};
  // Last main-view frame (report).
  std::size_t draws = 0, triangles = 0;
  Classifier::Stats stats;
  std::array<std::size_t, 3> passDraws{};
  ~Impl() {
    glDeleteVertexArrays(1, &vao);
    for (auto *b : {&vbo, &ibo, &instances})
      glDeleteBuffers(1, b);
    for (auto *t : {&white, &flat, &orm})
      glDeleteTextures(1, t);
  }
  void append(const Mesh &m, Range &r) {
    r.baseVertex = static_cast<GLint>(vertices.size());
    r.firstIndex = indices.size();
    r.count = m.indices.size();
    vertices.insert(vertices.end(), m.vertices.begin(), m.vertices.end());
    indices.insert(indices.end(), m.indices.begin(), m.indices.end());
    dirty = true;
  }
  void upload() {
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(),
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(std::uint16_t)),
                 indices.data(), GL_STATIC_DRAW);
    dirty = false;
  }
};

Renderer::Renderer(const std::filesystem::path &data) : impl_(std::make_unique<Impl>()) {
  auto &p = *impl_;
  p.catalog = Catalog::load(data / "vegetation/species.json");
  p.catalog.merge(packSpecies()); // content-pack species for this stage
  auto valid = [](GLuint program) { return glGetUniformLocation(program, "uVegetationWind") >= 0; };
  p.shader = std::make_unique<rhi::ShaderProgram>(data / "shaders/pbr/vegetation.vert",
                                                  data / "shaders/pbr/pbr.frag", valid,
                                                  std::vector<std::string>{"VEGETATION"});
  p.coverageShader = std::make_unique<rhi::ShaderProgram>(
      data / "shaders/pbr/vegetation.vert", data / "shaders/pbr/pbr.frag", valid,
      std::vector<std::string>{"VEGETATION", "ALPHA_TO_COVERAGE"});
  p.environment = std::make_unique<pbr::Ibl>(data / "scenes/material_preview/ibl");
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
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, p.ibo);
  // Static per-vertex stream: set once, never per frame.
  const auto stride = static_cast<GLsizei>(sizeof(Vertex));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, bufferOffset(offsetof(Vertex, position)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 4, GL_INT_2_10_10_10_REV, GL_TRUE, stride,
                        bufferOffset(offsetof(Vertex, normal)));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_UNSIGNED_SHORT, GL_TRUE, stride, bufferOffset(offsetof(Vertex, uv)));
  glEnableVertexAttribArray(5);
  glVertexAttribPointer(5, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, bufferOffset(offsetof(Vertex, color)));
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

// Texture name and card crossing count are distinct GL/legacy quantities.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
std::size_t Renderer::addBand(std::string_view sprite, GLuint texture, unsigned crossings) {
  auto &p = *impl_;
  Band band;
  band.sprite = std::string(sprite);
  band.species = p.catalog.lookup(sprite);
  band.texture = texture;
  const auto seed = stageSeed(sprite);
  if (band.species.form != Form::Card)
    for (unsigned lod = 0; lod < MeshLods; ++lod)
      p.append(buildMesh(band.species, lod, seed), band.lods[lod]);
  p.append(buildCard(crossings), band.lods[ImpostorLod]);
  p.bands.push_back(std::move(band));
  return p.bands.size() - 1;
}
std::size_t Renderer::bands() const { return impl_->bands.size(); }
Form Renderer::form(std::size_t band) const { return impl_->bands.at(band).species.form; }

Classifier &Renderer::begin(glm::vec3 camera, const glm::mat4 &vp, Pass pass, RenderQuality quality) {
  auto &p = *impl_;
  p.pass = pass;
  p.camera = camera;
  p.classifier.begin(camera, vp, rangesFor(quality), pass, p.bands.size());
  return p.classifier;
}

std::size_t Renderer::submit(const glm::mat4 &vp, bool linearCapture) {
  auto &p = *impl_;
  draw::flush();
  Guard guard;
  p.shader->poll();
  p.coverageShader->poll();
  p.environment->pump();
  if (p.dirty)
    p.upload();
  // Gather every bucket into one contiguous upload (one orphan + one copy).
  p.staging.clear();
  auto &offsets = p.offsets;
  offsets.resize(p.bands.size());
  for (std::size_t b = 0; b < p.bands.size(); ++b)
    for (unsigned lod = 0; lod <= MeshLods; ++lod) {
      offsets[b][lod] = p.staging.size();
      const auto &bucket = p.classifier.bucket(b, lod);
      p.staging.insert(p.staging.end(), bucket.begin(), bucket.end());
    }
  if (p.staging.empty())
    return 0;
  glBindVertexArray(p.vao);
  glBindBuffer(GL_ARRAY_BUFFER, p.instances);
  const auto bytes = p.staging.size() * sizeof(Instance);
  if (bytes > p.instanceCapacity)
    p.instanceCapacity = std::max<std::size_t>(bytes * 3 / 2, std::size_t{64} * 1024);
  glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(p.instanceCapacity), nullptr, GL_STREAM_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(bytes), p.staging.data());

  GLint samples = 0;
  glGetIntegerv(GL_SAMPLES, &samples);
  const bool coverage = samples > 0 && !world::state().depthPass;
  const GLuint program = coverage ? p.coverageShader->id() : p.shader->id();
  glUseProgram(program);
  world::apply(program, glm::mat4(1), world::state().scene);
  auto uniform = [&](const char *name) { return glGetUniformLocation(program, name); };
  glUniformMatrix4fv(uniform("uViewProjection"), 1, GL_FALSE, glm::value_ptr(vp));
  const auto &ws = world::state();
  const bool lit = ws.settings.enabled;
  const glm::vec3 camera = lit ? glm::vec3(glm::inverse(ws.view)[3]) : p.camera;
  glUniform3fv(uniform("uCamPos"), 1, glm::value_ptr(camera));
  const glm::vec3 sun = lit ? ws.sun.direction : glm::normalize(glm::vec3(.2f, .5f, 1));
  const glm::vec3 sunColour = lit ? ws.sun.radiance : glm::vec3(3);
  const glm::vec3 ambient = lit ? ws.sun.ambient : glm::vec3(.08f);
  glUniform3fv(uniform("uSunDir"), 1, glm::value_ptr(sun));
  glUniform3fv(uniform("uSunColor"), 1, glm::value_ptr(sunColour));
  glUniform3fv(uniform("uAmbientColor"), 1, glm::value_ptr(ambient));
  glUniform1i(uniform("uPointLightCount"), 0);
  glUniform1i(uniform("uUseLocalProbe"), 0);
  glUniform1i(uniform("uToneMap"), linearCapture ? 0 : 1);
  glUniform1f(uniform("uMetallicFactor"), 0);
  glUniform1f(uniform("uAOStrength"), 0);
  glUniform1f(uniform("uNormalScale"), 1);
  for (auto [name, unit] : {std::pair{"uAlbedoMap", 0}, {"uNormalMap", 1}, {"uMetalRoughMap", 2},
                            {"uIrradianceMap", 3}, {"uPrefilterMap", 4}, {"uBrdfLUT", 5}, {"uLocalProbe", 7}})
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
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, p.flat);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, p.orm);
  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  glDisable(GL_CULL_FACE); // blades, fronds and cards are two-sided
  glDisable(GL_BLEND);
  glDisable(GL_FRAMEBUFFER_SRGB);
  coverage ? glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE) : glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
  // Wind: fixed prevailing direction, deterministic clock from the game.
  const glm::vec2 wind = glm::vec2(0.8f, 0.6f) * 1.0f;

  std::size_t draws = 0, triangles = 0;
  for (std::size_t b = 0; b < p.bands.size(); ++b) {
    const auto &band = p.bands[b];
    for (unsigned lod = 0; lod <= MeshLods; ++lod) {
      const auto count = p.classifier.bucket(b, lod).size();
      const auto &range = band.lods[lod];
      if (!count || !range.count)
        continue;
      const bool card = lod == ImpostorLod;
      glBindTexture(GL_TEXTURE_2D, card ? band.texture : p.white);
      glUniform1i(uniform("uAlbedoGamma"), card ? 1 : 0);
      glUniform1f(uniform("uAlphaCutoff"), card ? 0.5f : 0.0f);
      glUniform1f(uniform("uRoughnessFactor"), band.species.roughness);
      glUniform1f(uniform("uTranslucency"), band.species.translucency);
      glUniform4f(uniform("uVegetationWind"), wind.x, wind.y, animationTime,
                  card ? band.species.flex * 0.5f : band.species.flex);
      const auto offset = offsets[b][lod] * sizeof(Instance);
      glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Instance),
                            bufferOffset(offset + offsetof(Instance, a)));
      glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(Instance),
                            bufferOffset(offset + offsetof(Instance, b)));
      glDrawElementsInstancedBaseVertex(GL_TRIANGLES, static_cast<GLsizei>(range.count), GL_UNSIGNED_SHORT,
                                        bufferOffset(range.firstIndex * sizeof(std::uint16_t)),
                                        static_cast<GLsizei>(count), range.baseVertex);
      ++draws;
      triangles += range.count / 3 * count;
    }
  }
  if (validation::run.capture) {
    validation::run.frameDraws += draws;
    validation::run.frameTriangles += triangles;
  }
  p.passDraws[static_cast<std::size_t>(p.pass)] = draws;
  if (p.pass == Pass::Main) {
    p.draws = draws;
    p.triangles = triangles;
    p.stats = p.classifier.stats();
    activeRenderer = this;
  }
  return draws;
}

nlohmann::json Renderer::report() const {
  const auto &p = *impl_;
  nlohmann::json bands = nlohmann::json::array();
  for (const auto &b : p.bands) {
    nlohmann::json lods = nlohmann::json::array();
    for (const auto &r : b.lods)
      lods.push_back(r.count / 3);
    bands.push_back({{"sprite", b.sprite}, {"form", std::string(formName(b.species.form))}, {"triangles", lods}});
  }
  return {{"renderer", "instanced-3d"},
          {"bands", bands},
          {"draw_calls", p.draws},
          {"submitted_triangles", p.triangles},
          {"shadow_draw_calls_last_cascade", p.passDraws[1]},
          {"probe_draw_calls_last", p.passDraws[2]},
          {"instances_per_lod", p.stats.instances},
          {"instances_culled", p.stats.culled},
          {"instances_crossfading", p.stats.faded},
          {"mesh_bytes", p.vertices.size() * sizeof(Vertex) + p.indices.size() * sizeof(std::uint16_t)},
          {"instance_stream_bytes", p.instanceCapacity},
          {"animation_time", animationTime}};
}
void writeReport(const std::filesystem::path &path) {
  if (activeRenderer)
    std::ofstream(path) << activeRenderer->report().dump(2) << '\n';
}
} // namespace trackrs::vegetation
