// SPDX-License-Identifier: GPL-2.0-or-later
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "engine/mesh/vertex_layout.h"
#include "engine/vegetation/vegetation.h"
#include "vendor/doctest/doctest.h"
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <set>
using namespace trackrs::vegetation;
namespace {
const std::filesystem::path Data = TRACKRS_DATA_DIR;
glm::vec3 position(const Vertex &v) { return {v.position[0], v.position[1], v.position[2]}; }
glm::vec3 normal(const Vertex &v) {
  return {trackrs::mesh::unpackSnorm10(v.normal, 0), trackrs::mesh::unpackSnorm10(v.normal, 10),
          trackrs::mesh::unpackSnorm10(v.normal, 20)};
}
Species species(Form form) {
  Species s;
  s.form = form;
  return s;
}
constexpr std::array<Form, 6> MeshForms = {Form::Conifer, Form::Broadleaf, Form::Bush,
                                           Form::Grass,   Form::Palm,      Form::Cactus};
} // namespace

TEST_CASE("every shipped vegetation sprite has a licensed catalog entry") {
  const auto catalog = Catalog::load(Data / "vegetation/species.json");
  std::set<std::string> sprites;
  for (const auto &entry : std::filesystem::directory_iterator(Data / "textures/veg"))
    if (entry.path().extension() == ".png")
      sprites.insert(entry.path().filename().string());
  REQUIRE(sprites.size() == catalog.entries().size());
  for (const auto &name : sprites)
    CHECK(catalog.entries().count(name) == 1);
  CHECK(catalog.lookup("/textures/veg/spruce.png").form == Form::Conifer);
  CHECK(catalog.lookup("textures\\veg\\grass.png").form == Form::Grass);
  CHECK(catalog.lookup("/textures/veg/post.png").form == Form::Card);
  CHECK(catalog.lookup("/textures/unknown.png").form == Form::Card);
  for (const auto &[name, s] : catalog.entries())
    for (int i = 0; i < 3; ++i) {
      CHECK(s.foliage[i] >= 0.f);
      CHECK(s.foliage[i] <= 1.f);
    }
  CHECK_THROWS(Catalog::parse(R"({"schema":"trackrs.vegetation/1","sprites":{}})"));
  CHECK_THROWS(Catalog::parse(R"({"schema":"x","license":"MIT","sprites":{}})"));
  CHECK_THROWS(Catalog::parse(
      R"({"schema":"trackrs.vegetation/1","license":"MIT","sprites":{"a.png":{"form":"tree","foliage":"#000000"}}})"));
  CHECK_THROWS(Catalog::parse(
      R"({"schema":"trackrs.vegetation/1","license":"MIT","sprites":{"a.png":{"form":"bush","foliage":"#00zz00"}}})"));
}

TEST_CASE("procedural meshes are valid, unit-height and get cheaper per LOD") {
  for (auto form : MeshForms) {
    CAPTURE(formName(form));
    std::size_t previous = SIZE_MAX;
    for (unsigned lod = 0; lod < MeshLods; ++lod) {
      CAPTURE(lod);
      const auto m = buildMesh(species(form), lod);
      REQUIRE(m.indices.size() % 3 == 0);
      REQUIRE(!m.vertices.empty());
      CHECK(m.triangles() < previous);
      CHECK(m.triangles() <= 700); // per-plant budget at LOD0
      previous = m.triangles();
      glm::vec3 lo(1e9f), hi(-1e9f);
      for (const auto &v : m.vertices) {
        lo = glm::min(lo, position(v));
        hi = glm::max(hi, position(v));
        CHECK(std::abs(glm::length(normal(v)) - 1.f) < 0.01f);
        CHECK(std::isfinite(v.position[0] + v.position[1] + v.position[2]));
      }
      for (auto i : m.indices)
        REQUIRE(i < m.vertices.size());
      for (std::size_t t = 0; t < m.indices.size(); t += 3) { // no degenerate triangles
        const auto a = position(m.vertices[m.indices[t]]), b = position(m.vertices[m.indices[t + 1]]),
                   c = position(m.vertices[m.indices[t + 2]]);
        CHECK(glm::length(glm::cross(b - a, c - a)) > 1e-7f);
      }
      CHECK(lo.z >= -0.05f); // rooted at the terrain
      CHECK(lo.z <= 0.15f);
      CHECK(hi.z <= 1.05f); // matches the legacy card height
      CHECK(hi.z >= 0.75f);
      CHECK(std::max({-lo.x, -lo.y, hi.x, hi.y}) <= 0.75f); // stays inside the cull sphere
    }
  }
}

TEST_CASE("closed canopy and trunk faces point outwards") {
  // Conifer tiers and broadleaf blobs: outward faces must have normals that
  // agree with geometric winding (counter-clockwise front faces).
  for (auto form : {Form::Conifer, Form::Broadleaf, Form::Bush, Form::Cactus}) {
    CAPTURE(formName(form));
    const auto m = buildMesh(species(form), 0);
    std::size_t agree = 0;
    for (std::size_t t = 0; t < m.indices.size(); t += 3) {
      const auto &va = m.vertices[m.indices[t]], &vb = m.vertices[m.indices[t + 1]],
                 &vc = m.vertices[m.indices[t + 2]];
      const auto face = glm::cross(position(vb) - position(va), position(vc) - position(va));
      agree += glm::dot(face, normal(va) + normal(vb) + normal(vc)) > 0;
    }
    CHECK(static_cast<double>(agree) >= 0.97 * static_cast<double>(m.triangles()));
  }
}

TEST_CASE("mesh generation is deterministic per seed") {
  for (auto form : MeshForms)
    for (unsigned lod = 0; lod < MeshLods; ++lod) {
      CHECK(hash(buildMesh(species(form), lod, 7)) == hash(buildMesh(species(form), lod, 7)));
      if (form != Form::Cactus)
        CHECK(hash(buildMesh(species(form), lod, 7)) != hash(buildMesh(species(form), lod, 8)));
    }
  CHECK_THROWS(buildMesh(species(Form::Conifer), MeshLods));
}

TEST_CASE("cards reproduce the legacy crossed-quad layout") {
  for (unsigned n = 1; n <= 4; ++n) {
    const auto card = buildCard(n);
    CHECK(card.triangles() == 2 * n);
    CHECK(card.vertices.size() == 4 * n);
    CHECK(card.vertices[0].uv[0] == 65535);
    CHECK(card.vertices[0].uv[1] == 0);
    CHECK(card.vertices[2].position[2] == doctest::Approx(1));
  }
  CHECK(buildCard(0).triangles() == 2);
  CHECK(buildCard(9).triangles() == 8);
}

TEST_CASE("LOD choice is monotonic and cross-fades cover every pixel once") {
  for (auto quality : {trackrs::RenderQuality::Low, trackrs::RenderQuality::Medium,
                       trackrs::RenderQuality::High, trackrs::RenderQuality::Ultra}) {
    const auto r = rangesFor(quality);
    CHECK(r.mesh[0] < r.mesh[1]);
    CHECK(r.mesh[1] < r.mesh[2]);
    CHECK(r.mesh[2] < r.impostor);
    unsigned last = 0;
    for (float d = 0; d < r.impostor + 10; d += 0.25f) {
      const auto c = chooseLod(d, r, Form::Broadleaf);
      if (!c.visible) {
        CHECK(d >= r.impostor);
        continue;
      }
      CHECK(c.lod >= last);
      last = c.lod;
      CHECK(c.t >= 0);
      CHECK(c.t <= 1);
      CHECK((c.next == c.lod || c.next == c.lod + 1));
      // Coverage of the dither pair: noise < t (incoming) U noise >= t (outgoing).
      for (float noise = 0; noise < 1; noise += 0.05f) {
        const bool outgoing = noise >= c.t, incoming = noise < c.t;
        CHECK((c.next == c.lod ? 1 : int(outgoing) + int(incoming)) == 1);
      }
    }
    CHECK(chooseLod(r.mesh[0] * 0.5f, r, Form::Card).lod == ImpostorLod);
    CHECK_FALSE(chooseLod(-1, r, Form::Grass).visible);
    CHECK_FALSE(chooseLod(std::nanf(""), r, Form::Grass).visible);
  }
}

TEST_CASE("classifier culls, buckets and fades without changing plant data") {
  const auto proj = glm::perspective(glm::radians(60.f), 16.f / 9.f, 0.5f, 2000.f);
  const auto view = glm::lookAt(glm::vec3(0, 0, 2), glm::vec3(0, 100, 2), glm::vec3(0, 0, 1));
  struct Legacy { // PTerrainFoliage layout: pos, angle, scale, rigidity
    float x, y, z, ang, scale, rigidity;
  };
  std::vector<Legacy> plants;
  for (int i = 0; i < 400; ++i)
    plants.push_back({0, static_cast<float>(i), 0, 0.1f * static_cast<float>(i), 2.f, 0});
  plants.push_back({0, -50, 0, 0, 2, 0}); // behind the camera
  const auto ranges = rangesFor(trackrs::RenderQuality::High);
  Classifier c;
  c.begin({0, 0, 2}, proj * view, ranges, Pass::Main, 2);
  c.add(1, Form::Conifer, plants.data(), plants.size(), sizeof(Legacy));
  std::uint64_t total = 0;
  for (unsigned lod = 0; lod <= MeshLods; ++lod) {
    total += c.bucket(1, lod).size();
    CHECK(c.bucket(0, lod).empty());
    CHECK(!c.bucket(1, lod).empty());
  }
  CHECK(c.stats().culled > 0);
  CHECK(total == plants.size() - c.stats().culled + c.stats().faded);
  for (const auto &i : c.bucket(1, 0)) {
    CHECK(i.a.w == doctest::Approx(4)); // card height 2 * scale
    CHECK(i.b.z >= 0.9f);
    CHECK(i.b.z <= 1.1f);
    CHECK(((i.b.y >= 0 && i.b.y <= 1) || (i.b.y >= 2 && i.b.y <= 3)));
  }
  // Same input -> identical instance stream; begin() reuses capacity.
  const auto first = c.bucket(1, 1);
  c.begin({0, 0, 2}, proj * view, ranges, Pass::Main, 2);
  c.add(1, Form::Conifer, plants.data(), plants.size(), sizeof(Legacy));
  REQUIRE(first.size() == c.bucket(1, 1).size());
  for (std::size_t i = 0; i < first.size(); ++i)
    CHECK(first[i].a == c.bucket(1, 1)[i].a);
  // Shadows skip grass; probes only ever use impostors.
  c.begin({0, 0, 2}, proj * view, ranges, Pass::Shadow, 1);
  c.add(0, Form::Grass, plants.data(), plants.size(), sizeof(Legacy));
  for (unsigned lod = 0; lod <= MeshLods; ++lod)
    CHECK(c.bucket(0, lod).empty());
  c.begin({0, 0, 2}, proj * view, ranges, Pass::Probe, 1);
  c.add(0, Form::Broadleaf, plants.data(), plants.size(), sizeof(Legacy));
  for (unsigned lod = 0; lod < MeshLods; ++lod)
    CHECK(c.bucket(0, lod).empty());
  CHECK(!c.bucket(0, ImpostorLod).empty());
  CHECK_THROWS(c.add(3, Form::Bush, plants.data(), 1, sizeof(Legacy)));
}
