// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
/** PCG XSH-RR 64/32 stream. Owns its state; independent instances are thread-safe.
 * No clock/global libc RNG. Constant work per sample; 24-bit exact float conversion.
 */
#include <cstdint>
#include <string_view>
namespace trackrs {
class Pcg32 {
    std::uint64_t state_ = 0, increment_ = 1;
public:
    explicit Pcg32(std::uint64_t seed = 1000, std::uint64_t stream = 54) { reseed(seed, stream); }
    // Canonical PCG seed/stream API; both are intentionally full-width values.
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    void reseed(std::uint64_t seed, std::uint64_t stream = 54) noexcept {
        state_ = 0; increment_ = (stream << 1u) | 1u;
        next(); state_ += seed; next();
    }
    std::uint32_t next() noexcept {
        const auto old = state_;
        state_ = old * 6364136223846793005ULL + increment_;
        const auto x = static_cast<std::uint32_t>(((old >> 18u) ^ old) >> 27u);
        const auto rotation = static_cast<std::uint32_t>(old >> 59u);
        return (x >> rotation) | (x << ((0u - rotation) & 31u));
    }
    float unit() noexcept { return static_cast<float>(next() >> 8u) * (1.0f / 16777216.0f); }
    float symmetric() noexcept { return 2.0f * unit() - 1.0f; }
};
inline std::uint64_t stageSeed(std::string_view id) noexcept {
    std::uint64_t hash = 14695981039346656037ULL;
    for (char c : id) { hash ^= static_cast<unsigned char>(c); hash *= 1099511628211ULL; }
    return hash;
}
}
