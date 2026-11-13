// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
/**
 * Phase 4b CarRenderer (GL 3.3 core) for the PBR world.
 *
 * Purpose: draws a car as nodes — body (3 LODs), four wheels from the physics
 * wheel transforms (spin, steer and suspension travel already composed by
 * PSim), a procedural interior with live gauges, and a blended glass pass —
 * through pbr.frag compiled with CAR: role-based PBR (clear-coat paint, glass,
 * rubber, metal), livery-composited atlas, dirt/mud accumulation, emissive
 * head/tail/brake lights and brake-disc glow.
 * Ownership: one process-wide cache owns per-car-type GPU assets (keyed by
 * the caller's type key) and per-vehicle wear state (keyed by vehicle key);
 * released by shutdown(). Main (GL) thread only.
 * Performance: per car per pass at most 1 body + 4 wheel + 1 interior + 1 glass
 * draw; LODs by projected size; the shadow pass uses LOD >= 1 and no glass or
 * interior. Textures are built once per type (livery composited on the CPU).
 */
#include "car_assets.h"
#include <array>
#include "engine/mesh/mesh_importer.h"
#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
namespace trackrs::car {
struct Settings {
  bool enabled = true; ///< PBR world only; Legacy keeps the original path
};
void configure(Settings settings);
Settings settings();

struct Instance {
  const void *vehicle = nullptr;  ///< identity for wear state
  const void *type = nullptr;     ///< identity for GPU assets
  std::string name;               ///< for reports
  std::string bodyFile, wheelFile; ///< virtual (PhysFS) OBJ paths
  std::string livery;             ///< livery JSON text ("" = atlas as authored)
  glm::mat4 body{1};              ///< body model -> world (part scale included)
  std::vector<glm::mat4> wheels;  ///< wheel model -> world (scale included)
  float rpm = 0, redlineRpm = 8000, speed = 0; ///< speed m/s
  int gear = 0;
  float brake = 0;
  bool reverse = false;
  Surface surface = Surface::Tarmac;
  bool interior = true;           ///< draw the cockpit
  /// Phase 4c: simulation damage per corner (PDamage order FL, FR, RL, RR),
  /// clamped 0..1. Read only: drives dents, scratches, cracked glass, broken
  /// lamps and engine smoke.
  std::array<float, 4> damage{};
  bool damageIsVisual = false;    ///< damage holds visual severity (validation override), not simulation values
};
/// `read` loads virtual files (OBJ, MTL, PNG). Returns submitted draws.
std::size_t draw(const Instance &car, const mesh::ReadFile &read, const glm::mat4 &viewProjection,
                 glm::vec3 camera, float fovY, float time, bool linearCapture);
/// Advances one vehicle's wear (dust, mud, brake temperature) by one
/// simulation step. Called every tick by the game, so wear is independent of
/// how often (or whether) the car is drawn; draw() then only reads it.
void observe(const void *vehicle, const WearInput &input);
/// Validation only (--brake-heat): overrides the disc temperature (°C, all
/// four discs) so the glow can be inspected regardless of the drive. Reports
/// mark such runs as forced.
void forceBrakeTemperature(const void *vehicle, float celsius);
/// Phase 3e: motion vectors (into the bound velocity target) for every car
/// drawn in this frame's main view, then rolls current -> previous transforms.
/// Matrices: jittered current view-projection (rasterisation), unjittered
/// current and previous view-projections. Returns submitted draws.
std::size_t drawVelocity(const glm::mat4 &jittered, const glm::mat4 &current, const glm::mat4 &previous);
void shutdown();
nlohmann::json report();
void writeReport(const std::filesystem::path &path);
} // namespace trackrs::car
