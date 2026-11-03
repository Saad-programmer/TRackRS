// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "ktx2.h"
#include <functional>
#include <glm/glm.hpp>
namespace trackrs::assets {
using Environment = std::function<glm::vec3(glm::vec3)>;
struct IblCache {
  TextureData sky, irradiance, prefilter, brdf;
};
glm::vec3 cubeDirection(unsigned face, float u, float v);
IblCache bakeIbl(const Environment &environment, unsigned size = 64,
                 unsigned samples = 256);
glm::vec3 analyticSky(glm::vec3 direction);
} // namespace trackrs::assets
