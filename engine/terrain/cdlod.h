// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "engine/core/lru_cache.h"
#include <array>
#include <glm/glm.hpp>
#include <vector>
namespace trackrs::terrain {
constexpr unsigned GridCells = 32;
struct Patch {
  TileKey key;
  unsigned cells = GridCells;
};
struct Selection {
  std::vector<Patch> patches;
  unsigned tested = 0, culled = 0;
};
class Selector {
  LruCache<TileKey, Patch, TileHash> tiles_{2048};

public:
  Selection select(glm::vec3 camera, const glm::mat4 &viewProjection,
                   float spacing, float distance, glm::vec2 heightRange,
                   unsigned minimumLevel = 0);
  std::size_t residentTiles() const { return tiles_.size(); }
};
float morphFactor(float distance, unsigned cells, float spacing);
glm::vec2 morphPosition(const Patch &patch, glm::vec2 grid, glm::vec2 camera,
                        float spacing);
bool outside(const glm::mat4 &viewProjection, glm::vec3 minimum,
             glm::vec3 maximum);
} // namespace trackrs::terrain
