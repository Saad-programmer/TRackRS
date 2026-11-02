// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
/** Owns an in-memory MPEG-1 program-stream decoder and decoded RGB/PCM storage.
 * Main thread only; callbacks borrow storage until the next advance. Bounded
 * input and audio buffering; decoding work is proportional to elapsed movie
 * frames.
 */
#include <cstdint>
#include <memory>
#include <vector>
namespace trackrs {
class Movie {
  struct Impl;
  std::unique_ptr<Impl> impl_;

public:
  explicit Movie(std::vector<std::uint8_t> bytes);
  ~Movie();
  Movie(const Movie &) = delete;
  Movie &operator=(const Movie &) = delete;
  bool advance(double seconds);
  bool ended() const;
  unsigned width() const;
  unsigned height() const;
  int sampleRate() const;
  const std::vector<std::uint8_t> &rgb() const;
  const std::vector<float> &audio() const;
};
} // namespace trackrs
