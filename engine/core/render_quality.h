// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstdint>
#include <initializer_list>
#include <stdexcept>
#include <string_view>
namespace trackrs {
enum class RenderQuality : std::uint8_t { Low, Medium, High, Ultra };
constexpr unsigned carProbeResolution(RenderQuality quality) {
  return quality == RenderQuality::High || quality == RenderQuality::Ultra
             ? 128u
             : 32u;
}
constexpr std::string_view qualityName(RenderQuality quality) {
  switch (quality) {
  case RenderQuality::Low:
    return "low";
  case RenderQuality::Medium:
    return "medium";
  case RenderQuality::High:
    return "high";
  case RenderQuality::Ultra:
    return "ultra";
  }
  return "medium";
}
inline RenderQuality parseQuality(std::string_view name) {
  for (auto quality : {RenderQuality::Low, RenderQuality::Medium,
                       RenderQuality::High, RenderQuality::Ultra})
    if (qualityName(quality) == name)
      return quality;
  throw std::invalid_argument(
      "render quality must be low, medium, high or ultra");
}
} // namespace trackrs
