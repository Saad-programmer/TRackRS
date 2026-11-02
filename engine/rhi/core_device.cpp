// SPDX-License-Identifier: GPL-2.0-or-later
#include "core_device.h"
#include <stdexcept>
namespace trackrs::rhi {
struct CoreDevice::Impl {
  SDL_Window *screen;
  SDL_GLContext core;
  std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> legacy{
      nullptr, &SDL_DestroyWindow};
  struct Context {
    SDL_GLContext id = nullptr;
    ~Context() {
      if (id)
        SDL_GL_DeleteContext(id);
    }
  } compatibility;
  DebugMessages debug;
  std::unique_ptr<ShaderProgram> program;
  std::unique_ptr<UniformBuffer> frame;
  StateCache cache;
  GLuint texture = 0, vao = 0;
  int width, height;
  bool hotReload;
  bool quad = false;
  GLuint configuredProgram = 0;
  struct Extent {
    int width, height;
  };
  Impl(SDL_Window *window, SDL_GLContext context, Extent extent, bool reload)
      : screen(window), core(context), width(extent.width),
        height(extent.height), hotReload(reload) {}
  ~Impl() {
    if (compatibility.id && glDebugMessageCallback) {
      SDL_GL_MakeCurrent(legacy.get(), compatibility.id);
      glDebugMessageCallback(nullptr, nullptr);
    }
    SDL_GL_MakeCurrent(screen, core);
    if (texture)
      glDeleteTextures(1, &texture);
    if (vao)
      glDeleteVertexArrays(1, &vao);
    frame.reset();
    program.reset();
    if (glDebugMessageCallback)
      glDebugMessageCallback(nullptr, nullptr);
  }
  void legacyCurrent() {
    if (SDL_GL_MakeCurrent(legacy.get(), compatibility.id) != 0)
      throw std::runtime_error(SDL_GetError());
  }
};
CoreDevice::CoreDevice(SDL_Window *screen, SDL_GLContext core, int width,
                       int height, const std::filesystem::path &shaders,
                       bool hotReload)
    : impl_(std::make_unique<Impl>(screen, core, Impl::Extent{width, height},
                                   hotReload)) {
  auto &self = *impl_;
  GLint profile = 0, major = 0, minor = 0;
  glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);
  glGetIntegerv(GL_MAJOR_VERSION, &major);
  glGetIntegerv(GL_MINOR_VERSION, &minor);
  if (!(profile & GL_CONTEXT_CORE_PROFILE_BIT) || major < 3 ||
      (major == 3 && minor < 3))
    throw std::runtime_error("visible context is not GL 3.3 core");
  self.debug.install();
  self.program = std::make_unique<ShaderProgram>(
      shaders / "present.vert", shaders / "present.frag", [](GLuint candidate) {
        const auto block = glGetUniformBlockIndex(candidate, "Frame");
        GLint size = 0;
        if (block != GL_INVALID_INDEX)
          glGetActiveUniformBlockiv(candidate, block,
                                    GL_UNIFORM_BLOCK_DATA_SIZE, &size);
        return size == 16 &&
               glGetUniformLocation(candidate, "legacyColor") >= 0;
      });
  int stereo = 0;
  SDL_GL_GetAttribute(SDL_GL_STEREO, &stereo);
  self.quad = stereo != 0;
  self.frame = std::make_unique<UniformBuffer>(16);
  glGenVertexArrays(1, &self.vao);
  glGenTextures(1, &self.texture);
  glBindTexture(GL_TEXTURE_2D, self.texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, nullptr);
  self.debug.check();
  SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                      SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
  self.legacy.reset(SDL_CreateWindow("TrackRS 2a legacy bridge", 0, 0, width,
                                     height,
                                     SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN));
  if (!self.legacy)
    throw std::runtime_error(SDL_GetError());
  self.compatibility.id = SDL_GL_CreateContext(self.legacy.get());
  if (!self.compatibility.id ||
      !gladLoadGL(reinterpret_cast<GLADloadfunc>(SDL_GL_GetProcAddress))) {
    SDL_GL_MakeCurrent(screen, core);
    throw std::runtime_error("legacy bridge context creation failed");
  }
  if (!glIsTexture(self.texture))
    throw std::runtime_error("core/legacy context sharing failed");
  self.debug.install();
  SDL_GL_SetSwapInterval(0);
  std::fprintf(stderr,
               "RHI: visible GL %d.%d core, glad, KHR_debug; isolated "
               "compatibility bridge until 2b\n",
               major, minor);
}
CoreDevice::~CoreDevice() = default;
void CoreDevice::setSwapInterval(int value) {
  auto &s = *impl_;
  SDL_GL_MakeCurrent(s.screen, s.core);
  if (SDL_GL_SetSwapInterval(value) != 0 && value == -1)
    SDL_GL_SetSwapInterval(1);
  s.legacyCurrent();
}
void CoreDevice::present(const std::function<void()> &capture) {
  auto &s = *impl_;
  for (int eye = 0; eye < (s.quad ? 2 : 1); ++eye) {
    GLint active = 0, bound = 0, readBuffer = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &bound);
    glGetIntegerv(GL_READ_BUFFER, &readBuffer);
    const GLenum buffer =
        s.quad ? (eye == 0 ? GL_BACK_LEFT : GL_BACK_RIGHT) : GL_BACK;
    glBindTexture(GL_TEXTURE_2D, s.texture);
    glReadBuffer(buffer);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, s.width, s.height);
    glReadBuffer(static_cast<GLenum>(readBuffer));
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(bound));
    glActiveTexture(static_cast<GLenum>(active));
    glFinish(); // Publish the GPU copy to the shared core context.
    if (SDL_GL_MakeCurrent(s.screen, s.core) != 0)
      throw std::runtime_error(SDL_GetError());
    try {
      if (s.hotReload)
        s.program->poll();
      s.cache.presentationState();
      s.cache.viewport(s.width, s.height);
      s.cache.program(s.program->id());
      s.cache.vertexArray(s.vao);
      s.cache.texture(s.texture);
      glDrawBuffer(buffer);
      const std::array<GLint, 4> extent{{0, 0, s.width, s.height}};
      s.frame->update(extent.data(), sizeof(extent));
      s.frame->bind(0);
      if (s.configuredProgram != s.program->id()) {
        glUniformBlockBinding(s.program->id(),
                              glGetUniformBlockIndex(s.program->id(), "Frame"),
                              0);
        glUniform1i(glGetUniformLocation(s.program->id(), "legacyColor"), 0);
        s.configuredProgram = s.program->id();
      }
      glDrawArrays(GL_TRIANGLES, 0, 3);
      s.debug.check();
      if (eye == (s.quad ? 1 : 0)) {
        if (capture)
          capture();
        SDL_GL_SwapWindow(s.screen);
        s.debug.check();
      }
    } catch (...) {
      s.legacyCurrent();
      throw;
    }
    s.legacyCurrent();
  }
}
} // namespace trackrs::rhi
