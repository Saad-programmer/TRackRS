// SPDX-License-Identifier: GPL-2.0-or-later
#include "engine/terrain/renderer.h"
#include "engine/render/basic.h"
#include <chrono>
#include <cmath>
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <thread>
namespace {
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
} // namespace
nlohmann::json terrainGpuTests(const std::filesystem::path &data) {
  using namespace trackrs;
  struct BasicScope { BasicScope(const std::filesystem::path &root){draw::initialize(root / "shaders/basic",false);} ~BasicScope(){draw::shutdown();} } basic(data);
  auto previous = terrain::settings();
  terrain::configure({true, RenderQuality::Low});
  std::vector<float> height(256);
  std::vector<std::uint8_t> types(256);
  for (unsigned y = 0; y < 16; ++y)
    for (unsigned x = 0; x < 16; ++x) {
      height[y * 16 + x] =
          2.f * std::sin(float(x) * glm::two_pi<float>() / 16.f) +
          .5f * std::cos(float(y) * glm::two_pi<float>() / 16.f);
      types[y * 16 + x] = static_cast<std::uint8_t>(x % 9);
    }
  GLuint colour = 0, framebuffer = 0;
  glGenTextures(1, &colour);
  glBindTexture(GL_TEXTURE_2D, colour);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 16, 16, 0, GL_RGBA, GL_FLOAT,
               nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glGenFramebuffers(1, &framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         colour, 0);
  require(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
          "terrain test FBO incomplete");
  float maximumError = 0;
  {
    terrain::Renderer renderer(height, 16, 1, types, 16, data);
    auto view =
        glm::lookAt(glm::vec3(0, 0, 50), glm::vec3(0), glm::vec3(0, 1, 0));
    auto projection = glm::ortho(-8.5f, 7.5f, -8.5f, 7.5f, .1f, 200.f);
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    for (;;) {
      glViewport(0, 0, 16, 16);
      glClear(GL_COLOR_BUFFER_BIT);
      renderer.draw({0, 0, 50}, projection * view, false, 7);
      if (renderer.report().at("ready").get<bool>())
        break;
      require(std::chrono::steady_clock::now() < deadline,
              "terrain surface upload timeout");
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    std::array<float, 16 * 16 * 4> pixels{};
    glReadPixels(0, 0, 16, 16, GL_RGBA, GL_FLOAT, pixels.data());
    auto sample = [&](int x, int y) {
      return height[std::size_t((y % 16 + 16) % 16) * 16 +
                    static_cast<unsigned>((x % 16 + 16) % 16)];
    };
    for (int y = 0; y < 16; ++y)
      for (int x = 0; x < 16; ++x) {
        int wx = x - 8, wy = y - 8;
        auto normal = glm::normalize(
            glm::vec3(-(sample(wx + 1, wy) - sample(wx - 1, wy)),
                      -(sample(wx, wy + 1) - sample(wx, wy - 1)), 2));
        for (unsigned c = 0; c < 3; ++c) {
          float value = pixels[std::size_t(y * 16 + x) * 4 + c];
          require(std::isfinite(value), "nonfinite GPU height normal");
          maximumError =
              std::max(maximumError, std::abs(value - (normal[c] * .5f + .5f)));
        }
      }
    require(maximumError < .002f,
            "GPU height normals differ from periodic CPU source derivatives");
    require(renderer.report().at("nodes_culled").get<unsigned>() > 0,
            "terrain frustum did not cull nodes");
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glDeleteFramebuffers(1, &framebuffer);
  glDeleteTextures(1, &colour);
  glViewport(0, 0, 960, 640);
  terrain::configure(previous);
  return {{"gpu_periodic_height_normals_max_error", maximumError},
          {"negative_world_coordinates", true},
          {"shared_cpu_height_source", true},
          {"frustum_culling", "pass"}};
}
