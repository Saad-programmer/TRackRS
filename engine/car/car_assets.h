// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
/**
 * Phase 4b car assets — CPU half (no GL/SDL; unit-tested).
 *
 * Purpose: everything the CarRenderer prepares on the CPU for a car body:
 *  - role maps: per-texel material roles derived from a single-atlas body
 *    (paint, glass, rubber, metal, head/tail light) because the shipped OBJ
 *    bodies carry no part names or extra materials;
 *  - liveries: recolour the primary/secondary paint inside the paint mask,
 *    paint race numbers with an original stroke font, place original
 *    procedurally generated *fictional* sponsor logos in sponsor slots;
 *  - mesh LODs (meshoptimizer simplifier) and bounds;
 *  - wear/state models: dirt and mud accumulation, brake-disc temperature,
 *    gauge needle angles;
 *  - a procedural interior (dash, instrument cluster, seats, roll cage).
 * Ownership: value types. Thread-safety: pure functions / plain structs.
 * Determinism: no clocks or global RNG; procedural content is seeded.
 * Legal: no real sponsor, team or livery is reproduced; all logo glyphs and
 * digits are drawn from code (original stroke geometry).
 */
#include "engine/vegetation/vegetation.h"
#include <array>
#include <cstdint>
#include <glm/glm.hpp>
#include <string>
#include <string_view>
#include <vector>
namespace trackrs::car {

struct Image {
  int width = 0, height = 0;
  std::vector<std::uint8_t> rgba; ///< width * height * 4, row 0 = top
  bool empty() const { return rgba.empty(); }
  std::uint8_t *at(int x, int y) { return &rgba[(static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * 4]; }
  const std::uint8_t *at(int x, int y) const {
    return &rgba[(static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * 4];
  }
};
Image makeImage(int width, int height, std::array<std::uint8_t, 4> fill = {0, 0, 0, 255});

// ---- material roles ---------------------------------------------------------
/// Role map channels (RGBA8): R paint (clear-coated, livery-recolourable),
/// G glass, B rubber, A metal. Light roles are carried separately.
struct Roles {
  Image map;   ///< RGBA role weights (0/255)
  Image lights; ///< R tail/brake light, G headlight (0/255)
  std::array<std::size_t, 6> counts{}; ///< paint, glass, rubber, metal, tail, head texels
};
/// Heuristic classification of a body atlas (+ optional emission map, whose
/// lit texels are tail/brake lights). Thresholds are documented in the .cpp.
Roles classifyRoles(const Image &albedo, const Image *emission);

// ---- liveries -----------------------------------------------------------------
struct Rect {
  float u = 0, v = 0, w = 0, h = 0; ///< normalised atlas rect (v from the top)
  int rotation = 0;                 ///< 0, 90, 180, 270 degrees (text orientation)
};
struct Livery {
  std::string id;
  glm::vec3 primary{-1}, secondary{-1};      ///< target sRGB 0..1 (<0 = keep)
  glm::vec3 primarySource{-1}, secondarySource{-1}; ///< colours replaced (<0 = auto)
  int number = -1;                            ///< race number (<0 = keep atlas)
  glm::vec3 numberColour{0, 0, 0}, panelColour{1, 1, 1};
  std::vector<Rect> numberPanels;
  std::vector<Rect> sponsorSlots;             ///< each slot gets a generated logo
  std::uint64_t sponsorSeed = 1;
};
Livery parseLivery(std::string_view json);
/// Two dominant paint colours (k-means on paint texels, deterministic).
std::array<glm::vec3, 2> dominantPaint(const Image &albedo, const Roles &roles);
/// Applies the livery in place. Recolouring keeps each texel's luminance
/// detail (shading, panel lines) and only touches paint-role texels close to
/// the source colour.
void applyLivery(Image &albedo, const Roles &roles, const Livery &livery);
/// Original stroke-font digits; returns false for numbers outside 0..999.
bool drawNumber(Image &image, const Rect &rect, int number, glm::vec3 colour);
/// Original procedural fictional sponsor logo (seeded): emblem + stroke text.
Image sponsorLogo(std::uint64_t seed, int width, int height);
std::string sponsorName(std::uint64_t seed);

// ---- LODs ------------------------------------------------------------------------
struct LodSet {
  std::array<std::vector<std::uint32_t>, 3> indices; ///< LOD0 full, LOD1, LOD2
  glm::vec3 minimum{0}, maximum{0};
  float radius = 0;
};
/// LOD1 ~50 %, LOD2 ~15 % of LOD0 triangles (meshoptimizer simplifier,
/// attribute seams locked). positions: xyz floats with `stride` bytes.
LodSet buildLods(const float *positions, std::size_t vertexCount, std::size_t stride,
                 const std::vector<std::uint32_t> &indices);
/// LOD from projected size: fraction of the screen height the bounding
/// sphere covers (fovY radians). 0 >= 0.2, 1 >= 0.06, else 2.
unsigned chooseLod(float radius, float distance, float fovY);

// ---- wear & gauges ----------------------------------------------------------------
enum class Surface : std::uint8_t { Tarmac, Gravel, Dirt, Snow, Mud, Grass, Sand, Other };
struct Wear {
  float dust = 0, mud = 0;                 ///< 0..1 coverage
  std::array<float, 4> brakeC{{25, 25, 25, 25}}; ///< disc temperature °C (FL, FR, RL, RR)
};
struct WearInput {
  float speed = 0;     ///< m/s
  float brake = 0;     ///< 0..1 foot brake
  float wetness = 0;   ///< 0..1 scene wetness
  float dt = 0;        ///< seconds
  Surface surface = Surface::Tarmac;
  float frontBias = .62f;
};
/// Dust/mud accumulate with distance driven on loose surfaces (mud when wet),
/// rain slowly washes dust; brakes heat with braking power and cool towards
/// ambient. Deterministic, frame-rate independent for small dt.
void updateWear(Wear &wear, const WearInput &input);
/// Visible disc glow 0..1 (dull red from ~350 °C, bright by ~650 °C).
float brakeGlow(float celsius);
/// Needle angle (radians) over a sweep starting at `start`.
float needleAngle(float value, float maximum, float start = -2.3562f, float sweep = 4.7124f);

// ---- interior ------------------------------------------------------------------------
/// Gauge vertices use uv.x = 1 (tacho needle), .75 (speed needle), uv.y the
/// pivot-local radius; the vertex shader rotates them by the live angles.
struct Interior {
  vegetation::Mesh mesh;
  glm::vec3 tachoPivot{0}, speedPivot{0}; ///< body space
};
/// Measured cabin envelope: roof skin height above the seats and the body
/// half-width at the roof edge (bounds alone include mirrors/spoilers).
struct Cabin {
  float roofZ = 0, halfWidth = 0;
  bool measured = false;
};
/// Samples body vertices in the cabin's length band near the centre line.
Cabin measureCabin(const float *positions, std::size_t vertexCount, std::size_t stride, glm::vec3 minimum,
                   glm::vec3 maximum);
/// Procedural cockpit inside the body bounds (body space: +y forward, +z up),
/// kept under the measured roof skin when `cabin.measured`.
Interior buildInterior(glm::vec3 minimum, glm::vec3 maximum, Cabin cabin = {});
} // namespace trackrs::car
