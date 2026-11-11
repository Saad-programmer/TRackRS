// SPDX-License-Identifier: GPL-2.0-or-later
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "engine/car/car_assets.h"
#include "vendor/doctest/doctest.h"
#include <cmath>
#include <vector>
using namespace trackrs::car;
namespace {
void fill(Image &im, int x0, int y0, int x1, int y1, std::array<std::uint8_t, 4> c) {
  for (int y = y0; y < y1; ++y)
    for (int x = x0; x < x1; ++x)
      std::copy(c.begin(), c.end(), im.at(x, y));
}
// A synthetic atlas: blue paint (with shading), a white second paint colour,
// a smooth dark window, a textured dark tyre, a chrome checker, a number panel.
Image atlas() {
  Image im = makeImage(128, 128, {40, 70, 160, 255});
  for (int y = 0; y < 64; ++y)
    for (int x = 0; x < 64; ++x) { // shaded paint
      auto *p = im.at(x, y);
      const float s = .85f + .3f * static_cast<float>(y) / 64.f;
      p[0] = static_cast<std::uint8_t>(40 * s), p[1] = static_cast<std::uint8_t>(70 * s), p[2] = static_cast<std::uint8_t>(160 * s);
    }
  fill(im, 64, 0, 128, 32, {235, 235, 235, 255});   // second paint (white)
  fill(im, 0, 64, 40, 100, {12, 14, 18, 255});       // window: smooth dark
  for (int y = 64; y < 100; ++y)
    for (int x = 40; x < 80; ++x)                  // tyre tread: textured dark
      std::fill_n(im.at(x, y), 3, static_cast<std::uint8_t>((x + y) % 4 < 2 ? 10 : 50));
  for (int y = 100; y < 128; ++y)
    for (int x = 0; x < 40; ++x)                   // chrome: bright grey checker
      std::fill_n(im.at(x, y), 3, static_cast<std::uint8_t>((x / 2 + y / 2) % 2 ? 230 : 150));
  fill(im, 88, 96, 120, 120, {250, 250, 250, 255}); // number panel
  return im;
}
std::size_t countRole(const Roles &r, int channel) {
  std::size_t n = 0;
  for (int y = 0; y < r.map.height; ++y)
    for (int x = 0; x < r.map.width; ++x)
      n += r.map.at(x, y)[channel] ? 1 : 0;
  return n;
}
} // namespace

TEST_CASE("role classification separates paint, glass, rubber and metal") {
  const auto im = atlas();
  Image emission = makeImage(128, 128, {0, 0, 0, 255});
  fill(emission, 100, 40, 110, 50, {255, 30, 20, 255});
  const auto roles = classifyRoles(im, &emission);
  CHECK(roles.map.at(10, 10)[0] == 255);   // paint
  CHECK(roles.map.at(20, 80)[1] == 255);   // window -> glass
  CHECK(roles.map.at(60, 80)[2] == 255);   // tread -> rubber
  CHECK(roles.map.at(20, 115)[3] == 255);  // chrome -> metal
  CHECK(roles.lights.at(105, 45)[0] == 255); // emission -> tail light
  CHECK(roles.lights.at(10, 10)[0] == 0);
  // Every opaque texel gets exactly one material role.
  for (int y = 0; y < 128; ++y)
    for (int x = 0; x < 128; ++x) {
      const auto *r = roles.map.at(x, y);
      CHECK((r[0] ? 1 : 0) + (r[1] ? 1 : 0) + (r[2] ? 1 : 0) + (r[3] ? 1 : 0) == 1);
    }
  CHECK(roles.counts[1] == countRole(roles, 1));
  CHECK_THROWS(classifyRoles(Image{}, nullptr));
}

TEST_CASE("livery recolours paint, keeps shading, paints numbers and sponsors") {
  auto im = atlas();
  const auto roles = classifyRoles(im, nullptr);
  const auto dominant = dominantPaint(im, roles);
  CHECK(dominant[0].b > dominant[0].r); // blue is the largest paint cluster
  CHECK(dominant[1].r > .8f);           // white second
  const auto shadeBefore = static_cast<int>(im.at(10, 60)[2]) - static_cast<int>(im.at(10, 2)[2]);
  Livery l;
  l.primary = {.8f, .1f, .1f};
  l.number = 42;
  l.numberPanels.push_back({88.f / 128, 96.f / 128, 32.f / 128, 24.f / 128, 0});
  l.sponsorSlots.push_back({64.f / 128, 4.f / 128, 60.f / 128, 24.f / 128, 0});
  l.sponsorSeed = 7;
  applyLivery(im, roles, l);
  const auto *p = im.at(10, 10);
  CHECK(p[0] > 150);            // now red
  CHECK(p[2] < 60);
  const auto shadeAfter = static_cast<int>(im.at(10, 60)[0]) - static_cast<int>(im.at(10, 2)[0]);
  CHECK(shadeBefore > 0);
  CHECK(shadeAfter > 0);        // shading gradient survives
  CHECK(im.at(20, 80)[0] < 30); // glass untouched
  // Number panel now has dark digit strokes on the white panel.
  int dark = 0, light = 0;
  for (int y = 96; y < 120; ++y)
    for (int x = 88; x < 120; ++x)
      (im.at(x, y)[0] < 80 ? dark : light)++;
  CHECK(dark > 20);
  CHECK(light > dark);
  // Sponsor slot changed some white texels to logo colours deterministically.
  auto again = atlas();
  applyLivery(again, roles, l);
  CHECK(again.rgba == im.rgba);
}

TEST_CASE("stroke font, sponsor logos and livery parsing") {
  for (int n : {0, 7, 18, 99, 123, 999}) {
    Image im = makeImage(64, 32, {255, 255, 255, 255});
    CHECK(drawNumber(im, {0, 0, 1, 1, 0}, n, {0, 0, 0}));
    int ink = 0;
    for (int y = 0; y < 32; ++y)
      for (int x = 0; x < 64; ++x)
        ink += im.at(x, y)[0] < 128;
    CHECK(ink > 30);
  }
  Image im = makeImage(16, 16);
  CHECK_FALSE(drawNumber(im, {0, 0, 1, 1, 0}, 1000, {0, 0, 0}));
  CHECK_FALSE(drawNumber(im, {0, 0, 1, 1, 0}, -1, {0, 0, 0}));
  // Rotated text occupies the transposed footprint.
  Image wide = makeImage(32, 64, {255, 255, 255, 255});
  drawNumber(wide, {0, 0, 1, 1, 90}, 11, {0, 0, 0});
  int top = 0, bottom = 0;
  for (int y = 0; y < 64; ++y)
    for (int x = 0; x < 32; ++x)
      if (wide.at(x, y)[0] < 128)
        (y < 32 ? top : bottom)++;
  CHECK(top > 0);
  CHECK(bottom > 0);
  const auto a = sponsorLogo(3, 96, 32), b = sponsorLogo(3, 96, 32), c = sponsorLogo(4, 96, 32);
  CHECK(a.rgba == b.rgba);
  CHECK(a.rgba != c.rgba);
  CHECK(sponsorName(3) == sponsorName(3));
  CHECK(sponsorName(3).size() >= 4);
  const auto l = parseLivery(R"({"schema":"trackrs.livery/1","id":"x","primary":"#ff0000","number":7,
    "number_panels":[{"rect":[0.1,0.1,0.2,0.1],"rotation":90}],"sponsor_slots":[{"rect":[0,0,0.5,0.1]}],"sponsor_seed":9})");
  CHECK(l.primary.r == doctest::Approx(1));
  CHECK(l.secondary.x < 0);
  CHECK(l.numberPanels.at(0).rotation == 90);
  CHECK(l.sponsorSeed == 9);
  CHECK_THROWS(parseLivery(R"({"schema":"nope"})"));
  CHECK_THROWS(parseLivery(R"({"schema":"trackrs.livery/1","primary":"red"})"));
  CHECK_THROWS(parseLivery(R"({"schema":"trackrs.livery/1","number_panels":[{"rect":[0.9,0,0.5,0.1]}]})"));
  CHECK_THROWS(parseLivery(R"({"schema":"trackrs.livery/1","number_panels":[{"rect":[0,0,0.1,0.1],"rotation":45}]})"));
  CHECK_THROWS(parseLivery(R"({"schema":"trackrs.livery/1","number":1000})"));
}

TEST_CASE("LODs reduce triangles and keep bounds; LOD choice by screen size") {
  // A finely tessellated curved panel.
  std::vector<float> pos;
  std::vector<std::uint32_t> idx;
  const int n = 40;
  for (int y = 0; y <= n; ++y)
    for (int x = 0; x <= n; ++x) {
      const float u = static_cast<float>(x) / n, v = static_cast<float>(y) / n;
      pos.insert(pos.end(), {u, v, .2f * std::sin(u * 3.f) * std::cos(v * 2.f)});
    }
  for (int y = 0; y < n; ++y)
    for (int x = 0; x < n; ++x) {
      const auto a = static_cast<std::uint32_t>(y * (n + 1) + x), b = a + 1, c = a + n + 1, d = c + 1;
      idx.insert(idx.end(), {a, b, d, a, d, c});
    }
  const auto lods = buildLods(pos.data(), pos.size() / 3, 12, idx);
  CHECK(lods.indices[0].size() == idx.size());
  CHECK(lods.indices[1].size() < lods.indices[0].size());
  CHECK(lods.indices[2].size() < lods.indices[1].size());
  CHECK(lods.indices[2].size() >= 3);
  CHECK(lods.indices[2].size() % 3 == 0);
  CHECK(lods.maximum.x == doctest::Approx(1));
  CHECK(lods.radius > .7f);
  CHECK(chooseLod(2.f, 5.f, 1.f) == 0);
  CHECK(chooseLod(2.f, 40.f, 1.f) == 1);
  CHECK(chooseLod(2.f, 400.f, 1.f) == 2);
  CHECK(chooseLod(2.f, 0.f, 1.f) == 0);
  CHECK_THROWS(buildLods(nullptr, 0, 12, idx));
}

TEST_CASE("wear: dust on dry gravel, mud when wet, rinse, brake heat and cooling") {
  Wear w;
  WearInput dry{30.f, 0.f, 0.f, .02f, Surface::Gravel, .62f};
  for (int i = 0; i < 3000; ++i) // 60 s at 30 m/s = 1.8 km
    updateWear(w, dry);
  CHECK(w.dust > .6f);
  CHECK(w.mud < w.dust);
  Wear tarmac;
  WearInput road = dry;
  road.surface = Surface::Tarmac;
  for (int i = 0; i < 3000; ++i)
    updateWear(tarmac, road);
  CHECK(tarmac.dust < .1f);
  Wear wet;
  WearInput mud = dry;
  mud.surface = Surface::Mud;
  mud.wetness = 1;
  for (int i = 0; i < 3000; ++i)
    updateWear(wet, mud);
  CHECK(wet.mud > .8f);
  CHECK(wet.dust < .1f);
  // Hard stop from 160 km/h heats the fronts more than the rears, then cools.
  Wear b;
  WearInput brake{44.f, 1.f, 0.f, .02f, Surface::Tarmac, .62f};
  for (int i = 0; i < 150; ++i)
    updateWear(b, brake);
  CHECK(b.brakeC[0] > 300.f);
  CHECK(b.brakeC[0] > b.brakeC[2]);
  const float hot = b.brakeC[0];
  WearInput coast{0.f, 0.f, 0.f, .02f, Surface::Tarmac, .62f};
  for (int i = 0; i < 3000; ++i)
    updateWear(b, coast);
  CHECK(b.brakeC[0] < hot * .5f);
  CHECK(brakeGlow(25) == 0);
  CHECK(brakeGlow(800) == 1);
  CHECK(brakeGlow(500) > 0);
  CHECK(needleAngle(0, 8000) == doctest::Approx(-2.3562f));
  CHECK(needleAngle(8000, 8000) == doctest::Approx(-2.3562f + 4.7124f));
  CHECK(needleAngle(20000, 8000) == needleAngle(8000, 8000));
  CHECK(needleAngle(std::nanf(""), 8000) == needleAngle(0, 8000));
}

TEST_CASE("procedural interior fits inside the body") {
  const glm::vec3 lo(-80, -200, -28), hi(80, 160, 85);
  const auto in = buildInterior(lo, hi);
  REQUIRE(!in.mesh.indices.empty());
  bool tacho = false, speedo = false;
  for (const auto &v : in.mesh.vertices) {
    for (int k = 0; k < 3; ++k) {
      CHECK(v.position[k] >= lo[k] - 1e-3f);
      CHECK(v.position[k] <= hi[k] + 1e-3f);
    }
    tacho = tacho || v.uv[0] == 65535;
    speedo = speedo || (v.uv[0] > 40000 && v.uv[0] < 60000);
  }
  CHECK(tacho);
  CHECK(speedo);
  CHECK(in.tachoPivot.x < in.speedPivot.x);
  CHECK_THROWS(buildInterior(hi, lo));
}

TEST_CASE("cabin measurement keeps the cage under a low roof") {
  // A box body whose roof skin (centre line, cabin band) sits at z = 40 while
  // a tall spoiler at the rear pushes the bounds to z = 85.
  const glm::vec3 lo(-80, -200, -28), hi(80, 160, 85);
  std::vector<float> pos;
  auto add = [&](float x, float y, float z) { pos.insert(pos.end(), {x, y, z}); };
  for (float y : {-200.f, -100.f, -40.f, 0.f, 20.f, 160.f})
    for (float x : {-80.f, -60.f, 0.f, 60.f, 80.f}) {
      add(x, y, -28);
      add(x, y, std::abs(x) <= 60 ? 40.f : 10.f);
    }
  add(0, -195, 85); // spoiler tip
  add(70, 0, 5);    // shoulder below the roof band
  const auto cabin = measureCabin(pos.data(), pos.size() / 3, 12, lo, hi);
  REQUIRE(cabin.measured);
  CHECK(cabin.roofZ == doctest::Approx(40));
  CHECK(cabin.halfWidth <= 80);
  const auto in = buildInterior(lo, hi, cabin);
  for (const auto &v : in.mesh.vertices) {
    CHECK(v.position[2] < 40.f);
    if (v.position[2] > 30.f) // upper cabin (cage) stays inside the roof edge; the dash sits lower and wider
      CHECK(std::abs(v.position[0]) < cabin.halfWidth + 1e-3f);
  }
  CHECK_FALSE(measureCabin(pos.data(), 0, 12, lo, hi).measured);
}

TEST_CASE("emission alpha marks lamps; saturated darks stay paint") {
  Image im = makeImage(32, 32, {190, 18, 31, 255}); // deep rally red: luma < .22 but saturated
  fill(im, 0, 0, 8, 8, {21, 28, 39, 255});           // dark navy paint (smooth, saturated)
  Image emission = makeImage(32, 32, {255, 255, 255, 0}); // RGB everywhere, alpha = lamp mask
  fill(emission, 20, 20, 24, 24, {255, 40, 20, 255});
  const auto roles = classifyRoles(im, &emission);
  CHECK(roles.map.at(16, 16)[0] == 255);
  CHECK(roles.map.at(16, 16)[2] == 0);
  CHECK(roles.map.at(4, 4)[1] == 0);  // navy is not glass...
  CHECK(roles.map.at(4, 4)[2] == 0);  // ...nor rubber
  CHECK(roles.lights.at(22, 22)[0] == 255);
  CHECK(roles.lights.at(5, 5)[0] == 0);
  CHECK(roles.counts[4] == 16);
}
