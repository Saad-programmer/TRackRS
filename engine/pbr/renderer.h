// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "engine/mesh/mesh_importer.h"
#include "ibl.h"
#include <filesystem>
#include <glm/glm.hpp>
#include <memory>
#include <vector>
namespace trackrs::pbr {
struct PointLight {
  glm::vec3 position{0};
  glm::vec3 radiance{0};
  float range = 10;
};
struct Shadows {
  unsigned texture = 0; // borrowed GL_TEXTURE_2D_ARRAY comparison texture
  glm::mat4 view{1};
  std::array<glm::mat4, 4> lightViewProjection{
      {glm::mat4(1), glm::mat4(1), glm::mat4(1), glm::mat4(1)}};
  glm::vec4 splits{10, 30, 90, 270};
  int count = 0;
};
struct Frame {
  glm::mat4 viewProjection{1};
  glm::vec3 camera{0};
  glm::vec3 sunDirection{.2f, .5f, 1}, sunRadiance{3}, ambient{.08f};
  std::vector<PointLight> lights;
  std::array<glm::vec3, 2> headlightPositions{
      {glm::vec3(-.55f, 1.8f, .45f), glm::vec3(.55f, 1.8f, .45f)}};
  Shadows shadows;
  const Ibl *environment = nullptr;
  GLuint reflectionProbe = 0;
  float probeMaxLod = 0;
  glm::mat3 environmentRotation{1}, probeRotation{1};
  float wetness = 0;
  float alpha = 1;
  int debugView = 0;
  bool toneMap = true, headlights = true;
};
struct Statistics {
  std::uint64_t draws = 0, triangles = 0;
};
class Model {
  struct Impl;
  std::unique_ptr<Impl> impl_;

public:
  Model(const mesh::MeshData &data, const std::filesystem::path &shaders);
  ~Model();
  Model(const Model &) = delete;
  Statistics draw(const Frame &frame, const std::vector<glm::mat4> &models,
                  bool packed = true, bool instanced = false);
  // Adapter to the existing CPU transform stack. Uses eye-space lighting;
  // preview/tool callers supply world-space Frame and model matrices directly.
  void drawCurrent(float alpha = 1);
  bool drawCurrentInstances(const std::vector<glm::mat4> &models, float alpha = 1);
};
// Vehicle composition supplies one shared pair, including wheel draws.
class HeadlightScope {
  std::vector<PointLight> previous_;
  glm::vec3 previousCenter_;

public:
  explicit HeadlightScope(const glm::mat4 &bodyToEye);
  ~HeadlightScope();
  HeadlightScope(const HeadlightScope &) = delete;
};
void setWorldView(const glm::mat4 &view);
void setProbeScene(SceneCapture scene);
void writeProfile(const std::filesystem::path &path);
void shutdown();
void configure(const std::filesystem::path &shaders);
const std::filesystem::path &shaderDirectory();
} // namespace trackrs::pbr
