// SPDX-License-Identifier: GPL-2.0-or-later
#include "vegetation.h"
#include "engine/core/pcg32.h"
#include "engine/mesh/vertex_layout.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>
namespace trackrs::vegetation {
namespace {
constexpr float Pi = 3.14159265358979f;
constexpr std::array<std::string_view, 7> FormNames = {
    "card", "conifer", "broadleaf", "bush", "grass", "palm", "cactus"};

float srgbToLinear(float c) {
  return c <= 0.04045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
}
glm::vec3 parseColour(const std::string &hex) {
  if (hex.size() != 7 || hex[0] != '#')
    throw std::runtime_error("vegetation colour must be #rrggbb: " + hex);
  glm::vec3 c;
  for (int i = 0; i < 3; ++i) {
    std::size_t used = 0;
    const auto value = std::stoul(hex.substr(1 + 2 * i, 2), &used, 16);
    if (used != 2)
      throw std::runtime_error("invalid vegetation colour: " + hex);
    c[i] = srgbToLinear(static_cast<float>(value) / 255.0f);
  }
  return c;
}

// Assembles one mesh. All primitives emit counter-clockwise outward faces.
// The primitive helpers take shape parameters (radii, heights, angles) in the
// natural order of the shape; they are internal and covered by geometry tests.
// NOLINTBEGIN(bugprone-easily-swappable-parameters)
struct Builder {
  Mesh mesh;
  Pcg32 rng;
  explicit Builder(std::uint64_t seed) : rng(seed, 0x5eedULL) {}
  std::uint16_t vertex(glm::vec3 p, glm::vec3 n, glm::vec3 colour, float ao,
                       glm::vec2 uv = glm::vec2(0.5f)) {
    if (mesh.vertices.size() >= 65535)
      throw std::length_error("vegetation mesh exceeds 16-bit indices");
    n = glm::normalize(n);
    Vertex v{};
    v.position[0] = p.x;
    v.position[1] = p.y;
    v.position[2] = p.z;
    v.normal = mesh::packSnorm2_10_10_10(n.x, n.y, n.z, 0);
    v.color = mesh::packRGBA8(colour.r, colour.g, colour.b, ao);
    v.uv[0] = static_cast<std::uint16_t>(std::lround(std::clamp(uv.x, 0.f, 1.f) * 65535.f));
    v.uv[1] = static_cast<std::uint16_t>(std::lround(std::clamp(uv.y, 0.f, 1.f) * 65535.f));
    mesh.vertices.push_back(v);
    return static_cast<std::uint16_t>(mesh.vertices.size() - 1);
  }
  void tri(std::uint16_t a, std::uint16_t b, std::uint16_t c) {
    mesh.indices.insert(mesh.indices.end(), {a, b, c});
  }
  // Tapered tube from a to b (closed side, open ends: ends are hidden by the
  // ground or by foliage).
  void tube(glm::vec3 a, glm::vec3 b, float ra, float rb, unsigned segments,
            glm::vec3 colour, float aoA, float aoB) {
    const glm::vec3 axis = glm::normalize(b - a);
    const glm::vec3 helper = std::abs(axis.z) < 0.9f ? glm::vec3(0, 0, 1) : glm::vec3(1, 0, 0);
    const glm::vec3 u = glm::normalize(glm::cross(helper, axis)), w = glm::cross(axis, u);
    // Orient the ring so that (u, w) is counter-clockwise around +axis.
    const auto first = static_cast<std::uint16_t>(mesh.vertices.size());
    const float slope = (ra - rb) / glm::length(b - a);
    for (unsigned i = 0; i < segments; ++i) {
      const float angle = 2 * Pi * static_cast<float>(i) / static_cast<float>(segments);
      const glm::vec3 radial = std::cos(angle) * u + std::sin(angle) * w;
      const glm::vec3 normal = radial + axis * slope;
      vertex(a + radial * ra, normal, colour, aoA);
      vertex(b + radial * rb, normal, colour, aoB);
    }
    for (unsigned i = 0; i < segments; ++i) {
      const auto j = (i + 1) % segments;
      const auto b0 = static_cast<std::uint16_t>(first + 2 * i), t0 = static_cast<std::uint16_t>(b0 + 1);
      const auto b1 = static_cast<std::uint16_t>(first + 2 * j), t1 = static_cast<std::uint16_t>(b1 + 1);
      tri(b0, b1, t1);
      tri(b0, t1, t0);
    }
  }
  // One conifer tier: jittered drooping ring, shared apex, darker underside.
  void cone(float z0, float z1, float radius, unsigned segments, glm::vec3 colour) {
    const auto apex = vertex({0, 0, z1}, {0, 0, 1}, colour * 1.08f, 1.0f);
    const auto centre = vertex({0, 0, z0 + 0.03f}, {0, 0, -1}, colour * 0.75f, 0.6f);
    const auto first = static_cast<std::uint16_t>(mesh.vertices.size());
    const float phase = rng.unit() * 2 * Pi;
    // Each ring position is emitted twice: an upper vertex with the slope
    // normal and a darker underside vertex facing down (hard crease).
    for (unsigned i = 0; i < segments; ++i) {
      const float angle = phase + 2 * Pi * static_cast<float>(i) / static_cast<float>(segments);
      const float r = radius * (0.86f + 0.28f * rng.unit());
      const float droop = 0.02f + 0.05f * rng.unit();
      const glm::vec3 radial(std::cos(angle), std::sin(angle), 0);
      // Bent towards the sky (foliage shading): a needle tier scatters light
      // more like a volume than a hard cone.
      const glm::vec3 n = glm::normalize(radial * (z1 - z0) + glm::vec3(0, 0, r)) + glm::vec3(0, 0, 0.6f);
      const glm::vec3 p = radial * r + glm::vec3(0, 0, z0 - droop);
      vertex(p, n, colour, 0.7f);
      vertex(p, radial * 0.3f - glm::vec3(0, 0, 1), colour * 0.8f, 0.65f);
    }
    for (unsigned i = 0; i < segments; ++i) {
      const auto a = static_cast<std::uint16_t>(first + 2 * i);
      const auto b = static_cast<std::uint16_t>(first + 2 * ((i + 1) % segments));
      tri(a, b, apex);
      tri(static_cast<std::uint16_t>(b + 1), static_cast<std::uint16_t>(a + 1), centre);
    }
  }
  // Jittered ellipsoid used for broadleaf and bush canopies.
  void blob(glm::vec3 centre, glm::vec3 radii, unsigned slices, unsigned stacks, glm::vec3 colour) {
    auto point = [&](float theta, float phi, float jitter, float &ao) {
      const glm::vec3 unit(std::sin(theta) * std::cos(phi), std::sin(theta) * std::sin(phi), std::cos(theta));
      ao = 0.5f + 0.5f * (unit.z * 0.5f + 0.5f);
      const glm::vec3 p = centre + unit * radii * jitter;
      // Ellipsoid gradient bent upward: soft, sky-facing canopy shading.
      const glm::vec3 n = glm::normalize(unit / radii) + glm::vec3(0, 0, 0.6f);
      return std::make_pair(p, n);
    };
    float ao = 1;
    auto top = point(0, 0, 1.0f, ao);
    const auto north = vertex(top.first, top.second, colour * 1.06f, ao);
    std::vector<std::uint16_t> rings;
    rings.reserve(std::size_t(slices) * (stacks - 1));
    for (unsigned i = 1; i < stacks; ++i)
      for (unsigned j = 0; j < slices; ++j) {
        const float theta = Pi * static_cast<float>(i) / static_cast<float>(stacks);
        const float phi = 2 * Pi * (static_cast<float>(j) + 0.5f * static_cast<float>(i % 2)) /
                          static_cast<float>(slices);
        auto v = point(theta, phi, 0.82f + 0.3f * rng.unit(), ao);
        rings.push_back(vertex(v.first, v.second, colour * (0.9f + 0.18f * rng.unit()), ao));
      }
    auto bottom = point(Pi, 0, 0.9f, ao);
    const auto south = vertex(bottom.first, bottom.second, colour * 0.7f, ao);
    auto at = [&](unsigned ring, unsigned j) { return rings[ring * slices + j % slices]; };
    for (unsigned j = 0; j < slices; ++j)
      tri(north, at(0, j), at(0, j + 1));
    for (unsigned i = 0; i + 1 < stacks - 1; ++i)
      for (unsigned j = 0; j < slices; ++j) {
        tri(at(i, j), at(i + 1, j), at(i + 1, j + 1));
        tri(at(i, j), at(i + 1, j + 1), at(i, j + 1));
      }
    for (unsigned j = 0; j < slices; ++j)
      tri(south, at(stacks - 2, j + 1), at(stacks - 2, j));
  }
  // Two-sided tapered blade (grass, crop): bent-up normal, dark root.
  void blade(glm::vec2 root, float yaw, float height, float width, float lean, glm::vec3 colour) {
    const glm::vec2 side(std::cos(yaw), std::sin(yaw)), forward(-side.y, side.x);
    const glm::vec3 n = glm::normalize(glm::vec3(forward * 0.25f, 1));
    const glm::vec3 base(root, 0), tip(root + forward * lean * height, height);
    const auto a = vertex(base - glm::vec3(side * width, 0), n, colour * 0.65f, 0.55f);
    const auto b = vertex(base + glm::vec3(side * width, 0), n, colour * 0.65f, 0.55f);
    const auto c = vertex(tip, n, colour * 1.12f, 1.0f);
    tri(a, b, c);
  }
  // Arching palm frond as a two-sided quad strip.
  void frond(glm::vec3 crown, float yaw, float length, unsigned segments, glm::vec3 colour) {
    const glm::vec3 out(std::cos(yaw), std::sin(yaw), 0), side(-out.y, out.x, 0);
    std::uint16_t previous[2] = {0, 0};
    for (unsigned i = 0; i <= segments; ++i) {
      const float s = static_cast<float>(i) / static_cast<float>(segments);
      const glm::vec3 p = crown + out * (s * length) + glm::vec3(0, 0, 0.08f * std::sin(Pi * s * 0.8f) - 0.3f * s * s);
      const float halfWidth = 0.11f * std::sin(Pi * std::min(0.1f + 0.9f * s, 1.0f)) + 0.012f;
      const glm::vec3 n = glm::normalize(glm::vec3(0, 0, 1) + out * (0.4f * s));
      const auto l = vertex(p - side * halfWidth, n, colour * (0.85f + 0.2f * s), 0.75f + 0.25f * s);
      const auto r = vertex(p + side * halfWidth, n, colour * (0.85f + 0.2f * s), 0.75f + 0.25f * s);
      if (i) {
        tri(previous[0], previous[1], r);
        tri(previous[0], r, l);
      }
      previous[0] = l;
      previous[1] = r;
    }
  }
};
// NOLINTEND(bugprone-easily-swappable-parameters)

void conifer(Builder &g, const Species &s, unsigned lod) {
  constexpr std::array<unsigned, MeshLods> Tiers = {7, 4, 1}, Segments = {11, 7, 5};
  if (lod < 2)
    g.tube({0, 0, 0}, {0, 0, 0.3f}, 0.045f, 0.03f, lod ? 4 : 6, s.bark, 0.5f, 0.8f);
  const unsigned tiers = Tiers[lod];
  for (unsigned k = 0; k < tiers; ++k) {
    const float f = static_cast<float>(k) / static_cast<float>(tiers);
    // Steep, strongly overlapping tiers read as a dense crown, not stacked discs.
    const float base = tiers == 1 ? 0.12f : 0.12f + 0.74f * f;
    const float top = k + 1 == tiers ? 1.0f : base + 0.3f;
    const float radius = (tiers == 1 ? 0.38f : 0.42f) * (1.0f - 0.8f * f);
    g.cone(base, top, radius, Segments[lod], s.foliage * (0.85f + 0.2f * f));
  }
}
void broadleaf(Builder &g, const Species &s, unsigned lod, bool bush) {
  constexpr std::array<unsigned, MeshLods> Blobs = {5, 3, 1}, Slices = {8, 6, 5}, Stacks = {6, 4, 3};
  if (!bush) {
    constexpr std::array<unsigned, MeshLods> Trunk = {6, 4, 3};
    g.tube({0, 0, 0}, {0.02f, 0, 0.52f}, 0.05f, 0.032f, Trunk[lod], s.bark, 0.45f, 0.75f);
    if (lod == 0) // one visible limb breaks the lollipop silhouette
      g.tube({0.015f, 0, 0.36f}, {0.14f, 0.05f, 0.58f}, 0.022f, 0.012f, 4, s.bark, 0.6f, 0.75f);
  }
  const float z = bush ? 0.4f : 0.66f;
  const glm::vec3 main = bush ? glm::vec3(0.46f, 0.46f, 0.4f) : glm::vec3(0.36f, 0.36f, 0.3f);
  const unsigned blobs = Blobs[lod];
  if (blobs == 1) {
    g.blob({0, 0, z}, main * 1.12f, Slices[lod], Stacks[lod], s.foliage);
    return;
  }
  g.blob({0, 0, z + 0.04f}, main, Slices[lod], Stacks[lod], s.foliage);
  const float phase = g.rng.unit() * 2 * Pi;
  for (unsigned k = 1; k < blobs; ++k) {
    const float angle = phase + 2 * Pi * static_cast<float>(k) / static_cast<float>(blobs - 1);
    const float offset = bush ? 0.24f : 0.2f;
    const glm::vec3 centre(std::cos(angle) * offset, std::sin(angle) * offset,
                           z + (g.rng.unit() - 0.55f) * (bush ? 0.12f : 0.22f));
    g.blob(centre, main * (0.58f + 0.14f * g.rng.unit()), Slices[lod], Stacks[lod],
           s.foliage * (0.88f + 0.2f * g.rng.unit()));
  }
}
void grass(Builder &g, const Species &s, unsigned lod) {
  constexpr std::array<unsigned, MeshLods> Blades = {28, 14, 6};
  constexpr std::array<float, MeshLods> Width = {0.022f, 0.032f, 0.05f};
  for (unsigned k = 0; k < Blades[lod]; ++k) {
    // Tuft: roots clustered near the centre, blades fanning outwards.
    const float r = 0.12f * std::sqrt(g.rng.unit()), a = 2 * Pi * g.rng.unit();
    const float lean = 0.15f + 0.4f * g.rng.unit();
    g.blade({r * std::cos(a), r * std::sin(a)}, a - Pi / 2, 0.5f + 0.5f * g.rng.unit(),
            Width[lod] * (0.7f + 0.6f * g.rng.unit()), lean, s.foliage * (0.8f + 0.35f * g.rng.unit()));
  }
}
void palm(Builder &g, const Species &s, unsigned lod) {
  constexpr std::array<unsigned, MeshLods> Pieces = {4, 3, 2}, Segments = {6, 5, 4}, Fronds = {12, 8, 6},
                                           Steps = {4, 3, 2};
  auto curve = [](float t) { return glm::vec3(0.16f * t * t, 0, 0.82f * t); };
  for (unsigned i = 0; i < Pieces[lod]; ++i) {
    const float t0 = static_cast<float>(i) / static_cast<float>(Pieces[lod]);
    const float t1 = static_cast<float>(i + 1) / static_cast<float>(Pieces[lod]);
    g.tube(curve(t0), curve(t1), 0.026f - 0.008f * t0, 0.026f - 0.008f * t1, Segments[lod], s.bark,
           0.55f + 0.3f * t0, 0.55f + 0.3f * t1);
  }
  const glm::vec3 crown = curve(1);
  const float phase = g.rng.unit() * 2 * Pi;
  for (unsigned k = 0; k < Fronds[lod]; ++k)
    g.frond(crown, phase + 2 * Pi * static_cast<float>(k) / static_cast<float>(Fronds[lod]),
            0.44f + 0.08f * g.rng.unit(), Steps[lod], s.foliage * (0.85f + 0.25f * g.rng.unit()));
}
void cactus(Builder &g, const Species &s, unsigned lod) {
  constexpr std::array<unsigned, MeshLods> Segments = {8, 6, 5};
  const unsigned n = Segments[lod];
  g.tube({0, 0, 0}, {0, 0, 0.88f}, 0.1f, 0.085f, n, s.foliage, 0.6f, 1.0f);
  g.cone(0.86f, 1.0f, 0.085f, n, s.foliage);
  if (lod == 2)
    return;
  for (float side : {-1.f, 1.f}) {
    const float z = side < 0 ? 0.32f : 0.44f;
    g.tube({0, 0, z}, {side * 0.22f, 0, z + 0.04f}, 0.055f, 0.05f, n - 2, s.foliage, 0.75f, 0.85f);
    g.tube({side * 0.22f, 0, z}, {side * 0.24f, 0, z + 0.32f}, 0.055f, 0.045f, n - 2, s.foliage, 0.8f, 1.0f);
  }
}
} // namespace

Form parseForm(std::string_view name) {
  for (std::size_t i = 0; i < FormNames.size(); ++i)
    if (FormNames[i] == name)
      return static_cast<Form>(i);
  throw std::runtime_error("unknown vegetation form: " + std::string(name));
}
std::string_view formName(Form form) { return FormNames.at(static_cast<std::size_t>(form)); }

Catalog Catalog::load(const std::filesystem::path &file) {
  std::ifstream in(file, std::ios::binary);
  if (!in)
    throw std::runtime_error("cannot open vegetation catalog: " + file.string());
  std::stringstream text;
  text << in.rdbuf();
  return parse(text.str());
}
Catalog Catalog::parse(std::string_view text) {
  const auto json = nlohmann::json::parse(text);
  if (json.at("schema").get<std::string>() != "trackrs.vegetation/1")
    throw std::runtime_error("unsupported vegetation schema");
  if (!json.contains("license"))
    throw std::runtime_error("vegetation catalog lacks a license");
  Catalog catalog;
  for (const auto &[name, entry] : json.at("sprites").items()) {
    Species s;
    s.form = parseForm(entry.at("form").get<std::string>());
    s.foliage = parseColour(entry.at("foliage").get<std::string>());
    s.width = entry.value("width", 0.8f);
    if (!(s.width > 0.05f && s.width <= 1.0f))
      throw std::runtime_error("vegetation width must be in (0.05, 1]: " + name);
    if (entry.contains("bark"))
      s.bark = parseColour(entry.at("bark").get<std::string>());
    s.flex = s.form == Form::Grass ? 1.6f : s.form == Form::Bush ? 0.8f
           : s.form == Form::Cactus                              ? 0.05f
           : s.form == Form::Palm                                ? 0.9f
                                                                 : 0.55f;
    s.translucency = s.form == Form::Cactus ? 0.0f : s.form == Form::Grass ? 0.35f : 0.22f;
    s.roughness = s.form == Form::Cactus ? 0.6f : 0.82f;
    catalog.sprites_.emplace(name, s);
  }
  return catalog;
}
Species Catalog::lookup(std::string_view path) const {
  const auto slash = path.find_last_of("/\\");
  const auto name = slash == std::string_view::npos ? path : path.substr(slash + 1);
  const auto it = sprites_.find(name);
  return it == sprites_.end() ? Species{} : it->second;
}

Mesh buildMesh(const Species &species, unsigned lod, std::uint64_t seed) {
  if (lod >= MeshLods)
    throw std::out_of_range("vegetation mesh LOD");
  Builder g(seed * 0x9e3779b97f4a7c15ULL + lod);
  switch (species.form) {
  case Form::Card:
    return buildCard(2);
  case Form::Conifer:
    conifer(g, species, lod);
    break;
  case Form::Broadleaf:
    broadleaf(g, species, lod, false);
    break;
  case Form::Bush:
    broadleaf(g, species, lod, true);
    break;
  case Form::Grass:
    grass(g, species, lod);
    break;
  case Form::Palm:
    palm(g, species, lod);
    break;
  case Form::Cactus:
    cactus(g, species, lod);
    break;
  }
  // Match the sprite silhouette width (bounded so forms keep their character).
  // Normals of an xy-scaled surface scale by the inverse factor.
  constexpr std::array<float, 7> Nominal = {0.5f, 0.42f, 0.45f, 0.5f, 0.4f, 0.48f, 0.3f};
  const float scale = std::clamp(0.5f * species.width / Nominal[static_cast<std::size_t>(species.form)], 0.6f, 1.4f);
  for (auto &v : g.mesh.vertices) {
    v.position[0] *= scale;
    v.position[1] *= scale;
    const glm::vec3 n(mesh::unpackSnorm10(v.normal, 0) / scale, mesh::unpackSnorm10(v.normal, 10) / scale,
                      mesh::unpackSnorm10(v.normal, 20));
    const glm::vec3 u = glm::normalize(n);
    v.normal = mesh::packSnorm2_10_10_10(u.x, u.y, u.z, 0);
  }
  return std::move(g.mesh);
}

Mesh buildCard(unsigned crossings) {
  crossings = std::clamp(crossings, 1u, 4u);
  Builder g(1);
  for (unsigned i = 0; i < crossings; ++i) {
    const float angle = Pi * static_cast<float>(i) / static_cast<float>(crossings);
    const glm::vec3 d(0.5f * std::cos(angle), 0.5f * std::sin(angle), 0);
    const glm::vec3 up(0, 0, 1), white(1);
    // Same corner/UV order as the legacy PTerrain card buffers.
    const auto a = g.vertex(d, up, white, 0.75f, {1, 0});
    const auto b = g.vertex(-d, up, white, 0.75f, {0, 0});
    const auto c = g.vertex(-d + up, up, white, 1.0f, {0, 1});
    const auto e = g.vertex(d + up, up, white, 1.0f, {1, 1});
    g.tri(a, b, c);
    g.tri(a, c, e);
  }
  return std::move(g.mesh);
}

std::uint64_t hash(const Mesh &mesh) {
  std::uint64_t h = 14695981039346656037ULL;
  auto feed = [&](const void *data, std::size_t bytes) {
    const auto *p = static_cast<const unsigned char *>(data);
    for (std::size_t i = 0; i < bytes; ++i) {
      h ^= p[i];
      h *= 1099511628211ULL;
    }
  };
  feed(mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex));
  feed(mesh.indices.data(), mesh.indices.size() * sizeof(std::uint16_t));
  return h;
}

LodRanges rangesFor(RenderQuality quality) {
  switch (quality) {
  case RenderQuality::Low:
    return {{18, 40, 70}, 170, 0.15f};
  case RenderQuality::Medium:
    return {{28, 60, 110}, 230, 0.15f};
  case RenderQuality::High:
    return {{40, 90, 160}, 300, 0.15f};
  case RenderQuality::Ultra:
    return {{55, 130, 230}, 400, 0.15f};
  }
  return rangesFor(RenderQuality::Medium);
}

LodChoice chooseLod(float distance, const LodRanges &r, Form form) {
  LodChoice c;
  if (!(distance >= 0) || distance >= r.impostor) {
    c.visible = false;
    return c;
  }
  const std::array<float, MeshLods + 1> ends = {r.mesh[0], r.mesh[1], r.mesh[2], r.impostor};
  unsigned lod = form == Form::Card ? ImpostorLod : 0;
  while (lod < ImpostorLod && distance >= ends[lod])
    ++lod;
  c.lod = c.next = lod;
  const float width = r.band * ends[lod];
  const float start = ends[lod] - width;
  if (width > 0 && distance > start) {
    c.next = lod + 1; // MeshLods + 1 means "fade out to nothing"
    c.t = std::clamp((distance - start) / width, 0.0f, 1.0f);
  }
  return c;
}

void Classifier::begin(glm::vec3 camera, const glm::mat4 &vp, const LodRanges &ranges, Pass pass,
                       std::size_t slots) {
  camera_ = camera;
  ranges_ = ranges;
  pass_ = pass;
  if (pass == Pass::Probe) // probes refresh at <= 4 Hz into tiny cubes: impostors only, short range
    ranges_.impostor = std::min(ranges_.impostor, 160.f);
  stats_ = {};
  buckets_.resize(slots * (MeshLods + 1));
  for (auto &b : buckets_)
    b.clear();
  for (int i = 0; i < 3; ++i)
    for (int s = 0; s < 2; ++s) {
      glm::vec4 plane;
      for (int k = 0; k < 4; ++k)
        plane[k] = vp[k][3] + (s ? -1.f : 1.f) * vp[k][i];
      planes_[static_cast<std::size_t>(i) * 2 + static_cast<std::size_t>(s)] = plane / glm::length(glm::vec3(plane));
    }
}

void Classifier::push(std::size_t slot, unsigned lod, const Plant &p, float height, float fade) {
  if (lod > ImpostorLod)
    return;
  // Per-plant tint from position: stable across frames, LODs and runs.
  const auto key = static_cast<std::uint32_t>(std::lround(p.x * 8.f)) * 73856093u ^
                   static_cast<std::uint32_t>(std::lround(p.y * 8.f)) * 19349663u;
  const float tint = 0.9f + 0.2f * static_cast<float>((key >> 8u) & 0xffu) / 255.f;
  buckets_[slot * (MeshLods + 1) + lod].push_back(
      {glm::vec4(p.x, p.y, p.z, height), glm::vec4(p.yaw, fade, tint, 0)});
  ++stats_.instances[lod];
}

// count/stride follow the (pointer, count, stride) convention of GL arrays.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void Classifier::add(std::size_t slot, Form form, const void *base, std::size_t count, std::size_t stride) {
  if (slot * (MeshLods + 1) >= buckets_.size())
    throw std::out_of_range("vegetation slot");
  if ((pass_ != Pass::Main && form == Form::Grass))
    return; // grass neither casts CSM shadows nor appears in reflection probes
  const auto *bytes = static_cast<const unsigned char *>(base);
  for (std::size_t i = 0; i < count; ++i) {
    Plant p;
    std::copy_n(bytes + i * stride, sizeof(Plant), reinterpret_cast<unsigned char *>(&p));
    const float height = cardHeight(p.scale);
    const glm::vec3 centre(p.x, p.y, p.z + 0.5f * height);
    const float radius = 0.75f * height;
    bool inside = true;
    for (const auto &plane : planes_)
      inside = inside && glm::dot(glm::vec3(plane), centre) + plane.w >= -radius;
    const auto choice = chooseLod(glm::length(centre - camera_), ranges_,
                                  pass_ == Pass::Probe ? Form::Card : form);
    if (!inside || !choice.visible) {
      ++stats_.culled;
      continue;
    }
    if (choice.next == choice.lod) {
      push(slot, choice.lod, p, height, 1.0f);
      continue;
    }
    stats_.faded += choice.next <= ImpostorLod;
    push(slot, choice.lod, p, height, choice.t + 2.0f); // outgoing: keep noise >= t
    push(slot, choice.next, p, height, choice.t);       // incoming: keep noise < t
  }
}
} // namespace trackrs::vegetation
