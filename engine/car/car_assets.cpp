// SPDX-License-Identifier: GPL-2.0-or-later
#include "car_assets.h"
#include "engine/core/pcg32.h"
#include "engine/mesh/vertex_layout.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <meshoptimizer.h>
#include <nlohmann/json.hpp>
#include <stdexcept>
namespace trackrs::car {
namespace {
constexpr float Pi = 3.14159265358979f;
float channel(std::uint8_t v) { return static_cast<float>(v) / 255.f; }
glm::vec3 colourAt(const Image &im, int x, int y) {
  const auto *p = im.at(x, y);
  return {channel(p[0]), channel(p[1]), channel(p[2])};
}
float luma(glm::vec3 c) { return .2126f * c.r + .7152f * c.g + .0722f * c.b; }
float saturation(glm::vec3 c) {
  const float hi = std::max({c.r, c.g, c.b}), lo = std::min({c.r, c.g, c.b});
  return hi > 1e-4f ? (hi - lo) / hi : 0.f;
}
std::uint8_t byte(float v) { return static_cast<std::uint8_t>(std::lround(std::clamp(v, 0.f, 1.f) * 255.f)); }
void blend(Image &im, int x, int y, glm::vec3 c, float a) {
  if (x < 0 || y < 0 || x >= im.width || y >= im.height || a <= 0)
    return;
  auto *p = im.at(x, y);
  for (int k = 0; k < 3; ++k)
    p[k] = byte(channel(p[k]) * (1.f - a) + c[k] * a);
}

// ---- original stroke font ---------------------------------------------------
// Glyphs on a unit box (x right, y down) as line segments; drawn with round
// caps. Authored for TrackRS; covers 0-9 and A-Z.
using Segment = std::array<float, 4>;
const std::map<char, std::vector<Segment>> &font() {
  static const std::map<char, std::vector<Segment>> glyphs = {
      {'0', {{0, 0, 1, 0}, {1, 0, 1, 1}, {1, 1, 0, 1}, {0, 1, 0, 0}, {0, 1, 1, 0}}},
      {'1', {{.3f, .2f, .6f, 0}, {.6f, 0, .6f, 1}, {.25f, 1, .95f, 1}}},
      {'2', {{0, 0, 1, 0}, {1, 0, 1, .5f}, {1, .5f, 0, .5f}, {0, .5f, 0, 1}, {0, 1, 1, 1}}},
      {'3', {{0, 0, 1, 0}, {1, 0, 1, 1}, {1, 1, 0, 1}, {.2f, .5f, 1, .5f}}},
      {'4', {{0, 0, 0, .55f}, {0, .55f, 1, .55f}, {.75f, 0, .75f, 1}}},
      {'5', {{1, 0, 0, 0}, {0, 0, 0, .5f}, {0, .5f, 1, .5f}, {1, .5f, 1, 1}, {1, 1, 0, 1}}},
      {'6', {{1, 0, 0, 0}, {0, 0, 0, 1}, {0, 1, 1, 1}, {1, 1, 1, .5f}, {1, .5f, 0, .5f}}},
      {'7', {{0, 0, 1, 0}, {1, 0, .35f, 1}}},
      {'8', {{0, 0, 1, 0}, {1, 0, 1, 1}, {1, 1, 0, 1}, {0, 1, 0, 0}, {0, .5f, 1, .5f}}},
      {'9', {{1, .5f, 0, .5f}, {0, .5f, 0, 0}, {0, 0, 1, 0}, {1, 0, 1, 1}, {1, 1, 0, 1}}},
      {'A', {{0, 1, .5f, 0}, {.5f, 0, 1, 1}, {.25f, .55f, .75f, .55f}}},
      {'B', {{0, 0, 0, 1}, {0, 0, .8f, 0}, {.8f, 0, .8f, .5f}, {0, .5f, 1, .5f}, {1, .5f, 1, 1}, {1, 1, 0, 1}}},
      {'C', {{1, 0, 0, 0}, {0, 0, 0, 1}, {0, 1, 1, 1}}},
      {'D', {{0, 0, 0, 1}, {0, 0, .7f, 0}, {.7f, 0, 1, .3f}, {1, .3f, 1, .7f}, {1, .7f, .7f, 1}, {.7f, 1, 0, 1}}},
      {'E', {{1, 0, 0, 0}, {0, 0, 0, 1}, {0, 1, 1, 1}, {0, .5f, .7f, .5f}}},
      {'F', {{1, 0, 0, 0}, {0, 0, 0, 1}, {0, .5f, .7f, .5f}}},
      {'G', {{1, 0, 0, 0}, {0, 0, 0, 1}, {0, 1, 1, 1}, {1, 1, 1, .55f}, {1, .55f, .5f, .55f}}},
      {'H', {{0, 0, 0, 1}, {1, 0, 1, 1}, {0, .5f, 1, .5f}}},
      {'I', {{.5f, 0, .5f, 1}, {.2f, 0, .8f, 0}, {.2f, 1, .8f, 1}}},
      {'J', {{1, 0, 1, 1}, {1, 1, 0, 1}, {0, 1, 0, .7f}}},
      {'K', {{0, 0, 0, 1}, {1, 0, 0, .55f}, {.3f, .4f, 1, 1}}},
      {'L', {{0, 0, 0, 1}, {0, 1, 1, 1}}},
      {'M', {{0, 1, 0, 0}, {0, 0, .5f, .55f}, {.5f, .55f, 1, 0}, {1, 0, 1, 1}}},
      {'N', {{0, 1, 0, 0}, {0, 0, 1, 1}, {1, 1, 1, 0}}},
      {'O', {{0, 0, 1, 0}, {1, 0, 1, 1}, {1, 1, 0, 1}, {0, 1, 0, 0}}},
      {'P', {{0, 1, 0, 0}, {0, 0, 1, 0}, {1, 0, 1, .5f}, {1, .5f, 0, .5f}}},
      {'Q', {{0, 0, 1, 0}, {1, 0, 1, 1}, {1, 1, 0, 1}, {0, 1, 0, 0}, {.6f, .6f, 1.1f, 1.1f}}},
      {'R', {{0, 1, 0, 0}, {0, 0, 1, 0}, {1, 0, 1, .5f}, {1, .5f, 0, .5f}, {.4f, .5f, 1, 1}}},
      {'S', {{1, 0, 0, 0}, {0, 0, 0, .5f}, {0, .5f, 1, .5f}, {1, .5f, 1, 1}, {1, 1, 0, 1}}},
      {'T', {{0, 0, 1, 0}, {.5f, 0, .5f, 1}}},
      {'U', {{0, 0, 0, 1}, {0, 1, 1, 1}, {1, 1, 1, 0}}},
      {'V', {{0, 0, .5f, 1}, {.5f, 1, 1, 0}}},
      {'W', {{0, 0, .25f, 1}, {.25f, 1, .5f, .45f}, {.5f, .45f, .75f, 1}, {.75f, 1, 1, 0}}},
      {'X', {{0, 0, 1, 1}, {1, 0, 0, 1}}},
      {'Y', {{0, 0, .5f, .5f}, {1, 0, .5f, .5f}, {.5f, .5f, .5f, 1}}},
      {'Z', {{0, 0, 1, 0}, {1, 0, 0, 1}, {0, 1, 1, 1}}},
  };
  return glyphs;
}
float segmentDistance(glm::vec2 p, const Segment &s) {
  const glm::vec2 a(s[0], s[1]), b(s[2], s[3]), ab = b - a;
  const float l2 = glm::dot(ab, ab);
  const float t = l2 > 0 ? std::clamp(glm::dot(p - a, ab) / l2, 0.f, 1.f) : 0.f;
  return glm::length(p - (a + ab * t));
}
// Map a pixel inside `r` (atlas pixels) to text-frame coordinates (s, t) in
// [0,1]^2 honouring the rect's text rotation. Returns the text frame aspect.
// Pixel x/y follow the image convention used throughout.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
glm::vec2 textFrame(const Rect &r, const Image &im, int x, int y, float &aspect) {
  const float u = ((static_cast<float>(x) + .5f) / static_cast<float>(im.width) - r.u) / r.w;
  const float v = ((static_cast<float>(y) + .5f) / static_cast<float>(im.height) - r.v) / r.h;
  const float pw = r.w * static_cast<float>(im.width), ph = r.h * static_cast<float>(im.height);
  switch (((r.rotation % 360) + 360) % 360) {
  case 90: aspect = ph / pw; return {v, 1.f - u};
  case 180: aspect = pw / ph; return {1.f - u, 1.f - v};
  case 270: aspect = ph / pw; return {1.f - v, u};
  default: aspect = pw / ph; return {u, v};
  }
}
// Draws `text` centred in the rect's text frame with stroke font glyphs.
void drawText(Image &im, const Rect &r, std::string_view text, glm::vec3 colour, float heightFraction) {
  if (text.empty())
    return;
  const int x0 = std::max(0, static_cast<int>(std::floor(r.u * static_cast<float>(im.width))));
  const int y0 = std::max(0, static_cast<int>(std::floor(r.v * static_cast<float>(im.height))));
  const int x1 = std::min(im.width, static_cast<int>(std::ceil((r.u + r.w) * static_cast<float>(im.width))));
  const int y1 = std::min(im.height, static_cast<int>(std::ceil((r.v + r.h) * static_cast<float>(im.height))));
  for (int y = y0; y < y1; ++y)
    for (int x = x0; x < x1; ++x) {
      float aspect = 1;
      const glm::vec2 st = textFrame(r, im, x, y, aspect);
      // Glyph cell: height = heightFraction of the frame, width 0.6 x height.
      const float gh = heightFraction, gw = .6f * gh / std::max(aspect, 1e-3f), gap = .25f * gw;
      const float total = static_cast<float>(text.size()) * gw + static_cast<float>(text.size() - 1) * gap;
      const float left = .5f - total * .5f, top = .5f - gh * .5f;
      float best = 1e9f;
      for (std::size_t i = 0; i < text.size(); ++i) {
        const auto it = font().find(text[i]);
        if (it == font().end())
          continue;
        const float cx = left + static_cast<float>(i) * (gw + gap);
        // Glyph-local coordinates in "height units" so strokes are isotropic.
        const glm::vec2 local((st.x - cx) / gw, (st.y - top) / gh);
        if (local.x < -.3f || local.x > 1.3f || local.y < -.3f || local.y > 1.3f)
          continue;
        const glm::vec2 scaled(local.x * .6f, local.y);
        for (const auto &seg : it->second)
          best = std::min(best, segmentDistance(scaled, {seg[0] * .6f, seg[1], seg[2] * .6f, seg[3]}));
      }
      // Stroke half-width 0.09 glyph heights; 1-pixel-ish soft edge.
      const float px = 1.f / (gh * std::min(r.w * static_cast<float>(im.width), r.h * static_cast<float>(im.height)));
      const float a = std::clamp((.09f + px - best) / (2.f * px), 0.f, 1.f);
      blend(im, x, y, colour, a);
    }
}
glm::vec3 parseColour(const nlohmann::json &j) {
  if (!j.is_string())
    return glm::vec3(-1);
  const auto s = j.get<std::string>();
  if (s.size() != 7 || s[0] != '#')
    throw std::runtime_error("livery colour must be #rrggbb: " + s);
  glm::vec3 c;
  for (int i = 0; i < 3; ++i)
    c[i] = static_cast<float>(std::stoul(s.substr(1 + 2 * static_cast<std::size_t>(i), 2), nullptr, 16)) / 255.f;
  return c;
}
Rect parseRect(const nlohmann::json &j) {
  Rect r;
  const auto &a = j.at("rect");
  r.u = a.at(0).get<float>();
  r.v = a.at(1).get<float>();
  r.w = a.at(2).get<float>();
  r.h = a.at(3).get<float>();
  r.rotation = j.value("rotation", 0);
  if (r.w <= 0 || r.h <= 0 || r.u < 0 || r.v < 0 || r.u + r.w > 1.0001f || r.v + r.h > 1.0001f)
    throw std::runtime_error("livery rect outside the atlas");
  if (r.rotation % 90)
    throw std::runtime_error("livery rotation must be a multiple of 90");
  return r;
}
} // namespace

Image makeImage(int width, int height, std::array<std::uint8_t, 4> fill) {
  if (width <= 0 || height <= 0)
    throw std::invalid_argument("image size");
  Image im;
  im.width = width;
  im.height = height;
  im.rgba.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4);
  for (std::size_t i = 0; i < im.rgba.size(); i += 4)
    std::copy(fill.begin(), fill.end(), im.rgba.begin() + static_cast<std::ptrdiff_t>(i));
  return im;
}

Roles classifyRoles(const Image &albedo, const Image *emission) {
  if (albedo.empty())
    throw std::invalid_argument("empty albedo");
  Roles r;
  r.map = makeImage(albedo.width, albedo.height, {0, 0, 0, 0});
  r.lights = makeImage(albedo.width, albedo.height, {0, 0, 0, 0});
  const int w = albedo.width, h = albedo.height;
  // Local luminance variance over a 5x5 window via summed-area tables:
  // smooth dark regions are painted glass, textured dark regions are rubber.
  std::vector<double> sum(static_cast<std::size_t>(w + 1) * static_cast<std::size_t>(h + 1)), sq(sum.size());
  auto idx = [&](int x, int y) { return static_cast<std::size_t>(y) * static_cast<std::size_t>(w + 1) + static_cast<std::size_t>(x); };
  for (int y = 0; y < h; ++y)
    for (int x = 0; x < w; ++x) {
      const double l = luma(colourAt(albedo, x, y));
      sum[idx(x + 1, y + 1)] = l + sum[idx(x, y + 1)] + sum[idx(x + 1, y)] - sum[idx(x, y)];
      sq[idx(x + 1, y + 1)] = l * l + sq[idx(x, y + 1)] + sq[idx(x + 1, y)] - sq[idx(x, y)];
    }
  const bool emissionMatches = emission && emission->width == w && emission->height == h;
  for (int y = 0; y < h; ++y)
    for (int x = 0; x < w; ++x) {
      if (albedo.at(x, y)[3] < 128)
        continue;
      const int ax = std::max(0, x - 2), ay = std::max(0, y - 2), bx = std::min(w, x + 3), by = std::min(h, y + 3);
      const double n = static_cast<double>((bx - ax) * (by - ay));
      const double s = sum[idx(bx, by)] - sum[idx(ax, by)] - sum[idx(bx, ay)] + sum[idx(ax, ay)];
      const double q = sq[idx(bx, by)] - sq[idx(ax, by)] - sq[idx(bx, ay)] + sq[idx(ax, ay)];
      const float variance = static_cast<float>(std::max(0.0, q / n - (s / n) * (s / n)));
      const glm::vec3 c = colourAt(albedo, x, y);
      const float l = luma(c), sat = saturation(c);
      auto *role = r.map.at(x, y);
      auto *light = r.lights.at(x, y);
      if (emissionMatches) {
        const auto *e = emission->at(x, y);
        // Shipped emission maps keep RGB everywhere and mark lamps in alpha.
        if (std::max({e[0], e[1], e[2]}) * e[3] > 60 * 255) {
          light[0] = 255;
          ++r.counts[4];
        }
      }
      if (l > .82f && sat < .15f && variance > .0015f) { // bright textured whites: lamps
        light[1] = 255;
        ++r.counts[5];
      }
      if (l < .16f && sat < .35f && variance < .0015f) { // smooth dark: glass
        role[1] = 255;
        ++r.counts[1];
      } else if (l < .22f && sat < .35f) { // textured dark grey/black: tyres, grilles, rubber trim
        // (saturated darks such as deep red or navy are paint, not rubber)
        role[2] = 255;
        ++r.counts[2];
      } else if (l > .55f && sat < .12f && variance > .004f) { // bright, grey, detailed: metal
        role[3] = 255;
        ++r.counts[3];
      } else { // everything else is clear-coated paint
        role[0] = 255;
        ++r.counts[0];
      }
    }
  return r;
}

std::array<glm::vec3, 2> dominantPaint(const Image &albedo, const Roles &roles) {
  std::vector<glm::vec3> samples;
  for (int y = 0; y < albedo.height; y += 2)
    for (int x = 0; x < albedo.width; x += 2)
      if (roles.map.at(x, y)[0])
        samples.push_back(colourAt(albedo, x, y));
  if (samples.empty())
    return {glm::vec3(.5f), glm::vec3(.5f)};
  // Deterministic 2-means seeded with the darkest and brightest samples.
  auto by = [](glm::vec3 a, glm::vec3 b) { return luma(a) < luma(b); };
  std::array<glm::vec3, 2> centre = {*std::min_element(samples.begin(), samples.end(), by),
                                     *std::max_element(samples.begin(), samples.end(), by)};
  std::array<std::size_t, 2> count{};
  for (int iteration = 0; iteration < 10; ++iteration) {
    std::array<glm::vec3, 2> acc{};
    count = {};
    for (const auto &s : samples) {
      const std::size_t k = glm::length(s - centre[0]) <= glm::length(s - centre[1]) ? 0 : 1;
      acc[k] += s;
      ++count[k];
    }
    for (std::size_t k = 0; k < 2; ++k)
      if (count[k])
        centre[k] = acc[k] / static_cast<float>(count[k]);
  }
  if (count[1] > count[0])
    std::swap(centre[0], centre[1]);
  return centre;
}

Livery parseLivery(std::string_view text) {
  const auto j = nlohmann::json::parse(text);
  if (j.value("schema", std::string()) != "trackrs.livery/1")
    throw std::runtime_error("livery schema must be trackrs.livery/1");
  Livery l;
  l.id = j.value("id", std::string());
  if (j.contains("primary"))
    l.primary = parseColour(j["primary"]);
  if (j.contains("secondary"))
    l.secondary = parseColour(j["secondary"]);
  if (j.contains("primary_source"))
    l.primarySource = parseColour(j["primary_source"]);
  if (j.contains("secondary_source"))
    l.secondarySource = parseColour(j["secondary_source"]);
  l.number = j.value("number", -1);
  if (j.contains("number_colour"))
    l.numberColour = parseColour(j["number_colour"]);
  if (j.contains("panel_colour"))
    l.panelColour = parseColour(j["panel_colour"]);
  for (const auto &p : j.value("number_panels", nlohmann::json::array()))
    l.numberPanels.push_back(parseRect(p));
  for (const auto &p : j.value("sponsor_slots", nlohmann::json::array()))
    l.sponsorSlots.push_back(parseRect(p));
  l.sponsorSeed = j.value("sponsor_seed", std::uint64_t{1});
  if (l.number > 999)
    throw std::runtime_error("race number must be 0..999");
  return l;
}

void applyLivery(Image &albedo, const Roles &roles, const Livery &l) {
  if (roles.map.width != albedo.width || roles.map.height != albedo.height)
    throw std::invalid_argument("role map does not match the atlas");
  const auto dominant = dominantPaint(albedo, roles);
  const glm::vec3 sources[2] = {l.primarySource.x >= 0 ? l.primarySource : dominant[0],
                                l.secondarySource.x >= 0 ? l.secondarySource : dominant[1]};
  const glm::vec3 targets[2] = {l.primary, l.secondary};
  if (targets[0].x >= 0 || targets[1].x >= 0)
    for (int y = 0; y < albedo.height; ++y)
      for (int x = 0; x < albedo.width; ++x) {
        if (!roles.map.at(x, y)[0])
          continue;
        const glm::vec3 c = colourAt(albedo, x, y);
        const float d0 = glm::length(c - sources[0]), d1 = glm::length(c - sources[1]);
        const std::size_t k = d0 <= d1 ? 0 : 1;
        if (targets[k].x < 0)
          continue;
        // Keep the texel's shading relative to its source colour.
        const float weight = 1.f - std::clamp(((k ? d1 : d0) - .12f) / .14f, 0.f, 1.f);
        if (weight <= 0)
          continue;
        const float shade = std::clamp(luma(c) / std::max(luma(sources[k]), .03f), 0.f, 1.8f);
        const glm::vec3 recoloured = glm::clamp(targets[k] * shade, glm::vec3(0), glm::vec3(1));
        blend(albedo, x, y, recoloured, weight);
      }
  for (const auto &panel : l.numberPanels) {
    const int x0 = static_cast<int>(panel.u * static_cast<float>(albedo.width)), y0 = static_cast<int>(panel.v * static_cast<float>(albedo.height));
    const int x1 = static_cast<int>((panel.u + panel.w) * static_cast<float>(albedo.width)), y1 = static_cast<int>((panel.v + panel.h) * static_cast<float>(albedo.height));
    if (l.number >= 0) {
      for (int y = std::max(0, y0); y < std::min(albedo.height, y1); ++y)
        for (int x = std::max(0, x0); x < std::min(albedo.width, x1); ++x)
          blend(albedo, x, y, l.panelColour, 1.f);
      drawNumber(albedo, panel, l.number, l.numberColour);
    }
  }
  for (std::size_t i = 0; i < l.sponsorSlots.size(); ++i) {
    const auto &slot = l.sponsorSlots[i];
    const int x0 = static_cast<int>(slot.u * static_cast<float>(albedo.width)), y0 = static_cast<int>(slot.v * static_cast<float>(albedo.height));
    const int pw = std::max(1, static_cast<int>(slot.w * static_cast<float>(albedo.width))), ph = std::max(1, static_cast<int>(slot.h * static_cast<float>(albedo.height)));
    const bool turned = slot.rotation % 180 != 0;
    const Image logo = sponsorLogo(l.sponsorSeed + i, turned ? ph : pw, turned ? pw : ph);
    for (int y = 0; y < ph; ++y)
      for (int x = 0; x < pw; ++x) {
        // Atlas pixel -> logo pixel through the slot rotation.
        int lx = x, ly = y;
        switch (((slot.rotation % 360) + 360) % 360) {
        case 90: lx = y; ly = pw - 1 - x; break;
        case 180: lx = pw - 1 - x; ly = ph - 1 - y; break;
        case 270: lx = ph - 1 - y; ly = x; break;
        default: break;
        }
        if (lx < 0 || ly < 0 || lx >= logo.width || ly >= logo.height)
          continue;
        const auto *src = logo.at(lx, ly);
        blend(albedo, x0 + x, y0 + y, {channel(src[0]), channel(src[1]), channel(src[2])}, channel(src[3]));
      }
  }
}

bool drawNumber(Image &image, const Rect &rect, int number, glm::vec3 colour) {
  if (number < 0 || number > 999)
    return false;
  drawText(image, rect, std::to_string(number), colour, .7f);
  return true;
}

std::string sponsorName(std::uint64_t seed) {
  // Invented syllables; checked against nothing but deliberately generic.
  static const char *first[] = {"VEL", "NOR", "KAL", "TRI", "ARO", "ZEN", "OBR", "LUM", "QUA", "MIR", "DEX", "FOR"};
  static const char *second[] = {"ON", "AX", "IKA", "ERA", "UM", "OLT", "IS", "ANO", "EX", "ITE"};
  Pcg32 rng(seed, 0x5bULL);
  std::string name = first[rng.next() % 12u];
  name += second[rng.next() % 10u];
  return name;
}

// Seed, width and height are distinct quantities in the declared order.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
Image sponsorLogo(std::uint64_t seed, int width, int height) {
  Image logo = makeImage(width, height, {0, 0, 0, 0});
  Pcg32 rng(seed, 0x10c0ULL);
  static const glm::vec3 palette[] = {{.95f, .95f, .95f}, {.08f, .08f, .1f}, {.9f, .75f, .1f},
                                      {.1f, .35f, .8f},   {.8f, .12f, .1f},  {.1f, .6f, .3f}};
  const glm::vec3 fill = palette[rng.next() % 6u];
  glm::vec3 ink = palette[rng.next() % 6u];
  if (glm::length(ink - fill) < .5f)
    ink = luma(fill) > .5f ? palette[1] : palette[0];
  const unsigned shape = rng.next() % 3u; // 0 plate, 1 lozenge, 2 chevron band
  for (int y = 0; y < height; ++y)
    for (int x = 0; x < width; ++x) {
      const float u = (static_cast<float>(x) + .5f) / static_cast<float>(width) * 2.f - 1.f;
      const float v = (static_cast<float>(y) + .5f) / static_cast<float>(height) * 2.f - 1.f;
      float inside = 0;
      if (shape == 0)
        inside = std::max(std::abs(u), std::abs(v)) < .94f ? 1.f : 0.f;
      else if (shape == 1)
        inside = std::abs(u) * .6f + std::abs(v) * .9f < .95f ? 1.f : 0.f;
      else
        inside = std::abs(v) < .7f && std::abs(u + .25f * v) < .98f ? 1.f : 0.f;
      if (inside > 0) {
        blend(logo, x, y, fill, 1.f);
        logo.at(x, y)[3] = 255;
      }
    }
  // Text fills the middle band: render a white-on-black coverage mask, then ink.
  Rect band{.08f, .2f, .84f, .6f, 0};
  Image mask = makeImage(width, height, {0, 0, 0, 255});
  drawText(mask, band, sponsorName(seed), glm::vec3(1), .8f);
  for (int y = 0; y < height; ++y)
    for (int x = 0; x < width; ++x)
      if (logo.at(x, y)[3])
        blend(logo, x, y, ink, channel(mask.at(x, y)[0]));
  return logo;
}

LodSet buildLods(const float *positions, std::size_t vertexCount, std::size_t stride, const std::vector<std::uint32_t> &indices) {
  if (!positions || !vertexCount || indices.size() < 3 || indices.size() % 3)
    throw std::invalid_argument("invalid mesh for LODs");
  LodSet lods;
  lods.indices[0] = indices;
  lods.minimum = glm::vec3(1e30f);
  lods.maximum = glm::vec3(-1e30f);
  const auto *bytes = reinterpret_cast<const std::uint8_t *>(positions);
  for (std::size_t i = 0; i < vertexCount; ++i) {
    glm::vec3 p;
    std::copy_n(reinterpret_cast<const float *>(bytes + i * stride), 3, &p.x);
    lods.minimum = glm::min(lods.minimum, p);
    lods.maximum = glm::max(lods.maximum, p);
  }
  lods.radius = .5f * glm::length(lods.maximum - lods.minimum);
  const std::array<float, 2> ratio = {.5f, .15f};
  for (std::size_t level = 1; level < 3; ++level) {
    const auto target = std::max<std::size_t>(3, static_cast<std::size_t>(static_cast<float>(indices.size()) * ratio[level - 1]) / 3 * 3);
    std::vector<std::uint32_t> out(indices.size());
    float error = 0;
    auto size = meshopt_simplify(out.data(), indices.data(), indices.size(), positions, vertexCount, stride, target, .05f, 0, &error);
    if (size > target + target / 2) // UV seams block collapses: fall back to sloppy clustering
      size = meshopt_simplifySloppy(out.data(), indices.data(), indices.size(), positions, vertexCount, stride, target, .08f, &error);
    out.resize(size);
    if (out.empty() || out.size() > lods.indices[level - 1].size()) // never coarser LOD with more triangles
      out = lods.indices[level - 1];
    lods.indices[level] = std::move(out);
  }
  return lods;
}

unsigned chooseLod(float radius, float distance, float fovY) {
  if (!(distance > 0))
    return 0;
  const float fraction = radius / (distance * std::tan(.5f * fovY));
  return fraction >= .2f ? 0u : fraction >= .06f ? 1u : 2u;
}

void updateWear(Wear &w, const WearInput &in) {
  const float dt = std::clamp(in.dt, 0.f, .25f);
  const float distanceKm = std::max(in.speed, 0.f) * dt / 1000.f;
  float dusty = 0, muddy = 0;
  switch (in.surface) {
  case Surface::Gravel: dusty = 1.f; muddy = .5f; break;
  case Surface::Dirt: dusty = .9f; muddy = .8f; break;
  case Surface::Sand: dusty = 1.2f; break;
  case Surface::Mud: dusty = .2f; muddy = 1.6f; break;
  case Surface::Snow: muddy = .15f; break;
  case Surface::Grass: dusty = .2f; muddy = .5f; break;
  default: dusty = .03f; break; // tarmac/other: road grime only
  }
  // ~2 km of dry gravel to heavy dust; wet loose surfaces build mud instead.
  w.dust += distanceKm * .5f * dusty * (1.f - in.wetness);
  w.mud += distanceKm * .6f * muddy * (.25f + .75f * in.wetness);
  w.dust -= dt * .004f * in.wetness * w.dust; // rain slowly rinses dust
  w.dust = std::clamp(w.dust, 0.f, 1.f);
  w.mud = std::clamp(w.mud, 0.f, 1.f);
  // Brakes: absorbed power ~ brake * speed; Newtonian cooling to 25 °C.
  for (std::size_t i = 0; i < 4; ++i) {
    const float share = i < 2 ? in.frontBias : 1.f - in.frontBias;
    const float heat = std::clamp(in.brake, 0.f, 1.f) * std::max(in.speed, 0.f) * share * 14.f;
    const float cool = (w.brakeC[i] - 25.f) * (.06f + .004f * in.speed);
    w.brakeC[i] = std::clamp(w.brakeC[i] + (heat - cool) * dt, 25.f, 950.f);
  }
}

float brakeGlow(float c) { return std::clamp((c - 350.f) / 300.f, 0.f, 1.f); }

// Gauge value/range and sweep angles: distinct units, documented in the header.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
float needleAngle(float value, float maximum, float start, float sweep) {
  const float t = maximum > 0 && std::isfinite(value) ? std::clamp(value / maximum, 0.f, 1.f) : 0.f;
  return start + sweep * t;
}

// Count and stride follow the meshoptimizer vertex-stream convention.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
Cabin measureCabin(const float *positions, std::size_t vertexCount, std::size_t stride, glm::vec3 lo, glm::vec3 hi) {
  Cabin cabin;
  const glm::vec3 size = hi - lo;
  if (!positions || !vertexCount || !(size.x > 0 && size.y > 0 && size.z > 0))
    return cabin;
  const auto *bytes = reinterpret_cast<const std::uint8_t *>(positions);
  auto at = [&](std::size_t i) {
    glm::vec3 p;
    std::copy_n(reinterpret_cast<const float *>(bytes + i * stride), 3, &p.x);
    return p;
  };
  const float cx = .5f * (lo.x + hi.x), y0 = lo.y + .35f * size.y, y1 = lo.y + .6f * size.y;
  // Roof skin = the lowest of the per-slice centre-line maxima across the
  // cabin band, so scoops, vents or antennas cannot lift it.
  constexpr int Slices = 6;
  std::array<float, Slices> top;
  top.fill(-1e30f);
  for (std::size_t i = 0; i < vertexCount; ++i) {
    const glm::vec3 p = at(i);
    if (p.y < y0 || p.y > y1 || std::abs(p.x - cx) >= .15f * size.x)
      continue;
    const int s = std::min(Slices - 1, static_cast<int>((p.y - y0) / (y1 - y0) * Slices));
    top[static_cast<std::size_t>(s)] = std::max(top[static_cast<std::size_t>(s)], p.z);
  }
  // Slices without a roof-height vertex (sparse low-poly layouts, floor only) are skipped.
  float roof = 1e30f;
  for (float z : top)
    if (z >= lo.z + .4f * size.z)
      roof = std::min(roof, z);
  if (roof > hi.z) // no roof skin found (open car or odd layout)
    return cabin;
  float width = 0;
  for (std::size_t i = 0; i < vertexCount; ++i) {
    const glm::vec3 p = at(i);
    // Width at the roof edge (low-poly bodies have no vertices between belt line and roof).
    if (p.y >= y0 && p.y <= y1 && p.z >= roof - .15f * size.z && p.z <= roof + .01f * size.z)
      width = std::max(width, std::abs(p.x - cx));
  }
  if (width <= 0)
    return cabin;
  cabin.roofZ = roof;
  cabin.halfWidth = width;
  cabin.measured = true;
  return cabin;
}

Interior buildInterior(glm::vec3 lo, glm::vec3 hi, Cabin cabin) {
  Interior in;
  auto &m = in.mesh;
  const glm::vec3 size = hi - lo;
  if (!(size.x > 0 && size.y > 0 && size.z > 0))
    throw std::invalid_argument("invalid body bounds");
  // Cabin estimate for a hatchback-style rally body (body space: +y forward):
  // dash at 62 % of the length, seats around 45 %, roof at 92 % height.
  const float floorZ = lo.z + .22f * size.z, dashY = lo.y + .62f * size.y, seatY = lo.y + .42f * size.y;
  // Cage inset: bounds include mirrors and the roof tumbles in, so stay well inside.
  float halfW = .28f * size.x, roofZ = lo.z + .82f * size.z;
  if (cabin.measured) { // tubes run just under the measured roof skin and inside the shoulders
    roofZ = std::min(roofZ, cabin.roofZ - .07f * size.z);
    halfW = std::min(halfW, .85f * cabin.halfWidth);
  }
  const float dashZ = std::min(lo.z + .58f * size.z, roofZ - .2f * size.z);
  auto box = [&](glm::vec3 a, glm::vec3 b, glm::vec3 colour, glm::vec2 uv = glm::vec2(0)) {
    const glm::vec3 c[8] = {{a.x, a.y, a.z}, {b.x, a.y, a.z}, {b.x, b.y, a.z}, {a.x, b.y, a.z},
                            {a.x, a.y, b.z}, {b.x, a.y, b.z}, {b.x, b.y, b.z}, {a.x, b.y, b.z}};
    const int faces[6][4] = {{0, 3, 2, 1}, {4, 5, 6, 7}, {0, 1, 5, 4}, {1, 2, 6, 5}, {2, 3, 7, 6}, {3, 0, 4, 7}};
    const glm::vec3 centre = (a + b) * .5f;
    for (const auto &face : faces) {
      int f[4] = {face[0], face[1], face[2], face[3]};
      glm::vec3 n = glm::normalize(glm::cross(c[f[1]] - c[f[0]], c[f[2]] - c[f[0]]));
      if (glm::dot(n, (c[f[0]] + c[f[2]]) * .5f - centre) < 0) {
        std::swap(f[1], f[3]);
        n = -n;
      }
      const auto base = static_cast<std::uint16_t>(m.vertices.size());
      for (int k : f) {
        vegetation::Vertex v{};
        v.position[0] = c[k].x;
        v.position[1] = c[k].y;
        v.position[2] = c[k].z;
        v.normal = mesh::packSnorm2_10_10_10(n.x, n.y, n.z, 0);
        v.color = mesh::packRGBA8(colour.r, colour.g, colour.b, 1);
        v.uv[0] = static_cast<std::uint16_t>(std::lround(uv.x * 65535.f));
        v.uv[1] = static_cast<std::uint16_t>(std::lround(uv.y * 65535.f));
        m.vertices.push_back(v);
      }
      m.indices.insert(m.indices.end(), {base, static_cast<std::uint16_t>(base + 1), static_cast<std::uint16_t>(base + 2), base,
                                         static_cast<std::uint16_t>(base + 2), static_cast<std::uint16_t>(base + 3)});
    }
  };
  const glm::vec3 trim(.05f), seat(.12f, .12f, .14f), cage(.35f, .35f, .37f), face(.02f);
  const float t = .02f * size.x; // tube / panel thickness
  // Dashboard and instrument binnacle (driver on the left, +x right).
  box({lo.x + .1f * size.x, dashY, dashZ - .08f * size.z}, {hi.x - .1f * size.x, dashY + .08f * size.y, dashZ}, trim);
  const float clusterX = -.2f * size.x, clusterZ = dashZ + .02f * size.z;
  box({clusterX - .09f * size.x, dashY - t, clusterZ - .035f * size.z}, {clusterX + .09f * size.x, dashY, clusterZ + .035f * size.z}, face);
  // Seats (driver + co-driver).
  for (float side : {-1.f, 1.f}) {
    const float cx = side * .2f * size.x;
    box({cx - .09f * size.x, seatY - .05f * size.y, floorZ}, {cx + .09f * size.x, seatY + .05f * size.y, floorZ + .12f * size.z}, seat);
    box({cx - .09f * size.x, seatY - .07f * size.y, floorZ + .1f * size.z}, {cx + .09f * size.x, seatY - .045f * size.y, dashZ + .12f * size.z}, seat);
  }
  // Roll cage: main hoop behind the seats, A-pillar bars, roof bars, diagonal.
  const float hoopY = seatY - .1f * size.y;
  for (float side : {-1.f, 1.f}) {
    const float x = side * halfW;
    box({x - t, hoopY - t, floorZ}, {x + t, hoopY + t, roofZ}, cage);
    box({x - t, dashY - t, floorZ}, {x + t, dashY + t, roofZ - .05f * size.z}, cage);
    box({x - t, hoopY, roofZ - t}, {x + t, dashY, roofZ + t}, cage);
  }
  box({-halfW, hoopY - t, roofZ - t}, {halfW, hoopY + t, roofZ + t}, cage);
  box({-halfW, hoopY - t, floorZ + .3f * size.z}, {halfW, hoopY + t, floorZ + .3f * size.z + 2 * t}, cage);
  // Gauge needles (unit length along +z in pivot space; uv.y = length).
  in.tachoPivot = {clusterX - .045f * size.x, dashY - 1.5f * t, clusterZ};
  in.speedPivot = {clusterX + .045f * size.x, dashY - 1.5f * t, clusterZ};
  const float needle = .03f * size.z;
  box({in.tachoPivot.x - t * .25f, in.tachoPivot.y - t * .2f, in.tachoPivot.z}, {in.tachoPivot.x + t * .25f, in.tachoPivot.y, in.tachoPivot.z + needle},
      {.95f, .25f, .05f}, {1.f, 0.f});
  box({in.speedPivot.x - t * .25f, in.speedPivot.y - t * .2f, in.speedPivot.z}, {in.speedPivot.x + t * .25f, in.speedPivot.y, in.speedPivot.z + needle},
      {.95f, .95f, .95f}, {.75f, 0.f});
  return in;
}
} // namespace trackrs::car
