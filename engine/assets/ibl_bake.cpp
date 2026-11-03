// SPDX-License-Identifier: GPL-2.0-or-later
#include "ibl_bake.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <stdexcept>
namespace trackrs::assets {
namespace {
float radical(unsigned b) {
  b = (b << 16) | (b >> 16);
  b = ((b & 0x55555555u) << 1) | ((b & 0xaaaaaaaau) >> 1);
  b = ((b & 0x33333333u) << 2) | ((b & 0xccccccccu) >> 2);
  b = ((b & 0x0f0f0f0fu) << 4) | ((b & 0xf0f0f0f0u) >> 4);
  b = ((b & 0x00ff00ffu) << 8) | ((b & 0xff00ff00u) >> 8);
  return static_cast<float>(b) * 2.3283064365386963e-10f;
}
glm::vec3 basis(glm::vec3 n, glm::vec3 h) {
  auto t = glm::normalize(glm::cross(
      std::abs(n.z) < .999f ? glm::vec3(0, 0, 1) : glm::vec3(1, 0, 0), n));
  return glm::normalize(t * h.x + glm::cross(n, t) * h.y + n * h.z);
}
glm::vec3 ggx(float x, float y, float rough) {
  float a = rough * rough;
  float ct = std::sqrt((1 - y) / (1 + (a * a - 1) * y));
  float st = std::sqrt(std::max(0.f, 1 - ct * ct)),
        phi = glm::two_pi<float>() * x;
  return {std::cos(phi) * st, std::sin(phi) * st, ct};
}
float geometry(float n, float k) { return n / (n * (1 - k) + k); }
} // namespace
glm::vec3 cubeDirection(unsigned f, float u, float v) {
  glm::vec3 d;
  switch (f) {
  case 0:
    d = {1, -v, -u};
    break;
  case 1:
    d = {-1, -v, u};
    break;
  case 2:
    d = {u, 1, v};
    break;
  case 3:
    d = {u, -1, -v};
    break;
  case 4:
    d = {u, -v, 1};
    break;
  case 5:
    d = {-u, -v, -1};
    break;
  default:
    throw std::invalid_argument("cube face");
  }
  return glm::normalize(d);
}
glm::vec3 analyticSky(glm::vec3 n) {
  auto sun = glm::normalize(glm::vec3(-.4f, .8f, .2f));
  float elevation = std::clamp(n.y * .5f + .5f, 0.f, 1.f);
  glm::vec3 color = glm::mix(glm::vec3(.35f, .3f, .22f),
                             glm::vec3(.08f, .22f, .55f), elevation);
  color +=
      glm::vec3(7, 6, 4) * std::pow(std::max(glm::dot(n, sun), 0.f), 256.f);
  color += glm::vec3(.3f) * std::pow(std::max(0.f, 1 - std::abs(n.y)), 8.f);
  return color;
}
IblCache bakeIbl(const Environment &env, unsigned size, unsigned samples) {
  if (size < 4 || size > 256 || (size & (size - 1)) || samples < 32 ||
      samples > 4096)
    throw std::invalid_argument("invalid IBL bake quality");
  IblCache cache;
  cache.sky = {Format::BC6H, size, size, 6, false, {}};
  cache.prefilter = cache.sky;
  unsigned irradianceSize = std::min(16u, size);
  cache.irradiance = {
      Format::BC6H, irradianceSize, irradianceSize, 6, false, {}};
  for (unsigned l = 0, w = size; l < mipCount(size, size);
       ++l, w = std::max(1u, w / 2)) {
    Bytes sky, spec;
    float rough =
        std::max(.04f, static_cast<float>(l) /
                           static_cast<float>(mipCount(size, size) - 1));
    for (unsigned f = 0; f < 6; ++f) {
      std::vector<Pixel> sp(std::size_t(w) * w), raw(sp.size());
      for (unsigned y = 0; y < w; ++y)
        for (unsigned x = 0; x < w; ++x) {
          auto n = cubeDirection(f, (float(x) + .5f) * 2 / float(w) - 1,
                                 (float(y) + .5f) * 2 / float(w) - 1);
          auto value = env(n);
          raw[std::size_t(y) * w + x] = {value.x, value.y, value.z, 1};
          glm::vec3 sum(0);
          float total = 0;
          if (l == 0) {
            sum = value;
            total = 1;
          } else
            for (unsigned i = 0; i < samples; ++i) {
              auto h = basis(
                  n, ggx((float(i) + .5f) / float(samples), radical(i), rough));
              auto light = glm::normalize(glm::reflect(-n, h));
              float nl = std::max(glm::dot(n, light), 0.f);
              if (nl > 0) {
                sum += env(light) * nl;
                total += nl;
              }
            }
          sum /= std::max(total, 1e-8f);
          sp[std::size_t(y) * w + x] = {sum.x, sum.y, sum.z, 1};
        }
      auto bytes = encode(Format::BC6H, w, w, sp);
      spec.insert(spec.end(), bytes.begin(), bytes.end());
      bytes = encode(Format::BC6H, w, w, raw);
      sky.insert(sky.end(), bytes.begin(), bytes.end());
    }
    cache.sky.levels.push_back(std::move(sky));
    cache.prefilter.levels.push_back(std::move(spec));
  }
  std::vector<std::vector<Pixel>> diffuse(6);
  for (unsigned f = 0; f < 6; ++f) {
    auto &pixels = diffuse[f];
    pixels.resize(std::size_t(irradianceSize) * irradianceSize);
    for (unsigned y = 0; y < irradianceSize; ++y)
      for (unsigned x = 0; x < irradianceSize; ++x) {
        auto n =
            cubeDirection(f, (float(x) + .5f) * 2 / float(irradianceSize) - 1,
                          (float(y) + .5f) * 2 / float(irradianceSize) - 1);
        glm::vec3 sum(0);
        for (unsigned i = 0; i < samples; ++i) {
          float u = (float(i) + .5f) / float(samples), v = radical(i),
                phi = glm::two_pi<float>() * u;
          auto light = basis(n, glm::vec3(std::sqrt(v) * std::cos(phi),
                                          std::sqrt(v) * std::sin(phi),
                                          std::sqrt(1 - v)));
          sum += env(light);
        }
        sum /= float(samples);
        pixels[std::size_t(y) * irradianceSize + x] = {sum.x, sum.y, sum.z, 1};
      }
  }
  for (unsigned w = irradianceSize;; w = std::max(1u, w / 2)) {
    Bytes level;
    for (auto &pixels : diffuse) {
      auto b = encode(Format::BC6H, w, w, pixels);
      level.insert(level.end(), b.begin(), b.end());
      if (w > 1)
        pixels = downsample(w, w, pixels, Semantic::Environment);
    }
    cache.irradiance.levels.push_back(std::move(level));
    if (w == 1)
      break;
  }
  std::vector<Pixel> lut(256 * 256);
  for (unsigned y = 0; y < 256; ++y)
    for (unsigned x = 0; x < 256; ++x) {
      float nv = (float(x) + .5f) / 256, rough = (float(y) + .5f) / 256;
      glm::vec3 v(std::sqrt(1 - nv * nv), 0, nv);
      float a = 0, b = 0;
      for (unsigned i = 0; i < samples; ++i) {
        auto h = ggx((float(i) + .5f) / float(samples), radical(i), rough);
        auto light = glm::normalize(glm::reflect(-v, h));
        float nl = std::max(light.z, 0.f), vh = std::max(glm::dot(v, h), 0.f);
        if (nl > 0) {
          float k = rough * rough * .5f;
          float g = geometry(nv, k) * geometry(nl, k);
          float weight = g * vh / std::max(h.z * nv, 1e-7f);
          float fc = std::pow(1 - vh, 5.f);
          a += (1 - fc) * weight;
          b += fc * weight;
        }
      }
      a /= float(samples);
      b /= float(samples);
      float scale = std::max(1.f, a + b);
      lut[std::size_t(y) * 256 + x] = {a / scale, b / scale, 0, 1};
    }
  cache.brdf = bake2D(Format::RG16F, 256, 256, std::move(lut), Semantic::Brdf);
  validate(cache.sky, Semantic::Environment);
  validate(cache.irradiance, Semantic::Environment);
  validate(cache.prefilter, Semantic::Environment);
  return cache;
}
} // namespace trackrs::assets
