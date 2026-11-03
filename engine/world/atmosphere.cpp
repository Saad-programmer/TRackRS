// SPDX-License-Identifier: GPL-2.0-or-later
#include "atmosphere.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>
namespace trackrs::world {
Sun sunlight(float hour) {
  if (!std::isfinite(hour))
    throw std::invalid_argument("nonfinite solar time");
  float angle = (hour - 6.f) * glm::pi<float>() / 12.f;
  Sun result;
  result.direction = glm::normalize(
      glm::vec3(.25f * std::cos(angle), std::cos(angle), std::sin(angle)));
  float elevation = result.direction.z;
  result.daylight = glm::smoothstep(-.12f, .08f, elevation);
  float height = std::clamp(elevation, 0.f, 1.f);
  auto colour = glm::mix(glm::vec3(1, .24f, .07f), glm::vec3(1, .97f, .9f),
                         std::sqrt(height));
  result.radiance = colour * (3.f * glm::smoothstep(-.025f, .25f, elevation) *
                              (.2f + .8f * std::sqrt(height)));
  result.ambient = glm::mix(glm::vec3(.001f, .002f, .005f), glm::vec3(.08f),
                            result.daylight);
  return result;
}
float presetHour(std::string_view name) {
  if (name == "dawn")
    return 6;
  if (name == "noon")
    return 12;
  if (name == "golden-hour")
    return 17;
  if (name == "dusk")
    return 18.5f;
  if (name == "night")
    return 22;
  if (name == "midnight")
    return 0;
  throw std::invalid_argument(
      "time must be dawn, noon, golden-hour, dusk, night or midnight");
}
float fogOpticalDepth(float density, float falloff, float a, float b,
                      float distance) {
  if (density < 0 || falloff < 0 || distance < 0 || !std::isfinite(a) ||
      !std::isfinite(b))
    throw std::invalid_argument("invalid height fog domain");
  float x = std::clamp(falloff * (b - a), -50.f, 50.f);
  float integral =
      std::abs(x) < 1e-4f ? 1.f - x * .5f + x * x / 6.f : -std::expm1(-x) / x;
  return density * distance * std::exp(std::clamp(-falloff * a, -50.f, 50.f)) *
         integral;
}
Cascades makeCascades(const glm::mat4 &view, const glm::mat4 &projection,
                      glm::vec3 sun, unsigned count, unsigned resolution,
                      float distance) {
  if (count < 3 || count > 4 || resolution < 32 || distance <= 1)
    throw std::invalid_argument("invalid cascade configuration");
  Cascades result;
  result.count = count;
  result.resolution = resolution;
  auto inverse = glm::inverse(view);
  const float near = .1f, tangent = 1.f / projection[1][1],
              aspect = projection[1][1] / projection[0][0];
  sun = glm::normalize(sun);
  auto up = std::abs(sun.z) > .95f ? glm::vec3(0, 1, 0) : glm::vec3(0, 0, 1);
  auto rotation = glm::lookAt(glm::vec3(0), -sun, up);
  float previous = near;
  for (unsigned i = 0; i < count; ++i) {
    float fraction = float(i + 1) / float(count);
    float end = glm::mix(near + (distance - near) * fraction,
                         near * std::pow(distance / near, fraction), .7f);
    result.splits[i] = end;
    std::array<glm::vec3, 8> corners{};
    unsigned index = 0;
    // Overlap the preceding split by 10% for continuous cross-fades.
    float start = i ? previous * .9f : near;
    for (float depth : {start, end})
      for (float y : {-1.f, 1.f})
        for (float x : {-1.f, 1.f})
          corners[index++] =
              glm::vec3(inverse * glm::vec4(x * depth * tangent * aspect,
                                            y * depth * tangent, -depth, 1));
    glm::vec3 center(0);
    for (auto p : corners)
      center += p / 8.f;
    float radius = 0;
    for (auto p : corners)
      radius = std::max(radius, glm::length(p - center));
    radius = std::ceil(radius * 16.f) / 16.f;
    float texel = 2.f * radius / float(resolution);
    auto lightCenter = rotation * glm::vec4(center, 1);
    lightCenter.x = std::floor(lightCenter.x / texel + .5f) * texel;
    lightCenter.y = std::floor(lightCenter.y / texel + .5f) * texel;
    center = glm::vec3(glm::inverse(rotation) * lightCenter);
    constexpr float casterPadding = 300;
    auto lightView =
        glm::lookAt(center + sun * (radius + casterPadding), center, up);
    result.matrices[i] = glm::ortho(-radius, radius, -radius, radius, .1f,
                                    2.f * (radius + casterPadding)) *
                         lightView;
    result.bias[i] = texel / (2.f * (radius + casterPadding));
    previous = end;
  }
  return result;
}
} // namespace trackrs::world
