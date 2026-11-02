// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
/** Render-only snapshot interpolation. Values own no engine resources; reentrant.
 * Quaternion order x,y,z,w. Shortest-arc nlerp never writes simulation state.
 */
#include <algorithm>
#include <array>
#include <cmath>
namespace trackrs {
inline std::array<float, 4> interpolateRotation(std::array<float, 4> a,
                                               std::array<float, 4> b, float alpha) {
    alpha = std::clamp(alpha, 0.0f, 1.0f);
    float dot = 0;
    for (unsigned i = 0; i < 4; ++i) dot += a[i] * b[i];
    const float sign = dot < 0 ? -1.0f : 1.0f;
    float length = 0;
    for (unsigned i = 0; i < 4; ++i) { a[i] += (sign * b[i] - a[i]) * alpha; length += a[i] * a[i]; }
    if (length <= 0) return {0, 0, 0, 1};
    const float inverse = 1.0f / std::sqrt(length);
    for (auto& component : a) component *= inverse;
    return a;
}
}
