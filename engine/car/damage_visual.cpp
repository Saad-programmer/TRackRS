// SPDX-License-Identifier: GPL-2.0-or-later
#include "damage_visual.h"
#include <algorithm>
namespace trackrs::car {
namespace {
float smoothstep(float a, float b, float x) {
  const float t = std::clamp((x - a) / (b - a), 0.f, 1.f);
  return t * t * (3.f - 2.f * t);
}
} // namespace

DamageState fromVisual(float fl, float fr, float rl, float rr) {
  DamageState d;
  d.corner = {std::clamp(fl, 0.f, 1.f), std::clamp(fr, 0.f, 1.f), std::clamp(rl, 0.f, 1.f), std::clamp(rr, 0.f, 1.f)};
  return d;
}

DamageState fromSimulation(float fl, float fr, float rl, float rr) {
  return fromVisual(fl * VisualGain, fr * VisualGain, rl * VisualGain, rr * VisualGain);
}

float damageAt(const DamageState &d, const BodyExtent &e, glm::vec2 xy) {
  const glm::vec2 size = glm::max(e.max - e.min, glm::vec2(1e-4f));
  const std::array<glm::vec2, 4> corners = {glm::vec2(e.min.x, e.max.y), glm::vec2(e.max.x, e.max.y),
                                            glm::vec2(e.min.x, e.min.y), glm::vec2(e.max.x, e.min.y)};
  float strongest = 0;
  for (std::size_t i = 0; i < 4; ++i) {
    // Distance in body-normalised units: 0 at the corner, ~1.4 at the far one.
    const float distance = glm::length((xy - corners[i]) / size);
    strongest = std::max(strongest, d.corner[i] * (1.f - smoothstep(.12f, .7f, distance)));
  }
  return strongest;
}

DamageEffects effects(const DamageState &d) {
  DamageEffects e;
  e.headlightBroken = {d.corner[0] >= .8f, d.corner[1] >= .8f};
  e.taillightBroken = {d.corner[2] >= .8f, d.corner[3] >= .8f};
  e.smoke = std::clamp((std::max(d.corner[0], d.corner[1]) - .55f) / .45f, 0.f, 1.f);
  return e;
}
} // namespace trackrs::car
