// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
/** Periodic terrain coordinates. Stateless/thread-safe; no ownership.
 * Bounded modulo replaces distance-proportional loops; negative values use floor.
 */
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace trackrs {
inline float wrapCoord(float x, float period) {
    if (!std::isfinite(x) || !std::isfinite(period) || period <= 0)
        throw std::invalid_argument("invalid periodic coordinate");
    if (x >= 0 && x < period) return x;
    double r = std::fmod(static_cast<double>(x), static_cast<double>(period));
    if (r < 0) r += period;
    const float result = static_cast<float>(r);
    return result < period ? result : std::nextafter(period, 0.0f);
}
struct PeriodicCell { int index; float fraction; };
inline PeriodicCell periodicCell(float coordinate, int period) {
    if (period <= 0 || !std::isfinite(coordinate)) throw std::invalid_argument("invalid periodic cell");
    const float base = std::floor(coordinate);
    return {static_cast<int>(wrapCoord(base, static_cast<float>(period))), coordinate - base};
}
inline int mapSample(float x, float period, int pixels, bool legacyWorldQuantization = false) {
    if (pixels <= 0) throw std::invalid_argument("empty periodic map");
    float p = wrapCoord(x, period);
    if (legacyWorldQuantization) p = std::floor(p);
    return static_cast<int>(std::min(std::lround(p * static_cast<float>(pixels) / period),
                                     static_cast<long>(pixels - 1)));
}
}
