// SPDX-License-Identifier: GPL-2.0-or-later
#include "state.h"
#include <glm/gtc/type_ptr.hpp>
namespace trackrs::world {
State &state() {
  static State value;
  return value;
}
void configure(Settings settings) {
  state().settings = settings;
  state().sun = sunlight(settings.hour);
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
