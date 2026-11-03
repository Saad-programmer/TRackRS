// SPDX-License-Identifier: GPL-2.0-or-later
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "engine/assets/ktx2.h"
#include <cmath>
#include <doctest/doctest.h>
using namespace trackrs::assets;
TEST_CASE(
    "standard BC blocks decode independently and preserve typed samples") {
  for (auto format : {Format::BC4, Format::BC5, Format::BC6H, Format::BC7,
                      Format::BC7Srgb, Format::RG16F}) {
    std::vector<Pixel> pixels(16, Pixel{.24f, .51f, .78f, 1});
    if (format == Format::BC6H)
      for (auto &p : pixels)
        p = {1, 4, 32, 1};
    auto bytes = encode(format, 4, 4, pixels);
    auto result = decode(format, 4, 4, bytes);
    unsigned channels = format == Format::BC4                              ? 1
                        : format == Format::BC5 || format == Format::RG16F ? 2
                                                                           : 3;
    for (auto p : result)
      for (unsigned c = 0; c < channels; ++c)
        CHECK(p[c] == doctest::Approx(pixels[0][c]).epsilon(.015));
  }
}
TEST_CASE("complete KTX2 mip chains round trip and validate semantic formats") {
  for (auto format : {Format::BC4, Format::BC5, Format::BC6H, Format::BC7,
                      Format::BC7Srgb, Format::RG16F}) {
    auto t =
        bake2D(format, 16, 8, std::vector<Pixel>(128, Pixel{.2f, .4f, .6f, 1}),
               Semantic::Any);
    auto file = writeKtx2(t);
    auto read = readKtx2(file);
    CHECK(read.width == 16);
    CHECK(read.height == 8);
    CHECK(read.levels == t.levels);
    CHECK(read.format == format);
    CHECK(read.levels.size() == 5);
    auto bad = t;
    bad.levels.pop_back();
    CHECK_THROWS(writeKtx2(bad));
    file[48] = 255;
    CHECK_THROWS(readKtx2(file));
  }
  auto albedo =
      bake2D(Format::BC7Srgb, 8, 8,
             std::vector<Pixel>(64, Pixel{.2f, .4f, .6f, 1}), Semantic::Albedo);
  CHECK_NOTHROW(readKtx2(writeKtx2(albedo), Semantic::Albedo));
  CHECK_THROWS(readKtx2(writeKtx2(albedo), Semantic::Orm));
  albedo.format = Format::BC7;
  CHECK_THROWS(validate(albedo, Semantic::Albedo));
  auto atlas =
      bake2D(Format::BC7, 8, 8, std::vector<Pixel>(64, Pixel{1, 1, 1, 1}),
             Semantic::Orm, true);
  CHECK(readKtx2(writeKtx2(atlas)).atlas);
  CHECK_THROWS(bake2D(Format::BC7, 7, 8,
                      std::vector<Pixel>(56, Pixel{1, 1, 1, 1}), Semantic::Orm,
                      true));
}
TEST_CASE(
    "mips filter albedo in linear light and renormalize tangent normals") {
  auto pixels = std::vector<Pixel>{
      {0, 0, 0, 1}, {1, 1, 1, 1}, {0, 0, 0, 1}, {1, 1, 1, 1}};
  auto mip = downsample(2, 2, pixels, Semantic::Albedo);
  CHECK(mip[0][0] == doctest::Approx(.73535698));
  pixels.assign(4, Pixel{.7f, .6f, .94f, 1});
  mip = downsample(2, 2, pixels, Semantic::Normal);
  float x = mip[0][0] * 2 - 1, y = mip[0][1] * 2 - 1, z = mip[0][2] * 2 - 1;
  CHECK(x * x + y * y + z * z == doctest::Approx(1));
}
TEST_CASE("KTX2 rejects malformed index, missing mips and wrong DFD transfer") {
  auto t = bake2D(Format::BC7Srgb, 8, 8,
                  std::vector<Pixel>(64, Pixel{1, 1, 1, 1}), Semantic::Albedo);
  auto bytes = writeKtx2(t);
  auto d = 80 + t.levels.size() * 24;
  bytes[d + 14] = 1;
  CHECK_THROWS(readKtx2(bytes));
  bytes = writeKtx2(t);
  bytes[40] = 1;
  CHECK_THROWS(readKtx2(bytes));
  bytes = writeKtx2(t);
  for (int i = 0; i < 8; ++i)
    bytes[80 + i] = 255;
  CHECK_THROWS(readKtx2(bytes));
}
