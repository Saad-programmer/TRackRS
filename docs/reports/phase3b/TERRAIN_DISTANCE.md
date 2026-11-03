# Terrain-only distance submissions

Actual calls to the production CDLOD Renderer::draw, High quality (1800m range),32cellsharedgrid,60degree verticalFOV,16:9aspect, camera looking at world origin. Controlled periodic16x16height fixture, not a legacy-stage or hardware-performance benchmark. These counts exclude foliage, cars, probes, sky, HUD and shadows. Each patch submits2048triangles in one draw. Camera rises with distance, so visible area changes; counts need not decrease monotonically.

|View|Ground distance m|Height m|Terrain draws|Terrain triangles|Culled nodes|
|---|---:|---:|---:|---:|---:|
|Near|32|31.2|34|69632|84|
|Mid|256|109.6|216|442368|106|
|Far|1024|378.4|144|294912|70|

Reproduce with `material_preview --test --report results.json`; see `terrain.terrain_only_distance_submissions`. Geometry counts are hardware-independent for the same inputs; GPU times must be measured on the target GPU. The full3a reports also contain real-stage main-terrain sidecars and whole-scene measurements.
