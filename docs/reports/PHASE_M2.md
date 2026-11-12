# M2: Zagora Prologue vertical slice (gate)

Amendment §A9.4: one playable stage near Zagora on real terrain, with two
rally-raid cars. Stage `morocco-sahara:zagora-prologue`, 4.31 km, 14
checkpoints, `"geographic_fidelity": "derived"`. Cars
`morocco-sahara:draa_raid_t1` and `morocco-sahara:erg_lite_t3`.

All images come from the harness on **software GL (llvmpipe)**: PBR renderer,
medium quality, 960×540, `--autopilot 110`. Timings are software timings,
flagged; they are not budgets.

## What the gate asked for

| Gate item | Result | Evidence |
|---|---|---|
| Sunrise, noon, golden-hour and night screenshots | Done, with both cars across the four times | `sunrise.jpg`, `noon.jpg`, `golden-hour.jpg`, `night.jpg` |
| Night capture of **brake-disc glow** after a long gravel run with hard braking, with visible dust and mud build-up | **Partly met, see below.** The real run (2.1 km of hamada gravel, then 8 hard-braking cycles from 70 to 30 km/h and a full stop) builds dust 1.0 and mud {{MUD}}, but the discs peak at **{{PEAK}} °C**, below the 4b model's glow threshold of 350 °C, so they do not glow. A second, labelled capture forces 700 °C (`--brake-heat`, reported as `brake_forced`) to show the glow rendering itself works. | `brake-measured.jpg`, `brake-chase.jpg`, `brake-forced.jpg` |
| Cockpit-camera capture showing the gauges | Done. New `cockpit` camera with a **placeholder** screen-space dashboard: tachometer, speedometer, steering-wheel rim and rally-raid tripmaster (distance and CAP heading). There is no 3D interior. | `cockpit.jpg` |
| Hash checks | {{HASH}} | |
| Benchmark | {{BENCH}} | |
| Licence checks | {{LICENCE}} | |

### Why the discs do not glow in the real run

A 1950 kg car stopping from 70 km/h puts about 370 kJ into its brakes, about
214 kJ of it into the front pair. Two cast-iron discs of about 8 kg each
(c ≈ 450 J/kg·K) rise only about 30 K per stop. The 4b heat model is already
stronger than that, and it still settles near {{PEAK}} °C, because the T1's
legacy physics tops out near 80 km/h on gravel. Real discs glow after
repeated braking from much higher speeds or on long descents. M2 does not
fake this: the glow is shown only in the forced, labelled capture. **Your
call:** accept the forced demonstration for this gate, or ask for a
high-speed tarmac (N9) braking zone or a retuned heat model in a later
phase.

## What shipped

| Part | Where |
|---|---|
| Real terrain | `tools/fetch_dem.py`: four zoom-13 SRTM tiles via Tilezen with per-tile provenance (`tools/stage_specs/dem/zagora/provenance.json`, sources `srtm/N30W006.tif`). Centre 30.316 N, −5.832 W; 4096 m square at 4 m. Elevation above 740 m is compressed ×0.15 so the 8-bit heightmap does not terrace the plain (recorded in `stage.json` `terrain_source`). The real relief is 702–975 m. |
| Stage | `tools/stage_specs/zagora-prologue.json`, then `stage_from_heightmap.py`. Route invented: palmeraie start, oued crossing, 2.3 km of hamada under the hill, hard sand and a barchan field to the finish. Hand-painted surfaces (A2 allows it). |
| Dressing | Ksar, mud and concrete houses, water tower, khaima with well, camel and goat herds, power line, cairns, start arch, camel and foot trails, plus the existing generic crowd and camera crews (5a). 122 of 123 props placed (1 too near the road). |
| Bilingual signs | `tools/bake_sahara_signs.py`: French and Arabic, shaped with raqm in Noto Naskh Arabic and Noto Sans (OFL, `tools/fonts/`). Atlas cells go on `sign_board` props via `"face"`. Event signs may stand at the road edge (0.5 m clearance, never on the road; dressing test). **All Arabic is `verified:false`** in `textures/signs/signs.json`. |
| Cars | `vehicles/draa_raid_t1` (T1 4×4 prototype, 1950 kg) and `vehicles/erg_lite_t3` (T3 lightweight, 950 kg): Car v2, every figure invented, `verified` all false, provenance stated. They borrow the fictional bodies `cordo_super500` and `fox_kitcar` as placeholder models. Fictional liveries recolour the bodies, add a race number (T1) and generate fictional sponsor marks over the bodies' legacy lettering. Pack cars are listed by the game, and the harness accepts `--vehicle <pack>:<car>`. |
| Cameras and harness | `cockpit` and `trackside` camera modes; `--camera`, `--brake-cycles`, `--brake-heat` (forced, reported). |

## Checks (A10)

| Check | Result |
|---|---|
| Legacy content and hashes | {{GOLDEN}} |
| Determinism | {{DET}} |
| Times of day | dawn (sunrise), noon, golden hour and night |
| Budgets | software timings below, flagged |
| No props on the road | 122 of 123 placed, 1 refused as too near the road; event signs keep 0.5 m clearance |
| Licence audit | {{LICENCE}} |
| Cultural review | [CULTURAL_M2.md](../reviews/CULTURAL_M2.md), **`reviewed: false`**. The Arabic sign text needs a fluent reviewer. |
| Localisation | Sign text only (baked French and Arabic, Latin digits). UI localisation and RTL screenshots are M4 (A6). |

### Budgets (software GL; flagged, not budgets)

{{BUDGETS}}

## Images

{{IMAGES}}

## Not done in M2

* **Rally-raid models:** placeholders on borrowed fictional bodies, with no
  long-travel suspension (Phase 6) and no 3D cockpit (the dashboard is a
  screen-space placeholder).
* **Real-run disc glow:** see above.
* **Roadbook, waypoints, bivouac and the championship file:** these are M3/A4.
* **Arabic sign text** is unreviewed.
