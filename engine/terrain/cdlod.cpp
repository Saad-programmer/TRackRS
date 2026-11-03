// SPDX-License-Identifier: GPL-2.0-or-later
#include "cdlod.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_access.hpp>
#include <stdexcept>
namespace trackrs::terrain {
float morphFactor(float distance, unsigned cells, float spacing) {
  const float radius = 8.f * static_cast<float>(cells) * spacing;
  return std::clamp((distance - .7f * radius) / (.3f * radius), 0.f, 1.f);
}
glm::vec2 morphPosition(const Patch &patch, glm::vec2 grid, glm::vec2 camera,
                        float spacing) {
  glm::vec2 origin(patch.key.x, patch.key.y);
  const float step = static_cast<float>(patch.cells) / GridCells;
  auto pixel = origin + grid * step;
  float factor =
      morphFactor(glm::length(pixel * spacing - camera), patch.cells, spacing);
  return (origin + (grid - glm::mod(grid, glm::vec2(2)) * factor) * step) *
         spacing;
}
bool outside(const glm::mat4 &vp, glm::vec3 minimum, glm::vec3 maximum) {
  for (int axis = 0; axis < 3; ++axis)
    for (float sign : {-1.f, 1.f}) {
      auto plane = glm::row(vp, 3) + sign * glm::row(vp, axis);
      glm::vec3 positive(plane.x >= 0 ? maximum.x : minimum.x,
                         plane.y >= 0 ? maximum.y : minimum.y,
                         plane.z >= 0 ? maximum.z : minimum.z);
      if (glm::dot(glm::vec3(plane), positive) + plane.w < 0)
        return true;
    }
  return false;
}
Selection Selector::select(glm::vec3 camera, const glm::mat4 &vp, float spacing,
                           float distance, glm::vec2 heightRange,
                           unsigned minimumLevel) {
  if (minimumLevel > 8 || !std::isfinite(spacing) || spacing <= 0 ||
      !std::isfinite(distance) || distance <= 0 ||
      distance / spacing > 1048576 || !std::isfinite(camera.x) ||
      !std::isfinite(camera.y) || std::abs(camera.x / spacing) > 100000000 ||
      std::abs(camera.y / spacing) > 100000000)
    throw std::invalid_argument("invalid CDLOD domain");
  unsigned root = GridCells << minimumLevel;
  int level = static_cast<int>(minimumLevel);
  while (static_cast<float>(root) * spacing < distance * 2) {
    root *= 2;
    ++level;
  }
  Selection result;
  auto visit = [&](auto &&self, int x, int y, unsigned cells, int lod) -> void {
    ++result.tested;
    glm::vec2 minimum = glm::vec2(x, y) * spacing;
    glm::vec2 maximum = minimum + static_cast<float>(cells) * spacing;
    float nearest = glm::length(
        glm::vec2(camera) - glm::clamp(glm::vec2(camera), minimum, maximum));
    if (nearest > distance ||
        outside(vp, {minimum, heightRange.x}, {maximum, heightRange.y})) {
      ++result.culled;
      return;
    }
    if (lod > static_cast<int>(minimumLevel) &&
        nearest < 4.f * static_cast<float>(cells) * spacing) {
      const auto half = cells / 2;
      for (int dy = 0; dy < 2; ++dy)
        for (int dx = 0; dx < 2; ++dx)
          self(self, x + dx * static_cast<int>(half),
               y + dy * static_cast<int>(half), half, lod - 1);
    } else {
      TileKey key{x, y, lod};
      result.patches.push_back(
          tiles_.get(key, [&] { return Patch{key, cells}; }));
    }
  };
  const float span = static_cast<float>(root) * spacing;
  int x0 = static_cast<int>(std::floor((camera.x - distance) / span));
  int y0 = static_cast<int>(std::floor((camera.y - distance) / span));
  int x1 = static_cast<int>(std::floor((camera.x + distance) / span));
  int y1 = static_cast<int>(std::floor((camera.y + distance) / span));
  for (int y = y0; y <= y1; ++y)
    for (int x = x0; x <= x1; ++x)
      visit(visit, x * static_cast<int>(root), y * static_cast<int>(root), root,
            level);
  return result;
}
} // namespace trackrs::terrain
