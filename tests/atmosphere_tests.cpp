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
