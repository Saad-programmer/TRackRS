// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
// Appendix B ABI. CPU-only; GPU attribute setup lives in the PBR mesh owner.
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <type_traits>
#include <vector>
namespace trackrs::mesh {
struct Vertex {
  float position[3]{};
  float normal[3]{};
  float uv[2]{};
  float tangent[3]{};
  float bitangent[3]{};
};
struct VertexPacked {
  float position[3]{};
  std::uint32_t normal = 0, tangent = 0;
  float uv[2]{};
  std::uint32_t color = 0xffffffffu;
};
static_assert(std::is_standard_layout_v<Vertex> &&
              std::is_standard_layout_v<VertexPacked>);
static_assert(sizeof(Vertex) == 56 && alignof(Vertex) == 4);
static_assert(offsetof(Vertex, position) == 0 &&
              offsetof(Vertex, normal) == 12 && offsetof(Vertex, uv) == 24 &&
              offsetof(Vertex, tangent) == 32 &&
              offsetof(Vertex, bitangent) == 44);
static_assert(sizeof(VertexPacked) == 32 && alignof(VertexPacked) == 4);
static_assert(offsetof(VertexPacked, position) == 0 &&
              offsetof(VertexPacked, normal) == 12 &&
              offsetof(VertexPacked, tangent) == 16 &&
              offsetof(VertexPacked, uv) == 20 &&
              offsetof(VertexPacked, color) == 28);
// Coordinate order follows the GL packed XYZW format.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
inline std::uint32_t packSnorm2_10_10_10(float x, float y, float z, int w) {
  const auto component = [](float v) {
    if (!std::isfinite(v))
      throw std::invalid_argument("non-finite packed direction");
    return static_cast<std::uint32_t>(
               std::lround(std::clamp(v, -1.f, 1.f) * 511.f)) &
           0x3ffu;
  };
  const std::uint32_t sign = w > 0 ? 1u : w < 0 ? 3u : 0u;
  return component(x) | (component(y) << 10) | (component(z) << 20) |
         (sign << 30);
}
inline std::uint32_t packRGBA8(float r, float g, float b, float a) {
  const auto component = [](float v) {
    if (!std::isfinite(v))
      throw std::invalid_argument("non-finite packed color");
    return static_cast<std::uint32_t>(
        std::lround(std::clamp(v, 0.f, 1.f) * 255.f));
  };
  return component(r) | (component(g) << 8) | (component(b) << 16) |
         (component(a) << 24);
}
inline float unpackSnorm10(std::uint32_t value, unsigned shift) {
  const auto bits = (value >> shift) & 1023u;
  const int signedValue =
      bits & 512u ? static_cast<int>(bits) - 1024 : static_cast<int>(bits);
  return std::max(-1.f, static_cast<float>(signedValue) / 511.f);
}
inline int handedness(const Vertex &v) {
  const float cross[3] = {
      v.normal[1] * v.tangent[2] - v.normal[2] * v.tangent[1],
      v.normal[2] * v.tangent[0] - v.normal[0] * v.tangent[2],
      v.normal[0] * v.tangent[1] - v.normal[1] * v.tangent[0]};
  return cross[0] * v.bitangent[0] + cross[1] * v.bitangent[1] +
                     cross[2] * v.bitangent[2] <
                 0
             ? -1
             : 1;
}
inline VertexPacked packVertex(const Vertex &v,
                               std::uint32_t color = 0xffffffffu) {
  VertexPacked out;
  std::copy_n(v.position, 3, out.position);
  std::copy_n(v.uv, 2, out.uv);
  out.normal = packSnorm2_10_10_10(v.normal[0], v.normal[1], v.normal[2], 0);
  out.tangent = packSnorm2_10_10_10(v.tangent[0], v.tangent[1], v.tangent[2],
                                    handedness(v));
  out.color = color;
  return out;
}
// Splits per-corner tangent discontinuities with reference MikkTSpace. Indices
// and optional colors are remapped together; degenerate UVs get a finite basis.
void computeTangents(std::vector<Vertex> &vertices,
                     std::vector<std::uint32_t> &indices,
                     std::vector<std::uint32_t> *colors = nullptr);
} // namespace trackrs::mesh
