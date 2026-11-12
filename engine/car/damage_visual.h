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
  std::array<float, 4> corner{}; ///< 0..1 (simulation values above 1 clamp)
};
/// Model-space body footprint: x left→right, y rear→front (as PDamage).
struct BodyExtent {
  glm::vec2 min{-1}, max{1};
};
/// Clamps simulation damage (which can exceed 1) into a DamageState.
DamageState fromSimulation(float fl, float fr, float rl, float rr);
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
