// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "engine/rhi/rhi.h"
#include "texture_stream.h"
#include <filesystem>
#include <functional>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
namespace trackrs::pbr {
class Ibl {
  std::shared_ptr<TextureStream> stream_;
  std::array<std::shared_ptr<StreamedTexture>, 4> maps_;

public:
  explicit Ibl(const std::filesystem::path &directory);
  void pump();
  bool ready() const;
  GLuint sky() const;
  GLuint irradiance() const;
  GLuint prefilter() const;
  GLuint brdf() const;
  float maxLod() const;
  std::size_t bytes() const;
};
using SceneCapture =
    std::function<void(const glm::mat4 &projection, const glm::mat4 &view,
                       const glm::vec3 &center)>;
class ReflectionProbe {
  struct Impl;
  std::unique_ptr<Impl> impl_;

public:
  ReflectionProbe(const std::filesystem::path &shaders, unsigned size = 32,
                  double interval = .5, std::function<double()> clock = {});
  ~ReflectionProbe();
  bool update(const Ibl &environment, glm::vec3 center,
              const SceneCapture &scene = {},
              glm::mat3 skyRotation = glm::mat3(1));
  GLuint texture() const;
  float maxLod() const;
  nlohmann::json report();
};
void profileEnabled(bool enabled);
void materialMeasurement(const std::string &name, std::size_t residentBytes,
                         const std::function<void()> &draw);
nlohmann::json materialReport();
void clearMeasurements();
} // namespace trackrs::pbr
