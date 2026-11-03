// SPDX-License-Identifier: GPL-2.0-or-later
#include "renderer.h"
#include "engine/pbr/ibl.h"
#include "engine/pbr/renderer.h"
#include "engine/render/basic.h"
#include "engine/rhi/rhi.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
namespace trackrs::terrain {
namespace {
Settings current;
Renderer *activeRenderer = nullptr;
class State {
  GLint program = 0, vao = 0, buffer = 0, active = 0, pbo = 0, align = 0,
        row = 0, front = 0, cull = 0;
  GLboolean depthMask = 0;
  std::array<GLint, 10> two{}, cube{}, array{};
  static constexpr std::array<GLenum, 7> caps = {GL_DEPTH_TEST,
                                                 GL_CULL_FACE,
                                                 GL_BLEND,
                                                 GL_FRAMEBUFFER_SRGB,
                                                 GL_SAMPLE_ALPHA_TO_COVERAGE,
                                                 GL_TEXTURE_CUBE_MAP_SEAMLESS,
                                                 GL_SCISSOR_TEST};
  std::array<GLboolean, 7> enabled{};

public:
  State() {
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &buffer);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
    glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &pbo);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &align);
    glGetIntegerv(GL_UNPACK_ROW_LENGTH, &row);
    glGetIntegerv(GL_FRONT_FACE, &front);
    glGetIntegerv(GL_CULL_FACE_MODE, &cull);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
    for (std::size_t i = 0; i < caps.size(); ++i)
      enabled[i] = glIsEnabled(caps[i]);
    for (unsigned i = 0; i < two.size(); ++i) {
      glActiveTexture(GL_TEXTURE0 + i);
      glGetIntegerv(GL_TEXTURE_BINDING_2D, &two[i]);
      glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &cube[i]);
      glGetIntegerv(GL_TEXTURE_BINDING_2D_ARRAY, &array[i]);
    }
  }
  ~State() {
    for (unsigned i = 0; i < two.size(); ++i) {
      glActiveTexture(GL_TEXTURE0 + i);
      glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(two[i]));
      glBindTexture(GL_TEXTURE_CUBE_MAP, static_cast<GLuint>(cube[i]));
      glBindTexture(GL_TEXTURE_2D_ARRAY, static_cast<GLuint>(array[i]));
    }
    glActiveTexture(static_cast<GLenum>(active));
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, static_cast<GLuint>(pbo));
    glPixelStorei(GL_UNPACK_ALIGNMENT, align);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, row);
    glUseProgram(static_cast<GLuint>(program));
    glBindVertexArray(static_cast<GLuint>(vao));
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(buffer));
    glFrontFace(static_cast<GLenum>(front));
    glCullFace(static_cast<GLenum>(cull));
    glDepthMask(depthMask);
    for (std::size_t i = 0; i < caps.size(); ++i)
      enabled[i] ? glEnable(caps[i]) : glDisable(caps[i]);
    draw::invalidateGpuState();
  }
};
void integer(GLuint p, const char *name, int value) {
  glUniform1i(glGetUniformLocation(p, name), value);
}
void scalar(GLuint p, const char *name, float value) {
  glUniform1f(glGetUniformLocation(p, name), value);
}
void vector(GLuint p, const char *name, glm::vec3 value) {
  glUniform3fv(glGetUniformLocation(p, name), 1, glm::value_ptr(value));
}
} // namespace
void configure(Settings settings) { current = settings; }
Settings settings() { return current; }
struct Renderer::Impl {
  std::unique_ptr<rhi::ShaderProgram> shader;
  std::shared_ptr<pbr::TextureStream> stream;
  std::array<std::shared_ptr<pbr::StreamedTexture>, 3> maps;
  std::unique_ptr<pbr::Ibl> environment;
  GLuint vao = 0, vbo = 0, ibo = 0, height = 0, type = 0;
  unsigned size = 0, typeSize = 0;
  float spacing = 1;
  glm::vec2 heights{0};
  Selector selector;
  Selection selected;
  bool ready = false;
  std::uint64_t probeCalls = 0, probeDraws = 0, probeTriangles = 0;
  ~Impl() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ibo);
    glDeleteTextures(1, &height);
    glDeleteTextures(1, &type);
  }
};
Renderer::Renderer(const std::vector<float> &heights, unsigned size,
                   float spacing, const std::vector<std::uint8_t> &types,
                   unsigned typeSize, const std::filesystem::path &data)
    : impl_(std::make_unique<Impl>()) {
  if (!size || heights.size() != std::size_t(size) * size || !typeSize ||
      types.size() != std::size_t(typeSize) * typeSize || spacing <= 0)
    throw std::invalid_argument("invalid terrain texture dimensions");
  State guard;
  auto &p = *impl_;
  p.size = size;
  p.typeSize = typeSize;
  p.spacing = spacing;
  auto range = std::minmax_element(heights.begin(), heights.end());
  p.heights = {*range.first, *range.second};
  p.shader = std::make_unique<rhi::ShaderProgram>(
      data / "shaders/pbr/terrain.vert", data / "shaders/pbr/pbr.frag",
      [](GLuint program) {
        return glGetUniformLocation(program, "uHeight") >= 0 &&
               glGetUniformLocation(program, "uTerrainTypes") >= 0;
      },
      std::vector<std::string>{"TERRAIN"});
  p.stream = pbr::textureStream();
  constexpr std::array<const char *, 9> names = {"tarmac", "gravel", "dirt",
                                                 "grass",  "sand",   "snow",
                                                 "ice",    "mud",    "water"};
  constexpr std::array<const char *, 3> roles = {"albedo", "normal", "orm"};
  constexpr std::array<assets::Semantic, 3> semantics = {
      assets::Semantic::Albedo, assets::Semantic::Normal,
      assets::Semantic::Orm};
  for (unsigned role = 0; role < 3; ++role) {
    std::vector<assets::Bytes> layers;
    layers.reserve(names.size());
    for (auto name : names)
      layers.push_back(
          assets::readFile(data / "terrain/pbr" /
                           (std::string(name) + "-" + roles[role] + ".ktx2")));
    p.maps[role] = p.stream->requestArray(layers, semantics[role]);
  }
  p.environment =
      std::make_unique<pbr::Ibl>(data / "scenes/material_preview/ibl");
  std::vector<glm::vec2> grid;
  std::vector<std::uint16_t> indices;
  grid.reserve(std::size_t(GridCells + 1) * (GridCells + 1));
  indices.reserve(std::size_t(GridCells) * GridCells * 6);
  for (unsigned y = 0; y <= GridCells; ++y)
    for (unsigned x = 0; x <= GridCells; ++x)
      grid.emplace_back(x, y);
  for (unsigned y = 0; y < GridCells; ++y)
    for (unsigned x = 0; x < GridCells; ++x) {
      auto a = static_cast<std::uint16_t>(y * (GridCells + 1) + x),
           b = static_cast<std::uint16_t>(a + 1),
           c = static_cast<std::uint16_t>(a + GridCells + 1),
           d = static_cast<std::uint16_t>(c + 1);
      indices.insert(indices.end(), {c, a, d, d, a, b});
    }
  glGenVertexArrays(1, &p.vao);
  glBindVertexArray(p.vao);
  glGenBuffers(1, &p.vbo);
  glBindBuffer(GL_ARRAY_BUFFER, p.vbo);
  glBufferData(GL_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(grid.size() * sizeof(glm::vec2)),
               grid.data(), GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), nullptr);
  glGenBuffers(1, &p.ibo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, p.ibo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(indices.size() * sizeof(std::uint16_t)),
               indices.data(), GL_STATIC_DRAW);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glActiveTexture(GL_TEXTURE0);
  glGenTextures(1, &p.height);
  glBindTexture(GL_TEXTURE_2D, p.height);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, static_cast<GLsizei>(size),
               static_cast<GLsizei>(size), 0, GL_RED, GL_FLOAT, heights.data());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glGenTextures(1, &p.type);
  glBindTexture(GL_TEXTURE_2D, p.type);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, static_cast<GLsizei>(typeSize),
               static_cast<GLsizei>(typeSize), 0, GL_RED_INTEGER,
               GL_UNSIGNED_BYTE, types.data());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}
Renderer::~Renderer() {
  if (activeRenderer == this)
    activeRenderer = nullptr;
}
void writeReport(const std::filesystem::path &path) {
  if (activeRenderer)
    std::ofstream(path) << activeRenderer->report().dump(2) << '\n';
}
std::size_t Renderer::draw(glm::vec3 camera, const glm::mat4 &vp, bool capture,
                           int debugView) {
  draw::flush();
  State guard;
  auto &p = *impl_;
  p.shader->poll();
  p.environment->pump();
  p.ready = p.environment->ready();
  for (const auto &map : p.maps) {
    if (!map->error.empty())
      throw std::runtime_error(map->error);
    p.ready = p.ready && map->ready;
  }
  float distance = current.quality == RenderQuality::Low      ? 768.f
                   : current.quality == RenderQuality::Medium ? 1200.f
                   : current.quality == RenderQuality::High   ? 1800.f
                                                              : 2400.f;
  if (capture)
    distance = std::min(distance, 768.f);
  auto selection = p.selector.select(
      camera, vp, p.spacing, distance, p.heights,
      capture ? (carProbeResolution(current.quality) == 128 ? 1u : 2u) : 0u);
  if (capture) {
    ++p.probeCalls;
    p.probeDraws += selection.patches.size();
    p.probeTriangles += selection.patches.size() * GridCells * GridCells * 2;
  }

  if (!capture) {
    p.selected = selection;
    activeRenderer = this;
  }
  auto program = p.shader->id();
  glUseProgram(program);
  glBindVertexArray(p.vao);
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);
  glDisable(GL_BLEND);
  glDisable(GL_FRAMEBUFFER_SRGB);
  glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
  glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
  glDisable(GL_SCISSOR_TEST);
  glUniformMatrix4fv(glGetUniformLocation(program, "uViewProjection"), 1,
                     GL_FALSE, glm::value_ptr(vp));
  vector(program, "uCamPos", camera);
  vector(program, "uSunDir", glm::normalize(glm::vec3(.2f, .5f, 1)));
  vector(program, "uSunColor", glm::vec3(3));
  vector(program, "uAmbientColor", glm::vec3(.08f));
  scalar(program, "uWorldPeriod", static_cast<float>(p.size) * p.spacing);
  scalar(program, "uAOStrength", 1);
  integer(program, "uPointLightCount", 0);
  integer(program, "uDebugView", debugView);
  integer(program, "uHasIBL", p.environment->ready() ? 1 : 0);
  integer(program, "uUseLocalProbe", 0);
  integer(program, "uToneMap", capture ? 0 : 1);
  constexpr std::array<const char *, 3> names = {
      "uTerrainAlbedo", "uTerrainNormal", "uTerrainOrm"};
  for (unsigned i = 0; i < 3; ++i) {
    glActiveTexture(GL_TEXTURE0 + i);
    glBindTexture(GL_TEXTURE_2D_ARRAY, p.maps[i]->id());
    integer(program, names[i], static_cast<int>(i));
  }
  glActiveTexture(GL_TEXTURE6);
  glBindTexture(GL_TEXTURE_2D, p.height);
  integer(program, "uHeight", 6);
  glActiveTexture(GL_TEXTURE8);
  glBindTexture(GL_TEXTURE_2D, p.type);
  integer(program, "uTerrainTypes", 8);
  integer(program, "uIrradianceMap", 3);
  integer(program, "uPrefilterMap", 4);
  integer(program, "uBrdfLUT", 5);
  integer(program, "uLocalProbe", 7);
  if (p.environment->ready()) {
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_CUBE_MAP, p.environment->irradiance());
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_CUBE_MAP, p.environment->prefilter());
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, p.environment->brdf());
    scalar(program, "uPrefilterMaxLod", p.environment->maxLod());
    auto rotate = glm::mat3(
        glm::rotate(glm::mat4(1), glm::radians(-90.f), glm::vec3(1, 0, 0)));
    glUniformMatrix3fv(glGetUniformLocation(program, "uEnvironmentRotation"), 1,
                       GL_FALSE, glm::value_ptr(rotate));
  }
  for (const auto &patch : selection.patches) {
    glUniform4f(glGetUniformLocation(program, "uPatch"),
                static_cast<float>(patch.key.x),
                static_cast<float>(patch.key.y),
                static_cast<float>(patch.cells), p.spacing);
    glDrawElements(GL_TRIANGLES,
                   static_cast<GLsizei>(GridCells * GridCells * 6),
                   GL_UNSIGNED_SHORT, nullptr);
  }
  return selection.patches.size();
}
nlohmann::json Renderer::report() const {
  const auto &p = *impl_;
  std::size_t textures = 0;
  for (const auto &map : p.maps)
    textures += map->residentBytes;
  return {{"renderer", "cdlod"},
          {"probe_capture_calls", p.probeCalls},
          {"probe_draws_total", p.probeDraws},
          {"probe_triangles_total", p.probeTriangles},
          {"quality", std::string(qualityName(current.quality))},
          {"ready", p.ready},
          {"draw_calls", p.selected.patches.size()},
          {"submitted_triangles",
           p.selected.patches.size() * GridCells * GridCells * 2},
          {"nodes_tested", p.selected.tested},
          {"nodes_culled", p.selected.culled},
          {"cache_tiles", p.selector.residentTiles()},
          {"height_texture_bytes", std::size_t(p.size) * p.size * 4},
          {"type_texture_bytes", std::size_t(p.typeSize) * p.typeSize},
          {"surface_texture_bytes", textures},
          {"environment_texture_bytes", p.environment->bytes()},
          {"shared_grid_bytes",
           std::size_t(GridCells + 1) * (GridCells + 1) * sizeof(glm::vec2) +
               std::size_t(GridCells) * GridCells * 6 * sizeof(std::uint16_t)}};
}
} // namespace trackrs::terrain
