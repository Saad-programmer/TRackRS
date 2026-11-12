// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
/** Application-owned content access rules. Paths are virtual pack paths, never titles.
 * Immutable between pack reloads; queried on the application thread only.
 */
#include <set>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
namespace trackrs {
class EditionPolicy {
public:
    enum class Edition : std::uint8_t { Demo, Full, Dev };
    Edition edition = Edition::Demo;
    std::set<std::string, std::less<>> demoStages;
    std::set<std::string, std::less<>> demoPacks; ///< content packs allowed in the demo (M0)
    static Edition parse(std::string_view mode) {
        if (mode == "demo") return Edition::Demo;
        if (mode == "full") return Edition::Full;
        if (mode == "dev") return Edition::Dev;
        throw std::invalid_argument("unknown edition");
    }
    bool stage(std::string_view path) const { return edition != Edition::Demo || demoStages.count(path) != 0; }
    bool events() const noexcept { return edition != Edition::Demo; }
    bool championship(bool earned) const noexcept { return edition == Edition::Dev || earned; }
    bool event(bool locked, bool unlocked) const noexcept { return events() && championship(!locked || unlocked); }
};
}
