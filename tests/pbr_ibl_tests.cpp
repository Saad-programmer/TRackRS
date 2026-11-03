// SPDX-License-Identifier: GPL-2.0-or-later
#include "engine/pbr/renderer.h"
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <thread>
namespace {
using namespace trackrs;
void require(bool v, const char *text) {
  if (!v)
    throw std::runtime_error(text);
}
void wait(pbr::TextureStream &stream) {
  auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
  while (!stream.idle()) {
    require(std::chrono::steady_clock::now() < deadline,
            "async upload timeout");
    stream.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}
mesh::MeshData furnace(const std::filesystem::path &data) {
  mesh::MeshData result;
  auto tex = [&](const char *file) {
    return mesh::Texture{file,
                         assets::readFile(data / "vehicles/pbr_test" / file)};
  };
  for (unsigned row = 0; row < 4; ++row)
    for (unsigned col = 0; col < 11; ++col) {
      mesh::Material m;
      m.path = mesh::MaterialPath::Pbr;
      m.name = "IBL furnace";
      m.albedo = tex("white.png");
      m.normal = tex("normal.png");
      m.metalRough = tex("white.png");
      m.roughness = col ? float(col) * .1f : .04f;
      m.metallic = float(row % 2);
      m.clearcoat = row >= 2 ? 1.f : 0.f;
      m.clearcoatRoughness = .04f;
      mesh::Submesh s;
      s.material = result.materials.size();
      result.materials.push_back(m);
      float left = -1 + 2 * float(col) / 11,
            right = -1 + 2 * float(col + 1) / 11,
            bottom = -1 + 2 * float(row) / 4, top = -1 + 2 * float(row + 1) / 4;
      for (auto p : {glm::vec2(left, bottom), glm::vec2(right, bottom),
                     glm::vec2(right, top), glm::vec2(left, top)}) {
        mesh::Vertex v;
        v.position[0] = p.x;
        v.position[1] = p.y;
        v.normal[2] = 1;
        v.uv[0] = p.x;
        v.uv[1] = p.y;
        s.vertices.push_back(v);
      }
      s.indices = {0, 1, 2, 0, 2, 3};
      mesh::computeTangents(s.vertices, s.indices);
      for (const auto &v : s.vertices)
        s.packed.push_back(mesh::packVertex(v));
      result.submeshes.push_back(std::move(s));
    }
  return result;
}
} // namespace
nlohmann::json iblIntegrationTests(const std::filesystem::path &data) {
  using namespace trackrs;
  nlohmann::json report;
  for (bool fallback : {false, true}) {
    pbr::TextureStream stream(fallback);
    auto pixels =
        std::vector<assets::Pixel>(64, assets::Pixel{.7f, .2f, .1f, 1});
    auto source = assets::writeKtx2(assets::bake2D(
        assets::Format::BC7Srgb, 8, 8, pixels, assets::Semantic::Albedo));
    GLuint canary = 0;
    glGenTextures(1, &canary);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, canary);
    auto ticket = stream.request(source, assets::Semantic::Albedo);
    require(!ticket->ready && ticket->complete == 0,
            "async request bypassed placeholder");
    wait(stream);
    require(ticket->ready && ticket->error.empty(), "async texture failed");
    GLint active = 0, bound = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &bound);
    require(active == GL_TEXTURE5 && bound == static_cast<GLint>(canary),
            "texture streaming leaked bindings");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ticket->id());
    GLint width = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    require(width == 8 && ticket->levels == 4, "full mips not uploaded");
    require(!fallback || !ticket->native, "CPU fallback was not exercised");
    auto invalid = source;
    invalid[40] = 1;
    auto rejected = stream.request(invalid, assets::Semantic::Albedo);
    wait(stream);
    require(!rejected->ready && !rejected->error.empty(),
            "async loader accepted missing mips");
    report[fallback ? "cpu_fallback" : "native_pbo"] = {
        {"passed", true},
        {"resident_bytes", ticket->residentBytes},
        {"pbo_bytes", stream.pboBytes()},
        {"uploads", stream.uploads()},
        {"placeholder_first", true}};
    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteTextures(1, &canary);
  }
  auto whitePath = data.parent_path() / "tests/fixtures/ibl_white";
  pbr::Ibl environment(whitePath);
  auto stream = pbr::textureStream();
  wait(*stream);
  environment.pump();
  require(environment.ready(), "white IBL cache not ready");
  {
    std::vector<float> cube(16 * 16 * 3);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, environment.irradiance());
    glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, GL_FLOAT,
                  cube.data());
    for (float v : cube)
      if (!std::isfinite(v) || std::abs(v - 1) > 1e-4f)
        throw std::runtime_error("white cube GPU sample=" + std::to_string(v));
    std::vector<float> lut(256 * 256 * 2);
    glBindTexture(GL_TEXTURE_2D, environment.brdf());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RG, GL_FLOAT, lut.data());
    for (float v : lut)
      require(std::isfinite(v), "GPU LUT contains NaN");
  }

  GLuint framebuffer = 0, color = 0;
  glGenFramebuffers(1, &framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
  glGenTextures(1, &color);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, color);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 110, 40, 0, GL_RGBA, GL_FLOAT,
               nullptr);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         color, 0);
  require(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
          "IBL furnace target incomplete");
  glViewport(0, 0, 110, 40);
  float maximum = 0, minimum = 2, roughMinimum = 2;
  {
    pbr::Model model(furnace(data), data / "shaders/pbr");
    pbr::Frame frame;
    frame.environment = &environment;
    frame.headlights = false;
    frame.toneMap = false;
    frame.sunRadiance = {0, 0, 0};
    frame.ambient = {0, 0, 0};
    frame.viewProjection =
        glm::ortho(-1.f, 1.f, -1.f, 1.f, .1f, 10.f) *
        glm::lookAt(glm::vec3(0, 0, 3), glm::vec3(0), glm::vec3(0, 1, 0));
    for (auto camera : {glm::vec3(0, 0, 3), glm::vec3(3, 0, 2)}) {
      frame.camera = camera;
      glClear(GL_COLOR_BUFFER_BIT);
      model.draw(frame, {glm::mat4(1)});
      for (unsigned row = 0; row < 4; ++row)
        for (unsigned col = 0; col < 11; ++col) {
          std::array<float, 4> pixel{};
          glReadPixels(static_cast<GLint>(col * 10 + 5),
                       static_cast<GLint>(row * 10 + 5), 1, 1, GL_RGBA,
                       GL_FLOAT, pixel.data());
          for (unsigned c = 0; c < 3; ++c) {
            if (!std::isfinite(pixel[c]) || pixel[c] > 1.0001f ||
                pixel[c] < .95f)
              throw std::runtime_error(
                  "IBL furnace row=" + std::to_string(row) + " col=" +
                  std::to_string(col) + " channel=" + std::to_string(c) +
                  " energy=" + std::to_string(pixel[c]));
            maximum = std::max(maximum, pixel[c]);
            minimum = std::min(minimum, pixel[c]);
            if (col == 10)
              roughMinimum = std::min(roughMinimum, pixel[c]);
          }
        }
    }
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glDeleteFramebuffers(1, &framebuffer);
  glDeleteTextures(1, &color);
  glViewport(0, 0, 960, 640);
  report["ibl_white_furnace"] = {{"roughness", {.04, 1}},
                                 {"metal", {0, 1}},
                                 {"clearcoat", {0, 1}},
                                 {"view_angles", 2},
                                 {"max_energy", maximum},
                                 {"min_energy", minimum},
                                 {"roughness_one_min", roughMinimum},
                                 {"loss_limit", .05},
                                 {"linear_hdr_before_tonemap", true}};
  {
    auto fixture = mesh::MeshImporter::load(
        (data / "vehicles/pbr_test/car.gltf").string());
    pbr::Model car(fixture, data / "shaders/pbr");
    pbr::Frame frame;
    frame.headlights = false;
    auto single = car.draw(frame, {glm::mat4(1)});
    auto instances = car.draw(frame, {glm::mat4(1), glm::mat4(1)}, true, true);
    require(car.canInstance(),
            "unused glTF default material must not reject instance adapter");
    require(single.draws == 4 && single.triangles == 144,
            "car must submit one draw per material");
    require(instances.draws == 4 && instances.triangles == 288,
            "car instances must share material draws");
    report["material_batching"] = {{"materials", 4},
                                   {"single_draws", single.draws},
                                   {"instance_draws", instances.draws},
                                   {"instance_triangles", instances.triangles}};
  }
  double clock = 0;
  {
    pbr::ReflectionProbe probe(data / "shaders/pbr", 16, .5,
                               [&] { return clock; });
    require(probe.update(environment, {0, 0, 0}),
            "initial probe update missing");
    clock = .249;
    require(!probe.update(environment, {0, 0, 0}), "probe exceeded 4Hz");
    clock = .499;
    require(!probe.update(environment, {0, 0, 0}),
            "probe exceeded configured 2Hz");
    clock = .5;
    require(probe.update(environment, {0, 0, 0}), "probe did not refresh");
    glFinish();
    report["probe"] = probe.report();
    report["probe"]["clock"] = "synthetic rate-limit test";
  }
  pbr::clearMeasurements();
  return report;
}
