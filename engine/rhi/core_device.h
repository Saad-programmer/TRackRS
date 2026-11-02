// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
/** Borrows one visible core context; owns core rendering resources. No other
 * SDL window/context or compatibility bridge is created. Main render thread. */
#include "rhi.h"
#include <SDL2/SDL.h>
#include <functional>
#include <memory>
namespace trackrs::rhi {
class CoreDevice {
  struct Impl;
  std::unique_ptr<Impl> impl_;

public:
  CoreDevice(SDL_Window *screen, SDL_GLContext core, int width, int height,
             const std::filesystem::path &shaderDirectory, bool hotReload);
  ~CoreDevice();
  CoreDevice(const CoreDevice &) = delete;
  CoreDevice &operator=(const CoreDevice &) = delete;
  void present(const std::function<void()> &capture);
  void setSwapInterval(int value);
};
} // namespace trackrs::rhi
