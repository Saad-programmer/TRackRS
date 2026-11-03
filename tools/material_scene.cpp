// SPDX-License-Identifier: GPL-2.0-or-later
#include "engine/pbr/renderer.h"
#include <SDL2/SDL.h>
#include <chrono>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
namespace {
using namespace trackrs;
mesh::Texture texture(const std::filesystem::path &directory,
                      const std::string &name) {
  return {name, assets::readFile(directory / name)};
}
class Sky {
  rhi::ShaderProgram program_;
  GLuint vao_ = 0;

public:
  explicit Sky(const std::filesystem::path &shaders)
      : program_(
            shaders / "cube.vert", shaders / "background.frag", [](GLuint id) {
              return glGetUniformLocation(id, "uSky") >= 0 &&
                     glGetUniformLocation(id, "uInverseViewProjection") >= 0;
            }) {
    glGenVertexArrays(1, &vao_);
  }
  ~Sky() { glDeleteVertexArrays(1, &vao_); }
  void draw(const pbr::Frame &frame, const pbr::Ibl &environment) {
    program_.poll();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDisable(GL_FRAMEBUFFER_SRGB);
    glUseProgram(program_.id());
    glBindVertexArray(vao_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, environment.sky());
    glUniform1i(glGetUniformLocation(program_.id(), "uSky"), 0);
    auto inverse = glm::inverse(frame.viewProjection);
    glUniformMatrix4fv(
        glGetUniformLocation(program_.id(), "uInverseViewProjection"), 1,
        GL_FALSE, glm::value_ptr(inverse));
    glUniform3fv(glGetUniformLocation(program_.id(), "uCamera"), 1,
                 glm::value_ptr(frame.camera));
    glUniform2f(glGetUniformLocation(program_.id(), "uResolution"), 960, 640);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glUseProgram(0);
  }
};
} // namespace
nlohmann::json runMaterialScene(
    SDL_Window *window, const std::filesystem::path &data, unsigned frames,
    bool profiling,
    const std::function<trackrs::mesh::MeshData(const std::filesystem::path &)>
        &sphereFactory,
    const std::function<void()> &capture) {
  using namespace trackrs;
  auto scenePath = data / "scenes/material_preview";
  auto document =
      nlohmann::json::parse(std::ifstream(scenePath / "scene.json"));
  auto load = [&](const char *key) {
    return mesh::MeshImporter::load(
        (scenePath / document.at(key).get<std::string>())
            .lexically_normal()
            .string());
  };
  auto shaders = data / "shaders/pbr",
       materialPath = data / "vehicles/pbr_test";
  pbr::Model car(load("car"), shaders), asphalt(load("asphalt"), shaders);
  pbr::Ibl environment(scenePath / document.at("ibl").get<std::string>());
  pbr::ReflectionProbe probe(shaders, 32, .5);
  Sky sky(shaders);
  auto stream = pbr::textureStream();
  std::vector<std::unique_ptr<pbr::Model>> swatches;
  for (int row = 0; row < 2; ++row)
    for (float rough : document.at("roughness").get<std::vector<float>>()) {
      auto fixture = sphereFactory(materialPath);
      auto &material = fixture.materials.at(0);
      material.name = (row ? "metal" : "dielectric") +
                      std::string(" roughness ") + std::to_string(rough);
      material.metallic = float(row);
      material.roughness = rough;
      material.albedo = texture(materialPath, "white-albedo.ktx2");
      material.normal = texture(materialPath, "white-normal.ktx2");
      material.metalRough = texture(materialPath, "white-orm.ktx2");
      swatches.push_back(std::make_unique<pbr::Model>(fixture, shaders));
    }
  pbr::Frame frame;
  frame.headlightPositions = {
      {glm::vec3(-.55f, .45f, -1.8f), glm::vec3(.55f, .45f, -1.8f)}};
  bool running = true, ibl = true, local = true;
  unsigned total = 0, readyFrames = 0;
  auto start = std::chrono::steady_clock::now();
  pbr::Statistics stats;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT)
        running = false;
      if (event.type == SDL_KEYDOWN) {
        auto key = event.key.keysym.sym;
        if (key == SDLK_ESCAPE)
          running = false;
        if (key >= SDLK_0 && key <= SDLK_6)
          frame.debugView = key - SDLK_0;
        if (key >= SDLK_F1 && key <= SDLK_F7)
          frame.debugView = key - SDLK_F1;
        if (key == SDLK_h)
          frame.headlights = !frame.headlights;
        if (key == SDLK_i)
          ibl = !ibl;
        if (key == SDLK_p)
          local = !local;
      }
    }
    environment.pump();
    bool ready = environment.ready() && stream->idle();
    pbr::profileEnabled(profiling && ready && readyFrames >= 3);
    frame.environment = ibl ? &environment : nullptr;
    frame.camera = {5, 3, 7};
    frame.viewProjection =
        glm::perspective(glm::radians(45.f), 1.5f, .1f, 100.f) *
        glm::lookAt(frame.camera, glm::vec3(0, .5f, 0), glm::vec3(0, 1, 0));
    frame.sunDirection = {-3.5f, 3.2f, -5.5f};
    if (ready && local && ibl)
      probe.update(environment, {0, .3f, 0},
                   [&](const glm::mat4 &projection, const glm::mat4 &view,
                       const glm::vec3 &center) {
                     pbr::Frame captureFrame = frame;
                     captureFrame.camera = center;
                     captureFrame.viewProjection = projection * view;
                     captureFrame.reflectionProbe = 0;
                     captureFrame.headlights = false;
                     captureFrame.wetness = .8f;
                     captureFrame.toneMap = false;
                     asphalt.draw(captureFrame, {glm::mat4(1)});
                   });
    glViewport(0, 0, 960, 640);
    glDepthMask(GL_TRUE);
    glClearColor(.035f, .045f, .065f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (ibl)
      sky.draw(frame, environment);
    frame.reflectionProbe = local && ibl ? probe.texture() : 0;
    frame.probeMaxLod = probe.maxLod();
    stats = car.draw(frame, {glm::mat4(1)});
    auto floorFrame = frame;
    floorFrame.reflectionProbe = 0;
    floorFrame.headlights = false;
    floorFrame.wetness = .8f;
    auto floor = asphalt.draw(floorFrame, {glm::mat4(1)});
    stats.draws += floor.draws;
    stats.triangles += floor.triangles;
    auto swatchFrame = frame;
    swatchFrame.reflectionProbe = 0;
    swatchFrame.headlights = false;
    for (std::size_t i = 0; i < swatches.size(); ++i) {
      auto transform = glm::translate(
          glm::mat4(1), glm::vec3((float(i % 5) - 2) * 1.05f,
                                  2.2f + float(i / 5) * .9f, -2.5f));
      transform = glm::scale(transform, glm::vec3(.5f));
      auto sub = swatches[i]->draw(swatchFrame, {transform});
      stats.draws += sub.draws;
      stats.triangles += sub.triangles;
    }
    ++total;
    if (ready)
      ++readyFrames;
    if (frames && readyFrames >= frames + (profiling ? 3u : 0u) && total >= 4 &&
        (!profiling || !ibl || !local || probe.updates() >= 3)) {
      capture();
      running = false;
    }
    SDL_GL_SwapWindow(window);
    if (!frames)
      SDL_Delay(16);
    if (frames &&
        std::chrono::steady_clock::now() - start > std::chrono::seconds(120))
      throw std::runtime_error(
          "material preview did not finish loading/rendering");
  }
  glFinish();
  auto report = nlohmann::json{
      {"schema", "trackrs.phase2d-profile/1"},
      {"renderer", reinterpret_cast<const char *>(glGetString(GL_RENDERER))},
      {"frames", total},
      {"ready_frames", readyFrames},
      {"profiling_warmup_frames", profiling ? 3 : 0},
      {"measured_frames", profiling && readyFrames > 3 ? readyFrames - 3 : 0},
      {"draw_calls", stats.draws},
      {"triangles", stats.triangles},
      {"materials", pbr::materialReport()},
      {"probe", probe.report()},
      {"environment_texture_bytes", environment.bytes()},
      {"pbo_bytes", stream->pboBytes()},
      {"unique_streamed_texture_bytes", stream->residentBytes()},
      {"memory_scope",
       "logical texture/PBO/depth storage; per-material bindings include "
       "shared textures; driver padding excluded"}};
  pbr::profileEnabled(false);
  pbr::clearMeasurements();
  return report;
}
