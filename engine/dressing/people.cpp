// SPDX-License-Identifier: GPL-2.0-or-later
#include "people.h"
#include "engine/core/pcg32.h"
#include "engine/mesh/vertex_layout.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace trackrs::dressing {
namespace {
constexpr float Pi = 3.14159265358979f;
constexpr float SampleStep = 4.f;
constexpr float RoadHalfWidth = 5.f; // assumed driven half-width around the centreline

struct Sample {
  glm::vec2 p, t; // position, unit tangent
  float s;        // arc length
};
std::vector<Sample> resample(const std::vector<glm::vec3> &route) {
  std::vector<Sample> out;
  float s = 0;
  for (std::size_t i = 0; i + 1 < route.size(); ++i) {
    const glm::vec2 a(route[i]), b(route[i + 1]);
    const float len = glm::length(b - a);
    if (len < 1e-3f)
      continue;
    const glm::vec2 t = (b - a) / len;
    const int steps = static_cast<int>(std::ceil(len / SampleStep));
    for (int k = 0; k < steps; ++k) {
      const float u = static_cast<float>(k) * SampleStep;
      out.push_back({a + t * u, t, s + u});
    }
    s += len;
  }
  if (!route.empty())
    out.push_back({glm::vec2(route.back()), out.empty() ? glm::vec2(1, 0) : out.back().t, s});
  return out;
}
float segmentDistance(glm::vec2 p, glm::vec2 a, glm::vec2 b) {
  const glm::vec2 ab = b - a;
  const float l2 = glm::dot(ab, ab);
  const float t = l2 > 0 ? std::clamp(glm::dot(p - a, ab) / l2, 0.f, 1.f) : 0.f;
  return glm::length(p - (a + ab * t));
}
glm::vec2 left(glm::vec2 t) { return {-t.y, t.x}; }
float yawOf(glm::vec2 d) { return std::atan2(d.y, d.x); }

// Signed heading change over +-window samples (left turn positive).
float turn(const std::vector<Sample> &s, std::size_t i, std::size_t window) {
  const auto a = s[i >= window ? i - window : 0].t, b = s[std::min(i + window, s.size() - 1)].t;
  return std::atan2(a.x * b.y - a.y * b.x, glm::dot(a, b));
}

struct Placer {
  const std::vector<glm::vec3> &route;
  const StageQueries &stage;
  const Rules &rules;
  std::vector<Sample> samples;
  std::vector<Person> people;
  Pcg32 rng;
  // The roadmap marks the no-penalty *corridor* (often 50-150 m wide), not the
  // driven surface, so the road half-width is an explicit assumption.
  float halfWidth(const Sample &) const { return RoadHalfWidth; }
  // `margin` is the clearance from the road edge.
  bool safe(glm::vec2 p, float margin) const {
    // Clearance from the centreline of *every* route segment (hairpin legs too).
    if (distanceToRoute(route, p) < std::max(rules.minRouteDistance, margin + RoadHalfWidth))
      return false;
    const float h = stage.height(p.x, p.y);
    if (!std::isfinite(h) || h < stage.waterHeight + .3f)
      return false;
    for (const glm::vec2 o : {glm::vec2(1, 0), glm::vec2(0, 1)})
      if (std::abs(stage.height(p.x + o.x, p.y + o.y) - h) > rules.maxSlope ||
          std::abs(stage.height(p.x - o.x, p.y - o.y) - h) > rules.maxSlope)
        return false;
    for (const auto &q : people) // personal space
      if (glm::length(glm::vec2(q.position) - p) < .9f)
        return false;
    return true;
  }
  glm::vec2 nearestRoadPoint(glm::vec2 p) const {
    float best = 1e30f;
    glm::vec2 out = p;
    for (const auto &s : samples) {
      const float d = glm::length(s.p - p);
      if (d < best) {
        best = d;
        out = s.p;
      }
    }
    return out;
  }
  // margin (m) and yaw (rad) have distinct units; internal helper.
  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  bool add(Kind kind, glm::vec2 p, float margin, float yaw, glm::vec3 lookAt) {
    if (!safe(p, margin))
      return false;
    Person person;
    person.kind = kind;
    person.position = {p, stage.height(p.x, p.y)};
    person.yaw = yaw;
    person.palette = static_cast<std::uint8_t>(rng.next() % 8u);
    person.pose = kind == Kind::Spectator ? static_cast<std::uint8_t>(rng.unit() < .45f) : 0;
    person.height = kind == Kind::TripodCamera ? 1.f : 1.62f + .28f * rng.unit();
    const glm::vec2 road = nearestRoadPoint(p);
    person.away = glm::length(p - road) > 0 ? glm::normalize(p - road) : glm::vec2(1, 0);
    person.lookAt = lookAt;
    people.push_back(person);
    return true;
  }
  // A crowd on one side of sample i, `depth` metres deep, `along` metres long.
  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  void crowd(std::size_t i, float side, float along, float depth, int count) {
    const auto &s = samples[i];
    const glm::vec2 n = left(s.t) * side;
    const float base = halfWidth(s) + rules.edgeMargin;
    const float margin = rules.edgeMargin;
    int placed = 0;
    for (int attempt = 0; attempt < count * 6 && placed < count; ++attempt) {
      const glm::vec2 p = s.p + s.t * (rng.symmetric() * along * .5f) + n * (base + rng.unit() * depth);
      const glm::vec2 road = nearestRoadPoint(p);
      const float yaw = yawOf(road - p) + .35f * rng.symmetric();
      placed += add(Kind::Spectator, p, margin, yaw, {road, stage.height(road.x, road.y)});
    }
  }
};
} // namespace

std::vector<glm::vec3> traceRoad(glm::vec3 start, const std::vector<glm::vec3> &checkpoints, const StageQueries &stage) {
  if (!stage.hasRoadmap || !stage.onRoad || !stage.height || checkpoints.empty())
    return {};
  constexpr float step = 4.f;
  std::vector<glm::vec3> out;
  glm::vec2 p(start), heading = glm::vec2(checkpoints.front()) - p;
  if (glm::length(heading) < 1e-3f)
    heading = {1, 0};
  heading = glm::normalize(heading);
  auto coverage = [&](glm::vec2 origin, glm::vec2 dir) {
    int n = 0;
    for (int k = 1; k <= 20; ++k) {
      const float d = 2.f * static_cast<float>(k);
      n += stage.onRoad(origin.x + dir.x * d, origin.y + dir.y * d) ? 1 : 0;
    }
    return n;
  };
  std::size_t traced = 0;
  // Leg by leg: follow the corridor towards each checkpoint within a length
  // budget; a leg that cannot be followed falls back to its straight chord, so
  // one bad junction never discards the whole stage.
  for (const auto &cp : checkpoints) {
    const glm::vec2 goal(cp), legStart = p;
    const float chord = glm::length(goal - p);
    const float arrive = std::clamp(.4f * chord, 12.f, 40.f);
    const auto budget = static_cast<int>((2.5f * chord + 120.f) / step);
    std::vector<glm::vec3> leg;
    glm::vec2 q = p, h = heading;
    bool reached = chord <= arrive;
    for (int i = 0; i < budget && !reached; ++i) {
      leg.push_back({q, stage.height(q.x, q.y)});
      const glm::vec2 toGoal = goal - q;
      const glm::vec2 goalDir = glm::length(toGoal) > 1e-3f ? glm::normalize(toGoal) : h;
      float best = -1e9f;
      glm::vec2 chosen = h;
      for (int k = -12; k <= 12; ++k) {
        const float a = static_cast<float>(k) * Pi / 36.f;
        const glm::vec2 dir(h.x * std::cos(a) - h.y * std::sin(a), h.x * std::sin(a) + h.y * std::cos(a));
        const float score = static_cast<float>(coverage(q, dir)) - .1f * std::abs(static_cast<float>(k)) +
                            2.f * glm::dot(dir, goalDir);
        if (score > best) {
          best = score;
          chosen = dir;
        }
      }
      h = glm::normalize(glm::mix(h, chosen, .7f));
      q += h * step;
      // Re-centre in the corridor (nearest contiguous on-road run, +-90 m).
      const glm::vec2 n(-h.y, h.x);
      auto on = [&](float o) { return stage.onRoad(q.x + n.x * o, q.y + n.y * o); };
      float seed = 1e9f;
      for (float o = 0.f; o <= 90.f && seed > 1e8f; o += 2.f) {
        if (on(o))
          seed = o;
        else if (on(-o))
          seed = -o;
      }
      if (seed < 1e8f) {
        float lo = seed, hi = seed;
        while (lo > seed - 180.f && on(lo - 2.f))
          lo -= 2.f;
        while (hi < seed + 180.f && on(hi + 2.f))
          hi += 2.f;
        q += n * ((lo + hi) * .5f * .6f);
      }
      reached = glm::length(goal - q) <= arrive;
    }
    if (reached) {
      // Continue from where the road actually is (checkpoints may sit off the
      // corridor centre); jumping to the checkpoint would cut the corner.
      out.insert(out.end(), leg.begin(), leg.end());
      heading = h;
      p = q;
      ++traced;
    } else { // straight chord fallback for this leg
      const int steps = static_cast<int>(std::ceil(chord / step));
      for (int k = 0; k < steps; ++k) {
        const glm::vec2 c = legStart + (goal - legStart) * (static_cast<float>(k) * step / chord);
        out.push_back({c, stage.height(c.x, c.y)});
      }
      heading = chord > 1e-3f ? (goal - legStart) / chord : heading;
      p = goal;
    }
  }
  out.push_back({checkpoints.back()});
  if (traced == 0)
    return {};
  return out;
}

float distanceToRoute(const std::vector<glm::vec3> &route, glm::vec2 p) {
  float best = 1e30f;
  for (std::size_t i = 0; i + 1 < route.size(); ++i)
    best = std::min(best, segmentDistance(p, glm::vec2(route[i]), glm::vec2(route[i + 1])));
  if (route.size() == 1)
    best = glm::length(p - glm::vec2(route[0]));
  return best;
}

std::vector<Person> dressStage(const std::vector<glm::vec3> &route, const StageQueries &stage, const Rules &rules,
                               std::uint64_t seed) {
  if (!stage.height || !stage.onRoad)
    throw std::invalid_argument("dressing needs height and road queries");
  Placer placer{route, stage, rules, {}, {}, Pcg32(seed, 0xd7e55ULL)};
  auto &s = placer.samples;
  s = resample(route);
  if (s.size() < 8 || rules.density <= 0)
    return {};
  const float density = std::clamp(rules.density, 0.f, 1.f);

  // Start: crowds on both sides behind the safety line, news crew to one side.
  const std::size_t start = std::min<std::size_t>(5, s.size() - 1);
  const int startCrowd = static_cast<int>(22.f * density);
  placer.crowd(start, 1.f, 30.f, 6.f, startCrowd);
  placer.crowd(start, -1.f, 30.f, 6.f, startCrowd);
  {
    const auto &a = s[0];
    const glm::vec2 n = left(a.t);
    const float edge = placer.halfWidth(a);
    // Reporter faces the camera with the start area behind them; the
    // camera operator stands further from the road, looking towards it.
    // Try both sides, a little behind or beside the start line, until the
    // reporter and the camera position are both safe.
    bool placed = false;
    for (float side : {1.f, -1.f})
      for (float back : {12.f, 22.f, 4.f, 32.f})
        for (float out : {14.f, 18.f, 24.f}) {
          if (placed)
            break;
          const glm::vec2 reporter = a.p - a.t * back + n * side * (edge + out);
          const glm::vec2 camera = reporter + n * side * 3.2f;
          const float margin = rules.edgeMargin + 4.f;
          if (!placer.safe(reporter, margin) || !placer.safe(camera, margin) ||
              glm::length(camera - reporter) < 1.f)
            continue;
          const glm::vec3 r3(reporter, stage.height(reporter.x, reporter.y));
          const glm::vec3 c3(camera, stage.height(camera.x, camera.y));
          placer.add(Kind::Reporter, reporter, margin, yawOf(camera - reporter), c3);
          placer.add(Kind::CameraOperator, camera, margin, yawOf(reporter - camera), r3);
          placed = true;
        }
  }
  // Corners: the tightest bends get a crowd on the inside (the outside of a
  // bend is the run-off/danger zone) and the four tightest a TV camera.
  struct Corner {
    std::size_t index;
    float angle;
  };
  std::vector<Corner> corners;
  const std::size_t window = 5; // +-20 m
  for (std::size_t i = window; i + window < s.size(); ++i) {
    const float a = turn(s, i, window);
    if (std::abs(a) < .55f)
      continue;
    if (!corners.empty() && s[i].s - s[corners.back().index].s < 120.f) {
      if (std::abs(a) > std::abs(corners.back().angle))
        corners.back() = {i, a};
      continue;
    }
    corners.push_back({i, a});
  }
  std::stable_sort(corners.begin(), corners.end(),
                   [](const Corner &x, const Corner &y) { return std::abs(x.angle) > std::abs(y.angle); });
  // Camera operators first (they need the best, unobstructed spots), then crowds.
  for (std::size_t c = 0; c < corners.size() && c < 4; ++c) {
    const float side = corners[c].angle > 0 ? 1.f : -1.f; // inside of the bend
    const auto &apex = s[corners[c].index];
    const glm::vec3 apex3(apex.p, stage.height(apex.p.x, apex.p.y));
    bool placed = false;
    for (std::size_t back : {3u, 5u, 1u, 7u})
      for (float extra : {0.f, 3.f, 6.f}) {
        if (placed || corners[c].index < back)
          continue;
        const auto &b = s[corners[c].index - back];
        const glm::vec2 op = b.p + left(b.t) * side * (placer.halfWidth(b) + rules.operatorMargin + extra);
        const float yaw = yawOf(apex.p - op);
        const glm::vec2 tripod = op + glm::vec2(std::cos(yaw), std::sin(yaw)) * .75f;
        if (distanceToRoute(route, tripod) < rules.operatorMargin - 1.f + RoadHalfWidth ||
            !placer.add(Kind::CameraOperator, op, rules.operatorMargin, yaw, apex3))
          continue;
        // The tripod stands beside its operator (exempt from personal space).
        Person cam;
        cam.kind = Kind::TripodCamera;
        cam.position = {tripod, stage.height(tripod.x, tripod.y)};
        cam.yaw = yaw;
        cam.height = 1.f;
        cam.away = placer.people.back().away;
        cam.lookAt = apex3;
        placer.people.push_back(cam);
        placed = true;
      }
  }
  const auto maxCrowds = static_cast<std::size_t>(std::lround(28.f * density));
  for (std::size_t c = 0; c < corners.size() && c < maxCrowds; ++c) {
    const float side = corners[c].angle > 0 ? 1.f : -1.f;
    const int size = 6 + static_cast<int>(placer.rng.next() % 12u);
    placer.crowd(corners[c].index, side, 16.f, 5.f, size);
  }
  // Finish: spectators on both sides.
  const std::size_t finish = s.size() > 4 ? s.size() - 4 : 0;
  const int finishCrowd = static_cast<int>(14.f * density);
  placer.crowd(finish, 1.f, 24.f, 5.f, finishCrowd);
  placer.crowd(finish, -1.f, 24.f, 5.f, finishCrowd);
  return std::move(placer.people);
}

Animated animate(const Person &p, glm::vec3 car, bool hasCar) {
  Animated a{p.position, p.yaw, 0.f};
  const float d = hasCar ? glm::length(glm::vec2(car - p.position)) : 1e9f;
  auto smooth = [](float e0, float e1, float x) {
    const float t = std::clamp((x - e0) / (e1 - e0), 0.f, 1.f);
    return t * t * (3.f - 2.f * t);
  };
  auto blendYaw = [](float from, float to, float t) {
    const float diff = std::remainder(to - from, 2.f * Pi);
    return from + diff * t;
  };
  switch (p.kind) {
  case Kind::Spectator: {
    a.excitement = smooth(90.f, 15.f, d);
    // Instinctive step back when a car passes close (never towards the road).
    a.position += glm::vec3(p.away * (1.4f * smooth(30.f, 8.f, d)), 0);
    if (hasCar)
      a.yaw = blendYaw(p.yaw, std::atan2(car.y - p.position.y, car.x - p.position.x), .8f * a.excitement);
    break;
  }
  case Kind::CameraOperator:
  case Kind::TripodCamera: {
    // Pan with a car within 160 m, otherwise frame the default target.
    const glm::vec3 target = hasCar && d < 160.f ? car : p.lookAt;
    if (glm::length(glm::vec2(target - p.position)) > 1e-3f)
      a.yaw = std::atan2(target.y - p.position.y, target.x - p.position.x);
    break;
  }
  case Kind::Reporter:
    break;
  }
  return a;
}

namespace {
// Oriented box from a to b with a square cross-section (flat-shaded faces).
struct Figure {
  vegetation::Mesh mesh;
  // Width/depth of a square-ish section, in natural order; internal helper.
  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  void beam(glm::vec3 a, glm::vec3 b, float w, float d, glm::vec3 colour, glm::vec2 uv) {
    const glm::vec3 axis = b - a;
    const float len = glm::length(axis);
    if (len < 1e-5f)
      return;
    const glm::vec3 z = axis / len;
    const glm::vec3 helper = std::abs(z.z) < .9f ? glm::vec3(0, 0, 1) : glm::vec3(1, 0, 0);
    const glm::vec3 x = glm::normalize(glm::cross(helper, z)) * (w * .5f);
    const glm::vec3 y = glm::normalize(glm::cross(z, x)) * (d * .5f);
    const glm::vec3 c[8] = {a - x - y, a + x - y, a + x + y, a - x + y, b - x - y, b + x - y, b + x + y, b - x + y};
    const int faces[6][4] = {{0, 3, 2, 1}, {4, 5, 6, 7}, {0, 1, 5, 4}, {1, 2, 6, 5}, {2, 3, 7, 6}, {3, 0, 4, 7}};
    const glm::vec3 centre = (a + b) * .5f;
    for (const auto &face : faces) {
      int f[4] = {face[0], face[1], face[2], face[3]};
      glm::vec3 n = glm::normalize(glm::cross(c[f[1]] - c[f[0]], c[f[2]] - c[f[0]]));
      const glm::vec3 faceCentre = (c[f[0]] + c[f[1]] + c[f[2]] + c[f[3]]) * .25f;
      if (glm::dot(n, faceCentre - centre) < 0) { // keep faces outward and CCW
        std::swap(f[1], f[3]);
        n = -n;
      }
      const auto base = static_cast<std::uint16_t>(mesh.vertices.size());
      for (int k = 0; k < 4; ++k) {
        vegetation::Vertex v{};
        v.position[0] = c[f[k]].x;
        v.position[1] = c[f[k]].y;
        v.position[2] = c[f[k]].z;
        v.normal = mesh::packSnorm2_10_10_10(n.x, n.y, n.z, 0);
        v.color = mesh::packRGBA8(colour.r, colour.g, colour.b, .85f + .15f * std::clamp(c[f[k]].z, 0.f, 1.f));
        v.uv[0] = static_cast<std::uint16_t>(std::lround(uv.x * 65535.f));
        v.uv[1] = static_cast<std::uint16_t>(std::lround(uv.y * 65535.f));
        mesh.vertices.push_back(v);
      }
      mesh.indices.insert(mesh.indices.end(), {base, static_cast<std::uint16_t>(base + 1), static_cast<std::uint16_t>(base + 2),
                                               base, static_cast<std::uint16_t>(base + 2), static_cast<std::uint16_t>(base + 3)});
    }
  }
};
// UV region codes (see header): x = animation part, y = colour region.
constexpr glm::vec2 Fixed(0, 0), Skin(0, .25f), Jacket(0, .5f), Trousers(0, .75f);
constexpr glm::vec2 LeftArm(.25f, .5f), RightArm(.5f, .5f), LeftHand(.25f, .25f), RightHand(.5f, .25f);
const glm::vec3 White(1), Black(.03f), Grey(.12f);
} // namespace

vegetation::Mesh buildFigure(Kind kind, unsigned lod) {
  if (lod >= PersonLods)
    throw std::out_of_range("figure LOD");
  Figure f;
  if (kind == Kind::TripodCamera) {
    // Tripod (three legs) + camera body + lens + viewfinder; dark matte.
    const glm::vec3 head(0, 0, .95f);
    for (int i = 0; i < 3; ++i) {
      const float a = 2.f * Pi * static_cast<float>(i) / 3.f;
      f.beam({.28f * std::cos(a), .28f * std::sin(a), .012f}, head, .03f, .03f, Grey, Fixed);
    }
    f.beam({-.18f, 0, 1.05f}, {.18f, 0, 1.05f}, .14f, .18f, Black, Fixed);
    if (lod < 2)
      f.beam({.18f, 0, 1.06f}, {.34f, 0, 1.06f}, .09f, .09f, Grey, Fixed);
    if (lod == 0)
      f.beam({-.2f, .08f, 1.12f}, {-.1f, .08f, 1.12f}, .06f, .05f, Black, Fixed);
    return std::move(f.mesh);
  }
  // Human, unit height, facing +x, lateral +y, up +z.
  const float hip = .48f, shoulder = .80f, neck = .84f;
  if (lod == 2) { // distant: two boxes
    f.beam({0, 0, 0}, {0, 0, hip}, .28f, .18f, White, Trousers);
    f.beam({0, 0, hip}, {0, 0, neck}, .34f, .2f, White, Jacket);
    f.beam({0, 0, neck}, {0, 0, 1.f}, .14f, .14f, White, Skin);
    return std::move(f.mesh);
  }
  if (lod == 0) {
    f.beam({0, .08f, 0}, {0, .08f, hip}, .12f, .14f, White, Trousers);
    f.beam({0, -.08f, 0}, {0, -.08f, hip}, .12f, .14f, White, Trousers);
    f.beam({.04f, .08f, 0}, {.04f, .08f, .04f}, .1f, .22f, Black, Fixed); // shoes
    f.beam({.04f, -.08f, 0}, {.04f, -.08f, .04f}, .1f, .22f, Black, Fixed);
  } else {
    f.beam({0, 0, 0}, {0, 0, hip}, .26f, .16f, White, Trousers);
  }
  f.beam({0, 0, hip}, {0, 0, shoulder + .02f}, .36f, .2f, White, Jacket);
  f.beam({0, 0, neck - .03f}, {0, 0, 1.f}, .15f, .16f, White, Skin);
  if (lod == 0 && kind == Kind::Spectator) // cap for variety (jacket colour)
    f.beam({.02f, 0, .97f}, {.02f, 0, 1.02f}, .17f, .18f, White, Jacket);
  const float arm = lod == 0 ? .085f : .1f;
  switch (kind) {
  case Kind::Spectator:
    // Hanging arms; the vertex shader swings them up when cheering.
    f.beam({0, .21f, shoulder}, {0, .21f, .5f}, arm, arm, White, LeftArm);
    f.beam({0, -.21f, shoulder}, {0, -.21f, .5f}, arm, arm, White, RightArm);
    if (lod == 0) {
      f.beam({0, .21f, .5f}, {0, .21f, .44f}, .07f, .07f, White, LeftHand);
      f.beam({0, -.21f, .5f}, {0, -.21f, .44f}, .07f, .07f, White, RightHand);
    }
    break;
  case Kind::CameraOperator:
    // Shoulder-mounted broadcast camera held at eye level.
    f.beam({0, -.21f, shoulder}, {.2f, -.12f, .86f}, arm, arm, White, Jacket);
    f.beam({0, .21f, shoulder}, {.22f, .05f, .82f}, arm, arm, White, Jacket);
    f.beam({-.1f, -.14f, .9f}, {.3f, -.14f, .9f}, .14f, .12f, Black, Fixed);
    if (lod == 0)
      f.beam({.3f, -.14f, .9f}, {.4f, -.14f, .9f}, .08f, .08f, Grey, Fixed);
    break;
  case Kind::Reporter:
    // Microphone raised in the right hand; left arm relaxed.
    f.beam({0, .21f, shoulder}, {0, .21f, .5f}, arm, arm, White, Jacket);
    f.beam({0, -.21f, shoulder}, {.18f, -.14f, .78f}, arm, arm, White, Jacket);
    f.beam({.18f, -.14f, .78f}, {.24f, -.1f, .9f}, .04f, .04f, Black, Fixed);
    if (lod == 0)
      f.beam({.24f, -.1f, .9f}, {.26f, -.09f, .94f}, .055f, .055f, Grey, Fixed);
    break;
  case Kind::TripodCamera:
    break;
  }
  return std::move(f.mesh);
}
} // namespace trackrs::dressing
