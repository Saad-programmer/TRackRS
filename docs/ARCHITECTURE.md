# Engine baseline and migration boundary

The game uses one GL 3.3-or-later core context. Phase 2b removes the shared
compatibility context and all removed fixed-function driver entry points.
The Reality target architecture is specified in CODEX_MASTER_PROMPT.md; the
directories and features described there are a migration target, not a claim
about implemented systems.

## Current layers

- `src/PEngine`: SDL platform loop, PhysFS resources, core OpenGL
  rendering, terrain, effects/material loading, images, models, OpenAL audio,
  configuration and maths support.
- `src/PSim`: rigid bodies, vehicles, engine model, collision and damage.
  This legacy layer still depends on terrain/model engine types; it is not yet
  an independent, GL-free simulation library.
- `src/Trigger`: application/game state, stages/events, menus, HUD, controls,
  ghosts, co-driver logic, game rendering.
- `data`: original XML stages/events/vehicles, image sets, OBJ models, audio
  and plugin archives. The existing loaders remain the source of runtime behavior.

The PApp loop owns the window/context and legacy subsystems. MainApp loads
TriggerGame, which owns PSim and PTerrain. The Phase 1 module ownership and fixed-step changes are recorded below.
Phase 2a adds core presentation without changing legacy content or vehicle dynamics.

## Phase 0 seams

`validation::Run` owns one process's replay, rolling state hash and measurements.
CLI options are parsed before SDL startup. Interactive runs keep the original
timing, input and rendering paths; opt-in validation supplies a 0.004-second
delta and scripted controls to the real game and simulation.

Each scenario starts in its own process and fresh profile because legacy
function-static accumulators, RNG and other shared state have not been fixed.
The harness deliberately exposes this boundary instead of pretending those
defects are already resolved.

`legacy_metrics.h` owns CPU/GPU pass scopes. `engine/render/basic.cpp` records
actual GPU submissions and submitted triangles, including instanced multiplicity
and strip degenerates. Pending stream geometry drains at frame presentation.
Pass state is borrowed from the process-owned Run; query objects are deleted
before context teardown. The profiler is main-thread-only.

CMake builds the same source layers and uses conservative floating-point flags.
Only the new CPU-only doctest executable is independent of SDL/GL. Linux and
Windows CI jobs exercise the build boundary; their actual results determine
which platform gates pass.

## Later phases

Follow the supplied phase gates before moving responsibilities into
`engine/`, `sim/`, `game/`, `ui/` and `content/`. Preserve the recorded legacy
hashes/screenshots, explain intentional changes, and maintain the original
loaders behind adapters as the new renderer and schemas are introduced.

## Phase 1 component boundary

CMake compiles `trackrs_core`, `trackrs_platform`, `trackrs_renderer`, `trackrs_ui`,
`trackrs_audio`, `trackrs_world` and `trackrs_sim` independently, then links the
application's `src/Trigger` composition. Header-only Core primitives have CPU-only
unit tests; the typed application state and edition JSON loader sit above Core.
Renderer consumes borrowed vehicle snapshots; interpolation does not mutate Sim.
The platform configuration host is PApp, without a MainApp downcast.

`tests/check_layering.py` follows transitive local headers. Core cannot include
other project layers; engine components cannot include the application/game
layer. CMake runs this at configure and every game build, including when tests
are disabled. UI widgets moved out of application menu composition; application
state dispatch and audio logic have separate sources. Legacy World/Sim/render
interfaces still share legacy engine types; the core rendering boundary is below.

## Phase 2b core rendering boundary

PApp owns one CoreDevice and one SDL core context. CoreDevice owns the core-only
renderer and KHR_debug callback; assets and their GPU objects are destroyed before
the device/context. There is no hidden window, compatibility context, framebuffer
copy or fullscreen presentation bridge.

`engine/render/basic` preserves the original look with a basic GLSL 330 shader:
vertex directional lighting/specular, exponential fog, explicit planar/detail UVs,
alpha cutoff and the original blend/depth order. It does not implement PBR.
Static vehicle/model meshes retain VAO/VBO storage; terrain and foliage explicitly
bind core vertex attributes and indexed buffers. PVBuffer is a move-only GPU owner.

Dynamic scene/UI geometry is tessellated on the CPU and appended to ordered
vertex batches. CPU transform stacks are ordinary math, never driver matrix
state; transformed vertices allow adjacent UI items to batch across color and
transform changes. Texture/raster/shader-state changes drain batches in order.
Text uses signed-distance alpha fields derived from the original licensed font
atlas, preserving its glyph layout, advances and RGB outline.

Dust, rain and snow are instanced. Three GPU instance-buffer slots rotate with
GLsync fences, mapping only after their previous draw completes. Instance bases,
color gradients and UV orientation reproduce the old billboards/rain ribbons.
GPU tests wrap the ring ten times and validate pixels and actual submission counts.

RHI shader hot reload, include bounds, last-good program rollback, Frame UBOs and
fatal KHR_debug checks remain active. Identity shaders used by RHI tests live under
`tests/fixtures/rhi`; game shaders live under `data/shaders/basic`.

See [the 2b report](reports/PHASE_2B.md) for image tolerances, state hashes,
submission counts and platform limits. Phase 2c is not part of this boundary.
