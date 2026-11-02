// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
/** Strict SemVer value for engine/content compatibility. Owns suffix strings;
 * parsing is reentrant, thread-safe and linear in the input length.
 */
#include <array>
#include <charconv>
#include <stdexcept>
#include <string>
#include <string_view>
namespace trackrs {
struct Version {
    unsigned major = 0, minor = 0, patch = 0;
    std::string prerelease, build;
    static bool identifiers(std::string_view value, bool numericLeadingZeros) {
        if (value.empty()) return false;
        for (std::size_t start = 0; start < value.size();) {
            auto end = value.find('.', start);
            if (end == std::string_view::npos) end = value.size();
            const auto id = value.substr(start, end - start);
            if (id.empty()) return false;
            bool numeric = true;
            for (char c : id) {
                const bool digit = c >= '0' && c <= '9';
                if (!digit && !(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') && c != '-') return false;
                numeric = numeric && digit;
            }
            if (!numericLeadingZeros && numeric && id.size() > 1 && id[0] == '0') return false;
            if (end == value.size()) return true;
            start = end + 1;
        }
        return false;
    }
    static Version parse(std::string_view text) {
        const auto suffix = text.find_first_of("-+");
        const auto core = text.substr(0, suffix);
        std::array<unsigned, 3> numbers{};
        std::size_t start = 0;
        for (unsigned i = 0; i < 3; ++i) {
            const auto end = i == 2 ? core.size() : core.find('.', start);
            if (end == std::string_view::npos || end <= start)
                throw std::invalid_argument("invalid semantic version");
            auto value = core.substr(start, end - start);
            if (value.size() > 1 && value[0] == '0') throw std::invalid_argument("leading version zero");
            const auto parsed = std::from_chars(value.data(), value.data() + value.size(), numbers[i]);
            if (parsed.ec != std::errc() || parsed.ptr != value.data() + value.size())
                throw std::invalid_argument("invalid numeric version");
            start = end + 1;
        }
        Version result{numbers[0], numbers[1], numbers[2], {}, {}};
        if (suffix != std::string_view::npos) {
            const auto plus = text.find('+', suffix);
            if (text[suffix] == '-') {
                result.prerelease = text.substr(suffix + 1, plus == std::string_view::npos ? plus : plus - suffix - 1);
                if (!identifiers(result.prerelease, false)) throw std::invalid_argument("invalid prerelease");
            }
            if (plus != std::string_view::npos) {
                result.build = text.substr(plus + 1);
                if (!identifiers(result.build, true)) throw std::invalid_argument("invalid build metadata");
            }
        }
        return result;
    }
};
}
