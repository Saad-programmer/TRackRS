// SPDX-License-Identifier: GPL-2.0-or-later
#include "material_manifest.h"
#include <nlohmann/json.hpp>
#include <stdexcept>
namespace trackrs::assets {
MaterialInfo validateMaterial(const std::filesystem::path &path) {
  auto bytes = readFile(path);
  if (bytes.size() > 65536)
    throw std::runtime_error("material manifest exceeds 64KiB");
  auto json = nlohmann::json::parse(bytes);
  MaterialInfo info;
  info.name = json.value("name", path.stem().string());
  bool atlas = json.value("atlas", false);
  unsigned index = 0;
  for (auto role :
       {std::pair<const char *, Semantic>{"albedo", Semantic::Albedo},
        {"normal", Semantic::Normal},
        {"orm", Semantic::Orm}}) {
    auto relative =
        std::filesystem::path(json.at(role.first).get<std::string>())
            .lexically_normal();
    if (relative.empty() || relative.is_absolute() ||
        relative.has_root_name() || *relative.begin() == "..")
      throw std::runtime_error(
          "material texture must stay inside manifest directory");
    auto texture =
        readKtx2(readFile(path.parent_path() / relative), role.second, atlas);
    info.textureBytes += texture.bytes();
    info.mipCounts[index++] = static_cast<unsigned>(texture.levels.size());
  }
  return info;
}
} // namespace trackrs::assets
