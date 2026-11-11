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
 *
 * Phase 3e TAA (Settings::taa): the scene projection is jittered by a Halton
 * sub-pixel offset each frame (jitterMatrix(), applied by the caller); end()
 * first resolves the frame against a reprojected, neighbourhood-clamped
 * history (RGBA16F ping-pong), then runs exposure/bloom/SSAO on the result.
 * Static scenery reprojects from depth; cars write motion vectors between
 * beginVelocity()/endVelocity(). A light sharpen in the composite restores
 * the detail the resolve softens. +1 fullscreen pass, +16 B/px (2 history
 * targets) + 8 B/px velocity.
 */
#include <filesystem>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
namespace trackrs::post {
struct Settings {
  bool enabled = true;      ///< only effective with the PBR world renderer
  bool autoExposure = true;
  bool bloom = true;
  float bloomIntensity = .06f;
  float vignette = .18f;
  bool ambientOcclusion = true;
  float aoStrength = .65f, aoRadius = .6f; ///< radius in metres at 25 m
  bool taa = true;          ///< graphics.aa = taa|off
  float sharpen = .25f;     ///< composite sharpen strength while TAA is on
};
void configure(Settings settings);
Settings settings();
/// Binds the HDR target for a width x height frame (resizes on change).
void begin(int width, int height, const std::filesystem::path &data);
/// Resolves to the default framebuffer. `time` is the deterministic animation
/// clock; adaptation uses its delta since the previous resolve. `projection`
/// is the (jittered) scene projection and `view` the scene view (SSAO
/// reconstructs view space from depth; TAA reprojects with both).
void end(float time, const glm::mat4 &projection, const glm::mat4 &view);
bool active();
/// True while TAA runs this frame (post active and enabled).
bool temporal();
/// Frame index for temporally varying patterns (0 while TAA is off).
unsigned temporalFrame();
/// This frame's jitter, to left-multiply onto the scene projection (identity
/// while TAA is off).
glm::mat4 jitterMatrix();
/// Motion-vector pass for moving objects. begin binds the velocity target
/// (sharing the scene depth, cleared to "no motion") and returns false when
/// TAA is off. Matrices: the jittered current view-projection, the unjittered
/// current one and the previous frame's unjittered one.
struct VelocityPass {
  bool active = false;
  glm::mat4 jittered{1}, current{1}, previous{1};
};
VelocityPass beginVelocity(const glm::mat4 &view, const glm::mat4 &jitteredProjection);
void endVelocity();
void shutdown();
nlohmann::json report();
void writeReport(const std::filesystem::path &path);
} // namespace trackrs::post
