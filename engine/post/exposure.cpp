// SPDX-License-Identifier: GPL-2.0-or-later
#include "exposure.h"
#include <algorithm>
#include <cmath>
namespace trackrs::post {
float targetExposure(float average, const ExposureSettings &s) {
  if (!(average > 0) || !std::isfinite(average))
    return 1.f;
  return std::clamp(s.key / average, s.minExposure, s.maxExposure);
}
float adaptLuminance(float previous, float target, float dt, const ExposureSettings &s) {
  if (!(previous > 0) || !std::isfinite(previous))
    return target; // first frame: no fade-in from black
  if (!(dt > 0) || !std::isfinite(dt))
    return previous;
  const float speed = target > previous ? s.speedUp : s.speedDown;
  return previous + (target - previous) * (1.f - std::exp(-dt * speed));
}
std::vector<glm::ivec2> bloomChain(glm::ivec2 frame, int levels) {
  std::vector<glm::ivec2> chain;
  glm::ivec2 size = frame;
  for (int i = 0; i < levels; ++i) {
    size = glm::max(size / 2, glm::ivec2(1));
    if (size.x < 8 || size.y < 8)
      break;
    chain.push_back(size);
  }
  return chain;
}
std::vector<glm::vec3> ssaoKernel(int count) {
  std::vector<glm::vec3> kernel;
  if (count <= 0)
    return kernel;
  kernel.reserve(static_cast<std::size_t>(count));
  constexpr float golden = 2.39996323f; // pi * (3 - sqrt(5))
  for (int i = 0; i < count; ++i) {
    const float t = (static_cast<float>(i) + .5f) / static_cast<float>(count);
    const float z = std::sqrt(1.f - t);             // cosine-weighted elevation
    const float r = std::sqrt(std::max(1.f - z * z, 0.f));
    const float a = golden * static_cast<float>(i);
    const float scale = .15f + .85f * t * t;        // dense near the centre
    kernel.emplace_back(r * std::cos(a) * scale, r * std::sin(a) * scale, std::max(z, .1f) * scale);
  }
  return kernel;
}
float bloomPrefilter(float brightness, float threshold, float knee) {
  // Quadratic soft knee (as in common dual-filter implementations).
  float soft = std::clamp(brightness - threshold + knee, 0.f, 2.f * knee);
  soft = soft * soft / (4.f * knee + 1e-5f);
  return std::max(soft, brightness - threshold) / std::max(brightness, 1e-5f);
}
float halton(unsigned index, unsigned base) {
  if (base < 2)
    return 0.f;
  float result = 0.f, fraction = 1.f;
  while (index > 0) {
    fraction /= static_cast<float>(base);
    result += fraction * static_cast<float>(index % base);
    index /= base;
  }
  return result;
}
glm::vec2 taaJitter(unsigned frame) {
  const unsigned i = frame % 8u + 1u;
  return {halton(i, 2) - .5f, halton(i, 3) - .5f};
}
glm::mat4 taaJitterMatrix(glm::vec2 pixels, glm::ivec2 size) {
  glm::mat4 m(1.f);
  if (size.x > 0 && size.y > 0) {
    // Applied after projection: x_clip += 2 * j / size * w_clip.
    m[3][0] = 2.f * pixels.x / static_cast<float>(size.x);
    m[3][1] = 2.f * pixels.y / static_cast<float>(size.y);
  }
  return m;
}
} // namespace trackrs::post
