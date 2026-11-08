// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
/**
 * Phase 3d post-processing — CPU reference math (no GL; unit-tested).
 * The GLSL in data/shaders/post mirrors these functions exactly; tests pin the
 * constants so the two cannot drift silently.
 * Thread-safety: pure functions.
 */
#include <array>
#include <glm/glm.hpp>
#include <vector>
namespace trackrs::post {
struct ExposureSettings {
  float key = .16f;                     // target middle grey after exposure
  float minExposure = .35f, maxExposure = 3.f;
  float speedUp = 3.f, speedDown = 1.2f; // 1/s (pupil closes faster than it opens)
};
/// Exposure that maps the scene's geometric-mean luminance to `key`.
float targetExposure(float averageLuminance, const ExposureSettings &s = {});
/// Frame-rate independent exponential adaptation of the *luminance* estimate.
float adaptLuminance(float previous, float target, float dt, const ExposureSettings &s = {});
/// Dual-filter bloom pyramid: successive half resolutions starting at half
/// the frame, stopping at `levels` or when a side would drop below 8 px.
std::vector<glm::ivec2> bloomChain(glm::ivec2 frame, int levels = 6);
/// Soft-knee bright-pass weight used by the first bloom downsample.
float bloomPrefilter(float brightness, float threshold = 1.f, float knee = .5f);
} // namespace trackrs::post
