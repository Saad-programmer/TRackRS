// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
/** Owns one simulation's fractional time. Value lifetime; single-thread-only.
 * Retains legacy float arithmetic and step order; bounded catch-up prevents stalls.
 */
#include <cmath>
#include <cstddef>
#include <stdexcept>
namespace trackrs {
class FixedStep {
    float remainder_ = 0;
    float step_;
    std::size_t limit_;
    double dropped_ = 0;
public:
    explicit FixedStep(float step = 0.004f, std::size_t limit = 512) : step_(step), limit_(limit) {
        if (!std::isfinite(step) || step <= 0 || !limit) throw std::invalid_argument("invalid fixed step");
    }
    template<class F> std::size_t advance(float delta, F&& tick) {
        if (!std::isfinite(delta) || delta <= 0) return 0;
        const float budget = step_ * static_cast<float>(limit_);
        if (delta > budget) { dropped_ += static_cast<double>(delta - budget); delta = budget; }
        remainder_ += delta;
        std::size_t n = 0;
        while (remainder_ >= step_ && n < limit_) {
            remainder_ -= step_;
            tick(step_);
            ++n;
        }
        return n;
    }
    float alpha() const noexcept { return remainder_ / step_; }
    float step() const noexcept { return step_; }
    double droppedSeconds() const noexcept { return dropped_; }
    void reset() noexcept { remainder_ = 0; dropped_ = 0; }
};
}
