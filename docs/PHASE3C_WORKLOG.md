# Phase 3c worklog

Base: `ec087af` (Phase 3b report complete). Branch `claude/awesome-wright-gnj0mg`.
Scope of this session: the first Phase 3c item, native instanced 3D vegetation.
Weather, water/puddles, road ribbon, night headlights and post-FX are not started.
Legacy remains the default renderer; its goldens and physics are untouched.

## Environment note

This container's toolchain (Ubuntu 24.04 GCC, Mesa llvmpipe) produces legacy
state hashes that differ from the committed `tests/baselines/linux-gcc` manifest
for the *unmodified* base commit too (ahead: `6c59e573…` here vs `8bf21a75…`
committed). Regression evidence is therefore an A/B of the base binary against
the new binary, both built here: identical state hashes and identical framebuffer
SHA-256 on all 13 stages. The committed manifest is not rewritten.

## Item 1: instanced 3D vegetation

- CPU module `engine/vegetation/vegetation.{h,cpp}`: catalog, procedural meshes
  (6 forms x 3 LODs), cards, LOD choice, classifier. 7 doctest cases.
- Tests caught three real defects before integration: broadleaf LOD2 floated
  (no trunk), conifer/cactus undersides shared top normals (inward lighting), and
  fade-to-nothing instances were double-counted.
- First GPU run failed with `GL_INVALID_OPERATION` on every draw: `uLocalProbe`
  (samplerCube) defaulted to texture unit 0 alongside the 2D albedo sampler. Bound
  it to unit 7 like terrain. Pass scopes must be pre-registered in `main.cpp`.
- Visual review against the card control led to: per-sprite silhouette width,
  bright-half colour statistic, sky-bent foliage normals, thinner fanned grass,
  thin palm trunks with wide drooping fronds (2-step minimum so far LOD fronds
  are not wedges), lighter conifer undersides.
- Strict clang-tidy (production lint set) clean after adding `bufferOffset` and
  scoped NOLINTs for internal geometry helpers.
- Wind first used `coursetime`, which is 0 during the countdown. Added
  `TriggerGame::animationtime` (advances every tick, presentation only); world
  runner physics hash unchanged (`72d1a3ffd170d57c`), ahead/glacier legacy
  state + framebuffer re-verified identical to base with the final binary.
- Species freshness CTest falls back to a coverage-only check when Pillow is
  not installed (CI images), so colours are re-measured only where possible.

## Item 2: weather

- First rain render looked silvery: a smooth wet film everywhere mirrored the
  bright overcast dome and plants turned snow-grey. Film now keeps roughness
  >= .28 (plants .45); only puddles are mirrors. Physics hash identical for
  every preset; legacy A/B re-run on ahead, glacier, mudbath.

## Item 3: headlights

- First aim (5 degrees down) lit only a few metres; changed to ~1 degree with a
  longer range. Golden-hour storm test exposed that daylight alone is a poor
  switch; rule now also uses the cloud-attenuated beam.

## Item 4: water

- First grid test failed because the test camera nudge crossed a cell boundary
  (test bug, not snapping). Start cameras on most water stages do not face the
  water; swamp and mud do and were used for review.

Next Phase 3c item (not started): road ribbon/decals. Real-GPU timing
of `--vegetation 3d` vs `cards` is requested from the maintainer.
