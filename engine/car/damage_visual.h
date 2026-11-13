// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
/**
 * Phase 4c damage visuals: pure functions shared by the car renderer and its
 * tests. The input is the existing simulation damage (PDamage: four corner
 * values, front-left, front-right, rear-left, rear-right). This code only
 * reads it, so physics and state hashes are unchanged.
 *
 * damageAt() is mirrored exactly by car.vert/pbr.frag (keep them in step):
 * each corner's damage spreads over the body with a smooth falloff from that
 * corner, and the strongest corner wins.
 */
#include <array>
#include <glm/glm.hpp>
namespace trackrs::car {
/// Corner order matches PDamage::DamageSide.
enum class Corner { FrontLeft = 0, FrontRight, RearLeft, RearRight };
struct DamageState {
  std::array<float, 4> corner{}; ///< visual severity 0..1
};
/// Simulation damage accumulates slowly (impact force × 1e-7 per contact):
/// measured on `ahead` with tests/replays/crash_rollover.csv (14 flat-out
/// flicks with rolls), the worst corner reached 0.094. Visual severity is
/// therefore gain × simulation damage, clamped, so such a session reads as
/// heavily damaged. Physics keeps the raw value. The gain is the setting
/// graphics.damagegain (default below), which a car may override with
/// "damage.visual_gain" in car.json. Phase 6 backlog: when the damage model
/// is rebuilt to mean severity 0..1, switch to 1:1 and remove the gain.
constexpr float DefaultVisualGain = 6.f;
constexpr float MinVisualGain = .5f, MaxVisualGain = 20.f;
/// The gain in force for a car: its own override (> 0) or the global setting,
/// clamped to [MinVisualGain, MaxVisualGain].
float effectiveGain(float perCar, float setting);
/// Model-space body footprint: x left→right, y rear→front (as PDamage).
struct BodyExtent {
  glm::vec2 min{-1}, max{1};
};
/// Simulation damage (PDamage values, can exceed 1) to visual severity:
/// clamp(gain × value, 0, 1).
DamageState fromSimulation(float fl, float fr, float rl, float rr, float gain = DefaultVisualGain);
/// Visual severity given directly (validation --damage-visual), clamped 0..1.
DamageState fromVisual(float fl, float fr, float rl, float rr);
/// Damage felt at a model-space point (x, y): 0..1.
float damageAt(const DamageState &d, const BodyExtent &e, glm::vec2 xy);
/// Discrete consequences of heavy damage.
struct DamageEffects {
  std::array<bool, 2> headlightBroken{}; ///< left, right (front corners >= 0.8)
  std::array<bool, 2> taillightBroken{}; ///< left, right (rear corners >= 0.8)
  float smoke = 0;                       ///< engine smoke 0..1 from front damage above 0.55
};
DamageEffects effects(const DamageState &d);
} // namespace trackrs::car
