// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
/**
 * Stage dressing: people (Phase 5 early slice) — CPU half, no GL.
 *
 * Purpose: deterministic placement of spectators, TV camera operators (with
 * tripod cameras) and a news reporter crew in *logical, safe* places: on the
 * inside of corners and around the start/finish, always beyond the road edge
 * plus a safety margin from every part of the route (including other legs of a
 * hairpin), never on steep slopes or in water. Also builds the procedural,
 * original low-poly figure meshes (3 LODs) and the per-frame behaviour
 * (step back and cheer as a car approaches; camera operators pan with it).
 * Ownership: value types only. Thread-safety: pure functions; a Crowd is
 * single-threaded. Determinism: placement depends only on inputs + seed.
 * Physics: none. People are visual; they never feed collision or the state hash.
 */
#include "engine/vegetation/vegetation.h"
#include <cstdint>
#include <functional>
#include <glm/glm.hpp>
#include <vector>
namespace trackrs::dressing {

enum class Kind : std::uint8_t { Spectator, CameraOperator, TripodCamera, Reporter };
constexpr unsigned KindCount = 4;
constexpr unsigned PersonLods = 3;

struct Person {
  glm::vec3 position{0};  ///< feet on the terrain (world)
  float yaw = 0;          ///< facing direction (radians, +x = 0)
  Kind kind = Kind::Spectator;
  std::uint8_t palette = 0; ///< clothing palette index 0..7
  std::uint8_t pose = 0;    ///< spectator: 0 calm, 1 cheering
  float height = 1.75f;     ///< metres
  glm::vec2 away{0};        ///< unit direction away from the road (step-back)
  glm::vec3 lookAt{0};      ///< default target (road apex) for operators
};

struct StageQueries {
  std::function<float(float, float)> height;     ///< terrain height
  std::function<bool(float, float)> onRoad;      ///< roadmap test (true everywhere if none)
  float waterHeight = -1e9f;                     ///< water plane, or very low if none
  bool hasRoadmap = true;                        ///< false: onRoad is meaningless (always true)
};
struct Rules {
  float edgeMargin = 6.f;        ///< clearance from any road surface (spectators)
  float operatorMargin = 9.f;    ///< camera operators stand further back
  float minRouteDistance = 10.f; ///< no-roadmap fallback: from the route centreline
  float maxSlope = .45f;         ///< rise per metre under a figure
  float density = 1.f;           ///< 0..1 crowd density (quality)
};

/// Route = stage start followed by the checkpoints in order (world space).
std::vector<Person> dressStage(const std::vector<glm::vec3> &route, const StageQueries &stage,
                               const Rules &rules, std::uint64_t seed);
/// Dense road centreline traced on the roadmap from `start`, steering towards
/// each checkpoint in turn at junctions (4 m steps, re-centred across the road
/// every step). Returns {} when the roadmap cannot be followed.
std::vector<glm::vec3> traceRoad(glm::vec3 start, const std::vector<glm::vec3> &checkpoints,
                                 const StageQueries &stage);
/// Shortest horizontal distance from p to the resampled route polyline.
float distanceToRoute(const std::vector<glm::vec3> &route, glm::vec2 p);

/// Unit-height procedural figure / camera mesh. Vertex UV encodes animation:
/// uv.x = 0 static, .25 left arm, .5 right arm; uv.y = 0 fixed colour,
/// .5 jacket (palette), 1 trousers (palette).
vegetation::Mesh buildFigure(Kind kind, unsigned lod);

/// Per-frame behaviour for one person given the nearest car position.
struct Animated {
  glm::vec3 position;
  float yaw, excitement;
};
Animated animate(const Person &person, glm::vec3 nearestCar, bool hasCar);
} // namespace trackrs::dressing
