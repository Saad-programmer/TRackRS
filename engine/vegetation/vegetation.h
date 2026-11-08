// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
/**
 * Native 3D vegetation — CPU half (no GL, no SDL; unit-testable).
 *
 * Purpose: procedural, original low-poly plant meshes (3 LODs per form) that
 * replace the legacy crossed sprite cards near the camera, plus the per-frame
 * instance classification that picks a LOD (or the card impostor) for every
 * plant and produces dithered cross-fade values so LOD changes never pop.
 *
 * Ownership/lifetime: all types are values. Catalog owns its parsed species
 * table; generated meshes are returned by value and are immutable after build.
 * Thread-safety: free functions are pure; a Catalog is read-only after load and
 * may be shared between threads. Classifier is single-threaded per instance.
 * Determinism: geometry depends only on (form, lod, seed) through Pcg32; no
 * clock, global RNG or unordered iteration influences output.
 * Performance: Vertex is 24 B (packed normal/colour/UV); classification is a
 * branch-light O(n) loop over plants with no allocation once buckets are warm.
 */
#include "engine/core/render_quality.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <glm/glm.hpp>
#include <map>
#include <string>
#include <string_view>
#include <vector>
namespace trackrs::vegetation {

enum class Form : std::uint8_t { Card, Conifer, Broadleaf, Bush, Grass, Palm, Cactus };
constexpr unsigned MeshLods = 3; // LOD0..2 are meshes; LOD3 is the card impostor
constexpr unsigned ImpostorLod = MeshLods;
Form parseForm(std::string_view name);
std::string_view formName(Form form);

struct Species {
  Form form = Form::Card;
  glm::vec3 foliage{0.2f, 0.3f, 0.1f}; // linear albedo
  glm::vec3 bark{0.07f, 0.045f, 0.025f};
  float flex = 0.5f;          // wind bend scale at the crown
  float translucency = 0.2f;  // back-lit transmission weight
  float roughness = 0.8f;
  float width = 0.8f;         // sprite content width / card width (silhouette match)
};

/// Sprite basename -> species. Loaded from data/vegetation/species.json.
class Catalog {
  std::map<std::string, Species, std::less<>> sprites_;

public:
  static Catalog load(const std::filesystem::path &file);
  static Catalog parse(std::string_view json);
  /// Unknown sprites map to Form::Card (legacy look preserved).
  Species lookup(std::string_view spritePath) const;
  const std::map<std::string, Species, std::less<>> &entries() const { return sprites_; }
};

/// 24-byte interleaved vertex. normal = GL_INT_2_10_10_10_REV, colour = RGBA8
/// (rgb linear albedo, a = ambient occlusion), uv = 2 x unorm16.
struct Vertex {
  float position[3];
  std::uint32_t normal;
  std::uint32_t color;
  std::uint16_t uv[2];
};
static_assert(sizeof(Vertex) == 24, "vegetation vertex must stay 24 bytes");

struct Mesh {
  std::vector<Vertex> vertices;
  std::vector<std::uint16_t> indices; // every vegetation mesh fits 16-bit indices
  std::size_t triangles() const { return indices.size() / 3; }
};

/// Unit-height plant (base at z = 0, crown at z ~= 1) for one form and LOD.
Mesh buildMesh(const Species &species, unsigned lod, std::uint64_t seed = 1);
/// `crossings` vertical quads rotated by pi/crossings (legacy card layout):
/// base at z = 0, height 1, half-width 0.5. Normals are bent upwards.
Mesh buildCard(unsigned crossings);
/// Order-independent FNV-1a hash of a mesh's bytes (determinism tests).
std::uint64_t hash(const Mesh &mesh);

/// Per-instance GPU record (32 B, two vec4 attributes).
/// a = world position xyz + height in metres; b = yaw, fade code, tint, 0.
/// Fade code f: f <= 1 -> keep fragment if noise < f (incoming / fully
/// visible at 1); f >= 2 -> keep if noise >= f - 2 (outgoing). The pair
/// (t, t + 2) therefore covers every pixel exactly once during a cross-fade.
struct Instance {
  glm::vec4 a;
  glm::vec4 b;
};
static_assert(sizeof(Instance) == 32);

struct LodRanges {
  std::array<float, MeshLods> mesh{}; // end distance of LOD0..2 (metres)
  float impostor = 0;                 // end distance of the card impostor
  float band = 0.15f;                 // cross-fade width as a fraction of range
};
LodRanges rangesFor(RenderQuality quality);

struct LodChoice {
  unsigned lod = 0, next = 0; // next == lod when no cross-fade is active
  float t = 0;                // 0..1 progress towards `next`
  bool visible = true;
};
/// Pure LOD selection for one distance. Forms without meshes (Card) always use
/// the impostor. Beyond ranges.impostor the plant is not drawn.
LodChoice chooseLod(float distance, const LodRanges &ranges, Form form);

/// Pass the classifier is running for; shadows and probes are cheaper.
enum class Pass : std::uint8_t { Main, Shadow, Probe };

/// Collects visible instances into (slot, lod) buckets with zero steady-state
/// allocation. Slots are registered once per stage (one per foliage band).
class Classifier {
public:
  struct Plant {
    float x, y, z, yaw, scale; // legacy PTerrainFoliage prefix layout
  };
  struct Stats {
    std::array<std::uint64_t, MeshLods + 1> instances{};
    std::uint64_t culled = 0, faded = 0;
  };
  void begin(glm::vec3 camera, const glm::mat4 &viewProjection, const LodRanges &ranges,
             Pass pass, std::size_t slots);
  /// `base` points at the first plant; `stride` is the byte distance between
  /// plants (lets the legacy foliage array be read in place, no copy).
  void add(std::size_t slot, Form form, const void *base, std::size_t count, std::size_t stride);
  const std::vector<Instance> &bucket(std::size_t slot, unsigned lod) const {
    return buckets_[slot * (MeshLods + 1) + lod];
  }
  const Stats &stats() const { return stats_; }

private:
  void push(std::size_t slot, unsigned lod, const Plant &p, float height, float fade);
  std::vector<std::vector<Instance>> buckets_;
  std::array<glm::vec4, 6> planes_{};
  glm::vec3 camera_{0};
  LodRanges ranges_;
  Pass pass_ = Pass::Main;
  Stats stats_;
};
/// Legacy sprite card height for a plant of legacy `scale` (VMULT = 2).
constexpr float cardHeight(float scale) { return 2.0f * scale; }

} // namespace trackrs::vegetation
