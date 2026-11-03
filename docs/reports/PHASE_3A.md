# Phase 3a — selectable CDLOD terrain

The car-probe quality prerequisite and terrain portion of R-05 are implemented.
Legacy remains the default. Phase 3b, the vegetation portion of R-05, and R-06
sky/shadow work have not started. The final gate is recorded by `phase-3a-done`;
until that tag exists this is a candidate report.

## Running it

In Options select **World renderer: PBR**, then Render quality. Selecting Legacy
restores the accepted terrain path. Settings persist as `graphics.renderer` and
`graphics.renderquality`. Validation also accepts `--renderer legacy|pbr`,
`--quality low|medium|high|ultra`, and `--width`/`--height`. Medium remains the
default quality. Car probes use 32² on Low/Medium and 128² on High/Ultra;
[measured probe costs](phase3a/PROBE_QUALITY.md) include logical storage.

## Terrain delivered

- Shared 32-cell indexed grid, CPU CDLOD selection and conservative six-plane
  frustum culling. GPU parent-grid morphing joins adjacent levels without skirts.
  The eight-patch-span morph range is checked by exhaustive shared-edge tests;
  the tighter four-span experiment failed and was rejected.
- GPU periodic R32F height sampling, triangular interpolation and central-difference
  normals. Physics still reads the original CPU heightmap through `wrapCoord`.
  [Source comparison](phase3a/checkpoints/physics-source.json) proves the physics
  height/contact/surface functions and wrapping header unchanged from Phase 2d.
- Average O(1) hash/LRU caches: 128 legacy foliage tiles and 2,048 CDLOD metadata
  entries. One shared 21,000-byte grid replaces per-patch GPU geometry.
- Nine original CC0 PBR surface sets in mipmapped KTX2 arrays; bilinear categorical
  terrain-type splats, world-space triplanar detail normals and macro variation.
  Existing terrain classes map to tarmac, gravel, dirt, grass, sand, snow, ice,
  mud and water. Missing/unknown maps infer snow/grass/sand from the colour map,
  otherwise dirt. Type-map physics is unchanged.
- Existing baked sky IBL and fixed lighting remain. Retained foliage cards and
  blob shadows are intentionally still present until their later gates.
  Probe captures use coarser nearby terrain/foliage and refresh at most once per
  frame, retaining the default 500 ms interval and API minimum 250 ms.

## Verification and limits

All 13 accepted 1,500-tick simulation hashes and all eight legacy RGB hashes are
unchanged: [manifest](phase3a/manifest.json), [exact screenshot review](phase3a/legacy-review/).
No tolerance or golden was changed. All 13 stages also pass paired Legacy/PBR
120-tick loads with identical simulation state, plus a High PBR-car probe case:
[functional results](phase3a/functional/result.json).

Release CPU 13/13 and native ASan/UBSan CPU 14/14 pass. GPU checks cover periodic
normals (maximum error 1.2e-7), 10,756 covered transition pixels, CPU shared-edge
coincidence, nine surface colours/roughnesses, native/fallback array uploads,
existing white-furnace and normal-handedness tests. All 23 shader combinations
compile/link and all 68 Khronos/material checks pass. Native graphics sanitizer
results and production lint are preserved in [checkpoints](phase3a/checkpoints/).
The allocation-origin LSan policy is unchanged.

llvmpipe/softpipe decode BPTC once on the texture worker; hardware drivers retain
native uploads. Direct native-codec integration tests still run. The controlled
32-tick [comparison](phase3a/texture-policy/) preserves physics and geometry while
reducing software frame mean from 2,516.81 to 183.18 ms. This increases logical
surface-array storage from 148,284 to 442,692 bytes and environment storage from
417,664 to 890,224 bytes. These are payload counts, excluding driver padding.
Software timings do not establish reference-GPU compliance.

[Final 1080p comparisons and performance/budget tables](phase3a/benchmark/PERFORMANCE.md)
include raw per-pass measurements and reviewed side-by-side images. New terrain
materials intentionally change appearance; exact image preservation applies to
the selectable legacy path.

A [full moving 1,500-tick Ahead pair](phase3a/moving/PERFORMANCE.md) also preserves
the accepted `6c59e5730498f6bb` state. Its matching-tick images were inspected:
car/HUD/particles and terrain are present, with the intentional new terrain
appearance. Mean draws/triangles fall from 217/477,008 to 198/260,663; software
frame time rises from 67.70 to 259.35 ms. This is a geometry improvement and a
software timing regression, not a claim of hardware frame-budget compliance.

Final software-policy checks are in `checkpoints/policy-*`: both CPU suites,
strict lint, native ASan/UBSan PBR/IBL and terrain integration pass. Validation
explicitly disables automatic video mode before applying dimensions; release
and sanitized 640×480 captures pass on a deliberately different 1024×768
desktop (`viewport-release.json`, `policy-terrain-asan.json`).
