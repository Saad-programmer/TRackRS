// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
/**
 * GL 3.3 renderer for terrain::Tracks (amendment A3.3).
 * Ownership: one VAO, a vertex buffer sized for the ring plus the static
 * trails, and a static index buffer. Main (GL) thread only.
 * Each segment is a ground-hugging quad; tracks.frag rebuilds the rut, lip
 * and print relief per pixel and multiplies the lit terrain by the relief's
 * shading ratio (same albedo, so no material lookup is needed). Only newly
 * written ring slots are re-uploaded (glBufferSubData), so a frame costs at
 * most two draws and a few hundred bytes of upload while driving.
 */
#include "tracks.h"
#include <filesystem>
#include <functional>
#include <memory>
namespace trackrs::terrain {
class TracksRenderer {
  struct Impl;
  std::unique_ptr<Impl> impl_;

public:
  TracksRenderer(const std::filesystem::path &data, std::function<float(float, float)> height);
  ~TracksRenderer();
  TracksRenderer(const TracksRenderer &) = delete;
  TracksRenderer &operator=(const TracksRenderer &) = delete;
  /// Draws `tracks` into the bound (HDR) target. Returns draw calls.
  std::size_t draw(const Tracks &tracks, glm::vec3 camera, const glm::mat4 &viewProjection);
};
} // namespace trackrs::terrain
