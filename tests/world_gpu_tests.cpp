// SPDX-License-Identifier: GPL-2.0-or-later
#include "engine/render/basic.h"
#include "engine/rhi/rhi.h"
#include "engine/world/renderer.h"
#include <chrono>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <thread>
namespace {
void require(bool test, const char *message) {
  if (!test)
    throw std::runtime_error(message);
}
} // namespace
nlohmann::json worldGpuTests(const std::filesystem::path &data,
                             const std::filesystem::path &output) {
  using namespace trackrs;
  const auto previous = world::state().settings;
  draw::initialize(data / "shaders/basic", false);
  world::configure({true, true, 12});
  auto view =
      glm::lookAt(glm::vec3(0, -12, 5), glm::vec3(0, 0, 1), glm::vec3(0, 0, 1));
  auto projection =
      glm::perspective(glm::radians(60.f), 16.f / 9.f, .1f, 2400.f);
  float dark = 1, light = 0;
  unsigned stars = 0;
  float moonCenter = 0;
  {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    do {
      world::prepare(view, projection, RenderQuality::Low, data);
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      require(std::chrono::steady_clock::now() < deadline,
              "procedural sky IBL not ready");
    } while (!world::state().sky);
    world::renderShadows([&](const glm::mat4 &matrix) {
      draw::SavedState saved;
      draw::matrix(draw::Projection);
      draw::identity();
      draw::multiply(glm::value_ptr(matrix));
      draw::matrix(draw::View);
      draw::identity();
      draw::disable(GL_CULL_FACE);
      draw::disable(GL_TEXTURE_2D);
      draw::disable(draw::AlphaTest);
      draw::color(1, 1, 1);
      // Closed box from z=.5 to2.5, above the receiver at z=0.
      for (unsigned axis = 0; axis < 3; ++axis)
        for (float sign : {-1.f, 1.f}) {
          auto normal = glm::vec3(0);
          normal[axis] = sign;
          draw::begin(draw::Quads);
          for (auto uv : {glm::vec2(-1, -1), glm::vec2(1, -1), glm::vec2(1, 1),
                          glm::vec2(-1, 1)}) {
            glm::vec3 p(0);
            p[axis] = sign;
            p[(axis + 1) % 3] = uv.x;
            p[(axis + 2) % 3] = uv.y;
            p.z += 1.5f;
            draw::normal(glm::value_ptr(normal));
            draw::vertex(p.x, p.y, p.z);
          }
          draw::end();
        }
      draw::flush();
    });
    GLuint fbo = 0, colour = 0, vao = 0;
    glGenFramebuffers(1, &fbo);
    glGenTextures(1, &colour);
    glGenVertexArrays(1, &vao);
    glBindTexture(GL_TEXTURE_2D, colour);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 1, 1, 0, GL_RGBA, GL_FLOAT,
                 nullptr);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           colour, 0);
    require(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
            "CSM readback FBO incomplete");
    rhi::ShaderProgram program(data / "shaders/pbr/cube.vert",
                               data / "shaders/world/visibility_test.frag");
    glUseProgram(program.id());
    world::apply(program.id(), glm::mat4(1), true);
    glBindVertexArray(vao);
    glViewport(0, 0, 1, 1);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    auto sample = [&](glm::vec3 point) {
      glUniform3fv(glGetUniformLocation(program.id(), "uTestPoint"), 1,
                   glm::value_ptr(point));
      glDrawArrays(GL_TRIANGLES, 0, 3);
      std::array<float, 4> value{};
      glReadPixels(0, 0, 1, 1, GL_RGBA, GL_FLOAT, value.data());
      return value[0];
    };
    dark = sample({0, 0, 0});
    light = sample({5, 5, 0});

    world::configure({true, true, 22});
    auto moon = glm::normalize(-world::state().sun.direction +
                               glm::vec3(.15f, 0, .18f));
    auto moonView = glm::lookAt(glm::vec3(0), moon, glm::vec3(0, 0, 1));
    auto moonProjection =
        glm::perspective(glm::radians(30.f), 16.f / 9.f, .1f, 2400.f);
    world::prepare(moonView, moonProjection, RenderQuality::Low, data);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glBindTexture(GL_TEXTURE_2D, colour);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 640, 360, 0, GL_RGBA, GL_FLOAT,
                 nullptr);
    glViewport(0, 0, 640, 360);
    world::drawSky(moonProjection, moonView, glm::vec3(0));
    std::vector<unsigned char> skyPixels(640 * 360 * 3);
    glReadPixels(0, 0, 640, 360, GL_RGB, GL_UNSIGNED_BYTE, skyPixels.data());
    for (int y = 0; y < 360; ++y)
      for (int x = 0; x < 640; ++x) {
        auto index = static_cast<std::size_t>((y * 640 + x) * 3);
        auto peak = std::max(
            {skyPixels[index], skyPixels[index + 1], skyPixels[index + 2]});
        if (std::abs(x - 320) > 12 || std::abs(y - 180) > 12) {
          if (peak > 100)
            ++stars;
        } else
          moonCenter = std::max(moonCenter, float(peak) / 255.f);
      }
    require(moonCenter > .4f, "procedural moon did not appear");
    require(stars > 5, "night sky has no resolved stars");
    if (!output.empty()) {
      std::filesystem::create_directories(output);
      std::ofstream image(output / "night-sky.ppm", std::ios::binary);
      image << "P6\n640 360\n255\n";
      for (int y = 359; y >= 0; --y)
        image.write(
            reinterpret_cast<const char *>(skyPixels.data() + y * 640 * 3),
            640 * 3);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &colour);
    glDeleteVertexArrays(1, &vao);
    require(dark < .2f, "CSM did not occlude receiver beneath caster");
    require(light > .8f, "CSM incorrectly shadowed unoccluded receiver");
  }
  auto result = world::report();
  result["known_occluded_visibility"] = dark;
  result["known_lit_visibility"] = light;
  result["resolved_star_pixels"] = stars;
  result["moon_peak_srgb"] = moonCenter;
  world::shutdown();
  world::configure(previous);
  draw::shutdown();
  glViewport(0, 0, 960, 640);
  return result;
}
