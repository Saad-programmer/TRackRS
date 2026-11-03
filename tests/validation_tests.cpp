// SPDX-License-Identifier: GPL-2.0-or-later
/** CPU regression tests. Owns temporary replay files; single-threaded, no GL. */
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "validation.h"
#include <cmath>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
static unsigned checks = 0;
static void check(bool result) {
    ++checks;
    CHECK_MESSAGE(result, "validation check ", checks);
}
template<class F> static void rejects(F f) {
    bool rejected = false;
    try { f(); } catch (const std::exception&) { rejected = true; }
    check(rejected);
}
TEST_CASE("legacy validation options, replay, hash and metrics") {
    try {
        char name[] = "trackrs", mode[] = "--headless", key[] = "--frames", zero[] = "0";
        char* good[] = {name, mode};
        check(validation::parse(2, good).enabled);
        char* bad[] = {name, mode, key, zero};
        rejects([&] { validation::parse(4, bad); });
        rejects([&] { validation::parse(3, bad); });
        char bogus[] = "--bogus"; char* unknown[] = {name, bogus};
        rejects([&] { validation::parse(2, unknown); });
        auto inputs = validation::readReplay("");
        check(validation::inputAt(inputs, 100).throttle == 1);
        inputs.push_back({10, .5f, -.1f, 1, 0});
        check(validation::inputAt(inputs, 9).throttle == 1);
        check(validation::inputAt(inputs, 10).throttle == .5f);
        check(validation::inputAt(inputs, 1000).brake == 1);
        std::uint64_t a = 14695981039346656037ULL, b = a, c = a;
        validation::hashFloat(a, 1.f); validation::hashFloat(b, 1.f); validation::hashFloat(c, 2.f);
        check(a == b && a != c);
        check(a == 0x4b72477f9c5c2f98ULL); // FNV-1a of IEEE754 1.0 in little-endian order.
        rejects([&] { validation::hashFloat(a, std::numeric_limits<float>::quiet_NaN()); });
        check(validation::percentile({4, 1, 3, 2}, .5) == 2);
        check(validation::percentile({4, 1, 3, 2}, .99) == 4);
        rejects([] { validation::percentile({}, .5); });
        check(validation::quote("a\n\"\\") == "\"a\\u000a\\\"\\\\\"");
        const auto path = std::filesystem::temp_directory_path() /
            ("trackrs-validation-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".csv");
        { std::ofstream out(path); out << "# valid replay\n0,0.5,-0.2,0,0\n10,1,0,1,0\n"; }
        const auto parsed = validation::readReplay(path.string());
        check(parsed.size() == 2);
        check(validation::inputAt(parsed, 10).brake == 1);
        for (const char* invalid : {"", "1,1,0,0,0\n", "0,2,0,0,0\n", "0,1,0,0,0\n0,0,0,0,0\n", "0,1,0,0,0,extra\n"}) {
            { std::ofstream out(path); out << invalid; }
            rejects([&] { validation::readReplay(path.string()); });
        }
        std::filesystem::remove(path);
        std::cout << checks << " validation checks passed\n";
        CHECK(checks == 22);
    } catch (const std::exception& e) { FAIL(e.what()); }
}

TEST_CASE("world renderer quality and benchmark viewport options validate ranges") {
  char name[]="trackrs", renderer[]="--renderer", modern[]="pbr", quality[]="--quality", high[]="high";
  char width[]="--width", w[]="1920", height[]="--height", h[]="1080";
  char *args[]={name,renderer,modern,quality,high,width,w,height,h};
  auto options=validation::parse(9,args);
  CHECK(options.renderer=="pbr");CHECK(options.quality=="high");CHECK(options.width==1920);CHECK(options.height==1080);
  char invalid[]="999999";char *tooBig[]={name,width,invalid};
  CHECK_THROWS(validation::parse(3,tooBig));
  char unknown[]="cinematic";char *badQuality[]={name,quality,unknown};
  CHECK_THROWS(validation::parse(3,badQuality));
  char *badRenderer[]={name,renderer,unknown};CHECK_THROWS(validation::parse(3,badRenderer));
}
