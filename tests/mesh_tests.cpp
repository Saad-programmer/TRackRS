// SPDX-License-Identifier: GPL-2.0-or-later
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "engine/mesh/mesh_importer.h"
#include <cstring>
#include <doctest/doctest.h>
#include <filesystem>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <map>
#include <nlohmann/json.hpp>
using namespace trackrs::mesh;
namespace {
std::vector<Vertex> quad() {
  std::vector<Vertex> v(4);
  float positions[4][3] = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
  for (int i = 0; i < 4; ++i) {
    std::copy_n(positions[i], 3, v[i].position);
    v[i].normal[2] = 1;
    v[i].uv[0] = positions[i][0];
    v[i].uv[1] = positions[i][1];
  }
  return v;
}
} // namespace
TEST_CASE("Appendix C MikkTSpace quad, mirrored UV and degenerate UV") {
  for (int mode = 0; mode < 3; ++mode) {
    auto v = quad();
    std::vector<std::uint32_t> indices = {0, 1, 2, 0, 2, 3};
    for (auto &vertex : v)
      if (mode == 1)
        vertex.uv[0] = -vertex.uv[0];
      else if (mode == 2)
        vertex.uv[0] = vertex.uv[1] = 0;
    computeTangents(v, indices);
    for (auto &vertex : v) {
      auto n = glm::make_vec3(vertex.normal),
           t = glm::make_vec3(vertex.tangent),
           b = glm::make_vec3(vertex.bitangent);
      CHECK(glm::length(t) == doctest::Approx(1));
      CHECK(glm::length(b) == doctest::Approx(1));
      CHECK(glm::dot(n, t) == doctest::Approx(0));
      CHECK(glm::dot(n, b) == doctest::Approx(0));
      if (mode < 2) {
        CHECK(vertex.tangent[0] == doctest::Approx(mode == 1 ? -1 : 1));
        CHECK(vertex.bitangent[1] == doctest::Approx(1));
        CHECK(handedness(vertex) == (mode == 1 ? -1 : 1));
      }
    }
  }
}
TEST_CASE("Appendix B ABI, packed signs, saturation and quantization") {
  CHECK(sizeof(Vertex) == 56);
  CHECK(sizeof(VertexPacked) == 32);
  CHECK(packRGBA8(-1, 1, 2, .5f) == 0x80ffff00u);
  auto v = quad();
  std::vector<std::uint32_t> indices = {0, 1, 2};
  computeTangents(v, indices);
  for (auto &vertex : v) {
    auto p = packVertex(vertex);
    CHECK(unpackSnorm10(p.normal, 20) == 1);
    CHECK(unpackSnorm10(p.tangent, 0) == 1);
    CHECK((p.tangent >> 30) == 1);
  }
  CHECK((packSnorm2_10_10_10(-2, 0, 2, -1) >> 30) == 3);
  CHECK_THROWS(packSnorm2_10_10_10(NAN, 0, 0, 1));
  std::vector<std::uint32_t> bad = {0, 1, 99};
  CHECK_THROWS(computeTangents(v, bad));
}
TEST_CASE("OBJ negative indices, multiple materials, reference tangents and "
          "optimizer") {
  const std::string source =
      "v 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\nvt 0 0\nvt 1 0\nvt 1 1\nvt 0 "
      "1\nusemtl paint\nf -4/1 -3/2 -2/3 -1/4\nusemtl glass\nf 1/1 2/2 3/3\n";
  auto read = [&](const std::string &) {
    return Bytes(source.begin(), source.end());
  };
  auto mesh = MeshImporter::load("car.obj", read);
  REQUIRE(mesh.submeshes.size() == 2);
  CHECK(mesh.optimized);
  CHECK(mesh.sourceCorners == 9);
  CHECK(mesh.submeshes[0].vertices.size() == 4);
  for (auto &sub : mesh.submeshes) {
    CHECK(mesh.materials[sub.material].path == MaterialPath::Legacy);
    CHECK(sub.packed.size() == sub.vertices.size());
    for (auto index : sub.indices)
      CHECK(index < sub.vertices.size());
  }
  auto invalid = [](const std::string &) {
    std::string s = "v 0 0 0\nf 1 2 3\n";
    return Bytes(s.begin(), s.end());
  };
  CHECK_THROWS(MeshImporter::load("bad.obj", invalid));
  ImportOptions limit;
  limit.maxBytes = 3;
  CHECK_THROWS(MeshImporter::load("car.obj", read, limit));
}
TEST_CASE("cgltf test car carries complete PBR materials and optimized indexed "
          "submeshes") {
  auto file = std::filesystem::path(TRACKRS_TEST_ASSETS) / "car.gltf";
  auto mesh = MeshImporter::load(file.string());
  REQUIRE(mesh.submeshes.size() == 8);
  CHECK(mesh.sourceCorners == 288);
  CHECK(mesh.optimized);
  for (const auto &sub : mesh.submeshes) {
    REQUIRE(sub.material < mesh.materials.size());
    const auto &m = mesh.materials[sub.material];
    CHECK(m.path == MaterialPath::Pbr);
    CHECK_FALSE(m.albedo.empty());
    CHECK_FALSE(m.normal.empty());
    CHECK_FALSE(m.metalRough.empty());
    CHECK(sub.indices.size() == 36);
    CHECK(sub.vertices.size() <= 36);
    for (const auto &v : sub.vertices) {
      CHECK(std::isfinite(v.tangent[0]));
      CHECK(glm::length(glm::make_vec3(v.tangent)) == doctest::Approx(1));
    }
  }
  ImportOptions raw;
  raw.optimize = false;
  auto unoptimized = MeshImporter::load(file.string(), {}, raw);
  CHECK(unoptimized.sourceCorners == mesh.sourceCorners);
  CHECK_FALSE(unoptimized.optimized);
  for (std::size_t i = 0; i < mesh.submeshes.size(); ++i) {
    auto triangles = [](const Submesh &sub) {
      std::vector<std::array<float, 9>> result;
      for (std::size_t j = 0; j < sub.indices.size(); j += 3) {
        std::array<float, 9> triangle{};
        for (std::size_t k = 0; k < 3; ++k)
          std::copy_n(sub.vertices[sub.indices[j + k]].position, 3,
                      triangle.begin() + static_cast<std::ptrdiff_t>(k * 3));
        result.push_back(triangle);
      }
      std::sort(result.begin(), result.end());
      return result;
    };
    CHECK(triangles(mesh.submeshes[i]) == triangles(unoptimized.submeshes[i]));
  }
}
TEST_CASE("OBJ external dependencies cannot escape the model root") {
  std::string source = "mtllib ../outside.mtl\n";
  int reads = 0;
  auto reader = [&](const std::string &) {
    ++reads;
    return Bytes(source.begin(), source.end());
  };
  CHECK_THROWS(MeshImporter::load("models/car.obj", reader));
  CHECK(reads == 1);
}
TEST_CASE("cgltf external and GLB buffers, node transforms, missing textures "
          "and budgets") {
  std::array<float, 24> floats = {0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1,
                                  0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1};
  Bytes binary(sizeof(floats));
  std::memcpy(binary.data(), floats.data(), binary.size());
  nlohmann::json document = {
      {"asset", {{"version", "2.0"}}},
      {"buffers", {{{"uri", "model.bin"}, {"byteLength", binary.size()}}}},
      {"bufferViews",
       {{{"buffer", 0}, {"byteOffset", 0}, {"byteLength", 36}},
        {{"buffer", 0}, {"byteOffset", 36}, {"byteLength", 36}},
        {{"buffer", 0}, {"byteOffset", 72}, {"byteLength", 24}}}},
      {"accessors",
       {{{"bufferView", 0},
         {"componentType", 5126},
         {"count", 3},
         {"type", "VEC3"},
         {"min", {0, 0, 0}},
         {"max", {1, 1, 0}}},
        {{"bufferView", 1},
         {"componentType", 5126},
         {"count", 3},
         {"type", "VEC3"}},
        {{"bufferView", 2},
         {"componentType", 5126},
         {"count", 3},
         {"type", "VEC2"}}}},
      {"meshes",
       {{{"primitives",
          {{{"attributes",
             {{"POSITION", 0}, {"NORMAL", 1}, {"TEXCOORD_0", 2}}}}}}}}},
      {"nodes",
       {{{"mesh", 0}, {"translation", {2, 3, 4}}, {"scale", {-2, 3, 1}}}}},
      {"scenes", {{{"nodes", {0}}}}},
      {"scene", 0}};
  auto reader = [&](const std::string &path) {
    if (std::filesystem::path(path).extension() == ".bin")
      return binary;
    auto text = document.dump();
    return Bytes(text.begin(), text.end());
  };
  auto mesh = MeshImporter::load("model.gltf", reader);
  REQUIRE(mesh.submeshes.size() == 1);
  CHECK(mesh.materials[0].path == MaterialPath::Legacy);
  CHECK(mesh.submeshes[0].indices.size() == 3);
  auto &sub = mesh.submeshes[0];
  auto a = glm::make_vec3(sub.vertices[sub.indices[0]].position),
       b = glm::make_vec3(sub.vertices[sub.indices[1]].position),
       c = glm::make_vec3(sub.vertices[sub.indices[2]].position);
  CHECK(glm::cross(b - a, c - a).z > 0);
  for (auto &v : sub.vertices) {
    CHECK(v.position[2] == 4);
    CHECK(v.normal[2] == 1);
    CHECK(handedness(v) == -1);
  }
  ImportOptions cap;
  cap.maxCorners = 2;
  CHECK_THROWS(MeshImporter::load("model.gltf", reader, cap));
  document["buffers"][0]["uri"] = "../outside.bin";
  CHECK_THROWS(MeshImporter::load("models/model.gltf", reader));
  document["buffers"][0].erase("uri");
  auto text = document.dump();
  while (text.size() % 4)
    text += ' ';
  Bytes glb;
  auto word = [&](std::uint32_t v) {
    for (unsigned i = 0; i < 4; ++i)
      glb.push_back(static_cast<std::uint8_t>(v >> (i * 8)));
  };
  word(0x46546c67u);
  word(2);
  word(static_cast<std::uint32_t>(12 + 8 + text.size() + 8 + binary.size()));
  word(static_cast<std::uint32_t>(text.size()));
  word(0x4e4f534au);
  glb.insert(glb.end(), text.begin(), text.end());
  word(static_cast<std::uint32_t>(binary.size()));
  word(0x004e4942u);
  glb.insert(glb.end(), binary.begin(), binary.end());
  auto embedded = [&](const std::string &) { return glb; };
  CHECK(MeshImporter::load("model.glb", embedded).sourceCorners == 3);
}
TEST_CASE("incomplete glTF PBR texture sets choose the legacy material path") {
  auto file = std::filesystem::path(TRACKRS_TEST_ASSETS) / "car.gltf";
  auto read = [&](const std::string &path) {
    std::ifstream input(path, std::ios::binary);
    REQUIRE(bool(input));
    Bytes bytes(std::istreambuf_iterator<char>(input), {});
    if (std::filesystem::path(path).extension() == ".gltf") {
      auto document = nlohmann::json::parse(bytes);
      document["materials"][0].erase("normalTexture");
      auto text = document.dump();
      return Bytes(text.begin(), text.end());
    }
    return bytes;
  };
  auto mesh = MeshImporter::load(file.string(), read);
  CHECK(mesh.materials[0].path == MaterialPath::Legacy);
  CHECK(mesh.materials[1].path == MaterialPath::Pbr);
  CHECK(mesh.materials[2].path == MaterialPath::Pbr);
}
TEST_CASE("OBJ material textures resolve relative to the MTL directory") {
  std::map<std::string, std::string> files = {
      {"model/car.obj", "mtllib materials/paint.mtl\nv 0 0 0\nv 1 0 0\nv 0 1 "
                        "0\nusemtl Red\nf 1 2 3\nusemtl Blue\nf 1 2 3\n"},
      {"model/materials/paint.mtl",
       "newmtl Red\nKd 0.3 0.2 0.1\nmap_Kd ../textures/red.png\nnewmtl "
       "Blue\nKd 0.1 0.2 0.3\n"},
      {"model/textures/red.png", "fixture-image"}};
  auto read = [&](const std::string &file) {
    const auto &bytes = files.at(file);
    return Bytes(bytes.begin(), bytes.end());
  };
  auto model = MeshImporter::load("model/car.obj", read);
  REQUIRE(model.submeshes.size() == 2);
  const auto &red = model.materials[model.submeshes[0].material];
  CHECK(red.name == "Red");
  CHECK(red.baseColor[0] == doctest::Approx(.3));
  CHECK_FALSE(red.albedo.empty());
  CHECK(red.path == MaterialPath::Legacy);
  CHECK(model.materials[model.submeshes[1].material].name == "Blue");
}
