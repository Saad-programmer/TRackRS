# Phase 2b — core-only legacy-look rendering

Runtime implementation: `0ab5517`; shader interface protection: `4e98b5d`;
menu/removed-API regression checks: `1b860c5`; coplanar depth and batch-fence
correction: `f2fa1a7`. This gate covers **2b only**.
The annotated `phase-2b-done` tag is published only after the final main CI passes
and records that exact run. **Phase 2c has not started.**

## Rendering changes

The game creates **one OpenGL 3.3-or-later core context**. The hidden compatibility
window/context, cross-context texture copy, synchronization and fullscreen bridge
are removed. glad is regenerated for core 3.3; removed GL functions are absent
from its declarations as well as production calls. KHR_debug remains mandatory.

- Vehicle/model geometry uploads once into retained VAO/VBO meshes. Terrain,
  foliage and signs use explicit core attributes and indexed GPU buffers.
  PVBuffer is now a move-only GPU owner; the obsolete CPU buffer fallback is gone.
- Dynamic scene, menu, co-driver and HUD geometry is tessellated into ordered
  CPU vertex batches, then submitted through a streaming VBO/VAO. Depth-sensitive
  shadow/water geometry uses raw vertices and the GPU transform, matching terrain
  depth arithmetic. CPU transform
  stacks are ordinary math. Per-vertex baked transforms/colors permit batching
  across UI transforms without driver matrix/color state. Texture, raster or
  shader-state changes drain batches in order; these are not renamed GL calls.
- Text derives a signed-distance alpha field from the original licensed atlas
  at load time and reconstructs coverage with shader derivatives. Glyph positions,
  advances, original RGB outline and the existing font assets are preserved.
- Dust, rain and snow use instanced draws and a **three-slot fenced instance
  buffer ring**. Reuse waits for the previous GPU reader; mapped uploads are
  bounded. Billboard bases, rain color gradients, point snow and textured-snow
  UV orientation preserve the old geometry.
- The basic GLSL 330 shader reproduces directional vertex lighting/specular,
  exponential fog, alpha cutoff, planar UV mapping and signed-add terrain detail.
  Original depth/blend ordering remains. It adds no PBR behavior.
- Shader reload retains the last good program on syntax/link/Frame-UBO-interface
  errors. An actual 64-byte Frame UBO is consumed by both mesh/UI and particle
  programs. The old identity shaders now live only in RHI test fixtures.

## Verification

Release CTest: **9/9**. Native ASan/UBSan CTest: **10/10**, including the
application-allocation leak canary and exact-origin audit-policy tests.
Core GPU integration passes in release and ASan/UBSan:

- UI batches combine different colors/transforms into one actual draw, with
  readback verifying both halves; retained mesh output is checked separately.
- Readback verifies directional lighting, exponential fog and SDF coverage.
  The SDF test observes blended RGB, because the default test framebuffer does
  not guarantee an alpha attachment.
- Ten particle-ring wraps verify pixels, instanced draw counts and triangle
  multiplicity. Basic shader UBO-contract rejection and reload recovery retain
  the correct pixels. KHR_debug's intentional API-error canary remains fatal.
- The real 1,500-tick ASan game replay returns `6c59e5730498f6bb` with no
  application errors. Native exit 23 contains only **108,124 bytes** from the
  same two audited Mesa allocation origins. The policy is unchanged; it is
  not a native any-frame suppression and unknown/TrackRS-origin leaks fail.
- Intro playback and Escape skip pass in the real window. Demo/full/dev menu
  startup, event-list navigation, F12 capture and clean exit pass. Their
  [side-by-side review](phase2b/menu-review/side-by-side.png) preserves locks,
  event labels and text layout. Animated backgrounds/build timestamps differ;
  these interactive captures are not deterministic pixel goldens.
- The removed-entry-point scan covers `src`, `engine` and generated glad.
  It rejects immediate mode, driver matrix/light/fog/material/texture environment,
  alpha test, client arrays, display lists and compatibility-context creation.
  Core query/vertex-attribute APIs are intentionally allowed.
- Layer checks, actionlint and new production clang-tidy checks pass.
  Legacy loading still parses **256 XML definitions** and validates **25 archive
  CRCs**; real replay processes load the existing vehicle definitions/assets.

The [runtime candidate CI](https://github.com/Saad-programmer/TRackRS/actions/runs/41501085007)
passed all six Linux/Windows jobs, including MSVC and MinGW packaging. An earlier
MinGW job failed in the unchanged MSYS2 setup step; the subsequent full run passed.
The test-only `9af6227` follow-up makes the menu helper select null audio and
software GL explicitly, independently of SDK environment variables, and adds
focused CI diagnostics. Final-main CI evidence is recorded in the annotated gate
tag.

## Physics and screenshot acceptance

All **13 final physics hashes** match the accepted Phase 1/2a values exactly.
[Manifest](phase2b/manifest.json) and [per-stage reports](phase2b/replays/) retain
1,500 ticks and 1,499 measured frames per stage. The authoritative final suite
uses one exact-baseline comparison per stage after the shared shader/depth fix;
earlier paired probes were superseded. Country also has a separate targeted
post-fix replay, and CI repeats Ahead for determinism.

All **eight final screenshots** pass every declared threshold. I reviewed all
pairs [side by side](phase2b/images/side-by-side.png), including terrain, cars,
particles, sky/fog, text, map and gauges. [Full RGB hashes and metrics](phase2b/images/review.json)
record the expected framebuffer byte-hash changes from SDF edges and core
rounding. Maximum observed RGB mean error is **0.1705/255**;
minimum block SSIM is **0.999078**.

Country initially failed with 1.503% of pixels exceeding 24 levels and p99 error
27; [the failed result](phase2b/country-before-fix.json) is retained, along with [its image](phase2b/country-before-fix.png). The projected
shadow was CPU-transformed while its coplanar terrain used a GPU transform,
changing depth rounding. Depth-sensitive world streams now use the same GPU
matrix arithmetic; UI remains CPU-baked for batching. The original tolerance
was not relaxed. A GPU regression test verifies coplanar shadow/terrain depth,
and another verifies batch drains before draw-buffer switches.

The limits were declared before full image measurements in
[tolerance.json](phase2b/tolerance.json). Every captured image must pass **all**:

| Metric | Limit |
| --- | ---: |
| RGB mean absolute error, 0–255 | ≤ 2.0 |
| 99th percentile of per-pixel maximum channel error | ≤ 24 |
| Fraction of pixels with any channel error > 24 | ≤ 1% |
| Mean non-overlapping 8×8 luminance SSIM | ≥ 0.985 |
| RGB mean error in top 24% plus bottom 35% HUD regions | ≤ 3.5 |

SDF edge reconstruction and core arithmetic rounding can change RGB bytes.
Physics hashes have **zero tolerance**. Original Phase 0/1/2a baselines are not
rewritten. Full before/after RGB hashes, metrics and images are retained for review.

## Submission and triangle counts

| Stage / vehicle | Before submissions + bridge | After submissions | Reduction | Before triangles + bridge | After triangles |
| --- | ---: | ---: | ---: | ---: | ---: |
| ahead / cordo_kitcar | 336.30 | 217.23 | 35.4% | 477008.83 | 477007.83 |
| aegyptian / evo_wrc | 267.40 | 123.18 | 53.9% | 465473.90 | 465472.90 |
| greenland / fox_super500 | 209.94 | 114.81 | 45.3% | 122640.83 | 122639.83 |
| mudbath / cordo_kitcar | 779.61 | 182.91 | 76.5% | 280410.95 | 280409.95 |
| april / evo_wrc | 2476.30 | 141.88 | 94.3% | 122250.88 | 122249.88 |
| ancient / fox_super500 | 229.92 | 93.12 | 59.5% | 115712.76 | 115711.76 |
| coastline / cordo_kitcar | 361.37 | 255.89 | 29.2% | 146540.08 | 146539.08 |
| country / evo_wrc | 335.39 | 148.83 | 55.6% | 135796.47 | 135795.47 |
| darkbushes / fox_super500 | 320.72 | 248.62 | 22.5% | 534081.30 | 534080.30 |
| barren / cordo_kitcar | 330.90 | 243.87 | 26.3% | 485989.18 | 485988.18 |
| altitude / evo_wrc | 429.93 | 260.21 | 39.5% | 447351.95 | 447350.95 |
| arizonachase / fox_super500 | 210.41 | 128.93 | 38.7% | 425654.13 | 425653.13 |
| glacier / cordo_kitcar | 243.40 | 171.89 | 29.4% | 134826.57 | 134825.57 |

[Machine-readable counts](phase2b/counts.json) retain scene and bridge components separately.
Scene triangle counts are identical in all thirteen cases; only the bridge
triangle is removed. API submission reductions range from **22.5% to 94.3%**.

Counts use the same thirteen 1,500-tick stage/vehicle scenarios as 2a, excluding
the loading frame. Before values average the two retained 2a runs; after values
come from the final exact-baseline run. The old scene instrumentation counts
legacy submission groups, including empty groups and strip degenerates; new
counts record actual `DrawArrays`/`DrawElements`/instanced API submissions with
instance multiplicity. These are application/API counts, not a claim about the
hardware driver's internal command coalescing. Capture pass fences can split
batches, so interactive UI may batch further.

The 2a raw counters excluded its presentation triangle. The table explicitly
adds **one draw and one triangle** to those before totals. 2b has no presentation
bridge. Raw replay timings include concurrent verification work and timestamp
instrumentation; they are not controlled performance benchmarks or hardware FPS
claims. This gate reports submission/triangle changes rather than a speedup.

## Limits and gate

Local rendering uses Mesa llvmpipe. Windows CI verifies both toolchains, CPU tests
and packaging; Windows hardware rendering, other GPU vendors and hardware
quad-buffer stereo remain unverified. Wide lines clamp to the driver's supported
range. KHR_debug and core 3.3 are still required; compatibility support is not.

No Appendix A PBR shader, Appendix B shipping layout, new mesh importer,
MikkTSpace, KTX2, texture-set, IBL or probe work is included. The basic MeshVertex
is only the legacy position/normal/UV adapter, not the Phase 2c packed layout.
**Stop at the 2b gate.**
