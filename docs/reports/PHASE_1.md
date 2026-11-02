# Phase 1 — defect purge

Phase 1 implementation and local acceptance checks are complete. Phase 2 has not
started. The final `phase-1-done` tag is created only after the final main-branch
CI run passes.

Phase 0 closed at `18c75bd088a4d1116c11403edf826496916359ca`, with
[green CI](https://github.com/Saad-programmer/TRackRS/actions/runs/41406279839)
before `phase-0-done` was pushed. The allocation-origin policy in ADR 006 remains
unchanged: audited Mesa allocations may have TrackRS callers; TrackRS-origin leaks
remain fatal. `lsan.supp` is deliberately not a native any-frame suppression file.

## Delivered behavior

| Defect | Phase 1 result |
| --- | --- |
| R-07 | High-resolution frame timing, off/on/adaptive vsync with fallback, optional frame cap, sleep plus final short yield; redundant pre-swap `glFlush` removed. |
| R-08 | Per-PSim accumulator, default 0.004-second step, configurable constructor step, bounded catch-up, reset support; render-only GLM body interpolation. Validation keeps exact fixed ticks and current-state images. |
| R-09 | All Make entry points use CMake; release uses O3, no fast-math, `ffp-contract=off`, x86-64-v2 on x86-64; native instructions opt in, supported LTO enabled. |
| R-10 | Audited, not fabricated: the snapshot has nine fictional and two additional vehicle definitions, with existing meshes. Car Definition v2/calibration remains the explicitly scheduled Phase 4 work. |
| R-11 | JSON `trackrs.edition/1` policy: demo path allowlist, full installed-content access with championship locks, dev unlock bypass. One policy API also governs vehicle unlocks. |
| R-12 | Removed the shell command and external ffplay process. Existing licensed splash is displayed inside SDL/GL and its remaining delay is skippable after loading. |
| R-13 | Owned subsystem/resource/simulation/game/terrain/particle/audio lifetimes; failed vehicle-type loads release allocations; PCG streams replace all libc RNG calls; ASE token state is per load; unsafe C copies/formatting removed and clang-tidy rejects an unsafe-string canary. LTO also exposed and fixed an inconsistent OpenAL sample class layout. |
| R-14 | Shared bounded wrapping/cell sampling handles exact multiples, negative fractions and distant coordinates; large color/road/surface/height bodies moved out of render.h. |
| R-15 | Single CMake project version generates runtime header, appdata, package metadata and README badge output; DEB/ZIP/AppImage/NSIS use a shared install layout and existing license exclusions. Windows packages include resolved non-system DLLs. |
| R-16 | Separately compiled Core, Platform, Renderer, UI, Audio, World and Sim; typed application state; UI primitives, audio application logic and state dispatch separated; transitive layer guard runs at configure/build even with testing disabled. |

Default launch still shows the legacy TrackRS demo with Ahead available. Set
`data/policy/edition.json` to `full` or `dev` for the corresponding installed-content
policy; no C++ edit is needed. The program version remains 0.6.7 during this
staged migration. This is a foundation update, not a graphics upgrade.

## Hash preservation and explained changes

Original `tests/baselines/linux-gcc/` artifacts were not edited. All 13 pre-RNG
physics hashes and all eight comparable pre-RNG framebuffer hashes still matched
Phase 0. The separately captured ninth Phase 0 Glacier image was not part of that
eight-image repeat set.

The intentional R-13 replacement changes **13/13** rolling physics hashes. Vehicle
wheel bumps and terrain foliage no longer consume or reseed one shared libc RNG
according to render traversal. Vehicle/stage streams and tile-coordinate streams
now have explicit independent state. Legacy stage seeds derive from the virtual
path, optional decimal `stage_seed` overrides it, and validation combines `--seed`.
Cosmetic systems have separate owned streams.

Each new hash below passed two complete 1,500-tick runs (six simulated seconds).
The isolated [legacy-RNG patch](phase1/legacy-rng-probe.patch), applied only to an
experimental source copy, retains the other fixes and restores the original RNG
sites. **Every one of the 13 original hashes returns exactly.** Thus these changes
are attributable to R-13, rather than unexplained accumulator/wrapping drift.

| Stage | Original Phase 0 | Phase 1 | Cause / evidence |
| --- | --- | --- | --- |
| ahead | `8bf21a75413742fd` | `6c59e5730498f6bb` | R-13 PCG streams; original hash restored by RNG-only A/B |
| aegyptian | `8f375462ee59fca0` | `6ef69d2bfba26f27` | R-13 PCG streams; original hash restored by RNG-only A/B |
| greenland | `2cb4462eecf85584` | `804029ba668f1ffb` | R-13 PCG streams; original hash restored by RNG-only A/B |
| mudbath | `6376dec7d8f03b57` | `0aaa524773512bc5` | R-13 PCG streams; original hash restored by RNG-only A/B |
| april | `476ee9b721ac8e9e` | `c04e7703e85bb3b5` | R-13 PCG streams; original hash restored by RNG-only A/B |
| ancient | `65d15b0ed754f2f8` | `e78aa8175f104310` | R-13 PCG streams; original hash restored by RNG-only A/B |
| coastline | `e71e7d78e0f815b6` | `6c1fc7772b1256fa` | R-13 PCG streams; original hash restored by RNG-only A/B |
| country | `7ff5c5f0fe9b19f4` | `826ac4b1dfbe92d6` | R-13 PCG streams; original hash restored by RNG-only A/B |
| darkbushes | `367a39c224d54ab8` | `b6d2c008c0056040` | R-13 PCG streams; original hash restored by RNG-only A/B |
| barren | `00de95e24be1a73b` | `08fa4a1ec9780ada` | R-13 PCG streams; original hash restored by RNG-only A/B |
| altitude | `c965c6916b2d3676` | `12ad3892a6b31069` | R-13 PCG streams; original hash restored by RNG-only A/B |
| arizonachase | `c1567463257d03f3` | `722602d179d1ee7d` | R-13 PCG streams; original hash restored by RNG-only A/B |
| glacier | `5800b5cffe9ae60c` | `a348e60ebf050d09` | R-13 PCG streams; original hash restored by RNG-only A/B |

Evidence: [final repeats](phase1/final-manifest.json),
[pre-RNG repeats](phase1/pre-rng-manifest.json),
[legacy-RNG control](phase1/legacy-rng-manifest.json).
The full suite used frozen candidate `fb19301`; later runtime cleanup changes
only edition/configuration/layer boundaries. A final-head Ahead repeat compares
against the same exact 1,500-tick hash. Comparisons are GCC 14.2/Linux x86-64;
platform-specific compiler/libm equality is not claimed.

Eight final framebuffer captures also matched exactly between repeats. Their
changes from Phase 0 are expected from seeded foliage/effect placement and changed
vehicle trajectories; no art or shader upgrade is claimed.

| Ahead before | Ahead after |
| --- | --- |
| ![Phase 0 Ahead](../../tests/baselines/linux-gcc/screenshots/ahead.png) | ![Phase 1 Ahead](phase1/screenshots/ahead.png) |

## Validation

- Local GCC release and Clang 19 release: 7/7 CTest targets.
- ASan/UBSan: 8/8 CTest targets, including adversarial policy cases and a real
  TrackRS-origin leak canary that remains fatal.
- TSan: 7/7 CTest targets.
- Final ASan/UBSan soak: **705.8 wall seconds**, two stage load/replay/render/unload
  cycles, 5,000 ticks / **20 simulated seconds**, zero application errors.
- Final TSan soak: **860.0 wall seconds**, the same cycle/tick counts, zero reported
  application races. Mesa worker threads are disabled with `LP_NUM_THREADS=0`;
  no race suppression is used. The earlier uninstrumented-worker report is retained
  [verbatim](phase1/tsan-mesa-worker-report.txt).
- Both soaks target at least ten minutes of wall-clock sanitizer execution, not
  ten simulated driving minutes. Their measured domains are explicit in
  [ASan](phase1/soak-asan.json) and [TSan](phase1/soak-tsan.json).
- Native ASan exit status 23 and raw stacks remain visible. Each process accepts
  only the same audited 108,124-byte Mesa allocation origins. See the
  [Ahead audit](phase1/asan-ahead-audit.json) and [Greenland audit](phase1/asan-greenland-audit.json).
- New core/application/renderer code passes clang-tidy. Deliberate unsafe-string
  and upward-include canaries fail. Configure rejects the latter even with
  `BUILD_TESTING=OFF`; a clean no-tests configuration passes the mandatory target.
- Demo/full/dev were launched in isolated profiles against real source content;
  demo keeps locked menu entries, full/dev enter the populated event list. F12
  captures the back buffer before swap and produces nonblank images.
- ZIP and AppImage launched from `/tmp`, used their packaged relative data layout,
  loaded Ahead and produced 31 measured rendered samples. Their reports are
  [ZIP](phase1/zip-smoke.json) and [AppImage](phase1/appimage-smoke.json).
- [Runtime CI 430c25d](https://github.com/Saad-programmer/TRackRS/actions/runs/41419992572)
  passed Linux GCC/Clang release/ASan and Windows MSVC/MinGW, including DEB, ZIP,
  AppImage and NSIS creation. Final main CI is checked again before tagging.

## Measurements and limits

Three reference scenarios ran serially without concurrent build/render work on
Mesa llvmpipe (LLVM 19.1.7), at 1,250 ticks each. These are software-rendered,
instrumented measurements, not reference-GPU budget results.

| Scenario | Archived Phase 0 ms/frame | Phase 1 ms/frame | Same fixes, legacy RNG control |
| --- | ---: | ---: | ---: |
| Ahead | 22.711 | 29.742 | 30.176 |
| Greenland | 11.611 | 13.626 | 13.616 |
| Mudbath | 23.377 | 23.669 | 38.171 |

The new run is slower than the archived sample in all three scenarios. The
same-fixes legacy-RNG control restores each archived benchmark state hash, yet
also shows substantial timing variation. Changed foliage/trajectory work and
shared-host/driver timing prevent attributing these one-shot values to a specific
optimization. No renderer speedup is claimed. Raw [new reports](phase1/benchmarks/summary.json)
and [control reports](phase1/benchmarks-legacy-rng/summary.json) include submitted
primitives, CPU/GPU scopes and memory; VRAM remains unavailable.

The standalone idle-loop pacing probe measured 2,000 ms CPU without a cap versus
18.5 ms CPU over approximately two wall seconds with a 120 FPS cap: 240 frames,
8.35 ms mean interval. This measures idle pacing only, not game FPS. Probe source
is `tools/pacing_probe.cpp`; [uncapped](phase1/pacing-uncapped.json) and
[capped](phase1/pacing-capped.json) results are retained.

The compatibility renderer and stereo's existing separate eye draws remain;
instanced multiview/modern GL is outside the Phase 1 pacing work. Scene/menu
composition remains in the application, while engine modules cannot include the
higher application layer. Simulation remains single-threaded. Hardware graphics,
hardware audio, real-car calibration, long-distance driving endurance and later
phase budgets are not claimed here.

All distributions preserve the previous exclusion of 17 plugin archives marked
unlicensed, while the checkout retains its 25 archives. Existing data/license
credits and dependency notices are installed. No proprietary assets or guessed
car specifications were added. Reproduction commands and tool digests are in
[BUILDING](../BUILDING.md), [tools](../../tools/README.md) and ADR 007–010.
