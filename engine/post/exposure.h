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
/// SSAO hemisphere kernel in tangent space (+z = surface normal): `count`
/// directions on a golden-angle spiral, lengths growing quadratically so most
/// samples stay near the pixel. Deterministic; all within the unit hemisphere.
std::vector<glm::vec3> ssaoKernel(int count);
/// Soft-knee bright-pass weight used by the first bloom downsample.
float bloomPrefilter(float brightness, float threshold = 1.f, float knee = .5f);

// ---- Phase 3e temporal anti-aliasing ------------------------------------------
/// Radical inverse in `base` of `index` (Halton sequence), in [0, 1).
float halton(unsigned index, unsigned base);
/// Sub-pixel jitter for frame `frame`, in pixels, in [-0.5, 0.5)^2: Halton(2,3)
/// over an 8-frame cycle starting at index 1 (index 0 would be the origin).
glm::vec2 taaJitter(unsigned frame);
/// Clip-space translation that shifts a projection by `pixels` on a viewport of
/// `size`: jittered = taaJitterMatrix(...) * projection.
glm::mat4 taaJitterMatrix(glm::vec2 pixels, glm::ivec2 size);
/// History weight of the resolve (the current frame gets 1 - this).
constexpr float TaaFeedback = .9f;
} // namespace trackrs::post
