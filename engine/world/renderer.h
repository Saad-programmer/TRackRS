// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "engine/core/render_quality.h"
#include "state.h"
#include <filesystem>
#include <functional>
#include <nlohmann/json.hpp>
namespace trackrs::world {
void prepare(const glm::mat4 &view, const glm::mat4 &projection,
             RenderQuality quality, const std::filesystem::path &data);
void drawSky(const glm::mat4 &projection, const glm::mat4 &view,
             glm::vec3 camera, bool linear = false);
void drawWater(const glm::mat4 &viewProjection, glm::vec3 camera, float waterHeight,
               const std::function<float(float, float)> &terrainHeight, float time);
void renderShadows(const std::function<void(const glm::mat4 &)> &casters);
void shutdown();
nlohmann::json report();
void writeReport(const std::filesystem::path &path);
} // namespace trackrs::world
