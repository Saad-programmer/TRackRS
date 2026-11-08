// SPDX-License-Identifier: GPL-2.0-or-later
#include "water.h"
#include <cmath>
#include <cstdint>
#include <stdexcept>
namespace trackrs::world {
void buildWaterGrid(glm::vec2 camera, float height, const std::function<float(float, float)> &terrain,
                    std::vector<WaterVertex> &out) {
  if (!std::isfinite(camera.x) || !std::isfinite(camera.y) || !std::isfinite(height) || !terrain)
    throw std::invalid_argument("invalid water grid input");
  constexpr int side = 2 * WaterGridHalf + 1;
  const float ox = std::floor(camera.x / WaterGridSpacing) * WaterGridSpacing;
  const float oy = std::floor(camera.y / WaterGridSpacing) * WaterGridSpacing;
  out.clear();
  out.reserve(static_cast<std::size_t>(side) * side);
  for (int j = -WaterGridHalf; j <= WaterGridHalf; ++j)
    for (int i = -WaterGridHalf; i <= WaterGridHalf; ++i) {
      const float x = ox + static_cast<float>(i) * WaterGridSpacing;
      const float y = oy + static_cast<float>(j) * WaterGridSpacing;
      out.push_back({x, y, height - terrain(x, y)});
    }
}
std::vector<std::uint32_t> waterGridIndices() {
  constexpr std::uint32_t side = 2 * WaterGridHalf + 1;
  std::vector<std::uint32_t> indices;
  indices.reserve(static_cast<std::size_t>(side - 1) * (side - 1) * 6);
  for (std::uint32_t y = 0; y + 1 < side; ++y)
    for (std::uint32_t x = 0; x + 1 < side; ++x) {
      const std::uint32_t a = y * side + x, b = a + 1, c = a + side, d = c + 1;
      indices.insert(indices.end(), {a, b, d, a, d, c});
    }
  return indices;
}
} // namespace trackrs::world
