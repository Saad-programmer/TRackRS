// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
/**
 * Phase 3d HDR post-processing for the PBR world.
 *
 * Purpose: the 3D scene renders linear HDR into an RGBA16F target; end()
 * resolves it to the window: histogram-free auto-exposure (log-luminance mip
 * chain + temporal adaptation on the GPU, no read-back), dual-filter bloom,
 * ACES fit, gamma 2.2 and a light vignette. The HUD is drawn afterwards.
 * Ownership: one process-wide processor owns its FBOs/textures/programs; they
 * are created lazily on the GL thread and released by shutdown().
 * Thread-safety: main (GL) thread only.
 * Performance: bloom works at half resolution and below in R11F_G11F_B10F
 * (4 B/px); luminance is reduced at 64x64. ~2 + 2*levels fullscreen passes.
 */
#include <filesystem>
#include <nlohmann/json.hpp>
namespace trackrs::post {
struct Settings {
  bool enabled = true;      ///< only effective with the PBR world renderer
  bool autoExposure = true;
  bool bloom = true;
  float bloomIntensity = .06f;
  float vignette = .18f;
};
void configure(Settings settings);
Settings settings();
/// Binds the HDR target for a width x height frame (resizes on change).
void begin(int width, int height, const std::filesystem::path &data);
/// Resolves to the default framebuffer. `time` is the deterministic animation
/// clock; adaptation uses its delta since the previous resolve.
void end(float time);
bool active();
void shutdown();
nlohmann::json report();
void writeReport(const std::filesystem::path &path);
} // namespace trackrs::post
