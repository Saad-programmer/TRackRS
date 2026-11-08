// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
/**
 * Native 3D vegetation — GL 3.3 core renderer.
 *
 * Purpose: draws the plants of the legacy foliage bands as instanced
 * procedural meshes (LOD0..2) and instanced sprite-card impostors (LOD3),
 * lit by the shared PBR/world pipeline (sun, sky IBL, CSM, height fog).
 * Ownership: Renderer owns its VAO, mesh VBO/IBO, instance stream and 1x1
 * default textures; sprite textures are borrowed (legacy PTexture ids) and
 * must outlive the renderer, which PTerrain guarantees by owning both.
 * Thread-safety: main (GL) thread only.
 * Performance: one glDrawElementsInstancedBaseVertex per (band, LOD) with a
 * non-empty bucket; all instances of a pass are uploaded with one orphaned
 * glBufferData + glBufferSubData. Mesh buffers are rebuilt only when a band is
 * registered (stage load). GL 3.3 has no base-instance, so the two instance
 * attribute pointers are re-pointed per batch (2 cheap VAO state calls).
 */
#include "engine/core/render_quality.h"
#include "vegetation.h"
#include <filesystem>
#include <glad/gl.h>
#include <memory>
#include <nlohmann/json.hpp>
namespace trackrs::vegetation {
struct Settings {
  bool meshes = true; ///< false keeps the legacy card renderer
};
void configure(Settings settings);
Settings settings();
/// Deterministic animation clock (seconds) driving wind; set by the game.
void setAnimationTime(float seconds);
void writeReport(const std::filesystem::path &path);

class Renderer {
  struct Impl;
  std::unique_ptr<Impl> impl_;

public:
  explicit Renderer(const std::filesystem::path &data);
  ~Renderer();
  Renderer(const Renderer &) = delete;
  Renderer &operator=(const Renderer &) = delete;
  /// Registers one foliage band. `texture` is the band's sprite (impostor).
  std::size_t addBand(std::string_view sprite, GLuint texture, unsigned crossings);
  std::size_t bands() const;
  Form form(std::size_t band) const;
  /// Starts a pass; returns the classifier to feed with plants (see add()).
  Classifier &begin(glm::vec3 camera, const glm::mat4 &viewProjection, Pass pass,
                    RenderQuality quality);
  /// Draws everything collected since begin(). Returns submitted draw calls.
  std::size_t submit(const glm::mat4 &viewProjection, bool linearCapture);
  nlohmann::json report() const;
};
} // namespace trackrs::vegetation
