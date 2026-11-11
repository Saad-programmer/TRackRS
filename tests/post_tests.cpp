// SPDX-License-Identifier: GPL-2.0-or-later
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "engine/post/exposure.h"
#include "vendor/doctest/doctest.h"
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
using namespace trackrs::post;
TEST_CASE("exposure maps mean luminance to the key and stays bounded") {
  const ExposureSettings s;
  CHECK(targetExposure(s.key, s) == doctest::Approx(1));
  CHECK(targetExposure(1e6f, s) == s.minExposure);
  CHECK(targetExposure(1e-6f, s) == s.maxExposure);
  CHECK(targetExposure(0, s) == 1);
  CHECK(targetExposure(std::nanf(""), s) == 1);
  CHECK(targetExposure(.08f, s) > targetExposure(.32f, s));
}
TEST_CASE("adaptation is frame-rate independent and asymmetric") {
  const ExposureSettings s;
  float one = adaptLuminance(.1f, 1.f, .2f, s);
  float many = .1f;
  for (int i = 0; i < 50; ++i)
    many = adaptLuminance(many, 1.f, .004f, s);
  CHECK(one == doctest::Approx(many).epsilon(1e-4));
  CHECK(one > .1f);
  CHECK(one < 1.f);
  const float brighter = adaptLuminance(.5f, 1.f, .1f, s) - .5f, darker = .5f - adaptLuminance(.5f, 0.f, .1f, s);
  CHECK(brighter / .5f > darker / .5f); // eyes close faster than they open
  CHECK(adaptLuminance(0, .3f, .1f, s) == .3f);   // first frame snaps
  CHECK(adaptLuminance(.2f, .3f, 0, s) == .2f);
  CHECK(adaptLuminance(.2f, .3f, std::nanf(""), s) == .2f);
}
TEST_CASE("bloom chain halves until 8 px") {
  auto chain = bloomChain({1920, 1080});
  REQUIRE(chain.size() == 6);
  CHECK(chain[0] == glm::ivec2(960, 540));
  CHECK(chain[5] == glm::ivec2(30, 16));
  CHECK(bloomChain({64, 64}).size() == 3);
  CHECK(bloomChain({8, 8}).empty());
}
TEST_CASE("soft-knee prefilter passes highlights and rejects midtones") {
  CHECK(bloomPrefilter(.2f) == doctest::Approx(0));
  CHECK(bloomPrefilter(.75f) > 0);
  CHECK(bloomPrefilter(.75f) < bloomPrefilter(1.2f));
  CHECK(bloomPrefilter(10.f) == doctest::Approx(.9f));
}
TEST_CASE("SSAO kernel stays in the unit hemisphere and favours short samples") {
  auto k = ssaoKernel(12);
  REQUIRE(k.size() == 12);
  float previous = 0;
  for (const auto &s : k) {
    CHECK(s.z > 0);
    CHECK(glm::length(s) <= 1.0001f);
    CHECK(glm::length(s) >= previous - 1e-5f);
    previous = glm::length(s);
  }
  CHECK(glm::length(k.front()) < .3f);
  CHECK(ssaoKernel(12) == k); // deterministic
  CHECK(ssaoKernel(0).empty());
}

TEST_CASE("TAA jitter: Halton(2,3), 8-frame cycle, sub-pixel, zero mean") {
  using namespace trackrs::post;
  CHECK(halton(1, 2) == doctest::Approx(.5f));
  CHECK(halton(2, 2) == doctest::Approx(.25f));
  CHECK(halton(3, 2) == doctest::Approx(.75f));
  CHECK(halton(1, 3) == doctest::Approx(1.f / 3.f));
  CHECK(halton(5, 3) == doctest::Approx(7.f / 9.f));
  glm::vec2 sum(0.f);
  for (unsigned f = 0; f < 8; ++f) {
    const auto j = taaJitter(f);
    CHECK(j.x >= -.5f);
    CHECK(j.x < .5f);
    CHECK(j.y >= -.5f);
    CHECK(j.y < .5f);
    CHECK(taaJitter(f + 8) == j); // cycle
    for (unsigned g = 0; g < f; ++g)
      CHECK(taaJitter(g) != j);   // distinct samples
    sum += j;
  }
  CHECK(std::abs(sum.x / 8.f) < .07f);
  CHECK(std::abs(sum.y / 8.f) < .07f);
  // The matrix shifts a projected point by exactly the jitter in pixels.
  const glm::ivec2 size(640, 480);
  const glm::mat4 projection = glm::perspective(.8f, 640.f / 480.f, .1f, 1000.f);
  const glm::vec4 p = projection * glm::vec4(1.f, 2.f, -20.f, 1.f);
  const glm::vec4 q = taaJitterMatrix({.25f, -.5f}, size) * p;
  const glm::vec2 shift = (glm::vec2(q) / q.w - glm::vec2(p) / p.w) * .5f * glm::vec2(size);
  CHECK(shift.x == doctest::Approx(.25f));
  CHECK(shift.y == doctest::Approx(-.5f));
  CHECK(q.z == doctest::Approx(p.z)); // depth untouched
  CHECK(taaJitterMatrix({.3f, .3f}, {0, 0}) == glm::mat4(1.f));
}
