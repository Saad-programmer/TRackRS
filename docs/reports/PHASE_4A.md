# Phase 4a — Car Definition v2 (gate)

Status: **done, stopped at the gate for maintainer review.** 4b (car rendering)
has not started. Legacy cars, hashes and screenshots are unchanged.

## What shipped

* **Schema, loader and validator.** `car.json`, schema `trackrs.car/2` (§4.3),
  in SI units, documented in [CAR_DEFINITION.md](../CAR_DEFINITION.md). The
  code is in `engine/content/car_definition.{h,cpp}`. The validator enforces
  ranges and the §2.4 honesty rules:
  * nothing is `verified` without an https source;
  * a provenance note is required otherwise;
  * real marques need a trademark notice;
  * a car without a licensed mesh borrows a fictional body and is shown as
    "(placeholder model)".
* **Legacy adapter, both directions:**
  * `fromLegacyVehicle`: every original `.vehicle` converts and validates. The
    9 fictional cars (plus `evo_rally`, `lancer_evo` and `pbr_test`) **keep
    driving from their XML**, so physics and goldens are untouched.
  * `toLegacyVehicle`: a `car.json` drives today's physics through its fallback
    body. Gearing is mapped exactly (wheel revolutions per engine revolution =
    1/(ratio × final)), along with mass, drive split, tyre radius, brake bias,
    and the torque curve sampled every 250 rpm (legacy interpolates *power*).
    Power uses one documented constant calibrated from `lancer_evo`; legacy
    power is not SI.
* **The 11 real cars.**
  * Integration: authored as `data/vehicles/<id>/car.json`, listed in the game
    next to the original cars, and selectable with `--vehicle <id>`.
  * Distinct data: each has its own torque curve, gear ratios, final drive,
    mass, tyre, aero and brake data, with no shared curve or gearbox (tested).
  * Generator: `tools/seed_real_cars.py`, with a freshness CTest.
* **Calibration, report-only.** `tests/run_calibration.py` runs a flat, dry
  tarmac stage (`data/testing/calibration/`, not listed in game) with a new
  `--telemetry <csv>` validation option. It measures 0–100 km/h, the highest
  speed in 30 s and the 100→0 km/h braking distance, plus a static dyno of
  peak power and torque. It runs every one of the 23 cars and becomes blocking
  in Phase 6.

## Important: what is *not* real data

The 11 `.vehicle` stubs named in the brief **are not in this repository**:
neither the working tree nor git history has them, as `ASSETS_TODO` already
recorded. When I tried to source specifications, reference sites
(en.wikipedia.org) were blocked by this session's network policy. Following
"never invent specs or sources":

* every value of the 11 cars is `verified: false`, `sources` is empty, and
  `provenance` says the numbers are **unverified seeds**: approximate headline
  figures (power, torque and their rpm, mass, layout, gear count, top speed)
  recalled without a source;
* gear ratios and final drive are **synthetic**. Each car's `derivation` says
  so: final drive 4.1, top gear reaching the claimed top speed at the redline,
  a traction-limited first gear (μ 1.3), and geometric spacing;
* torque curves are generated from each car's own peak figures, using a turbo
  or naturally aspirated shape.

To make any value real, add the citation URL to `sources`, correct the value
in the generator table, and flip that `verified` key. The validator then
accepts it as verified. Allowing `en.wikipedia.org` (or another reference
host) in the environment would let me do this sourcing pass.

## Calibration (report-only)

Full table: [calibration.md](phase4a/calibration.md); raw data:
[calibration.json](phase4a/calibration.json). Extract:

| car | 0–100 s | max km/h in 30 s | claimed top | 100→0 m |
|---|---|---|---|---|
| Audi Quattro A2 | 4.95 | 220.0 | 220 | 27.4 |
| Peugeot 205 T16 E2 | 5.54 | 212.3 | 210 | 25.3 |
| Lancia Stratos HF | 6.50 | 234.4 | 230 | 25.5 |
| Subaru Impreza WRC97 | 5.24 | 211.2 | 210 | 28.7 |
| Toyota Celica GT-Four ST185 | 4.88 | 210.7 | 210 | 28.4 |
| Cordo (fictional, `cordo_kitcar`) | 5.26 | 206.7 | — | 29.1 |

How to read it:

* **Top speed is not an independent check.** Synthetic gearing was fitted to
  the claimed top speed at the redline, so each car tops out at its claim.
* **0–100 and braking come from the legacy physics**, which has no wheel
  inertia, a non-SI power scale and a simple brake model. For example, the
  205 T16 E2 is slower to 100 than lighter-but-weaker cars, because a
  traction-limited first gear and the legacy wheel model punish its short
  gearing. These numbers are the Phase 6 baseline, not a claim of realism.
* **Pre-existing defect:** `lancer_evo` (an existing legacy file) goes
  non-finite within about 0.4 s on every stage, both here and with the pre-4a
  `main` binary. The report marks it "aborted"; it is recorded in ASSETS_TODO
  and was not changed.

## GPU memory and time per car

[gpu.md](phase4a/gpu.md). The 11 real cars are placeholders, so their GPU
cost *is* their fallback body's:

| body | triangles | mesh MB | textures MB (with mips) | mesh_submit GPU ms/frame (llvmpipe) |
|---|---|---|---|---|
| `evo_rally` (AWD placeholders) | 3618 | 0.33 | 10.67 | 0.12–0.16 |
| `fox_wrc` (RWD placeholders) | 778 | 0.07 | 5.33 | 0.06–0.07 |

Mesh memory is estimated as triangles × 3 vertices × 32 B; textures as
w × h × 4 B × 4/3. GPU times come from the software renderer, so they are
relative only. Real-GPU numbers per car will come with the 4b `CarRenderer`.

## Verification

* `car-definition-v2`, five doctest cases:
  * all 11 real cars validate, are honest (nothing verified, provenance,
    placeholder, synthetic gearing, trademark notice) and distinct, and reach
    their claimed top speed in top gear;
  * every legacy `.vehicle` converts and validates;
  * the `lancer_evo` power constant is pinned at 223.7 kW @ 5500 rpm;
  * Car v2 → legacy XML maps gearing exactly and round-trips power, torque,
    split, radius and bias;
  * the validator rejects dishonest or broken definitions.
* `car-seed-fresh`, layering and the production clang-tidy set (now including
  `car_definition.cpp`) pass. All 21 CTest tests pass.
* Legacy A/B (ahead/cordo, aegyptian/evo, greenland/fox): identical state and
  framebuffer hashes against the base build. The world runner's physics hash
  is unchanged.
