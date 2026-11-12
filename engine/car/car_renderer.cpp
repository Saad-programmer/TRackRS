// SPDX-License-Identifier: GPL-2.0-or-later
#include "car_renderer.h"
#include "engine/pbr/ibl.h"
#include "engine/pbr/renderer.h"
#include "engine/render/basic.h"
#include "engine/rhi/rhi.h"
#include "engine/world/state.h"
#include "validation.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <array>
#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <map>
#include <memory>
#include <stdexcept>
namespace trackrs::car {
namespace {
Settings current;
const void *bufferOffset(std::size_t bytes) {
  // GL buffer-object offsets are passed through the pointer parameter.
  // NOLINTNEXTLINE(performance-no-int-to-ptr)
  return reinterpret_cast<const void *>(bytes);
}
Image decode(const mesh::Texture &texture) {
  if (texture.empty())
    return {};
  std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)> decoded(
      IMG_Load_RW(SDL_RWFromConstMem(texture.encoded.data(), static_cast<int>(texture.encoded.size())), 1),
      SDL_FreeSurface);
  if (!decoded)
    throw std::runtime_error("cannot decode car texture " + texture.name);
  std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)> rgba(
      SDL_ConvertSurfaceFormat(decoded.get(), SDL_PIXELFORMAT_RGBA32, 0), SDL_FreeSurface);
  if (!rgba)
    throw std::runtime_error(SDL_GetError());
  Image im = makeImage(rgba->w, rgba->h);
  for (int y = 0; y < rgba->h; ++y)
    std::copy_n(static_cast<const std::uint8_t *>(rgba->pixels) + static_cast<std::ptrdiff_t>(y) * rgba->pitch,
                static_cast<std::size_t>(rgba->w) * 4, im.at(0, y));
  return im;
}
GLuint upload(const Image &im, GLenum internal, std::size_t &bytes) {
  GLuint id = 0;
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  // OBJ UVs have v = 0 at the image bottom; GL rows start at v = 0: upload bottom row first.
  std::vector<std::uint8_t> rows(im.rgba.size());
  const auto stride = static_cast<std::size_t>(im.width) * 4;
  for (int y = 0; y < im.height; ++y)
    std::copy_n(im.at(0, y), stride, rows.data() + static_cast<std::size_t>(im.height - 1 - y) * stride);
  glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internal), im.width, im.height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               rows.data());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glGenerateMipmap(GL_TEXTURE_2D);
  const std::size_t texel = internal == GL_RG8 ? 2 : 4;
  bytes += static_cast<std::size_t>(im.width) * static_cast<std::size_t>(im.height) * texel * 4 / 3;
  return id;
}
struct Part {
  GLuint vao = 0, vbo = 0, ibo = 0;
  std::array<std::size_t, 3> first{}, count{};
  LodSet lods;
  std::size_t bytes = 0;
  bool clockwise = false; ///< outward faces wind clockwise (negative signed volume)
  void release() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ibo);
    vao = vbo = ibo = 0;
  }
};
// Merge all submeshes, build LODs and upload one packed VBO + concatenated LOD IBO.
Part uploadMesh(const mesh::MeshData &data) {
  std::vector<mesh::VertexPacked> vertices;
  std::vector<std::uint32_t> indices;
  for (const auto &s : data.submeshes) {
    const auto base = static_cast<std::uint32_t>(vertices.size());
    vertices.insert(vertices.end(), s.packed.begin(), s.packed.end());
    for (auto i : s.indices)
      indices.push_back(base + i);
  }
  if (vertices.empty() || indices.empty())
    throw std::runtime_error("empty car mesh");
  Part p;
  // Winding from the authored normals (bodies are open shells, so a signed
  // volume is unreliable): outward faces agree with their vertex normals.
  long agree = 0;
  for (const auto &s : data.submeshes) {
    if (s.vertices.size() != s.packed.size())
      continue;
    for (std::size_t i = 0; i + 2 < s.indices.size(); i += 3) {
      const auto &a = s.vertices[s.indices[i]], &b = s.vertices[s.indices[i + 1]], &c = s.vertices[s.indices[i + 2]];
      const glm::vec3 pa(a.position[0], a.position[1], a.position[2]), pb(b.position[0], b.position[1], b.position[2]),
          pc(c.position[0], c.position[1], c.position[2]);
      const glm::vec3 n(a.normal[0] + b.normal[0] + c.normal[0], a.normal[1] + b.normal[1] + c.normal[1],
                        a.normal[2] + b.normal[2] + c.normal[2]);
      const float d = glm::dot(glm::cross(pb - pa, pc - pa), n);
      agree += d > 0 ? 1 : d < 0 ? -1 : 0;
    }
  }
  p.clockwise = agree < 0;
  p.lods = buildLods(vertices.front().position, vertices.size(), sizeof(mesh::VertexPacked), indices);
  std::vector<std::uint32_t> all;
  for (std::size_t l = 0; l < 3; ++l) {
    p.first[l] = all.size();
    p.count[l] = p.lods.indices[l].size();
    all.insert(all.end(), p.lods.indices[l].begin(), p.lods.indices[l].end());
  }
  glGenVertexArrays(1, &p.vao);
  glGenBuffers(1, &p.vbo);
  glGenBuffers(1, &p.ibo);
  glBindVertexArray(p.vao);
  glBindBuffer(GL_ARRAY_BUFFER, p.vbo);
  glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(mesh::VertexPacked)), vertices.data(),
               GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, p.ibo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(all.size() * 4), all.data(), GL_STATIC_DRAW);
  const auto stride = static_cast<GLsizei>(sizeof(mesh::VertexPacked));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, bufferOffset(offsetof(mesh::VertexPacked, position)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 4, GL_INT_2_10_10_10_REV, GL_TRUE, stride, bufferOffset(offsetof(mesh::VertexPacked, normal)));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, bufferOffset(offsetof(mesh::VertexPacked, uv)));
  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3, 4, GL_INT_2_10_10_10_REV, GL_TRUE, stride, bufferOffset(offsetof(mesh::VertexPacked, tangent)));
  glEnableVertexAttribArray(5);
  glVertexAttribPointer(5, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, bufferOffset(offsetof(mesh::VertexPacked, color)));
  glBindVertexArray(0);
  p.bytes = vertices.size() * sizeof(mesh::VertexPacked) + all.size() * 4;
  return p;
}
struct Textures {
  GLuint albedo = 0, roles = 0, lights = 0;
  std::size_t bytes = 0;
  std::array<std::size_t, 6> counts{};
  void release() {
    for (auto *t : {&albedo, &roles, &lights})
      glDeleteTextures(1, t);
  }
};
Textures buildTextures(const mesh::MeshData &data, const std::string &livery) {
  const auto &material = data.materials.at(data.submeshes.empty() ? 0 : data.submeshes.front().material);
  Image albedo = decode(material.albedo);
  if (albedo.empty())
    albedo = makeImage(4, 4, {200, 200, 200, 255});
  const Image emission = decode(material.emissive);
  const auto roles = classifyRoles(albedo, emission.empty() ? nullptr : &emission);
  if (!livery.empty())
    applyLivery(albedo, roles, parseLivery(livery));
  Textures t;
  t.counts = roles.counts;
  t.albedo = upload(albedo, GL_SRGB8_ALPHA8, t.bytes);
  t.roles = upload(roles.map, GL_RGBA8, t.bytes);
  t.lights = upload(roles.lights, GL_RG8, t.bytes);
  return t;
}
struct Assets {
  std::string name;
  Part body, wheel;
  Textures bodyTextures, wheelTextures;
  bool wheelSharesAtlas = false;
  GLuint interiorVao = 0, interiorVbo = 0, interiorIbo = 0;
  GLsizei interiorCount = 0;
  Interior interior;
  Cabin cabin;
  float wheelRadius = 1;
  std::size_t bytes() const {
    return body.bytes + wheel.bytes + bodyTextures.bytes + (wheelSharesAtlas ? 0 : wheelTextures.bytes) +
           interior.mesh.vertices.size() * sizeof(vegetation::Vertex) + interior.mesh.indices.size() * 2;
  }
  void release() {
    body.release();
    wheel.release();
    bodyTextures.release();
    if (!wheelSharesAtlas)
      wheelTextures.release();
    glDeleteVertexArrays(1, &interiorVao);
    glDeleteBuffers(1, &interiorVbo);
    glDeleteBuffers(1, &interiorIbo);
  }
};
struct VehicleState {
  Wear wear;
  float lastTime = -1;
  unsigned lod = 0;
  std::size_t draws = 0, triangles = 0;
  // Phase 3e motion vectors: this frame's main-view transforms and last frame's.
  const void *type = nullptr;
  bool drawn = false, hasPrevious = false;
  bool observed = false;
  bool brakeForced = false; ///< --brake-heat override active ///< wear driven per simulation tick by observe()
  glm::mat4 body{1}, previousBody{1};
  std::vector<glm::mat4> wheels, previousWheels;
};
struct Cache {
  std::unique_ptr<rhi::ShaderProgram> shader, velocity;
  std::unique_ptr<pbr::Ibl> environment;
  std::map<const void *, Assets> types;
  std::map<const void *, VehicleState> vehicles;
  std::map<const void *, std::string> vehicleTypes;
  GLuint flat = 0, white = 0;
  std::size_t frameDraws = 0, velocityDraws = 0;
};
std::unique_ptr<Cache> cache;

Assets load(const Instance &car, const mesh::ReadFile &read) {
  mesh::ImportOptions options;
  options.optimize = true;
  options.assetRoot = std::filesystem::path(car.bodyFile).parent_path().parent_path().generic_string();
  const auto bodyData = mesh::MeshImporter::load(car.bodyFile, read, options);
  const auto wheelData = mesh::MeshImporter::load(car.wheelFile, read, options);
  Assets a;
  a.name = car.name;
  a.body = uploadMesh(bodyData);
  a.wheel = uploadMesh(wheelData);
  a.bodyTextures = buildTextures(bodyData, car.livery);
  const auto &bm = bodyData.materials.at(bodyData.submeshes.front().material);
  const auto &wm = wheelData.materials.at(wheelData.submeshes.front().material);
  a.wheelSharesAtlas = !wm.albedo.empty() && wm.albedo.encoded == bm.albedo.encoded;
  a.wheelTextures = a.wheelSharesAtlas ? a.bodyTextures : buildTextures(wheelData, "");
  const auto &w = a.wheel.lods;
  a.wheelRadius = std::max({std::abs(w.minimum.y), std::abs(w.maximum.y), std::abs(w.minimum.z), std::abs(w.maximum.z)});
  {
    std::vector<mesh::VertexPacked> all;
    for (const auto &s : bodyData.submeshes)
      all.insert(all.end(), s.packed.begin(), s.packed.end());
    const auto cabin = measureCabin(all.front().position, all.size(), sizeof(mesh::VertexPacked), a.body.lods.minimum,
                                    a.body.lods.maximum);
    a.cabin = cabin;
    a.interior = buildInterior(a.body.lods.minimum, a.body.lods.maximum, cabin);
  }
  auto &m = a.interior.mesh;
  glGenVertexArrays(1, &a.interiorVao);
  glGenBuffers(1, &a.interiorVbo);
  glGenBuffers(1, &a.interiorIbo);
  glBindVertexArray(a.interiorVao);
  glBindBuffer(GL_ARRAY_BUFFER, a.interiorVbo);
  glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m.vertices.size() * sizeof(vegetation::Vertex)), m.vertices.data(),
               GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, a.interiorIbo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(m.indices.size() * 2), m.indices.data(), GL_STATIC_DRAW);
  const auto stride = static_cast<GLsizei>(sizeof(vegetation::Vertex));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, bufferOffset(offsetof(vegetation::Vertex, position)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 4, GL_INT_2_10_10_10_REV, GL_TRUE, stride, bufferOffset(offsetof(vegetation::Vertex, normal)));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_UNSIGNED_SHORT, GL_TRUE, stride, bufferOffset(offsetof(vegetation::Vertex, uv)));
  glDisableVertexAttribArray(3);
  glVertexAttrib4f(3, 1, 0, 0, 1);
  glEnableVertexAttribArray(5);
  glVertexAttribPointer(5, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, bufferOffset(offsetof(vegetation::Vertex, color)));
  glBindVertexArray(0);
  a.interiorCount = static_cast<GLsizei>(m.indices.size());
  return a;
}
GLuint pixel(GLenum internal, std::array<std::uint8_t, 4> rgba) {
  GLuint id = 0;
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);
  glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internal), 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  return id;
}
// Saves/restores the GL state this renderer touches.
class Guard {
  GLint program = 0, vao = 0, active = 0, blendSrc = 0, blendDst = 0, front = 0, cullMode = 0, depthFunc = 0;
  std::array<GLint, 12> two{}, cube{};
  GLboolean depthMask = 0, cull = 0, blend = 0, depth = 0;

public:
  Guard() {
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
    glGetIntegerv(GL_BLEND_SRC_RGB, &blendSrc);
    glGetIntegerv(GL_BLEND_DST_RGB, &blendDst);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
    glGetIntegerv(GL_FRONT_FACE, &front);
    glGetIntegerv(GL_CULL_FACE_MODE, &cullMode);
    glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);
    cull = glIsEnabled(GL_CULL_FACE);
    blend = glIsEnabled(GL_BLEND);
    depth = glIsEnabled(GL_DEPTH_TEST);
    for (unsigned i = 0; i < two.size(); ++i) {
      if (i == 10)
        continue; // scene shadow array is owned by world::apply
      glActiveTexture(GL_TEXTURE0 + i);
      glGetIntegerv(GL_TEXTURE_BINDING_2D, &two[i]);
      glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &cube[i]);
    }
  }
  ~Guard() {
    for (unsigned i = 0; i < two.size(); ++i) {
      if (i == 10)
        continue;
      glActiveTexture(GL_TEXTURE0 + i);
      glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(two[i]));
      glBindTexture(GL_TEXTURE_CUBE_MAP, static_cast<GLuint>(cube[i]));
    }
    glActiveTexture(static_cast<GLenum>(active));
    glUseProgram(static_cast<GLuint>(program));
    glBindVertexArray(static_cast<GLuint>(vao));
    glBlendFunc(static_cast<GLenum>(blendSrc), static_cast<GLenum>(blendDst));
    glDepthMask(depthMask);
    glFrontFace(static_cast<GLenum>(front));
    glCullFace(static_cast<GLenum>(cullMode));
    glDepthFunc(static_cast<GLenum>(depthFunc));
    for (auto [cap, on] : {std::pair{GL_CULL_FACE, cull}, {GL_BLEND, blend}, {GL_DEPTH_TEST, depth}})
      on ? glEnable(cap) : glDisable(cap);
    draw::invalidateGpuState();
  }
};
} // namespace

void configure(Settings settings) { current = settings; }
Settings settings() { return current; }

// fovY (radians) and time (seconds) are distinct units.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
std::size_t draw(const Instance &car, const mesh::ReadFile &read, const glm::mat4 &vp, glm::vec3 camera, float fovY,
                 float time, bool linearCapture) {
  if (car.wheels.size() > 8)
    throw std::invalid_argument("too many wheels");
  draw::flush();
  Guard guard;
  if (!cache)
    cache = std::make_unique<Cache>();
  if (!cache->shader) {
    const auto data = pbr::shaderDirectory().parent_path().parent_path();
    cache->shader = std::make_unique<rhi::ShaderProgram>(
        data / "shaders/pbr/car.vert", data / "shaders/pbr/pbr.frag",
        [](GLuint id) { return glGetUniformLocation(id, "uRoleMap") >= 0; }, std::vector<std::string>{"CAR"});
    cache->environment = std::make_unique<pbr::Ibl>(data / "scenes/material_preview/ibl");
    cache->flat = pixel(GL_RGBA8, {128, 128, 255, 255});
    cache->white = pixel(GL_RGBA8, {255, 255, 255, 255});
  }
  auto &c = *cache;
  auto found = c.types.find(car.type);
  if (found == c.types.end())
    found = c.types.emplace(car.type, load(car, read)).first;
  Assets &a = found->second;
  auto &state = c.vehicles[car.vehicle];
  c.vehicleTypes[car.vehicle] = car.name;
  const auto &ws = world::state();
  const bool shadow = ws.depthPass;
  if (!shadow && !state.observed) { // fallback: wear advances once per main-view frame
    const float dt = state.lastTime < 0 ? 0.f : std::clamp(time - state.lastTime, 0.f, .25f);
    state.lastTime = time;
    WearInput in;
    in.speed = car.speed;
    in.brake = car.brake;
    in.wetness = ws.settings.weather.wetness;
    in.dt = dt;
    in.surface = car.surface;
    updateWear(state.wear, in);
  }
  // LOD from the projected size of the body's bounding sphere.
  const glm::vec3 centreModel = (a.body.lods.minimum + a.body.lods.maximum) * .5f;
  const glm::vec3 centre = glm::vec3(car.body * glm::vec4(centreModel, 1));
  const float scale = glm::length(glm::vec3(car.body[0]));
  unsigned lod = chooseLod(a.body.lods.radius * scale, glm::length(centre - camera), fovY);
  if (shadow)
    lod = std::max(lod, 1u);
  c.shader->poll();
  c.environment->pump();
  const GLuint p = c.shader->id();
  glUseProgram(p);
  world::apply(p, glm::mat4(1), ws.scene);
  auto u = [&](const char *name) { return glGetUniformLocation(p, name); };
  glUniformMatrix4fv(u("uViewProj"), 1, GL_FALSE, glm::value_ptr(vp));
  const glm::vec3 eye = ws.settings.enabled && !shadow ? glm::vec3(glm::inverse(ws.view)[3]) : camera;
  glUniform3fv(u("uCamPos"), 1, glm::value_ptr(eye));
  const bool lit = ws.settings.enabled;
  const glm::vec3 sun = lit ? ws.sun.direction : glm::normalize(glm::vec3(.2f, .5f, 1));
  const glm::vec3 sunColour = lit ? ws.sun.radiance : glm::vec3(3), ambient = lit ? ws.sun.ambient : glm::vec3(.08f);
  glUniform3fv(u("uSunDir"), 1, glm::value_ptr(sun));
  glUniform3fv(u("uSunColor"), 1, glm::value_ptr(sunColour));
  glUniform3fv(u("uAmbientColor"), 1, glm::value_ptr(ambient));
  glUniform1i(u("uPointLightCount"), 0);
  glUniform1i(u("uUseLocalProbe"), 0);
  glUniform1i(u("uToneMap"), linearCapture ? 0 : 1);
  glUniform1f(u("uMetallicFactor"), 1);
  glUniform1f(u("uRoughnessFactor"), 1);
  glUniform1f(u("uAOStrength"), 0);
  glUniform1f(u("uAlphaCutoff"), 0);
  glUniform1f(u("uClearcoatRough"), .04f);
  glUniform1f(u("uWetness"), 0);
  for (auto [name, unit] : {std::pair{"uAlbedoMap", 0}, {"uNormalMap", 1}, {"uMetalRoughMap", 2}, {"uIrradianceMap", 3},
                            {"uPrefilterMap", 4}, {"uBrdfLUT", 5}, {"uLocalProbe", 7}, {"uRoleMap", 9}, {"uLightMap", 11}})
    glUniform1i(u(name), unit);
  const bool ibl = c.environment->ready();
  glUniform1i(u("uHasIBL"), ibl ? 1 : 0);
  if (ibl) {
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_CUBE_MAP, c.environment->irradiance());
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_CUBE_MAP, c.environment->prefilter());
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, c.environment->brdf());
    glUniform1f(u("uPrefilterMaxLod"), c.environment->maxLod());
    glUniformMatrix3fv(u("uEnvironmentRotation"), 1, GL_FALSE, glm::value_ptr(glm::mat3(1)));
  }
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, c.flat);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, c.white);
  const bool lights = lit && world::headlightsOn(ws.sun, ws.settings.weather.overcast);
  glUniform4f(u("uCarLights"), lights ? 1.f : 0.f, lights ? 1.f : 0.f, std::clamp(car.brake, 0.f, 1.f), car.reverse ? 1.f : 0.f);
  glUniform4f(u("uDirt"), state.wear.dust, state.wear.mud, ws.settings.weather.wetness, 0);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  std::size_t draws = 0, triangles = 0;
  auto bindTextures = [&](const Textures &t) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, t.albedo);
    glActiveTexture(GL_TEXTURE9);
    glBindTexture(GL_TEXTURE_2D, t.roles);
    glActiveTexture(GL_TEXTURE11);
    glBindTexture(GL_TEXTURE_2D, t.lights);
  };
  // Part kind (shader enum) and LOD level are distinct small integers.
  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  auto drawPart = [&](const Part &part, const glm::mat4 &model, int kind, unsigned level, bool glass) {
    glUniformMatrix4fv(u("uModel"), 1, GL_FALSE, glm::value_ptr(model));
    const glm::mat3 normal = glm::transpose(glm::inverse(glm::mat3(model)));
    glUniformMatrix3fv(u("uNormalMatrix"), 1, GL_FALSE, glm::value_ptr(normal));
    glUniform1i(u("uCarPart"), kind);
    glUniform1i(u("uGlassPass"), glass ? 1 : 0);
    glFrontFace(part.clockwise ? GL_CW : GL_CCW);
    glBindVertexArray(part.vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(part.count[level]), GL_UNSIGNED_INT, bufferOffset(part.first[level] * 4));
    ++draws;
    triangles += part.count[level] / 3;
  };
  const auto &b = a.body.lods;
  glUniform4f(u("uBodyBounds"), b.minimum.z, b.maximum.z, b.maximum.y, a.wheelRadius);
  // Opaque: body, wheels, interior.
  bindTextures(a.bodyTextures);
  drawPart(a.body, car.body, 0, lod, false);
  bindTextures(a.wheelTextures);
  glUniform4f(u("uBodyBounds"), b.minimum.z, b.maximum.z, b.maximum.y, a.wheelRadius);
  for (std::size_t i = 0; i < car.wheels.size(); ++i) {
    glUniform1f(u("uBrakeGlow"), brakeGlow(state.wear.brakeC[std::min<std::size_t>(i, 3)]));
    drawPart(a.wheel, car.wheels[i], 1, lod, false);
  }
  glUniform1f(u("uBrakeGlow"), 0);
  if (!shadow && car.interior && lod == 0) {
    glUniformMatrix4fv(u("uModel"), 1, GL_FALSE, glm::value_ptr(car.body));
    glUniformMatrix3fv(u("uNormalMatrix"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(glm::mat3(car.body)))));
    glUniform1i(u("uCarPart"), 2);
    glUniform1i(u("uGlassPass"), 0);
    glUniform3fv(u("uTachoPivot"), 1, glm::value_ptr(a.interior.tachoPivot));
    glUniform3fv(u("uSpeedPivot"), 1, glm::value_ptr(a.interior.speedPivot));
    glUniform2f(u("uNeedles"), needleAngle(car.rpm, car.redlineRpm * 1.1f), needleAngle(car.speed * 3.6f, 240.f));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, c.white);
    glDisable(GL_CULL_FACE);
    glBindVertexArray(a.interiorVao);
    glDrawElements(GL_TRIANGLES, a.interiorCount, GL_UNSIGNED_SHORT, nullptr);
    ++draws;
    triangles += static_cast<std::size_t>(a.interiorCount) / 3;
    glEnable(GL_CULL_FACE);
  }
  // Glass: blended over the cabin, no depth writes.
  if (!shadow) {
    bindTextures(a.bodyTextures);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    drawPart(a.body, car.body, 0, lod, true);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
  }
  if (validation::run.capture) {
    validation::run.frameDraws += draws;
    validation::run.frameTriangles += triangles;
  }
  if (!shadow) {
    state.lod = lod;
    state.draws = draws;
    state.triangles = triangles;
    state.type = car.type;
    state.drawn = true;
    state.body = car.body;
    state.wheels = car.wheels;
  }
  return draws;
}

void observe(const void *vehicle, const WearInput &input) {
  if (!cache)
    cache = std::make_unique<Cache>(); // GL objects are created lazily by draw()
  auto &s = cache->vehicles[vehicle];
  s.observed = true;
  updateWear(s.wear, input);
}

void forceBrakeTemperature(const void *vehicle, float celsius) {
  if (!cache)
    cache = std::make_unique<Cache>();
  auto &s = cache->vehicles[vehicle];
  s.wear.brakeC.fill(std::clamp(celsius, 25.f, 950.f));
  s.brakeForced = true;
}

std::size_t drawVelocity(const glm::mat4 &jittered, const glm::mat4 &current, const glm::mat4 &previous) {
  if (!cache)
    return 0;
  auto &c = *cache;
  std::size_t draws = 0;
  bool any = false;
  for (auto &[key, s] : c.vehicles)
    any = any || s.drawn;
  if (any) {
    draw::flush();
    Guard guard;
    if (!c.velocity) {
      const auto data = pbr::shaderDirectory().parent_path().parent_path();
      c.velocity = std::make_unique<rhi::ShaderProgram>(data / "shaders/pbr/car_velocity.vert",
                                                        data / "shaders/pbr/car_velocity.frag");
    }
    c.velocity->poll();
    const GLuint p = c.velocity->id();
    glUseProgram(p);
    auto u = [&](const char *name) { return glGetUniformLocation(p, name); };
    glUniformMatrix4fv(u("uViewProjection"), 1, GL_FALSE, glm::value_ptr(jittered));
    glUniformMatrix4fv(u("uCurrentViewProjection"), 1, GL_FALSE, glm::value_ptr(current));
    glUniformMatrix4fv(u("uPreviousViewProjection"), 1, GL_FALSE, glm::value_ptr(previous));
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL); // same geometry and matrices as the colour pass
    glDepthMask(GL_FALSE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    for (auto &[key, s] : c.vehicles) {
      if (!s.drawn)
        continue;
      const auto found = c.types.find(s.type);
      if (found == c.types.end())
        continue;
      const Assets &a = found->second;
      auto part = [&](const Part &mesh, const glm::mat4 &model, const glm::mat4 &before) {
        glUniformMatrix4fv(u("uModel"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(u("uPreviousModel"), 1, GL_FALSE, glm::value_ptr(before));
        glFrontFace(mesh.clockwise ? GL_CW : GL_CCW);
        glBindVertexArray(mesh.vao);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.count[s.lod]), GL_UNSIGNED_INT,
                       bufferOffset(mesh.first[s.lod] * 4));
        ++draws;
      };
      part(a.body, s.body, s.hasPrevious ? s.previousBody : s.body);
      for (std::size_t i = 0; i < s.wheels.size(); ++i)
        part(a.wheel, s.wheels[i],
             s.hasPrevious && i < s.previousWheels.size() ? s.previousWheels[i] : s.wheels[i]);
    }
    glDepthMask(GL_TRUE);
  }
  // Roll the frame: cars not drawn this frame lose their history.
  for (auto &[key, s] : c.vehicles) {
    s.hasPrevious = s.drawn;
    if (s.drawn) {
      s.previousBody = s.body;
      s.previousWheels = s.wheels;
    }
    s.drawn = false;
  }
  if (validation::run.capture)
    validation::run.frameDraws += draws;
  c.velocityDraws += draws;
  return draws;
}

void shutdown() {
  if (!cache)
    return;
  for (auto &[key, a] : cache->types)
    a.release();
  glDeleteTextures(1, &cache->flat);
  glDeleteTextures(1, &cache->white);
  cache.reset();
}

nlohmann::json report() {
  nlohmann::json types = nlohmann::json::array(), vehicles = nlohmann::json::array();
  if (cache) {
    for (const auto &[key, a] : cache->types)
      types.push_back({{"name", a.name},
                       {"body_triangles_per_lod", {a.body.count[0] / 3, a.body.count[1] / 3, a.body.count[2] / 3}},
                       {"wheel_triangles_per_lod", {a.wheel.count[0] / 3, a.wheel.count[1] / 3, a.wheel.count[2] / 3}},
                       {"interior_triangles", a.interiorCount / 3},
                       {"cabin", {{"measured", a.cabin.measured}, {"roof_z", a.cabin.roofZ}, {"half_width", a.cabin.halfWidth}}},
                       {"bounds", {{"min", {a.body.lods.minimum.x, a.body.lods.minimum.y, a.body.lods.minimum.z}},
                                   {"max", {a.body.lods.maximum.x, a.body.lods.maximum.y, a.body.lods.maximum.z}}}},
                       {"gpu_bytes", a.bytes()},
                       {"texture_bytes", a.bodyTextures.bytes + (a.wheelSharesAtlas ? 0 : a.wheelTextures.bytes)},
                       {"roles_texels", {{"paint", a.bodyTextures.counts[0]},
                                         {"glass", a.bodyTextures.counts[1]},
                                         {"rubber", a.bodyTextures.counts[2]},
                                         {"metal", a.bodyTextures.counts[3]},
                                         {"tail_light", a.bodyTextures.counts[4]},
                                         {"head_light", a.bodyTextures.counts[5]}}}});
    for (const auto &[key, s] : cache->vehicles)
      vehicles.push_back({{"name", cache->vehicleTypes[key]},
                          {"lod", s.lod},
                          {"draws", s.draws},
                          {"triangles", s.triangles},
                          {"dust", s.wear.dust},
                          {"mud", s.wear.mud},
                          {"brake_c", s.wear.brakeC}, {"brake_forced", s.brakeForced}});
  }
  return {{"renderer", "car-v2"},
          {"enabled", current.enabled},
          {"types", types},
          {"vehicles", vehicles},
          {"velocity_draws", cache ? cache->velocityDraws : std::size_t{0}}};
}
void writeReport(const std::filesystem::path &path) {
  if (cache)
    std::ofstream(path) << report().dump(2) << '\n';
}
} // namespace trackrs::car
