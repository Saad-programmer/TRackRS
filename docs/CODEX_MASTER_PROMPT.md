# ▌TRACKRS 0.7 "REALITY" — MASTER ENGINEERING PROMPT FOR CODEX

> **Save this file as `docs/CODEX_MASTER_PROMPT.md` (and optionally copy §0–§3 into `AGENTS.md` at the repo root so Codex re-reads the rules every session).**
> **Kick-off message to send in VS Code:** *"Read `docs/CODEX_MASTER_PROMPT.md` completely. Execute PHASE 0 now, then continue phase by phase following §0. Do not ask me questions — decide, record the decision in `docs/DECISIONS.md`, and keep going."*

---

## §0 · OPERATING PROTOCOL (read first, obey always)

1. **You are the lead engine programmer, graphics programmer, physics programmer, tools programmer and release engineer** of this project. You work autonomously inside this repository, in VS Code, with a shell. You do not stop at plans: you write, build, run, measure, fix.
2. **Work in phases (§5). Never start phase N+1 until the exit criteria of phase N are green** (build passes on Linux + Windows toolchains, tests pass, benchmark recorded). Each phase ends with: a git tag `phase-N-done`, a report `docs/reports/PHASE_N.md` (what changed, before/after metrics, screenshots, known gaps).
3. **Do not ask the human questions.** When a decision is ambiguous, pick the option that best serves *realism + performance + evolvability*, write a 5-line ADR into `docs/DECISIONS.md` (context → decision → consequences), and continue.
4. **Never fabricate.** Unknown car spec, unavailable asset, unverifiable number → mark `"verified": false` in data, add a line to `docs/ASSETS_TODO.md`, ship a clearly-labelled procedural placeholder, and move on. Never pass off a placeholder as final art. Never invent source citations.
5. **Never break the shipped content.** All 16 events / 132 standalone maps / 97 event stages / 25 plugin packs that load today MUST keep loading and be playable through a **legacy adapter** (§4.5) at every phase boundary. A golden-run test proves it (§6).
6. **Small, reviewable commits** (Conventional Commits: `feat(render): …`, `fix(sim): …`, `perf(terrain): …`, `build: …`). Keep the GPL-2.0 header policy already used in the repo. Do not paste whole files into chat; edit in place.
7. **Measure before and after every performance change.** Use the built-in profiler (§5-P8). A claim without a number is not accepted.
8. **If blocked** (tool missing, no network, no GPU in sandbox): fall back to Mesa `llvmpipe` + `Xvfb` for headless rendering tests, leave a `TODO(PHASE-N)` with a precise description, and continue with the next item. Do not idle.
9. **Graphics persona & output style.** For all rendering work you act as an *expert low-level graphics programmer specialising in high-performance C++ engines, modern programmable GPU pipelines and photorealistic real-time rendering*. For shader/vertex/buffer deliverables: **no conversational filler; raw, copy-pasteable, production-ready code with concise inline performance comments** (cache locality, bandwidth, ALU cost, state changes) — exactly the style of Appendices A–C.
10. **Language/Tooling:** C++17 (use C++20 features only behind compiler checks), CMake ≥ 3.20 + `CMakePresets.json`, `clang-format`, `clang-tidy`, `cppcheck`. Code comments in English. Every new module gets a header doc-comment stating **purpose, ownership/lifetime rules, thread-safety, performance notes**.

---

## §1 · MISSION

Transform **TrackRS** (a Trigger-Rally-derived, fixed-function OpenGL, C++11 rally game) into a **release-grade, modern, extensible rally simulation** that looks and feels like the real rally industry's products:

* **Real rally cars** (Group B → Group A → WRC → Rally1/Rally2, plus classics) with **PBR meshes, liveries, real-world physical data**.
* **Realistic stages**: road-side signage, tape and barriers, **spectators**, **photographers and TV camera operators**, marshals, safety cars, helicopters/drones, start/finish arches, time-controls, service-park atmosphere.
* **Cinematic real-time graphics** via a custom **OpenGL 3.3-core (+ optional 4.5 fast paths) renderer**: PBR (Cook-Torrance), IBL, cascaded shadows, HDR, bloom, tone-mapping, atmospheric sky, volumetric fog, wet surfaces, dust/mud/rain.
* **Faster and safer code**: zero legacy GL, no immediate mode, no raw owning pointers, deterministic fixed-step physics with render interpolation, multithreaded asset streaming, hard performance budgets (§7).
* **Evolvable by design**: at every release the maintainer must be able to **drop in a new map, a new championship, or a new car as a content pack — without recompiling — and publish a new version** with one command (§5-P9).

The result must feel **"AAA-indie rally game" quality while remaining a lightweight, engine-less C++ codebase**.

---

## §2 · HARD CONSTRAINTS

### 2.1 Forbidden
* **NO game engines or editors**: no Unity, Unreal, Godot, Unigine, Stride, O3DE, Cocos, Defold, Flax, Ogre, Irrlicht, Raylib-based frameworks, or any "engine-class" middleware. **NO commercial physics/audio engines** (PhysX, Havok, FMOD, Wwise). Remove the stale **FMOD** references from the Windows docs — it is proprietary and conflicts with GPL-2.0.
* **NO ripped, scraped, or unlicensed assets** (3D models, textures, sounds, fonts, game-extracted data). **NO assets whose license is unknown.**
* **NO network/telemetry** in the game.

### 2.2 Allowed libraries (C++ libraries only; all must be GPL-2.0-compatible: MIT / BSD / zlib / Public-Domain / LGPL-dynamic. Avoid Apache-2.0-only because the project is GPL-**2**.0)
| Domain | Allowed |
|---|---|
| Window/input/haptics | **SDL2** (keep) |
| GL loading | **glad** (generated for 3.3 core + needed 4.x extensions) — replaces GLEW |
| Math | **glm** (or keep `vmath.h` behind a thin adapter, but new code uses glm types) |
| Images | **stb_image / stb_image_write**, optional **libktx** (KTX2 + BC7/BC5/BC4 + Basis) |
| Meshes | **cgltf** (glTF 2.0 loader), **meshoptimizer** (vertex cache/overdraw/fetch/LOD simplification), **mikktspace** (reference tangent space) |
| Audio | **OpenAL-soft** (replace deprecated **ALUT**), **stb_vorbis** or **dr_libs** for Ogg/WAV/FLAC decode |
| Files | **PhysFS** (keep), zip packs via PhysFS |
| Data | **tinyxml2** (legacy files only), **nlohmann/json** or **simdjson** (new schemas) |
| Text/UI | **stb_truetype** or **FreeType** (SDF font atlas), **Dear ImGui** (dev/debug tools build only) |
| Video | **pl_mpeg** (replace the `std::system("ffplay …")` hack) |
| Testing | **doctest** or **Catch2** |
| Logging/Profiling | **spdlog** (optional), built-in CPU/GPU profiler, optional **Tracy** (dev builds) |
| Dependency mgmt | `vcpkg` manifest **or** `FetchContent` with **pinned hashes** (no floating tags) |

### 2.3 Platforms
Linux (Mesa/NVIDIA/AMD) and Windows 10/11 (MSVC 2022 **and** MinGW-w64) are first-class. macOS is out of scope. **Vulkan is out of scope for 0.7**, but the renderer must be written against a thin **RHI** (§4.2) so a Vulkan backend can be added later.

### 2.4 Legal & asset policy (non-negotiable)
Real cars, liveries, sponsors and team names are **third-party trademarks and likenesses**.
* Implement **content packs with an explicit `license` block** (§4.4). The repo **must build and ship a legally clean default build**: the existing fictional cars (`cordo_*`, `evo_*`, `fox_*`) are retained as the **`core-fictional` pack**. Real-car packs are separate packs, each carrying: `source_url`, `license` (SPDX id), `author`, `attribution`, `trademark_notice`, `verified` flags.
* Only integrate real-car models that are **CC0 / CC-BY / own-made / explicitly licensed**; auto-generate `docs/CREDITS.md` and `DATA_AUTHORS.txt` from pack manifests. The build has a flag `-DTRACKRS_BUNDLE_LICENSED_MARQUES=ON/OFF`.
* A CI job **fails** if any asset file lacks a manifest entry with a license.

---

## §3 · GROUND TRUTH — WHAT THE REPOSITORY IS (verified from the repo snapshot)

**Identity:** *TrackRS* v0.6.7 — rebranded fork of *Trigger Rally*; GPL-2.0; author header `Saad AIT YAHIA`.
**Layers** (`src/`): `PEngine` (app, render, terrain, texture, model, vbuffer, fxman, audio, config, rigidity, physfs_rw, util, vmath) → `PSim` (sim, rigidbody, vehicle, engine, collision, damage) → `Trigger` (main, game, menu, control, option, ghost, render/HUD).
**Stack:** SDL2, OpenGL **compat 2.0** + GLEW, OpenAL + ALUT, PhysFS, TinyXML-2, C++11, 3 hand-written GNUmakefiles (`GNUmakefile`, `.MSYS`, `.MSYS64`).
**Data:** XML `.level` / `.event` / `.vehicle`; heightmap + colormap + foliagemap + roadmap + terrainmap PNGs per stage; Wavefront `.obj/.mtl` cars; `rigidity.xml`; 3 co-driver voice sets.
**Content volume to preserve:** 16 events, 132 maps, 97 event stages, 9 fictional cars, 25 plugin zips, 11 more "real-car" `.vehicle` stubs (Audi Quattro, BMW M3 E30, Ford Escort Cosworth, Lancia Delta, Lancia Stratos, Peugeot 205 T16, Peugeot 206, Porsche 911, Renault A110, Subaru Impreza, Toyota Celica).

### §3.1 DEFECT REGISTER — every item below MUST be resolved (ID is referenced by tests/reports)

| ID | Evidence in the code | Required fix |
|---|---|---|
| **R-01** | `PApp` requests an SDL GL **compatibility 2.0** context. | Request **core 3.3** (fallback chain 4.6→4.5→4.3→3.3), `GL_KHR_debug` callback in debug builds, abort with a human-readable error if < 3.3. |
| **R-02** | **0** shader programs. Fixed-function everywhere: `GL_LIGHTING`, `glLight*`, `glMaterial*`, `glFog*`, `glTexEnv`, `GL_ALPHA_TEST`/`glAlphaFunc`, `PEffect` "techniques/passes" map to fixed state. | Delete the fixed-function path. Replace `PEffect` with a **ShaderProgram + Material** system (§5-P2). |
| **R-03** | **~56 `glBegin` blocks / ~235 `glVertex*` calls.** `PSSRender::drawModel` submits **every triangle of every car every frame** through `glBegin(GL_TRIANGLES)` with three std::vector lookups per vertex; particles are `GL_QUADS` immediate mode; text draws **one `glBegin` per glyph** with push/pop matrix; the menu's scanlines/particles/glow are immediate mode. | Zero `glBegin`/`glVertex`/`glMatrix*`/`glPushMatrix` in the tree. All geometry via VAO/VBO/IBO. Particles → **instanced quads from a dynamic ring-buffer VBO**. Text/menu/HUD → **batched 2D renderer** (`UIBatch`, one dynamic VBO, one shader, SDF font). |
| **R-04** | `PMesh` keeps **separate index streams** (`vt/tc/nr`) and has **no tangents**. OBJ loader accepts only `f v/vt/vn` triples (9-value `sscanf`), triangles only, **single material per file** (documented FIXME), 64 KB line buffer, and `strtok2()` uses a **function-static pointer** (not re-entrant). ASE path uses `strcpy` ×10. | New **MeshImporter**: glTF 2.0 (primary), OBJ+MTL (legacy, with `Pr/Pm/map_Bump` extensions), de-index/weld to a single interleaved stream, **MikkTSpace tangents**, multi-material submeshes, quads/n-gons triangulated, robust parsing with error context, **meshoptimizer** pass, output **VertexPacked (32 B)** (Appendix B). Remove `strtok2`/`strcpy`. |
| **R-05** | Terrain tiles: **position-only** vertex VBO + shared strip IBO drawn with client-array state and `glDrawRangeElements`; **no normals, no LOD** (README claims LOD; the code has none); lighting baked in colormap; tile cache is `std::list` with **linear search** (`getTile`, `getTileAtPos`); foliage = camera-facing sprite quads. | **Geometry-clipmap or CDLOD terrain** with GPU-side normals from height texture, skirts/morphing to avoid cracks, frustum culling, **O(1) tile hash-map + LRU**, **PBR splat-blending of 9 surfaces** (tarmac, gravel, dirt, grass, sand, snow, ice, mud, water) driven by `terrainmap` with triplanar detail normals, **instanced 3D vegetation** with LODs + billboard impostors + wind + alpha-to-coverage. |
| **R-06** | Shadows = one blob texture (`tex_shadow`); sky = scrolling cloud texture; fog = `GL_EXP`. | **Cascaded shadow maps** (3–4 cascades, stabilised), procedural **atmospheric sky + time-of-day**, analytic **height-fog with sun in-scattering**, environment **probe → irradiance SH + prefiltered cubemap** for IBL. |
| **R-07** | No `SDL_GL_SetSwapInterval` anywhere; `glFlush()` before swap; main loop never sleeps (burns a core); stereo renders the whole scene twice. | Vsync modes (off/on/adaptive), optional frame cap with **precise sleep + spin**, proper frame pacing, stereo implemented with a **single culling pass / instanced multiview where available**. |
| **R-08** | Main loop uses millisecond `SDL_GetTicks`, clamps delta to 100 ms; `PSim::tick` uses a **function-`static float t` accumulator** (shared across *all* sims → breaks replays, tests, multiple sims); render draws the latest physics state with **no interpolation** → micro-stutter on non-250 Hz displays. | `SDL_GetPerformanceCounter`; accumulator becomes a **member**; fixed 250 Hz step (configurable); **render-time interpolation/extrapolation** of rigid-body transforms (`alpha = t / dt`); spiral-of-death guard; physics may run on its own thread behind a double-buffered state snapshot. |
| **R-09** | All makefiles default to **`-Ofast -march=native -mtune=native`**, including packaged `.deb` builds. | Release = `-O3 -ffp-contract=off` (**no fast-math** — keeps physics, ghosts and replays deterministic); `-march=native` only via opt-in `-DTRACKRS_NATIVE=ON`; ship `x86-64-v2` baseline (+ optional v3 build); LTO on; dispatch AVX2 hot paths at runtime only if profiled worthwhile. |
| **R-10** | The 11 real-car `.vehicle` files **all share the same 5-point engine power curve** (`rpm 2000…12500`, peak at 8500) and the gearbox block is identical in the files inspected; they differ only in `mass`, `powerscale` and wheel offsets. Their `.obj`/textures are **absent from the snapshot** (verify on disk; missing meshes make `PSSModel::loadModel` return `nullptr`). `.mtl` uses `Ns 0`, `Ks 0.8` for every car (identical plastic look). `scale`/`wheelscale` are hand-tuned magic numbers (0.0102–0.0105 / 0.033). | **Car Definition v2** (§4.3): real torque curves in N·m, real gear ratios, final drive, drivetrain layout & differentials, tyre/suspension/aero data, PBR material set, livery slots, LODs, wheel/brake/cockpit anchors, units in SI with an explicit import scale. Add the **physics calibration tests** (§6.3). |
| **R-11** | Demo-mode lock is **hard-coded in `menu.cpp`** (only the map named "Ahead" is allowed; many `(locked)` checks scattered through menu code). | Replace with an **`EditionPolicy`** (data-driven: `full`, `demo`, `dev`) queried through one API; unlock rules come from pack manifests/championship rules. |
| **R-12** | `main()` builds a shell command by **string concatenation** and calls `std::system("ffplay -fs …")` before the window exists. | Remove `system()`. Play the intro with **pl_mpeg** (or an image-sequence fallback) inside the GL window; skippable; zero external processes. |
| **R-13** | Dozens of raw `new`/`delete`; owning raw pointers (`sslist`, `PSim` bodies/vehicles, resource lists); `PTerrain` carries a `// TODO: make this RAII conformant`; `sprintf` for screenshots; `throw ()` (**illegal in C++17**); **`rand()` ×24** seeded from the clock → foliage differs between runs, ghosts/replays/screenshots are not reproducible. | `std::unique_ptr`/value semantics/arenas, `std::string_view`, `std::filesystem`, `noexcept`; **seeded `pcg32` RNG per stage** (`stage_seed` stored in level/pack) used by foliage, spectators, props, particles-that-affect-gameplay; ban `strcpy/sprintf/strcat` with a clang-tidy rule. |
| **R-14** | Coordinate wrap logic is **copy-pasted 3×** (`RoadMap::isOnRoad`, `PTerrain::getCmapColor`, `PTerrain::getRoadSurface`) as `do { px -= ms; } while (px > ms);` — **off-by-one** (`>` should be `>=`), **O(n)** for far/negative input, and integer truncation (`static_cast<int>`) *before* wrapping negatives. | One `wrapCoord()` using `floor`/power-of-two masks, unit-tested, shared by height/colour/road/surface lookups; move big inline bodies out of `render.h`. |
| **R-15** | **3 GNUmakefiles** with duplicated `DISTVER`; no CMake, CI or tests; **26 committed `.d` dependency files**; stray `repomix-output.xml` files inside `data/vehicles/cordo_kitcar/` and `docs/`; duplicated `doc/` vs `docs/`; docs reference g++ 4.9 and FMOD; the `.deb` depends on `libphysfs1`, `libalut0`. | **CMake + presets** (`linux-release`, `linux-debug-asan`, `win-msvc-release`, `win-mingw-release`), single-source version (`project(VERSION)` → generated `version.h`, appdata, NSIS, deb, README badge), delete committed build artefacts and junk, `.gitignore`, **GitHub Actions** matrix (build + test + sanitizers + packaging), CPack (`.deb`, `.AppImage`, NSIS `.exe`, `.zip`). |
| **R-16** | `Trigger/menu.cpp` (~2 000 lines), `Trigger/render.cpp` (~1 900 lines) and `MainApp` act as god-objects; the state machine (`AS_LOAD_1…`, `AS_IN_GAME`, …) lives in `main.cpp`. | Split into `Application`, `GameStateMachine`, `Renderer`, `UI`, `Audio`, `World`, `Sim`; no module may include a higher layer; enforce with a CMake **layering check**. |
| **R-17** | Co-driver, HUD and menu use bespoke immediate-mode font/quads; `drawText` loops per glyph. | New UI layer (`UIBatch` + SDF font atlas + layout), **resolution-independent**, 4K/ultrawide/HiDPI-aware, controller-navigable. |

---

## §4 · TARGET ARCHITECTURE

### 4.1 Repository layout (target)
```
CMakeLists.txt  CMakePresets.json  vcpkg.json
cmake/  (toolchains, warnings, sanitizers, packaging, version header)
engine/                 # NO game rules here
  core/      (log, assert, profiler, jobs, arena, pcg32, math glue, vfs)
  platform/  (SDL2 window/input/haptics, high-res timer, frame pacer)
  rhi/       (GL backend: Device, Buffer, Texture, Sampler, Framebuffer, ShaderProgram, UBO, GpuTimer, DebugGroup)
  render/    (RenderGraph-lite, Passes, Materials, Meshes, Terrain, Sky, Shadows, PostFX, Particles, UIBatch, DebugDraw)
  audio/     (OpenAL-soft, mixer, DSP, 3D, occlusion/reverb zones)
  assets/    (importers: glTF/OBJ/KTX2/PNG/WAV/OGG, async streaming, hot-reload)
sim/                    # deterministic, no GL, no SDL
  (rigidbody, tyre/Pacejka, suspension, drivetrain, aero, damage, collision, terrain query, surface table)
game/
  (rules, stage runner, championship, career, co-driver, ghost/rivals, replay, AI-time model, edition policy)
ui/                     # menus, HUD, pace-note icons, settings, photo mode
content/                # data only — see §4.4
  packs/core-fictional/  packs/classic-legacy-stages/  packs/<your-future-packs>/
tools/                  # command-line content tools (§5-P9)
tests/                  # unit, golden-image, golden-run, physics calibration, content validation
docs/                   # ARCHITECTURE.md, ADDING_CONTENT.md, RELEASING.md, DECISIONS.md, reports/
```

### 4.2 Renderer design rules
* **RHI-thin**: all GL calls live in `engine/rhi`. Game code never includes `glad.h`.
* **State caching** (bound program/VAO/texture/blend/depth) → no redundant GL calls; **sort keys** `pass | shader | material | depth-bucket`.
* **UBOs (std140)**: `FrameUBO` (time, sun, fog, exposure, probe params), `ViewUBO` (view/proj/viewProj/invViewProj/camPos/jitter, cascades), `MaterialUBO`, `InstanceSSBO/Texture-buffer` (model matrices, tints, livery id). **Instanced draws** for everything repeated (spectators, trees, signs, tyre stacks, hay bales, cones).
* **Streaming uploads**: persistent-mapped ring buffers (`GL_ARB_buffer_storage`) with fences when available; `glBufferSubData` orphaning fallback on 3.3.
* **Textures**: KTX2 with BC7 (albedo), BC5 (normals), BC4/BC7 (ORM), BC6H (HDR env); mipmaps mandatory; anisotropic ≥ 8× default; **sRGB formats for albedo** (hardware linearisation — no `pow` in shader), linear for data maps; async upload via PBOs.
* **Colour pipeline**: everything in **linear HDR** (`RGBA16F`), MSAA 4× or TAA, tone-map once in the post pass (ACES-fit or AgX selectable), **gamma 2.2/sRGB encode last**. The reference material shader (Appendix A) has a `uToneMap` switch for the "no-post" low-end path.
* **Debuggability**: `GL_KHR_debug` groups per pass, RenderDoc-friendly labels, `F-keys` debug views (albedo / normal / roughness / metallic / AO / shadow cascades / overdraw / LOD / UV checker / tangent frame).

### 4.3 Car Definition v2 (`car.json`) — data-driven, SI units, real data
```jsonc
{
  "schema": "trackrs.car/2",
  "id": "lancia_delta_s4_1986",
  "display_name": "Lancia Delta S4",
  "marque": "Lancia", "year": 1986, "class": "GroupB", "era": "1983-1986",
  "license": { "model": "CC-BY-4.0", "author": "…", "source_url": "…", "trademark_notice": "…" },
  "verified": { "mass": false, "engine": false, "gearbox": false, "tyres": false },   // flip to true only with a cited source
  "body": {
    "mesh": "meshes/delta_s4.gltf", "lods": [ {"tris": 60000}, {"tris": 18000}, {"tris": 5000}, {"impostor": true} ],
    "import_scale": 1.0, "unit": "m",
    "materials": "materials/delta_s4.mat.json",           // albedo / normal / metallicRoughness(+AO) / emissive, clearcoat, dirt mask
    "livery_slots": [ {"id":"base_paint","mask":"tex/mask_paint.ktx2"}, {"id":"number_plate","decal_rect":[0.1,0.4,0.2,0.2]},
                      {"id":"sponsors","atlas":"tex/sponsors.ktx2"} ],
    "anchors": { "wheel_FL":[…], "wheel_FR":[…], "wheel_RL":[…], "wheel_RR":[…], "driver_head":[…], "codriver_head":[…],
                 "headlights":[…], "roof_vent":[…], "exhaust":[…], "camera_hood":[…], "camera_bumper":[…], "camera_cockpit":[…] }
  },
  "mass": { "kerb_kg": 890, "cog_m": [0.0, 0.05, 0.46], "inertia_kgm2": [ … ], "front_weight_dist": 0.41 },
  "engine": {
    "layout": "I4 supercharged+turbo, mid-longitudinal", "idle_rpm": 1000, "redline_rpm": 8000, "inertia_kgm2": 0.18,
    "torque_curve_nm": [[1000,120],[2500,260],[4000,470],[5500,560],[7000,520],[8000,430]],   // rpm → N·m  (approximate seed; verify)
    "turbo": { "lag_s": 0.35, "boost_response": 3.0, "antilag": { "enabled": true, "pop_chance": 0.5 } },
    "fuel": { "capacity_l": 100, "consumption_model": "bsfc" }
  },
  "drivetrain": {
    "layout": "AWD", "gearbox": { "type": "sequential", "ratios": [ … ], "reverse": …, "final_drive": …, "shift_time_s": 0.18 },
    "diffs": { "front": {"type":"LSD","preload_nm":40,"ramp_deg":[45,60]}, "centre": {"type":"viscous","split_front":0.30}, "rear": {"type":"LSD", "preload_nm":60,"ramp_deg":[45,60]} },
    "clutch": { "max_torque_nm": 900, "engage_time_s": 0.08 }, "brake_bias_front": 0.62
  },
  "tyres": { "compound_defaults": {"gravel":"gravel_soft","tarmac":"tarmac_medium","snow":"studded"}, "radius_m": 0.33, "width_m": 0.235,
             "pacejka": "tyres/pacejka_gravel_soft.json" },
  "suspension": { "front": { "type":"strut", "travel_m":0.28, "spring_nm":38000, "damper_bump":2800, "damper_rebound":4200, "arb_nm":14000 }, "rear": { … }, "ride_height_m":0.22 },
  "aero": { "cd":0.40, "frontal_area_m2":1.9, "cl_front":0.05, "cl_rear":0.12 },
  "damage": { "zones": [ … ], "detachables": ["front_bumper","mirror_L","mirror_R","rear_wing"], "deform_texture": 256 },
  "audio": { "engine_bank": "audio/delta_s4_engine.bank.json", "turbo": "…", "gear_whine": "…", "exhaust_pops": "…", "intake": "…" },
  "ui": { "icon": "ui/delta_s4.ktx2", "bio": "…", "stats_hint": { "power_kw": …, "mass_kg": … } }
}
```
A **legacy adapter** converts old `.vehicle` XML → in-memory Car v2 (so the 9 fictional cars keep working) and a tool `trackrs-content migrate-vehicle` writes real `car.json`.

### 4.4 Content-pack system (what makes releases cheap)
```
content/packs/<pack-id>/
  pack.json        # {"schema":"trackrs.pack/1","id":"wrc-2017-pack","version":"1.2.0","requires_engine":">=0.7 <1.0",
                   #  "depends":[{"id":"core-fictional","version":"^1.0"}],"authors":[…],"license":{…},"changelog":"CHANGELOG.md"}
  cars/<id>/car.json + meshes/ + tex/ + audio/
  stages/<id>/stage.json (+ heightmap/colour/road/terrain-type/foliage/dressing/pacenotes)
  championships/<id>.championship.json
  dressing/<style>.dressing.json     # stage-dressing rule sets (§5-P5)
  materials/ surfaces/ tyres/ sounds/ ui/
```
* **Discovery**: scan `<install>/packs/*`, `<userdir>/packs/*`, and `*.trpack` (zip) via PhysFS. Packs are **hot-listed**, validated at startup (schema + hashes + license present), and a broken pack is **disabled with a clear UI message — never a crash**.
* **ID namespacing** (`pack:id`), semantic versioning, dependency resolution, **save-game & profile migrations** (`profile.version` → migration chain).
* **Stable Content API**: `content_api = N` integer in the engine; additive changes only inside a major; breaking changes bump the major and ship a migration tool.
* **Championship file** (`*.championship.json`): calendar of rounds → `{stage_id, surface, weather preset, time-of-day, length, service-after?}`, `classes`, `entries` (AI driver table with pace model + consistency + mistake rate), `points` (FIA-style 25-18-15-12-10-8-6-4-2-1 + Power-Stage bonus configurable), `regulations` (tyre allocation, repair-time budget, parc-fermé setup lock, penalties), `unlocks`, `liveries`, `intro_cinematic`.
* **Stage file** (`stage.json`): supports the legacy terrain image set **and** new: road **spline** (Catmull-Rom/Bezier) with width/camber/surface profile, spectator zones, hazard markers, time-controls, weather timeline, per-stage `stage_seed`, ambience sound zones, pace-note track.
* **Edition policy** (`demo`/`full`/`dev`) is just a list of allowed pack ids/stage ids.

### 4.5 Legacy adapter (never remove)
`LegacyLevelLoader`, `LegacyEventLoader`, `LegacyVehicleLoader` read today's XML and produce the same in-memory structs as the new JSON loaders. The 132 maps are exposed as the auto-generated pack `classic-legacy-stages`. Terrain/colour/road images remain valid inputs.

---

## §5 · PHASES (each: tasks → exit criteria)

### PHASE 0 — SAFETY NET (do this first; no behaviour change)
1. Create branch `reality/0.7`, tag `v0.6.7-baseline`.
2. **CMake + presets** building the *current* code unchanged (C++17 where trivial: remove `throw ()`), plus ASan/UBSan/TSan presets, `-Wall -Wextra -Wpedantic -Wconversion` (warnings baseline recorded, then ratcheted down).
3. **CI** (GitHub Actions): Linux GCC/Clang, Windows MSVC + MinGW; artefacts uploaded.
4. **Golden-run harness**: `trackrs --headless --stage <id> --replay <file> --dump-state <hash>` runs N seconds of deterministic physics with scripted input and prints a state hash. Record hashes for ≥ 12 representative stages (all surfaces, ≥ 3 vehicles). Baseline screenshots at fixed cameras for 8 stages via `Xvfb + llvmpipe`.
5. **Benchmark mode** `trackrs --benchmark --json out.json`: deterministic fly-through of 3 reference stages; reports avg/1%/0.1% frame time, CPU/GPU ms per pass, draw calls, triangles, VRAM, RAM, load time.
6. Remove committed `.d` files, stray `repomix-output.xml`, duplicate docs; fix `.gitignore`.
**Exit:** all builds green; baseline hashes, screenshots and benchmark JSON committed under `tests/baselines/`.

### PHASE 1 — DEFECT PURGE (R-08…R-15)
Fix R-07 (pacing), R-08 (fixed step + interpolation + hi-res timer, member accumulator), R-09 (flags), R-12 (no `system()`), R-13 (ownership, RNG, banned C string functions), R-14 (`wrapCoord`), R-15 (build hygiene), R-11 (EditionPolicy), R-16 (module split, no behaviour change). Write unit tests for every fixed function (wrap, RNG determinism, accumulator, version parsing).
**Exit:** golden-run hashes unchanged **or** changed only where the fix is the cause (document each in the report); sanitizers clean on a 10-minute scripted soak; `clang-tidy` clean for new code.

### PHASE 2 — MODERN RENDERER CORE (R-01…R-04, R-17) + the four mandatory HD systems
Implement, in this order:
1. **Context & RHI**: GL 3.3 core, `glad`, state cache, `GpuTimer`, `KHR_debug`, shader programs with `#include`/`#define` permutations, **hot reload** (file-watch in dev builds), UBO plumbing.
2. **Physically Based Rendering shaders (Cook-Torrance BRDF).** Start from **Appendix A** (`pbr.vert`/`pbr.frag`, already validated: GLSL 330 core, GGX NDF, Smith–Schlick-GGX geometry, Schlick Fresnel, Lambert diffuse with energy conservation, normal mapping, clear-coat, wetness, specular AA, ACES + gamma 2.2). Extend with: CSM shadow sampling, point/spot lights (headlights) via small clustered/tiled list, fog, vertex-colour AO, alpha-to-coverage, instancing, two-sided foliage. Everything must be **energy-conserving** and obey the *furnace test* (white furnace ≤ 1.0 reflectance).
3. **Extended vertex layouts.** Implement `Vertex` (56 B) for tools and **`VertexPacked` (32 B)** for shipping exactly as **Appendix B**: Position(3f), Normal (2_10_10_10), Tangent (2_10_10_10 + handedness), UV(2f), colour (RGBA8). MikkTSpace tangents at import; **bitangent reconstructed in the vertex shader** (`cross(N,T)*w`); explicit-bitangent variant kept for tooling. Document strides/offsets, add `static_assert`s, and unit tests for packing/tangent generation (Appendix C).
4. **Texture channel budgeting (PBR texture sets).** Every material = **Albedo (sRGB, BC7)**, **Normal (BC5, 2-channel, Z reconstructed)**, **Metallic-Roughness (+AO in R) packed** (glTF layout), optional **Emissive**, optional **Clear-coat/Dirt masks**. Texel-density rules: cars 1 texel ≈ 1 mm at 4K atlas (2 × 4096² per car at *Ultra*, 1 × 2048² at *Low*), props 512–1024². Provide `trackrs-content bake-material` to pack channels and compress to KTX2. Provide a **material validator** (rejects non-power-of-two atlases, missing mips, wrong colour space flags).
5. **Image-Based Lighting.** Implement an **IBL baker** (run at load or offline, cached in KTX2): equirect/cubemap HDR → **diffuse irradiance (SH9 or 32² cubemap)**, **GGX prefiltered specular cubemap with full mip chain** (importance sampling, 128–256 samples), **BRDF LUT 256² RG16F**. Runtime: split-sum approximation, **parallax-corrected local probe** for cars (128² cube re-rendered at ≤ 4 Hz from the live scene + sky), horizon/specular occlusion. Result: sharp crisp **chrome and clear-coat reflections** on curved panels, believable wet asphalt.
6. **Replace every immediate-mode site** (R-03): meshes, particles (instanced ring-buffer quads), text, menu effects, HUD, minimap, co-driver icons → `UIBatch` + `ParticleRenderer`. Remove `PEffect/PSSEffect/fx_*`.
7. **Material & mesh managers**: ref-counted, async-loaded (jobs), placeholder-on-load (checkerboard/magenta never reaches release), LRU eviction with budgets.
**Exit:** `grep -R "glBegin\|glVertex\|glMatrixMode\|glLight\|GL_LIGHTING\|glFog\|glTexEnv\|glAlphaFunc"` returns nothing; benchmark ≥ baseline FPS on the reference low machine; golden images updated and **reviewed side-by-side in the report**; white-furnace and energy-conservation tests pass.

### PHASE 3 — WORLD RENDERING (R-05, R-06)
* **Terrain**: CDLOD/clipmap, GPU normals, height/colour/surface-type textures, **PBR splat + triplanar detail**, wetness mask from weather, macro-variation noise to kill tiling, **tessellation-free** crack-free LOD morphing; collision/physics keeps using the CPU heightmap (single source of truth, shared `wrapCoord`).
* **Road ribbon**: optional mesh generated from road spline/roadmap with **tarmac / gravel / dirt PBR materials**, centre-line/edge markings on tarmac, camber, **gravel berms**, cracks and patches via decal textures, **persistent tyre marks and ruts** (rolling decal buffer or deformation texture), dust-and-mud deposition.
* **Sky & light**: procedural sky (Hosek–Wilkie/Preetham) + **time-of-day** (dawn, noon, golden hour, dusk, night with stars/moon), cloud layer (2D noise advected, soft self-shadow), **sun shafts**, lens glare; sun colour/intensity feed IBL, fog and shadows consistently. **Night stages**: dual headlights (shadow-casting spot optional), roof light-bar, rear lights/brake lights emissive + bloom, spectators with torches/flares.
* **Shadows**: 3–4 **CSM** (2048², texel-snapped, PCF 3×3 → optional PCSS-lite), **cars cast contact shadows + AO under chassis**; foliage uses cheap alpha-tested shadow pass; far-distance shadow fade.
* **Vegetation**: instanced 3D trees/bushes/grass with 3 LODs + impostors, wind (vertex-shader sway, per-species), alpha-to-coverage, dense roadside brush; keep existing foliage maps as spawn masks (seeded RNG, R-13).
* **Water/puddles**: planar or screen-space reflections, Fresnel, depth-fade, foam; **puddles** spawn from rain; cars throw spray.
* **Weather**: rain (GPU particles + streaks + wet-surface response + windscreen droplets/wipers), snow (accumulation tint + particle), fog banks, dynamic transitions, **lightning optional**.
* **Post-FX stack** (all toggleable, quality-scaled): HDR → **bloom (dual-filter)**, **auto-exposure (histogram, eye adaptation)**, **ACES/AgX tone-map**, colour grading LUT, **SSAO/GTAO**, **TAA or SMAA**, **per-object motion blur** (velocity buffer), **depth-of-field for replay/photo**, vignette/chromatic-aberration/film-grain at low intensity, lens dirt on bloom, **camera shake** from suspension/engine vibration.
**Exit:** side-by-side reports at 6 times of day × 3 weather states; stable ≥ 90 FPS on reference mid machine at 1080p *High* (budgets §7).

### PHASE 4 — REAL RALLY CARS (R-10)
1. Implement **Car Definition v2** loader + legacy adapter, **`CarRenderer`** (LODs, instancing for AI/ghost cars, wheels as separate rotating nodes with brake-disc glow at night, suspension-linked wheel transforms, steering-wheel/driver-arm animation, exhaust flame/anti-lag pops, rear-lights/brake-lights/headlights emissive channels).
2. **Livery system**: paint-mask colourisation (base paint colour + secondary stripe colour + number/decal placement + sponsor atlas), per-team palette presets, driver names/country flags on doors; **dirt/mud/dust accumulation shader** (inputs: speed, surface type, wetness, time driven; world-space noise + wheel-arch/lower-body bias), **water-beading** when raining, **clear-coat** car paint, **metal flake** option, carbon/matte/rubber materials, **tyre sidewall text** via normal+albedo, **glass** pass (tint, reflections, cracks via damage mask), **interior** with working gauges (tachometer, speed, gear indicator), roll-cage, co-driver/driver models.
3. **Damage visuals**: vertex-shader **dent morph** from an impact deformation texture or lattice; scratch/mud decals; detachable parts (bumpers, mirrors, spoiler, doors optional) as separate physics-lite debris; smoke/steam particles; windscreen/rear-glass cracks; wheel wobble; headlight break. Damage already affects performance in `PSim` — keep it and map it to visuals.
4. **Roster (seed list — Codex: research real homologation specs where web access exists; otherwise leave `verified:false`)**, grouped by *class pack* so each can ship in a different release:
   * **Classics (1970s)**: Lancia Stratos HF, Alpine-Renault A110 1800, Ford Escort Mk2 RS1800, Porsche 911 (SC/RS), Fiat 131 Abarth, Opel Ascona 400, Datsun/Nissan 240Z.
   * **Group B (1983–86)**: Audi Sport quattro S1 E2, Lancia Delta S4, Peugeot 205 T16 E2, Ford RS200, MG Metro 6R4, Lancia 037, Renault 5 Turbo.
   * **Group A (1987–96)**: Lancia Delta HF Integrale 16V, Toyota Celica GT-Four ST185, Subaru Impreza 555, Mitsubishi Lancer Evo III, Ford Escort RS Cosworth, BMW M3 E30.
   * **World Rally Cars 1997–2010**: Subaru Impreza WRC, Ford Focus RS WRC, Peugeot 206 WRC, Citroën Xsara WRC, Mitsubishi Lancer WRC, Škoda Octavia WRC, Toyota Corolla WRC.
   * **WRC 2011–2021**: Citroën DS3 WRC, Ford Fiesta RS WRC, VW Polo R WRC, Hyundai i20 WRC, Toyota Yaris WRC, Citroën C3 WRC.
   * **Rally1 hybrid (2022→)**: Toyota GR Yaris Rally1, Hyundai i20 N Rally1, Ford Puma Rally1 (with **hybrid boost** deploy logic).
   * **Rally2 / Rally3 (customer)**: Škoda Fabia RS Rally2, Hyundai i20 N Rally2, Citroën C3 Rally2, Ford Fiesta Rally3.
   The 11 stub `.vehicle` files in the repo become the first real `car.json` migrations; each receives **its own torque curve, gear ratios, final drive, diff set-up, tyre and aero data** (no more shared curve).
5. **Hard rule:** if a real mesh is missing or unlicensed, load the **fictional fallback body** and label the car "(placeholder model)" in the UI; never ship a mislabelled body.
**Exit:** every car loads, renders in all 4 LODs, passes the **calibration tests** (§6.3), has complete `license` + `verified` metadata; `docs/CARS.md` auto-generated from manifests.

### PHASE 5 — STAGE DRESSING: SIGNS, SPECTATORS, CAMERA CREWS, MARSHALS (the "rally atmosphere" pass)
Build a **`StageDresser`**: a deterministic (seeded by `stage_seed`) generator + data format (`*.dressing.json`) that places props along the road spline using rules, with a **manual override layer** so stage designers can paint/move/delete props. It runs offline (cache baked to `stage.dress.bin`) or at load; everything is **GPU-instanced**, LOD'd, frustum + distance culled, and budgeted.

**5.1 Road-side signage & safety furniture (all PBR, retro-reflective at night — add a `retroreflective` material flag that boosts specular toward the light direction)**
* Distance boards (**km-to-go / km-from-start**), **chevron boards** on outside of bends, **warning triangles**, speed-limit and junction boards, **arrows/“keep right/left”**, **start line gantry with countdown lights + marshals**, **inflatable start/finish arch**, **Flying-Finish banner**, **STOP (stop-line) board**, **time-control (clock) boards**, stage-name board, **sponsor banners** and barrier-wrap, **plastic barriers**, **safety tape on poles** (cloth/line simulation: cheap verlet strip), **hay/straw bales**, **tyre stacks** and **chicanes**, **cones**, **“No spectators / Danger” red-zone signage**, **yellow/blue/red marshal flags**, **radio post** and **medical post** with ambulance.
* Placement rules use the **road spline**, **curvature**, **slope**, **jump detection** (from heightmap), junctions and **pace-note events** (hairpins, jumps, finish, crests) to decide where signs/bales/spectator clusters go. All counts/densities/styles are **data**.

**5.2 Spectators (never a flat sprite wall)**
* **Instanced low-poly human models**, 6 body types × 8 clothing palettes × hats/jackets/umbrellas/flags, **3 LODs + impostor billboards** (LOD0 ≈ 800 tris, LOD1 ≈ 200, LOD2 impostor); per-instance tint/variant via instance data + texture array.
* **Vertex-animation textures (VAT)** for idle sway, clapping, waving flags, jumping, **stepping back** when a car comes within ~25 m, cheering after the car passes; phase-offset per instance. Cloth flags/banners animated in the vertex shader.
* Cluster generation: Poisson-disc inside **spectator zones**, fenced/marked **safe zones** vs **red "no-go" zones** (empty or with tape), camping chairs, tents, vans, campfire smoke at night, **flares/phone-torches at night stages** (point-light budget via light-atlas of tiny sprites).
* **Crowd audio**: spatialised crowd layers (murmur, cheers on pass, air-horns) driven by cluster size/distance; Doppler on shouts is off, but low-pass by distance.

**5.3 Camera operators, photographers, media**
* **TV camera operators** with tripod/shoulder cameras at key corners that **smoothly track the nearest car** (critically-damped yaw/pitch controller), **photographers** kneeling with long lenses (occasional **flash burst** at dusk/night), **drone** following the leader on selected stages (rotor sound + light), **TV helicopter** on a spline with Doppler and rotor-wash on nearby dust/trees, **media vehicles**, **TV compound** at start/finish.
* These same camera nodes feed the **broadcast replay director** (§5-P7): "trackside TV cam", "helicopter cam", "drone cam", "bumper cam", "roof cam".

**5.4 Marshals & event vehicles**
* **Marshals** in orange/red bibs with radios and flags; **yellow flag** raised when a car stops ahead (data-driven from sim events); **red flag** on stage stop; **safety car "000/00/0"** at stage start; **medical car**, **recovery truck** after crashes (optional), **organiser pickups** at junctions; **service-park** scene for championship mode (tents, mechanics, repair animation timeline).

**5.5 Quality & performance gates for the dressing layer**
* ≤ 300 draw calls for all dressing in view (instancing), ≤ 400 k dressing triangles visible at *High*, ≤ 64 MB GPU memory for the whole dressing set (texture arrays + atlases), dressing generation ≤ 1.5 s/stage (cached).
* **Placement determinism test**: same seed → identical `stage.dress.bin` hash.
* **Safety realism test**: no spectators on the road surface or within `road_half_width + min_safe_distance` unless inside a flagged zone; no props intersect the physics collision corridor *unless flagged as obstacles*; no floating props (snap to terrain with slope alignment).
**Exit:** 3 flagship stages (tarmac, gravel forest, night snow) fully dressed; before/after screenshots in report; budgets met.

### PHASE 6 — PHYSICS REALISM (keep `PSim` architecture, upgrade the models)
* **Tyres**: **Pacejka Magic Formula** (longitudinal, lateral, combined slip via friction ellipse), **load sensitivity**, per-surface μ table (tarmac dry/wet, gravel, dirt, sand, snow, ice, mud, grass), **surface sinkage/rolling resistance** (keep existing wheel sinking), tyre temperature/wear (optional), tyre compounds as data (`tyres/*.json`).
* **Suspension**: spring/damper with bump-stop and rebound, anti-roll bars, camber/toe, ride height, **weight transfer**; long travel for rally, **jump landing** behaviour.
* **Drivetrain**: engine torque curve from N·m data, **turbo lag/boost build**, **anti-lag**, flywheel inertia, clutch, **sequential gearbox** with real ratios/shift time/cut, **front/centre/rear differentials** (open/LSD ramp+preload/viscous/active centre), **launch control**, 4WD torque split, **hybrid boost** for Rally1.
* **Aero**: Cd·A drag, front/rear downforce, **yaw-dependent drag**, slipstream (optional).
* **Brakes**: bias, handbrake, fade (optional). **Damage model** couples to suspension geometry, steering alignment, engine power, radiator overheating.
* **Determinism**: fixed 250 Hz step (or 500 Hz option), no fast-math, no unordered iteration affecting results, **bit-identical results across runs on one machine** and tolerance-identical across compilers.
* **Setup menu**: gear-ratio pair, suspension stiffness/ride height, brake bias, diff ramps, tyre choice, aero level — saved per car/stage-type; AI/championship uses defaults.
* **Telemetry** overlay (dev/tuning): tyre loads, slip angles, torque split, engine RPM/torque; CSV export.
**Exit:** calibration tests pass (§6.3); driving feel reviewed in report with graphs (throttle-steer, handbrake turns, jump landings).

### PHASE 7 — AUDIO, CAMERAS, HUD, REPLAY, PHOTO MODE, CO-DRIVER
* **Audio**: OpenAL-soft; **layered engine model** (RPM-crossfaded loops per load state, intake/exhaust/turbo/blow-off/anti-lag pops, gear whine, driveline lash), gravel/dirt/tarmac tyre loops with slip-based pitch, **suspension bottom-out/impact**, wind, **interior vs exterior mix** (low-pass + convolution reverb IR), **occlusion/reverb zones** (forest, canyon, tunnel, open), Doppler, crowd/helicopter/drone beds. Remove ALUT.
* **Cameras**: chase, bumper, hood, **cockpit with head-bob & G-force lean**, roof, side, TV-trackside, helicopter, drone; FOV/speed effects; **camera collision**; HUD-less cinematic mode.
* **HUD** (WRC-style): speed, gear, RPM bar, stage map with sector splits, **live gap to rival ghosts/target time**, pace-note icon strip (existing co-driver sign sets kept), damage indicators, tyre/pit info in championships, **resolution-independent**, safe-area aware.
* **Co-driver**: keep voice sets; add **call-timing based on speed (lookahead distance scaled by velocity)**, caution/“don’t cut”/surface-change calls, **editable pace-note files** (`pacenotes.json`), and a **recce mode**.
* **Replay**: record input + sim snapshots (deterministic), scrub/pause/slow-mo, **broadcast director** auto-cuts between cameras (§5.3), **photo mode** (free camera, DoF, exposure, FOV, filters, hide HUD, save PNG/EXR, burst).
* **Input**: SDL2 gamepad API, wheel/pedals **force-feedback** (SDL haptics), dead-zone/curve editor, per-device profiles, full remapping UI.

### PHASE 8 — PERFORMANCE ENGINEERING (continuous, but a dedicated gate)
1. **Built-in profiler**: scoped CPU markers + **GL timer queries** per pass, in-game overlay (F3) + JSON export + flamegraph-friendly dump.
2. **Job system** (work-stealing; N = cores-1): async texture/mesh decode, KTX2 transcode, terrain tile generation, dressing generation, audio streaming, shadow-caster culling. **No GL calls off the main thread** (use shared-context upload thread *only* if profiling proves benefit; otherwise PBO + main-thread submit).
3. **Zero steady-state allocations per frame** (custom frame arena; debug allocator counter asserts 0 after warm-up).
4. **Draw-call & state budget**: sort + instance + (optionally) `glMultiDrawElementsIndirect` on 4.3+; texture arrays/atlases to cut binds; `GL_ARB_bindless_texture` only as optional fast path.
5. **Culling**: frustum + distance + **hierarchical Z occlusion (optional)** + portal-less terrain occluders; LODs by screen-space error; **impostors** for far trees/spectators.
6. **Memory layout**: SoA for particles/spectator instances, 32 B vertices, `uint16` indices when possible, texture compression everywhere (BC7/BC5/BC4/BC6H), **mip-streaming** with budget, KTX2 + zstd in packs, startup **cache of baked IBL/dressing/terrain tiles**.
7. **Quality presets** (Ultra/High/Medium/Low/Potato) change: resolution scale (50–150 %), shadow res & cascades, SSAO, MSAA/TAA, vegetation density, spectator count & LOD bias, particle density, anisotropy, reflection probe rate, texture mip bias. **Dynamic resolution scaling** targeting a frame-time goal (optional **TAA-upsampling**).
8. **Regression gate**: CI runs `--benchmark` on a software renderer for correctness and (self-hosted/optional) on a real GPU for timing; PR fails on > 5 % regression of any pass.
**Exit:** budgets in §7 met on the reference machines; report with before/after flame graphs and per-pass GPU timings.

### PHASE 9 — EVOLUTION & RELEASE ENGINEERING ("add a map/championship/car, ship a version")
1. **`tools/trackrs-content` CLI** (C++17, same libs): `validate`, `pack`, `unpack`, `new-car`, `new-stage`, `new-championship`, `migrate-vehicle`, `migrate-level`, `bake-material`, `bake-ibl`, `bake-terrain`, `ktx-convert`, `stage-from-heightmap`, `lint-licenses`, `sign` (SHA-256 manifest), `docs` (generate catalogues).
2. **Docs that make evolution trivial**: `docs/ADDING_CONTENT.md` — *"Add a new car in 15 minutes / new stage in 30 minutes / new championship in 10 minutes"* with copy-paste templates under `content/templates/`.
3. **Versioning & release**: single-source semver, **changelog fragments** (`changelog.d/*.md` → `CHANGELOG.md`), `scripts/release.sh` (or `trackrs-release` tool): bump → validate packs → run test matrix → build → package (`.deb`, AppImage, NSIS, zip) → checksums → GitHub Release notes (auto from fragments) → website/README badges refresh. **DLC-style packs** can be released independently of the engine (pack `requires_engine` range).
4. **Settings/profile/save migrations** with automated tests (old profiles from 0.6.x must import).
5. **Crash handling**: minidump/backtrace to user dir (no network), log rotation, "copy diagnostics" button.
6. **Accessibility & localisation**: string tables (`lang/en.json` …), UTF-8 everywhere, scalable UI, colour-blind-safe pace-note icon sets, subtitles for co-driver.
7. **First content wave to prove the pipeline** (must exist when phase ends): pack `core-fictional` (existing 9 cars), pack `classic-legacy-stages` (existing 132 maps/16 events), **one new championship** ("TrackRS Reality Cup", 6 rounds on dressed flagship stages), **one new real-car class pack** (e.g., Group A, only with licensed/own meshes or fictional-labelled placeholders), and **one brand-new stage** authored from a spline using the tool — all installed purely as packs.
**Exit:** a **fresh clone → one command → installers** works on CI; adding a new test car pack + stage + championship requires **zero C++ changes**; changelog/versions auto-generate.

---

## §6 · TESTING & VERIFICATION (definition of done)

1. **Unit tests** (`doctest`): math/wrap, packing, tangents, RNG, config/schema parsing, pack resolver (dependency/semver), migrations, Pacejka/diff functions, spline sampling, placement rules.
2. **Golden-run (physics)**: deterministic hash per (stage, car, input script). Changes must be explained in the phase report.
3. **Physics calibration tests** (per car, tolerance ±8 % unless source says otherwise): **0–100 km/h**, 0–200 m, top speed in 6th, max lateral g on dry tarmac, braking 100→0 distance, peak power/torque from the dyno sweep vs `car.json`, mass distribution, shift timing. Failing a calibration test blocks the car from the release roster.
4. **Golden-image tests** (headless `Xvfb + llvmpipe`): fixed cameras, fixed time-of-day, fixed seed; **SSIM ≥ 0.98** vs baseline for regression, and a **review PNG** is attached when intentionally updated.
5. **Shader tests**: compile every permutation on **glslang** in CI; run **white-furnace** (≤ 1.0), **reciprocity**, and **normal-map handedness** checks on a test sphere; no shader warnings.
6. **Content validation**: every pack passes `trackrs-content validate`: schema, hashes, license present, no missing files, texture colour-space flags, mip chains, max texture sizes, triangle budgets, unit scale sanity (car length 3.5–5.0 m), wheel radius vs anchors.
7. **Soak**: 30-min scripted driving across 5 stages under **ASan + UBSan + TSan**; no leaks, no races, no GL errors (`GL_KHR_debug` callback treated as test failure).
8. **Legacy compatibility**: auto-load all 132 maps + 16 events through the adapter; spawn the car; drive 20 s scripted; assert no exception and no NaN.
9. **Performance**: benchmark JSON compared with budget table (§7).

---

## §7 · PERFORMANCE & QUALITY BUDGETS

| Target | Reference machine | Setting | Budget |
|---|---|---|---|
| **Mid** | GTX 1060 6 GB / RX 580, 4c/8t i5-8400-class, 16 GB RAM | 1080p **High** | **avg ≥ 90 FPS**, 1 % low ≥ 60, frame ≤ 11 ms (CPU ≤ 6 ms, GPU ≤ 10 ms) |
| **Low** | Intel UHD 620 / Vega 8 iGPU, 8 GB RAM | 720p **Low** (+ resolution scale) | **avg ≥ 45 FPS**, 1 % low ≥ 30 |
| **High-end** | RTX 3070 / RX 6800-class | 4K **Ultra** | avg ≥ 75 FPS |

* **Draw calls** ≤ 1 500 typical frame (≤ 300 for all dressing; ≤ 120 for cars incl. AI); **visible triangles** ≤ 3 M (High).
* **Memory**: RAM ≤ 2.5 GB, **VRAM ≤ 3 GB (High)**, ≤ 1.5 GB (Low). Startup to main menu ≤ 8 s (SSD), stage load ≤ 5 s with streaming; no hitch > 16 ms during a run after warm-up (async uploads, tile streaming budget per frame).
* **Physics** ≤ 1.5 ms per 16 ms frame for 1 player + 3 ghost/AI cars.
* **Binary size**: executable ≤ 15 MB stripped; textures KTX2/zstd; install size reported per pack.

---

## §8 · THE "REALITY BACKLOG" (implement after the mandatory phases, in value-per-effort order; each item is optional but each shipped item needs tests + docs)
1. **Dynamic weather & time progression during a stage** (rain starts mid-stage → grip/visuals/audio change).
2. **Windscreen rain/mud/dust with wipers**; **interior/cockpit view** with driver/co-driver animation and working steering wheel.
3. **Persistent track evolution** (ruts, gravel displacement, tyre marks) affecting grip for later cars.
4. **Rival ghosts & stat-model opponents** (stage times from AI skill tables) producing championship standings and live splits.
5. **Service park & repair-time management**, tyre allocation, parc-fermé setup rules, penalties (false start, shortcut/cut, late arrival).
6. **Livery editor** (colours, number, sponsor slots, decals) exporting a pack.
7. **Pace-note editor + recce mode**; **stage editor** (spline road, spectator zones, props, weather timeline) using ImGui in dev builds.
8. **Replay broadcast director** with TV graphics (lower-thirds, split times, onboard camera tags).
9. **Dynamic resolution + temporal upscaler**; **HDR10 output**; ultrawide/triple-screen/VR-ready camera abstraction (no VR runtime dependency).
10. **FFB profiles per car/surface**, motion-platform UDP telemetry output (opt-in, local only).
11. **Time-attack with file-based ghost sharing**, per-stage stat sheets, achievements (local).
12. **Photo mode extras**: bracketed exposure, EXR export, focus peaking, rule-of-thirds overlay.
13. **Modding hooks**: Lua/JS scripting is **out** (keep engine lean); instead, expose data-driven rules and event triggers in JSON.

---

## §9 · DELIVERABLES CHECKLIST (the final repository must contain)
* `CMakeLists.txt`, `CMakePresets.json`, CI workflows, packaging (deb/AppImage/NSIS/zip), version single-source.
* New `engine/`, `sim/`, `game/`, `ui/`, `tools/`, `tests/` trees; legacy loaders; `content/packs/*`.
* Shaders in `engine/render/shaders/*.vert|frag|glsl` (with `#include`), **Appendix A as the baseline PBR shader**; **Appendix B/C as the vertex format + tests**.
* `docs/ARCHITECTURE.md`, `ADDING_CONTENT.md`, `RELEASING.md`, `DECISIONS.md`, `ASSETS_TODO.md`, `CREDITS.md` (generated), `CARS.md` (generated), `reports/PHASE_0…9.md`.
* README rewritten (features, screenshots, build, content-pack guide, performance tiers, legal/licensing note).
* Tags: `v0.6.7-baseline`, `phase-0-done` … `phase-9-done`, **`v0.7.0`**.

**Begin with PHASE 0 now.**

---
---

# APPENDIX A — REFERENCE PBR SHADERS (GLSL 330 core) · validated with `glslangValidator` (compile + link OK)

> Role of this code: the **baseline high-fidelity PBR material shader pair** to be integrated, tested and extended in PHASE 2.
> Cook-Torrance microfacet BRDF (D = GGX, G = Smith/Schlick-GGX, F = Schlick), Lambert diffuse, tangent-space normal mapping, packed Metallic-Roughness (glTF: G = roughness, B = metallic, R = optional AO), cubemap IBL (irradiance + prefiltered radiance + BRDF LUT), sun + point lights, clear-coat, wetness, specular anti-aliasing, ACES tone-map and gamma 2.2.
> Texture units: bind `uAlbedoMap`(sRGB), `uNormalMap`(linear), `uMetalRoughMap`(linear), `uIrradianceMap`, `uPrefilterMap`, `uBrdfLUT` to six distinct units.

## A.1 `pbr.vert`
```glsl
#version 330 core
// ============================================================================
// pbr.vert — TrackRS PBR vertex stage (GLSL 330 core)
// Works with BOTH vertex layouts in vertex_layout.h:
//   * Vertex        (56 B, explicit bitangent)  -> compile with -DEXPLICIT_BITANGENT
//   * VertexPacked  (32 B, 2_10_10_10 N/T, handedness in T.w, B = cross(N,T)*w)
// PERF: no per-vertex matrix inverse — uNormalMatrix is computed on the CPU once
//       per draw (or fetched per-instance). All varyings are interpolated in
//       world space so the fragment stage needs no view-space transforms.
// ============================================================================

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;     // normalised by the GPU (GL_TRUE) when packed
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec4 aTangent;    // xyz = tangent, w = handedness (+1/-1).
                                          // If fed from a vec3 attribute, w defaults to 1.0.
#ifdef EXPLICIT_BITANGENT
layout(location = 4) in vec3 aBitangent;
#endif
layout(location = 5) in vec4 aColor;      // optional: baked AO / wear / paint-mask (RGBA8, normalised)

uniform mat4 uModel;
uniform mat4 uViewProj;
uniform mat3 uNormalMatrix;               // transpose(inverse(mat3(uModel)))

out VS_OUT {
    vec3 worldPos;
    vec2 uv;
    vec3 T;
    vec3 B;
    vec3 N;
    vec4 color;
} vs;

void main()
{
    vec4 wp = uModel * vec4(aPosition, 1.0);

    vec3 N = normalize(uNormalMatrix * aNormal);
    vec3 T = normalize(uNormalMatrix * aTangent.xyz);
    T = normalize(T - dot(T, N) * N);                 // Gram-Schmidt re-orthogonalise after interpolation-safe transform
#ifdef EXPLICIT_BITANGENT
    vec3 B = normalize(uNormalMatrix * aBitangent);
#else
    vec3 B = cross(N, T) * aTangent.w;                // handedness-safe bitangent (mirrored UVs OK)
#endif

    vs.worldPos = wp.xyz;
    vs.uv       = aUV;
    vs.T = T; vs.B = B; vs.N = N;
    vs.color    = aColor;
    gl_Position = uViewProj * wp;
}
```

## A.2 `pbr.frag`
```glsl
#version 330 core
// ============================================================================
// pbr.frag — TrackRS Cook-Torrance PBR fragment stage (GLSL 330 core)
//
// BRDF  : Lambert diffuse + Cook-Torrance specular  f = D * G * F / (4 NdotL NdotV)
//           D = GGX / Trowbridge-Reitz        (D_GGX)
//           G = Smith, Schlick-GGX (direct k = (r+1)^2/8)   (G_SchlickGGX)
//           F = Schlick Fresnel               (F_Schlick)
// IBL   : split-sum  — irradiance cubemap + prefiltered radiance cubemap (mips = roughness)
//         + 2D BRDF LUT (scale, bias). Sharp chrome/clear-coat reflections come from the
//         prefiltered cubemap sampled with textureLod().
// EXTRAS: clear-coat lobe (car paint), wetness (asphalt/paint), specular anti-aliasing,
//         alpha-test, ACES filmic tone-map, gamma 2.2.
// TEXTURE CONTRACT (bind once per material, 4 sampler units for the surface set):
//   uAlbedoMap     sRGB8_A8  base colour (+alpha)   -> hardware sRGB->linear, NO pow() in shader
//   uNormalMap     RG/RGB    tangent space, LINEAR  -> Z reconstructed (BC5-ready)
//   uMetalRoughMap RGB       R=AO (optional)  G=roughness  B=metallic   (glTF 2.0 packing), LINEAR
//   IBL: uIrradianceMap (cube) / uPrefilterMap (cube, full mip chain) / uBrdfLUT (RG16F)
// ============================================================================

in VS_OUT {
    vec3 worldPos;
    vec2 uv;
    vec3 T;
    vec3 B;
    vec3 N;
    vec4 color;
} fs;

out vec4 FragColor;

// ---- material -------------------------------------------------------------
uniform sampler2D uAlbedoMap;
uniform sampler2D uNormalMap;
uniform sampler2D uMetalRoughMap;
uniform vec4  uBaseColorFactor = vec4(1.0);
uniform float uMetallicFactor  = 1.0;
uniform float uRoughnessFactor = 1.0;
uniform float uNormalScale     = 1.0;
uniform float uAOStrength      = 1.0;   // 0 disables the R channel of the MR map
uniform float uAlphaCutoff     = 0.0;   // >0 => alpha test (foliage, signs, fences)
uniform float uClearcoat       = 0.0;   // 0..1  (car paint = 1.0)
uniform float uClearcoatRough  = 0.03;
uniform float uWetness         = 0.0;   // 0..1  (rain / wet tarmac)

// ---- lighting -------------------------------------------------------------
uniform vec3  uCamPos;
uniform vec3  uSunDir;                  // direction TOWARDS the sun (normalised)
uniform vec3  uSunColor;                // linear radiance * intensity
uniform vec3  uAmbientColor;            // fallback when no IBL is bound
#define MAX_POINT_LIGHTS 8
uniform int   uPointLightCount;
uniform vec3  uPointLightPos[MAX_POINT_LIGHTS];
uniform vec4  uPointLightColorRange[MAX_POINT_LIGHTS];   // rgb = radiance, a = range (m)

// ---- image based lighting -------------------------------------------------
uniform samplerCube uIrradianceMap;
uniform samplerCube uPrefilterMap;
uniform sampler2D   uBrdfLUT;
uniform float uPrefilterMaxLod = 7.0;   // mip count - 1 of uPrefilterMap
uniform float uEnvIntensity    = 1.0;
uniform bool  uHasIBL          = false;

// ---- output ---------------------------------------------------------------
uniform float uExposure = 1.0;
uniform bool  uToneMap  = true;          // false when a later HDR post pass tone-maps
const float GAMMA = 2.2;
const float PI    = 3.14159265359;

// ============================================================================
// BRDF building blocks
// ============================================================================
float D_GGX(float NdotH, float alpha)                       // alpha = roughness^2
{
    float a2 = alpha * alpha;
    float d  = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d + 1e-7);
}

float G_SchlickGGX(float NdotX, float k)
{
    return NdotX / (NdotX * (1.0 - k) + k);
}

float G_Smith(float NdotV, float NdotL, float roughness)   // direct-light remap
{
    float r = roughness + 1.0;
    float k = (r * r) * 0.125;
    return G_SchlickGGX(NdotV, k) * G_SchlickGGX(NdotL, k);
}

vec3 F_Schlick(float cosTheta, vec3 F0)
{
    float f = clamp(1.0 - cosTheta, 0.0, 1.0);
    float f2 = f * f;
    return F0 + (1.0 - F0) * (f2 * f2 * f);                // x^5 without pow()
}

vec3 F_SchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    float f = clamp(1.0 - cosTheta, 0.0, 1.0);
    float f2 = f * f;
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * (f2 * f2 * f);
}

// Kaplanyan/Tokuyoshi specular AA: kills sparkle on normal-mapped, high-frequency surfaces
float specularAA(vec3 N, float roughness)
{
    vec3 dndx = dFdx(N), dndy = dFdy(N);
    float variance = 0.25 * (dot(dndx, dndx) + dot(dndy, dndy));
    return sqrt(clamp(roughness * roughness + min(2.0 * variance, 0.18), 0.0, 1.0));
}

vec3 tonemapACES(vec3 x)                                    // Narkowicz fit
{
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// Cook-Torrance + Lambert for ONE light. Returns outgoing radiance contribution.
vec3 evalLight(vec3 L, vec3 radiance, vec3 N, vec3 V, float NdotV,
               vec3 albedo, float metallic, float roughness, vec3 F0)
{
    vec3  H     = normalize(V + L);
    float NdotL = max(dot(N, L), 0.0);
    if (NdotL <= 0.0) return vec3(0.0);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    float alpha = roughness * roughness;
    float D = D_GGX(NdotH, alpha);
    float G = G_Smith(NdotV, NdotL, roughness);
    vec3  F = F_Schlick(VdotH, F0);

    vec3 specular = (D * G) * F / (4.0 * NdotV * NdotL + 1e-4);
    vec3 kD       = (vec3(1.0) - F) * (1.0 - metallic);     // energy conservation
    vec3 diffuse  = kD * albedo / PI;

    return (diffuse + specular) * radiance * NdotL;
}

// ============================================================================
void main()
{
    // ---- fetch material (3 texture reads for the whole surface) -----------
    vec4 baseSample = texture(uAlbedoMap, fs.uv) * uBaseColorFactor;
    if (uAlphaCutoff > 0.0 && baseSample.a < uAlphaCutoff) discard;

    vec3 albedo = baseSample.rgb * fs.color.rgb;            // vertex colour = baked wear/AO tint (white if unused)
    vec3 mr     = texture(uMetalRoughMap, fs.uv).rgb;
    float ao        = mix(1.0, mr.r, uAOStrength) * fs.color.a;
    float roughness = clamp(mr.g * uRoughnessFactor, 0.04, 1.0);
    float metallic  = clamp(mr.b * uMetallicFactor, 0.0, 1.0);

    // ---- wetness: darker diffuse, smoother surface -------------------------
    albedo    *= mix(1.0, 0.55, uWetness * (1.0 - metallic));
    roughness  = mix(roughness, min(roughness, 0.12), uWetness);

    // ---- tangent-space normal mapping ---------------------------------------
    vec2 nxy = (texture(uNormalMap, fs.uv).xy * 2.0 - 1.0) * uNormalScale;
    vec3 nts = vec3(nxy, sqrt(max(1.0 - dot(nxy, nxy), 0.0)));
    mat3 TBN = mat3(normalize(fs.T), normalize(fs.B), normalize(fs.N));
    vec3 N   = normalize(TBN * nts);
    if (!gl_FrontFacing) N = -N;                            // two-sided foliage / panels
    vec3 V   = normalize(uCamPos - fs.worldPos);
    float NdotV = max(dot(N, V), 1e-4);

    roughness = max(specularAA(N, roughness), 0.04);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);            // dielectric 4% / metal = albedo

    // ---- direct lighting -----------------------------------------------------
    vec3 Lo = evalLight(normalize(uSunDir), uSunColor, N, V, NdotV, albedo, metallic, roughness, F0);

    for (int i = 0; i < uPointLightCount; ++i) {            // headlights, fire, flash — keep count small
        vec3  toL  = uPointLightPos[i] - fs.worldPos;
        float dist = length(toL);
        float range = uPointLightColorRange[i].a;
        float att  = clamp(1.0 - pow(dist / range, 4.0), 0.0, 1.0);   // smooth finite-range window
        att = att * att / (dist * dist + 1.0);
        Lo += evalLight(toL / dist, uPointLightColorRange[i].rgb * att, N, V, NdotV,
                        albedo, metallic, roughness, F0);
    }

    // ---- clear-coat lobe (fixed F0 = 0.04) ------------------------------------
    float Fcc = 0.0;
    if (uClearcoat > 0.0) {
        vec3  L  = normalize(uSunDir);
        vec3  H  = normalize(V + L);
        float NdotL = max(dot(N, L), 0.0), NdotH = max(dot(N, H), 0.0), VdotH = max(dot(V, H), 0.0);
        float a  = uClearcoatRough * uClearcoatRough;
        Fcc      = uClearcoat * F_Schlick(VdotH, vec3(0.04)).x;
        float sc = D_GGX(NdotH, a) * G_Smith(NdotV, NdotL, uClearcoatRough) * Fcc
                   / (4.0 * NdotV * NdotL + 1e-4);
        Lo = Lo * (1.0 - Fcc) + sc * uSunColor * NdotL;     // base layer attenuated by coat
    }

    // ---- image based lighting (ambient) --------------------------------------
    vec3 ambient;
    if (uHasIBL) {
        vec3 F  = F_SchlickRoughness(NdotV, F0, roughness);
        vec3 kD = (1.0 - F) * (1.0 - metallic);
        vec3 irradiance = texture(uIrradianceMap, N).rgb;
        vec3 R = reflect(-V, N);
        vec3 prefiltered = textureLod(uPrefilterMap, R, roughness * uPrefilterMaxLod).rgb;
        vec2 brdf = texture(uBrdfLUT, vec2(NdotV, roughness)).rg;
        vec3 specIBL = prefiltered * (F * brdf.x + brdf.y);

        // clear-coat reflection: sharp mip, own Fresnel
        if (uClearcoat > 0.0) {
            vec3  Rc  = reflect(-V, normalize(fs.N));       // coat ignores the base normal map: glassy orange-peel-free
            vec3  cc  = textureLod(uPrefilterMap, Rc, uClearcoatRough * uPrefilterMaxLod).rgb;
            float fc  = uClearcoat * F_SchlickRoughness(NdotV, vec3(0.04), uClearcoatRough).x;
            specIBL   = specIBL * (1.0 - fc) + cc * fc;
            kD       *= (1.0 - fc);
        }
        ambient = (kD * irradiance * albedo + specIBL) * ao * uEnvIntensity;
    } else {
        ambient = uAmbientColor * albedo * ao;
    }

    vec3 color = (Lo + ambient) * uExposure;

    // ---- tone-map + gamma 2.2 ---------------------------------------------------
    if (uToneMap) {
        color = tonemapACES(color);
        color = pow(color, vec3(1.0 / GAMMA));
    }
    FragColor = vec4(color, baseSample.a);
}
```

---

# APPENDIX B — VERTEX STRUCTS, TANGENT GENERATION, ATTRIBUTE SETUP (C++17, OpenGL 3.3 core) · compiled `-Wall -Wextra -pedantic`, tested under ASan/UBSan

> Layouts: `Vertex` 56 B (explicit T/B for tools), `VertexPacked` 32 B (shipping; 2 vertices per 64 B cache line, 2_10_10_10 normal/tangent, handedness in T.w, RGBA8 AO/tint).
> Attribute locations match `pbr.vert`. Compile the vertex shader with `#define EXPLICIT_BITANGENT` **only** when feeding the 56 B layout.

```cpp
// ============================================================================
// vertex_layout.h — TrackRS PBR mesh vertex formats + GL 3.3 attribute setup
//
// Two layouts, same shader (pbr.vert):
//   Vertex        56 B  explicit Position/Normal/UV/Tangent/Bitangent   (tools, debug, mesh import)
//   VertexPacked  32 B  shipping layout: 2 vertices per 64-byte cache line
//
// PERF NOTES
//  * Interleave everything in ONE VBO: one cache stream per vertex fetch, one bind.
//  * 32 B stride divides the 64 B GPU/CPU cache line evenly -> no vertex straddles two lines.
//  * 2_10_10_10 normals/tangents: 4 B instead of 12 B, ~0.001 angular error — invisible after
//    per-pixel normal mapping. GL normalises them (normalized = GL_TRUE), zero ALU cost.
//  * Always draw indexed (GL_UNSIGNED_INT, or GL_UNSIGNED_SHORT when < 65536 verts) and run
//    meshoptimizer's vertex-cache + overdraw + vertex-fetch optimisation at import time.
//  * One VAO per mesh format; bind VAO once per draw. Never call glVertexAttribPointer per frame.
// ============================================================================
#pragma once

#include <GL/glew.h>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

// ---------------------------------------------------------------------------
// Attribute locations — must match pbr.vert (layout(location = N))
// ---------------------------------------------------------------------------
enum VertexAttrib : GLuint {
    ATTR_POSITION  = 0,
    ATTR_NORMAL    = 1,
    ATTR_UV        = 2,
    ATTR_TANGENT   = 3,
    ATTR_BITANGENT = 4,   // only the 56 B layout feeds it (compile shader with EXPLICIT_BITANGENT)
    ATTR_COLOR     = 5,
};

// ---------------------------------------------------------------------------
// 56-byte explicit layout (14 floats, 4-byte aligned, no padding)
// ---------------------------------------------------------------------------
struct Vertex {
    float position[3];    // offset  0
    float normal[3];      // offset 12
    float uv[2];          // offset 24
    float tangent[3];     // offset 32  (dPos/du, orthogonalised against N)
    float bitangent[3];   // offset 44  (cross(N,T) * handedness)
};
static_assert(sizeof(Vertex) == 56, "Vertex must stay tightly packed (14 floats)");
static_assert(offsetof(Vertex, normal)    == 12, "layout drift");
static_assert(offsetof(Vertex, uv)        == 24, "layout drift");
static_assert(offsetof(Vertex, tangent)   == 32, "layout drift");
static_assert(offsetof(Vertex, bitangent) == 44, "layout drift");

// ---------------------------------------------------------------------------
// 32-byte shipping layout
// ---------------------------------------------------------------------------
struct VertexPacked {
    float    position[3]; // offset  0  12 B
    uint32_t normal;      // offset 12   4 B  GL_INT_2_10_10_10_REV, normalised  (w unused)
    uint32_t tangent;     // offset 16   4 B  xyz = tangent, 2-bit w = handedness (+1 / -1)
    float    uv[2];       // offset 20   8 B
    uint32_t color;       // offset 28   4 B  RGBA8: rgb = tint/wear, a = baked AO (255 = none)
};
static_assert(sizeof(VertexPacked) == 32, "VertexPacked must be exactly half a cache line");
static_assert(offsetof(VertexPacked, normal)  == 12, "layout drift");
static_assert(offsetof(VertexPacked, tangent) == 16, "layout drift");
static_assert(offsetof(VertexPacked, uv)      == 20, "layout drift");
static_assert(offsetof(VertexPacked, color)   == 28, "layout drift");

// ---------------------------------------------------------------------------
// Packing helpers (CPU, import-time only)
// ---------------------------------------------------------------------------
// GL_INT_2_10_10_10_REV: x = bits 0..9, y = 10..19, z = 20..29, w = 30..31 (all two's complement).
// With normalized = GL_TRUE: 10-bit value / 511, 2-bit value in {-1, 0, +1} (0b11 = -1, 0b01 = +1).
inline uint32_t packSnorm2_10_10_10(float x, float y, float z, int w /* -1, 0 or +1 */)
{
    auto s10 = [](float v) -> uint32_t {
        const int i = static_cast<int>(std::lround(std::clamp(v, -1.0f, 1.0f) * 511.0f));
        return static_cast<uint32_t>(i) & 0x3FFu;
    };
    const uint32_t w2 = (w > 0) ? 0x1u : (w < 0 ? 0x3u : 0x0u);
    return s10(x) | (s10(y) << 10) | (s10(z) << 20) | (w2 << 30);
}

inline uint32_t packRGBA8(float r, float g, float b, float a)
{
    auto u8 = [](float v) -> uint32_t {
        return static_cast<uint32_t>(std::lround(std::clamp(v, 0.0f, 1.0f) * 255.0f));
    };
    return u8(r) | (u8(g) << 8) | (u8(b) << 16) | (u8(a) << 24);
}

// ---------------------------------------------------------------------------
// Tangent / bitangent generation (per-triangle accumulate, then orthogonalise).
// Call after positions, normals and UVs are filled and BEFORE any packing.
// Handles mirrored UVs (handedness) and degenerate UV triangles.
// ---------------------------------------------------------------------------
inline void computeTangents(std::vector<Vertex>& v, const std::vector<uint32_t>& idx)
{
    std::vector<float> tan1(v.size() * 3, 0.0f), tan2(v.size() * 3, 0.0f);

    for (size_t i = 0; i + 2 < idx.size(); i += 3) {
        const uint32_t i0 = idx[i], i1 = idx[i + 1], i2 = idx[i + 2];
        const Vertex &a = v[i0], &b = v[i1], &c = v[i2];

        const float e1[3] = { b.position[0] - a.position[0], b.position[1] - a.position[1], b.position[2] - a.position[2] };
        const float e2[3] = { c.position[0] - a.position[0], c.position[1] - a.position[1], c.position[2] - a.position[2] };
        const float du1 = b.uv[0] - a.uv[0], dv1 = b.uv[1] - a.uv[1];
        const float du2 = c.uv[0] - a.uv[0], dv2 = c.uv[1] - a.uv[1];

        const float det = du1 * dv2 - du2 * dv1;
        if (std::fabs(det) < 1e-12f) continue;           // degenerate UV mapping: skip
        const float r = 1.0f / det;

        float t[3], bt[3];
        for (int k = 0; k < 3; ++k) {
            t[k]  = (e1[k] * dv2 - e2[k] * dv1) * r;     // dP/du
            bt[k] = (e2[k] * du1 - e1[k] * du2) * r;     // dP/dv
        }
        for (uint32_t id : { i0, i1, i2 })
            for (int k = 0; k < 3; ++k) { tan1[id * 3 + k] += t[k]; tan2[id * 3 + k] += bt[k]; }
    }

    for (size_t i = 0; i < v.size(); ++i) {
        const float *n = v[i].normal, *t = &tan1[i * 3], *b2 = &tan2[i * 3];
        float d = n[0] * t[0] + n[1] * t[1] + n[2] * t[2];
        float T[3] = { t[0] - n[0] * d, t[1] - n[1] * d, t[2] - n[2] * d };   // Gram-Schmidt
        float len = std::sqrt(T[0] * T[0] + T[1] * T[1] + T[2] * T[2]);
        if (len < 1e-8f) {                                // fallback: any vector perpendicular to N
            const bool useX = std::fabs(n[0]) < 0.9f;
            const float a[3] = { useX ? 1.0f : 0.0f, useX ? 0.0f : 1.0f, 0.0f };   // axis least aligned with N
            T[0] = n[1] * a[2] - n[2] * a[1];                 // T = cross(N, a)
            T[1] = n[2] * a[0] - n[0] * a[2];
            T[2] = n[0] * a[1] - n[1] * a[0];
            len = std::sqrt(T[0] * T[0] + T[1] * T[1] + T[2] * T[2]);
        }
        const float inv = 1.0f / std::max(len, 1e-12f);
        T[0] *= inv; T[1] *= inv; T[2] *= inv;

        const float cx = n[1] * T[2] - n[2] * T[1];       // cross(N, T)
        const float cy = n[2] * T[0] - n[0] * T[2];
        const float cz = n[0] * T[1] - n[1] * T[0];
        const float handed = (cx * b2[0] + cy * b2[1] + cz * b2[2]) < 0.0f ? -1.0f : 1.0f;

        for (int k = 0; k < 3; ++k) v[i].tangent[k] = T[k];
        v[i].bitangent[0] = cx * handed; v[i].bitangent[1] = cy * handed; v[i].bitangent[2] = cz * handed;
    }
}

// Convert the explicit 56 B build-time vertex into the 32 B shipping vertex.
inline VertexPacked packVertex(const Vertex& s, uint32_t rgba = 0xFFFFFFFFu)
{
    const float cx = s.normal[1] * s.tangent[2] - s.normal[2] * s.tangent[1];
    const float cy = s.normal[2] * s.tangent[0] - s.normal[0] * s.tangent[2];
    const float cz = s.normal[0] * s.tangent[1] - s.normal[1] * s.tangent[0];
    const int handed = (cx * s.bitangent[0] + cy * s.bitangent[1] + cz * s.bitangent[2]) < 0.0f ? -1 : 1;

    VertexPacked p;
    p.position[0] = s.position[0]; p.position[1] = s.position[1]; p.position[2] = s.position[2];
    p.normal  = packSnorm2_10_10_10(s.normal[0], s.normal[1], s.normal[2], 0);
    p.tangent = packSnorm2_10_10_10(s.tangent[0], s.tangent[1], s.tangent[2], handed);
    p.uv[0] = s.uv[0]; p.uv[1] = s.uv[1];
    p.color = rgba;
    return p;
}

// ---------------------------------------------------------------------------
// GL 3.3 core attribute setup. Call ONCE at mesh creation with the VAO bound.
// ---------------------------------------------------------------------------
inline void bindVertexFormat56(GLuint vao, GLuint vbo, GLuint ibo)
{
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);          // IBO binding is captured by the VAO

    const GLsizei stride = sizeof(Vertex);               // 56
    glEnableVertexAttribArray(ATTR_POSITION);
    glVertexAttribPointer(ATTR_POSITION,  3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(ATTR_NORMAL);
    glVertexAttribPointer(ATTR_NORMAL,    3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(ATTR_UV);
    glVertexAttribPointer(ATTR_UV,        2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(Vertex, uv)));
    glEnableVertexAttribArray(ATTR_TANGENT);
    glVertexAttribPointer(ATTR_TANGENT,   3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(Vertex, tangent)));
    glEnableVertexAttribArray(ATTR_BITANGENT);
    glVertexAttribPointer(ATTR_BITANGENT, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(Vertex, bitangent)));
    glDisableVertexAttribArray(ATTR_COLOR);
    glVertexAttrib4f(ATTR_COLOR, 1.0f, 1.0f, 1.0f, 1.0f);   // constant white when the mesh has no vertex colour

    glBindVertexArray(0);
}

inline void bindVertexFormat32(GLuint vao, GLuint vbo, GLuint ibo)
{
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

    const GLsizei stride = sizeof(VertexPacked);         // 32
    glEnableVertexAttribArray(ATTR_POSITION);
    glVertexAttribPointer(ATTR_POSITION, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(VertexPacked, position)));
    glEnableVertexAttribArray(ATTR_NORMAL);              // 4 B -> vec3, GPU un-packs and normalises for free
    glVertexAttribPointer(ATTR_NORMAL,   4, GL_INT_2_10_10_10_REV, GL_TRUE, stride, reinterpret_cast<const void*>(offsetof(VertexPacked, normal)));
    glEnableVertexAttribArray(ATTR_UV);
    glVertexAttribPointer(ATTR_UV,       2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(VertexPacked, uv)));
    glEnableVertexAttribArray(ATTR_TANGENT);             // .w = handedness
    glVertexAttribPointer(ATTR_TANGENT,  4, GL_INT_2_10_10_10_REV, GL_TRUE, stride, reinterpret_cast<const void*>(offsetof(VertexPacked, tangent)));
    glDisableVertexAttribArray(ATTR_BITANGENT);          // derived in the vertex shader
    glEnableVertexAttribArray(ATTR_COLOR);
    glVertexAttribPointer(ATTR_COLOR,    4, GL_UNSIGNED_BYTE, GL_TRUE, stride, reinterpret_cast<const void*>(offsetof(VertexPacked, color)));

    glBindVertexArray(0);
}

// ---------------------------------------------------------------------------
// Minimal RAII mesh: one VAO + one interleaved VBO + one IBO.
// ---------------------------------------------------------------------------
class GpuMesh {
public:
    GpuMesh() = default;
    GpuMesh(const GpuMesh&) = delete;
    GpuMesh& operator=(const GpuMesh&) = delete;
    GpuMesh(GpuMesh&& o) noexcept { swap(o); }
    GpuMesh& operator=(GpuMesh&& o) noexcept { if (this != &o) { release(); swap(o); } return *this; }
    ~GpuMesh() { release(); }

    // Upload the 32 B shipping format (recommended).
    void uploadPacked(const std::vector<VertexPacked>& verts, const std::vector<uint32_t>& indices)
    {
        create(verts.data(), verts.size() * sizeof(VertexPacked), indices);
        bindVertexFormat32(vao_, vbo_, ibo_);
    }
    // Upload the 56 B explicit format (tools / debug).
    void uploadExplicit(const std::vector<Vertex>& verts, const std::vector<uint32_t>& indices)
    {
        create(verts.data(), verts.size() * sizeof(Vertex), indices);
        bindVertexFormat56(vao_, vbo_, ibo_);
    }

    void draw() const
    {
        glBindVertexArray(vao_);
        glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
    }
    void drawInstanced(GLsizei instances) const
    {
        glBindVertexArray(vao_);
        glDrawElementsInstanced(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr, instances);
    }
    GLuint vao() const { return vao_; }

private:
    void create(const void* vdata, size_t vbytes, const std::vector<uint32_t>& indices)
    {
        release();
        indexCount_ = static_cast<GLsizei>(indices.size());
        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);
        glGenBuffers(1, &ibo_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vbytes), vdata, GL_STATIC_DRAW);
        glBindVertexArray(vao_);                          // so the IBO upload is not captured by a stale VAO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(uint32_t)),
                     indices.data(), GL_STATIC_DRAW);
        glBindVertexArray(0);
    }
    void release()
    {
        if (ibo_) glDeleteBuffers(1, &ibo_);
        if (vbo_) glDeleteBuffers(1, &vbo_);
        if (vao_) glDeleteVertexArrays(1, &vao_);
        vao_ = vbo_ = ibo_ = 0; indexCount_ = 0;
    }
    void swap(GpuMesh& o) noexcept
    {
        std::swap(vao_, o.vao_); std::swap(vbo_, o.vbo_);
        std::swap(ibo_, o.ibo_); std::swap(indexCount_, o.indexCount_);
    }
    GLuint  vao_ = 0, vbo_ = 0, ibo_ = 0;
    GLsizei indexCount_ = 0;
};
```

---

# APPENDIX C — CPU-ONLY UNIT TEST FOR THE LAYOUT CODE (extend into the real test suite)

```cpp
// CPU-only test of the tangent maths + packing (no GL context needed).
#include "vertex_layout.h"
#include <cassert>
#include <cstdio>

static float dot3(const float* a, const float* b) { return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]; }

int main()
{
    // Quad in XY plane facing +Z, UV = (x, y), plus a mirrored-UV copy
    std::vector<Vertex> v(4);
    const float P[4][3] = {{0,0,0},{1,0,0},{1,1,0},{0,1,0}};
    const float UV[4][2] = {{0,0},{1,0},{1,1},{0,1}};
    for (int i = 0; i < 4; ++i) {
        for (int k = 0; k < 3; ++k) v[i].position[k] = P[i][k];
        v[i].normal[0] = 0; v[i].normal[1] = 0; v[i].normal[2] = 1;
        v[i].uv[0] = UV[i][0]; v[i].uv[1] = UV[i][1];
    }
    std::vector<uint32_t> idx = {0,1,2, 0,2,3};
    computeTangents(v, idx);
    for (auto& x : v) {
        assert(std::fabs(x.tangent[0] - 1) < 1e-5f && std::fabs(x.tangent[1]) < 1e-5f);     // T = +X
        assert(std::fabs(x.bitangent[1] - 1) < 1e-5f);                                      // B = +Y
        assert(std::fabs(dot3(x.tangent, x.normal)) < 1e-5f);
    }

    // Mirrored U -> handedness must flip so B stays dP/dv-consistent
    std::vector<Vertex> m = v;
    for (auto& x : m) x.uv[0] = 1.0f - x.uv[0];
    computeTangents(m, idx);
    VertexPacked pm = packVertex(m[0]);
    assert((pm.tangent >> 30) == 0x1u || (pm.tangent >> 30) == 0x3u);

    VertexPacked p = packVertex(v[0]);
    assert((p.tangent >> 30) == 0x1u);                       // right-handed => +1
    assert((p.normal & 0x3FFu) == 0u && ((p.normal >> 20) & 0x3FFu) == 511u);   // N = (0,0,1)
    assert(sizeof(VertexPacked) == 32 && sizeof(Vertex) == 56);

    // degenerate-UV fallback must still produce unit, perpendicular tangents
    std::vector<Vertex> d = v;
    for (auto& x : d) { x.uv[0] = 0; x.uv[1] = 0; }
    computeTangents(d, idx);
    for (auto& x : d) {
        float l = std::sqrt(dot3(x.tangent, x.tangent));
        assert(std::fabs(l - 1.0f) < 1e-4f && std::fabs(dot3(x.tangent, x.normal)) < 1e-4f);
    }
    std::puts("layout tests passed");
    return 0;
}
```

---

# APPENDIX D — ACCEPTANCE CHECK-LIST FOR THE PBR/VERTEX WORK (PHASE 2 gate)
- [ ] `glslangValidator -l pbr.vert pbr.frag` (+ all permutations) passes in CI.
- [ ] White-furnace test (rough = 0.04…1.0, metal = 0/1) never exceeds 1.0 and loses < 5 % energy at roughness 1.0 with IBL.
- [ ] Normal-map handedness test: mirrored-UV sphere shows continuous lighting across the seam.
- [ ] `static_assert` stride/offset checks present; `sizeof(VertexPacked) == 32`.
- [ ] No `glVertexAttribPointer` call executes per frame (only at mesh creation).
- [ ] Debug views (albedo/normal/roughness/metallic/AO/tangent frame) available on F-keys.
- [ ] Car paint shows a **sharp environment reflection with clear-coat**, chrome rims reflect the sky with correct Fresnel, wet asphalt reflects the sun glint at grazing angles, rubber stays rough and dark.
- [ ] Cars: 1 draw call per material per LOD; no CPU-side vertex work per frame.