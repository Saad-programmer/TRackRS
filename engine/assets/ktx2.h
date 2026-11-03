// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
namespace trackrs::assets {
using Bytes = std::vector<std::uint8_t>;
using Pixel = std::array<float, 4>;
enum class Format : std::uint32_t {
  BC4 = 139,
  BC5 = 141,
  BC6H = 143,
  BC7 = 145,
  BC7Srgb = 146,
  RG16F = 83
};
enum class Semantic : std::uint8_t {
  Any,
  Albedo,
  Normal,
  Orm,
  Scalar,
  Environment,
  Brdf
};
struct TextureData {
  Format format = Format::BC7;
  unsigned width = 0, height = 0, faces = 1;
  bool atlas = false;
  std::vector<Bytes> levels;
  std::size_t bytes() const;
};
unsigned mipCount(unsigned width, unsigned height);
std::size_t faceBytes(Format format, unsigned width, unsigned height);
bool isSrgb(Format format);
bool isKtx2(const Bytes &bytes);
void validate(const TextureData &texture, Semantic semantic = Semantic::Any,
              bool atlas = false);
TextureData readKtx2(const Bytes &bytes, Semantic semantic = Semantic::Any,
                     bool atlas = false);
Bytes writeKtx2(const TextureData &texture);
Bytes readFile(const std::filesystem::path &file);
void writeFile(const std::filesystem::path &file, const Bytes &bytes);
// Deterministic portable block encoders: BC7 mode 6, BC6H mode 11, BC4/5 UNORM.
// The independent pinned bcdec decoder accepts every standard mode on fallback.
Bytes encode(Format format, unsigned width, unsigned height,
             const std::vector<Pixel> &pixels);
std::vector<Pixel> decode(Format format, unsigned width, unsigned height,
                          const Bytes &bytes);
TextureData bake2D(Format format, unsigned width, unsigned height,
                   std::vector<Pixel> pixels, Semantic semantic,
                   bool atlas = false);
std::vector<Pixel> downsample(unsigned width, unsigned height,
                              const std::vector<Pixel> &pixels,
                              Semantic semantic);
} // namespace trackrs::assets
