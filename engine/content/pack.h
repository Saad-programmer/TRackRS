// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
/**
 * M0 "pack-lite" content packs (master prompt §4.4, amendment §A9.2).
 *
 * Purpose: parse and validate content packs, resolve their dependencies and
 * the edition policy, and produce the catalogue of playable pack stages and
 * championships. A pack is a directory `/packs/<id>/` (or a `<id>.trpack`
 * zip mounted there) holding:
 *   pack.json                          trackrs.pack/1
 *   stages/<stage>/stage.json          trackrs.stage/1 (+ the stage's files)
 *   championships/<id>.championship.json  trackrs.championship/1
 * Legacy content is untouched: today's /maps and /events keep loading through
 * the legacy loaders and are listed as the virtual pack `classic-legacy-stages`.
 *
 * Licence audit (amendment A10): every file inside a pack must be covered by
 * an `assets` entry carrying an SPDX licence and an attribution; uncovered or
 * unlicensed files make the pack invalid. `credits()` renders CREDITS.md.
 *
 * Failure policy: nothing here throws for bad content once inside
 * `discover()`. A broken pack is returned disabled with readable issues, so
 * the game can show "pack X disabled: reason" and carry on.
 * Ownership: value types. Thread-safety: pure functions, no global state.
 */
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
namespace trackrs::content {

// ---- versions -------------------------------------------------------------------
struct SemVer {
  int major = 0, minor = 0, patch = 0;
  static std::optional<SemVer> parse(std::string_view text); ///< "1", "1.2", "1.2.3"
  friend bool operator<(const SemVer &a, const SemVer &b);
  friend bool operator==(const SemVer &a, const SemVer &b);
  std::string str() const;
};
/// Space-separated comparator set, all must hold: ">=0.7 <1.0", "^1.2",
/// "~1.2.3", "=1.0.0", "1.0.0" (exact), "*" or "" (any).
struct VersionRange {
  std::string text;
  static std::optional<VersionRange> parse(std::string_view text);
  bool satisfies(const SemVer &v) const;
};
/// Engine version the packs are checked against (`requires_engine`).
SemVer engineVersion();
/// Content API level; additive changes only within a level.
constexpr int ContentApi = 1;

// ---- data ----------------------------------------------------------------------------
struct License {
  std::string spdx;          ///< SPDX identifier, e.g. "CC0-1.0", "CC-BY-4.0", "GPL-2.0-or-later"
  std::string attribution;   ///< human-readable credit line
  std::string author, sourceUrl;
  bool verified = false;     ///< true only when a maintainer checked the source
};
struct Asset {
  std::string path;          ///< glob relative to the pack root ("stages/x/*.png", "**")
  std::string kind;          ///< dem, texture, model, font, sound, track, data, ...
  License license;
};
struct Dependency {
  std::string id;
  VersionRange range;
};
struct PackManifest {
  std::string id, name, description;
  SemVer version;
  VersionRange requiresEngine;
  int contentApi = 1;
  std::vector<Dependency> depends;
  std::vector<std::string> authors;
  License license;           ///< pack-level licence (data files default to this)
  std::vector<Asset> assets;
  std::vector<std::string> notices; ///< e.g. ODbL / trademark notices to show in credits
};
struct StageDefinition {
  std::string id;            ///< local id; global id is "<pack>:<id>"
  std::string name, region, description;
  std::string level;         ///< level file, relative to the stage dir ("x.level")
  std::uint64_t seed = 0;    ///< stage_seed for dressing and props
  std::string geographicFidelity; ///< "procedural" | "derived" | "surveyed"
  double lengthKm = 0;
  std::vector<std::string> surfaces, timesOfDay;
  std::string terrainSource; ///< e.g. "procedural", "SRTM 1 arc-second"
  bool testOnly = false;     ///< hidden from players (test stages)
};
struct Round {
  std::string stage;         ///< "<pack>:<stage>" or local "<stage>"
  int day = 1;
  std::string kind = "special"; ///< special | liaison | service | marathon
  std::string timeOfDay, weather;
  bool serviceAfter = false;
};
struct Championship {
  std::string id, name;
  std::vector<Round> rounds;
  std::vector<int> points;   ///< points by finishing position
  std::map<std::string, double> penalties; ///< seconds, e.g. "missed_waypoint"
};

// ---- parsing (throw std::runtime_error with context) ----------------------------
PackManifest parsePack(std::string_view json);
StageDefinition parseStage(std::string_view json);
Championship parseChampionship(std::string_view json);

// ---- discovery ------------------------------------------------------------------------
/// Virtual file access ("/packs/<id>/..."): list a directory's entries (names
/// only; directories end with '/') and read a file. Absent -> empty / nullopt.
struct Source {
  std::function<std::vector<std::string>(const std::string &dir)> list;
  std::function<std::optional<std::string>(const std::string &path)> read;
};
struct Issue {
  enum class Severity : std::uint8_t { Error, Warning };
  Severity severity = Severity::Error;
  std::string message;
};
struct LoadedPack {
  std::string id;            ///< directory name (manifest id must match)
  std::string root;          ///< virtual root, "/packs/<id>"
  PackManifest manifest;
  std::vector<StageDefinition> stages;
  std::vector<Championship> championships;
  std::vector<std::string> files; ///< every file under the root, relative
  std::vector<Issue> issues;
  bool enabled = false;
  std::string disabledReason; ///< first error, for the UI
  bool valid() const;
};
enum class Edition : std::uint8_t { Demo, Full, Dev };
struct DiscoverOptions {
  Edition edition = Edition::Full;
  std::vector<std::string> demoPacks = {"classic-legacy-stages"}; ///< allowed in Demo
};
struct Catalog {
  std::vector<LoadedPack> packs;
  const LoadedPack *find(std::string_view id) const;
  /// Enabled stage by global id "<pack>:<stage>"; nullptr when unknown/disabled.
  const StageDefinition *stage(std::string_view globalId, const LoadedPack **owner = nullptr) const;
  /// Virtual path of the stage's level file.
  std::optional<std::string> levelPath(std::string_view globalId) const;
  std::size_t enabledCount() const;
};
/// Validates one pack in place (licence audit, references, ids); sets
/// `enabled` when there are no errors. Does not resolve dependencies.
void validate(LoadedPack &pack, const Source &source);
/// Scans "/packs/*/pack.json", validates every pack, resolves dependencies
/// (missing / out-of-range / disabled dependency disables the dependent) and
/// applies the edition. The virtual legacy pack is always present.
Catalog discover(const Source &source, const DiscoverOptions &options = {});
/// Markdown credits for every enabled pack: assets grouped by licence, with
/// attributions and notices (CREDITS.md).
std::string credits(const Catalog &catalog);
/// Machine-readable catalogue summary (validation reports, the menu).
std::string summaryJson(const Catalog &catalog);
/// Glob match: '*' within a path segment, "**" across segments.
bool globMatch(std::string_view pattern, std::string_view path);
} // namespace trackrs::content
