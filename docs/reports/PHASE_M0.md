# M0: pack-lite (gate)

Amendment §A9.2: `pack.json`, `stage.json`, `*.championship.json`, edition
policy, pack discovery and a validator, with the legacy adapter intact. The
Morocco content ships as pack `morocco-sahara`, and a broken pack is disabled
with a message, never a crash. The format is specified in
[CONTENT_PACKS.md](../CONTENT_PACKS.md).

## What shipped

| Part | Where |
|---|---|
| Pack model, parser, validator, semver/ranges, dependency resolution, edition filter, licence audit, credits, catalogue JSON | `engine/content/pack.{h,cpp}`: pure C++ with no GL and no PhysFS; reads through a two-function `Source` |
| Game integration | `MainApp::loadPacks()` mounts `<id>.trpack` zips, then discovers and validates `/packs/*` through PhysFS. Enabled pack stages join the stage list through the **same legacy level loader**. Disabled packs are logged and shown on the main menu (up to four lines). Validation runs write `<json>.packs.json`. |
| Legacy adapter | `/maps` and `/events` load exactly as before, listed as the virtual pack `classic-legacy-stages`. Mounting packs does not change a legacy run (hash-checked). |
| Edition policy | `data/policy/edition.json` gains an optional `demo_packs` list (`morocco-sahara` is allowed in the shipped demo edition). `test_only` stages are dev/harness only. |
| CLI | `trackrs-content validate <data>` and `trackrs-content credits <data> --output docs/CREDITS.md [--check]` |
| Stage tool | `tools/stage_from_heightmap.py`: spec, then a full legacy stage plus `stage.json`, from a real DEM (terrarium) or procedural terrain. Documented in [STAGE_TOOL.md](../STAGE_TOOL.md). |
| DEM fetch | `tools/fetch_dem.py`: Tilezen terrarium tiles with per-tile `X-Imagery-Sources` provenance and the hashed attribution document (used from M1) |
| Harness | `--stage <pack>:<stage>` and `--pack-root <dir>` |
| Pack | `data/packs/morocco-sahara`: manifest and README only. Stages arrive from M1 (test stage) and M2 (Zagora Prologue). |
| Credits | `docs/CREDITS.md`, generated (`credits-fresh` test) |

### Licence audit (A10)

* **The rule:** every file in a pack must be covered by an `assets` glob
  carrying an SPDX id and an attribution. An uncovered file disables the pack.
* **Tests:** the shipped data is validated by `content-packs-shipped`, and the
  credits file must be current (`credits-fresh`). Both fail CI otherwise.

## Validation

| Check | Result |
|---|---|
| `pack_tests` (7 cases, 96 assertions) | Covers semver and ranges; globbing; a well-formed pack; 12 broken-pack cases (bad JSON, missing manifest, id mismatch, unlicensed file, missing attribution, missing level, invalid fidelity, engine range, missing, broken and out-of-range dependencies, bad directory name), all disabled with a reason and none thrown; edition and test stages; championship references; path escapes. |
| `tests/run_packs.py` (new CI step, all Linux jobs) | See below. |
| `ctest` (26 tests) | pass |
| Layering | pass |
| clang-tidy production lint (adds `pack.cpp`) | pass (after replacing three string concatenations with a path-join helper) |
| Legacy A/B vs base (`run_baselines.py --compare`) | all 13 stages identical to base; the shipped `morocco-sahara` pack is mounted in every run |

`tests/run_packs.py` checks the following, all passing:

* With the fixture packs mounted, the game starts.
* `fixture-broken-json` and `fixture-unlicensed` are disabled, and their
  reasons appear in the log and in the catalogue.
* `fixture-good:mini` loads through the level loader. Two runs give the same
  state hash, `506333e74c4e7ca4`.
* A legacy stage gives the same hash with and without packs mounted
  (`3eebeb4e28341a5d`).
* The shipped data validates, the validator rejects the broken fixtures, and
  the credits are current.

### Not in M0

* **Profile and save migrations, signing and `.trpack` publishing:** these
  stay in Phase 9 with the rest of the master prompt's pack tooling. M0 is
  the "lite" subset the amendment pulls forward.
* **Championship runtime:** the event structure, points and penalties are
  M3. M0 only parses and validates championship files.
* **`draa-sahara-cup.championship.json`:** it ships when its stages exist
  (M5). The validator rejects rounds that reference missing stages.
