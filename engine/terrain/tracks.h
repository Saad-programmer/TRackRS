// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
/**
 * Persistent tracks in soft ground (amendment A3.3): tyre ruts with displaced
 * lips, plus static camel and foot-print trails authored per stage.
 *
 * Visual only: the simulation never reads this state, so legacy hashes are
 * untouched. Wheels are observed once per simulation tick (never per frame),
 * so the same replay yields the same tracks and the same tracksHash().
 * Storage is a fixed ring of segments (oldest overwritten), allocated once
 * per stage; observe() never allocates after beginStage().
 */
#include <array>
#include <filesystem>
#include <cstdint>
#include <functional>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <vector>
namespace trackrs::terrain {
enum class TrackKind : std::uint8_t { Tyre = 0, Camel = 1, Foot = 2 };
/// How a surface takes a mark. depth == 0 means no mark (rock, tarmac).
struct RutProfile {
  float depth = 0;   ///< rut floor below the surface, metres
  float lip = 0;     ///< height of the displaced sand ridge, metres
  float darken = 1;  ///< albedo factor of disturbed ground (<1 darker)
};
/// One ribbon vertex pair end: centre on the ground, unit side vector.
struct TrackPoint {
  glm::vec3 position{0};
  glm::vec2 side{1, 0};
  float along = 0; ///< metres from the trail start (print/tread phase)
};
struct TrackSegment {
  TrackPoint a, b;
  float halfWidth = .12f;
  RutProfile profile;
  TrackKind kind = TrackKind::Tyre;
};
class Tracks {
public:
  static constexpr std::size_t DefaultCapacity = 32768;
  static constexpr float Spacing = .5f; ///< metres between tyre samples

  /// Clears every trail and (re)allocates the ring.
  void beginStage(std::size_t capacity = DefaultCapacity);
  /// One wheel contact this tick. `wheel` identifies the trail (car, wheel);
  /// profile.depth == 0 or !contact ends the trail (a gap, not a smear).
  void observe(std::uint32_t wheel, glm::vec3 contact, glm::vec2 forward, float halfWidth, RutProfile profile,
               bool contact_);
  /// A static decal trail (camel or foot prints) through `points` (world metres).
  void addStatic(TrackKind kind, const std::vector<glm::vec3> &points, float halfWidth, RutProfile profile);
  /// Parses the "tracks" array of a trackrs.props/1 document (world metres);
  /// heights come from `height`. Returns the number of trails added.
  std::size_t addStatic(const nlohmann::json &props, const std::function<float(float, float)> &height);

  /// Tyre segments (a ring; `head()` is the oldest once it has wrapped).
  const std::vector<TrackSegment> &segments() const noexcept { return ring_; }
  /// Static print trails; never overwritten by tyre marks.
  const std::vector<TrackSegment> &staticSegments() const noexcept { return static_; }
  std::size_t size() const noexcept { return std::min(written_, ring_.size()); }
  std::size_t head() const noexcept { return ring_.empty() ? 0 : written_ % ring_.size(); }
  std::uint64_t written() const noexcept { return written_; }
  std::uint64_t revision() const noexcept { return written_ + static_.size(); }
  /// FNV-1a over every live segment (determinism evidence).
  std::uint64_t hash() const;
  nlohmann::json report() const;

private:
  struct Trail {
    std::uint32_t wheel = 0;
    bool open = false;
    TrackPoint last;
    RutProfile profile;
    float halfWidth = 0;
  };
  void push(const TrackSegment &segment);
  std::vector<TrackSegment> ring_;
  std::uint64_t written_ = 0;
  std::size_t staticTrails_ = 0;
  std::vector<TrackSegment> static_;
  std::vector<Trail> trails_;
};
/// The process-wide tracks of the running stage.
Tracks &tracks();
/// Writes tracks().report() (validation runs).
void writeTracksReport(const std::filesystem::path &path);
} // namespace trackrs::terrain
