// SPDX-License-Identifier: GPL-2.0-or-later
#include "renderer.h"
#include "engine/render/basic.h"
#include "engine/rhi/rhi.h"
#include "texture_stream.h"
#include "validation.h"
#include <SDL2/SDL_image.h>
#include <array>
#include <glm/gtc/type_ptr.hpp>
#include <limits>
#include <map>
#include <stdexcept>
namespace trackrs::pbr {
namespace {
std::filesystem::path shaderRoot;
std::vector<PointLight> currentHeadlights;
std::unique_ptr<Ibl> runtimeIbl;
std::unique_ptr<ReflectionProbe> runtimeProbe;
glm::mat4 worldView(1);
glm::vec3 currentCarCenter(0);
SceneCapture probeScene;

// Preserve caller GL state as well as invalidating the legacy renderer cache.
// This guard also covers lazy VAO/texture creation and exceptional exits.
class State {
  GLint program = 0, vao = 0, array = 0, active = 0, unpack = 0, front = 0,
        cull = 0, srcRGB = 0, dstRGB = 0, srcAlpha = 0, dstAlpha = 0;
  GLint rowLength = 0, shadowTexture = 0;
  GLboolean depthWrite = 0;
  std::array<GLboolean, 6> enabled{};
  std::array<GLint, 8> textures{}, cubes{};
  static constexpr std::array<GLenum, 6> caps = {GL_BLEND,
                                                 GL_CULL_FACE,
                                                 GL_DEPTH_TEST,
                                                 GL_SAMPLE_ALPHA_TO_COVERAGE,
                                                 GL_FRAMEBUFFER_SRGB,
                                                 GL_TEXTURE_CUBE_MAP_SEAMLESS};

public:
  State() {
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &array);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpack);
    glGetIntegerv(GL_UNPACK_ROW_LENGTH, &rowLength);
    glActiveTexture(GL_TEXTURE6);
    glGetIntegerv(GL_TEXTURE_BINDING_2D_ARRAY, &shadowTexture);
    glGetIntegerv(GL_FRONT_FACE, &front);
    glGetIntegerv(GL_CULL_FACE_MODE, &cull);
    glGetIntegerv(GL_BLEND_SRC_RGB, &srcRGB);
    glGetIntegerv(GL_BLEND_DST_RGB, &dstRGB);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &srcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &dstAlpha);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWrite);
    for (std::size_t i = 0; i < caps.size(); ++i)
      enabled[i] = glIsEnabled(caps[i]);
    for (unsigned i = 0; i < textures.size(); ++i) {
      glActiveTexture(GL_TEXTURE0 + i);
      glGetIntegerv(GL_TEXTURE_BINDING_2D, &textures[i]);
      glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &cubes[i]);
    }
  }
  ~State() {
    for (std::size_t i = 0; i < caps.size(); ++i)
      enabled[i] ? glEnable(caps[i]) : glDisable(caps[i]);
    glDepthMask(depthWrite);
    glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
    glFrontFace(front);
    glCullFace(cull);
    glPixelStorei(GL_UNPACK_ALIGNMENT, unpack);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, rowLength);
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D_ARRAY, static_cast<GLuint>(shadowTexture));
    for (unsigned i = 0; i < textures.size(); ++i) {
      glActiveTexture(GL_TEXTURE0 + i);
      glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(textures[i]));
      glBindTexture(GL_TEXTURE_CUBE_MAP, static_cast<GLuint>(cubes[i]));
    }
    glActiveTexture(static_cast<GLenum>(active));
    glUseProgram(static_cast<GLuint>(program));
    glBindVertexArray(static_cast<GLuint>(vao));
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(array));
  }
};
const void *offset(std::size_t bytes) {
  // OpenGL interprets this pointer as a byte offset into the bound buffer.
  // NOLINTNEXTLINE(performance-no-int-to-ptr)
  return reinterpret_cast<const void *>(bytes);
}
void integer(GLuint p, const char *name, int v) {
  glUniform1i(glGetUniformLocation(p, name), v);
}
void scalar(GLuint p, const char *name, float v) {
  glUniform1f(glGetUniformLocation(p, name), v);
}
void vector(GLuint p, const char *name, glm::vec3 v) {
  glUniform3fv(glGetUniformLocation(p, name), 1, glm::value_ptr(v));
}
struct Texture {
  GLuint id = 0;
  std::shared_ptr<TextureStream> stream;
  std::shared_ptr<StreamedTexture> ticket;
  std::size_t bytes = 4;
  std::size_t resident() const {
    return ticket ? ticket->residentBytes : bytes;
  }
  GLuint name() const {
    if (ticket && !ticket->error.empty())
      throw std::runtime_error(ticket->error);
    return ticket ? ticket->id() : id;
  }
  Texture(const mesh::Texture &source, bool srgb, assets::Semantic semantic) {
    if (assets::isKtx2(source.encoded)) {
      stream = textureStream();
      ticket = stream->request(source.encoded, semantic);
      return;
    }

    try {
      glGenTextures(1, &id);
      glBindTexture(GL_TEXTURE_2D, id);
      std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)> decoded(
          nullptr, SDL_FreeSurface),
          rgba(nullptr, SDL_FreeSurface);
      if (source.empty()) {
        const std::array<unsigned char, 4> white = {255, 255, 255, 255};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, white.data());
      } else {
        if (source.encoded.size() >
            static_cast<std::size_t>(std::numeric_limits<int>::max()))
          throw std::runtime_error("image too large");
        auto rw = SDL_RWFromConstMem(source.encoded.data(),
                                     static_cast<int>(source.encoded.size()));
        decoded.reset(IMG_Load_RW(rw, 1));
        if (!decoded || decoded->w > 8192 || decoded->h > 8192)
          throw std::runtime_error("invalid/oversized PBR image: " +
                                   source.name);
        rgba.reset(
            SDL_ConvertSurfaceFormat(decoded.get(), SDL_PIXELFORMAT_RGBA32, 0));
        if (!rgba)
          throw std::runtime_error(SDL_GetError());
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, rgba->pitch / 4);
        glTexImage2D(GL_TEXTURE_2D, 0, srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8,
                     rgba->w, rgba->h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     rgba->pixels);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
      }
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                      GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glGenerateMipmap(GL_TEXTURE_2D);
      if (rgba) {
        bytes = 0;
        for (int w = rgba->w, h = rgba->h;;
             w = std::max(1, w / 2), h = std::max(1, h / 2)) {
          bytes += std::size_t(w) * std::size_t(h) * 4;
          if (w == 1 && h == 1)
            break;
        }
      }

    } catch (...) {
      glDeleteTextures(1, &id);
      throw;
    }
  }
  ~Texture() { glDeleteTextures(1, &id); }
  Texture(const Texture &) = delete;
};
struct Instance {
  float model[16], normal[9], parity;
};
static_assert(sizeof(Instance) == 104);
struct Geometry {
  GLuint vao[2]{}, vbo[2]{}, indices = 0, instances = 0, colors = 0;
  GLsizei count = 0;
  std::size_t material;
  Geometry(const mesh::Submesh &source)
      : count(static_cast<GLsizei>(source.indices.size())),
        material(source.material) {
    if (source.vertices.size() != source.packed.size() ||
        source.indices.size() % 3 ||
        source.indices.size() >
            static_cast<std::size_t>(std::numeric_limits<GLsizei>::max()))
      throw std::invalid_argument("invalid GPU mesh layout/count");
    for (auto index : source.indices)
      if (index >= source.vertices.size())
        throw std::out_of_range("GPU mesh index");
    glGenVertexArrays(2, vao);
    glGenBuffers(2, vbo);
    glGenBuffers(1, &indices);
    glGenBuffers(1, &instances);
    glGenBuffers(1, &colors);
    glBindBuffer(GL_ARRAY_BUFFER, colors);
    std::vector<std::uint32_t> tint;
    tint.reserve(source.packed.size());
    for (const auto &v : source.packed)
      tint.push_back(v.color);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(tint.size() * sizeof(std::uint32_t)),
                 tint.data(), GL_STATIC_DRAW);
    for (unsigned layout = 0; layout < 2; ++layout) {
      glBindVertexArray(vao[layout]);
      glBindBuffer(GL_ARRAY_BUFFER, vbo[layout]);
      const bool packed = layout == 1;
      GLsizei stride =
          packed ? sizeof(mesh::VertexPacked) : sizeof(mesh::Vertex);
      glBufferData(GL_ARRAY_BUFFER,
                   static_cast<GLsizeiptr>(source.vertices.size() *
                                           static_cast<std::size_t>(stride)),
                   packed ? static_cast<const void *>(source.packed.data())
                          : source.vertices.data(),
                   GL_STATIC_DRAW);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices);
      if (!layout)
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(source.indices.size() *
                                             sizeof(std::uint32_t)),
                     source.indices.data(), GL_STATIC_DRAW);
      for (unsigned loc : {0u, 1u, 2u, 3u})
        glEnableVertexAttribArray(loc);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, offset(0));
      glVertexAttribPointer(1, packed ? 4 : 3,
                            packed ? GL_INT_2_10_10_10_REV : GL_FLOAT,
                            packed ? GL_TRUE : GL_FALSE, stride, offset(12));
      glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride,
                            offset(packed ? 20 : 24));
      glVertexAttribPointer(
          3, packed ? 4 : 3, packed ? GL_INT_2_10_10_10_REV : GL_FLOAT,
          packed ? GL_TRUE : GL_FALSE, stride, offset(packed ? 16 : 32));
      if (packed) {
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride,
                              offset(28));
      } else {
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride, offset(44));
        glBindBuffer(GL_ARRAY_BUFFER, colors);
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 4, GL_UNSIGNED_BYTE, GL_TRUE,
                              sizeof(std::uint32_t), nullptr);
      }
      glBindBuffer(GL_ARRAY_BUFFER, instances);
      for (unsigned col = 0; col < 4; ++col) {
        glEnableVertexAttribArray(6 + col);
        glVertexAttribPointer(6 + col, 4, GL_FLOAT, GL_FALSE, sizeof(Instance),
                              offset(std::size_t{col} * 16));
        glVertexAttribDivisor(6 + col, 1);
      }
      for (unsigned col = 0; col < 3; ++col) {
        glEnableVertexAttribArray(10 + col);
        glVertexAttribPointer(10 + col, 3, GL_FLOAT, GL_FALSE, sizeof(Instance),
                              offset(64 + std::size_t{col} * 12));
        glVertexAttribDivisor(10 + col, 1);
      }
      glEnableVertexAttribArray(13);
      glVertexAttribPointer(13, 1, GL_FLOAT, GL_FALSE, sizeof(Instance),
                            offset(100));
      glVertexAttribDivisor(13, 1);
    }
  }
  ~Geometry() {
    glDeleteVertexArrays(2, vao);
    glDeleteBuffers(2, vbo);
    glDeleteBuffers(1, &indices);
    glDeleteBuffers(1, &instances);
    glDeleteBuffers(1, &colors);
  }
  Geometry(const Geometry &) = delete;
};
} // namespace
struct Model::Impl {
  std::filesystem::path shaders;
  std::vector<mesh::Material> materials;
  struct Surface {
    std::unique_ptr<Texture> albedo, normal, mr;
  };
  std::vector<Surface> surfaces;
  std::vector<std::unique_ptr<Geometry>> meshes;
  mesh::MeshData source;
  std::vector<std::shared_ptr<draw::Mesh>> legacy;
  std::map<unsigned, std::unique_ptr<rhi::ShaderProgram>> programs;
  Impl(const mesh::MeshData &data, const std::filesystem::path &path)
      : shaders(path), materials(data.materials), source(data) {
    State guard;
    glActiveTexture(GL_TEXTURE0);
    for (const auto &m : materials)
      surfaces.push_back(
          {std::make_unique<Texture>(m.albedo,
                                     m.path == mesh::MaterialPath::Pbr,
                                     assets::Semantic::Albedo),
           std::make_unique<Texture>(m.normal, false, assets::Semantic::Normal),
           std::make_unique<Texture>(m.metalRough, false,
                                     assets::Semantic::Orm)});
    for (const auto &sub : data.submeshes) {
      if (sub.material >= materials.size())
        throw std::runtime_error("material index out of bounds");
      meshes.push_back(std::make_unique<Geometry>(sub));
    }
  }
  GLuint program(unsigned flags) {
    auto &p = programs[flags];
    if (!p) {
      std::vector<std::string> defines;
      if (flags & 1)
        defines.push_back("EXPLICIT_BITANGENT");
      if (flags & 2)
        defines.push_back("INSTANCED");
      if (flags & 4)
        defines.push_back("ALPHA_TO_COVERAGE");
      if (flags & 8)
        defines.push_back("USE_CSM");
      p = std::make_unique<rhi::ShaderProgram>(
          shaders / "pbr.vert", shaders / "pbr.frag",
          [](GLuint candidate) {
            return glGetUniformLocation(candidate, "uViewProj") >= 0 &&
                   glGetUniformLocation(candidate, "uAlbedoMap") >= 0;
          },
          defines);
    }
    p->poll();
    return p->id();
  }
};
HeadlightScope::HeadlightScope(const glm::mat4 &bodyToEye) {
  std::vector<PointLight> points;
  points.reserve(2);
  for (float x : {-.55f, .55f})
    points.push_back({glm::vec3(bodyToEye * glm::vec4(x, 1.8f, .45f, 1)),
                      glm::vec3(8, 7.5f, 6), 12});
  previousCenter_ = currentCarCenter;
  currentCarCenter = glm::vec3(bodyToEye[3]);
  previous_ = std::move(currentHeadlights);
  currentHeadlights = std::move(points);
}
HeadlightScope::~HeadlightScope() {
  currentHeadlights = std::move(previous_);
  currentCarCenter = previousCenter_;
}
void setWorldView(const glm::mat4 &view) { worldView = view; }
void setProbeScene(SceneCapture scene) { probeScene = std::move(scene); }
void shutdown() {
  runtimeProbe.reset();
  runtimeIbl.reset();
  probeScene = {};
  clearMeasurements();
}
void configure(const std::filesystem::path &shaders) { shaderRoot = shaders; }
const std::filesystem::path &shaderDirectory() { return shaderRoot; }
Model::Model(const mesh::MeshData &data, const std::filesystem::path &shaders)
    : impl_(std::make_unique<Impl>(data, shaders)) {}
Model::~Model() = default;
Statistics Model::draw(const Frame &frame, const std::vector<glm::mat4> &models,
                       bool packed, bool instanced) {
  if (models.empty())
    return {};
  State guard;
  for (const auto &surface : impl_->surfaces)
    if (surface.albedo->stream) {
      surface.albedo->stream->pump();
      break;
    }
  Statistics stats;
  glDisable(GL_FRAMEBUFFER_SRGB);
  glEnable(GL_DEPTH_TEST);
  std::vector<Instance> stream;
  for (const auto &model : models) {
    const float det = glm::determinant(glm::mat3(model));
    if (!std::isfinite(det) || std::abs(det) < 1e-12f)
      throw std::runtime_error("singular PBR transform");
    Instance instance{};
    std::copy_n(glm::value_ptr(model), 16, instance.model);
    auto normal = glm::transpose(glm::inverse(glm::mat3(model)));
    std::copy_n(glm::value_ptr(normal), 9, instance.normal);
    instance.parity = det < 0 ? -1.f : 1.f;
    stream.push_back(instance);
  }
  for (const auto &geometry : impl_->meshes) {
    const auto &m = impl_->materials[geometry->material];
    if (m.path != mesh::MaterialPath::Pbr)
      continue;
    auto &textures = impl_->surfaces[geometry->material];
    GLint samples = 0;
    glGetIntegerv(GL_SAMPLES, &samples);
    bool coverage = m.alpha == mesh::AlphaMode::Mask && samples > 1;
    coverage ? glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE)
             : glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    if (m.doubleSided)
      glDisable(GL_CULL_FACE);
    else {
      glEnable(GL_CULL_FACE);
      glCullFace(GL_BACK);
    }
    if (m.alpha == mesh::AlphaMode::Blend || frame.alpha < 1) {
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glDepthMask(GL_FALSE);
    } else {
      glDisable(GL_BLEND);
      glDepthMask(GL_TRUE);
    }
    auto p = impl_->program((packed ? 0u : 1u) | (instanced ? 2u : 0u) |
                            (coverage ? 4u : 0u) |
                            (frame.shadows.texture ? 8u : 0u));
    glUseProgram(p);
    glUniformMatrix4fv(glGetUniformLocation(p, "uViewProj"), 1, GL_FALSE,
                       glm::value_ptr(frame.viewProjection));
    vector(p, "uCamPos", frame.camera);
    vector(p, "uSunDir", frame.sunDirection);
    vector(p, "uSunColor", frame.sunRadiance);
    vector(p, "uAmbientColor", frame.ambient);
    integer(p, "uDebugView", frame.debugView);
    integer(p, "uToneMap", frame.toneMap);
    bool hasIbl = frame.environment && frame.environment->ready();
    integer(p, "uHasIBL", hasIbl);
    scalar(p, "uWetness", frame.wetness);
    if (hasIbl) {
      glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
      glActiveTexture(GL_TEXTURE3);
      glBindTexture(GL_TEXTURE_CUBE_MAP, frame.environment->irradiance());
      glActiveTexture(GL_TEXTURE4);
      glBindTexture(GL_TEXTURE_CUBE_MAP, frame.environment->prefilter());
      glActiveTexture(GL_TEXTURE5);
      glBindTexture(GL_TEXTURE_2D, frame.environment->brdf());
      scalar(p, "uPrefilterMaxLod", frame.environment->maxLod());
      glUniformMatrix3fv(glGetUniformLocation(p, "uEnvironmentRotation"), 1,
                         GL_FALSE, glm::value_ptr(frame.environmentRotation));
      glUniformMatrix3fv(glGetUniformLocation(p, "uProbeRotation"), 1, GL_FALSE,
                         glm::value_ptr(frame.probeRotation));
      integer(p, "uUseLocalProbe", frame.reflectionProbe != 0);
      integer(p, "uLocalProbe", 7);
      scalar(p, "uProbeMaxLod", frame.probeMaxLod);
      if (frame.reflectionProbe) {
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_CUBE_MAP, frame.reflectionProbe);
      }
    }

    if (frame.shadows.texture) {
      if (frame.shadows.count < 1 || frame.shadows.count > 4)
        throw std::runtime_error("CSM count must be 1..4");
      integer(p, "uShadowMap", 6);
      integer(p, "uShadowEnabled", 1);
      integer(p, "uCascadeCount", frame.shadows.count);
      glUniformMatrix4fv(glGetUniformLocation(p, "uView"), 1, GL_FALSE,
                         glm::value_ptr(frame.shadows.view));
      glUniformMatrix4fv(glGetUniformLocation(p, "uLightViewProj[0]"), 4,
                         GL_FALSE,
                         glm::value_ptr(frame.shadows.lightViewProjection[0]));
      glUniform4fv(glGetUniformLocation(p, "uCascadeSplits"), 1,
                   glm::value_ptr(frame.shadows.splits));
      glActiveTexture(GL_TEXTURE6);
      glBindTexture(GL_TEXTURE_2D_ARRAY, frame.shadows.texture);
    }

    integer(p, "uAlbedoMap", 0);
    integer(p, "uNormalMap", 1);
    integer(p, "uMetalRoughMap", 2);
    integer(p, "uIrradianceMap", 3);
    integer(p, "uPrefilterMap", 4);
    integer(p, "uBrdfLUT", 5);
    integer(p,"uLocalProbe",7);
    auto baseColor = m.baseColor;
    baseColor[3] *= frame.alpha;
    glUniform4fv(glGetUniformLocation(p, "uBaseColorFactor"), 1,
                 baseColor.data());
    scalar(p, "uMetallicFactor", m.metallic);
    scalar(p, "uRoughnessFactor", m.roughness);
    scalar(p, "uNormalScale", m.normalScale);
    scalar(p, "uAOStrength", m.aoStrength);
    scalar(p, "uClearcoat", m.clearcoat);
    scalar(p, "uClearcoatRough", m.clearcoatRoughness);
    scalar(p, "uAlphaCutoff",
           m.alpha == mesh::AlphaMode::Mask ? m.alphaCutoff : 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textures.albedo->name());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textures.normal->name());
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, textures.mr->name());
    glBindVertexArray(geometry->vao[packed ? 1 : 0]);

    auto lights = [&](const glm::mat4 &model) {
      auto points = frame.lights;
      if (frame.headlights) {
        for (const auto &position : frame.headlightPositions) {
          glm::vec3 point(model * glm::vec4(position, 1));
          points.push_back({point, glm::vec3(8, 7.5f, 6), 12});
        }
      }
      if (points.size() > 8)
        points.resize(8);
      integer(p, "uPointLightCount", static_cast<int>(points.size()));
      for (std::size_t i = 0; i < points.size(); ++i) {
        std::string name = "uPointLightPos[" + std::to_string(i) + "]";
        vector(p, name.c_str(), points[i].position);
        name = "uPointLightColorRange[" + std::to_string(i) + "]";
        glm::vec4 value(points[i].radiance, points[i].range);
        glUniform4fv(glGetUniformLocation(p, name.c_str()), 1,
                     glm::value_ptr(value));
      }
    };
    if (instanced) {
      if (frame.headlights && models.size() > 1)
        throw std::runtime_error(
            "instanced batches require explicit shared point lights");
      lights(models.front());
      glFrontFace(stream.front().parity < 0 ? GL_CW : GL_CCW);
      for (const auto &entry : stream)
        if (entry.parity != stream.front().parity)
          throw std::runtime_error(
              "split mirrored instances into a separate batch");
      glBindBuffer(GL_ARRAY_BUFFER, geometry->instances);
      glBufferData(GL_ARRAY_BUFFER,
                   static_cast<GLsizeiptr>(stream.size() * sizeof(Instance)),
                   stream.data(), GL_STREAM_DRAW);
      materialMeasurement(
          m.name,
          textures.albedo->resident() + textures.normal->resident() +
              textures.mr->resident(),
          [&] {
            glDrawElementsInstanced(GL_TRIANGLES, geometry->count,
                                    GL_UNSIGNED_INT, nullptr,
                                    static_cast<GLsizei>(models.size()));
          });
      ++stats.draws;
    } else
      for (std::size_t i = 0; i < models.size(); ++i) {
        lights(models[i]);
        glFrontFace(stream[i].parity < 0 ? GL_CW : GL_CCW);
        glUniformMatrix4fv(glGetUniformLocation(p, "uModel"), 1, GL_FALSE,
                           stream[i].model);
        glUniformMatrix3fv(glGetUniformLocation(p, "uNormalMatrix"), 1,
                           GL_FALSE, stream[i].normal);
        scalar(p, "uModelParity", stream[i].parity);
        materialMeasurement(m.name,
                            textures.albedo->resident() +
                                textures.normal->resident() +
                                textures.mr->resident(),
                            [&] {
                              glDrawElements(GL_TRIANGLES, geometry->count,
                                             GL_UNSIGNED_INT, nullptr);
                            });
        ++stats.draws;
      }
    stats.triangles +=
        static_cast<std::uint64_t>(geometry->count / 3) * models.size();
  }
  return stats;
}
void Model::drawCurrent(float alpha) {
  draw::SavedState saved;
  draw::flush();
  glm::mat4 mv, projection;
  draw::read(draw::ViewMatrix, glm::value_ptr(mv));
  draw::read(draw::ProjectionMatrix, glm::value_ptr(projection));
  Frame frame;
  frame.viewProjection = projection;
  frame.alpha = alpha;
  frame.headlights = false;
  frame.lights = currentHeadlights;
  frame.sunDirection = glm::vec3(.2f, .5f, 1);
  frame.camera = glm::vec3(0);
  if (!runtimeIbl) {
    auto directory =
        shaderRoot.parent_path().parent_path() / "scenes/material_preview/ibl";
    if (std::filesystem::exists(directory / "cache.json"))
      runtimeIbl = std::make_unique<Ibl>(directory);
  }
  if (runtimeIbl) {
    runtimeIbl->pump();
    frame.environment = runtimeIbl.get();
    auto inverse = glm::inverse(worldView);
    frame.probeRotation = glm::mat3(inverse);
    frame.environmentRotation =
        glm::mat3(glm::rotate(glm::mat4(1), glm::radians(-90.f),
                              glm::vec3(1, 0, 0))) *
        frame.probeRotation;
    if (runtimeIbl->ready()) {
      if (!runtimeProbe)
        runtimeProbe = std::make_unique<ReflectionProbe>(shaderRoot);
      runtimeProbe->update(
          *runtimeIbl, glm::vec3(inverse * glm::vec4(currentCarCenter, 1)),
          probeScene,
          glm::mat3(glm::rotate(glm::mat4(1), glm::radians(-90.f),
                                glm::vec3(1, 0, 0))));
      frame.reflectionProbe = runtimeProbe->texture();
      frame.probeMaxLod = runtimeProbe->maxLod();
    }
  }

  const auto stats = draw(frame, {mv});
  draw::invalidateGpuState();
  if (impl_->legacy.empty())
    impl_->legacy.resize(impl_->source.submeshes.size());
  for (std::size_t i = 0; i < impl_->source.submeshes.size(); ++i) {
    const auto &sub = impl_->source.submeshes[i];
    const auto &m = impl_->materials[sub.material];
    if (m.path != mesh::MaterialPath::Legacy)
      continue;
    if (!impl_->legacy[i]) {
      std::vector<draw::MeshVertex> vertices;
      for (auto index : sub.indices) {
        const auto &v = sub.vertices[index];
        vertices.push_back({glm::make_vec3(v.position),
                            glm::make_vec3(v.normal), glm::make_vec2(v.uv)});
      }
      impl_->legacy[i] = std::make_shared<draw::Mesh>(vertices);
    }
    draw::activeTexture(GL_TEXTURE0);
    if (m.albedo.empty())
      draw::disable(GL_TEXTURE_2D);
    else {
      draw::enable(GL_TEXTURE_2D);
      draw::bindTexture(GL_TEXTURE_2D,
                        impl_->surfaces[sub.material].albedo->name());
    }
    draw::color(m.baseColor[0], m.baseColor[1], m.baseColor[2],
                m.baseColor[3] * alpha);
    draw::enable(draw::Lighting);
    impl_->legacy[i]->draw();
  }
  if (validation::run.capture) {
    validation::run.frameDraws += stats.draws;
    validation::run.frameTriangles += stats.triangles;
  }
}
} // namespace trackrs::pbr
