// SPDX-License-Identifier: GPL-2.0-or-later
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "engine/world/atmosphere.h"
#include "vendor/doctest/doctest.h"
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
using namespace trackrs::world;
TEST_CASE("one periodic sun state feeds world lighting") {
  for (float h : {0.f, 6.f, 12.f, 17.f, 18.5f, 22.f, 24.f}) {
    auto s = sunlight(h);
    CHECK(glm::length(s.direction) == doctest::Approx(1));
    CHECK(s.daylight >= 0);
    CHECK(s.daylight <= 1);
    CHECK(s.radiance.x >= 0);
  }
  CHECK(glm::length(sunlight(0).direction - sunlight(24).direction) < 1e-5);
  CHECK(sunlight(12).radiance.x > sunlight(17).radiance.x);
  CHECK(sunlight(22).radiance.x == 0);
  CHECK(sunlight(18.5f).skyRadiance.x > 0);
  CHECK(sunlight(22).skyRadiance.x == 0);
  CHECK(presetHour("dusk") == 18.5f);
  CHECK_THROWS(presetHour("invalid"));
}
TEST_CASE("analytic height fog matches numerical line integral including "
          "horizontal rays") {
  for (float a : {-20.f, 0.f, 100.f})
    for (float b : {-20.f, 0.f, 100.f}) {
      double numeric = 0;
      for (int i = 0; i < 10000; ++i)
        numeric += .003 * 500 *
                   std::exp(-.006 * (a + (b - a) * (i + .5) / 10000)) / 10000;
      CHECK(fogOpticalDepth(.003f, .006f, a, b, 500) ==
            doctest::Approx(numeric).epsilon(1e-5));
    }
  CHECK(fogOpticalDepth(.003f, 0, 50, 50, 0) == 0);
}
TEST_CASE("cascades cover split corners and remain stable under subtexel "
          "camera movement") {
  auto view = glm::lookAt(glm::vec3(10, -20, 5), glm::vec3(10, 0, 5),
                          glm::vec3(0, 0, 1));
  auto projection =
      glm::perspective(glm::radians(60.f), 16.f / 9.f, .1f, 10000.f);
  for (unsigned count : {3u, 4u}) {
    auto a = makeCascades({view, projection}, {.2f, .5f, 1}, count, 2048, 768);
    auto b = makeCascades(
        {view * glm::translate(glm::mat4(1), glm::vec3(.00001f, 0, 0)),
         projection},
        {.2f, .5f, 1}, count, 2048, 768);
    float previous = .1f;
    for (unsigned i = 0; i < count; ++i) {
      CHECK(a.splits[i] > previous);
      CHECK(a.bias[i] > 0);
      for (float depth : {previous, a.splits[i]})
        for (float y : {-1.f, 1.f})
          for (float x : {-1.f, 1.f}) {
            auto world = glm::inverse(view) *
                         glm::vec4(x * depth * .5773503f * 16.f / 9.f,
                                   y * depth * .5773503f, -depth, 1);
            auto clip = a.matrices[i] * world;
            CHECK(std::abs(clip.x) <= 1.001f);
            CHECK(std::abs(clip.y) <= 1.001f);
            CHECK(std::abs(clip.z) <= 1.001f);
          }
      for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 2; ++r)
          CHECK(std::abs(a.matrices[i][c][r] - b.matrices[i][c][r]) < 1e-5);
      previous = a.splits[i];
    }
    CHECK(a.splits[count - 1] == doctest::Approx(768));
  }
}

TEST_CASE("solar motion does not flip the stabilised shadow basis") {
  auto view = glm::lookAt(glm::vec3(10, -20, 5), glm::vec3(10, 0, 5),
                          glm::vec3(0, 0, 1));
  auto projection =
      glm::perspective(glm::radians(60.f), 16.f / 9.f, .1f, 10000.f);
  glm::vec3 previous(0);
  for (int step = 0; step <= 240; ++step) {
    auto cascades =
        makeCascades({view, projection}, sunlight(float(step) * .1f).direction,
                     3, 2048, 768);
    auto matrix = cascades.matrices[0];
    glm::vec3 axis =
        glm::normalize(glm::vec3(matrix[0][0], matrix[1][0], matrix[2][0]));
    if (step)
      CHECK(glm::dot(axis, previous) > .99f);
    previous = axis;
  }
}
TEST_CASE("stage weather maps to bounded presentation state") {
  auto dry = weatherFromLevel(0, 0, .01f);
  CHECK(dry.wetness == 0);
  CHECK(dry.snow == 0);
  CHECK(dry.overcast == 0);
  CHECK(dry.fogDensity == doctest::Approx(.003f)); // old default -> Phase 3b constant
  auto light = weatherFromLevel(140, 0, .007f), heavy = weatherFromLevel(4000, 0, .007f);
  CHECK(light.wetness > 0);
  CHECK(heavy.wetness > light.wetness);
  CHECK(heavy.puddles > light.puddles);
  CHECK(heavy.overcast > light.overcast);
  for (const auto &w : {light, heavy, weatherFromLevel(0, 800, .02f), weatherFromLevel(1e9f, 1e9f, 1e9f),
                        weatherFromLevel(-5, std::nanf(""), -1)})
    for (float v : {w.wetness, w.puddles, w.snow, w.overcast}) {
      CHECK(v >= 0);
      CHECK(v <= 1);
    }
  CHECK(weatherFromLevel(0, 800, .002f).snow > .5f);
  CHECK(weatherFromLevel(0, 0, 1e9f).fogDensity <= .012f);
  CHECK(weatherFromLevel(0, 0, 0).fogDensity >= .0005f);
  CHECK(weatherPreset("level", heavy).puddles == heavy.puddles);
  CHECK(weatherPreset("clear", heavy).wetness == 0);
  CHECK(weatherPreset("rain", dry).puddles > 0);
  CHECK(weatherPreset("snow", dry).snow > 0);
  CHECK(weatherPreset("fog", dry).fogDensity > weatherPreset("clear", dry).fogDensity);
  CHECK_THROWS(weatherPreset("hail", dry));
  CHECK(validWeatherName("rain"));
  CHECK_FALSE(validWeatherName("hail"));
}
TEST_CASE("cloud cover dims the beam but keeps a sky source") {
  auto clear = sunlight(12), cloudy = overcastSun(clear, 1);
  CHECK(cloudy.radiance.x < .2f * clear.radiance.x);
  CHECK(cloudy.skyRadiance.x > 0);
  CHECK(cloudy.ambient.x > clear.ambient.x);
  CHECK(overcastSun(clear, 0).radiance == clear.radiance);
  CHECK(overcastSun(clear, std::nanf("")).radiance == clear.radiance);
  CHECK(glm::length(cloudy.direction - clear.direction) < 1e-6f);
}
TEST_CASE("headlights sit at the car nose, aim forward and slightly down") {
  const glm::vec3 p(10, 20, 3), right(1, 0, 0), forward(0, 1, 0), up(0, 0, 1);
  auto lamps = headlights({p, right, forward, up});
  CHECK(lamps[0].position.x < p.x);
  CHECK(lamps[1].position.x > p.x);
  for (const auto &l : lamps) {
    CHECK(l.position.y > p.y + 1.5f);
    CHECK(glm::length(l.direction) == doctest::Approx(1));
    CHECK(l.direction.y > .99f);
    CHECK(l.direction.z < 0);
    CHECK(l.cosInner > l.cosOuter);
    CHECK(l.radiance.x > 0);
  }
  // Rotated body: lamps follow the body axes.
  auto turned = headlights({p, {0, -1, 0}, {1, 0, 0}, up});
  CHECK(turned[0].direction.x > .99f);
  CHECK(headlightsOn(sunlight(22), 0));
  CHECK_FALSE(headlightsOn(sunlight(12), 0));
  CHECK(headlightsOn(overcastSun(sunlight(17), 1), 1));
}
TEST_CASE("headlights stay off on a bright overcast noon") {
  CHECK_FALSE(headlightsOn(overcastSun(sunlight(12), .85f), .85f));
}
