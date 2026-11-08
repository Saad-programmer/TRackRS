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
  result.daylight = glm::smoothstep(-.25f, .1f, elevation);
  float height = std::clamp(elevation, 0.f, 1.f);
  auto colour = glm::mix(glm::vec3(1, .24f, .07f), glm::vec3(1, .97f, .9f),
                         std::sqrt(height));
  result.radiance = colour * (3.f * glm::smoothstep(-.025f, .25f, elevation) *
                              (.2f + .8f * std::sqrt(height)));
  result.skyRadiance =
      colour * std::max(std::max({result.radiance.x, result.radiance.y,
                                  result.radiance.z}),
                        2.f * result.daylight);
  result.ambient = glm::mix(glm::vec3(.008f, .012f, .025f), glm::vec3(.08f),
                            result.daylight);
  return result;
}
Sun overcastSun(Sun sun, float overcast) {
  const float o = std::clamp(std::isfinite(overcast) ? overcast : 0.f, 0.f, 1.f);
  const float beam = 1.f - .85f * o;
  // Clouds scatter the beam into a whiter, dimmer sky dome.
  const glm::vec3 grey(std::max({sun.skyRadiance.x, sun.skyRadiance.y, sun.skyRadiance.z}));
  sun.radiance *= beam;
  sun.skyRadiance = glm::mix(sun.skyRadiance, grey * .55f, o);
  sun.ambient = glm::mix(sun.ambient, sun.ambient * 1.6f + glm::vec3(.02f) * sun.daylight, o);
  return sun;
}
Weather weatherFromLevel(float rain, float snowfall, float fog) {
  auto finite = [](float v) { return std::isfinite(v) ? std::max(v, 0.f) : 0.f; };
  rain = finite(rain);
  snowfall = finite(snowfall);
  fog = finite(fog);
  Weather w;
  const float r = std::clamp(rain / 1500.f, 0.f, 1.f), s = std::clamp(snowfall / 1000.f, 0.f, 1.f);
  if (rain > 0) {
    w.wetness = .35f + .6f * r;
    w.puddles = .2f + .6f * r;
  }
  if (snowfall > 0) {
    w.snow = .45f + .45f * s;
    w.wetness = std::max(w.wetness, .15f);
  }
  w.overcast = std::max(rain > 0 ? .45f + .45f * r : 0.f, snowfall > 0 ? .4f + .3f * s : 0.f);
  // Legacy GL_EXP densities were tuned for a short far plane; .01 (the old
  // default) maps to the Phase 3b constant .003 /m.
  w.fogDensity = std::clamp(fog * .3f, .0005f, .012f);
  return w;
}
bool validWeatherName(std::string_view name) {
  return name == "level" || name == "clear" || name == "rain" || name == "snow" || name == "fog";
}
Weather weatherPreset(std::string_view name, const Weather &level) {
  if (name == "level")
    return level;
  Weather w;
  if (name == "clear")
    w.fogDensity = .0015f;
  else if (name == "rain")
    w = {.9f, .7f, 0.f, .85f, .005f};
  else if (name == "snow")
    w = {.2f, 0.f, .85f, .65f, .005f};
  else if (name == "fog")
    w = {.3f, .1f, 0.f, .55f, .012f};
  else
    throw std::invalid_argument("weather must be level, clear, rain, snow or fog");
  return w;
}
std::array<SpotLight, 2> headlights(glm::vec3 p, glm::vec3 right, glm::vec3 forward, glm::vec3 up) {
  std::array<SpotLight, 2> lamps{};
  const glm::vec3 f = glm::normalize(forward), r = glm::normalize(right), u = glm::normalize(up);
  for (int side = 0; side < 2; ++side) {
    auto &l = lamps[static_cast<std::size_t>(side)];
    l.position = p + f * 1.9f + u * .15f + r * (side ? .62f : -.62f);
    l.range = 90.f;
    l.direction = glm::normalize(f - u * .02f);   // low beam: ~1 degree down, reaches ~40-60 m
    l.radiance = glm::vec3(1.f, .93f, .8f) * 420.f; // warm halogen, finite range
  }
  return lamps;
}
bool headlightsOn(const Sun &sun, float overcast) {
  // `sun` is already attenuated by cloud cover; heavy fog/cloud with a low
  // sun (weak beam) also counts as gloomy.
  const float beam = std::max({sun.radiance.x, sun.radiance.y, sun.radiance.z});
  return sun.daylight < .65f || beam < .6f || (overcast > .9f && beam < 1.f);
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
Cascades makeCascades(const CameraMatrices &camera, glm::vec3 sun,
                      unsigned count, unsigned resolution, float distance) {
  if (count < 3 || count > 4 || resolution < 32 || distance <= 1)
    throw std::invalid_argument("invalid cascade configuration");
  Cascades result;
  result.count = count;
  result.resolution = resolution;
  auto inverse = glm::inverse(camera.view);
  const float near = .1f, tangent = 1.f / camera.projection[1][1],
              aspect = camera.projection[1][1] / camera.projection[0][0];
  sun = glm::normalize(sun);
  auto up = std::abs(sun.x) > .95f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
  auto rotation = glm::lookAt(glm::vec3(0), -sun, up);
  float previous = near;
  for (unsigned i = 0; i < count; ++i) {
    float fraction = float(i + 1) / float(count);
    float end = glm::mix(near + (distance - near) * fraction,
                         near * std::pow(distance / near, fraction), .7f);
    result.splits[static_cast<int>(i)] = end;
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
    result.bias[static_cast<int>(i)] = texel / (2.f * (radius + casterPadding));
    previous = end;
  }
  return result;
}
} // namespace trackrs::world
