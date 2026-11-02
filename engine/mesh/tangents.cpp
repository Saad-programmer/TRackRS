// SPDX-License-Identifier: GPL-2.0-or-later
#include "vertex_layout.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <limits>
#include <mikktspace.h>
namespace trackrs::mesh {
namespace {
struct Context {
  std::vector<Vertex> *corners;
};
Vertex &corner(const SMikkTSpaceContext *context, int face, int vertex) {
  return static_cast<Context *>(context->m_pUserData)
      ->corners->at(static_cast<std::size_t>(face) * 3 +
                    static_cast<std::size_t>(vertex));
}
int faces(const SMikkTSpaceContext *c) {
  return static_cast<int>(
      static_cast<Context *>(c->m_pUserData)->corners->size() / 3);
}
int faceVertices(const SMikkTSpaceContext *, int) { return 3; }
void position(const SMikkTSpaceContext *c, float out[], int f, int v) {
  std::copy_n(corner(c, f, v).position, 3, out);
}
void normal(const SMikkTSpaceContext *c, float out[], int f, int v) {
  std::copy_n(corner(c, f, v).normal, 3, out);
}
void uv(const SMikkTSpaceContext *c, float out[], int f, int v) {
  std::copy_n(corner(c, f, v).uv, 2, out);
}
// Reference MikkTSpace callback ABI fixes parameter order and types.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void tangent(const SMikkTSpaceContext *c, const float t[], float sign, int f,
             int v) {
  auto &vertex = corner(c, f, v);
  std::copy_n(t, 3, vertex.tangent);
  const auto b =
      glm::cross(glm::make_vec3(vertex.normal), glm::make_vec3(t)) * sign;
  std::copy_n(glm::value_ptr(b), 3, vertex.bitangent);
}
} // namespace
void computeTangents(std::vector<Vertex> &vertices,
                     std::vector<std::uint32_t> &indices,
                     std::vector<std::uint32_t> *colors) {
  if (indices.size() % 3 ||
      indices.size() >
          static_cast<std::size_t>(std::numeric_limits<int>::max()))
    throw std::invalid_argument("invalid triangle index count");
  if (colors && colors->size() != vertices.size())
    throw std::invalid_argument("vertex color count mismatch");
  std::vector<Vertex> corners;
  std::vector<std::uint32_t> cornerColors;
  corners.reserve(indices.size());
  for (auto index : indices) {
    if (index >= vertices.size())
      throw std::out_of_range("tangent index");
    corners.push_back(vertices[index]);
    if (colors)
      cornerColors.push_back(colors->at(index));
  }
  for (auto &v : corners) {
    for (float value : v.position)
      if (!std::isfinite(value))
        throw std::invalid_argument("non-finite tangent position");
    for (float value : v.uv)
      if (!std::isfinite(value))
        throw std::invalid_argument("non-finite tangent UV");
    auto n = glm::make_vec3(v.normal);
    if (!std::isfinite(glm::dot(n, n)) || glm::dot(n, n) < 1e-20f)
      throw std::invalid_argument("invalid tangent normal");
    n = glm::normalize(n);
    std::copy_n(glm::value_ptr(n), 3, v.normal);
  }
  Context user{&corners};
  SMikkTSpaceInterface interface{};
  interface.m_getNumFaces = faces;
  interface.m_getNumVerticesOfFace = faceVertices;
  interface.m_getPosition = position;
  interface.m_getNormal = normal;
  interface.m_getTexCoord = uv;
  interface.m_setTSpaceBasic = tangent;
  SMikkTSpaceContext context{&interface, &user};
  if (!corners.empty() && !genTangSpaceDefault(&context))
    throw std::runtime_error("MikkTSpace generation failed");
  for (auto &v : corners) {
    const auto n = glm::make_vec3(v.normal);
    auto t = glm::make_vec3(v.tangent);
    t -= n * glm::dot(n, t);
    if (!std::isfinite(glm::dot(t, t)) || glm::dot(t, t) < 1e-12f)
      t = glm::cross(
          std::abs(n.z) < .9f ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0), n);
    t = glm::normalize(t);
    const auto old = glm::make_vec3(v.bitangent);
    const float sign = glm::dot(glm::cross(n, t), old) < 0 ? -1.f : 1.f;
    const auto b = glm::cross(n, t) * sign;
    std::copy_n(glm::value_ptr(t), 3, v.tangent);
    std::copy_n(glm::value_ptr(b), 3, v.bitangent);
  }
  vertices = std::move(corners);
  indices.resize(vertices.size());
  for (std::size_t i = 0; i < indices.size(); ++i)
    indices[i] = static_cast<std::uint32_t>(i);
  if (colors)
    *colors = std::move(cornerColors);
}
} // namespace trackrs::mesh
