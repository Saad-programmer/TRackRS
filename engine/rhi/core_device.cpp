// SPDX-License-Identifier: GPL-2.0-or-later
#include "core_device.h"
#include "../render/basic.h"
#include <cstdio>
#include <stdexcept>
namespace trackrs::rhi {
struct CoreDevice::Impl {
  SDL_Window *window;
  SDL_GLContext context;
  DebugMessages debug;
  Impl(SDL_Window *w, SDL_GLContext c) : window(w), context(c) {}
  ~Impl() {
    draw::shutdown();
    glDebugMessageCallback(nullptr, nullptr);
  }
};
CoreDevice::CoreDevice(SDL_Window *window, SDL_GLContext context, int, int,
                       const std::filesystem::path &shaders, bool reload)
    : impl_(std::make_unique<Impl>(window, context)) {
  GLint profile = 0, major = 0, minor = 0;
  glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);
  glGetIntegerv(GL_MAJOR_VERSION, &major);
  glGetIntegerv(GL_MINOR_VERSION, &minor);
  if (!(profile & GL_CONTEXT_CORE_PROFILE_BIT) || major < 3 ||
      (major == 3 && minor < 3))
    throw std::runtime_error("OpenGL 3.3 core required");
  impl_->debug.install();
  draw::initialize(shaders, reload);
  impl_->debug.check();
  std::fprintf(stderr,
               "RHI: GL %d.%d core only; retained meshes, batched UI, "
               "instanced particle ring; no compatibility context\n",
               major, minor);
}
CoreDevice::~CoreDevice() = default;
void CoreDevice::setSwapInterval(int interval) {
  if (SDL_GL_SetSwapInterval(interval) != 0 && interval == -1)
    SDL_GL_SetSwapInterval(1);
}
void CoreDevice::present(const std::function<void()> &capture) {
  draw::finishFrame();
  impl_->debug.check();
  if (capture)
    capture();
  SDL_GL_SwapWindow(impl_->window);
  impl_->debug.check();
}
} // namespace trackrs::rhi
