// SPDX-License-Identifier: GPL-2.0-or-later
#include "texture_stream.h"
#include <algorithm>
#include <cmath>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <map>
#include <mutex>
#include <stdexcept>
#include <thread>
namespace trackrs::pbr {
namespace {
bool extension(const char *name) {
  GLint n = 0;
  glGetIntegerv(GL_NUM_EXTENSIONS, &n);
  for (GLint i = 0; i < n; ++i)
    if (std::strcmp(reinterpret_cast<const char *>(
                        glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(i))),
                    name) == 0)
      return true;
  return false;
}
GLenum internal(assets::Format f) {
  switch (f) {
  case assets::Format::BC4:
    return GL_COMPRESSED_RED_RGTC1;
  case assets::Format::BC5:
    return GL_COMPRESSED_RG_RGTC2;
  case assets::Format::BC6H:
    return 0x8e8f;
  case assets::Format::BC7:
    return 0x8e8c;
  case assets::Format::BC7Srgb:
    return 0x8e8d;
  case assets::Format::RG16F:
    return GL_RG16F;
  }
  throw std::runtime_error("unsupported texture format");
}
struct Binding {
  GLint active = 0, tex2d = 0, cube = 0, pbo = 0, alignment = 0, row = 0;
  Binding() {
    glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &tex2d);
    glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &cube);
    glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &pbo);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
    glGetIntegerv(GL_UNPACK_ROW_LENGTH, &row);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  }
  ~Binding() {
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(tex2d));
    glBindTexture(GL_TEXTURE_CUBE_MAP, static_cast<GLuint>(cube));
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, static_cast<GLuint>(pbo));
    glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, row);
    glActiveTexture(static_cast<GLenum>(active));
  }
};
// OpenGL's buffer-offset API does not dereference this pointer on the CPU.
const void *byteOffset(std::size_t n) {
  return reinterpret_cast<const void *>(n);
} // NOLINT(performance-no-int-to-ptr)
} // namespace
StreamedTexture::~StreamedTexture() {
  glDeleteTextures(1, &placeholder);
  glDeleteTextures(1, &complete);
}
struct TextureStream::Impl {
  struct Job {
    std::weak_ptr<StreamedTexture> target;
    assets::Bytes bytes;
    assets::Semantic semantic;
  };
  struct Ready {
    std::weak_ptr<StreamedTexture> target;
    assets::TextureData data;
    std::string error;
    bool native = true;
  };
  struct Slot {
    GLuint pbo = 0;
    GLsync fence = nullptr;
    std::weak_ptr<StreamedTexture> target;
    std::size_t size = 0;
  };
  mutable std::mutex mutex;
  std::condition_variable wake;
  std::deque<Job> jobs;
  std::deque<Ready> ready;
  std::map<std::string, std::weak_ptr<StreamedTexture>> cache;
  std::array<Slot, 3> slots{};
  bool stopping = false, busy = false, bptc = false, anisotropic = false;
  float anisotropy = 1;
  std::thread worker;
  std::size_t uploaded = 0;
  explicit Impl(bool force) {
    bptc = !force && extension("GL_ARB_texture_compression_bptc");
    anisotropic = extension("GL_EXT_texture_filter_anisotropic") ||
                  extension("GL_ARB_texture_filter_anisotropic");
    if (anisotropic) {
      glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &anisotropy);
      anisotropy = std::min(8.f, anisotropy);
    }
    for (auto &slot : slots)
      glGenBuffers(1, &slot.pbo);
    worker = std::thread([this] { run(); });
  }
  ~Impl() {
    {
      std::lock_guard<std::mutex> lock(mutex);
      stopping = true;
      jobs.clear();
    }
    wake.notify_one();
    worker.join();
    for (auto &slot : slots) {
      if (slot.fence)
        glDeleteSync(slot.fence);
      glDeleteBuffers(1, &slot.pbo);
    }
  }
  void run() {
    for (;;) {
      Job job;
      {
        std::unique_lock<std::mutex> lock(mutex);
        wake.wait(lock, [this] { return stopping || !jobs.empty(); });
        if (stopping)
          return;
        job = std::move(jobs.front());
        jobs.pop_front();
        busy = true;
      }
      Ready result;
      result.target = job.target;
      try {
        result.data = assets::readKtx2(job.bytes, job.semantic);
        auto f = result.data.format;
        result.native = bptc || f == assets::Format::BC4 ||
                        f == assets::Format::BC5 || f == assets::Format::RG16F;
        if (!result.native) {
          unsigned w = result.data.width, h = result.data.height;
          std::size_t total = 0;
          for (auto &level : result.data.levels) {
            assets::Bytes decoded;
            auto size = assets::faceBytes(f, w, h);
            for (unsigned face = 0; face < result.data.faces; ++face) {
              assets::Bytes compressed(
                  level.begin() + static_cast<std::ptrdiff_t>(face * size),
                  level.begin() +
                      static_cast<std::ptrdiff_t>((face + 1) * size));
              auto pixels = assets::decode(f, w, h, compressed);
              auto old = decoded.size();
              if (f == assets::Format::BC6H) {
                decoded.resize(old + pixels.size() * sizeof(assets::Pixel));
                std::memcpy(decoded.data() + old, pixels.data(),
                            pixels.size() * sizeof(assets::Pixel));
              } else {
                decoded.reserve(old + pixels.size() * 4);
                for (auto &p : pixels)
                  for (auto v : p)
                    decoded.push_back(static_cast<std::uint8_t>(
                        std::lround(std::clamp(v, 0.f, 1.f) * 255)));
              }
            }
            total += decoded.size();
            if (total > 256 * 1024 * 1024)
              throw std::runtime_error(
                  "decoded texture exceeds staging budget");
            level = std::move(decoded);
            w = std::max(1u, w / 2);
            h = std::max(1u, h / 2);
          }
        }
      } catch (const std::exception &e) {
        result.error = e.what();
      }
      {
        std::lock_guard<std::mutex> lock(mutex);
        busy = false;
        ready.push_back(std::move(result));
      }
    }
  }
};
TextureStream::TextureStream(bool force)
    : impl_(std::make_unique<Impl>(force)) {}
TextureStream::~TextureStream() = default;
std::shared_ptr<StreamedTexture>
TextureStream::request(const assets::Bytes &bytes, assets::Semantic semantic) {
  std::uint64_t hash = 14695981039346656037ULL;
  for (auto c : bytes) {
    hash ^= c;
    hash *= 1099511628211ULL;
  }
  std::string key = std::to_string(hash) + ":" + std::to_string(bytes.size()) +
                    ":" + std::to_string(static_cast<unsigned>(semantic));
  auto &cached = impl_->cache[key];
  if (auto value = cached.lock())
    return value;
  Binding saved;
  auto ticket = std::make_shared<StreamedTexture>();
  ticket->target = semantic == assets::Semantic::Environment
                       ? GL_TEXTURE_CUBE_MAP
                       : GL_TEXTURE_2D;
  glGenTextures(1, &ticket->placeholder);
  glBindTexture(ticket->target, ticket->placeholder);
  std::array<unsigned char, 4> pixel =
      semantic == assets::Semantic::Normal
          ? std::array<unsigned char, 4>{128, 128, 255, 255}
      : semantic == assets::Semantic::Orm
          ? std::array<unsigned char, 4>{255, 204, 0, 255}
          : std::array<unsigned char, 4>{128, 128, 128, 255};
  for (unsigned f = 0; f < (ticket->target == GL_TEXTURE_CUBE_MAP ? 6u : 1u);
       ++f)
    glTexImage2D(
        ticket->target == GL_TEXTURE_CUBE_MAP
            ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + f
            : GL_TEXTURE_2D,
        0, semantic == assets::Semantic::Albedo ? GL_SRGB8_ALPHA8 : GL_RGBA8, 1,
        1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
  glTexParameteri(ticket->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(ticket->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(ticket->target, GL_TEXTURE_MAX_LEVEL, 0);
  ticket->residentBytes = ticket->target == GL_TEXTURE_CUBE_MAP ? 24 : 4;
  cached = ticket;
  {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->jobs.push_back({ticket, bytes, semantic});
  }
  impl_->wake.notify_one();
  return ticket;
}
void TextureStream::pump() {
  if (idle())
    return;
  Binding saved;
  for (auto &slot : impl_->slots) {
    if (slot.fence) {
      auto status = glClientWaitSync(slot.fence, 0, 0);
      if (status == GL_WAIT_FAILED)
        throw std::runtime_error("texture upload fence failed");
      if (status == GL_TIMEOUT_EXPIRED)
        continue;
      glDeleteSync(slot.fence);
      slot.fence = nullptr;
      if (auto t = slot.target.lock())
        t->ready = true;
      slot.target.reset();
    }
    Impl::Ready item;
    {
      std::lock_guard<std::mutex> lock(impl_->mutex);
      if (impl_->ready.empty())
        continue;
      item = std::move(impl_->ready.front());
      impl_->ready.pop_front();
    }
    auto t = item.target.lock();
    if (!t)
      continue;
    if (!item.error.empty()) {
      t->error = item.error;
      continue;
    }
    auto &data = item.data;
    if ((data.faces == 6) != (t->target == GL_TEXTURE_CUBE_MAP)) {
      t->error = "KTX2 target disagrees with semantic";
      continue;
    }
    std::size_t size = data.bytes();
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, slot.pbo);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, static_cast<GLsizeiptr>(size), nullptr,
                 GL_STREAM_DRAW);
    auto *mapped = static_cast<std::uint8_t *>(glMapBufferRange(
        GL_PIXEL_UNPACK_BUFFER, 0, static_cast<GLsizeiptr>(size),
        GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));
    if (!mapped)
      throw std::runtime_error("PBO map failed");
    std::size_t offset = 0;
    for (const auto &l : data.levels) {
      std::memcpy(mapped + offset, l.data(), l.size());
      offset += l.size();
    }
    if (!glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER))
      throw std::runtime_error("PBO contents invalidated");
    glGenTextures(1, &t->complete);
    glBindTexture(t->target, t->complete);
    offset = 0;
    unsigned w = data.width, h = data.height;
    std::size_t resident = 0;
    for (unsigned level = 0; level < data.levels.size(); ++level) {
      auto faceSize = data.levels[level].size() / data.faces;
      for (unsigned face = 0; face < data.faces; ++face) {
        auto target = t->target == GL_TEXTURE_CUBE_MAP
                          ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + face
                          : GL_TEXTURE_2D;
        if (item.native && data.format != assets::Format::RG16F) {
          glCompressedTexImage2D(
              target, static_cast<GLint>(level), internal(data.format),
              static_cast<GLsizei>(w), static_cast<GLsizei>(h), 0,
              static_cast<GLsizei>(faceSize), byteOffset(offset));
          resident += faceSize;
        } else {
          bool hdr = data.format == assets::Format::BC6H,
               lut = data.format == assets::Format::RG16F;
          glTexImage2D(target, static_cast<GLint>(level),
                       lut                           ? GL_RG16F
                       : hdr                         ? GL_RGBA16F
                       : assets::isSrgb(data.format) ? GL_SRGB8_ALPHA8
                                                     : GL_RGBA8,
                       static_cast<GLsizei>(w), static_cast<GLsizei>(h), 0,
                       lut ? GL_RG : GL_RGBA,
                       lut   ? GL_HALF_FLOAT
                       : hdr ? GL_FLOAT
                             : GL_UNSIGNED_BYTE,
                       byteOffset(offset));
          resident += std::size_t(w) * h * (lut ? 4 : hdr ? 8 : 4);
        }
        offset += faceSize;
      }
      w = std::max(1u, w / 2);
      h = std::max(1u, h / 2);
    }
    glTexParameteri(t->target, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(t->target, GL_TEXTURE_MAX_LEVEL,
                    static_cast<GLint>(data.levels.size() - 1));
    glTexParameteri(t->target, GL_TEXTURE_MIN_FILTER,
                    data.format == assets::Format::RG16F
                        ? GL_LINEAR
                        : GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(t->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    for (auto axis : {GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_TEXTURE_WRAP_R})
      glTexParameteri(t->target, axis,
                      t->target == GL_TEXTURE_CUBE_MAP ||
                              data.format == assets::Format::RG16F
                          ? GL_CLAMP_TO_EDGE
                          : GL_REPEAT);
    if (impl_->anisotropic && t->target == GL_TEXTURE_2D &&
        data.format != assets::Format::RG16F)
      glTexParameterf(t->target, GL_TEXTURE_MAX_ANISOTROPY_EXT,
                      impl_->anisotropy);
    t->residentBytes += resident;
    t->levels = static_cast<unsigned>(data.levels.size());
    t->native = item.native;
    slot.size = size;
    slot.target = t;
    slot.fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    glFlush();
    ++impl_->uploaded;
  }
}
bool TextureStream::idle() const {
  {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (impl_->busy || !impl_->jobs.empty() || !impl_->ready.empty())
      return false;
  }
  return std::none_of(impl_->slots.begin(), impl_->slots.end(),
                      [](const auto &s) { return s.fence != nullptr; });
}
std::size_t TextureStream::pboBytes() const {
  std::size_t n = 0;
  for (const auto &s : impl_->slots)
    n += s.size;
  return n;
}
std::size_t TextureStream::uploads() const { return impl_->uploaded; }
std::shared_ptr<TextureStream> textureStream() {
  static std::weak_ptr<TextureStream> stream;
  if (auto s = stream.lock())
    return s;
  auto s = std::make_shared<TextureStream>();
  stream = s;
  return s;
}
} // namespace trackrs::pbr
