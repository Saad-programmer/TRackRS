# Phase 2a — core presentation and RHI

Implementation: `56b9cb2`, with final shared-texture ownership synchronization in
`0fce40d`, and a type-preserving debug callback cast in `70324e3`. Phase 1 audit: `00fbe0f`; pl_mpeg and pose proof:
`9d1eab2`. This report covers **2a only**. Gates 2b, 2c and 2d have not started.
The annotated `phase-2a-done` tag records the successful final main-branch CI
run; publication waits for that run to pass.

## Implemented boundary

The visible SDL window requests and checks a debug **OpenGL 3.3 core** profile
(Mesa supplied 4.5 core locally). Vendored glad 2.0.8 replaces GLEW. The RHI owns
programs, buffers and the core presentation VAO; its state cache avoids repeated
program, VAO, texture, viewport and presentation-state calls. A std140 Frame UBO
is updated and consumed by the presentation shader.

Shader programs support bounded, root-contained includes, define permutations,
and 250 ms interactive dependency polling. Compile, link or interface failures
retain the last good program. Successful reloads rebind the Frame block and
sampler. Validation disables reload to keep captures reproducible. KHR_debug is
required on both contexts; unexpected GL API errors/undefined behavior are fatal.
The integration test deliberately injects one API error to prove the callback.

Legacy meshes, particles, text, menu and HUD still draw in a **hidden shared
compatibility context**. Each eye is copied into a shared RGBA8 texture and drawn
by a core fullscreen triangle in the visible window. Screenshots read that core
framebuffer before swap. This explicit temporary bridge meets the gate ordering
and keeps the old loaders playable; immediate-mode replacement belongs to 2b.
It costs a GPU copy, synchronization and context switches. See ADR-011.

## Verification

- Release CTest: **8/8**. ASan/UBSan CTest: **9/9**, including the application-leak
  canary. Sanitizer runs require execution outside the sandbox's ptrace wrapper;
  the initial sandbox attempt could not perform LSan introspection.
- Core-only RHI integration: real GL profile, byte-exact textured output, UBO
  round trip and size validation, redundant-state avoidance, successful reload,
  invalid shader/interface rollback, included-file pixel change, define variant,
  include cycle/path escape rejection, and expected KHR_debug error canary.
- ASan/UBSan RHI: native exit 23, **108,124 bytes** from the two previously audited
  Mesa allocation origins accepted; no unexpected application errors.
- ASan/UBSan game: **1,500 ticks**, state `6c59e5730498f6bb`, native exit 23,
  **108,124 bytes** at the same audited origins; no application errors. The audit
  policy and native sanitizer fatal behavior are unchanged; no broad suppression.
- In-window intro playback and Escape skip passed through the core bridge.
  Decoder tests exercise real RGB/PCM output. Production launches no media process.
- Legacy content: **256 XML definitions** and **25 archive CRCs** pass; actual
  replay processes load all 11 vehicle definitions. Layer checks and actionlint pass.
- clang-tidy checked the new RHI/device/media production sources with no unsuppressed
  application diagnostics. CI exposed an additional callback cast diagnostic;
  `70324e3` fixes the cast directly. Third-party headers remain system headers.

The [initial full candidate CI](https://github.com/Saad-programmer/TRackRS/actions/runs/41457202959)
passed all six jobs (GCC/Clang release and ASan, Windows MSVC and MinGW).
The final annotated gate tag records CI for the final main commit, including the
synchronization guard and callback-cast correction. Linux CI runs core integration,
replay determinism and the ASan audit; release CI also exercises intro playback.

## Golden review

[Manifest](phase2a/manifest.json): all **13 state hashes** equal the accepted
Phase 1 manifest, with two 1,500-tick runs per stage. Three completed stage pairs
were retained after a process interruption; the remaining ten were resumed in a
separate invocation with the same frozen candidate binary. The 26 per-run
[replay reports](phase2a/replays/) retain ticks, hashes and rendering measurements
(their timings are not isolated benchmarks). The only later RHI
changes add shader include/permutation handling, exception-safe program cleanup,
a flushed test marker, and explicit completion of core texture sampling before
legacy overwrites; default shaders and physics are unchanged. The final binary
also passed [two Ahead runs](phase2a/final-ahead.json), matching state and RGB
hashes exactly after the synchronization guard.

All **eight captured RGB hashes** are unchanged. I reviewed every pair in the
[pixel-preserving side-by-side sheet](phase2a/images/side-by-side.png): terrain,
vehicle, sky/weather, particles, text, minimap and gauges match. The
[HTML review](phase2a/images/review.html) and [RGB comparison data](phase2a/images/review.json)
retain both images without resizing. There are **no hash changes to explain**
relative to Phase 1. Original Phase 0 baselines are untouched; their intentional
Phase 1 PCG changes remain explained in the Phase 1 report.

## Isolated software-renderer measurements

The final synchronization behavior was measured serially at 1,250 ticks per
scenario, with no concurrent build, lint or rendering workload. Earlier
non-isolated timing attempts were discarded. [Raw summaries](phase2a/benchmarks/summary.json)
retain the full measurements. The final callback-cast correction changes no GL
calls or rendering behavior.

| Stage | Phase 1 average ms | 2a average ms | 2a p99 ms |
| --- | ---: | ---: | ---: |
| ahead | 29.74 | 39.51 | 175.02 |
| greenland | 13.63 | 19.80 | 47.97 |
| mudbath | 23.67 | 36.25 | 88.27 |

All three benchmark state hashes match Phase 1. These are separate llvmpipe runs
on shared cloud hardware, not controlled GPU performance claims. The bridge
adds copy/synchronization/context-switch costs; gate 2a does not claim a speedup
or meet the eventual hardware frame budget. Existing submission counters measure
legacy calls and do not include the additional fullscreen presentation triangle.

## Limits and gate

Only software Mesa/llvmpipe was available locally. Windows CI checks compilation,
CPU tests and packaging, not a hardware-driver rendering session. Hardware
quad-buffer stereo and other GPU vendors remain unverified. A driver must support
KHR_debug and sharing between the core and compatibility contexts; startup fails
explicitly otherwise. No PBR shaders, new vertex layouts, importer, texture sets,
KTX2 loader or IBL/probe work is included. **Stop at 2a after the green main CI
run and `phase-2a-done` tag.**

The final publication also fixes the pre-existing Pages failure: Jekyll 3.10
interpreted the master prompt's literal C++ `{{...}}` initializers as Liquid.
`docs/_config.yml` disables Liquid for that file only, preserving the specification
text and normal Markdown rendering. This is a documentation build fix, not a
renderer gate expansion.
