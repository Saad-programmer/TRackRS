#include <cstddef>
#include <cstdio>
// SPDX-License-Identifier: GPL-2.0-or-later
#include "movie.h"
#include <algorithm>
#include <cmath>
#include <pl_mpeg.h>
#include <stdexcept>
namespace trackrs {
struct Movie::Impl {
  std::vector<std::uint8_t> bytes, pixels;
  std::vector<float> pcm;
  std::unique_ptr<plm_t, decltype(&plm_destroy)> decoder{nullptr, &plm_destroy};
  unsigned width = 0, height = 0;
  bool fresh = false;
  bool invalidFrame = false;
};
Movie::Movie(std::vector<std::uint8_t> bytes)
    : impl_(std::make_unique<Impl>()) {
  if (bytes.empty() || bytes.size() > std::size_t{64} * 1024 * 1024)
    throw std::invalid_argument("invalid intro size");
  impl_->bytes = std::move(bytes);
  impl_->decoder.reset(
      plm_create_with_memory(impl_->bytes.data(), impl_->bytes.size(), 0));
  auto *decoder = impl_->decoder.get();
  if (!decoder || !plm_has_headers(decoder) ||
      !plm_get_num_video_streams(decoder))
    throw std::invalid_argument("intro must be an MPEG-1 program stream");
  impl_->width = static_cast<unsigned>(plm_get_width(decoder));
  impl_->height = static_cast<unsigned>(plm_get_height(decoder));
  if (!impl_->width || !impl_->height || impl_->width > 4096 ||
      impl_->height > 4096)
    throw std::invalid_argument("invalid intro dimensions");
  impl_->pixels.resize(static_cast<std::size_t>(impl_->width) * impl_->height *
                       3);
  impl_->pcm.reserve(96000);
  plm_set_video_decode_callback(
      decoder,
      [](plm_t *, plm_frame_t *frame, void *user) {
        auto &self = *static_cast<Impl *>(user);
        if (frame->width != self.width || frame->height != self.height) {
          self.invalidFrame = true;
          return;
        }
        plm_frame_to_rgb(frame, self.pixels.data(),
                         static_cast<int>(self.width * 3));
        self.fresh = true;
      },
      impl_.get());
  plm_set_audio_decode_callback(
      decoder,
      [](plm_t *, plm_samples_t *samples, void *user) {
        auto &self = *static_cast<Impl *>(user);
        const auto count = static_cast<std::size_t>(samples->count) * 2;
        if (self.pcm.size() + count <= 96000)
          self.pcm.insert(self.pcm.end(), samples->interleaved,
                          samples->interleaved + count);
      },
      impl_.get());
}
Movie::~Movie() = default;
bool Movie::advance(double seconds) {
  impl_->fresh = false;
  impl_->pcm.clear();
  if (std::isfinite(seconds) && seconds > 0)
    plm_decode(impl_->decoder.get(), std::min(seconds, .1));
  if (impl_->invalidFrame)
    throw std::runtime_error("intro dimensions changed during playback");
  return impl_->fresh;
}
bool Movie::ended() const { return plm_has_ended(impl_->decoder.get()) != 0; }
unsigned Movie::width() const { return impl_->width; }
unsigned Movie::height() const { return impl_->height; }
int Movie::sampleRate() const {
  return plm_get_samplerate(impl_->decoder.get());
}
const std::vector<std::uint8_t> &Movie::rgb() const { return impl_->pixels; }
const std::vector<float> &Movie::audio() const { return impl_->pcm; }
} // namespace trackrs
