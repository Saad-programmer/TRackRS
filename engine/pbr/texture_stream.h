// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "engine/assets/ktx2.h"
#include <glad/gl.h>
#include <memory>
namespace trackrs::pbr {
struct StreamedTexture {
  GLuint placeholder = 0, complete = 0;
  GLenum target = GL_TEXTURE_2D;
  bool ready = false, native = true;
  std::size_t residentBytes = 4;
  unsigned levels = 1;
  std::string error;
  ~StreamedTexture();
  GLuint id() const { return ready ? complete : placeholder; }
};
class TextureStream {
  struct Impl;
  std::unique_ptr<Impl> impl_;

public:
  explicit TextureStream(bool forceCpuDecode = false);
  ~TextureStream();
  TextureStream(const TextureStream &) = delete;
  std::shared_ptr<StreamedTexture> request(const assets::Bytes &bytes,
                                           assets::Semantic semantic);
  void pump();
  bool idle() const;
  std::size_t pboBytes() const;
  std::size_t uploads() const;
};
std::shared_ptr<TextureStream> textureStream();
} // namespace trackrs::pbr
