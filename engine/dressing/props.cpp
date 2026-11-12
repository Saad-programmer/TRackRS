// SPDX-License-Identifier: GPL-2.0-or-later
#include "props.h"
#include "engine/core/pcg32.h"
#include "engine/mesh/vertex_layout.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <nlohmann/json.hpp>
#include <stdexcept>
namespace trackrs::dressing {
namespace {
constexpr float Pi = 3.14159265358979f;
constexpr std::array<std::string_view, PropKindCount> Names = {
    "camel", "goat", "khaima", "well", "cairn", "ksar_wall", "ksar_tower", "ksar_gate", "mud_house",
    "concrete_house", "water_tower", "power_pole", "seguia", "start_arch", "sign_board"};
constexpr std::array<float, PropKindCount> Radius = {1.6f, 0.7f, 3.8f, 1.2f, 0.8f, 5.2f, 3.0f, 3.4f, 4.2f,
                                                     5.0f, 2.6f, 1.3f, 2.1f, 8.2f, 1.2f};

// Linear albedo palette (earth, wool, stone, paint), authored for TrackRS.
const glm::vec3 Pise(0.40f, 0.22f, 0.12f), PiseDark(0.24f, 0.12f, 0.06f), PiseLight(0.50f, 0.32f, 0.18f);
const glm::vec3 Wood(0.20f, 0.12f, 0.06f), Stone(0.30f, 0.26f, 0.21f), Concrete(0.42f, 0.40f, 0.37f);
const glm::vec3 Wool(0.045f, 0.035f, 0.03f), WoolStripe(0.30f, 0.24f, 0.18f), Window(0.02f, 0.02f, 0.025f);

// Mesh builders take positions, extents and colours in fixed, documented
// order (as in vegetation.cpp).
// NOLINTBEGIN(bugprone-easily-swappable-parameters)
class Builder {
public:
  vegetation::Mesh mesh;
  std::uint16_t vertex(glm::vec3 p, glm::vec3 n, glm::vec3 colour, float ao, glm::vec2 tag) {
    vegetation::Vertex v{};
    v.position[0] = p.x;
    v.position[1] = p.y;
    v.position[2] = p.z;
    const glm::vec3 u = glm::normalize(n);
    v.normal = mesh::packSnorm2_10_10_10(u.x, u.y, u.z, 0);
    v.uv[0] = static_cast<std::uint16_t>(std::lround(std::clamp(tag.x, 0.f, 1.f) * 65535.f));
    v.uv[1] = static_cast<std::uint16_t>(std::lround(std::clamp(tag.y, 0.f, 1.f) * 65535.f));
    const glm::vec3 c = glm::clamp(colour, glm::vec3(0), glm::vec3(1));
    v.color = mesh::packRGBA8(c.r, c.g, c.b, std::clamp(ao, 0.f, 1.f));
    if (mesh.vertices.size() >= 65535)
      throw std::length_error("prop mesh exceeds 16-bit indices");
    mesh.vertices.push_back(v);
    return static_cast<std::uint16_t>(mesh.vertices.size() - 1);
  }
  void quad(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, glm::vec3 colour, glm::vec2 tag = {}, float ao = 1) {
    const glm::vec3 n = glm::cross(b - a, d - a);
    if (glm::dot(n, n) < 1e-12f)
      return;
    const float lowAo = std::min(ao, 0.75f);
    const auto i0 = vertex(a, n, colour, a.z < 0.05f ? lowAo : ao, tag), i1 = vertex(b, n, colour, b.z < 0.05f ? lowAo : ao, tag);
    const auto i2 = vertex(c, n, colour, c.z < 0.05f ? lowAo : ao, tag), i3 = vertex(d, n, colour, d.z < 0.05f ? lowAo : ao, tag);
    mesh.indices.insert(mesh.indices.end(), {i0, i1, i2, i0, i2, i3});
  }
  void triangle(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 colour, glm::vec2 tag = {}) {
    const glm::vec3 n = glm::cross(b - a, c - a);
    if (glm::dot(n, n) < 1e-12f)
      return;
    mesh.indices.insert(mesh.indices.end(), {vertex(a, n, colour, 1, tag), vertex(b, n, colour, 1, tag), vertex(c, n, colour, 1, tag)});
  }
  /// Tapered box: bottom rectangle `lo` half-extents at z0, top `hi` at z1,
  /// centred at (cx, cy), rotated `pitch` about +y around its own centre.
  void frustum(glm::vec2 c, float z0, float z1, glm::vec2 lo, glm::vec2 hi, glm::vec3 colour, glm::vec2 tag = {},
               float pitch = 0, bool bottom = false) {
    std::array<glm::vec3, 8> v = {glm::vec3(-lo.x, -lo.y, z0), glm::vec3(lo.x, -lo.y, z0), glm::vec3(lo.x, lo.y, z0),
                                  glm::vec3(-lo.x, lo.y, z0),  glm::vec3(-hi.x, -hi.y, z1), glm::vec3(hi.x, -hi.y, z1),
                                  glm::vec3(hi.x, hi.y, z1),   glm::vec3(-hi.x, hi.y, z1)};
    const float mid = 0.5f * (z0 + z1), cp = std::cos(pitch), sp = std::sin(pitch);
    for (auto &p : v) {
      const float x = p.x, z = p.z - mid;
      p = glm::vec3(c.x + x * cp + z * sp, c.y + p.y, mid - x * sp + z * cp);
    }
    quad(v[0], v[1], v[5], v[4], colour, tag); // -y
    quad(v[1], v[2], v[6], v[5], colour, tag); // +x
    quad(v[2], v[3], v[7], v[6], colour, tag); // +y
    quad(v[3], v[0], v[4], v[7], colour, tag); // -x
    quad(v[4], v[5], v[6], v[7], colour * 1.05f, tag);
    if (bottom)
      quad(v[3], v[2], v[1], v[0], colour * 0.6f, tag);
  }
  void box(glm::vec3 centre, glm::vec3 half, glm::vec3 colour, glm::vec2 tag = {}, float pitch = 0) {
    frustum({centre.x, centre.y}, centre.z - half.z, centre.z + half.z, {half.x, half.y}, {half.x, half.y}, colour, tag,
            pitch, centre.z - half.z > 0.05f);
  }
  void cylinder(glm::vec3 base, float height, float radius, unsigned segments, glm::vec3 colour, bool cap = true) {
    for (unsigned i = 0; i < segments; ++i) {
      const float a0 = 2 * Pi * static_cast<float>(i) / static_cast<float>(segments);
      const float a1 = 2 * Pi * static_cast<float>(i + 1) / static_cast<float>(segments);
      const glm::vec3 p0 = base + glm::vec3(std::cos(a0) * radius, std::sin(a0) * radius, 0);
      const glm::vec3 p1 = base + glm::vec3(std::cos(a1) * radius, std::sin(a1) * radius, 0);
      quad(p0, p1, p1 + glm::vec3(0, 0, height), p0 + glm::vec3(0, 0, height), colour);
      if (cap)
        triangle(base + glm::vec3(0, 0, height), p0 + glm::vec3(0, 0, height), p1 + glm::vec3(0, 0, height), colour * 1.05f);
    }
  }
  void pyramid(glm::vec3 base, glm::vec2 half, float height, glm::vec3 colour) {
    const glm::vec3 top = base + glm::vec3(0, 0, height);
    const glm::vec3 a = base + glm::vec3(-half.x, -half.y, 0), b = base + glm::vec3(half.x, -half.y, 0);
    const glm::vec3 c = base + glm::vec3(half.x, half.y, 0), d = base + glm::vec3(-half.x, half.y, 0);
    triangle(a, b, top, colour);
    triangle(b, c, top, colour);
    triangle(c, d, top, colour);
    triangle(d, a, top, colour);
  }
};

void quadruped(Builder &g, unsigned lod, float s, glm::vec3 coat, bool hump) {
  // Legs (tagged for the walk cycle), torso, optional hump, neck + head.
  const std::array<glm::vec2, 4> feet = {glm::vec2(0.55f, 0.18f), glm::vec2(0.55f, -0.18f), glm::vec2(-0.55f, 0.18f),
                                         glm::vec2(-0.55f, -0.18f)};
  const float legHeight = hump ? 1.2f : 0.55f / 0.45f, legHalf = hump ? 0.07f : 0.09f;
  for (unsigned i = 0; i < 4; ++i)
    g.box(glm::vec3(feet[i] * s, 0.5f * legHeight * s), glm::vec3(legHalf * s, legHalf * s, 0.5f * legHeight * s),
          coat * 0.85f, {0.25f * static_cast<float>(i + 1), 0});
  const float torsoZ = legHeight + (hump ? 0.3f : 0.32f);
  g.box(glm::vec3(0, 0, torsoZ * s), glm::vec3(0.75f, 0.28f, 0.3f) * s, coat);
  if (hump && lod < 2)
    g.frustum({-0.05f * s, 0}, (torsoZ + 0.28f) * s, (torsoZ + 0.68f) * s, glm::vec2(0.42f, 0.24f) * s,
              glm::vec2(0.12f, 0.08f) * s, coat * 1.05f);
  const float neckPitch = hump ? -0.9f : -0.6f;
  g.box(glm::vec3(0.88f * s, 0, (torsoZ + (hump ? 0.3f : 0.2f)) * s),
        glm::vec3(hump ? 0.34f : 0.22f, 0.11f, 0.11f) * s, coat, {0, 1}, neckPitch);
  if (lod < 2)
    g.box(glm::vec3((hump ? 1.18f : 1.05f) * s, 0, (torsoZ + (hump ? 0.62f : 0.42f)) * s),
          glm::vec3(hump ? 0.24f : 0.16f, 0.1f, 0.11f) * s, coat * 0.95f, {0, 1});
  if (!hump && lod == 0) // goat horns
    for (float side : {-1.f, 1.f})
      g.box(glm::vec3(1.0f * s, 0.06f * side * s, (torsoZ + 0.58f) * s), glm::vec3(0.1f, 0.025f, 0.025f) * s,
            glm::vec3(0.12f, 0.1f, 0.08f), {0, 1}, -0.7f);
}

void ksarRelief(Builder &g, float x0, float x1, float y, float z, float side, unsigned count, glm::vec3 colour) {
  // Geometric relief band: alternating raised diamonds and chevron pairs,
  // a generic motif of the region's earthen architecture (original layout).
  for (unsigned i = 0; i < count; ++i) {
    const float x = x0 + (x1 - x0) * (static_cast<float>(i) + 0.5f) / static_cast<float>(count);
    const float face = y + side * 0.02f;
    const glm::vec3 c(x, face, z);
    const float w = 0.28f, h = 0.36f;
    const glm::vec3 out(0, side * 0.06f, 0);
    if (i % 2 == 0) {
      g.triangle(c + glm::vec3(0, 0, h) + out, c + glm::vec3(side * w, 0, 0) + out, c + glm::vec3(0, 0, -h) + out, colour);
      g.triangle(c + glm::vec3(0, 0, h) + out, c + glm::vec3(0, 0, -h) + out, c + glm::vec3(-side * w, 0, 0) + out, colour);
    } else {
      for (float dz : {-0.16f, 0.16f})
        g.triangle(c + glm::vec3(-w, 0, dz - 0.1f) + out, c + glm::vec3(0, 0, dz + 0.1f) + out,
                   c + glm::vec3(w, 0, dz - 0.1f) + out, colour);
    }
  }
}

void build(Builder &g, PropKind kind, unsigned lod) {
  switch (kind) {
  case PropKind::Camel:
    quadruped(g, lod, 1.0f, glm::vec3(0.36f, 0.24f, 0.13f), true);
    break;
  case PropKind::Goat:
    quadruped(g, lod, 0.45f, glm::vec3(0.16f, 0.12f, 0.09f), false);
    break;
  case PropKind::Khaima: {
    // Low nomad tent: a ridge along x, sloping wool roof in striped panels.
    const float halfL = 3.0f, halfW = 2.0f, ridge = 2.1f, eave = 0.8f;
    const unsigned panels = lod == 0 ? 6 : lod == 1 ? 3 : 1;
    for (unsigned i = 0; i < panels; ++i) {
      const float x0 = -halfL + 2 * halfL * static_cast<float>(i) / static_cast<float>(panels);
      const float x1 = -halfL + 2 * halfL * static_cast<float>(i + 1) / static_cast<float>(panels);
      const glm::vec3 colour = (i % 3 == 1 && lod < 2) ? WoolStripe : Wool;
      g.quad({x0, -halfW, eave}, {x1, -halfW, eave}, {x1, 0, ridge}, {x0, 0, ridge}, colour);
      g.quad({x1, halfW, eave}, {x0, halfW, eave}, {x0, 0, ridge}, {x1, 0, ridge}, colour);
      g.quad({x1, -halfW, eave}, {x0, -halfW, eave}, {x0, -halfW, 0}, {x1, -halfW, 0}, colour * 0.8f); // side curtain
    }
    g.triangle({-halfL, -halfW, eave}, {-halfL, 0, ridge}, {-halfL, halfW, eave}, Wool * 0.9f);
    if (lod < 2)
      for (float x : {-halfL + 0.2f, 0.f, halfL - 0.2f})
        g.box({x, 0, ridge * 0.5f}, {0.05f, 0.05f, ridge * 0.5f}, Wood);
    break;
  }
  case PropKind::Well:
    g.cylinder({0, 0, 0}, 0.8f, 0.9f, lod == 0 ? 14 : 8, Stone, false);
    g.cylinder({0, 0, 0.78f}, 0.02f, 0.72f, lod == 0 ? 14 : 8, glm::vec3(0.01f), true);
    if (lod < 2) {
      for (float y : {-0.85f, 0.85f})
        g.box({0, y, 1.3f}, {0.06f, 0.06f, 0.75f}, Wood);
      g.box({0, 0, 2.0f}, {0.05f, 0.95f, 0.05f}, Wood);
    }
    break;
  case PropKind::Cairn: {
    const std::array<glm::vec4, 6> stones = {glm::vec4(0, 0, 0.15f, 0.42f), glm::vec4(0.1f, -0.05f, 0.42f, 0.32f),
                                             glm::vec4(-0.06f, 0.04f, 0.65f, 0.25f), glm::vec4(0.02f, 0.02f, 0.84f, 0.18f),
                                             glm::vec4(0.25f, 0.2f, 0.1f, 0.18f), glm::vec4(-0.3f, 0.1f, 0.1f, 0.15f)};
    for (std::size_t i = 0; i < (lod == 0 ? 6u : lod == 1 ? 4u : 2u); ++i)
      g.box(glm::vec3(stones[i]), glm::vec3(stones[i].w, stones[i].w * 0.8f, stones[i].w * 0.55f),
            Stone * (0.8f + 0.1f * static_cast<float>(i % 3)), {}, 0.25f * static_cast<float>(i));
    break;
  }
  case PropKind::KsarWall: {
    const float half = 5.0f, h = 4.5f, t = 0.45f;
    g.frustum({0, 0}, 0, h, {half, t + 0.08f}, {half, t}, Pise);
    if (lod < 2) {
      const unsigned merlons = lod == 0 ? 8 : 4;
      for (unsigned i = 0; i < merlons; ++i) {
        const float x = -half + 2 * half * (static_cast<float>(i) + 0.5f) / static_cast<float>(merlons);
        g.pyramid({x, 0, h}, {0.28f, t * 0.9f}, 0.6f, Pise);
      }
    }
    if (lod == 0)
      for (float side : {-1.f, 1.f})
        ksarRelief(g, -half + 0.6f, half - 0.6f, side * t, h - 0.8f, side, 8, PiseDark);
    break;
  }
  case PropKind::KsarTower: {
    const float h = 11.f;
    g.frustum({0, 0}, 0, h, {2.0f, 2.0f}, {1.6f, 1.6f}, Pise);
    if (lod < 2) {
      for (float x : {-1.f, 1.f})
        for (float y : {-1.f, 1.f})
          g.pyramid({1.35f * x, 1.35f * y, h}, {0.35f, 0.35f}, 1.3f, Pise);
      for (float x : {-1.f, 1.f})
        g.pyramid({0, 1.35f * x, h}, {0.3f, 0.25f}, 0.8f, Pise);
    }
    if (lod == 0) {
      for (float side : {-1.f, 1.f}) {
        ksarRelief(g, -1.4f, 1.4f, side * 1.66f, h - 1.6f, side, 4, PiseDark);
        ksarRelief(g, -1.3f, 1.3f, side * 1.70f, h - 2.7f, side, 3, PiseDark);
        g.box({0.f, side * 1.82f, h * 0.55f}, {0.18f, 0.02f, 0.35f}, Window); // narrow slit window
      }
    }
    break;
  }
  case PropKind::KsarGate:
    for (float y : {-1.0f, 1.0f})
      g.frustum({0, y * 2.0f}, 0, 6.5f, {1.1f, 1.0f}, {1.0f, 0.9f}, Pise);
    g.box({0, 0, 5.3f}, {1.1f, 1.2f, 1.0f}, Pise);
    if (lod < 2) {
      for (float y : {-0.55f, 0.55f})
        g.box({0.15f, y, 2.1f}, {0.06f, 0.48f, 2.1f}, Wood, {}, 0);
      for (float y : {-2.0f, 0.f, 2.0f})
        g.pyramid({0, y, 6.3f}, {0.35f, 0.35f}, 0.9f, Pise);
    }
    if (lod == 0)
      ksarRelief(g, -0.9f, 0.9f, 1.25f, 5.4f, 1, 3, PiseDark);
    break;
  case PropKind::MudHouse:
    g.frustum({0, 0}, 0, 3.6f, {3.0f, 2.5f}, {2.9f, 2.4f}, PiseLight);
    if (lod < 2) {
      g.box({0, 2.35f, 3.75f}, {2.9f, 0.12f, 0.2f}, PiseLight);
      g.box({0, -2.35f, 3.75f}, {2.9f, 0.12f, 0.2f}, PiseLight);
      g.box({3.0f, 0, 1.0f}, {0.03f, 0.45f, 1.0f}, Wood);              // door
      g.box({3.0f, 1.5f, 2.3f}, {0.03f, 0.25f, 0.25f}, Window);        // windows
      g.box({-0.8f, -2.52f, 2.3f}, {0.25f, 0.03f, 0.25f}, Window);
    }
    break;
  case PropKind::ConcreteHouse: {
    const glm::vec3 paint(0.60f, 0.47f, 0.40f), shutters(0.05f, 0.17f, 0.12f);
    g.box({0, 0, 3.0f}, {3.5f, 3.0f, 3.0f}, paint);
    if (lod < 2) {
      g.box({0, 0, 6.15f}, {3.5f, 3.0f, 0.15f}, paint * 1.05f);
      for (float z : {1.4f, 4.4f})
        for (float y : {-1.6f, 1.6f})
          g.box({3.52f, y, z}, {0.03f, 0.45f, 0.55f}, shutters);
      g.box({3.52f, 0, 1.1f}, {0.03f, 0.55f, 1.1f}, Wood);
    }
    if (lod == 0) // rebar stubs left for a future storey (common in the region)
      for (float x : {-3.3f, 3.3f})
        for (float y : {-2.8f, 2.8f})
          g.box({x, y, 6.6f}, {0.03f, 0.03f, 0.35f}, glm::vec3(0.12f, 0.06f, 0.04f));
    break;
  }
  case PropKind::WaterTower:
    for (float x : {-1.2f, 1.2f})
      for (float y : {-1.2f, 1.2f})
        g.box({x, y, 5.0f}, {0.18f, 0.18f, 5.0f}, Concrete);
    g.cylinder({0, 0, 10.0f}, 3.0f, 2.3f, lod == 0 ? 16 : lod == 1 ? 10 : 6, glm::vec3(0.55f, 0.55f, 0.53f));
    if (lod == 0)
      g.cylinder({0, 0, 11.6f}, 0.5f, 2.32f, 16, glm::vec3(0.05f, 0.12f, 0.3f), false); // painted band
    break;
  case PropKind::PowerPole:
    g.frustum({0, 0}, 0, 9.0f, {0.16f, 0.16f}, {0.09f, 0.09f}, Concrete);
    g.box({0, 0, 8.4f}, {0.07f, 1.1f, 0.07f}, Concrete * 0.8f);
    if (lod == 0)
      for (float y : {-0.95f, 0.f, 0.95f})
        g.cylinder({0, y, 8.47f}, 0.25f, 0.05f, 6, glm::vec3(0.6f, 0.55f, 0.45f));
    break;
  case PropKind::Seguia:
    g.box({0, 0, 0.04f}, {2.0f, 0.42f, 0.04f}, Concrete * 0.9f);
    for (float y : {-0.36f, 0.36f})
      g.box({0, y, 0.25f}, {2.0f, 0.07f, 0.25f}, Concrete);
    if (lod == 0)
      g.box({0, 0, 0.1f}, {1.98f, 0.28f, 0.02f}, glm::vec3(0.05f, 0.08f, 0.07f)); // water
    break;
  case PropKind::StartArch: {
    // Fictional event arch: terracotta and indigo bands, no names or logos.
    const glm::vec3 terracotta(0.42f, 0.11f, 0.05f), indigo(0.04f, 0.05f, 0.2f), sand(0.62f, 0.5f, 0.34f);
    for (float y : {-7.2f, 7.2f}) {
      g.box({0, y, 3.2f}, {0.7f, 0.7f, 3.2f}, terracotta);
      if (lod < 2)
        g.box({0, y, 6.6f}, {0.75f, 0.75f, 0.2f}, sand);
    }
    g.box({0, 0, 7.4f}, {0.6f, 7.9f, 0.8f}, indigo);
    if (lod < 2)
      for (int i = -6; i <= 6; i += 2) // zellige-like diamond band (original pattern)
        g.box({0.62f, static_cast<float>(i), 7.4f}, {0.02f, 0.35f, 0.35f}, sand, {}, 0.785f);
    break;
  }
  case PropKind::SignBoard:
    for (float y : {-0.7f, 0.7f})
      g.box({0, y, 1.0f}, {0.05f, 0.05f, 1.0f}, glm::vec3(0.35f));
    g.box({0, 0, 2.2f}, {0.04f, 1.0f, 0.55f}, glm::vec3(0.75f));
    if (lod < 2)
      g.box({0.045f, 0, 2.2f}, {0.004f, 0.92f, 0.47f}, glm::vec3(0.62f, 0.62f, 0.6f));
    break;
  }
}
// NOLINTEND(bugprone-easily-swappable-parameters)
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
std::uint64_t fnv(std::uint64_t h, double v) {
  const auto q = static_cast<std::int64_t>(std::llround(v * 100.0));
  for (int i = 0; i < 8; ++i) {
    h ^= static_cast<std::uint64_t>(q >> (8 * i)) & 0xffu;
    h *= 1099511628211ULL;
  }
  return h;
}
} // namespace

vegetation::Mesh buildSignFace(unsigned face) {
  if (face >= SignFaces)
    throw std::invalid_argument("sign face out of range");
  // The sign_board panel: x = +-0.045 (faces), y in [-0.92, 0.92], z in [1.73, 2.67].
  const float u0 = static_cast<float>(face % SignColumns) / SignColumns, u1 = u0 + 1.f / SignColumns;
  const unsigned row = face / SignColumns; // integer row of the cell
  const float v0 = static_cast<float>(row) / static_cast<float>(SignRows), v1 = v0 + 1.f / SignRows; // v0 = top row
  Builder g;
  for (float side : {1.f, -1.f}) {
    // Seen from +x the reader's right is +y; from -x it is -y.
    std::array<std::uint16_t, 4> v{};
    for (int k = 0; k < 4; ++k) {
      const float y = (k & 1) ? .92f : -.92f, z = (k & 2) ? 2.67f : 1.73f;
      v[static_cast<std::size_t>(k)] = g.vertex({side * .047f, y, z}, {side, 0, 0}, glm::vec3(1), 1.f,
                                                {side * y > 0 ? u1 : u0, (k & 2) ? v0 : v1});
    }
    for (std::size_t i : {0u, 1u, 3u, 0u, 3u, 2u})
      g.mesh.indices.push_back(v[i]);
  }
  auto m = std::move(g.mesh);
  return m;
}

std::string_view propName(PropKind kind) { return Names.at(static_cast<std::size_t>(kind)); }
PropKind parsePropKind(std::string_view name) {
  for (std::size_t i = 0; i < Names.size(); ++i)
    if (Names[i] == name)
      return static_cast<PropKind>(i);
  throw std::runtime_error("unknown prop kind: " + std::string(name));
}
bool animated(PropKind kind) { return kind == PropKind::Camel || kind == PropKind::Goat; }
bool spansRoad(PropKind kind) { return kind == PropKind::StartArch; }
float propRadius(PropKind kind) { return Radius.at(static_cast<std::size_t>(kind)); }

vegetation::Mesh buildProp(PropKind kind, unsigned lod) {
  if (lod >= PropLods)
    throw std::out_of_range("prop LOD");
  Builder g;
  build(g, kind, lod);
  return std::move(g.mesh);
}

std::vector<Prop> placeProps(std::string_view text, const PropQueries &stage, const PropRules &rules, std::uint64_t seed,
                             PlacementReport *report) {
  const auto doc = nlohmann::json::parse(text);
  if (doc.value("schema", std::string()) != "trackrs.props/1")
    throw std::runtime_error("props schema must be trackrs.props/1");
  Pcg32 rng(seed, 0x50524f50ULL);
  auto unit = [&] { return static_cast<float>(rng.next() >> 8) / 16777216.f; };
  std::vector<Prop> wanted;
  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  auto add = [&](PropKind kind, float x, float y, float yaw, float scale) {
    Prop p;
    p.kind = kind;
    p.position = {x, y, 0};
    p.yaw = yaw;
    p.scale = scale;
    p.tint = static_cast<std::uint8_t>(rng.next() % 8);
    if (animated(kind)) {
      const float a = unit() * 2 * Pi;
      p.wander = {std::cos(a), std::sin(a)};
    }
    wanted.push_back(p);
  };
  auto num = [](const nlohmann::json &j, const char *k, float fallback) { return j.value(k, fallback); };
  for (const auto &p : doc.value("props", nlohmann::json::array())) {
    add(parsePropKind(p.at("kind").get<std::string>()), p.at("x").get<float>(), p.at("y").get<float>(),
        glm::radians(num(p, "yaw_deg", 0)), num(p, "scale", 1));
    if (p.contains("face")) {
      const int face = p.at("face").get<int>();
      if (wanted.back().kind != PropKind::SignBoard || face < 0 || face >= static_cast<int>(SignFaces))
        throw std::runtime_error("\"face\" needs a sign_board and a face in 0..7");
      wanted.back().face = static_cast<std::int8_t>(face);
    }
  }
  for (const auto &h : doc.value("herds", nlohmann::json::array())) {
    const auto kind = parsePropKind(h.at("kind").get<std::string>());
    const float cx = h.at("x").get<float>(), cy = h.at("y").get<float>(), r = num(h, "radius", 15);
    for (int i = 0; i < h.value("count", 6); ++i) {
      const float a = unit() * 2 * Pi, d = r * std::sqrt(unit());
      add(kind, cx + std::cos(a) * d, cy + std::sin(a) * d, unit() * 2 * Pi, 0.9f + 0.2f * unit());
    }
  }
  for (const auto &s : doc.value("scatter", nlohmann::json::array())) {
    const auto kind = parsePropKind(s.at("kind").get<std::string>());
    const float cx = s.at("x").get<float>(), cy = s.at("y").get<float>(), r = num(s, "radius", 40);
    const bool grid = kind == PropKind::MudHouse || kind == PropKind::ConcreteHouse;
    for (int i = 0; i < s.value("count", 5); ++i) {
      const float a = unit() * 2 * Pi, d = r * std::sqrt(unit());
      const float yaw = grid ? glm::radians(num(s, "yaw_deg", 0)) + (Pi / 2) * static_cast<float>(rng.next() % 4) + 0.1f * (unit() - 0.5f)
                             : unit() * 2 * Pi;
      add(kind, cx + std::cos(a) * d, cy + std::sin(a) * d, yaw, 0.9f + 0.25f * unit());
    }
  }
  for (const auto &l : doc.value("line", nlohmann::json::array())) {
    const auto kind = parsePropKind(l.at("kind").get<std::string>());
    const glm::vec2 a(l.at("from").at(0).get<float>(), l.at("from").at(1).get<float>());
    const glm::vec2 b(l.at("to").at(0).get<float>(), l.at("to").at(1).get<float>());
    const float spacing = std::max(num(l, "spacing", 40), 1.f), len = glm::length(b - a);
    const float yaw = std::atan2(b.y - a.y, b.x - a.x);
    for (int i = 0; i <= static_cast<int>(len / spacing); ++i) {
      const glm::vec2 p = a + (b - a) * (static_cast<float>(i) * spacing / std::max(len, 1e-3f));
      add(kind, p.x, p.y, yaw, 1);
    }
  }
  for (const auto &k : doc.value("ksar", nlohmann::json::array())) {
    const glm::vec2 c(k.at("x").get<float>(), k.at("y").get<float>());
    const float w = num(k, "width", 40), d = num(k, "depth", 30), yaw = glm::radians(num(k, "yaw_deg", 0));
    const int gateSide = k.value("gate_side", 0);
    const glm::mat2 rot(std::cos(yaw), std::sin(yaw), -std::sin(yaw), std::cos(yaw));
    const std::array<glm::vec2, 4> corners = {glm::vec2(-w / 2, -d / 2), glm::vec2(w / 2, -d / 2), glm::vec2(w / 2, d / 2),
                                              glm::vec2(-w / 2, d / 2)};
    for (int side = 0; side < 4; ++side) {
      const glm::vec2 p0 = corners[static_cast<std::size_t>(side)], p1 = corners[static_cast<std::size_t>((side + 1) % 4)];
      const float len = glm::length(p1 - p0);
      const int n = std::max(1, static_cast<int>(std::round(len / 10.f)));
      const float segYaw = yaw + std::atan2(p1.y - p0.y, p1.x - p0.x);
      for (int i = 0; i < n; ++i) {
        const glm::vec2 m = c + rot * (p0 + (p1 - p0) * ((static_cast<float>(i) + 0.5f) / static_cast<float>(n)));
        const bool gate = side == gateSide && i == n / 2;
        add(gate ? PropKind::KsarGate : PropKind::KsarWall, m.x, m.y, gate ? segYaw + Pi / 2 : segYaw,
            gate ? 1.f : len / (10.f * static_cast<float>(n)));
      }
      const glm::vec2 t = c + rot * p0;
      add(PropKind::KsarTower, t.x, t.y, yaw, 1);
    }
  }
  PlacementReport rep;
  rep.requested = wanted.size();
  rep.hash = 14695981039346656037ULL;
  std::vector<Prop> placed;
  for (auto p : wanted) {
    const float r = propRadius(p.kind) * p.scale;
    bool road = false;
    if (!spansRoad(p.kind)) {
      // Event signs (a sign_board showing a face) stand at the road edge, as
      // real ones do: 0.5 m clearance instead of the full margin. They are
      // still never on the road.
      const bool eventSign = p.kind == PropKind::SignBoard && p.face >= 0;
      const float reach = r + (eventSign ? .5f : rules.roadMargin);
      road = stage.onRoad(p.position.x, p.position.y);
      for (int i = 0; i < 12 && !road; ++i) {
        const float a = 2 * Pi * static_cast<float>(i) / 12.f;
        for (float f : {0.5f, 1.0f})
          road = road || stage.onRoad(p.position.x + std::cos(a) * reach * f, p.position.y + std::sin(a) * reach * f);
      }
    }
    if (road) {
      ++rep.rejectedRoad;
      continue;
    }
    float lo = 1e30f, hi = -1e30f;
    for (int i = 0; i < 4; ++i) {
      const float a = Pi / 4 + Pi / 2 * static_cast<float>(i);
      const float h = stage.height(p.position.x + std::cos(a) * r * 0.7f, p.position.y + std::sin(a) * r * 0.7f);
      lo = std::min(lo, h);
      hi = std::max(hi, h);
    }
    if (!spansRoad(p.kind) && (hi - lo) / std::max(1.4f * r, 0.5f) > rules.maxSlope) {
      ++rep.rejectedSlope;
      continue;
    }
    // Sit on the lowest footprint corner so nothing floats (walls sink a little).
    p.position.z = std::min(lo, stage.height(p.position.x, p.position.y)) - (animated(p.kind) ? 0.f : 0.05f);
    for (double v : {static_cast<double>(p.kind), static_cast<double>(p.position.x), static_cast<double>(p.position.y),
                     static_cast<double>(p.position.z), static_cast<double>(p.yaw), static_cast<double>(p.scale)})
      rep.hash = fnv(rep.hash, v);
    placed.push_back(p);
  }
  rep.placed = placed.size();
  if (report)
    *report = rep;
  return placed;
}

PropPose animateProp(const Prop &prop, float time, const std::function<float(float, float)> &height) {
  if (!animated(prop.kind) || glm::dot(prop.wander, prop.wander) < 1e-6f)
    return {prop.position, prop.yaw, 0};
  // Slow grazing walk: 6 m out and back over two minutes, turning at the ends.
  const float phase = std::fmod(std::abs(prop.position.x * 0.37f + prop.position.y * 0.11f), 2 * Pi);
  const float t = time * (2 * Pi / 120.f) + phase;
  const float reach = prop.kind == PropKind::Camel ? 6.f : 3.f;
  const glm::vec2 p = glm::vec2(prop.position) + prop.wander * (reach * std::sin(t));
  const float velocity = std::cos(t);
  const float yaw = std::atan2(prop.wander.y, prop.wander.x) + (velocity < 0 ? Pi : 0.f);
  const float stride = std::abs(velocity) > 0.15f ? std::fmod(time * (prop.kind == PropKind::Camel ? 0.9f : 1.6f) + phase, 1.f) : 0.f;
  return {glm::vec3(p, height(p.x, p.y)), yaw, stride};
}
} // namespace trackrs::dressing
