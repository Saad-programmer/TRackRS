// SPDX-License-Identifier: GPL-2.0-or-later
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "engine/terrain/tracks.h"
#include "vendor/doctest/doctest.h"
using namespace trackrs::terrain;
namespace {
const RutProfile sand{.08f, .035f, .76f};
// One wheel driving +x at 20 m/s for `ticks` ticks of 1/125 s.
void drive(Tracks &t, int ticks, RutProfile surface = sand, std::uint32_t wheel = 0, float y = 0) {
  for (int i = 0; i < ticks; ++i)
    t.observe(wheel, {i * .16f, y, 0}, {1, 0}, .15f, surface, true);
}
} // namespace

TEST_CASE("tyre trails sample every half metre and are continuous") {
  Tracks t;
  t.beginStage(1024);
  drive(t, 126); // 20 m
  // Samples land on 0.16 m tick steps: one segment per 4 ticks (0.64 m).
  CHECK(t.size() == 31);
  for (std::size_t i = 0; i < t.size(); ++i) {
    const float length = glm::length(t.segments()[i].b.position - t.segments()[i].a.position);
    CHECK(length >= Tracks::Spacing);
    CHECK(length < 2 * Tracks::Spacing);
  }
  for (std::size_t i = 1; i < t.size(); ++i) {
    CHECK(t.segments()[i].a.position == t.segments()[i - 1].b.position);
    CHECK(t.segments()[i].a.along == doctest::Approx(t.segments()[i - 1].b.along));
  }
  CHECK(t.segments()[0].a.side.y == doctest::Approx(-1)); // right of +x is -y
}

TEST_CASE("hard ground, lost contact and teleports break the trail") {
  Tracks t;
  t.beginStage(1024);
  drive(t, 30);
  const auto before = t.size();
  for (int i = 30; i < 60; ++i)
    t.observe(0, {i * .16f, 0, 0}, {1, 0}, .15f, RutProfile{}, true); // rock
  CHECK(t.size() == before);
  t.observe(0, {20, 0, 0}, {1, 0}, .15f, sand, false); // airborne
  t.observe(0, {20.6f, 0, 0}, {1, 0}, .15f, sand, true);
  t.observe(0, {21.2f, 0, 0}, {1, 0}, .15f, sand, true);
  CHECK(t.size() == before + 1); // restarted: one new segment, no bridge across the gap
  t.observe(0, {90, 0, 0}, {1, 0}, .15f, sand, true); // reset to the start line
  CHECK(t.size() == before + 1);
}

TEST_CASE("the ring keeps the newest segments and never grows") {
  Tracks t;
  t.beginStage(64);
  const auto *data = t.segments().data();
  drive(t, 2000);
  CHECK(t.size() == 64);
  CHECK(t.written() > 64);
  CHECK(t.segments().data() == data);
  // The slot before head() holds the newest segment.
  const auto &newest = t.segments()[(t.head() + 63) % 64];
  CHECK(newest.b.position.x > 300.f);
}

TEST_CASE("same input gives the same hash; any change alters it") {
  Tracks a, b, c;
  for (auto *t : {&a, &b, &c})
    t->beginStage(256);
  drive(a, 500);
  drive(b, 500);
  drive(c, 500, sand, 0, .01f);
  CHECK(a.hash() == b.hash());
  CHECK(a.hash() != c.hash());
  CHECK(a.report().at("tracks_hash").get<std::string>().size() == 16);
}

TEST_CASE("static print trails parse from trackrs.props/1 and survive tyre wrap") {
  Tracks t;
  t.beginStage(16);
  const auto doc = nlohmann::json::parse(R"({"schema":"trackrs.props/1","tracks":[
    {"kind":"camel","points":[[0,0],[10,0],[10,10]]},{"kind":"foot","points":[[0,5],[3,5]]}]})");
  CHECK(t.addStatic(doc, [](float x, float) { return x * .1f; }) == 2);
  const auto statics = t.staticSegments().size();
  CHECK(statics == 5 + 5 + 2); // legs split into <= 2 m pieces
  CHECK(t.staticSegments()[0].kind == TrackKind::Camel);
  CHECK(t.staticSegments()[0].b.position.z == doctest::Approx(.2f));
  CHECK(t.staticSegments()[5].a.along == doctest::Approx(10.f)); // continues round the corner
  drive(t, 1000);
  CHECK(t.staticSegments().size() == statics);
  CHECK_THROWS(t.addStatic(nlohmann::json::parse(R"({"tracks":[{"kind":"horse","points":[[0,0],[1,1]]}]})"),
                           [](float, float) { return 0.f; }));
}
