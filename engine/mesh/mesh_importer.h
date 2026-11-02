// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "vertex_layout.h"
#include <array>
#include <functional>
#include <string>
#include <vector>
namespace trackrs::mesh {
using Bytes = std::vector<std::uint8_t>;
using ReadFile = std::function<Bytes(const std::string &)>;
struct Texture {
  std::string name;
  Bytes encoded;
  bool empty() const { return encoded.empty(); }
};
enum class MaterialPath : std::uint8_t { Legacy, Pbr };
enum class AlphaMode : std::uint8_t { Opaque, Mask, Blend };
struct Material {
  std::string name;
  MaterialPath path = MaterialPath::Legacy;
  Texture albedo, normal, metalRough;
  std::array<float, 4> baseColor{{1, 1, 1, 1}};
  float metallic = 1, roughness = 1, normalScale = 1, aoStrength = 0,
        clearcoat = 0, clearcoatRoughness = .03f, alphaCutoff = .5f;
  bool doubleSided = false;
  AlphaMode alpha = AlphaMode::Opaque;
};
struct Submesh {
  std::string name;
  std::size_t material = 0;
  std::vector<Vertex> vertices;
  std::vector<VertexPacked> packed;
  std::vector<std::uint32_t> indices;
};
struct MeshData {
  std::vector<Material> materials;
  std::vector<Submesh> submeshes;
  std::size_t sourceCorners = 0;
  bool optimized = false;
};
struct ImportOptions {
  bool optimize = true;
  bool gltfYUpToZUp = false;
  std::size_t maxBytes = std::size_t{128} * 1024 * 1024;
  std::size_t maxCorners = 6000000;
};
class MeshImporter {
public:
  // Reads are local filesystem by default, or virtual PhysFS reads supplied by
  // the application. External URIs are restricted to the model's directory
  // tree.
  static MeshData load(const std::string &filename, const ReadFile &reader = {},
                       const ImportOptions &options = {});
};
} // namespace trackrs::mesh
