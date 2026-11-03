// SPDX-License-Identifier: GPL-2.0-or-later
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "engine/terrain/cdlod.h"
#include <doctest/doctest.h>
#include <glm/gtc/matrix_transform.hpp>
using namespace trackrs;
TEST_CASE(
    "tile hash LRU has stable hits, bounded ownership and precise eviction") {
  LruCache<TileKey, int, TileHash> cache(2);
  int calls = 0;
  auto make = [&] { return ++calls; };
  auto *first = &cache.get({-2, 4, 0}, make);
  cache.get({-2, 4, 1}, make);
  CHECK(&cache.get({-2, 4, 0}, make) == first);
  cache.get({8, -3, 0}, make);
  CHECK(calls == 3);
  CHECK(cache.size() == 2);
  CHECK(cache.find({-2, 4, 1}) == nullptr);
  CHECK(cache.find({-2, 4, 0}) == first);
  CHECK_THROWS(cache.get({7, 7, 1}, [] {
    throw std::runtime_error("load failed");
    return 0;
  }));
  CHECK(cache.size() == 2);
  cache.clear();
  CHECK(cache.size() == 0);
}
TEST_CASE("frustum rejects all six outside half spaces conservatively") {
  glm::mat4 identity(1);
  CHECK_FALSE(terrain::outside(identity, glm::vec3(-.5f), glm::vec3(.5f)));
  for (int axis = 0; axis < 3; ++axis)
    for (float sign : {-1.f, 1.f}) {
      glm::vec3 center(0);
      center[axis] = sign * 3;
      CHECK(terrain::outside(identity, center - glm::vec3(.2f),
                             center + glm::vec3(.2f)));
    }
}
TEST_CASE("CDLOD morph shares exact parent edges across levels and negative "
          "coordinates") {
  for (auto camera :
       {glm::vec3(7.5f, -19.2f, 80), glm::vec3(-511.7f, 257.3f, 40)}) {
    terrain::Selector selector;
    auto vp = glm::ortho(camera.x - 2048, camera.x + 2048, camera.y - 2048,
                         camera.y + 2048, -1000.f, 1000.f);
    auto selection = selector.select(camera, vp, 1, 1800, {-100, 200});
    REQUIRE(!selection.patches.empty());
    unsigned seams = 0;
    for (auto &a : selection.patches)
      for (auto &b : selection.patches) {
        if (a.key.level >= b.key.level)
          continue;
        bool vertical = a.key.x + int(a.cells) == b.key.x ||
                        b.key.x + int(b.cells) == a.key.x;
        bool horizontal = a.key.y + int(a.cells) == b.key.y ||
                          b.key.y + int(b.cells) == a.key.y;
        int overlapX =
            std::min(a.key.x + int(a.cells), b.key.x + int(b.cells)) -
            std::max(a.key.x, b.key.x);
        int overlapY =
            std::min(a.key.y + int(a.cells), b.key.y + int(b.cells)) -
            std::max(a.key.y, b.key.y);
        if (!((vertical && overlapY > 0) || (horizontal && overlapX > 0)))
          continue;
        REQUIRE(b.key.level == a.key.level + 1);
        ++seams;
        for (unsigned i = 0; i <= terrain::GridCells; ++i) {
          glm::vec2 grid = vertical ? glm::vec2(a.key.x < b.key.x ? 32 : 0, i)
                                    : glm::vec2(i, a.key.y < b.key.y ? 32 : 0);
          auto initial =
              glm::vec2(a.key.x, a.key.y) + grid * (float(a.cells) / 32.f);
          auto parentGrid = glm::floor((initial - glm::vec2(b.key.x, b.key.y)) /
                                           (float(b.cells) / 32.f) +
                                       .00001f);
          auto fine = terrain::morphPosition(a, grid, glm::vec2(camera), 1);
          auto coarse =
              terrain::morphPosition(b, parentGrid, glm::vec2(camera), 1);
          CHECK(glm::length(fine - coarse) < .001f);
        }
      }
    CHECK(seams > 0);
    auto again = selector.select(camera, vp, 1, 1800, {-100, 200});
    CHECK(again.patches.size() == selection.patches.size());
    CHECK(selector.residentTiles() <= 2048);
    CHECK(selection.patches.size() * 32 * 32 * 2 <= 3000000);
  }
}
