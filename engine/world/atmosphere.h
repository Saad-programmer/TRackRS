// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <array>
#include <glm/glm.hpp>
#include <string_view>
namespace trackrs::world {
/// Surface/sky weather state shared by every PBR pass. All fields 0..1 except
/// fogDensity (extinction per metre at sea level). Presentation only: it never
/// feeds physics (grip changes belong to Phase 6).
struct Weather {
  float wetness = 0;    // thin water film: darker albedo, lower roughness
  float puddles = 0;    // standing water on flat road-like terrain
  float snow = 0;       // accumulation on upward-facing surfaces
  float overcast = 0;   // cloud cover: dims the direct beam, greys the sky
  float fogDensity = .003f;
};
/// Level <weather> attributes (legacy units: drops/s, flakes/s, GL_EXP density).
Weather weatherFromLevel(float rain, float snowfall, float legacyFogDensity);
/// "level" returns `level`; otherwise clear, rain, snow or fog.
Weather weatherPreset(std::string_view name, const Weather &level);
bool validWeatherName(std::string_view name);
struct Settings {
  bool enabled = false, csm = true;
  float hour = 12;
  Weather weather;
};
struct Sun {
  glm::vec3 direction{0, 0, 1}, radiance{3}, skyRadiance{3}, ambient{.08f};
  float daylight = 1;
};
Sun sunlight(float hour);
/// Cloud cover: attenuates the direct beam, keeps a diffuse sky source.
Sun overcastSun(Sun sun, float overcast);
float presetHour(std::string_view name);
float fogOpticalDepth(float density, float falloff, float cameraHeight,
                      float targetHeight, float distance);
struct Cascades {
  std::array<glm::mat4, 4> matrices{};
  glm::vec4 splits{0}, bias{0};
  unsigned count = 0, resolution = 0;
};
struct CameraMatrices {
  glm::mat4 view{1}, projection{1};
};
Cascades makeCascades(const CameraMatrices &camera, glm::vec3 sun,
                      unsigned count, unsigned resolution, float distance);
} // namespace trackrs::world
