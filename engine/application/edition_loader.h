// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
/** Parses the application-owned edition schema into value access rules.
 * No filesystem/GL ownership; reentrant, linear in document size. Throws on errors.
 */
#include "../core/edition_policy.h"
#include <nlohmann/json.hpp>
namespace trackrs {
inline EditionPolicy loadEditionPolicy(std::string_view text) {
    const auto document = nlohmann::json::parse(text.begin(), text.end());
    if (document.at("schema").get<std::string>() != "trackrs.edition/1")
        throw std::invalid_argument("unsupported edition policy schema");
    EditionPolicy result;
    result.edition = EditionPolicy::parse(document.at("mode").get<std::string>());
    const auto& stages = document.at("demo_stages");
    if (!stages.is_array()) throw std::invalid_argument("edition stages must be an array");
    for (const auto& entry : stages) {
        const auto path = entry.get<std::string>();
        if (path.empty() || path.front() != '/' || path.find("..") != std::string::npos || path.find('\\') != std::string::npos)
            throw std::invalid_argument("invalid edition stage path");
        result.demoStages.insert(path);
    }
    if (document.contains("demo_packs")) {   // optional (M0 pack-lite)
        const auto& packs = document.at("demo_packs");
        if (!packs.is_array()) throw std::invalid_argument("edition packs must be an array");
        for (const auto& entry : packs) result.demoPacks.insert(entry.get<std::string>());
    }
    return result;
}
}
