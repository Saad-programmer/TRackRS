// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <array>
#include <glm/glm.hpp>
#include <string_view>
namespace trackrs::world {
struct Settings {
  bool enabled = false, csm = true;
  float hour = 12;
};
struct Sun {
  glm::vec3 direction{0, 0, 1}, radiance{3}, skyRadiance{3}, ambient{.08f};
  float daylight = 1;
};
Sun sunlight(float hour);
float presetHour(std::string_view name);
float fogOpticalDepth(float density, float falloff, float cameraHeight,
                      float targetHeight, float distance);
struct Cascades {
  std::array<glm::mat4, 4> matrices{};
  glm::vec4 splits{0}, bias{0};
  unsigned count = 0, resolution = 0;
};
struct CameraMatrices {
  glm::mat4 view{1}, projection{1};
};
Cascades makeCascades(const CameraMatrices &camera, glm::vec3 sun,
                      unsigned count, unsigned resolution, float distance);
} // namespace trackrs::world
