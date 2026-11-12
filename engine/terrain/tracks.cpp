// SPDX-License-Identifier: GPL-2.0-or-later
#include "tracks.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <stdexcept>
namespace trackrs::terrain {
namespace {
glm::vec2 sideOf(glm::vec2 forward) {
  const float l = glm::length(forward);
  if (!(l > 1e-6f))
    return {1, 0};
  forward /= l;
  return {forward.y, -forward.x}; // right-hand side, z up
}
void fnv(std::uint64_t &h, float v) {
  std::uint32_t bits = 0;
  std::memcpy(&bits, &v, sizeof bits);
  for (int i = 0; i < 4; ++i) {
    h ^= (bits >> (8 * i)) & 0xffu;
    h *= 1099511628211ull;
  }
}
} // namespace

void Tracks::beginStage(std::size_t capacity) {
  if (capacity == 0)
    throw std::invalid_argument("tracks capacity must be positive");
  ring_.assign(capacity, {});
  written_ = 0;
  staticTrails_ = 0;
  static_.clear();
  trails_.clear();
  trails_.reserve(32);
}

void Tracks::push(const TrackSegment &segment) {
  if (ring_.empty())
    return;
  ring_[written_ % ring_.size()] = segment;
  ++written_;
}

void Tracks::observe(std::uint32_t wheel, glm::vec3 contact, glm::vec2 forward, float halfWidth, RutProfile profile,
                     bool contact_) {
  if (ring_.empty())
    return;
  auto it = std::find_if(trails_.begin(), trails_.end(), [&](const Trail &t) { return t.wheel == wheel; });
  if (it == trails_.end()) {
    trails_.push_back({});
    it = trails_.end() - 1;
    it->wheel = wheel;
  }
  auto &trail = *it;
  if (!contact_ || profile.depth <= 0 || !std::isfinite(contact.x + contact.y + contact.z)) {
    trail.open = false;
    return;
  }
  const TrackPoint here{contact, sideOf(forward), 0};
  if (!trail.open) {
    trail = {wheel, true, here, profile, halfWidth};
    return;
  }
  const float step = glm::length(glm::vec2(contact - trail.last.position));
  if (step < Spacing)
    return;
  if (step > 8 * Spacing) { // teleport, reset or airborne jump: restart the trail
    trail = {wheel, true, here, profile, halfWidth};
    return;
  }
  TrackPoint next = here;
  next.along = trail.last.along + step;
  // Surface changes mid-segment take the shallower profile (no hard steps).
  RutProfile p{std::min(profile.depth, trail.profile.depth), std::min(profile.lip, trail.profile.lip),
               std::max(profile.darken, trail.profile.darken)};
  push({trail.last, next, .5f * (halfWidth + trail.halfWidth), p, TrackKind::Tyre});
  trail.last = next;
  trail.profile = profile;
  trail.halfWidth = halfWidth;
}

void Tracks::addStatic(TrackKind kind, const std::vector<glm::vec3> &points, float halfWidth, RutProfile profile) {
  if (points.size() < 2)
    return;
  ++staticTrails_;
  float along = 0;
  for (std::size_t i = 0; i + 1 < points.size(); ++i) {
    const glm::vec2 d = glm::vec2(points[i + 1] - points[i]);
    const float length = glm::length(d);
    if (!(length > 1e-3f))
      continue;
    // Split long legs so the ribbon follows the ground.
    const int pieces = std::max(1, static_cast<int>(std::ceil(length / 2.f)));
    const glm::vec2 side = sideOf(d);
    for (int k = 0; k < pieces; ++k) {
      const float t0 = static_cast<float>(k) / static_cast<float>(pieces),
                  t1 = static_cast<float>(k + 1) / static_cast<float>(pieces);
      TrackPoint a{glm::mix(points[i], points[i + 1], t0), side, along + length * t0};
      TrackPoint b{glm::mix(points[i], points[i + 1], t1), side, along + length * t1};
      static_.push_back({a, b, halfWidth, profile, kind});
    }
    along += length;
  }
}

std::size_t Tracks::addStatic(const nlohmann::json &props, const std::function<float(float, float)> &height) {
  if (!props.is_object() || !props.contains("tracks"))
    return 0;
  std::size_t added = 0;
  for (const auto &t : props.at("tracks")) {
    const auto kindName = t.value("kind", std::string("camel"));
    TrackKind kind;
    RutProfile profile;
    float halfWidth;
    if (kindName == "camel") {
      kind = TrackKind::Camel;
      profile = {.06f, .015f, .62f};
      halfWidth = .45f;
    } else if (kindName == "foot") {
      kind = TrackKind::Foot;
      profile = {.045f, .01f, .66f};
      halfWidth = .3f;
    } else {
      throw std::runtime_error("unknown static track kind: " + kindName);
    }
    std::vector<glm::vec3> points;
    for (const auto &p : t.at("points")) {
      const float x = p.at(0).get<float>(), y = p.at(1).get<float>();
      points.emplace_back(x, y, height(x, y));
    }
    addStatic(kind, points, halfWidth, profile);
    ++added;
  }
  return added;
}

std::uint64_t Tracks::hash() const {
  std::uint64_t h = 1469598103934665603ull;
  const std::size_t n = size(), start = written_ > ring_.size() ? head() : 0;
  for (std::size_t i = 0; i < n + static_.size(); ++i) {
    const auto &s = i < n ? ring_[(start + i) % ring_.size()] : static_[i - n];
    for (float v : {s.a.position.x, s.a.position.y, s.a.position.z, s.b.position.x, s.b.position.y, s.b.position.z,
                    s.halfWidth, s.profile.depth, s.profile.lip, s.b.along})
      fnv(h, v);
    h ^= static_cast<std::uint64_t>(s.kind);
    h *= 1099511628211ull;
  }
  return h;
}

nlohmann::json Tracks::report() const {
  char hex[17];
  std::snprintf(hex, sizeof hex, "%016llx", static_cast<unsigned long long>(hash()));
  return {{"capacity_segments", ring_.size()},
          {"live_segments", size()},
          {"written_segments", written_},
          {"static_trails", staticTrails_},
          {"static_segments", static_.size()},
          {"ring_bytes", (ring_.size() + static_.size()) * sizeof(TrackSegment)},
          {"tracks_hash", hex}};
}

Tracks &tracks() {
  static Tracks instance;
  return instance;
}
void writeTracksReport(const std::filesystem::path &path) {
  std::ofstream out(path);
  if (!out)
    throw std::runtime_error("cannot write " + path.string());
  out << tracks().report().dump(2) << '\n';
}
} // namespace trackrs::terrain
