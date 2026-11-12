# M1: Saharan world tech (gate)

Amendment §A9.3: the A3 items 1–6 on one test stage,
`morocco-sahara:sahara-lab`. It is `test_only`,
`"geographic_fidelity": "procedural"` and represents no real place. All
images are from the harness on **software GL (llvmpipe)**: PBR renderer,
medium quality, 960×540, the default car `cordo_kitcar` on `--autopilot 100`.
GPU timings are therefore **software timings**, flagged as such; they are not
budgets.

## A3 coverage

| A3 item | What shipped | Evidence |
|---|---|---|
| **1. Desert PBR materials** | Nine CC0 sets (`sand_dune`, `fesh_fesh`, `hard_sand`, `hamada_stone`, `oued_gravel`, `dry_lake_clay`, `palmeraie_dirt`, `rock`, `tarmac_n9`) baked by `tools/bake_desert_materials.py` with per-set provenance. Nine new `TerrainType`s, with physics copied from their nearest legacy class (no legacy map uses the new colours). Terrain shader: 0.16 m wind-ripple normals with derivative fade, slip-face darkening on steep lee slopes, view-dependent grain glints, wetness off. | `noon-lake.png`, `hamada-noon.png`, `dawn-dunes.png` |
| **2. Dune generator** | `tools/stage_from_heightmap.py`: barchan, linear and star presets across the wind (78 % stoss, 22 % slip face), warped zone masks, smooth CDLOD normals. **No interactive editor:** dune fields are authored in the JSON spec. | `dawn-dunes.png` |
| **3. Persistent tracks** | New (TrackRS had no rut system to reuse; see ADR). `engine/terrain/tracks`: a 32768-segment ring fed once per tick from each wheel contact, surface-dependent rut depth, lip and darkening. Static camel and foot trails come from `props.json`. `tracks_renderer` draws ground ribbons whose fragment shader rebuilds the relief and multiplies the lit ground. Visual only, never read by physics. | `ruts-dune.png`, `ruts-palmeraie.png`, `prints-hood.png`, `prints-debug.png` |
| **4. Atmosphere** | Heat haze and mirage on flats (post, depth- and heat-scaled). Dry-air sky with turbidity from heat and dust, low-sun warm scatter, a Milky Way band and denser stars. `chergui` weather: orange grade, visibility, wind-driven streaks, dust that settles slowly and drifts with the wind. Climate comes from the level (`<weather wind winddegrees heat dust>`). | `night-stars.png`, `chergui.png`, `golden-ksar.png` |
| **5. Vegetation and props** | Pack species (date palm ×3 with dead-frond skirts, tamarisk, acacia, alfa grass, dead brush) as instanced 3D meshes with card impostors. Fifteen original prop kinds: camel and goat herds with vertex walk and graze animation, khaima, well, cairn, ksar kit (walls, crenellated towers, gate, geometric relief band), mud and concrete houses, water tower, power poles, seguia, start arch, sign board. Seeded placement keeps props off the road and off steep slopes. | `golden-ksar.png`, `ruts-palmeraie.png` |
| **6. Sun and shadow** | **CSM bug fixed:** the shadow pass inherited the scene's `glDepthRange(0, .999)`, so every terrain receiver read as occluded on every stage (legacy stages in PBR too). It now owns its depth range (ADR). Desert climates stretch the cascades ×1.6 for long views. Auto-exposure is the Phase 3d adaptive histogram and was not retuned: noon lake and hamada frames are no longer crushed now the shadows are right. Car glass and paint specular are the 4b car renderer's. | `noon-lake.png` against the M1 debugging notes in the ADR |

## Checks (A10)

| Check | Result |
|---|---|
| Legacy content and hashes | {{GOLDEN}} |
| Content packs (`tests/run_packs.py`) | {{PACKS}} |
| `ctest` | 27/27 pass (adds `terrain-tracks`: 5 cases, 141 assertions) |
| Production clang-tidy (adds `props`, `tracks`) | pass |
| Shader compile and link (`check_pbr_shaders.py`) | pass (adds `world/tracks`) |
| Determinism: two identical `sahara-lab` runs (6000 ticks) | {{DET}} |
| Times of day | dawn, noon, golden hour and night captured (`stage.json` lists all four) |
| No props on the road | {{ROAD}} |
| Licence audit | `trackrs-content validate data`: PASS. Every pack file is covered, `credits-fresh` passes, and `docs/CREDITS.md` is regenerated. |
| Cultural review | [CULTURAL_M1.md](../reviews/CULTURAL_M1.md), **`reviewed: false`** until the maintainer confirms |
| Localisation | No user-visible text added in M1 (sign boards are blank on `sahara-lab`). Bilingual signage starts in M2. |

### Budgets (software GL; flagged, not budgets)

{{BUDGETS}}

## Images

{{IMAGES}}

## Known limits (not done in M1)

* **Rut depth testing:** ribbons are pulled toward the eye in proportion to
  range, so they survive CDLOD's coarser far grids. Behind a nearby crest
  they can show through by a few decimetres. The exact fix is a
  depth-projected screen-space decal.
* **Prints at grazing angles:** camel and foot prints read at low sun and
  close range, and fade into texture noise when seen edge-on from far away
  (as in reality).
* **No rut physics:** wheels do not follow earlier ruts.
* **No dune editor UI:** dune fields are authored in the stage spec.
* **No sand particles in chergui:** the effect is screen-space streaks plus
  wind-carried car dust.
* **Exposure** was checked by eye on the noon frames, with no new metering
  mode.
