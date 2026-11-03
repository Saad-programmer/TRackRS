// SPDX-License-Identifier: GPL-2.0-or-later
#include "engine/assets/ibl_bake.h"
#include "engine/assets/ktx2.h"
#include "engine/assets/material_manifest.h"
#include <SDL2/SDL_image.h>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
using namespace trackrs::assets;
namespace {
struct Image {
  unsigned width, height;
  std::vector<Pixel> pixels;
};
Image image(const std::string &path) {
  std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)> source(
      IMG_Load(path.c_str()), SDL_FreeSurface),
      rgba(nullptr, SDL_FreeSurface);
  if (!source || source->w <= 0 || source->h <= 0 || source->w > 4096 ||
      source->h > 4096)
    throw std::runtime_error("invalid/oversized input image: " + path);
  rgba.reset(SDL_ConvertSurfaceFormat(source.get(), SDL_PIXELFORMAT_RGBA32, 0));
  if (!rgba)
    throw std::runtime_error(SDL_GetError());
  Image out{static_cast<unsigned>(rgba->w), static_cast<unsigned>(rgba->h), {}};
  out.pixels.resize(std::size_t(out.width) * out.height);
  for (unsigned y = 0; y < out.height; ++y)
    for (unsigned x = 0; x < out.width; ++x) {
      const auto *p = static_cast<unsigned char *>(rgba->pixels) +
                      std::size_t(y) * static_cast<std::size_t>(rgba->pitch) +
                      x * 4;
      out.pixels[std::size_t(y) * out.width + x] = {p[0] / 255.f, p[1] / 255.f,
                                                    p[2] / 255.f, p[3] / 255.f};
    }
  return out;
}
Semantic semantic(const std::string &s) {
  if (s == "albedo")
    return Semantic::Albedo;
  if (s == "normal")
    return Semantic::Normal;
  if (s == "orm")
    return Semantic::Orm;
  if (s == "scalar")
    return Semantic::Scalar;
  if (s == "environment")
    return Semantic::Environment;
  if (s == "brdf")
    return Semantic::Brdf;
  if (s == "any")
    return Semantic::Any;
  throw std::runtime_error("unknown semantic");
}
} // namespace
int main(int argc, char **argv) {
  try {
    std::map<std::string, std::string> options;
    bool atlas = false, white = false;
    for (int i = 1; i < argc; ++i) {
      std::string key = argv[i];
      if (key == "--atlas")
        atlas = true;
      else if (key == "--white")
        white = true;
      else if (key == "--help") {
        std::cout
            << "bake-material --input image --semantic "
               "albedo|normal|orm|scalar --output map.ktx2 [--ao image "
               "--roughness image --metallic image] "
               "[--atlas]\nmaterial-validator --material "
               "set.material.json\nmaterial-validator --input map.ktx2 "
               "--semantic "
               "albedo|normal|orm|scalar|environment|brdf [--atlas]\nibl-baker "
               "--output directory [--size 64 --samples 256 --white]\n";
        return 0;
      } else {
        if (i + 1 >= argc)
          throw std::runtime_error("missing option value");
        options[key] = argv[++i];
      }
    }
    std::string program = std::filesystem::path(argv[0]).stem().string();
    if (program == "ibl-baker") {
      if (!options.count("--output"))
        throw std::runtime_error("--output is required");
      unsigned size = options.count("--size")
                          ? static_cast<unsigned>(std::stoul(options["--size"]))
                          : 64,
               samples =
                   options.count("--samples")
                       ? static_cast<unsigned>(std::stoul(options["--samples"]))
                       : 256;
      auto cache =
          bakeIbl(white ? Environment([](glm::vec3) { return glm::vec3(1); })
                        : Environment(analyticSky),
                  size, samples);
      auto out = std::filesystem::path(options["--output"]);
      writeFile(out / "sky.ktx2", writeKtx2(cache.sky));
      writeFile(out / "irradiance.ktx2", writeKtx2(cache.irradiance));
      writeFile(out / "prefilter.ktx2", writeKtx2(cache.prefilter));
      writeFile(out / "brdf.ktx2", writeKtx2(cache.brdf));
      nlohmann::json metadata = {
          {"schema", "trackrs.ibl/1"},
          {"source", white ? "white furnace" : "original analytic sky"},
          {"size", size},
          {"samples", samples},
          {"irradiance_convention", "cosine integral divided by pi"},
          {"brdf_lut", "256x256 RG16F, split-sum"}};
      std::ofstream(out / "cache.json") << metadata.dump(2) << '\n';
      std::cout << metadata.dump() << '\n';
      return 0;
    }
    if (program == "material-validator" && options.count("--material")) {
      auto info = validateMaterial(options["--material"]);
      std::cout << nlohmann::json({{"valid", true},
                                   {"name", info.name},
                                   {"texture_bytes", info.textureBytes},
                                   {"mip_counts", info.mipCounts}})
                       .dump()
                << '\n';
      return 0;
    }
    if (!options.count("--input"))
      throw std::runtime_error("--input is required");
    auto kind =
        semantic(options.count("--semantic") ? options["--semantic"] : "any");
    if (program == "material-validator") {
      auto t = readKtx2(readFile(options["--input"]), kind, atlas);
      std::cout << nlohmann::json(
                       {{"valid", true},
                        {"width", t.width},
                        {"height", t.height},
                        {"faces", t.faces},
                        {"mips", t.levels.size()},
                        {"vkFormat", static_cast<unsigned>(t.format)},
                        {"srgb", isSrgb(t.format)},
                        {"texture_bytes", t.bytes()}})
                       .dump()
                << '\n';
      return 0;
    }
    if (!options.count("--output"))
      throw std::runtime_error("--output is required");
    auto input = image(options["--input"]);
    Format format = kind == Semantic::Albedo   ? Format::BC7Srgb
                    : kind == Semantic::Normal ? Format::BC5
                    : kind == Semantic::Scalar ? Format::BC4
                                               : Format::BC7;
    if (kind == Semantic::Any || kind == Semantic::Environment ||
        kind == Semantic::Brdf)
      throw std::runtime_error(
          "bake-material requires albedo, normal, orm or scalar semantic");
    if (kind == Semantic::Orm) {
      for (auto entry : {std::pair<const char *, unsigned>{"--ao", 0},
                         {"--roughness", 1},
                         {"--metallic", 2}})
        if (options.count(entry.first)) {
          auto channel = image(options[entry.first]);
          if (channel.width != input.width || channel.height != input.height)
            throw std::runtime_error("channel dimensions differ");
          for (std::size_t i = 0; i < input.pixels.size(); ++i)
            input.pixels[i][entry.second] = channel.pixels[i][0];
        }
    }
    auto texture = bake2D(format, input.width, input.height,
                          std::move(input.pixels), kind, atlas);
    writeFile(options["--output"], writeKtx2(texture));
    std::cout << "Baked " << options["--output"]
              << " mips=" << texture.levels.size()
              << " bytes=" << texture.bytes() << '\n';
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
