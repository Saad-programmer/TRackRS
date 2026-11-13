# TrackRS 0.7 "Reality" — progress against the master prompt

The binding [Moroccan Sahara amendment](MOROCCO_AMENDMENT.md) sets the order of the
remaining work (§A9): 3e, M0 pack-lite, M1 Saharan world tech, M2 Zagora
Prologue, then 4c–4d with the A5 car priorities. Gate commits and CI runs are
recorded in [GATES.md](GATES.md); tags are created by the maintainer.

Each line links to the report that holds its evidence. "Done" means
implemented, tested and on `main` with green CI. It does not mean certified
against the §7 hardware budgets: every timing so far comes from software
rendering (Mesa llvmpipe). Real-GPU numbers come from the maintainer.

| Phase | State | Report |
|---|---|---|
| 0 Safety net (CMake, CI, golden runs, benchmark) | Done | [PHASE_0](reports/PHASE_0.md) |
| 1 Defect purge (R-07…R-16) | Done | [PHASE_1](reports/PHASE_1.md) |
| 2 Modern renderer core (GL 3.3 core, RHI, PBR, vertex formats, IBL) | Done (2a–2d) | [2A](reports/PHASE_2A.md) [2B](reports/PHASE_2B.md) [2C](reports/PHASE_2C.md) [2D](reports/PHASE_2D.md) |
| 3a CDLOD terrain, PBR splat | Done | [PHASE_3A](reports/PHASE_3A.md) |
| 3b Sky, time of day, height fog, CSM | Done | [PHASE_3B](reports/PHASE_3B.md) |
| 3c World: 3D vegetation, weather surfaces, headlights, water | Done | [PHASE_3C](reports/PHASE_3C.md) |
| 3d Post-FX: HDR target, auto-exposure, bloom, ACES, SSAO | Done (core) | [PHASE_3D](reports/PHASE_3D.md) |
| 3e TAA + golden manifest regeneration | Done (gate) | [PHASE_3E](reports/PHASE_3E.md) |
| M0 Pack-lite (amendment §A9.2) | Done (gate) | [PHASE_M0](reports/PHASE_M0.md), [CONTENT_PACKS](CONTENT_PACKS.md) |
| M1 Saharan world tech (A3 1–6, test stage) | Done (gate) | [PHASE_M1](reports/PHASE_M1.md), [cultural review](reviews/CULTURAL_M1.md) |
| M2 Zagora Prologue vertical slice | Done (gate, accepted) | [PHASE_M2](reports/PHASE_M2.md), [cultural review](reviews/CULTURAL_M2.md) |
| 4 Real rally cars, gated 4a–4d | 4a, 4b done; 4c at its gate (awaiting review) | [PHASE_4A](reports/PHASE_4A.md), [PHASE_4B](reports/PHASE_4B.md), [PHASE_4C](reports/PHASE_4C.md) |
| 5 Stage dressing | 5a people done; rest planned | [PHASE_5A](reports/PHASE_5A.md) |
| 6 Physics realism | Not started | — |
| 7 Audio, cameras, HUD, replay, photo mode, co-driver | Not started | — |
| 8 Performance gate | Partly done (pass timers, budget assertions) | — |
| 9 Content tools, packs, release engineering | Not started | — |

## 3e — temporal AA and golden manifest (done; plan kept for reference)

1. **TAA.**
   * Sub-pixel Halton jitter applied to every PBR scene pass.
   * A velocity buffer: camera motion, plus per-object motion for cars.
   * History reprojection with neighbourhood clamping, and a sharpening pass.
   * This resolves the vegetation and people LOD screen-door dither into a
     smooth cross-fade, and removes specular aliasing on fences and plants.
     Ships behind `graphics.aa`.
2. **Golden manifest regeneration — a documented one-off, in its own commit.**
   The committed `tests/baselines/linux-gcc/manifest.json` was produced by an
   older toolchain. The CI toolchain (Ubuntu 24.04, GCC 13, Mesa llvmpipe)
   reproduces different legacy hashes for the *unchanged* base commit. In this
   step the manifest is regenerated **from the CI toolchain** (the `results`
   artifact of a green `main` run). That commit contains only the manifest plus
   a note naming the source run, toolchain and base commit, and explaining that
   no game code changed. From then on, the manifest is compared exactly.

## Phase 4 — real rally cars (gated)

Each sub-step ends with a report, a tag (`phase-4a-done` …) and a stop for
maintainer review.

* **4a Car Definition v2** (done):
  * `car.json` schema (§4.3), loader, validator, and a legacy adapter so the 9
    fictional cars keep working unchanged;
  * the 11 stub vehicles migrated to `car.json`, each with its own torque
    curve, gearing, final drive, mass, tyre and aero data, marked
    `verified:false` where unsourced;
  * the data feeds the existing physics through the adapter;
  * report-only calibration tests (blocking in Phase 6).
* **4b Car rendering** (done; PBR world only, Legacy unchanged):
  * `CarRenderer`: LODs, rotating wheel nodes, emissive lights, brake glow,
    suspension-linked wheels;
  * liveries (paint mask, numbers, decals, sponsor atlas);
  * dirt and mud accumulation, clear-coat, glass, interior gauges.
* **4c Damage visuals:** dents, decals, detachable parts, cracked glass, smoke,
  all driven by the existing damage model.
* **4d Roster and sourcing:**
  * `docs/ASSET_SOURCING.md`;
  * class packs with license and `verified` metadata;
  * a "(placeholder model)" fallback for missing meshes;
  * a generated `docs/CARS.md`.

Rules: §2.4 legal and asset policy; no invented specs or sources; legacy cars,
hashes and screenshots unchanged; GPU time and VRAM reported per car.

## Phase 5 — stage dressing (remaining)

* **Road ribbon** (moved here from 3c): a spline road mesh from the traced
  centreline (`engine/dressing` tracer), with tarmac, gravel and dirt
  materials, centre and edge markings, gravel berms, and persistent tyre marks
  and ruts.
* Signage and safety furniture (§5.1), marshals and event vehicles (§5.4),
  flags, tents and night torches, crowd audio, and VAT-animated people.

## Phase 6 — physics realism (backlog items already requested)

* **Damage model rebuild, then damage visuals 1:1.** Today `PDamage`
  accumulates contact impulses × 1e-7, so real play reaches only ~0.1 per
  corner, and the 4c visuals use a gain (`graphics.damagegain`, default ×6,
  per-car `damage.visual_gain` in `car.json`). When Phase 6 rebuilds the
  damage model so that its values mean severity 0..1, **switch the visuals to
  1:1 and remove the gain**: the setting, the per-car override and
  `trackrs::car::VisualGain`. Gate check: a real crash, with no override,
  reads the same before and after the switch for equal severity.

## M5 — remaining Moroccan stages (backlog)

The amendment's §A9.8 order: stages 1–8, one per gate, then the Draâ Sahara Cup.
Items already requested for M5:

* **Lake Iriki Flats (stage 4): high-speed braking zone.** Design the dry-lake
  run so normal racing reaches brake-glow temperatures without the
  `--brake-heat` override: a long flat-out section ending in a hard stop into
  a slow waypoint or a gravel corner. The 4b model glows from 350 °C. M2
  measured only ~164 °C after 8 stops from 70 km/h on the Zagora hamada,
  because the T1 tops out near 80 km/h there. The maintainer accepted the
  forced M2 capture as proof of the rendering only. Gate check: a measured
  (unforced) night capture with glowing discs, with its `brake_c` in the
  report.

## Defaults

Legacy stays the default world renderer. **Do not flip the default until the
maintainer's real-GPU measurements** confirm the §7 budgets with
`--renderer pbr`.
