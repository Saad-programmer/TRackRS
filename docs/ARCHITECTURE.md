# Engine baseline and migration boundary

The current executable is still the 0.6.7 compatibility renderer and simulation.
The Reality target architecture is specified in CODEX_MASTER_PROMPT.md; the
directories and features described there are a migration target, not a claim
about implemented systems.

## Current layers

- `src/PEngine`: SDL platform loop, PhysFS resources, compatibility OpenGL
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
TriggerGame, which owns PSim and PTerrain. No ownership refactor, new content-pack
schema, GL core conversion, or new vehicle dynamics has been introduced yet.

## Phase 0 seams

`validation::Run` owns one process's replay, rolling state hash and measurements.
CLI options are parsed before SDL startup. Interactive runs keep the original
timing, input and rendering paths; opt-in validation supplies a 0.004-second
delta and scripted controls to the real game and simulation.

Each scenario starts in its own process and fresh profile because legacy
function-static accumulators, RNG and other shared state have not been fixed.
The harness deliberately exposes this boundary instead of pretending those
defects are already resolved.

`legacy_metrics.h` forwards the submission forms present in this revision to
their original GL functions, counting submitted primitives when capture is
enabled. Timestamp scope queries are collected at the existing frame fence.
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
