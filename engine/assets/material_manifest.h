// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "ktx2.h"
namespace trackrs::assets {
struct MaterialInfo {
  std::string name;
  std::size_t textureBytes = 0;
  std::array<unsigned, 3> mipCounts{};
};
MaterialInfo validateMaterial(const std::filesystem::path &manifest);
} // namespace trackrs::assets
