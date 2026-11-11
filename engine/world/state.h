// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "atmosphere.h"
#include <cstdint>
#include <string>
#include <glad/gl.h>
namespace trackrs::world {
struct State {
  Settings settings;
  Sun sun;
  Cascades cascades;
  glm::mat4 view{1}, shadowView{1};
  GLuint shadow = 0, sky = 0, irradiance = 0, prefilter = 0;
  float environmentLod = 0;
  std::array<std::uint64_t, 4> shadowDraws{}, shadowTriangles{};
  std::uint64_t blobDraws = 0;
  bool scene = false, depthPass = false, linearCapture = false;
  std::array<SpotLight, MaxSceneLights> lights{};
  unsigned lightCount = 0;          ///< reset by prepare(); filled per frame
  unsigned temporalFrame = 0;       ///< TAA frame index (0 = static dither patterns)
  std::string weatherName = "level"; ///< user selection (config / --weather)
  Weather levelWeather;              ///< parsed from the loaded stage
};
State &state();
/// Stores `settings` and resolves settings.weather from the weather selection
/// and the stage's level weather; the sun is attenuated by cloud cover.
void configure(Settings settings);
void selectWeather(const std::string &name);
void setLevelWeather(const Weather &weather);
/// Adds a light for this frame; ignored beyond MaxSceneLights.
void addLight(const SpotLight &light);
void apply(GLuint program, const glm::mat4 &localToWorld, bool scene);
} // namespace trackrs::world
