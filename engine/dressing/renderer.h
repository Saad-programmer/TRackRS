// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
/**
 * Stage dressing (people) — GL 3.3 instanced renderer.
 * Ownership: owns one VAO, a static mesh VBO/IBO (4 kinds x 3 LODs), an
 * orphaned instance stream and 1x1 default textures. Main (GL) thread only.
 * Performance: one glDrawElementsInstancedBaseVertex per non-empty (kind, LOD);
 * per-frame CPU work is O(people) (behaviour + LOD + frustum), no allocation
 * after warm-up. Shadow pass: LOD >= 1 within 120 m; probes skip people.
 */
#include "engine/core/render_quality.h"
#include "engine/vegetation/vegetation.h"
#include "people.h"
#include <filesystem>
#include <memory>
#include <nlohmann/json.hpp>
#include <vector>
namespace trackrs::dressing {
struct Settings {
  bool enabled = true; ///< spectators/crews in the PBR world
};
void configure(Settings settings);
Settings settings();
/// Per-frame inputs: deterministic clock and car positions (player first).
void setFrame(float time, const std::vector<glm::vec3> &cars);
void writeReport(const std::filesystem::path &path);

class Renderer {
  struct Impl;
  std::unique_ptr<Impl> impl_;

public:
  explicit Renderer(const std::filesystem::path &data);
  ~Renderer();
  Renderer(const Renderer &) = delete;
  Renderer &operator=(const Renderer &) = delete;
  void setPeople(std::vector<Person> people);
  std::size_t draw(glm::vec3 camera, const glm::mat4 &viewProjection, vegetation::Pass pass,
                   RenderQuality quality, bool linearCapture);
  nlohmann::json report() const;
};
} // namespace trackrs::dressing
