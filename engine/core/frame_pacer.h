// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
/** Main-thread frame deadline; owns only a clock timestamp.
 * Sleeps most of the interval, yields for the final 150us. No steady allocations.
 */
#include <chrono>
#include <thread>
namespace trackrs {
class FramePacer {
    using Clock = std::chrono::steady_clock;
    Clock::time_point start_ = Clock::now();
public:
    void begin() noexcept { start_ = Clock::now(); }
    void wait(unsigned fps) const {
        if (!fps) return;
        const auto deadline = start_ + std::chrono::duration_cast<Clock::duration>(
            std::chrono::duration<double>(1.0 / static_cast<double>(fps)));
        const auto sleepUntil = deadline - std::chrono::microseconds(150);
        if (Clock::now() < sleepUntil) std::this_thread::sleep_until(sleepUntil);
        while (Clock::now() < deadline) std::this_thread::yield();
    }
};
}
