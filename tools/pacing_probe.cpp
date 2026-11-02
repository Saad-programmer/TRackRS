// SPDX-License-Identifier: GPL-2.0-or-later
/** Synthetic idle-loop pacing measurement, not a game/GPU benchmark.
 * Owns only a deadline clock; single thread. CPU clock interpretation is POSIX.
 * Compare cap 0 and 120 on the same host; each probe runs at least two seconds.
 */
#include "engine/core/frame_pacer.h"
#include <charconv>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <string_view>
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    unsigned cap = 0;
    const std::string_view argument(argv[1]);
    const auto parsed = std::from_chars(argument.data(), argument.data() + argument.size(), cap);
    if (parsed.ec != std::errc() || parsed.ptr != argument.data() + argument.size() || cap > 1000) return 2;
    using Clock = std::chrono::steady_clock;
    const auto start = Clock::now();
    const auto cpuStart = std::clock();
    std::uint64_t frames = 0;
    trackrs::FramePacer pacer;
    while (Clock::now() - start < std::chrono::seconds(2)) {
        pacer.begin();
        ++frames;
        pacer.wait(cap);
    }
    const double elapsed = std::chrono::duration<double>(Clock::now() - start).count();
    const double cpuMs = 1000.0 * static_cast<double>(std::clock() - cpuStart) / CLOCKS_PER_SEC;
    std::cout << "{\"cap\":" << cap << ",\"frames\":" << frames << ",\"wall_seconds\":" << elapsed
              << ",\"cpu_ms\":" << cpuMs << ",\"average_frame_ms\":" << elapsed * 1000 / static_cast<double>(frames) << "}\n";
}
