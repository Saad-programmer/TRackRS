// SPDX-License-Identifier: GPL-2.0-or-later
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "engine/dressing/people.h"
#include "engine/mesh/vertex_layout.h"
#include "vendor/doctest/doctest.h"
#include <cmath>
using namespace trackrs::dressing;
namespace {
// Straight start, a 180-degree hairpin, a return leg 30 m away, then an S bend.
std::vector<glm::vec3> route() {
  std::vector<glm::vec3> r;
  for (float x = 0; x <= 300; x += 20)
    r.push_back({x, 0, 0});
  for (int i = 1; i < 12; ++i) {
    const float a = -1.5708f + 3.1416f * static_cast<float>(i) / 12.f;
    r.push_back({300 + 15 * std::cos(a), 15 + 15 * std::sin(a), 0});
  }
  for (float x = 300; x >= 0; x -= 20)
    r.push_back({x, 30, 0});
  for (int i = 1; i <= 20; ++i)
    r.push_back({-20.f * static_cast<float>(i), 30 + 40 * std::sin(.3f * static_cast<float>(i)), 0});
  return r;
}
StageQueries stage(const std::vector<glm::vec3> &r) {
  StageQueries q;
  q.onRoad = [r](float x, float y) { return distanceToRoute(r, {x, y}) < 4.f; };
  // Flat, except a lake below the water plane at x in [100,140], y < -20 and a cliff at y > 80.
  q.height = [](float x, float y) {
    if (x > 100 && x < 140 && y < -20)
      return -5.f;
    if (y > 80)
      return (y - 80) * 2.f;
    return 0.f;
  };
  q.waterHeight = -1.f;
  return q;
}
glm::vec3 facing(float yaw) { return {std::cos(yaw), std::sin(yaw), 0}; }
} // namespace

TEST_CASE("people stand in safe, logical places") {
  const auto r = route();
  const auto q = stage(r);
  const Rules rules;
  const auto people = dressStage(r, q, rules, 42);
  REQUIRE(people.size() > 40);
  unsigned operators = 0, tripods = 0, reporters = 0, spectators = 0;
  for (const auto &p : people) {
    const glm::vec2 xy(p.position);
    CHECK(distanceToRoute(r, xy) >= rules.minRouteDistance - 1e-3f); // includes the hairpin's other leg
    CHECK_FALSE(q.onRoad(xy.x, xy.y));
    CHECK(p.position.z > q.waterHeight); // never in the lake
    CHECK(p.position.y < 80.5f);         // never on the cliff
    CHECK(std::isfinite(p.yaw));
    switch (p.kind) {
    case Kind::Spectator:
      ++spectators;
      CHECK(p.height >= 1.6f);
      CHECK(p.height <= 1.91f);
      break;
    case Kind::CameraOperator:
      ++operators;
      break;
    case Kind::TripodCamera:
      ++tripods;
      break;
    case Kind::Reporter:
      ++reporters;
      break;
    }
    if (p.kind == Kind::Spectator) { // faces the road (roughly)
      const glm::vec3 toLook = p.lookAt - p.position;
      CHECK(glm::dot(facing(p.yaw), glm::normalize(glm::vec3(toLook.x, toLook.y, 0))) > .8f);
      CHECK(glm::dot(p.away, glm::vec2(toLook)) < 0); // "away" points away from the road
    }
  }
  CHECK(spectators > 30);
  CHECK(reporters == 1);
  CHECK(operators >= 2); // news crew + at least one trackside TV camera
  CHECK(tripods >= 1);
  CHECK(tripods <= 4);
  // The news crew is near the start, off the road.
  for (const auto &p : people)
    if (p.kind == Kind::Reporter)
      CHECK(glm::length(glm::vec2(p.position)) < 40.f);
  // Personal space: nobody stands inside anybody else (tripods sit by their operator).
  for (std::size_t i = 0; i < people.size(); ++i)
    for (std::size_t j = i + 1; j < people.size(); ++j)
      if (people[i].kind != Kind::TripodCamera && people[j].kind != Kind::TripodCamera)
        CHECK(glm::length(glm::vec2(people[i].position - people[j].position)) >= .9f - 1e-4f);
}

TEST_CASE("placement is deterministic per seed and density scales the crowd") {
  const auto r = route();
  const auto q = stage(r);
  const auto a = dressStage(r, q, {}, 7), b = dressStage(r, q, {}, 7), c = dressStage(r, q, {}, 8);
  REQUIRE(a.size() == b.size());
  for (std::size_t i = 0; i < a.size(); ++i) {
    CHECK(a[i].position == b[i].position);
    CHECK(a[i].yaw == b[i].yaw);
  }
  bool differs = a.size() != c.size();
  for (std::size_t i = 0; !differs && i < a.size(); ++i)
    differs = a[i].position != c[i].position;
  CHECK(differs);
  Rules sparse;
  sparse.density = .3f;
  CHECK(dressStage(r, q, sparse, 7).size() < a.size());
  sparse.density = 0;
  CHECK(dressStage(r, q, sparse, 7).empty());
  CHECK(dressStage({{0, 0, 0}, {10, 0, 0}}, q, {}, 7).empty()); // too short to dress
  CHECK_THROWS(dressStage(r, StageQueries{}, {}, 7));
}

TEST_CASE("crowd behaviour: step back and cheer, cameras pan with the car") {
  Person p;
  p.position = {0, 10, 0};
  p.away = {0, 1};
  p.yaw = -1.5708f;
  const auto calm = animate(p, {500, 0, 0}, true);
  CHECK(calm.excitement == doctest::Approx(0));
  CHECK(calm.position == p.position);
  const auto close = animate(p, {5, 0, 0}, true);
  CHECK(close.excitement > .9f);
  CHECK(close.position.y > p.position.y + 1.f); // stepped back, away from the road
  Person cam;
  cam.kind = Kind::CameraOperator;
  cam.position = {0, 10, 0};
  cam.lookAt = {0, 0, 0};
  CHECK(animate(cam, {50, 10, 0}, true).yaw == doctest::Approx(0).epsilon(1e-4)); // tracks the car
  CHECK(animate(cam, {900, 10, 0}, true).yaw == doctest::Approx(-1.5708f).epsilon(1e-3)); // out of range: apex
  Person reporter;
  reporter.kind = Kind::Reporter;
  reporter.yaw = 1;
  CHECK(animate(reporter, {1, 1, 0}, true).yaw == 1);
}

TEST_CASE("figure meshes are valid, outward-facing and cheaper per LOD") {
  for (auto kind : {Kind::Spectator, Kind::CameraOperator, Kind::TripodCamera, Kind::Reporter}) {
    std::size_t previous = SIZE_MAX;
    for (unsigned lod = 0; lod < PersonLods; ++lod) {
      const auto m = buildFigure(kind, lod);
      CAPTURE(static_cast<int>(kind));
      CAPTURE(lod);
      REQUIRE(!m.indices.empty());
      CHECK(m.triangles() <= previous);
      CHECK(m.triangles() <= 200);
      previous = m.triangles();
      float top = 0;
      for (const auto &v : m.vertices) {
        top = std::max(top, v.position[2]);
        CHECK(v.position[2] >= -1e-4f);
      }
      CHECK(top <= 1.15f);
      CHECK(top >= .9f);
      for (std::size_t t = 0; t < m.indices.size(); t += 3) {
        const auto &a = m.vertices[m.indices[t]], &b = m.vertices[m.indices[t + 1]], &c = m.vertices[m.indices[t + 2]];
        const glm::vec3 pa(a.position[0], a.position[1], a.position[2]), pb(b.position[0], b.position[1], b.position[2]),
            pc(c.position[0], c.position[1], c.position[2]);
        const glm::vec3 n(trackrs::mesh::unpackSnorm10(a.normal, 0), trackrs::mesh::unpackSnorm10(a.normal, 10),
                          trackrs::mesh::unpackSnorm10(a.normal, 20));
        CHECK(glm::dot(glm::cross(pb - pa, pc - pa), n) > 0); // CCW front faces match normals
      }
    }
  }
  CHECK_THROWS(buildFigure(Kind::Spectator, PersonLods));
}
TEST_CASE("the road tracer follows the roadmap between sparse checkpoints") {
  const auto truth = route();
  const auto q = stage(truth);
  // Only a few checkpoints, far apart: straight lines would cut the hairpin.
  const std::vector<glm::vec3> checkpoints{{200, 0, 0}, {300, 15, 0}, {100, 30, 0}};
  const auto traced = traceRoad({0, 0, 0}, checkpoints, q);
  REQUIRE(traced.size() > 100);
  for (const auto &p : traced)
    CHECK(distanceToRoute(truth, glm::vec2(p)) < 3.f); // stays on the real road
  // Passes through the hairpin apex rather than cutting across.
  float apex = 1e9f;
  for (const auto &p : traced)
    apex = std::min(apex, glm::length(glm::vec2(p) - glm::vec2(315, 15)));
  CHECK(apex < 3.f);
  StageQueries none = q;
  none.hasRoadmap = false;
  CHECK(traceRoad({0, 0, 0}, checkpoints, none).empty());
  // A checkpoint off the road network: that leg falls back to its chord and
  // the trace still ends at the last checkpoint (hybrid, never wanders off).
  const auto hybrid = traceRoad({0, 0, 0}, {{200, 0, 0}, {200, 500, 0}}, q);
  REQUIRE(!hybrid.empty());
  CHECK(glm::length(glm::vec2(hybrid.back()) - glm::vec2(200, 500)) < 1e-3f);
  float farthest = 0;
  for (const auto &p : hybrid)
    farthest = std::max(farthest, glm::length(glm::vec2(p)));
  CHECK(farthest < 560.f);
}
