// SPDX-License-Identifier: GPL-2.0-or-later
#include "engine/render/basic.h"
#include "validation.h"
#include <array>
#include <cmath>
#include <stdexcept>
namespace validation {
Run run;
}
namespace {
void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
std::array<unsigned char, 4> pixel(int x, int y) {
  std::array<unsigned char, 4> p{};
  glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, p.data());
  return p;
}
void quad(float left, float right) {
  using namespace trackrs::draw;
  begin(Quads);
  vertex(left, -1);
  vertex(right, -1);
  vertex(right, 1);
  vertex(left, 1);
  end();
}
} // namespace
void coreRenderTests(const std::filesystem::path &shaders) {
  using namespace trackrs::draw;
  initialize(shaders, false);
  try {
    viewport(0, 0, 64, 64);
    disable(GL_DEPTH_TEST);
    disable(GL_CULL_FACE);
    disable(GL_BLEND);
    validation::run.capture = true;
    validation::run.frameDraws = 0;
    validation::run.frameTriangles = 0;
    glClearColor(0, 0, 0, 1);
    clear(GL_COLOR_BUFFER_BIT);
    color(1, 0, 0);
    quad(-1, 0);
    color(0, 1, 0);
    push();
    translate(1, 0, 0);
    quad(-1, 0);
    pop();
    require(validation::run.frameDraws == 0,
            "UI geometry submitted before batch flush");
    flush();
    require(validation::run.frameDraws == 1 &&
                validation::run.frameTriangles == 4,
            "UI transforms/colors failed to batch");
    require(pixel(16, 32)[0] == 255 && pixel(48, 32)[1] == 255,
            "UI batch transform/color pixels changed");
    std::vector<MeshVertex> vertices = {
        {{-1, -1, 0}, {0, 0, 1}, {0, 0}}, {{1, -1, 0}, {0, 0, 1}, {1, 0}},
        {{1, 1, 0}, {0, 0, 1}, {1, 1}},   {{-1, -1, 0}, {0, 0, 1}, {0, 0}},
        {{1, 1, 0}, {0, 0, 1}, {1, 1}},   {{-1, 1, 0}, {0, 0, 1}, {0, 1}}};
    {
      Mesh mesh(vertices);
      color(0, 0, 1);
      mesh.draw();
      require(pixel(32, 32)[2] == 255, "retained VAO mesh failed");
      matrix(Projection);
      ortho(-1, 1, -1, 1, 0, 10);
      matrix(View);
      translate(0, 0, -1);
      const float red[4] = {1, 0, 0, 1};
      fog(FogColor, red);
      fog(FogDensity, .5f);
      enable(Fog);
      color(0, 1, 0);
      mesh.draw();
      const auto fogged = pixel(32, 32);
      require(std::abs(int(fogged[0]) - 100) <= 2 &&
                  std::abs(int(fogged[1]) - 155) <= 2,
              "exponential fog pixels incorrect");
      disable(Fog);
      const float white[4] = {1, 1, 1, 1}, black[4] = {0, 0, 0, 1},
                  direction[4] = {0, 0, 1, 0};
      material(MaterialColor, white);
      material(Specular, black);
      lightModel(GlobalAmbient, black);
      light(Diffuse, white);
      light(LightPosition, direction);
      enable(Lighting);
      mesh.draw();
      const auto lit = pixel(32, 32);
      require(lit[0] == 255 && lit[1] == 255 && lit[2] == 255,
              "basic directional lighting incorrect");
      disable(Lighting);
    }
    matrix(Projection);
    identity();
    matrix(View);
    identity();
    std::vector<Particle> particlesToDraw = {
        {{-.5f, 0, 0}, {.4f, 0, 0}, {0, .8f, 0}, {1, 0, 0, 1}, {1, 0, 0, 1}},
        {{.5f, 0, 0}, {.4f, 0, 0}, {0, .8f, 0}, {0, 1, 0, 1}, {0, 1, 0, 1}}};
    const auto draws = validation::run.frameDraws,
               triangles = validation::run.frameTriangles;
    for (int i = 0; i < 10; ++i) {
      clear(GL_COLOR_BUFFER_BIT);
      particles(particlesToDraw);
    }
    require(validation::run.frameDraws == draws + 10 &&
                validation::run.frameTriangles == triangles + 40,
            "particle instancing/ring-wrap counts incorrect");
    require(pixel(16, 32)[0] == 255 && pixel(48, 32)[1] == 255,
            "instanced particle pixels incorrect");
    std::vector<unsigned char> atlas(9 * 9 * 4, 255);
    for (int y = 0; y < 9; ++y)
      for (int x = 0; x < 9; ++x)
        atlas[(y * 9 + x) * 4 + 3] =
            (x >= 3 && x <= 5 && y >= 3 && y <= 5) ? 255 : 0;
    makeSdf(atlas.data(), 9, 9, 4);
    require(atlas[(4 * 9 + 4) * 4 + 3] > 128 && atlas[3] < 128,
            "SDF sign/distance generation incorrect");
    GLuint texture = 0;
    glGenTextures(1, &texture);
    bindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 9, 9, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 atlas.data());
    registerSdf(texture);
    enable(GL_TEXTURE_2D);
    color(1, 1, 1);
    enable(GL_BLEND);
    blend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    clear(GL_COLOR_BUFFER_BIT);
    begin(Quads);
    uv(0, 0);
    vertex(-1, -1);
    uv(1, 0);
    vertex(1, -1);
    uv(1, 1);
    vertex(1, 1);
    uv(0, 1);
    vertex(-1, 1);
    end();
    flush();
    require(pixel(32, 32)[0] > 250 && pixel(2, 2)[0] < 5,
            "SDF shader did not reconstruct coverage");
    deleteTextures(1, &texture);
    validation::run.capture = false;
    shutdown();
  } catch (...) {
    validation::run.capture = false;
    shutdown();
    throw;
  }
}
