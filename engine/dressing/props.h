// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
/**
 * Stage props and animals (amendment A3.5) — CPU half, no GL.
 *
 * Purpose: original low-poly procedural meshes for the Saharan dressing kit
 * (camels and goats with leg-swing tags for vertex animation, nomad tents
 * (khaima), wells, stone cairns, the modular ksar/kasbah kit: pisé wall
 * segments with merlons and geometric relief bands, crenellated corner
 * towers, wooden gates; mud-brick and flat-roof concrete houses, water
 * towers, power-line poles, seguia irrigation channels, the start arch and
 * plain sign boards), plus deterministic placement from a stage's
 * `props.json` with safety rules (no prop on or near the road unless the kind
 * is road-spanning, no steep slopes, within the map).
 * Ownership: value types. Thread-safety: pure functions.
 * Determinism: placement and the dressing hash depend only on inputs + seed.
 * Culture: shapes follow the general building traditions of the Draâ valley
 * (rammed-earth walls, crenellated towers, geometric relief); they are not
 * copies of any real building. Physics: none (visual only).
 */
#include "engine/vegetation/vegetation.h"
#include <cstdint>
#include <functional>
#include <glm/glm.hpp>
#include <string>
#include <string_view>
#include <vector>
namespace trackrs::dressing {

enum class PropKind : std::uint8_t {
  Camel, Goat, Khaima, Well, Cairn, KsarWall, KsarTower, KsarGate, MudHouse, ConcreteHouse,
  WaterTower, PowerPole, Seguia, StartArch, SignBoard
};
constexpr unsigned PropKindCount = 15;
constexpr unsigned PropLods = 3;
std::string_view propName(PropKind kind);
PropKind parsePropKind(std::string_view name); ///< throws on unknown names
bool animated(PropKind kind);                  ///< camels and goats walk
bool spansRoad(PropKind kind);                 ///< the start arch stands over the road
/// Approximate bounding radius (m) at scale 1 (culling, LOD and spacing).
float propRadius(PropKind kind);

/// Unit-scale mesh in metres (+x forward, +z up, origin on the ground).
/// Animals tag their legs in uv.x: .25 front-left, .5 front-right,
/// .75 back-left, 1 back-right (0 = rigid); uv.y = 1 marks the head/neck.
vegetation::Mesh buildProp(PropKind kind, unsigned lod);
/// Sign atlas: SignColumns x SignRows cells; buildSignFace(i) is the textured
/// front and back of the sign_board panel showing cell i (readable from both
/// sides), in sign_board model space.
constexpr unsigned SignColumns = 4, SignRows = 2, SignFaces = SignColumns * SignRows;
vegetation::Mesh buildSignFace(unsigned face);

struct Prop {
  PropKind kind = PropKind::Cairn;
  glm::vec3 position{0};     ///< on the terrain (world)
  float yaw = 0;             ///< radians, +x = 0
  float scale = 1;
  std::uint8_t tint = 0;     ///< palette variation 0..7
  glm::vec2 wander{0};       ///< animals: grazing walk direction (unit) or 0
  std::int8_t face = -1;     ///< sign_board: sign atlas cell (props.json "face"), -1 = blank
};
struct PropRules {
  float roadMargin = 4.f;    ///< clearance from road texels to the prop's radius
  float maxSlope = .35f;     ///< rise per metre across the footprint
  float mapSize = 0;         ///< world size (wraps); 0 = unbounded
};
struct PropQueries {
  std::function<float(float, float)> height;  ///< terrain height
  std::function<bool(float, float)> onRoad;   ///< roadmap test
};
struct PlacementReport {
  std::size_t requested = 0, placed = 0, rejectedRoad = 0, rejectedSlope = 0;
  std::uint64_t hash = 0;    ///< FNV-1a over the placed props (dressing determinism)
};
/// Parses a stage `props.json` (schema trackrs.props/1): explicit `props`
/// [{kind, x, y, yaw_deg, scale}] in world metres, and generators:
///   herds  [{kind, x, y, radius, count}]       animals scattered around a centre
///   ksar   [{x, y, width, depth, yaw_deg, gate_side}]  walls + corner towers + gate
///   line   [{kind, from:[x,y], to:[x,y], spacing}]     poles, seguia, cairns
///   scatter[{kind, x, y, radius, count}]       houses, tents, wells, cairns
/// then applies the safety rules. Rejected props are counted, never moved
/// onto the road. Throws on schema errors.
std::vector<Prop> placeProps(std::string_view json, const PropQueries &stage, const PropRules &rules,
                             std::uint64_t seed, PlacementReport *report = nullptr);
/// Animal pose at `time`: slow grazing walk along `wander` and back (2-minute
/// cycle), deterministic. Returns position and yaw; `stride` 0..1 for legs.
struct PropPose {
  glm::vec3 position;
  float yaw, stride;
};
PropPose animateProp(const Prop &prop, float time, const std::function<float(float, float)> &height);
} // namespace trackrs::dressing
