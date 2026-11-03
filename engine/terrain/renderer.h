// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "cdlod.h"
#include "engine/core/render_quality.h"
#include <filesystem>
#include <memory>
#include <nlohmann/json.hpp>
namespace trackrs::terrain {
struct Settings {
  bool enabled = false;
  RenderQuality quality = RenderQuality::Medium;
};
void configure(Settings settings);
Settings settings();
void writeReport(const std::filesystem::path &path);
class Renderer {
  struct Impl;
  std::unique_ptr<Impl> impl_;

public:
  Renderer(const std::vector<float> &heights, unsigned size, float spacing,
           const std::vector<std::uint8_t> &types, unsigned typeSize,
           const std::filesystem::path &data);
  ~Renderer();
  void draw(glm::vec3 camera, const glm::mat4 &viewProjection,
            bool capture = false);
  nlohmann::json report() const;
};
} // namespace trackrs::terrain
