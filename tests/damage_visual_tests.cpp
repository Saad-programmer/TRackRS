// SPDX-License-Identifier: GPL-2.0-or-later
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "engine/car/damage_visual.h"
#include "vendor/doctest/doctest.h"
using namespace trackrs::car;

TEST_CASE("simulation damage is scaled by the visual gain and clamped") {
  const auto d = fromSimulation(-0.02f, 0.05f, 3.f, 0.094f);
  CHECK(d.corner[0] == 0.f);
  CHECK(d.corner[1] == doctest::Approx(.05f * VisualGain));
  CHECK(d.corner[2] == 1.f);
  CHECK(d.corner[3] > .5f);             // the measured crash session reads as heavy
  const auto v = fromVisual(-1.f, .4f, 2.f, 1.f);
  CHECK(v.corner[0] == 0.f);
  CHECK(v.corner[1] == doctest::Approx(.4f));
  CHECK(v.corner[2] == 1.f);
}

TEST_CASE("damage is strongest at its corner and fades across the body") {
  const BodyExtent e{{-80, -200}, {80, 160}};
  const auto d = fromVisual(1.f, 0.f, 0.f, 0.f); // front-left only
  CHECK(damageAt(d, e, {-80, 160}) == doctest::Approx(1.f));
  CHECK(damageAt(d, e, {-75, 155}) == doctest::Approx(1.f)); // inside the full-strength core
  const float partial = damageAt(d, e, {-40, 80});            // a quarter of the way along
  CHECK(partial > .3f);
  CHECK(partial < .9f);
  CHECK(damageAt(d, e, {0, -20}) == 0.f);                     // mid-body is past the falloff
  CHECK(damageAt(d, e, {80, -200}) == 0.f); // opposite corner untouched
  CHECK(damageAt(fromVisual(0, 0, 0, 0), e, {-80, 160}) == 0.f);
}

TEST_CASE("each corner maps to its own side of the car") {
  const BodyExtent e{{-1, -2}, {1, 2}};
  CHECK(damageAt(fromVisual(0, 1, 0, 0), e, {1, 2}) == doctest::Approx(1.f));  // front-right
  CHECK(damageAt(fromVisual(0, 0, 1, 0), e, {-1, -2}) == doctest::Approx(1.f)); // rear-left
  CHECK(damageAt(fromVisual(0, 0, 0, 1), e, {1, -2}) == doctest::Approx(1.f));  // rear-right
  CHECK(damageAt(fromVisual(0, 0, 0, 1), e, {-1, 2}) == 0.f);
}

TEST_CASE("heavy damage breaks lamps and makes the engine smoke") {
  auto e = effects(fromVisual(.85f, .3f, .1f, .95f));
  CHECK(e.headlightBroken[0]);
  CHECK_FALSE(e.headlightBroken[1]);
  CHECK_FALSE(e.taillightBroken[0]);
  CHECK(e.taillightBroken[1]);
  CHECK(e.smoke == doctest::Approx((.85f - .55f) / .45f));
  CHECK(effects(fromVisual(.5f, .5f, 1, 1)).smoke == 0.f); // rear damage alone does not smoke
  CHECK(effects(fromVisual(1, 0, 0, 0)).smoke == 1.f);
}
