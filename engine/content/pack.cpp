// SPDX-License-Identifier: GPL-2.0-or-later
#include "pack.h"
#include <algorithm>
#include <cctype>
#include <nlohmann/json.hpp>
#include <set>
#include <sstream>
#include <stdexcept>
#include <tuple>
#ifndef TRACKRS_ENGINE_VERSION
#error "TRACKRS_ENGINE_VERSION must be defined by the build (project version)"
#endif
namespace trackrs::content {
namespace {
using nlohmann::json;
bool validId(std::string_view id) {
  if (id.empty() || id.size() > 64 || !std::islower(static_cast<unsigned char>(id.front())))
    return false;
  return std::all_of(id.begin(), id.end(), [](char c) {
    return std::islower(static_cast<unsigned char>(c)) || std::isdigit(static_cast<unsigned char>(c)) || c == '-' ||
           c == '_';
  });
}
/// Relative path inside a pack: no absolute paths, no "..", no backslashes.
bool safeRelative(std::string_view p) {
  if (p.empty() || p.front() == '/' || p.find('\\') != std::string_view::npos || p.find(':') != std::string_view::npos)
    return false;
  std::size_t start = 0;
  while (start <= p.size()) {
    const auto end = std::min(p.find('/', start), p.size());
    if (p.substr(start, end - start) == "..")
      return false;
    start = end + 1;
  }
  return true;
}
std::string text(const json &j, const char *key, bool required = false) {
  if (!j.contains(key)) {
    if (required)
      throw std::runtime_error(std::string("missing \"") + key + "\"");
    return {};
  }
  if (!j.at(key).is_string())
    throw std::runtime_error(std::string("\"") + key + "\" must be a string");
  return j.at(key).get<std::string>();
}
std::vector<std::string> strings(const json &j, const char *key) {
  std::vector<std::string> out;
  if (!j.contains(key))
    return out;
  if (!j.at(key).is_array())
    throw std::runtime_error(std::string("\"") + key + "\" must be an array");
  for (const auto &v : j.at(key)) {
    if (!v.is_string())
      throw std::runtime_error(std::string("\"") + key + "\" entries must be strings");
    out.push_back(v.get<std::string>());
  }
  return out;
}
License parseLicense(const json &j, const std::string &where) {
  if (!j.is_object())
    throw std::runtime_error(where + ": licence must be an object");
  License l;
  l.spdx = text(j, "spdx");
  l.attribution = text(j, "attribution");
  l.author = text(j, "author");
  l.sourceUrl = text(j, "source_url");
  l.verified = j.value("verified", false);
  return l;
}
json parseJson(std::string_view text, const char *schema) {
  json j = json::parse(text.begin(), text.end()); // throws json::parse_error with position
  if (!j.is_object())
    throw std::runtime_error("top level must be an object");
  const auto s = j.value("schema", std::string());
  if (s != schema)
    throw std::runtime_error(std::string("schema must be \"") + schema + "\" (found \"" + s + "\")");
  return j;
}
int number(std::string_view s, std::size_t &i) {
  if (i >= s.size() || !std::isdigit(static_cast<unsigned char>(s[i])))
    return -1;
  long v = 0;
  while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) {
    v = v * 10 + (s[i++] - '0');
    if (v > 1000000)
      return -1;
  }
  return static_cast<int>(v);
}
/// One comparator; ok=false on syntax error.
bool comparator(std::string_view term, const SemVer &v, bool &ok) {
  std::string_view op;
  for (std::string_view candidate : {">=", "<=", ">", "<", "=", "^", "~"})
    if (term.substr(0, candidate.size()) == candidate) {
      op = candidate;
      break;
    }
  const auto rest = term.substr(op.size());
  const auto base = SemVer::parse(rest);
  if (!base) {
    ok = false;
    return false;
  }
  const auto &b = *base;
  const int parts = static_cast<int>(std::count(rest.begin(), rest.end(), '.')) + 1;
  if (op == ">=")
    return !(v < b);
  if (op == "<=")
    return v < b || v == b;
  if (op == ">")
    return b < v;
  if (op == "<")
    return v < b;
  if (op == "^") { // same left-most non-zero component
    if (v < b)
      return false;
    if (b.major > 0)
      return v.major == b.major;
    if (b.minor > 0 || parts >= 2)
      return v.major == 0 && v.minor == b.minor;
    return v.major == 0;
  }
  if (op == "~") // same major.minor (or major if only that was given)
    return !(v < b) && v.major == b.major && (parts < 2 || v.minor == b.minor);
  // "=" or bare: exact on the given components
  return v.major == b.major && (parts < 2 || v.minor == b.minor) && (parts < 3 || v.patch == b.patch);
}
void error(LoadedPack &p, const std::string &message) { p.issues.push_back({Issue::Severity::Error, message}); }
void warning(LoadedPack &p, const std::string &message) { p.issues.push_back({Issue::Severity::Warning, message}); }
std::string join(const std::string &a, const std::string &b) {
  if (a.empty())
    return b;
  std::string out;
  out.reserve(a.size() + 1 + b.size());
  out.append(a).append(1, '/').append(b);
  return out;
}
void listRecursive(const Source &source, const std::string &root, const std::string &relative,
                   std::vector<std::string> &out, int depth) {
  if (depth > 8)
    return;
  for (const auto &entry : source.list(relative.empty() ? root : join(root, relative))) {
    if (entry.empty())
      continue;
    if (entry.back() == '/')
      listRecursive(source, root, join(relative, entry.substr(0, entry.size() - 1)), out, depth + 1);
    else
      out.push_back(join(relative, entry));
  }
}
std::string globalId(const std::string &pack, const std::string &stage) {
  return stage.find(':') == std::string::npos ? pack + ":" + stage : stage;
}
} // namespace

// ---- versions ----------------------------------------------------------------------
std::optional<SemVer> SemVer::parse(std::string_view s) {
  SemVer v;
  std::size_t i = 0;
  int *parts[3] = {&v.major, &v.minor, &v.patch};
  for (int k = 0; k < 3; ++k) {
    const int n = number(s, i);
    if (n < 0)
      return std::nullopt;
    *parts[k] = n;
    if (i == s.size())
      return v;
    if (s[i] != '.' || k == 2)
      return std::nullopt;
    ++i;
  }
  return std::nullopt;
}
bool operator<(const SemVer &a, const SemVer &b) {
  return std::tie(a.major, a.minor, a.patch) < std::tie(b.major, b.minor, b.patch);
}
bool operator==(const SemVer &a, const SemVer &b) {
  return std::tie(a.major, a.minor, a.patch) == std::tie(b.major, b.minor, b.patch);
}
std::string SemVer::str() const {
  return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
}
namespace {
std::vector<std::string_view> terms(std::string_view t) {
  std::vector<std::string_view> out;
  std::size_t i = 0;
  while (i < t.size()) {
    while (i < t.size() && t[i] == ' ')
      ++i;
    const auto start = i;
    while (i < t.size() && t[i] != ' ')
      ++i;
    if (i > start)
      out.push_back(t.substr(start, i - start));
  }
  return out;
}
} // namespace
std::optional<VersionRange> VersionRange::parse(std::string_view t) {
  VersionRange r;
  r.text = std::string(t);
  for (const auto term : terms(t)) {
    if (term == "*")
      continue;
    bool ok = true;
    comparator(term, SemVer{}, ok);
    if (!ok)
      return std::nullopt;
  }
  return r;
}
bool VersionRange::satisfies(const SemVer &v) const {
  for (const auto term : terms(text)) {
    if (term == "*")
      continue;
    bool ok = true;
    if (!comparator(term, v, ok) || !ok)
      return false;
  }
  return true;
}
SemVer engineVersion() {
  static const SemVer v = SemVer::parse(TRACKRS_ENGINE_VERSION).value_or(SemVer{});
  return v;
}

// ---- parsing ---------------------------------------------------------------------------
PackManifest parsePack(std::string_view source) {
  const json j = parseJson(source, "trackrs.pack/1");
  PackManifest m;
  m.id = text(j, "id", true);
  if (!validId(m.id))
    throw std::runtime_error("invalid pack id \"" + m.id + "\" (lower-case letters, digits, '-', '_')");
  m.name = text(j, "name", true);
  m.description = text(j, "description");
  const auto version = SemVer::parse(text(j, "version", true));
  if (!version)
    throw std::runtime_error("version must be MAJOR.MINOR.PATCH");
  m.version = *version;
  const auto engine = VersionRange::parse(text(j, "requires_engine", true));
  if (!engine)
    throw std::runtime_error("invalid requires_engine range");
  m.requiresEngine = *engine;
  m.contentApi = j.value("content_api", 1);
  m.authors = strings(j, "authors");
  if (!j.contains("license"))
    throw std::runtime_error("missing \"license\" block");
  m.license = parseLicense(j.at("license"), "pack");
  m.notices = strings(j, "notices");
  if (j.contains("depends")) {
    for (const auto &d : j.at("depends")) {
      Dependency dep;
      dep.id = text(d, "id", true);
      const auto range = VersionRange::parse(d.value("version", std::string("*")));
      if (!range)
        throw std::runtime_error("invalid version range for dependency " + dep.id);
      dep.range = *range;
      m.depends.push_back(std::move(dep));
    }
  }
  if (j.contains("assets")) {
    for (const auto &a : j.at("assets")) {
      Asset asset;
      asset.path = text(a, "path", true);
      asset.kind = text(a, "kind");
      if (!a.contains("license"))
        throw std::runtime_error("asset \"" + asset.path + "\" has no licence");
      asset.license = parseLicense(a.at("license"), "asset " + asset.path);
      m.assets.push_back(std::move(asset));
    }
  }
  return m;
}
StageDefinition parseStage(std::string_view source) {
  const json j = parseJson(source, "trackrs.stage/1");
  StageDefinition s;
  s.id = text(j, "id", true);
  if (!validId(s.id))
    throw std::runtime_error("invalid stage id \"" + s.id + "\"");
  s.name = text(j, "name", true);
  s.region = text(j, "region");
  s.description = text(j, "description");
  s.level = text(j, "level", true);
  if (!safeRelative(s.level))
    throw std::runtime_error("stage level must be a relative path inside the stage");
  s.seed = j.value("stage_seed", std::uint64_t{0});
  s.geographicFidelity = text(j, "geographic_fidelity", true);
  if (s.geographicFidelity != "procedural" && s.geographicFidelity != "derived" && s.geographicFidelity != "surveyed")
    throw std::runtime_error("geographic_fidelity must be procedural, derived or surveyed");
  s.lengthKm = j.value("length_km", 0.0);
  if (s.lengthKm < 0 || s.lengthKm > 1000)
    throw std::runtime_error("length_km out of range");
  s.surfaces = strings(j, "surfaces");
  s.timesOfDay = strings(j, "times_of_day");
  s.terrainSource = text(j, "terrain_source");
  s.testOnly = j.value("test_only", false);
  return s;
}
Championship parseChampionship(std::string_view source) {
  const json j = parseJson(source, "trackrs.championship/1");
  Championship c;
  c.id = text(j, "id", true);
  if (!validId(c.id))
    throw std::runtime_error("invalid championship id \"" + c.id + "\"");
  c.name = text(j, "name", true);
  if (!j.contains("rounds") || !j.at("rounds").is_array() || j.at("rounds").empty())
    throw std::runtime_error("a championship needs at least one round");
  for (const auto &r : j.at("rounds")) {
    Round round;
    round.stage = text(r, "stage", true);
    round.day = r.value("day", 1);
    round.kind = r.value("kind", std::string("special"));
    if (round.kind != "special" && round.kind != "liaison" && round.kind != "service" && round.kind != "marathon")
      throw std::runtime_error("round kind must be special, liaison, service or marathon");
    round.timeOfDay = text(r, "time_of_day");
    round.weather = text(r, "weather");
    round.serviceAfter = r.value("service_after", false);
    c.rounds.push_back(std::move(round));
  }
  if (j.contains("points"))
    for (const auto &p : j.at("points"))
      c.points.push_back(p.get<int>());
  if (j.contains("penalties"))
    for (const auto &[k, v] : j.at("penalties").items())
      c.penalties[k] = v.get<double>();
  return c;
}

// ---- validation ------------------------------------------------------------------------
bool LoadedPack::valid() const {
  return std::none_of(issues.begin(), issues.end(), [](const Issue &i) { return i.severity == Issue::Severity::Error; });
}
bool globMatch(std::string_view pattern, std::string_view path) {
  // Recursive wildcard match (patterns are short): '*' and '?' stay within a
  // path segment, "**" crosses segments and "**/" also matches no directory.
  if (pattern.empty())
    return path.empty();
  if (pattern.substr(0, 2) == "**") {
    auto rest = pattern.substr(2);
    if (!rest.empty() && rest.front() == '/' && globMatch(rest.substr(1), path))
      return true;
    for (std::size_t i = 0; i <= path.size(); ++i)
      if (globMatch(rest, path.substr(i)))
        return true;
    return false;
  }
  if (pattern.front() == '*') {
    for (std::size_t i = 0; i <= path.size(); ++i) {
      if (globMatch(pattern.substr(1), path.substr(i)))
        return true;
      if (i < path.size() && path[i] == '/')
        break;
    }
    return false;
  }
  if (path.empty() || (pattern.front() != '?' && pattern.front() != path.front()) || (pattern.front() == '?' && path.front() == '/'))
    return false;
  return globMatch(pattern.substr(1), path.substr(1));
}
void validate(LoadedPack &pack, const Source &source) {
  auto &m = pack.manifest;
  if (m.id != pack.id)
    error(pack, "pack id \"" + m.id + "\" does not match its directory \"" + pack.id + "\"");
  if (!m.requiresEngine.satisfies(engineVersion()))
    error(pack, "requires engine " + m.requiresEngine.text + ", this is " + engineVersion().str());
  if (m.contentApi > ContentApi)
    error(pack, "needs content API " + std::to_string(m.contentApi) + ", engine provides " + std::to_string(ContentApi));
  if (m.license.spdx.empty())
    error(pack, "pack licence has no SPDX identifier");
  if (m.authors.empty())
    error(pack, "no authors listed");
  // Licence audit: every file is covered by a licensed, attributed asset entry.
  for (const auto &a : m.assets) {
    if (!safeRelative(a.path) && a.path != "**")
      error(pack, "asset path \"" + a.path + "\" escapes the pack");
    if (a.license.spdx.empty())
      error(pack, "asset \"" + a.path + "\" has no SPDX licence");
    if (a.license.attribution.empty())
      error(pack, "asset \"" + a.path + "\" has no attribution");
  }
  for (const auto &file : pack.files) {
    if (file == "pack.json")
      continue;
    const bool covered =
        std::any_of(m.assets.begin(), m.assets.end(), [&](const Asset &a) { return globMatch(a.path, file); });
    if (!covered)
      error(pack, "licence audit: \"" + file + "\" is not covered by any asset entry");
  }
  // Stages and championships.
  std::set<std::string> ids;
  for (const auto &file : pack.files) {
    const bool stageFile = file.size() > 11 && file.rfind("stages/", 0) == 0 &&
                           file.compare(file.size() - 11, 11, "/stage.json") == 0 &&
                           std::count(file.begin(), file.end(), '/') == 2;
    const bool champFile = file.rfind("championships/", 0) == 0 && file.size() > 19 &&
                           file.compare(file.size() - 18, 18, ".championship.json") == 0;
    if (!stageFile && !champFile)
      continue;
    const auto content = source.read(pack.root + "/" + file);
    if (!content) {
      error(pack, "cannot read " + file);
      continue;
    }
    try {
      if (stageFile) {
        auto s = parseStage(*content);
        const auto dir = file.substr(7, file.size() - 7 - 11);
        if (s.id != dir)
          error(pack, file + ": stage id \"" + s.id + "\" does not match its directory");
        if (!ids.insert(s.id).second)
          error(pack, "duplicate stage id " + s.id);
        if (std::find(pack.files.begin(), pack.files.end(), "stages/" + dir + "/" + s.level) == pack.files.end())
          error(pack, file + ": level file \"" + s.level + "\" is missing");
        if (s.geographicFidelity == "surveyed")
          warning(pack, file + ": \"surveyed\" fidelity requires a maintainer-verified survey source");
        pack.stages.push_back(std::move(s));
      } else {
        pack.championships.push_back(parseChampionship(*content));
      }
    } catch (const std::exception &e) {
      error(pack, file + ": " + e.what());
    }
  }
  std::sort(pack.stages.begin(), pack.stages.end(),
            [](const StageDefinition &a, const StageDefinition &b) { return a.id < b.id; });
  for (const auto &c : pack.championships)
    for (const auto &r : c.rounds) {
      const auto id = globalId(pack.id, r.stage);
      const bool local = id.rfind(pack.id + ":", 0) == 0;
      if (local && !ids.count(id.substr(pack.id.size() + 1)))
        error(pack, "championship " + c.id + ": unknown stage " + id);
    }
  pack.enabled = pack.valid();
  if (!pack.enabled)
    for (const auto &i : pack.issues)
      if (i.severity == Issue::Severity::Error) {
        pack.disabledReason = i.message;
        break;
      }
}

// ---- discovery ----------------------------------------------------------------------
const LoadedPack *Catalog::find(std::string_view id) const {
  for (const auto &p : packs)
    if (p.id == id)
      return &p;
  return nullptr;
}
const StageDefinition *Catalog::stage(std::string_view id, const LoadedPack **owner) const {
  const auto colon = id.find(':');
  if (colon == std::string_view::npos)
    return nullptr;
  const auto *pack = find(id.substr(0, colon));
  if (!pack || !pack->enabled)
    return nullptr;
  for (const auto &s : pack->stages)
    if (s.id == id.substr(colon + 1)) {
      if (owner)
        *owner = pack;
      return &s;
    }
  return nullptr;
}
std::optional<std::string> Catalog::levelPath(std::string_view id) const {
  const LoadedPack *owner = nullptr;
  const auto *s = stage(id, &owner);
  if (!s)
    return std::nullopt;
  return owner->root + "/stages/" + s->id + "/" + s->level;
}
std::size_t Catalog::enabledCount() const {
  return static_cast<std::size_t>(std::count_if(packs.begin(), packs.end(), [](const LoadedPack &p) { return p.enabled; }));
}
Catalog discover(const Source &source, const DiscoverOptions &options) {
  Catalog catalog;
  // The legacy content as a virtual pack: always present, loaded by the
  // legacy level/event loaders exactly as before.
  LoadedPack legacy;
  legacy.id = legacy.manifest.id = "classic-legacy-stages";
  legacy.root = "/maps";
  legacy.manifest.name = "Classic legacy stages";
  legacy.manifest.version = engineVersion();
  legacy.manifest.license.spdx = "GPL-2.0-or-later";
  legacy.manifest.license.attribution = "Trigger Rally / TrackRS contributors (see data/ and DATA_AUTHORS)";
  legacy.enabled = true;
  catalog.packs.push_back(std::move(legacy));
  std::vector<std::string> dirs;
  for (const auto &entry : source.list("/packs"))
    if (entry.size() > 1 && entry.back() == '/')
      dirs.push_back(entry.substr(0, entry.size() - 1));
  std::sort(dirs.begin(), dirs.end());
  for (const auto &dir : dirs) {
    LoadedPack pack;
    pack.id = dir;
    pack.root = "/packs/" + dir;
    try {
      if (!validId(dir))
        throw std::runtime_error("invalid pack directory name");
      const auto manifest = source.read(pack.root + "/pack.json");
      if (!manifest)
        throw std::runtime_error("no pack.json");
      pack.manifest = parsePack(*manifest);
      listRecursive(source, pack.root, "", pack.files, 0);
      std::sort(pack.files.begin(), pack.files.end());
      validate(pack, source);
    } catch (const std::exception &e) {
      pack.issues.push_back({Issue::Severity::Error, std::string("pack.json: ") + e.what()});
      pack.enabled = false;
      pack.disabledReason = pack.issues.back().message;
    }
    catalog.packs.push_back(std::move(pack));
  }
  // Dependencies (iterate to a fixed point: disabling one pack can disable others).
  for (bool changed = true; changed;) {
    changed = false;
    for (auto &pack : catalog.packs) {
      if (!pack.enabled)
        continue;
      for (const auto &dep : pack.manifest.depends) {
        const auto *d = catalog.find(dep.id);
        std::string why;
        if (!d)
          why = "missing dependency " + dep.id;
        else if (!d->enabled)
          why = "dependency " + dep.id + " is disabled";
        else if (!dep.range.satisfies(d->manifest.version))
          why = "dependency " + dep.id + " " + d->manifest.version.str() + " does not satisfy " + dep.range.text;
        if (!why.empty()) {
          error(pack, why);
          pack.enabled = false;
          pack.disabledReason = why;
          changed = true;
          break;
        }
      }
    }
  }
  // Edition policy: demo allows only the listed packs; dev also lists test stages.
  if (options.edition == Edition::Demo)
    for (auto &pack : catalog.packs)
      if (pack.enabled && std::find(options.demoPacks.begin(), options.demoPacks.end(), pack.id) == options.demoPacks.end()) {
        pack.enabled = false;
        pack.disabledReason = "not part of the demo edition";
      }
  if (options.edition != Edition::Dev)
    for (auto &pack : catalog.packs)
      pack.stages.erase(std::remove_if(pack.stages.begin(), pack.stages.end(),
                                       [](const StageDefinition &s) { return s.testOnly; }),
                        pack.stages.end());
  return catalog;
}

// ---- reports ---------------------------------------------------------------------------
std::string credits(const Catalog &catalog) {
  std::ostringstream out;
  out << "# Credits\n\nGenerated from the content-pack manifests by `trackrs-content credits`; do not edit by hand.\n";
  for (const auto &pack : catalog.packs) {
    if (!pack.enabled || pack.id == "classic-legacy-stages")
      continue;
    const auto &m = pack.manifest;
    out << "\n## " << m.name << " (`" << m.id << "` " << m.version.str() << ")\n\n";
    out << "Pack licence: " << m.license.spdx;
    if (!m.license.attribution.empty())
      out << " — " << m.license.attribution;
    out << "\n";
    if (!m.authors.empty()) {
      out << "Authors: ";
      for (std::size_t i = 0; i < m.authors.size(); ++i)
        out << (i ? ", " : "") << m.authors[i];
      out << "\n";
    }
    std::map<std::string, std::vector<const Asset *>> byLicense;
    for (const auto &a : m.assets)
      byLicense[a.license.spdx].push_back(&a);
    for (const auto &[spdx, assets] : byLicense) {
      out << "\n### " << spdx << "\n\n";
      for (const auto *a : assets) {
        out << "* `" << a->path << "`" << (a->kind.empty() ? "" : " (" + a->kind + ")") << ": " << a->license.attribution;
        if (!a->license.sourceUrl.empty())
          out << " <" << a->license.sourceUrl << ">";
        if (!a->license.verified)
          out << " *(source not yet verified by the maintainer)*";
        out << "\n";
      }
    }
    for (const auto &n : m.notices)
      out << "\n> " << n << "\n";
  }
  return out.str();
}
std::string summaryJson(const Catalog &catalog) {
  json packs = json::array();
  for (const auto &p : catalog.packs) {
    json issues = json::array(), stages = json::array(), championships = json::array();
    for (const auto &i : p.issues)
      issues.push_back({{"severity", i.severity == Issue::Severity::Error ? "error" : "warning"}, {"message", i.message}});
    for (const auto &s : p.stages)
      stages.push_back({{"id", p.id + ":" + s.id},
                        {"name", s.name},
                        {"length_km", s.lengthKm},
                        {"geographic_fidelity", s.geographicFidelity},
                        {"seed", s.seed},
                        {"test_only", s.testOnly}});
    for (const auto &c : p.championships)
      championships.push_back({{"id", p.id + ":" + c.id}, {"name", c.name}, {"rounds", c.rounds.size()}});
    packs.push_back({{"id", p.id},
                     {"version", p.manifest.version.str()},
                     {"enabled", p.enabled},
                     {"disabled_reason", p.disabledReason},
                     {"files", p.files.size()},
                     {"stages", stages},
                     {"championships", championships},
                     {"issues", issues}});
  }
  return json{{"schema", "trackrs.pack-catalog/1"},
              {"engine", engineVersion().str()},
              {"content_api", ContentApi},
              {"enabled", catalog.enabledCount()},
              {"packs", packs}}
      .dump(2);
}
} // namespace trackrs::content
