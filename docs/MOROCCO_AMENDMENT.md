# ▌TRACKRS 0.7 — MOROCCAN SAHARA EDITION
## Amendment A to `docs/CODEX_MASTER_PROMPT.md` (binding; save as `docs/MOROCCO_AMENDMENT.md`)

**Precedence:** this amendment *extends* the master prompt. Every earlier rule stays in force: phase gates, golden hashes and screenshots, the legal/asset policy (§2.4), the defect register, performance budgets (§7), "never fabricate", "legacy content must keep loading", report-per-gate. Where this file and the master prompt conflict, the stricter rule wins.

---

## A0 · Housekeeping first (no feature work until done)
1. **Tags.** The proxy refuses tag pushes (HTTP 403). Stop retrying. Record every gate in `docs/GATES.md` (`gate name | commit SHA | CI run URL | date`) and push that file. The maintainer creates tags by hand. Note in `GATES.md` that `phase-3b-done` belongs on commit `ec087af` and `phase-4a-done` on `af02f36`.
2. Accept 4a as done (CI run 109 green on `af02f36`).
3. Do not flip the default renderer; that still waits for real-GPU numbers from the maintainer.

---

## A1 · CREATIVE BRIEF — "RALLYE SAHARA DRAÂ"
Turn TrackRS into a **Moroccan Saharan rally game**: the desert-gate region around **Zagora and the Draâ valley**, in the spirit of rally-raid events (long desert stages, roadbook navigation, bivouac) and Morocco's rally scene.

* **Atmosphere:** burning blue sky, golden hour and sunrise bivouacs, dust and heat haze, dunes, stony *hamada*, dry riverbeds (*oueds*), palm groves (*palmeraies*), mud-brick *ksour* and kasbahs, Amazigh (Berber) nomad tents, camels and goats, village edges, desert stars at night.
* **Fictional event, real geography.** The event name, teams, drivers, sponsors and routes are **invented**. Do NOT use the name "Dakar", ASO branding, any real event's logos, real sponsors or real people's names or likenesses. Real place names (Zagora, Tamegroute, Agdz, Mhamid, Foum Zguid, Tazzarine, Alnif, Ouarzazate, Jbel Saghro, Erg Chigaga, Lake Iriki) and real car makes follow the existing licence/trademark rules.
* **Respect.** Depict people and culture with care: no caricature, no clichés as comedy, no religious audio used as ambience. Cultural review checklist in A9 is a gate item.

---

## A2 · GEOGRAPHY AND STAGES (all shipped as a content pack `morocco-sahara`)
Eight special stages plus a prologue, each a **fictional route over real terrain character**. Game stages are 3–30 km; the "marathon" is a chain of three stages with a bivouac between.

| # | Stage | Terrain character |
|---|---|---|
| 0 | **Zagora Prologue** | palmeraie edges, packed-earth piste, kasbah backdrop, short, spectator-heavy start arch |
| 1 | **Draâ Palmeraie** | irrigation channels, narrow palm-lined pistes, dust and shade bands |
| 2 | **Tamegroute Ksar** | village approach, mud walls, tarmac-to-dirt transition, crowds, camera crews |
| 3 | **Jbel Saghro Rocks** | rocky hamada, steps, rock gardens, long-travel suspension test |
| 4 | **Lake Iriki Flats** | enormous dry lake bed, near-flat, extreme speed, mirage, navigation by compass |
| 5 | **Foum Zguid Oued** | dry riverbed gravel, banks, acacia, wash-outs |
| 6 | **Erg Chigaga Dunes** | big dunes, soft sand, slip faces, wind ripples |
| 7 | **Mhamid Marathon (3 parts)** | mixed, with a sunrise bivouac and a night section |
| 8 | **N9 Liaison Sprint** (tarmac) | desert highway, heat shimmer, camel crossings, optional |

* **Terrain data:** use **public elevation data** (NASA SRTM or Copernicus GLO-30; record licence and attribution in the pack manifest) through the existing `stage-from-heightmap` tool. Surface classification may be hand-painted or derived from public imagery with a compatible licence. OpenStreetMap track data is optional and **ODbL**: if used, isolate the derived data and carry the attribution and share-alike notice in the pack licence block.
* **If network access is unavailable,** generate terrain procedurally (ridged noise, wind-aligned dune fields, eroded wadis) and mark `"geographic_fidelity": "procedural"` in each stage. Never claim a stage is a surveyed replica.
* **Championship:** `draa-sahara-cup.championship.json` (prologue + 6 specials + marathon), with day structure (liaison, special, service/bivouac), time penalties, and a points table.

---

## A3 · SAHARAN RENDERING TECHNOLOGY (extends Phase 3; same quality gates)
1. **Sand and desert materials (PBR):** `sand_dune`, `fesh_fesh` (powdery fine sand), `hard_sand`, `hamada_stone`, `oued_gravel`, `dry_lake_clay` (cracked), `palmeraie_dirt`, `rock`, `tarmac_n9`. Sources must be CC0 / own-made with manifest entries. Add **wind-ripple normal detail** and **grain sparkle** (view-dependent glints), dune **slip-face shading** (steepness-driven darkening), and wetness off by default.
2. **Dune generator and editor:** wind-direction-aware dune fields (barchan/linear/star presets) feeding the heightmap, with CDLOD-friendly smooth normals.
3. **Persistent tracks in sand:** reuse the tyre-mark/rut system: deeper ruts, displaced sand lips, camel and foot tracks as static decals.
4. **Atmosphere:** heat-haze refraction (post-FX, distance- and temperature-scaled), **mirage** shimmer on flats, dry-air sky with strong blue gradient, sunrise/sunset warm scatter, **night sky with stars and Milky Way**, **sandstorm (chergui)** weather with orange grade, visibility fog and blowing-sand particles, dust plumes that hang and drift with wind (volumetric look, car-speed driven, long persistence for following cars).
5. **Vegetation and props (instanced, LOD'd, wind-animated):** date palms (several heights, dead-frond variants), tamarisk, acacia, alfa grass tufts, dead brush; camel herds and goats (instanced vertex-animation), nomad tents (*khaima*), wells, stone cairns, **ksar/kasbah modular kit** (pisé walls, crenellated corner towers, geometric Amazigh relief patterns, wooden gates), mud-brick village houses, flat-roof concrete houses, water towers, power-line poles, irrigation channels.
6. **Sun and shadow:** high-sun hard shadows with CSM tuned for long views; strong specular on glass and paint; auto-exposure tuned for bright scenes.

---

## A4 · NEW GAMEPLAY SYSTEMS (extends Phases 6–7)
1. **Roadbook mode** (alongside pace notes): tulip-diagram icons, compass heading (CAP), odometer, waypoint radius capture, **visible, hidden and safety waypoint types**, missed-waypoint time penalty, detour hints, off-route warning. Roadbook files are data (`roadbook.json`) with an authoring tool.
2. **Event structure:** liaison, special stage, service/bivouac, marathon stage (limited repairs), fuel range and refuel points, overheating, puncture and repair timers.
3. **Tyre pressure setup** (airing down for sand): changes contact patch, sinkage, grip and puncture risk. Part of the setup menu and physics.
4. **Sand and soft-surface physics:** sinkage, rolling resistance by depth and speed, momentum-dependent bogging, **getting stuck and recovery** (sand ladders, tow, dig-out minigame with time cost), dune crest launches and landings, low-range gearing.
5. **Heat and wear:** engine and brake temperature, dust in air filter (power loss) with a cleaning action at service.
6. **AI and rivals:** stat-model rivals post stage times; ghosts share via files.

---

## A5 · CARS AND CLASSES (updates Phase 4 roster priority)
Same legal rules as §2.4 (licensed or own models only; fictional fallback labelled "placeholder model"; `verified:false` unless cited).
1. **Priority 1, rally-raid:** T1+ 4×4 prototypes (Toyota Hilux T1+, Ford Ranger Raptor T1+, Mini John Cooper Works Rally, Peugeot 3008 DKR Maxi, Audi RS Q e-tron hybrid, Dacia Sandrider), T1/T2 production 4×4 (Toyota Land Cruiser 79, Nissan Patrol), **T3 lightweight prototypes**, and **T4 side-by-side vehicles** (optional).
2. **Priority 2, heritage raid:** Peugeot 205 T16 Grand Raid, Peugeot 405 T16 GR, Citroën ZX Rallye Raid, Mitsubishi Pajero, Porsche 959 Paris-Dakar.
3. **Priority 3, the existing rally roster** from §5-P4 (classics, Group B, Group A, WRC), kept for the tarmac and gravel stages.
4. **Fictional Moroccan teams and liveries:** palettes inspired by desert colours, zellige-style geometric decals, national colours used respectfully. All team names, numbers and drivers are invented; no real sponsors.
5. Physics data for raid cars: long-travel suspension, 4WD with locking centre, high ground clearance, big wheels, fuel capacity, restricted-power rules as data. Calibration tests stay report-only until Phase 6, then become blocking.
6. Trucks are **out of scope** for 0.7 (record as backlog).

---

## A6 · LOCALISATION — ARABIC, FRENCH, ENGLISH, DARIJA
1. **Languages:** English, French, Arabic (MSA), **Darija (Moroccan Arabic, Arabic script)** for UI strings and co-driver subtitles; optional Tamazight (Tifinagh) for menu names.
2. **Real RTL support:** bidirectional text and Arabic shaping with **HarfBuzz + FriBidi** (or equivalent compatible libraries; GPL-2 compatible), SDF glyph atlases generated at build time from **OFL fonts** (Noto Sans Arabic / Naskh; Noto Sans Tifinagh), mirrored layouts in menus and HUD, correct number handling. Default to **Latin digits** (as used in Morocco) with an Arabic-Indic option.
3. **String tables** (`lang/*.json`), pseudo-localisation test mode, automated **RTL screenshot tests** for every menu and the HUD, and text-overflow checks for long French strings.
4. **Voice:** do not synthesise or imitate real voices. Co-driver text is shown as subtitles and stage-notes in all languages; record `docs/VOICE_TODO.md` listing the lines needed so the maintainer can record Darija, French and English voice sets.
5. **Signage:** bilingual Arabic/Latin road, distance and warning signs in a Moroccan style (research and cite; mark `verified:false` if unsourced), plus Tifinagh decorative lettering on selected props.

---

## A7 · AUDIO AND MUSIC
Ambient beds: desert wind, insects, distant village, goats, camel calls, helicopter and drone, crowd murmur; surface-specific tyre sounds for sand, hamada, gravel and tarmac; engine and turbo as in Phase 7. **Music** only if original or CC0/CC-BY (instrument palette: oud, gimbri, bendir and ambient electronic), with attribution in CREDITS. No copyrighted tracks, no religious recitation as background.

---

## A8 · USER INTERFACE AND ART DIRECTION
Sand, terracotta, indigo and night-blue palette; geometric zellige and Amazigh patterns as UI motifs (original artwork); main menu with an in-engine Zagora sunrise scene; stage cards with a mini-map and surface breakdown; loading screens rendered in-engine; roadbook-style HUD option. All shipped artwork is original or licensed with a manifest entry.

---

## A9 · PHASING (replaces the order of remaining phases; gates and reports unchanged)
1. **A0 housekeeping** (tags, `GATES.md`), then **3e** (TAA + one-off baseline regeneration from the CI toolchain, separate commit).
2. **M0, Pack-lite (pulled forward from Phase 9):** `pack.json`, `stage.json`, `*.championship.json`, edition policy, pack discovery and validator, legacy adapter intact. The Morocco content ships as pack `morocco-sahara`; a broken pack is disabled with a message, never a crash.
3. **M1, Saharan world tech:** A3 items 1–6 on a single test stage.
4. **M2, Vertical slice:** **Zagora Prologue** fully dressed (palmeraie, ksar, spectators, camera crew, signage, bilingual signs) with 2 rally-raid cars. Gate includes screenshots at sunrise, noon, golden hour and night, and a short benchmark.
5. **4b to 4d** (car rendering, damage, roster) with the A5 priorities.
6. **M3, Gameplay:** A4 roadbook mode, tyre pressure, sand physics, bivouac and recovery.
7. **M4, Localisation:** A6, with RTL screenshot tests.
8. **M5, Remaining stages** (1–8) one per gate, then the Draâ Sahara Cup.
9. **Phases 5 to 9** continue with Moroccan flavour: spectators in local dress, trackside crews, helicopters and drones, marshals, plus the road ribbon, and then performance, tools and release. Release **v0.7.0 "Sahara"**.

---

## A10 · ACCEPTANCE CHECKS (added to every stage gate)
* Legacy content still loads; golden hashes and legacy screenshots unchanged.
* Each Moroccan stage: determinism test (same seed → same dressing hash), three times of day, budgets from §7 (software timings flagged as non-hardware), no props on the road, placement within safety rules.
* **Data and licence audit:** every DEM, texture, model, font, sound and track source has a manifest entry with licence and attribution; CI fails otherwise. `CREDITS.md` regenerates.
* **Cultural review checklist** committed with each gate: no real brands or persons, no stereotype props, respectful audio and signage, bilingual text verified by a human reviewer (mark `reviewed:false` until the maintainer confirms).
* **Localisation:** RTL screenshots for all menus, no clipped text in any language, Latin digits by default.

**Begin with A0, then 3e, then M0.**
