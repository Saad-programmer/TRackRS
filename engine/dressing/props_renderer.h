// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
/**
 * Stage props and animals (amendment A3.5) — GL 3.3 instanced renderer.
 * Ownership: one VAO, a static VBO/IBO (15 kinds x 3 LODs), an orphaned
 * instance stream and 1x1 default textures. Main (GL) thread only.
 * Performance: one glDrawElementsInstancedBaseVertex per non-empty (kind, LOD).
 * LOD and draw distances scale with each kind's size, so a kasbah stays in
 * view far beyond a goat. Shadows: LOD >= 1 within 250 m. Probes skip props.
 * Animals are re-posed on the CPU each frame (O(animals), no allocation after
 * warm-up); the walk cycle and grazing nod run in props.vert.
 */
#include "engine/core/render_quality.h"
#include "engine/vegetation/vegetation.h"
#include "props.h"
#include <filesystem>
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <vector>
namespace trackrs::dressing {
class PropsRenderer {
  struct Impl;
  std::unique_ptr<Impl> impl_;

public:
  explicit PropsRenderer(const std::filesystem::path &data);
  ~PropsRenderer();
  PropsRenderer(const PropsRenderer &) = delete;
  PropsRenderer &operator=(const PropsRenderer &) = delete;
  void setProps(std::vector<Prop> props, std::function<float(float, float)> height, PlacementReport placement);
  /// Sign atlas (RGBA8, top row first, SignColumns x SignRows cells), shown on
  /// sign_board props that carry a face. Uploaded lazily on the GL thread.
  void setSignAtlas(std::vector<std::uint8_t> rgba, int width, int height);
  std::size_t draw(glm::vec3 camera, const glm::mat4 &viewProjection, vegetation::Pass pass, RenderQuality quality,
                   bool linearCapture, float time);
  nlohmann::json report() const;
};
/// Writes the active props renderer's report (validation runs).
void writePropsReport(const std::filesystem::path &path);
} // namespace trackrs::dressing
