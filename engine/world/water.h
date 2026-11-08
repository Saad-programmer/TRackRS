// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
/**
 * PBR stage water (Phase 3c item 4).
 *
 * Purpose: replaces the legacy translucent textured sheet in the PBR world
 * with a physically based surface: Schlick Fresnel sky reflection from the
 * shared world IBL, GGX sun glint with CSM visibility, headlight glints,
 * depth-based Beer-Lambert absorption, shoreline foam, rain ripples and fog.
 * Ownership: the grid builder is pure CPU (value types, unit-tested); the GL
 * resources live in world::Renderer and are released by world::shutdown().
 * Thread-safety: main (GL) thread only for drawWater().
 * Performance: one indexed draw. The camera-snapped grid is 121x121 vertices
 * (12 B each, ~170 KB streamed per frame via buffer orphaning); indices are
 * static. Terrain depth is sampled on the CPU from the physics heightmap, the
 * same single source of truth the legacy water alpha used.
 */
#include <functional>
#include <glm/glm.hpp>
#include <vector>
namespace trackrs::world {
struct WaterVertex {
  float x, y, depth; ///< world xy; water height minus terrain height (m)
};
constexpr int WaterGridHalf = 60;      // cells on each side of the camera
constexpr float WaterGridSpacing = 8;  // metres; 960 m square
/// Camera-snapped grid (snapping keeps vertices fixed in world space so depth
/// sampling never swims). Reuses `out`'s capacity.
void buildWaterGrid(glm::vec2 camera, float waterHeight,
                    const std::function<float(float, float)> &terrainHeight,
                    std::vector<WaterVertex> &out);
/// Triangle-list indices for the (2*half+1)^2 grid, counter-clockwise from +z.
std::vector<std::uint32_t> waterGridIndices();
/// Draws the water for this frame. `time` is the deterministic animation clock.
void drawWater(const glm::mat4 &viewProjection, glm::vec3 camera, float waterHeight,
               const std::function<float(float, float)> &terrainHeight, float time);
} // namespace trackrs::world
