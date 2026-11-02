// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
/** Owns the temporary legacy compatibility context and core presentation RHI.
 * Borrows the visible core window/context; they outlive this object. Main
 * thread only. One GPU copy and core triangle per eye/frame; no CPU pixel
 * transfer. The bridge is removed in 2b, preserving a playable checkpoint at
 * gate 2a.
 */
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
