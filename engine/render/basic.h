// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
// Core-only legacy-look renderer. CPU geometry builders feed ordered GPU
// batches; static meshes own VAO/VBO storage and particles use fenced instance
// rings.
#include <filesystem>
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>
namespace trackrs::draw {
enum Parameter : unsigned {
  View = 0x100000,
  Projection,
  TextureTransform,
  ViewMatrix,
  ProjectionMatrix,
  CurrentColor,
  Lighting,
  Fog,
  FogDensity,
  FogColor,
  GlobalAmbient,
  TwoSided,
  Diffuse,
  Specular,
  LightPosition,
  MaterialColor,
  Shininess,
  AlphaTest,
  GenerateS,
  GenerateT,
  S,
  T,
  Quads,
  Polygon,
};
struct MeshVertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 uv;
};
struct Particle {
  glm::vec3 center, x, y;
  glm::vec4 left, right;
  glm::vec4 uv{0, 0, 1, 1};
  float swapUv = 0;
};
class Mesh {
  GLuint vao_ = 0, vbo_ = 0;
  GLsizei count_ = 0;

public:
  explicit Mesh(const std::vector<MeshVertex> &vertices);
  ~Mesh();
  Mesh(const Mesh &) = delete;
  Mesh &operator=(const Mesh &) = delete;
  void draw() const;
};
void initialize(const std::filesystem::path &shaders, bool reload);
void shutdown();
void flush();
void invalidateGpuState();
void finishFrame();
void worldSpace(bool enabled);
void finish();
void drawBuffer(unsigned buffer);
void beginBufferUpload();
void begin(unsigned topology);
void end();
void vertex(float x, float y, float z = 0);
void vertex(const float *p);
void uv(float x, float y);
void uv(const float *p);
void normal(const float *p);
void color(float r, float g, float b, float a = 1);
void color(const float *p);
void colorBytes(unsigned char r, unsigned char g, unsigned char b);
void matrix(unsigned mode);
void identity();
void push();
void pop();
void multiply(const float *p);
void translate(float x, float y, float z);
void scale(float x, float y, float z);
void rotate(float degrees, float x, float y, float z);
void ortho(double l, double r, double b, double t, double n, double f);
void frustum(double l, double r, double b, double t, double n, double f);
void read(unsigned property, float *out);
void enable(unsigned capability);
void disable(unsigned capability);
void activeTexture(unsigned unit);
void bindTexture(unsigned target, unsigned id);
void deleteTextures(GLsizei count, const GLuint *ids);
void blend(unsigned source, unsigned destination);
void depthMask(unsigned value);
void depthRange(double n, double f);
void depthFunc(unsigned value);
void cull(unsigned value);
void viewport(int x, int y, int width, int height);
void clear(unsigned mask);
void colorMask(unsigned r, unsigned g, unsigned b, unsigned a);
void lineWidth(float width);
void pointSize(float size);
struct AlphaCutoff {
  unsigned function;
  float reference;
};
void alpha(AlphaCutoff cutoff);
void fog(Parameter property, float value);
void fog(Parameter property, const float *value);
void light(Parameter property, const float *value);
void lightModel(Parameter property, const float *value);
void lightModel(Parameter property, int value);
void material(Parameter property, const float *value);
void material(Parameter property, float value);
void uvPlane(unsigned axis, const float *value);
struct IndexedMesh {
  unsigned topology;
  GLuint vertexBuffer, indexBuffer;
  int count;
  unsigned indexType;
  bool textured;
};
void indexed(const IndexedMesh &mesh);
void particles(const std::vector<Particle> &instances, bool points = false);
void registerSdf(GLuint texture);
void makeSdf(unsigned char *pixels, int width, int height, int channels);
class SavedState {
  struct Impl;
  std::unique_ptr<Impl> impl_;

public:
  SavedState();
  ~SavedState() noexcept(false);
  SavedState(const SavedState &) = delete;
};
} // namespace trackrs::draw
