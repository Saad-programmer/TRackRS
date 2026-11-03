# Phase 3b — shared sky, light, fog and cascaded shadows

The Phase 3a follow-ups and Phase 3b candidate are implemented. The release gate
is recorded by `phase-3b-done`; until that tag exists, final checks are pending.
Legacy remains the default and selectable. Phase 3c has not started.

## Phase 3a follow-ups

The nine original procedural surface sets now have a full
[CC0-1.0 license](../../data/terrain/pbr/CC0-1.0.txt) and
[per-set provenance manifest](../../data/terrain/pbr/PROVENANCE.json). Each entry
records source, project/Codex authorship, SPDX identifier, generation method,
seed, colour/roughness/normal/contrast parameters and the SHA256 of every source
PNG, compressed KTX2 and material manifest. No downloaded artwork was used.
The baker regenerates provenance. The new `terrain-license-provenance` CTest
checks all nine sets/63 files, generator freshness and complete asset coverage;
all Linux and Windows CI configurations invoke that check through CTest.

The [terrain-only near/mid/far report](phase3b/TERRAIN_DISTANCE.md) measures actual
production renderer submissions on a controlled fixture. Near: 34 draws/69,632
triangles; mid: 216/442,368; far: 144/294,912. It excludes foliage, cars, sky,
probes, shadows and UI. Distances, camera height, FOV and reproduction are stated;
these are geometry counts rather than hardware timing results.

## Delivered behavior

Select **World renderer: PBR** in Options, then Time of day and **Shadows (PBR):
CSM / Blob**. Time presets are dawn, noon, golden hour, dusk, night and midnight.
They represent game solar time, not the computer's local clock. Settings persist
as `graphics.renderer`, `timeofday`, `shadows` and `renderquality`. CLI equivalents
are `--renderer pbr --time-of-day golden-hour --shadows csm --quality high`.
Selecting Legacy retains the accepted sky, fog, lighting and blob shadow.

A single solar state supplies direction, colour, attenuated direct radiance,
twilight sky-source radiance and ambient light. Analytic Preetham/Perez daylight
uses that source; stable direction-space stars and a procedural full moon extend
it at night. The moon follows a simple opposite-sun direction, not an astronomical
ephemeris. Six time presets are provided; no post-processing auto-exposure is
introduced at this gate.

Probe captures retain linear light for both PBR and retained legacy geometry;
the main view applies its display transform separately. GPU readback checks this
domain boundary (0.22524 linear versus 0.61116 display-encoded for a known sample).

The same sky is GPU-baked into a 64² GGX-prefiltered cubemap with all mips and
an 8² diffuse irradiance/pi cube. Existing split-sum BRDF LUTs are reused.
The bake updates when solar time or sky/irradiance shader source changes. Local
car probes capture the matching environment. Rendering, IBL and height fog share
the solar source; the previous fixed sky no longer lights the new world path.

Height fog integrates exponential density analytically along the camera ray,
including the horizontal-ray limit. It uses absolute world height, density
0.003/m and falloff 0.006/m, with sun-directed in-scattering. It is applied to
linear scene lighting before tone mapping. Retained legacy meshes, foliage and
water use a world-lighting adapter; HUD/text remain outside that adapter.

CSM uses three 1024² cascades on Low/Medium and four 2048² cascades on High/Ultra.
Log/uniform split blending, fixed bounding spheres, light-space texel snapping,
a solar-path-safe light basis, slope/normal bias, 3×3 PCF, overlapping cascade
cross-fades and far-distance fade reduce seams and shimmer. Shadow reach is
512m/768m respectively. Terrain, vehicles and alpha-cutout foliage cast shadows.
A player car hidden by a first-person view still casts into CSM. Shadow-pass
material work has separate cascade timing and does not inflate main-view
per-material sample counts. The blob is suppressed only when PBR CSM is selected; PBR Blob and Legacy keep it.

## Verification

The accepted 13 simulation hashes and eight legacy RGB hashes remain unchanged;
no golden or tolerance was rewritten. Checkpoint manifests, native sanitizer
results and the exact screenshot comparison are under [phase3b](phase3b/).
CPU tests compare height fog with a numerical integral, cover cascade frusta,
check sub-texel camera stability and solar-path basis continuity. GPU readback
checks a known occluded receiver (0.0 visibility), a known lit receiver (1.0),
sky IBL readiness, and resolved stars/moon. Existing PBR white-furnace and normal
handedness checks remain enabled. Shader tests compile/link all material,
terrain, depth, sky, irradiance, Basic and particle pairs/permutations.

The integration runner compares Legacy, time presets and Blob controls using
identical replay input; it checks physics equality, dimensions, IBL readiness,
cascade calls, blob selection and High submission limits. CI runs the world
integration in all four Linux configurations; Windows runs the CPU/license tests.

## Real GPU hand-off

The automated graphics runners deliberately force software rendering. For your
hardware measurements, run the application directly from the repository root:

```sh
build/linux-release/bin/trackrs --benchmark --data-dir data --stage ahead --renderer pbr --quality high --time-of-day noon --shadows csm --width 1920 --height 1080 --frames 1500 --replay tests/replays/baseline.csv --json gpu-noon.json --screenshot gpu-noon.ppm
```

Use the Windows executable path if appropriate. Repeat with `--shadows blob`,
`--renderer legacy`, and another time preset. Use `--vehicle pbr_test` as well
to exercise PBR materials and the local car probe. The JSON includes renderer identity,
whole-frame submissions and CPU/GPU pass timings; `.world.json` includes cascade
submissions and logical shadow storage. Please retain the GPU/driver, resolution,
quality and command with results. Software measurements cannot certify the
master prompt's reference-hardware FPS, GPU-time or total VRAM budgets.
