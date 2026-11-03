// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "atmosphere.h"
#include <cstdint>
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
  bool scene = false, depthPass = false;
};
State &state();
void configure(Settings settings);
void apply(GLuint program, const glm::mat4 &localToWorld, bool scene);
} // namespace trackrs::world
