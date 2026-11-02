// SPDX-License-Identifier: GPL-2.0-or-later
/** CPU-only tests of phase-1 primitives; no SDL/GL and no shared simulation state. */
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "engine/core/fixed_step.h"
#include "engine/core/pcg32.h"
#include "engine/core/version.h"
#include "engine/core/wrap.h"
#include <limits>
#include "engine/core/edition_policy.h"
#include "engine/core/interpolation.h"

TEST_CASE("periodic coordinates handle boundaries, negative fractions and distant points") {
    CHECK(trackrs::wrapCoord(0, 8) == 0);
    CHECK(trackrs::wrapCoord(8, 8) == 0);
    CHECK(trackrs::wrapCoord(16, 8) == 0);
    CHECK(trackrs::wrapCoord(-16, 8) == 0);
    CHECK(trackrs::wrapCoord(-.25f, 8) == 7.75f);
    CHECK(trackrs::wrapCoord(8000001, 8) == 1);
    CHECK(trackrs::mapSample(-1.8f, 8, 8, true) == 6);
    CHECK(trackrs::mapSample(2.9f, 8, 8, true) == 2);
    CHECK(trackrs::mapSample(2.9f, 8, 8) == 3);
    CHECK(trackrs::mapSample(7.99f, 8, 8) == 7);
    CHECK_THROWS(trackrs::wrapCoord(1, 0));
    CHECK_THROWS(trackrs::wrapCoord(std::numeric_limits<float>::infinity(), 8));
    CHECK_THROWS(trackrs::mapSample(0, 8, 0));
}
TEST_CASE("fixed steps are per instance, bounded and resettable") {
    trackrs::FixedStep a, b;
    unsigned ac = 0, bc = 0;
    a.advance(.002f, [&](float) { ++ac; });
    b.advance(.004f, [&](float) { ++bc; });
    CHECK(ac == 0);
    CHECK(bc == 1);
    a.advance(.002f, [&](float dt) { CHECK(dt == .004f); ++ac; });
    CHECK(ac == 1);
    CHECK(a.alpha() == 0);
    CHECK(b.alpha() == 0);
    CHECK(a.advance(-1, [&](float) { ++ac; }) == 0);
    CHECK(a.advance(std::numeric_limits<float>::quiet_NaN(), [&](float) { ++ac; }) == 0);
    trackrs::FixedStep bounded(.004f, 25);
    CHECK(bounded.advance(100, [](float) {}) <= 25);
    CHECK(bounded.droppedSeconds() > 99);
    bounded.reset();
    CHECK(bounded.alpha() == 0);
    CHECK(bounded.droppedSeconds() == 0);
    CHECK_THROWS(trackrs::FixedStep(0));
}
TEST_CASE("PCG matches the reference sequence and independent streams") {
    trackrs::Pcg32 a(42, 54), b(42, 54), different(42, 55);
    for (auto expected : {0xa15c02b7u, 0x7b47f409u, 0xba1d3330u, 0x83d2f293u, 0xbfa4784bu, 0xcbed606eu}) {
        CHECK(a.next() == expected);
        CHECK(b.next() == expected);
    }
    a.reseed(42, 54);
    CHECK(a.next() != different.next());
    for (unsigned i = 0; i < 1000; ++i) {
        const auto v = a.unit();
        CHECK(v >= 0);
        CHECK(v < 1);
    }
    CHECK(trackrs::stageSeed("ahead") == trackrs::stageSeed("ahead"));
    CHECK(trackrs::stageSeed("ahead") != trackrs::stageSeed("greenland"));
}
TEST_CASE("strict versions accept SemVer and reject ambiguous compatibility strings") {
    auto v = trackrs::Version::parse("0.7.0-rc.1+build.02");
    CHECK(v.major == 0);
    CHECK(v.minor == 7);
    CHECK(v.patch == 0);
    CHECK(v.prerelease == "rc.1");
    CHECK(v.build == "build.02");
    for (const char* invalid : {"", "1", "1.2", "1.2.3.4", "01.2.3", "1.2.3-", "1.2.3-01", "1.2.3+a..b", "-1.2.3"})
        CHECK_THROWS(trackrs::Version::parse(invalid));
}

TEST_CASE("height cells preserve fractional coordinates at negative boundaries") {
    auto c = trackrs::periodicCell(-8, 8);
    CHECK(c.index == 0); CHECK(c.fraction == 0);
    c = trackrs::periodicCell(-.25f, 8);
    CHECK(c.index == 7); CHECK(c.fraction == .75f);
    c = trackrs::periodicCell(8000001, 8);
    CHECK(c.index == 1); CHECK(c.fraction == 0);
}
TEST_CASE("edition rules use stable paths and respect championship locks") {
    trackrs::EditionPolicy p;
    p.demoStages.insert("/maps/ahead/ahead.level");
    CHECK(p.stage("/maps/ahead/ahead.level")); CHECK_FALSE(p.stage("Ahead"));
    CHECK_FALSE(p.events());
    p.edition = trackrs::EditionPolicy::parse("full");
    CHECK(p.stage("any")); CHECK(p.events()); CHECK_FALSE(p.event(true, false));
    CHECK(p.event(true, true));
    p.edition = trackrs::EditionPolicy::parse("dev"); CHECK(p.event(true, false));
    CHECK_THROWS(trackrs::EditionPolicy::parse("unknown"));
}
TEST_CASE("render rotation handles antipodal quaternions without simulation writes") {
    const glm::quat a(1,0,0,0), b(-1,0,0,0);
    CHECK(trackrs::interpolateRotation(a,b,.5f) == a);
    const auto mid = trackrs::interpolateRotation(a,glm::quat(0,0,0,1),.5f);
    CHECK(mid.z == doctest::Approx(.70710678f));
    CHECK(mid.w == doctest::Approx(.70710678f));
    CHECK(trackrs::interpolateRotation(a,b,-1) == a);
}

#include "engine/application/game_state.h"
TEST_CASE("state ownership tracks real transitions only") {
    trackrs::GameStateMachine state;
    CHECK(static_cast<trackrs::AppState>(state) == trackrs::AppState::Load1);
    state = trackrs::AppState::Menu;
    CHECK(state.generation() == 1);
    state = trackrs::AppState::Menu;
    CHECK(state.generation() == 1);
    state = trackrs::AppState::Race;
    CHECK(state.generation() == 2);
}

#include "engine/application/edition_loader.h"
TEST_CASE("edition documents fail closed on malformed schema and paths") {
    const auto policy = trackrs::loadEditionPolicy(R"({"schema":"trackrs.edition/1","mode":"full","demo_stages":[]})");
    CHECK(policy.events());
    CHECK_THROWS(trackrs::loadEditionPolicy("{}"));
    CHECK_THROWS(trackrs::loadEditionPolicy(R"({"schema":"unknown","mode":"full","demo_stages":[]})"));
    CHECK_THROWS(trackrs::loadEditionPolicy(R"({"schema":"trackrs.edition/1","mode":"demo","demo_stages":["../unsafe"]})"));
    CHECK_THROWS(trackrs::loadEditionPolicy(R"({"schema":"trackrs.edition/1","mode":"demo","demo_stages":[1]})"));
}

TEST_CASE("edition stage lists require JSON arrays") {
    CHECK_THROWS(trackrs::loadEditionPolicy(R"({"schema":"trackrs.edition/1","mode":"demo","demo_stages":"/maps/ahead/ahead.level"})"));
}

TEST_CASE("dev policy bypasses championship vehicle unlocks through the same API") {
    trackrs::EditionPolicy policy;
    CHECK_FALSE(policy.championship(false));
    CHECK(policy.championship(true));
    policy.edition = trackrs::EditionPolicy::Edition::Full;
    CHECK_FALSE(policy.championship(false));
    policy.edition = trackrs::EditionPolicy::Edition::Dev;
    CHECK(policy.championship(false));
}
