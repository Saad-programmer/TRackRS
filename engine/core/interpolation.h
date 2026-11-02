// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
/** Render-only quaternion snapshot interpolation using glm types.
 * Values own no engine resources; reentrant, O(1), and never writes simulation state.
 */
#include <glm/gtc/quaternion.hpp>
namespace trackrs {
inline glm::quat interpolateRotation(glm::quat a, glm::quat b, float alpha) {
    alpha = glm::clamp(alpha, 0.0f, 1.0f);
    if (glm::dot(a, b) < 0) b = -b;
    const auto result = a * (1.0f - alpha) + b * alpha;
    return glm::dot(result, result) > 0 ? glm::normalize(result) : glm::quat(1, 0, 0, 0);
}
}
