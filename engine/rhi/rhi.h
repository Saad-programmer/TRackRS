// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
/** GL 3.3 RHI primitives. Own program/buffer names; caller keeps the owning
 * context current until destruction. Main render thread only. Cached state
 * changes avoid redundant GL calls; hot reload retains the last linked program.
 */
#include <array>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <functional>
#include <glad/gl.h>
#include <map>
#include <string>
#include <vector>
namespace trackrs::rhi {
struct DebugMessages {
  std::atomic<unsigned> errors{0};
  void install();
  void check() const;
};
class StateCache {
  GLuint program_ = ~0u, vao_ = ~0u, texture_ = ~0u;
  std::array<int, 4> viewport_{{-1, -1, -1, -1}};
  bool initialized_ = false;
  unsigned changes_ = 0;

public:
  void invalidate() noexcept {
    program_ = vao_ = texture_ = ~0u;
    initialized_ = false;
    viewport_ = {-1, -1, -1, -1};
  }
  void program(GLuint value);
  void vertexArray(GLuint value);
  void texture(GLuint value);
  void viewport(int width, int height);
  void presentationState();
  unsigned changes() const noexcept { return changes_; }
};
class ShaderProgram {
  GLuint id_ = 0;
  std::filesystem::path vertex_, fragment_;
  std::map<std::filesystem::path, std::filesystem::file_time_type> watched_;
  std::vector<std::string> defines_;
  std::chrono::steady_clock::time_point nextPoll_{};
  std::string error_;
  std::function<bool(GLuint)> validate_;

public:
  ShaderProgram(std::filesystem::path vertex, std::filesystem::path fragment,
                std::function<bool(GLuint)> validate = {},
                std::vector<std::string> defines = {});
  ~ShaderProgram();
  ShaderProgram(const ShaderProgram &) = delete;
  ShaderProgram &operator=(const ShaderProgram &) = delete;
  bool poll(bool force = false);
  GLuint id() const noexcept { return id_; }
  const std::string &error() const noexcept { return error_; }
};
class UniformBuffer {
  GLuint id_ = 0;
  std::size_t size_;

public:
  explicit UniformBuffer(std::size_t bytes);
  ~UniformBuffer();
  UniformBuffer(const UniformBuffer &) = delete;
  UniformBuffer &operator=(const UniformBuffer &) = delete;
  void update(const void *bytes, std::size_t size);
  void bind(unsigned binding) const;
  GLuint id() const noexcept { return id_; }
};
} // namespace trackrs::rhi
