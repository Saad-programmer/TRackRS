// SPDX-License-Identifier: GPL-2.0-or-later
#include "basic.h"
#include "../rhi/rhi.h"
#include "validation.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <exception>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <limits>
#include <set>
#include <stdexcept>
namespace trackrs::draw {
namespace {
struct TextureState {
  bool enabled = false, generateS = false, generateT = false;
  GLuint id = 0;
  glm::vec4 planeS{1, 0, 0, 0}, planeT{0, 1, 0, 0};
  std::vector<glm::mat4> matrices{glm::mat4(1)};
};
struct State {
  unsigned mode = View, unit = 0;
  std::vector<glm::mat4> view{glm::mat4(1)}, projection{glm::mat4(1)};
  std::array<TextureState, 2> textures;
  glm::vec4 color{1}, fogColor{0}, ambient{.2f, .2f, .2f, 1}, diffuse{1},
      specular{1}, light{0, 0, 1, 0}, material{.8f, .8f, .8f, 1},
      materialSpecular{0, 0, 0, 1};
  glm::vec3 normal{0, 0, 1};
  glm::vec2 uv{0};
  bool lighting = false, fog = false, alpha = false, twoSided = false;
  float density = 1, shininess = 0, alphaReference = 0, pointSize = 1;
  unsigned alphaFunction = GL_ALWAYS;
};
struct StreamVertex {
  glm::vec4 position;
  glm::vec3 eye, normal;
  glm::vec4 color;
  glm::vec2 uv0, uv1;
  float sdf;
};
struct Slot {
  GLuint vao = 0, buffer = 0;
  GLsync fence = nullptr;
  std::size_t capacity = 0;
};
struct Renderer {
  State state;
  rhi::StateCache cache;
  std::unique_ptr<rhi::ShaderProgram> basic, particle;
  std::unique_ptr<rhi::UniformBuffer> frame;
  GLuint streamVao = 0, streamBuffer = 0, indexVao = 0;
  std::vector<StreamVertex> primitive, batch;
  unsigned topology = GL_TRIANGLES, batchTopology = GL_TRIANGLES;
  bool reload = false;
  std::array<Slot, 3> slots;
  unsigned nextSlot = 0;
  std::set<GLuint> sdf;
};
std::unique_ptr<Renderer> renderer;
Renderer &r() {
  if (!renderer)
    throw std::logic_error("core renderer not initialized");
  return *renderer;
}
std::vector<glm::mat4> &matrices() {
  auto &s = r().state;
  return s.mode == Projection         ? s.projection
         : s.mode == TextureTransform ? s.textures[s.unit].matrices
                                      : s.view;
}
struct Submission {
  unsigned mode;
  std::uint64_t count;
  std::uint64_t instances = 1;
};
void measured(Submission draw) {
  const auto mode = draw.mode;
  const auto count = draw.count;
  const auto instances = draw.instances;
  if (!validation::run.capture)
    return;
  ++validation::run.frameDraws;
  const auto triangles =
      mode == GL_TRIANGLES
          ? count / 3
          : ((mode == GL_TRIANGLE_STRIP || mode == GL_TRIANGLE_FAN) && count > 2
                 ? count - 2
                 : 0);
  validation::run.frameTriangles += triangles * instances;
}
void uniform(GLuint p, const char *name, int value) {
  glUniform1i(glGetUniformLocation(p, name), value);
}
void uniform(GLuint p, const char *name, float value) {
  glUniform1f(glGetUniformLocation(p, name), value);
}
void uniform(GLuint p, const char *name, const glm::vec4 &value) {
  glUniform4fv(glGetUniformLocation(p, name), 1, glm::value_ptr(value));
}
void uniform(GLuint p, const char *name, const glm::mat4 &value) {
  glUniformMatrix4fv(glGetUniformLocation(p, name), 1, GL_FALSE,
                     glm::value_ptr(value));
}
void prepare(GLuint p, bool baked) {
  auto &a = r();
  auto &s = a.state;
  a.cache.program(p);
  uniform(p, "uBaked", int(baked));
  uniform(p, "uView", s.view.back());
  uniform(p, "uProjection", s.projection.back());
  uniform(p, "uColor", s.color);
  uniform(p, "uLighting", int(s.lighting));
  uniform(p, "uFog", int(s.fog));
  uniform(p, "uFogColor", s.fogColor);
  uniform(p, "uDensity", s.density);
  uniform(p, "uAmbient", s.ambient);
  uniform(p, "uDiffuse", s.diffuse);
  uniform(p, "uSpecular", s.specular);
  uniform(p, "uLight", s.light);
  uniform(p, "uMaterial", s.material);
  uniform(p, "uMaterialSpecular", s.materialSpecular);
  uniform(p, "uShininess", s.shininess);
  uniform(p, "uTwoSided", int(s.twoSided));
  uniform(p, "uAlphaFunction", int(s.alpha ? s.alphaFunction : GL_ALWAYS));
  uniform(p, "uAlphaReference", s.alphaReference);
  uniform(p, "uPointSize", s.pointSize);
  for (unsigned i = 0; i < 2; ++i) {
    const std::string suffix = std::to_string(i);
    auto &t = s.textures[i];
    uniform(p, ("uTexture" + suffix).c_str(), int(i));
    uniform(p, ("uTextured" + suffix).c_str(), int(t.enabled && t.id != 0));
    uniform(p, ("uTextureMatrix" + suffix).c_str(), t.matrices.back());
    uniform(p, ("uPlaneS" + suffix).c_str(), t.planeS);
    uniform(p, ("uPlaneT" + suffix).c_str(), t.planeT);
    uniform(p, ("uGenerate" + suffix).c_str(), int(t.generateS || t.generateT));
  }
  uniform(p, "uSdf", int(a.sdf.count(s.textures[0].id) != 0));
  // An actual UBO is consumed by the core shader, not just configured.
  const glm::mat4 vp = s.projection.back() * s.view.back();
  a.frame->update(glm::value_ptr(vp), sizeof(vp));
  a.frame->bind(0);
  glUniformBlockBinding(p, glGetUniformBlockIndex(p, "Frame"), 0);
}
const void *bufferOffset(std::size_t bytes) {
  // OpenGL's VBO API requires an encoded byte offset in its pointer argument.
  // NOLINTNEXTLINE(performance-no-int-to-ptr)
  return reinterpret_cast<const void *>(bytes);
}
void streamLayout() {
  const GLsizei stride = sizeof(StreamVertex);
  const std::array<int, 7> sizes{{4, 3, 3, 4, 2, 2, 1}};
  const std::array<std::size_t, 7> offsets{
      {offsetof(StreamVertex, position), offsetof(StreamVertex, eye),
       offsetof(StreamVertex, normal), offsetof(StreamVertex, color),
       offsetof(StreamVertex, uv0), offsetof(StreamVertex, uv1),
       offsetof(StreamVertex, sdf)}};
  for (unsigned i = 0; i < sizes.size(); ++i) {
    glEnableVertexAttribArray(i);
    glVertexAttribPointer(i, sizes[i], GL_FLOAT, GL_FALSE, stride,
                          bufferOffset(offsets[i]));
  }
}
} // namespace
void initialize(const std::filesystem::path &shaders, bool reload) {
  renderer = std::make_unique<Renderer>();
  auto &a = r();
  a.reload = reload;
  const auto validate = [](GLuint program) {
    const auto block = glGetUniformBlockIndex(program, "Frame");
    GLint size = 0;
    if (block != GL_INVALID_INDEX)
      glGetActiveUniformBlockiv(program, block, GL_UNIFORM_BLOCK_DATA_SIZE,
                                &size);
    return size == static_cast<GLint>(sizeof(glm::mat4)) &&
           glGetUniformLocation(program, "uTexture0") >= 0;
  };
  a.basic = std::make_unique<rhi::ShaderProgram>(
      shaders / "basic.vert", shaders / "basic.frag", validate);
  a.particle = std::make_unique<rhi::ShaderProgram>(
      shaders / "particle.vert", shaders / "basic.frag", validate);
  a.frame = std::make_unique<rhi::UniformBuffer>(sizeof(glm::mat4));
  glGenVertexArrays(1, &a.streamVao);
  glGenBuffers(1, &a.streamBuffer);
  a.cache.vertexArray(a.streamVao);
  glBindBuffer(GL_ARRAY_BUFFER, a.streamBuffer);
  streamLayout();
  glGenVertexArrays(1, &a.indexVao);
  for (auto &slot : a.slots) {
    glGenVertexArrays(1, &slot.vao);
    glGenBuffers(1, &slot.buffer);
  }
  glEnable(GL_PROGRAM_POINT_SIZE);
}
void shutdown() {
  if (!renderer)
    return;
  auto &a = r();
  flush();
  for (auto &slot : a.slots) {
    if (slot.fence)
      glDeleteSync(slot.fence);
    glDeleteBuffers(1, &slot.buffer);
    glDeleteVertexArrays(1, &slot.vao);
  }
  glDeleteBuffers(1, &a.streamBuffer);
  glDeleteVertexArrays(1, &a.streamVao);
  glDeleteVertexArrays(1, &a.indexVao);
  renderer.reset();
}
void flush() {
  auto &a = r();
  if (a.batch.empty())
    return;
  prepare(a.basic->id(), true);
  a.cache.vertexArray(a.streamVao);
  glBindBuffer(GL_ARRAY_BUFFER, a.streamBuffer);
  glBufferData(GL_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(a.batch.size() * sizeof(StreamVertex)),
               a.batch.data(), GL_STREAM_DRAW);
  glDrawArrays(a.batchTopology, 0, static_cast<GLsizei>(a.batch.size()));
  measured({a.batchTopology, a.batch.size()});
  a.batch.clear();
}
void beginBufferUpload() {
  flush();
  r().cache.vertexArray(r().indexVao);
}
void finishFrame() {
  flush();
  auto &a = r();
  if (a.reload) {
    a.basic->poll();
    a.particle->poll();
  }
}
void begin(unsigned topology) {
  auto &a = r();
  if (!a.primitive.empty())
    throw std::logic_error("nested geometry builder");
  a.topology = topology;
}
void end() {
  auto &a = r();
  auto &v = a.primitive;
  unsigned output = (a.topology == GL_LINES || a.topology == GL_LINE_LOOP)
                        ? GL_LINES
                    : a.topology == GL_POINTS ? GL_POINTS
                                              : GL_TRIANGLES;
  if (output != a.batchTopology || a.batch.size() > 65536) {
    flush();
    a.batchTopology = output;
  }
  auto emit = [&](std::size_t i) { a.batch.push_back(v.at(i)); };
  switch (a.topology) {
  case GL_TRIANGLES:
  case GL_LINES:
  case GL_POINTS:
    a.batch.insert(a.batch.end(), v.begin(), v.end());
    break;
  case Quads:
    for (std::size_t i = 0; i + 3 < v.size(); i += 4) {
      emit(i);
      emit(i + 1);
      emit(i + 2);
      emit(i);
      emit(i + 2);
      emit(i + 3);
    }
    break;
  case GL_TRIANGLE_STRIP:
    for (std::size_t i = 2; i < v.size(); ++i) {
      emit(i - 2 + (i % 2));
      emit(i - 1 - (i % 2));
      emit(i);
    }
    break;
  case GL_TRIANGLE_FAN:
  case Polygon:
    for (std::size_t i = 2; i < v.size(); ++i) {
      emit(0);
      emit(i - 1);
      emit(i);
    }
    break;
  case GL_LINE_LOOP:
    for (std::size_t i = 0; i < v.size(); ++i) {
      emit(i);
      emit((i + 1) % v.size());
    }
    break;
  default:
    throw std::logic_error("unsupported CPU geometry topology");
  }
  v.clear();
}
void vertex(float x, float y, float z) {
  auto &a = r();
  auto &s = a.state;
  const glm::vec4 object(x, y, z, 1), eye = s.view.back() * object;
  StreamVertex v{};
  v.position = s.projection.back() * eye;
  v.eye = glm::vec3(eye);
  v.normal = glm::transpose(glm::inverse(glm::mat3(s.view.back()))) * s.normal;
  v.color = s.color;
  for (unsigned i = 0; i < 2; ++i) {
    auto &t = s.textures[i];
    glm::vec4 tc(s.uv, 0, 1);
    if (t.generateS)
      tc.x = glm::dot(t.planeS, object);
    if (t.generateT)
      tc.y = glm::dot(t.planeT, object);
    tc = t.matrices.back() * tc;
    (i ? v.uv1 : v.uv0) = glm::vec2(tc);
  }
  v.sdf = a.sdf.count(s.textures[0].id) ? 1.f : 0.f;
  a.primitive.push_back(v);
}
void vertex(const float *p) { vertex(p[0], p[1], p[2]); }
void uv(float x, float y) { r().state.uv = {x, y}; }
void uv(const float *p) { uv(p[0], p[1]); }
void normal(const float *p) { r().state.normal = glm::make_vec3(p); }
void color(float x, float y, float z, float w) {
  r().state.color = {x, y, z, w};
}
void color(const float *p) { r().state.color = glm::make_vec4(p); }
void colorBytes(unsigned char x, unsigned char y, unsigned char z) {
  color(float(x) / 255, float(y) / 255, float(z) / 255);
}
void matrix(unsigned mode) { r().state.mode = mode; }
void identity() { matrices().back() = glm::mat4(1); }
void push() { matrices().push_back(matrices().back()); }
void pop() {
  auto &m = matrices();
  if (m.size() < 2)
    throw std::logic_error("transform stack underflow");
  m.pop_back();
}
void multiply(const float *p) { matrices().back() *= glm::make_mat4(p); }
void translate(float x, float y, float z) {
  matrices().back() = glm::translate(matrices().back(), {x, y, z});
}
void scale(float x, float y, float z) {
  matrices().back() = glm::scale(matrices().back(), {x, y, z});
}
void rotate(float a, float x, float y, float z) {
  matrices().back() = glm::rotate(matrices().back(), glm::radians(a),
                                  glm::normalize(glm::vec3(x, y, z)));
}
void ortho(double l, double rr, double b, double t, double n, double f) {
  matrices().back() *= glm::mat4(glm::ortho(l, rr, b, t, n, f));
}
void frustum(double l, double rr, double b, double t, double n, double f) {
  matrices().back() *= glm::mat4(glm::frustum(l, rr, b, t, n, f));
}
void read(unsigned property, float *out) {
  auto &s = r().state;
  switch (property) {
  case ViewMatrix:
    std::memcpy(out, glm::value_ptr(s.view.back()), 64);
    break;
  case ProjectionMatrix:
    std::memcpy(out, glm::value_ptr(s.projection.back()), 64);
    break;
  case CurrentColor:
    std::memcpy(out, glm::value_ptr(s.color), 16);
    break;
  case GL_POINT_SIZE:
    *out = s.pointSize;
    break;
  default:
    throw std::logic_error("unknown CPU state query");
  }
}
void capability(unsigned cap, bool value) {
  auto &s = r().state;
  bool *target = nullptr;
  switch (cap) {
  case GL_TEXTURE_2D:
    target = &s.textures[s.unit].enabled;
    break;
  case Lighting:
    target = &s.lighting;
    break;
  case Fog:
    target = &s.fog;
    break;
  case AlphaTest:
    target = &s.alpha;
    break;
  case GenerateS:
    s.textures[s.unit].generateS = value;
    return;
  case GenerateT:
    s.textures[s.unit].generateT = value;
    return;
  default:
    flush();
    if (value)
      glEnable(cap);
    else
      glDisable(cap);
    return;
  }
  if (*target != value) {
    flush();
    *target = value;
  }
}
void enable(unsigned c) { capability(c, true); }
void disable(unsigned c) { capability(c, false); }
void activeTexture(unsigned unit) {
  const unsigned index = unit - GL_TEXTURE0;
  if (index >= 2)
    throw std::out_of_range("texture unit");
  r().state.unit = index;
  glActiveTexture(unit);
}
void bindTexture(unsigned target, unsigned id) {
  auto &s = r().state;
  if (target == GL_TEXTURE_2D && s.textures[s.unit].id != id) {
    flush();
    s.textures[s.unit].id = id;
  }
  glBindTexture(target, id);
}
void deleteTextures(GLsizei count, const GLuint *ids) {
  if (renderer) {
    flush();
    for (int i = 0; i < count; ++i) {
      r().sdf.erase(ids[i]);
      for (auto &t : r().state.textures)
        if (t.id == ids[i])
          t.id = 0;
    }
  }
  glDeleteTextures(count, ids);
}
void blend(unsigned a, unsigned b) {
  flush();
  glBlendFunc(a, b);
}
void depthMask(unsigned v) {
  flush();
  glDepthMask(static_cast<GLboolean>(v));
}
void depthRange(double n, double f) {
  flush();
  glDepthRange(n, f);
}
void depthFunc(unsigned v) {
  flush();
  glDepthFunc(v);
}
void cull(unsigned v) {
  flush();
  glCullFace(v);
}
void viewport(int x, int y, int w, int h) {
  flush();
  glViewport(x, y, w, h);
}
void clear(unsigned mask) {
  flush();
  glClear(mask);
}
void colorMask(unsigned a, unsigned b, unsigned c, unsigned d) {
  flush();
  glColorMask(static_cast<GLboolean>(a), static_cast<GLboolean>(b),
              static_cast<GLboolean>(c), static_cast<GLboolean>(d));
}
void lineWidth(float v) {
  flush();
  GLfloat range[2];
  glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, range);
  glLineWidth(std::clamp(v, range[0], range[1]));
}
void pointSize(float v) {
  flush();
  r().state.pointSize = v;
}
void alpha(AlphaCutoff cutoff) {
  flush();
  r().state.alphaFunction = cutoff.function;
  r().state.alphaReference = cutoff.reference;
}
void fog(Parameter p, float v) {
  flush();
  if (p == FogDensity)
    r().state.density = v;
}
void fog(Parameter p, const float *v) {
  if (p == FogColor) {
    flush();
    r().state.fogColor = glm::make_vec4(v);
  }
}
void light(Parameter p, const float *v) {
  flush();
  auto &s = r().state;
  if (p == Diffuse)
    s.diffuse = glm::make_vec4(v);
  else if (p == Specular)
    s.specular = glm::make_vec4(v);
  else if (p == LightPosition)
    s.light = s.view.back() * glm::make_vec4(v);
}
void lightModel(Parameter p, const float *v) {
  flush();
  if (p == GlobalAmbient)
    r().state.ambient = glm::make_vec4(v);
}
void lightModel(Parameter p, int v) {
  flush();
  if (p == TwoSided)
    r().state.twoSided = v != 0;
}
void material(Parameter p, const float *v) {
  flush();
  if (p == MaterialColor)
    r().state.material = glm::make_vec4(v);
  else if (p == Specular)
    r().state.materialSpecular = glm::make_vec4(v);
}
void material(Parameter p, float v) {
  flush();
  if (p == Shininess)
    r().state.shininess = v;
}
void uvPlane(unsigned axis, const float *value) {
  auto &t = r().state.textures[r().state.unit];
  (axis == S ? t.planeS : t.planeT) = glm::make_vec4(value);
}
Mesh::Mesh(const std::vector<MeshVertex> &vertices)
    : count_(static_cast<GLsizei>(vertices.size())) {
  flush();
  glGenVertexArrays(1, &vao_);
  glGenBuffers(1, &vbo_);
  r().cache.vertexArray(vao_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  glBufferData(GL_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(vertices.size() * sizeof(MeshVertex)),
               vertices.data(), GL_STATIC_DRAW);
  for (unsigned location : {0u, 2u, 4u})
    glEnableVertexAttribArray(location);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                        bufferOffset(offsetof(MeshVertex, position)));
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                        bufferOffset(offsetof(MeshVertex, normal)));
  glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                        bufferOffset(offsetof(MeshVertex, uv)));
}
Mesh::~Mesh() {
  if (renderer)
    renderer->cache.invalidate();
  glDeleteBuffers(1, &vbo_);
  glDeleteVertexArrays(1, &vao_);
}
void Mesh::draw() const {
  flush();
  auto &a = r();
  prepare(a.basic->id(), false);
  a.cache.vertexArray(vao_);
  glDrawArrays(GL_TRIANGLES, 0, count_);
  measured({GL_TRIANGLES, static_cast<unsigned>(count_)});
}
void indexed(const IndexedMesh &mesh) {
  const auto topology = mesh.topology;
  const auto vb = mesh.vertexBuffer;
  const auto ib = mesh.indexBuffer;
  const auto count = mesh.count;
  const auto type = mesh.indexType;
  const auto textured = mesh.textured;
  flush();
  auto &a = r();
  prepare(a.basic->id(), false);
  a.cache.vertexArray(a.indexVao);
  glBindBuffer(GL_ARRAY_BUFFER, vb);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib);
  for (unsigned i = 0; i < 7; ++i)
    glDisableVertexAttribArray(i);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, textured ? 20 : 12,
                        bufferOffset(textured ? 8 : 0));
  if (textured) {
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, 20, nullptr);
  } else
    glVertexAttrib2f(4, a.state.uv.x, a.state.uv.y);
  glVertexAttrib3fv(2, glm::value_ptr(a.state.normal));
  glDrawElements(topology, count, type, nullptr);
  measured({topology, static_cast<unsigned>(count)});
}
void particles(const std::vector<Particle> &instances, bool points) {
  if (instances.empty())
    return;
  flush();
  auto &a = r();
  auto &slot = a.slots[a.nextSlot++ % a.slots.size()];
  if (slot.fence) {
    GLenum result;
    do {
      result =
          glClientWaitSync(slot.fence, GL_SYNC_FLUSH_COMMANDS_BIT, 1000000);
    } while (result == GL_TIMEOUT_EXPIRED);
    if (result == GL_WAIT_FAILED)
      throw std::runtime_error("particle ring fence failed");
    glDeleteSync(slot.fence);
    slot.fence = nullptr;
  }
  if (instances.size() > 1048576)
    throw std::length_error("particle instance limit");
  prepare(a.particle->id(), false);
  uniform(a.particle->id(), "uPoints", int(points));
  a.cache.vertexArray(slot.vao);
  glBindBuffer(GL_ARRAY_BUFFER, slot.buffer);
  const auto bytes = instances.size() * sizeof(Particle);
  if (bytes > slot.capacity) {
    slot.capacity = std::max(bytes, std::size_t(65536));
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(slot.capacity),
                 nullptr, GL_STREAM_DRAW);
  }
  void *mapped =
      glMapBufferRange(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(bytes),
                       GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
  if (!mapped)
    throw std::runtime_error("particle ring map failed");
  std::memcpy(mapped, instances.data(), bytes);
  if (!glUnmapBuffer(GL_ARRAY_BUFFER))
    throw std::runtime_error("particle ring unmap failed");
  const std::array<int, 7> sizes{{3, 3, 3, 4, 4, 4, 1}};
  const std::array<std::size_t, 7> offsets{
      {offsetof(Particle, center), offsetof(Particle, x), offsetof(Particle, y),
       offsetof(Particle, left), offsetof(Particle, right),
       offsetof(Particle, uv), offsetof(Particle, swapUv)}};
  for (unsigned i = 0; i < sizes.size(); ++i) {
    glEnableVertexAttribArray(i);
    glVertexAttribPointer(i, sizes[i], GL_FLOAT, GL_FALSE, sizeof(Particle),
                          bufferOffset(offsets[i]));
    glVertexAttribDivisor(i, 1);
  }
  glDrawArraysInstanced(points ? GL_POINTS : GL_TRIANGLES, 0, points ? 1 : 6,
                        static_cast<GLsizei>(instances.size()));
  measured({static_cast<unsigned>(points ? GL_POINTS : GL_TRIANGLES),
            points ? 1u : 6u, instances.size()});
  slot.fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}
void registerSdf(GLuint texture) { r().sdf.insert(texture); }
void makeSdf(unsigned char *pixels, int width, int height, int channels) {
  if (width <= 0 || height <= 0 || width > 16384 || height > 16384 ||
      static_cast<std::size_t>(width) * static_cast<std::size_t>(height) >
          16777216)
    throw std::invalid_argument("SDF dimensions exceed limit");
  if (channels != 4)
    throw std::invalid_argument("SDF source must be RGBA");
  const std::vector<unsigned char> source(
      pixels, pixels + static_cast<std::size_t>(width) *
                           static_cast<std::size_t>(height) *
                           static_cast<std::size_t>(channels));
  constexpr int radius = 6;
  for (int y = 0; y < height; ++y)
    for (int x = 0; x < width; ++x) {
      const auto index =
          (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
           static_cast<std::size_t>(x)) *
              static_cast<std::size_t>(channels) +
          3;
      const bool inside = source[index] >= 128;
      float distance = float(radius);
      for (int dy = -radius; dy <= radius; ++dy)
        for (int dx = -radius; dx <= radius; ++dx) {
          const int xx = x + dx, yy = y + dy;
          if (xx < 0 || xx >= width || yy < 0 || yy >= height)
            continue;
          const bool other = source[(static_cast<std::size_t>(yy) *
                                         static_cast<std::size_t>(width) +
                                     static_cast<std::size_t>(xx)) *
                                        static_cast<std::size_t>(channels) +
                                    3] >= 128;
          if (other != inside)
            distance = std::min(distance, std::sqrt(float(dx * dx + dy * dy)));
        }
      // Half a texel boundary with original coverage preserving edge placement.
      const float signedDistance = (inside ? distance - .5f : .5f - distance);
      pixels[index] = static_cast<unsigned char>(std::lround(
          std::clamp(.5f + signedDistance / (2 * radius), 0.f, 1.f) * 255.f));
    }
}
struct SavedState::Impl {
  State state;
  std::array<GLboolean, 3> enabled;
  GLint blendSource = 0, blendDestination = 0, unpack = 4;
  Impl()
      : state(r().state),
        enabled{{glIsEnabled(GL_DEPTH_TEST), glIsEnabled(GL_CULL_FACE),
                 glIsEnabled(GL_BLEND)}} {
    flush();
    glGetIntegerv(GL_BLEND_SRC_RGB, &blendSource);
    glGetIntegerv(GL_BLEND_DST_RGB, &blendDestination);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpack);
  }
};
SavedState::SavedState() : impl_(std::make_unique<Impl>()) {}
SavedState::~SavedState() noexcept(false) {
  if (!renderer)
    return;
  std::exception_ptr failure;
  if (std::uncaught_exceptions())
    renderer->batch.clear();
  else {
    try {
      flush();
    } catch (...) {
      failure = std::current_exception();
    }
  }
  renderer->state = std::move(impl_->state);
  unsigned i = 0;
  for (GLenum cap : {GL_DEPTH_TEST, GL_CULL_FACE, GL_BLEND}) {
    if (impl_->enabled[i++])
      glEnable(cap);
    else
      glDisable(cap);
  }
  glBlendFunc(static_cast<GLenum>(impl_->blendSource),
              static_cast<GLenum>(impl_->blendDestination));
  glPixelStorei(GL_UNPACK_ALIGNMENT, impl_->unpack);
  for (unsigned u = 0; u < 2; ++u) {
    glActiveTexture(GL_TEXTURE0 + u);
    glBindTexture(GL_TEXTURE_2D, r().state.textures[u].id);
  }
  glActiveTexture(GL_TEXTURE0 + r().state.unit);
  if (failure)
    std::rethrow_exception(failure);
}
} // namespace trackrs::draw
