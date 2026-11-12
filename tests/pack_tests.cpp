// SPDX-License-Identifier: GPL-2.0-or-later
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "engine/content/pack.h"
#include "vendor/doctest/doctest.h"
#include <algorithm>
#include <map>
using namespace trackrs::content;
namespace {
// In-memory PhysFS stand-in: path -> content; directories are implied.
struct Memory {
  std::map<std::string, std::string> files;
  Source source() const {
    Source s;
    s.list = [this](const std::string &dir) {
      std::vector<std::string> out;
      const auto prefix = dir + "/";
      for (const auto &[path, content] : files) {
        if (path.rfind(prefix, 0) != 0)
          continue;
        const auto rest = path.substr(prefix.size());
        const auto slash = rest.find('/');
        const auto entry = slash == std::string::npos ? rest : rest.substr(0, slash + 1);
        if (std::find(out.begin(), out.end(), entry) == out.end())
          out.push_back(entry);
      }
      return out;
    };
    s.read = [this](const std::string &path) -> std::optional<std::string> {
      const auto it = files.find(path);
      if (it == files.end())
        return std::nullopt;
      return it->second;
    };
    return s;
  }
};
std::string manifest(const std::string &id, const std::string &extra = "",
                     const std::string &assets = R"([{"path":"**","kind":"data","license":{"spdx":"CC0-1.0","attribution":"Test author"}}])") {
  return R"({"schema":"trackrs.pack/1","id":")" + id + R"(","name":"Test )" + id +
         R"(","version":"1.2.0","requires_engine":">=0.6 <1.0","authors":["Test author"],
             "license":{"spdx":"GPL-2.0-or-later","attribution":"Test author"},"assets":)" + assets + extra + "}";
}
std::string stage(const std::string &id, const std::string &extra = "") {
  return R"({"schema":"trackrs.stage/1","id":")" + id + R"(","name":"Stage )" + id +
         R"(","level":")" + id + R"(.level","stage_seed":42,"geographic_fidelity":"procedural","length_km":3.5)" + extra + "}";
}
Memory goodPack(const std::string &id = "test-pack") {
  Memory m;
  const auto root = "/packs/" + id;
  m.files[root + "/pack.json"] = manifest(id);
  m.files[root + "/stages/one/stage.json"] = stage("one");
  m.files[root + "/stages/one/one.level"] = "<level/>";
  m.files[root + "/championships/cup.championship.json"] =
      R"({"schema":"trackrs.championship/1","id":"cup","name":"Cup","rounds":[{"stage":"one","day":1}],"points":[10,6,4]})";
  return m;
}
} // namespace

TEST_CASE("semantic versions and ranges") {
  CHECK(SemVer::parse("1.2.3") == SemVer{1, 2, 3});
  CHECK(SemVer::parse("0.7") == SemVer{0, 7, 0});
  CHECK_FALSE(SemVer::parse("1.2.3.4"));
  CHECK_FALSE(SemVer::parse("a.b"));
  CHECK_FALSE(SemVer::parse(""));
  const auto range = [](const char *r, const char *v) { return VersionRange::parse(r)->satisfies(*SemVer::parse(v)); };
  CHECK(range(">=0.7 <1.0", "0.7.0"));
  CHECK(range(">=0.7 <1.0", "0.9.9"));
  CHECK_FALSE(range(">=0.7 <1.0", "1.0.0"));
  CHECK_FALSE(range(">=0.7 <1.0", "0.6.7"));
  CHECK(range("^1.2", "1.9.0"));
  CHECK_FALSE(range("^1.2", "2.0.0"));
  CHECK_FALSE(range("^1.2", "1.1.0"));
  CHECK(range("^0.3", "0.3.5"));
  CHECK_FALSE(range("^0.3", "0.4.0"));
  CHECK(range("~1.2.3", "1.2.9"));
  CHECK_FALSE(range("~1.2.3", "1.3.0"));
  CHECK(range("1.2.3", "1.2.3"));
  CHECK_FALSE(range("1.2.3", "1.2.4"));
  CHECK(range("*", "9.9.9"));
  CHECK(range("", "0.0.1"));
  CHECK_FALSE(VersionRange::parse(">=x"));
  CHECK(engineVersion().str() == TRACKRS_ENGINE_VERSION);
}

TEST_CASE("glob matching for asset coverage") {
  CHECK(globMatch("**", "stages/a/b.png"));
  CHECK(globMatch("stages/*/stage.json", "stages/one/stage.json"));
  CHECK_FALSE(globMatch("stages/*.json", "stages/one/stage.json"));
  CHECK(globMatch("stages/**/*.png", "stages/one/tex/a.png"));
  CHECK(globMatch("stages/**/*.png", "stages/a.png"));
  CHECK(globMatch("terrain/dem-?.raw", "terrain/dem-1.raw"));
  CHECK_FALSE(globMatch("terrain/*.raw", "terrain/x.png"));
}

TEST_CASE("a well-formed pack is enabled with its stages and championship") {
  const auto m = goodPack();
  const auto catalog = discover(m.source());
  REQUIRE(catalog.packs.size() == 2);
  CHECK(catalog.packs[0].id == "classic-legacy-stages"); // legacy adapter always present
  const auto *p = catalog.find("test-pack");
  REQUIRE(p);
  CHECK(p->enabled);
  CHECK(p->issues.empty());
  REQUIRE(p->stages.size() == 1);
  CHECK(p->stages[0].seed == 42);
  CHECK(p->championships.size() == 1);
  CHECK(catalog.levelPath("test-pack:one") == std::optional<std::string>("/packs/test-pack/stages/one/one.level"));
  CHECK_FALSE(catalog.levelPath("test-pack:missing"));
  CHECK_FALSE(catalog.levelPath("nopack:one"));
  CHECK(catalog.enabledCount() == 2);
  const auto credits = trackrs::content::credits(catalog);
  CHECK(credits.find("Test test-pack") != std::string::npos);
  CHECK(credits.find("CC0-1.0") != std::string::npos);
}

TEST_CASE("broken packs are disabled with a reason, never thrown") {
  Memory m = goodPack();
  m.files["/packs/bad-json/pack.json"] = "{ not json";
  m.files["/packs/no-manifest/readme.txt"] = "x";
  m.files["/packs/wrong-id/pack.json"] = manifest("other-id");
  m.files["/packs/unlicensed/pack.json"] = manifest("unlicensed", "", R"([{"path":"textures/*.png","kind":"texture","license":{"spdx":"CC0-1.0","attribution":"A"}}])");
  m.files["/packs/unlicensed/textures/a.png"] = "png";
  m.files["/packs/unlicensed/sounds/wind.ogg"] = "ogg"; // not covered by any asset entry
  m.files["/packs/no-attribution/pack.json"] =
      manifest("no-attribution", "", R"([{"path":"**","kind":"data","license":{"spdx":"CC0-1.0"}}])");
  m.files["/packs/missing-level/pack.json"] = manifest("missing-level");
  m.files["/packs/missing-level/stages/two/stage.json"] = stage("two");
  m.files["/packs/bad-stage/pack.json"] = manifest("bad-stage");
  m.files["/packs/bad-stage/stages/three/stage.json"] = stage("three", R"(,"geographic_fidelity":"replica")");
  m.files["/packs/bad-stage/stages/three/three.level"] = "<level/>";
  m.files["/packs/future/pack.json"] = R"({"schema":"trackrs.pack/1","id":"future","name":"F","version":"1.0.0",
      "requires_engine":">=9.0","authors":["a"],"license":{"spdx":"CC0-1.0","attribution":"a"},"assets":[]})";
  m.files["/packs/needs-missing/pack.json"] = manifest("needs-missing", R"(,"depends":[{"id":"absent","version":"^1.0"}])");
  m.files["/packs/needs-broken/pack.json"] = manifest("needs-broken", R"(,"depends":[{"id":"bad-json"}])");
  m.files["/packs/needs-newer/pack.json"] = manifest("needs-newer", R"(,"depends":[{"id":"test-pack","version":"^2.0"}])");
  m.files["/packs/needs-ok/pack.json"] = manifest("needs-ok", R"(,"depends":[{"id":"test-pack","version":"^1.0"}])");
  m.files["/packs/Bad Dir/pack.json"] = manifest("bad-dir");
  Catalog catalog;
  REQUIRE_NOTHROW(catalog = discover(m.source()));
  const auto disabled = [&](const char *id, const char *needle) {
    const auto *p = catalog.find(id);
    REQUIRE(p);
    CHECK_FALSE(p->enabled);
    CHECK(p->disabledReason.find(needle) != std::string::npos);
  };
  disabled("bad-json", "pack.json");
  disabled("no-manifest", "no pack.json");
  disabled("wrong-id", "does not match");
  disabled("unlicensed", "sounds/wind.ogg");
  disabled("no-attribution", "attribution");
  disabled("missing-level", "missing");
  disabled("bad-stage", "geographic_fidelity");
  disabled("future", "requires engine");
  disabled("needs-missing", "missing dependency absent");
  disabled("needs-broken", "bad-json is disabled");
  disabled("needs-newer", "does not satisfy");
  disabled("Bad Dir", "invalid pack directory");
  CHECK(catalog.find("test-pack")->enabled);
  CHECK(catalog.find("needs-ok")->enabled);
  const auto summary = summaryJson(catalog);
  CHECK(summary.find("\"disabled_reason\"") != std::string::npos);
}

TEST_CASE("edition policy and test-only stages") {
  Memory m = goodPack();
  m.files["/packs/test-pack/stages/lab/stage.json"] = stage("lab", R"(,"test_only":true)");
  m.files["/packs/test-pack/stages/lab/lab.level"] = "<level/>";
  DiscoverOptions demo;
  demo.edition = Edition::Demo;
  auto c = discover(m.source(), demo);
  CHECK(c.find("classic-legacy-stages")->enabled);
  CHECK_FALSE(c.find("test-pack")->enabled);
  CHECK(c.find("test-pack")->disabledReason == "not part of the demo edition");
  auto full = discover(m.source());
  CHECK(full.find("test-pack")->stages.size() == 1); // test stage hidden
  DiscoverOptions dev;
  dev.edition = Edition::Dev;
  auto d = discover(m.source(), dev);
  CHECK(d.find("test-pack")->stages.size() == 2);
  CHECK(d.levelPath("test-pack:lab"));
}

TEST_CASE("championship rounds must reference existing stages") {
  Memory m = goodPack();
  m.files["/packs/test-pack/championships/bad.championship.json"] =
      R"({"schema":"trackrs.championship/1","id":"bad","name":"Bad","rounds":[{"stage":"nowhere"}]})";
  const auto c = discover(m.source());
  CHECK_FALSE(c.find("test-pack")->enabled);
  CHECK(c.find("test-pack")->disabledReason.find("unknown stage test-pack:nowhere") != std::string::npos);
  CHECK_THROWS(parseChampionship(R"({"schema":"trackrs.championship/1","id":"x","name":"X","rounds":[]})"));
  CHECK_THROWS(parseChampionship(R"({"schema":"trackrs.championship/1","id":"x","name":"X","rounds":[{"stage":"a","kind":"party"}]})"));
}

TEST_CASE("stage paths cannot escape the pack") {
  CHECK_THROWS(parseStage(stage("x", R"(,"level":"../../maps/ahead/ahead.level")")));
  CHECK_THROWS(parseStage(stage("x", R"(,"level":"/maps/ahead/ahead.level")")));
  CHECK_THROWS(parseStage(stage("x", R"(,"length_km":-1)")));
}
