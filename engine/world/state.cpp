// SPDX-License-Identifier: GPL-2.0-or-later
#include "state.h"
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
namespace trackrs::world {
State &state() {
  static State value;
  return value;
}
void configure(Settings settings) {
  auto &s = state();
  settings.weather = weatherPreset(s.weatherName, s.levelWeather);
  s.settings = settings;
  s.sun = overcastSun(sunlight(settings.hour), settings.weather.overcast);
}
void selectWeather(const std::string &name) {
  if (!validWeatherName(name))
    throw std::invalid_argument("weather must be level, clear, rain, snow or fog");
  state().weatherName = name;
  configure(state().settings);
}
void setLevelWeather(const Weather &weather) {
  state().levelWeather = weather;
  configure(state().settings);
}
void apply(GLuint p, const glm::mat4 &localToWorld, bool scene) {
  auto &s = state();
  bool enabled = s.settings.enabled && scene;
  glUniform1i(glGetUniformLocation(p, "uSceneEnabled"), enabled);
  glUniform1i(glGetUniformLocation(p, "uDepthOnly"), s.depthPass);
  glUniform1i(glGetUniformLocation(p, "uSceneToneMap"), !s.linearCapture);
  glUniform1i(glGetUniformLocation(p, "uSceneShadow"), 10);
  glUniform1i(glGetUniformLocation(p, "uSceneShadowCount"), 0);
  if (!enabled)
    return;
  const auto &w = s.settings.weather;
  glUniform4f(glGetUniformLocation(p, "uSceneWeather"), w.wetness, w.puddles, w.snow, w.overcast);
  glUniform1f(glGetUniformLocation(p, "uSceneFogDensity"), w.fogDensity);
  glUniformMatrix4fv(glGetUniformLocation(p, "uSceneFromLocal"), 1, GL_FALSE,
                     glm::value_ptr(localToWorld));
  glUniformMatrix4fv(glGetUniformLocation(p, "uSceneView"), 1, GL_FALSE,
                     glm::value_ptr(s.shadowView));
  auto camera = glm::vec3(glm::inverse(s.view)[3]);
  glUniform3fv(glGetUniformLocation(p, "uSceneCamera"), 1,
               glm::value_ptr(camera));
  glUniform3fv(glGetUniformLocation(p, "uSolarDirection"), 1,
               glm::value_ptr(s.sun.direction));
  glUniform3fv(glGetUniformLocation(p, "uSolarRadiance"), 1,
               glm::value_ptr(s.sun.radiance));
  glUniform3fv(glGetUniformLocation(p, "uSceneAmbient"), 1,
               glm::value_ptr(s.sun.ambient));
  glUniform3fv(glGetUniformLocation(p, "uSolarSkyRadiance"), 1,
               glm::value_ptr(s.sun.skyRadiance));
  const bool shadow = s.settings.csm && s.shadow && !s.depthPass;
  glUniform1i(glGetUniformLocation(p, "uSceneShadowCount"),
              shadow ? static_cast<int>(s.cascades.count) : 0);
  if (shadow) {
    glUniformMatrix4fv(glGetUniformLocation(p, "uSceneLight[0]"), 4, GL_FALSE,
                       glm::value_ptr(s.cascades.matrices[0]));
    glUniform4fv(glGetUniformLocation(p, "uSceneSplits"), 1,
                 glm::value_ptr(s.cascades.splits));
    glUniform4fv(glGetUniformLocation(p, "uSceneBias"), 1,
                 glm::value_ptr(s.cascades.bias));
    glUniform1i(glGetUniformLocation(p, "uSceneShadow"), 10);
    GLint active = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
    glActiveTexture(GL_TEXTURE10);
    glBindTexture(GL_TEXTURE_2D_ARRAY, s.shadow);
    glActiveTexture(static_cast<GLenum>(active));
  }
}
} // namespace trackrs::world
