// SPDX-License-Identifier: GPL-2.0-or-later
#define DOCTEST_CONFIG_IMPLEMENT
#include "engine/media/movie.h"
#include <doctest/doctest.h>
#include <fstream>
#include <iterator>
static std::string fixture;
TEST_CASE("pl_mpeg decodes generated RGB and PCM without external playback") {
  std::ifstream input(fixture, std::ios::binary);
  std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)), {});
  trackrs::Movie movie(std::move(bytes));
  CHECK(movie.width() == 64);
  CHECK(movie.height() == 48);
  CHECK(movie.sampleRate() == 44100);
  unsigned frames = 0;
  bool audio = false;
  for (unsigned i = 0; i < 100 && !movie.ended(); ++i) {
    if (movie.advance(.04))
      ++frames;
    audio = audio || !movie.audio().empty();
  }
  CHECK(frames > 0);
  CHECK(audio);
  CHECK(movie.ended());
  CHECK(movie.rgb().size() == 64 * 48 * 3);
  CHECK_THROWS(trackrs::Movie(std::vector<std::uint8_t>{1, 2, 3}));
}
int main(int argc, char **argv) {
  if (argc < 2)
    return 2;
  fixture = argv[1];
  doctest::Context ctx;
  return ctx.run();
}
