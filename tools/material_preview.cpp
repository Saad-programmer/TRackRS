// SPDX-License-Identifier: GPL-2.0-or-later
#include "engine/pbr/renderer.h"
#include "engine/render/basic.h"
#include "engine/rhi/rhi.h"
#include "validation.h"
#include <SDL2/SDL_image.h>
#include <algorithm>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <nlohmann/json.hpp>
nlohmann::json worldGpuTests(const std::filesystem::path &data,const std::filesystem::path &output);
nlohmann::json terrainGpuTests(const std::filesystem::path &data);
nlohmann::json iblIntegrationTests(const std::filesystem::path &data);
nlohmann::json runMaterialScene(
    SDL_Window *window, const std::filesystem::path &data, unsigned frames,
    bool profiling, trackrs::RenderQuality quality,
    const std::function<trackrs::mesh::MeshData(const std::filesystem::path &)>
        &sphereFactory,
    const std::function<void()> &capture);
namespace validation {
Run run;
}
namespace {
using namespace trackrs;
void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
mesh::Bytes read(const std::filesystem::path &path) {
  std::ifstream file(path, std::ios::binary);
  require(bool(file), "missing fixture image");
  return mesh::Bytes(std::istreambuf_iterator<char>(file), {});
}
mesh::MeshData panel(const std::filesystem::path &assets,
                     bool mirrored = false) {
  mesh::MeshData data;
  mesh::Material m;
  m.path = mesh::MaterialPath::Pbr;
  m.albedo = {"white", read(assets / "white.png")};
  m.normal = {"tilted", read(assets / "tilted-normal.png")};
  m.metalRough = {"orm", read(assets / "paint-orm.png")};
  data.materials.push_back(m);
  mesh::Submesh sub;
  for (int side = 0; side < 2; ++side) {
    float left = side == 0 ? -.9f : .1f;
    float positions[4][2] = {
        {left, -.7f}, {left + .8f, -.7f}, {left + .8f, .7f}, {left, .7f}};
    for (int i = 0; i < 4; ++i) {
      mesh::Vertex v;
      v.position[0] = positions[i][0];
      v.position[1] = positions[i][1];
      v.normal[2] = 1;
      v.uv[0] = (i == 1 || i == 2) ? 1.f : 0.f;
      if (mirrored && side)
        v.uv[0] = -v.uv[0];
      v.uv[1] = i >= 2 ? 1.f : 0.f;
      sub.vertices.push_back(v);
    }
    for (unsigned index : {0u, 1u, 2u, 0u, 2u, 3u})
      sub.indices.push_back(static_cast<unsigned>(side * 4) + index);
  }
  mesh::computeTangents(sub.vertices, sub.indices);
  for (const auto &v : sub.vertices)
    sub.packed.push_back(mesh::packVertex(v));
  data.submeshes.push_back(std::move(sub));
  return data;
}
mesh::MeshData sphere(const std::filesystem::path &assets,
                      bool mirrored = false) {
  auto data = panel(assets);
  data.submeshes.clear();
  mesh::Submesh sub;
  constexpr unsigned rings = 16, sectors = 32;
  for (unsigned r = 0; r <= rings; ++r)
    for (unsigned sector = 0; sector <= sectors; ++sector) {
      float theta = glm::pi<float>() * static_cast<float>(r) / rings,
            phi = 2 * glm::pi<float>() * static_cast<float>(sector) / sectors;
      glm::vec3 p(std::sin(theta) * std::cos(phi), std::cos(theta),
                  std::sin(theta) * std::sin(phi));
      mesh::Vertex v;
      std::copy_n(glm::value_ptr(p), 3, v.position);
      std::copy_n(glm::value_ptr(p), 3, v.normal);
      v.uv[0] = static_cast<float>(sector) / sectors;
      if (mirrored)
        v.uv[0] = std::abs(v.uv[0] - .25f);
      v.uv[1] = static_cast<float>(r) / rings;
      sub.vertices.push_back(v);
    }
  for (unsigned r = 0; r < rings; ++r)
    for (unsigned sector = 0; sector < sectors; ++sector) {
      auto a = r * (sectors + 1) + sector, b = a + sectors + 1, c = b + 1,
           d = a + 1;
      if (r + 1 < rings)
        for (auto i : {a, c, b})
          sub.indices.push_back(i);
      if (r > 0)
        for (auto i : {a, d, c})
          sub.indices.push_back(i);
    }
  mesh::computeTangents(sub.vertices, sub.indices);
  for (const auto &v : sub.vertices)
    sub.packed.push_back(mesh::packVertex(v));
  data.submeshes.push_back(std::move(sub));
  return data;
}
std::array<unsigned char, 4> pixel(int x, int y) {
  std::array<unsigned char, 4> p{};
  glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, p.data());
  return p;
}
void save(const std::filesystem::path &file, int width, int height) {
  std::vector<unsigned char> pixels(
      static_cast<std::size_t>(width * height * 4)),
      row(static_cast<std::size_t>(width * 4));
  glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
  for (int y = 0; y < height / 2; ++y) {
    auto *a = pixels.data() + y * width * 4;
    auto *b = pixels.data() + (height - 1 - y) * width * 4;
    std::copy_n(a, width * 4, row.data());
    std::copy_n(b, width * 4, a);
    std::copy_n(row.data(), width * 4, b);
  }
  auto *surface = SDL_CreateRGBSurfaceWithFormatFrom(
      pixels.data(), width, height, 32, width * 4, SDL_PIXELFORMAT_RGBA32);
  require(surface, "PNG surface failed");
  int status = IMG_SavePNG(surface, file.string().c_str());
  SDL_FreeSurface(surface);
  require(status == 0, "PNG write failed");
}
nlohmann::json probes(const std::filesystem::path &data) {
  nlohmann::json report;
  auto shaders = data / "shaders/pbr";
  rhi::DebugMessages debug;
  debug.install();
  for (unsigned flags = 0; flags < 16; ++flags) {
    std::vector<std::string> defines;
    if (flags & 1)
      defines.push_back("EXPLICIT_BITANGENT");
    if (flags & 2)
      defines.push_back("INSTANCED");
    if (flags & 4)
      defines.push_back("USE_CSM");
    if (flags & 8)
      defines.push_back("ALPHA_TO_COVERAGE");
    rhi::ShaderProgram p(shaders / "pbr.vert", shaders / "pbr.frag", {},
                         defines);
  }
  report["driver_shader_permutations"] = 16;
  GLuint fbo = 0, texture = 0, vao = 0;
  glGenFramebuffers(1, &fbo);
  glGenTextures(1, &texture);
  glGenVertexArrays(1, &vao);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 11, 4, 0, GL_RGBA, GL_FLOAT,
               nullptr);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         texture, 0);
  require(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
          "furnace FBO incomplete");
  {
    rhi::ShaderProgram p(shaders / "furnace.vert", shaders / "furnace.frag");
    glUseProgram(p.id());
    glBindVertexArray(vao);
    glViewport(0, 0, 11, 4);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_FRAMEBUFFER_SRGB);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    std::array<float, 176> values{};
    glReadPixels(0, 0, 11, 4, GL_RGBA, GL_FLOAT, values.data());
    float maximum = 0, minimum = 1, reciprocity = 0;
    for (std::size_t i = 0; i < 44; ++i) {
      float energy = values[i * 4];
      reciprocity = std::max(reciprocity, values[i * 4 + 3]);
      require(std::isfinite(energy) && energy >= 0 && energy <= 1.0001f,
              "white furnace generated energy");
      maximum = std::max(maximum, energy);
      minimum = std::min(minimum, energy);
    }
    require(maximum > .95f, "white furnace unexpectedly dark");
    report["white_furnace"] = {
        {"samples_per_cell", 4096},
        {"roughness_min", .04},
        {"roughness_max", 1},
        {"maximum_energy", maximum},
        {"minimum_energy", minimum},
        {"upper_bound", 1.0001},
        {"scope", "direct Appendix A BRDF; no Phase 2d IBL"}};
    require(reciprocity <= 1e-5f, "BRDF reciprocity failed");
    report["reciprocity"] = {{"maximum_relative_error", reciprocity},
                             {"upper_bound", 1e-5}};
    glUseProgram(0);
  }
  glUseProgram(0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glDeleteFramebuffers(1, &fbo);
  glDeleteTextures(1, &texture);
  glDeleteVertexArrays(1, &vao);
  glViewport(0, 0, 960, 640);
  {
    pbr::Model model(panel(data / "vehicles/pbr_test", true), shaders);
    pbr::Frame frame;
    frame.viewProjection =
        glm::ortho(-1.f, 1.f, -1.f, 1.f, .1f, 10.f) *
        glm::lookAt(glm::vec3(0, 0, 3), glm::vec3(0), glm::vec3(0, 1, 0));
    frame.camera = {0, 0, 3};
    frame.debugView = 2;
    frame.headlights = false;
    std::array<unsigned char, 4> reference{};
    int worst = 0;
    for (bool packed : {false, true})
      for (bool instanced : {false, true}) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        auto stats = model.draw(frame, {glm::mat4(1)}, packed, instanced);
        require(stats.draws == 1 && stats.triangles == 4,
                "indexed/instanced submission count incorrect");
        auto a = pixel(240, 320), b = pixel(720, 320);
        for (int c = 0; c < 3; ++c) {
          require(std::abs(int(a[c]) - int(b[c])) <= 1,
                  "mirrored UV handedness seam");
          if (!packed && !instanced)
            reference = a;
          worst = std::max(worst, std::abs(int(a[c]) - int(reference[c])));
        }
        require(std::abs(int(a[1]) - 200) <= 2 && a[2] > 225,
                "normal map Y convention incorrect");
      }
    require(worst <= 1, "packed and explicit normal basis disagree");
    report["normal_handedness"] = {{"mirrored_uv", "pass"},
                                   {"layouts", {56, 32}},
                                   {"instancing", "pass"},
                                   {"max_channel_error", worst}};
    auto left = glm::scale(glm::translate(glm::mat4(1), glm::vec3(-.5f, 0, 0)),
                           glm::vec3(.4f));
    auto right = glm::scale(glm::translate(glm::mat4(1), glm::vec3(.5f, 0, 0)),
                            glm::vec3(.4f));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    auto batch = model.draw(frame, {left, right}, true, true);
    require(batch.draws == 1 && batch.triangles == 8,
            "two-instance batch counts wrong");
    require(pixel(144, 320)[1] > 195 && pixel(816, 320)[1] > 195,
            "instance matrices not applied");
    glm::mat4 transformed =
        glm::scale(glm::mat4(1), glm::vec3(-1.1f, .7f, 1.8f));
    transformed[0][2] = .4f;
    transformed =
        glm::rotate(glm::mat4(1), .3f, glm::vec3(0, 0, 1)) * transformed;
    auto normal =
        glm::normalize(glm::transpose(glm::inverse(glm::mat3(transformed))) *
                       glm::vec3(0, 0, 1));
    auto tangent = glm::mat3(transformed) * glm::vec3(1, 0, 0);
    tangent = glm::normalize(tangent - normal * glm::dot(normal, tangent));
    auto bitangent = -glm::cross(normal, tangent);
    float nx = 128.f / 255.f * 2 - 1, ny = 200.f / 255.f * 2 - 1;
    auto expected = glm::normalize(tangent * nx + bitangent * ny +
                                   normal * std::sqrt(1 - nx * nx - ny * ny));
    auto clip = frame.viewProjection * transformed * glm::vec4(-.5f, 0, 0, 1);
    int sampleX = static_cast<int>((clip.x / clip.w * .5f + .5f) * 960),
        sampleY = static_cast<int>((clip.y / clip.w * .5f + .5f) * 640);
    for (bool packed : {false, true})
      for (bool instanced : {false, true}) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        model.draw(frame, {transformed}, packed, instanced);
        auto actual = pixel(sampleX, sampleY);
        for (int c = 0; c < 3; ++c)
          require(std::abs(int(actual[c]) -
                           static_cast<int>((expected[c] * .5f + .5f) * 255)) <=
                      2,
                  "mirrored/nonuniform model basis incorrect");
      }
    {
      pbr::Model mirroredSphere(sphere(data / "vehicles/pbr_test", true),
                                shaders);
      for (bool packed : {false, true}) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        mirroredSphere.draw(frame, {glm::mat4(1)}, packed);
        auto a = pixel(475, 320), b = pixel(485, 320);
        require(a[1] < 70 && b[1] < 70 && a[2] > 225 && b[2] > 225,
                "mirrored sphere normal convention wrong");
        require(std::abs(int(a[1]) - int(b[1])) <= 1 &&
                    std::abs(int(a[2]) - int(b[2])) <= 1,
                "mirrored sphere tangent seam discontinuous");
      }
      report["normal_handedness"]["mirrored_sphere"] = "pass";
    }
    report["normal_handedness"]["negative_scale_and_shear"] = "pass";
    report["normal_handedness"]["two_instances"] = "pass";
    frame.debugView = 0;
    frame.sunRadiance = {0, 0, 0};
    frame.ambient = {0, 0, 0};
    frame.lights = {{{0, 1, 2}, {10, 10, 10}, 10}};
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    model.draw(frame, {glm::mat4(1)});
    auto lit = pixel(240, 320);
    frame.lights.clear();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    model.draw(frame, {glm::mat4(1)});
    auto dark = pixel(240, 320);
    require(lit[0] > dark[0] + 20, "point light did not illuminate material");
    report["point_lights"] = "pass";
    // A synthetic comparison map proves the CSM interface without a cascade
    // baker.
    frame.sunRadiance = {3, 3, 3};
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    model.draw(frame, {glm::mat4(1)});
    auto sun = pixel(240, 320);
    GLuint shadow = 0;
    glGenTextures(1, &shadow);
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D_ARRAY, shadow);
    std::array<float, 4> depths{};
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT24, 2, 2, 1, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, depths.data());
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE,
                    GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    frame.shadows.texture = shadow;
    frame.shadows.count = 1;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    model.draw(frame, {glm::mat4(1)});
    auto shade = pixel(240, 320);
    require(sun[0] > shade[0] + 20, "CSM comparison hook did not shadow sun");
    glDeleteTextures(1, &shadow);
    report["CSM_interface"] =
        "synthetic depth comparison passed; no cascade generation";
  }
  {
    GLuint ms = 0, resolve = 0, color = 0, depth = 0, texture = 0;
    glGenFramebuffers(1, &ms);
    glBindFramebuffer(GL_FRAMEBUFFER, ms);
    glGenRenderbuffers(1, &color);
    glBindRenderbuffer(GL_RENDERBUFFER, color);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_RGBA8, 960, 640);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_RENDERBUFFER, color);
    glGenRenderbuffers(1, &depth);
    glBindRenderbuffer(GL_RENDERBUFFER, depth);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT24,
                                     960, 640);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, depth);
    require(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
            "MSAA probe incomplete");
    glGenFramebuffers(1, &resolve);
    glBindFramebuffer(GL_FRAMEBUFFER, resolve);
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 960, 640, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           texture, 0);
    require(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
            "resolve probe incomplete");
    auto fixture = panel(data / "vehicles/pbr_test");
    fixture.materials[0].alpha = mesh::AlphaMode::Mask;
    fixture.materials[0].alphaCutoff = .5f;
    fixture.materials[0].baseColor[3] = .5f;
    pbr::Model model(fixture, shaders);
    pbr::Frame frame;
    frame.viewProjection =
        glm::ortho(-1.f, 1.f, -1.f, 1.f, .1f, 10.f) *
        glm::lookAt(glm::vec3(0, 0, 3), glm::vec3(0), glm::vec3(0, 1, 0));
    frame.camera = {0, 0, 3};
    frame.debugView = 2;
    frame.headlights = false;
    glBindFramebuffer(GL_FRAMEBUFFER, ms);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    model.draw(frame, {glm::mat4(1)});
    glBindFramebuffer(GL_READ_FRAMEBUFFER, ms);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolve);
    glBlitFramebuffer(0, 0, 960, 640, 0, 0, 960, 640, GL_COLOR_BUFFER_BIT,
                      GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, resolve);
    auto coverage = pixel(240, 320);
    require(coverage[0] >= 50 && coverage[0] <= 80,
            "alpha to coverage did not retain half the MSAA samples");
    report["alpha_to_coverage"] = {
        {"samples", 4}, {"resolved_red", coverage[0]}, {"pass", true}};
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &ms);
    glDeleteFramebuffers(1, &resolve);
    glDeleteRenderbuffers(1, &color);
    glDeleteRenderbuffers(1, &depth);
    glDeleteTextures(1, &texture);
  }
  debug.check();
  report["phase2d"] = iblIntegrationTests(data);
  debug.check();
  report["KHR_debug_errors"] = debug.errors.load();
  return report;
}
} // namespace
int main(int argc, char **argv) {
  SDL_Window *window = nullptr;
  SDL_GLContext context = nullptr;
  int result = 0;
  try {
    std::filesystem::path data = TRACKRS_PREVIEW_DATA, output, reportPath;
    if (!std::filesystem::exists(data / "shaders/pbr/pbr.vert"))
      data = std::filesystem::absolute(argv[0]).parent_path() /
             "../share/games/trackrs";
    unsigned frames = 0;
    bool test = false;
    auto quality = RenderQuality::Medium;
    for (int i = 1; i < argc; ++i) {
      std::string arg = argv[i];
      if (arg == "--data-dir" && i + 1 < argc)
        data = argv[++i];
      else if (arg == "--output" && i + 1 < argc)
        output = argv[++i];
      else if (arg == "--report" && i + 1 < argc)
        reportPath = argv[++i];
      else if (arg == "--frames" && i + 1 < argc)
        frames = static_cast<unsigned>(std::stoul(argv[++i]));
      else if (arg == "--quality" && i + 1 < argc)
        quality = parseQuality(argv[++i]);
      else if (arg == "--test")
        test = true;
      else if (arg == "--help") {
        std::cout << "material_preview [--data-dir data] [--frames N --output "
                     "image.png] [--quality low|medium|high|ultra] [--test "
                     "--report results.json]\n0-6 debug "
                     "views; H headlights, I IBL, P probe; Esc quits\n";
        return 0;
      } else
        throw std::runtime_error("unknown material preview option: " + arg);
    }
    SDL_SetMainReady();
    require(SDL_Init(SDL_INIT_VIDEO) == 0, SDL_GetError());
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    window = SDL_CreateWindow(
        "TrackRS PBR material preview | 0-6 views | H headlights", 0, 0, 960,
        640,
        SDL_WINDOW_OPENGL |
            (frames || test ? SDL_WINDOW_HIDDEN : SDL_WINDOW_SHOWN));
    require(window, SDL_GetError());
    context = SDL_GL_CreateContext(window);
    require(context, SDL_GetError());
    require(gladLoadGL(reinterpret_cast<GLADloadfunc>(SDL_GL_GetProcAddress)) !=
                0,
            "glad failed");
    SDL_GL_SetSwapInterval(0);
    {
      rhi::DebugMessages debug;
      debug.install();
      if (test) {
        auto report = probes(data);
        report["terrain"] = terrainGpuTests(data);
        report["world"] = worldGpuTests(data,reportPath.parent_path());
        debug.install();
        if (!reportPath.empty())
          std::ofstream(reportPath) << report.dump(2) << '\n';
        std::cout << report.dump(2) << '\n';
      } else {
        auto report = runMaterialScene(
            window, data, frames, !reportPath.empty(), quality,
            [](const std::filesystem::path &path) { return sphere(path); },
            [&] {
              if (!output.empty())
                save(output, 960, 640);
            });
        if (!reportPath.empty())
          std::ofstream(reportPath) << report.dump(2) << '\n';
        std::cout << "Preview draws=" << report["draw_calls"]
                  << " triangles=" << report["triangles"] << '\n';
      }
      debug.check();
    }
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    result = 1;
  }
  if (context) {
    pbr::shutdown();
    glDebugMessageCallback(nullptr, nullptr);
    SDL_GL_DeleteContext(context);
  }
  if (window)
    SDL_DestroyWindow(window);
  SDL_Quit();
  return result;
}
