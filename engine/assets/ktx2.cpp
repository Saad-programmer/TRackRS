// SPDX-License-Identifier: GPL-2.0-or-later
#include "ktx2.h"
#include <algorithm>
#include <bcdec.h>
#include <cmath>
#include <cstring>
#include <fstream>
#include <glm/gtc/packing.hpp>
#include <limits>
#include <stdexcept>
namespace trackrs::assets {
namespace {
constexpr std::array<std::uint8_t, 12> magic = {
    0xab, 0x4b, 0x54, 0x58, 0x20, 0x32, 0x30, 0xbb, 0x0d, 0x0a, 0x1a, 0x0a};
void require(bool v, const char *text) {
  if (!v)
    throw std::runtime_error(text);
}
void put(Bytes &out, std::uint64_t value, unsigned count) {
  for (unsigned i = 0; i < count; ++i)
    out.push_back(static_cast<std::uint8_t>(value >> (8 * i)));
}
std::uint64_t get(const Bytes &b, std::size_t offset, unsigned count) {
  require(offset <= b.size() && count <= b.size() - offset,
          "truncated KTX2 field");
  std::uint64_t v = 0;
  for (unsigned i = 0; i < count; ++i)
    v |= std::uint64_t(b[offset + i]) << (8 * i);
  return v;
}
void set(Bytes &b, std::size_t offset, std::uint64_t value, unsigned count) {
  for (unsigned i = 0; i < count; ++i)
    b.at(offset + i) = static_cast<std::uint8_t>(value >> (8 * i));
}
unsigned blockBytes(Format f) {
  switch (f) {
  case Format::BC4:
    return 8;
  case Format::BC5:
  case Format::BC6H:
  case Format::BC7:
  case Format::BC7Srgb:
    return 16;
  case Format::RG16F:
    return 4;
  }
  throw std::runtime_error("unsupported KTX2 Vulkan format");
}
Bytes descriptor(Format f) {
  bool rg = f == Format::RG16F, two = rg || f == Format::BC5;
  unsigned samples = two ? 2 : 1;
  Bytes d;
  put(d, 28 + 16 * samples, 4);
  put(d, 0, 4);
  put(d, 2u | ((24u + 16 * samples) << 16), 4);
  unsigned model = rg                  ? 1
                   : f == Format::BC4  ? 131
                   : f == Format::BC5  ? 132
                   : f == Format::BC6H ? 133
                                       : 134;
  put(d, model | (1u << 8) | ((isSrgb(f) ? 2u : 1u) << 16), 4);
  put(d, rg ? 0 : 0x303, 4);
  put(d, blockBytes(f), 4);
  put(d, 0, 4);
  for (unsigned i = 0; i < samples; ++i) {
    unsigned bits = rg ? 16 : f == Format::BC4 || f == Format::BC5 ? 64 : 128;
    unsigned channel = i | (rg ? 0xc0u : f == Format::BC6H ? 0x80u : 0u);
    put(d, (i * (rg ? 16u : 64u)) | ((bits - 1) << 16) | (channel << 24), 4);
    put(d, 0, 4);
    put(d, rg ? 0xbf800000 : 0, 4);
    put(d, rg || f == Format::BC6H ? 0x3f800000 : 0xffffffff, 4);
  }
  return d;
}
float srgbToLinear(float v) {
  return v <= .04045f ? v / 12.92f : std::pow((v + .055f) / 1.055f, 2.4f);
}
float linearToSrgb(float v) {
  return v <= .0031308f ? v * 12.92f : 1.055f * std::pow(v, 1 / 2.4f) - .055f;
}
struct Bits {
  Bytes data = Bytes(16, 0);
  unsigned pos = 0;
  void add(unsigned v, unsigned n) {
    for (unsigned i = 0; i < n; ++i, ++pos) {
      require(pos < 128, "block encoder overflow");
      data[pos / 8] |= static_cast<std::uint8_t>(((v >> i) & 1u) << (pos % 8));
    }
  }
};
constexpr unsigned weights[16] = {0,  4,  9,  13, 17, 21, 26, 30,
                                  34, 38, 43, 47, 51, 55, 60, 64};
unsigned byte(float v) {
  return static_cast<unsigned>(std::lround(std::clamp(v, 0.f, 1.f) * 255));
}
Bytes bc4(const std::array<Pixel, 16> &pixels, unsigned channel) {
  unsigned hi = 0, lo = 255;
  for (const auto &p : pixels) {
    hi = std::max(hi, byte(p[channel]));
    lo = std::min(lo, byte(p[channel]));
  }
  float palette[8] = {float(hi), float(lo)};
  for (unsigned i = 2; i < 8; ++i)
    palette[i] = float((8 - i) * hi + (i - 1) * lo) / 7;
  std::uint64_t indices = 0;
  for (unsigned p = 0; p < 16; ++p) {
    unsigned best = 0;
    float error = 1e30f;
    for (unsigned i = 0; i < 8; ++i) {
      float e = std::abs(float(byte(pixels[p][channel])) - palette[i]);
      if (e < error) {
        best = i;
        error = e;
      }
    }
    indices |= std::uint64_t(best) << (3 * p);
  }
  Bytes out{static_cast<std::uint8_t>(hi), static_cast<std::uint8_t>(lo)};
  put(out, indices, 6);
  return out;
}
float bc6Value(unsigned q) {
  unsigned unq = q == 0 ? 0 : q == 1023 ? 65535 : (q << 6) + 32;
  return glm::unpackHalf1x16(static_cast<std::uint16_t>((unq * 31) >> 6));
}
unsigned bc6Quant(float value) {
  unsigned low = 0, high = 1023;
  while (low < high) {
    unsigned mid = (low + high) / 2;
    if (bc6Value(mid) < value)
      low = mid + 1;
    else
      high = mid;
  }
  if (low &&
      std::abs(bc6Value(low - 1) - value) < std::abs(bc6Value(low) - value))
    --low;
  return low;
}
Bytes colorBlock(Format format, const std::array<Pixel, 16> &p) {
  bool hdr = format == Format::BC6H;
  unsigned channels = hdr ? 3 : 4;
  unsigned ep[2][4]{};
  float palette[16][4]{};
  for (unsigned c = 0; c < channels; ++c) {
    float lo = std::numeric_limits<float>::max(), hi = 0;
    for (const auto &v : p) {
      lo = std::min(lo, v[c]);
      hi = std::max(hi, v[c]);
    }
    ep[0][c] = hdr ? bc6Quant(lo) : byte(lo);
    ep[1][c] = hdr ? bc6Quant(hi) : byte(hi);
  }
  if (!hdr) {
    for (unsigned e = 0; e < 2; ++e) {
      unsigned parity = 0;
      float error = 1e30f;
      for (unsigned bit = 0; bit < 2; ++bit) {
        float sum = 0;
        for (unsigned c = 0; c < 4; ++c) {
          unsigned quant = (ep[e][c] & 254u) | bit;
          float delta = float(quant) - float(ep[e][c]);
          sum += delta * delta;
        }
        if (sum < error) {
          parity = bit;
          error = sum;
        }
      }
      for (unsigned c = 0; c < 4; ++c)
        ep[e][c] = (ep[e][c] & 254u) | parity;
    }
  }
  auto makePalette = [&]() {
    for (unsigned i = 0; i < 16; ++i)
      for (unsigned c = 0; c < channels; ++c) {
        if (hdr) {
          auto unq = [](unsigned q) {
            return q == 0 ? 0u : q == 1023 ? 65535u : (q << 6) + 32;
          };
          unsigned q = (unq(ep[0][c]) * (64 - weights[i]) +
                        unq(ep[1][c]) * weights[i] + 32) >>
                       6;
          palette[i][c] =
              glm::unpackHalf1x16(static_cast<std::uint16_t>((q * 31) >> 6));
        } else
          palette[i][c] = float((ep[0][c] * (64 - weights[i]) +
                                 ep[1][c] * weights[i] + 32) >>
                                6) /
                          255;
      }
  };
  makePalette();
  unsigned indices[16]{};
  for (unsigned pixel = 0; pixel < 16; ++pixel) {
    float error = std::numeric_limits<float>::max();
    for (unsigned i = 0; i < 16; ++i) {
      float sum = 0;
      for (unsigned c = 0; c < channels; ++c) {
        float delta = p[pixel][c] - palette[i][c];
        sum += delta * delta;
      }
      if (sum < error) {
        error = sum;
        indices[pixel] = i;
      }
    }
  }
  if (indices[0] >= 8) {
    for (unsigned c = 0; c < channels; ++c)
      std::swap(ep[0][c], ep[1][c]);
    for (auto &i : indices)
      i = 15 - i;
  }
  Bits b;
  if (hdr) {
    b.add(3, 5);
    for (auto &endpoint : ep)
      for (unsigned c = 0; c < 3; ++c)
        b.add(endpoint[c], 10);
  } else {
    b.add(64, 7);
    for (unsigned c = 0; c < 4; ++c)
      for (auto &endpoint : ep)
        b.add(endpoint[c] >> 1, 7);
    b.add(ep[0][0] & 1, 1);
    b.add(ep[1][0] & 1, 1);
  }
  for (unsigned i = 0; i < 16; ++i)
    b.add(indices[i], i ? 4 : 3);
  require(b.pos == 128, "incorrect encoded block size");
  return b.data;
}
} // namespace
std::size_t TextureData::bytes() const {
  std::size_t total = 0;
  for (const auto &l : levels)
    total += l.size();
  return total;
}
unsigned mipCount(unsigned w, unsigned h) {
  unsigned n = 1;
  while (w > 1 || h > 1) {
    w = std::max(1u, w / 2);
    h = std::max(1u, h / 2);
    ++n;
  }
  return n;
}
bool isSrgb(Format f) { return f == Format::BC7Srgb; }
std::size_t faceBytes(Format f, unsigned w, unsigned h) {
  return f == Format::RG16F
             ? std::size_t(w) * h * 4
             : std::size_t((w + 3) / 4) * ((h + 3) / 4) * blockBytes(f);
}
bool isKtx2(const Bytes &b) {
  return b.size() >= 12 && std::equal(magic.begin(), magic.end(), b.begin());
}
void validate(const TextureData &t, Semantic s, bool atlas) {
  blockBytes(t.format);
  require(t.width && t.height && t.width <= 8192 && t.height <= 8192,
          "KTX2 dimensions out of bounds");
  require(t.faces == 1 || t.faces == 6, "KTX2 must be 2D or cubemap");
  require(t.faces != 6 || t.width == t.height, "cubemap faces must be square");
  require(t.levels.size() == mipCount(t.width, t.height),
          "KTX2 requires a complete mip chain");
  if (atlas || t.atlas)
    require((t.width & (t.width - 1)) == 0 && (t.height & (t.height - 1)) == 0,
            "atlas must have power-of-two dimensions");
  if (s == Semantic::Albedo)
    require(isSrgb(t.format), "albedo must use sRGB BC7");
  else if (s != Semantic::Any)
    require(!isSrgb(t.format), "data and HDR maps must be linear");
  if (s == Semantic::Normal)
    require(t.format == Format::BC5, "normal map requires BC5");
  if (s == Semantic::Orm)
    require(t.format == Format::BC7,
            "packed AO/Roughness/Metallic requires BC7 (BC4 is scalar)");
  if (s == Semantic::Environment)
    require(t.format == Format::BC6H && t.faces == 6,
            "environment requires BC6H cubemap");
  if (s == Semantic::Brdf)
    require(t.format == Format::RG16F && t.faces == 1 && t.width == 256 &&
                t.height == 256,
            "BRDF LUT requires 256x256 RG16F");
  unsigned w = t.width, h = t.height;
  for (const auto &l : t.levels) {
    require(l.size() == faceBytes(t.format, w, h) * t.faces,
            "KTX2 mip byte length mismatch");
    w = std::max(1u, w / 2);
    h = std::max(1u, h / 2);
  }
  require(t.bytes() <= 512 * 1024 * 1024, "KTX2 exceeds upload budget");
}
Bytes writeKtx2(const TextureData &t) {
  validate(t);
  Bytes b(magic.begin(), magic.end());
  for (auto v : {static_cast<unsigned>(t.format),
                 t.format == Format::RG16F ? 2u : 1u, t.width, t.height, 0u, 0u,
                 t.faces, static_cast<unsigned>(t.levels.size()), 0u})
    put(b, v, 4);
  auto d = descriptor(t.format);
  unsigned dOffset = 80 + static_cast<unsigned>(t.levels.size()) * 24;
  put(b, dOffset, 4);
  put(b, d.size(), 4);
  unsigned kOffset = dOffset + static_cast<unsigned>(d.size());
  Bytes k;
  std::string entry = "KTXorientation";
  entry.push_back(0);
  entry += "rd";
  entry.push_back(0);
  put(k, entry.size(), 4);
  k.insert(k.end(), entry.begin(), entry.end());
  while (k.size() % 4)
    k.push_back(0);
  std::string writer = "KTXwriter";
  writer.push_back(0);
  writer += "TrackRS material tools 0.7";
  writer.push_back(0);
  put(k, writer.size(), 4);
  k.insert(k.end(), writer.begin(), writer.end());
  while (k.size() % 4)
    k.push_back(0);
  if (t.atlas) {
    std::string e = "trackrs.atlas";
    e.push_back(0);
    e += "1";
    put(k, e.size(), 4);
    k.insert(k.end(), e.begin(), e.end());
    while (k.size() % 4)
      k.push_back(0);
  }
  put(b, kOffset, 4);
  put(b, k.size(), 4);
  put(b, 0, 8);
  put(b, 0, 8);
  b.resize(dOffset);
  b.insert(b.end(), d.begin(), d.end());
  b.insert(b.end(), k.begin(), k.end());
  for (std::size_t level = t.levels.size(); level-- > 0;) {
    while (b.size() % blockBytes(t.format))
      b.push_back(0);
    set(b, 80 + level * 24, b.size(), 8);
    set(b, 88 + level * 24, t.levels[level].size(), 8);
    set(b, 96 + level * 24, t.levels[level].size(), 8);
    b.insert(b.end(), t.levels[level].begin(), t.levels[level].end());
  }
  return b;
}
TextureData readKtx2(const Bytes &b, Semantic s, bool atlas) {
  require(isKtx2(b) && b.size() >= 80 && b.size() <= 512 * 1024 * 1024,
          "invalid KTX2 header/budget");
  TextureData t;
  t.format = static_cast<Format>(get(b, 12, 4));
  blockBytes(t.format);
  require(get(b, 16, 4) == (t.format == Format::RG16F ? 2 : 1),
          "KTX2 typeSize mismatch");
  t.width = static_cast<unsigned>(get(b, 20, 4));
  t.height = static_cast<unsigned>(get(b, 24, 4));
  require(get(b, 28, 4) == 0 && get(b, 32, 4) == 0 && get(b, 44, 4) == 0 &&
              get(b, 64, 8) == 0 && get(b, 72, 8) == 0,
          "KTX2 arrays/volumes/supercompression unsupported");
  t.faces = static_cast<unsigned>(get(b, 36, 4));
  auto count = get(b, 40, 4);
  require(count && count <= 14 && 80 + count * 24 <= b.size(),
          "invalid KTX2 mip index");
  auto off = get(b, 48, 4), size = get(b, 52, 4);
  auto d = descriptor(t.format);
  require(size == d.size() && off >= 80 + count * 24 && off <= b.size() &&
              size <= b.size() - off,
          "invalid KTX2 DFD range");
  require(std::equal(d.begin(), d.end(),
                     b.begin() + static_cast<std::ptrdiff_t>(off)),
          "KTX2 DFD/color-space flags disagree with Vulkan format");
  auto ko = get(b, 56, 4), kl = get(b, 60, 4);
  std::uint64_t metadataEnd = off + size;
  if (kl) {
    require(ko >= metadataEnd && ko <= b.size() && kl <= b.size() - ko,
            "invalid KTX2 metadata range");
    metadataEnd = ko + kl;
    for (auto pos = ko; pos < ko + kl;) {
      auto n = get(b, static_cast<std::size_t>(pos), 4);
      pos += 4;
      require(n && n <= ko + kl - pos, "invalid KTX2 metadata entry");
      std::string entry(b.begin() + static_cast<std::ptrdiff_t>(pos),
                        b.begin() + static_cast<std::ptrdiff_t>(pos + n));
      if (entry == std::string("trackrs.atlas\0"
                               "1",
                               15))
        t.atlas = true;
      pos += n;
      pos = (pos + 3) & ~std::uint64_t(3);
      require(pos <= ko + kl, "invalid KTX2 metadata padding");
    }
  }
  std::vector<std::pair<std::uint64_t, std::uint64_t>> ranges;
  for (std::size_t l = 0; l < count; ++l) {
    auto begin = get(b, 80 + l * 24, 8), length = get(b, 88 + l * 24, 8);
    require(begin >= metadataEnd && begin % blockBytes(t.format) == 0 &&
                begin <= b.size() && length <= b.size() - begin &&
                get(b, 96 + l * 24, 8) == length,
            "invalid KTX2 mip range");
    ranges.emplace_back(begin, begin + length);
    t.levels.emplace_back(b.begin() + static_cast<std::ptrdiff_t>(begin),
                          b.begin() +
                              static_cast<std::ptrdiff_t>(begin + length));
  }
  std::sort(ranges.begin(), ranges.end());
  for (std::size_t i = 1; i < ranges.size(); ++i)
    require(ranges[i].first >= ranges[i - 1].second,
            "overlapping KTX2 mip ranges");
  validate(t, s, atlas);
  return t;
}
Bytes readFile(const std::filesystem::path &path) {
  std::ifstream f(path, std::ios::binary | std::ios::ate);
  require(bool(f), "cannot open texture file");
  auto length = f.tellg();
  require(length >= 0 && length <= 512 * 1024 * 1024, "texture file too large");
  Bytes b(static_cast<std::size_t>(length));
  f.seekg(0);
  require(bool(f.read(reinterpret_cast<char *>(b.data()), length)),
          "short texture read");
  return b;
}
void writeFile(const std::filesystem::path &path, const Bytes &bytes) {
  if (!path.parent_path().empty())
    std::filesystem::create_directories(path.parent_path());
  std::ofstream f(path, std::ios::binary);
  require(bool(f.write(reinterpret_cast<const char *>(bytes.data()),
                       static_cast<std::streamsize>(bytes.size()))),
          "texture write failed");
}
Bytes encode(Format f, unsigned w, unsigned h,
             const std::vector<Pixel> &pixels) {
  require(w && h && pixels.size() == std::size_t(w) * h,
          "invalid encoder dimensions");
  for (const auto &p : pixels)
    for (auto v : p)
      require(std::isfinite(v) && v >= 0 && v <= 65504,
              "invalid texture sample");
  Bytes out;
  if (f == Format::RG16F) {
    for (const auto &p : pixels)
      for (unsigned c = 0; c < 2; ++c)
        put(out, glm::packHalf1x16(p[c]), 2);
    return out;
  }
  for (unsigned y = 0; y < h; y += 4)
    for (unsigned x = 0; x < w; x += 4) {
      std::array<Pixel, 16> block;
      for (unsigned row = 0; row < 4; ++row)
        for (unsigned col = 0; col < 4; ++col)
          block[row * 4 + col] =
              pixels[std::size_t(std::min(y + row, h - 1)) * w +
                     std::min(x + col, w - 1)];
      Bytes data;
      if (f == Format::BC4 || f == Format::BC5) {
        data = bc4(block, 0);
        if (f == Format::BC5) {
          auto green = bc4(block, 1);
          data.insert(data.end(), green.begin(), green.end());
        }
      } else
        data = colorBlock(f, block);
      out.insert(out.end(), data.begin(), data.end());
    }
  return out;
}
std::vector<Pixel> decode(Format f, unsigned w, unsigned h,
                          const Bytes &bytes) {
  require(bytes.size() == faceBytes(f, w, h), "invalid block decoder size");
  std::vector<Pixel> out(std::size_t(w) * h);
  if (f == Format::RG16F) {
    for (std::size_t i = 0; i < out.size(); ++i)
      out[i] = {
          glm::unpackHalf1x16(static_cast<std::uint16_t>(get(bytes, i * 4, 2))),
          glm::unpackHalf1x16(
              static_cast<std::uint16_t>(get(bytes, i * 4 + 2, 2))),
          0, 1};
    return out;
  }
  std::size_t offset = 0;
  for (unsigned y = 0; y < h; y += 4)
    for (unsigned x = 0; x < w; x += 4) {
      alignas(16) std::array<std::uint8_t, 16> block{};
      std::copy_n(bytes.data() + offset, blockBytes(f), block.data());
      offset += blockBytes(f);
      unsigned char rgba[64]{};
      float hdr[48]{};
      if (f == Format::BC6H)
        bcdec_bc6h_float(block.data(), hdr, 12, 0);
      else if (f == Format::BC4)
        bcdec_bc4(block.data(), rgba, 4);
      else if (f == Format::BC5)
        bcdec_bc5(block.data(), rgba, 8);
      else
        bcdec_bc7(block.data(), rgba, 16);
      for (unsigned row = 0; row < 4 && y + row < h; ++row)
        for (unsigned col = 0; col < 4 && x + col < w; ++col) {
          auto &p = out[std::size_t(y + row) * w + x + col];
          unsigned i = row * 4 + col;
          if (f == Format::BC6H)
            p = {hdr[i * 3], hdr[i * 3 + 1], hdr[i * 3 + 2], 1};
          else if (f == Format::BC4)
            p = {rgba[i] / 255.f, 0, 0, 1};
          else if (f == Format::BC5)
            p = {rgba[i * 2] / 255.f, rgba[i * 2 + 1] / 255.f, 0, 1};
          else
            for (unsigned c = 0; c < 4; ++c)
              p[c] = rgba[i * 4 + c] / 255.f;
        }
    }
  return out;
}
std::vector<Pixel> downsample(unsigned w, unsigned h,
                              const std::vector<Pixel> &p, Semantic semantic) {
  unsigned nw = std::max(1u, w / 2), nh = std::max(1u, h / 2);
  std::vector<Pixel> out(std::size_t(nw) * nh);
  for (unsigned y = 0; y < nh; ++y)
    for (unsigned x = 0; x < nw; ++x) {
      Pixel sum{};
      for (unsigned dy = 0; dy < 2; ++dy)
        for (unsigned dx = 0; dx < 2; ++dx) {
          auto v = p[std::size_t(std::min(h - 1, y * 2 + dy)) * w +
                     std::min(w - 1, x * 2 + dx)];
          if (semantic == Semantic::Albedo)
            for (unsigned c = 0; c < 3; ++c)
              v[c] = srgbToLinear(v[c]);
          if (semantic == Semantic::Normal) {
            float nx = v[0] * 2 - 1, ny = v[1] * 2 - 1;
            v = {nx, ny, std::sqrt(std::max(0.f, 1 - nx * nx - ny * ny)), 1};
          }
          if (semantic == Semantic::Orm)
            v[1] *= v[1];
          for (unsigned c = 0; c < 4; ++c)
            sum[c] += v[c] * .25f;
        }
      if (semantic == Semantic::Albedo)
        for (unsigned c = 0; c < 3; ++c)
          sum[c] = linearToSrgb(sum[c]);
      if (semantic == Semantic::Normal) {
        float len =
            std::sqrt(sum[0] * sum[0] + sum[1] * sum[1] + sum[2] * sum[2]);
        if (len > 1e-8f)
          for (unsigned c = 0; c < 3; ++c)
            sum[c] = sum[c] / len * .5f + .5f;
        else
          sum = {.5f, .5f, 1, 1};
      }
      if (semantic == Semantic::Orm)
        sum[1] = std::sqrt(sum[1]);
      out[std::size_t(y) * nw + x] = sum;
    }
  return out;
}
TextureData bake2D(Format f, unsigned w, unsigned h, std::vector<Pixel> p,
                   Semantic s, bool atlas) {
  TextureData out{f, w, h, 1, atlas, {}};
  for (;;) {
    out.levels.push_back(encode(f, w, h, p));
    if (w == 1 && h == 1)
      break;
    p = downsample(w, h, p, s);
    w = std::max(1u, w / 2);
    h = std::max(1u, h / 2);
  }
  validate(out, s, atlas);
  return out;
}
} // namespace trackrs::assets
