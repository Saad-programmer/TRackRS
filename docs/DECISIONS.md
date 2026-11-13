# Engineering decisions

## ADR-001 — Gate the Reality migration with executable baselines
Context: the supplied roadmap replaces most engine systems and requires phase gates.
Decision: implement and verify Phase 0 before attempting later phases.
Consequences: existing compatibility rendering, gameplay and content remain in place.
Evidence: reports distinguish executed checks from CI definitions and outstanding gates.
Status: accepted; no phase-completion tag until every required gate passes.

## ADR-002 — Use the existing simulation and renderer in the safety harness
Context: a separate toy physics runner cannot detect legacy regressions.
Decision: opt-in CLI drives MainApp, TriggerGame and PSim at 250 Hz under a real GL context.
Consequences: headless Linux requires Xvfb; one scenario per process isolates legacy statics.
Evidence: trajectory hashes cover body position, orientation and linear/angular velocity each tick.
Status: accepted; compiler-specific hashes and missing benchmark counters are explicit.

## ADR-003 — Keep the working branch, deliver reviewed commits to main
Context: the roadmap requests reality/0.7; the user explicitly authorizes direct main pushes.
Decision: work on reality/0.7 and push verified commits to main without force.
Consequences: preserve baseline tag and refuse to overwrite concurrent remote work.
Evidence: compare remote main before publication and report the exact pushed commit.
Status: accepted.

## ADR-004 — Preserve historical documentation rather than deleting unknown duplicates
Context: doc/ contains licensing and author records; docs/ contains a distinct website/manual.
Decision: remove only generated repository dumps; retain unique documentation and licences.
Consequences: documentation consolidation is deferred until links and attribution can migrate safely.
Evidence: no tracked dependency files exist in this checkout despite the brief's older count.
Status: accepted.

## ADR-005 — Make CMake startup compatible with existing profiles
Context: old profiles use ../data; CMake outputs the executable deeper in build/.
Decision: try configured content first, then the checkout data only when none mounted.
Consequences: valid custom data keeps precedence; the legacy Make build is unchanged.
Evidence: normal interactive menu launch with the existing 0.6.x profile succeeded.
Status: accepted.

## ADR-006 — Audit driver allocation origins without hiding application leaks
Context: native LSan matches any frame; full Mesa stacks include TrackRS callers.
Decision: use lsan.supp as an exact driver-build/allocation-PC origin policy in tests/run_asan.py, never as native suppressions.
Consequences: the first post-allocator frame must be the audited system Mesa module; callbacks, unknown origins and other sanitizer errors fail.
Evidence: retain full unsuppressed logs/status, verify all reported byte/object totals, and test real application leaks plus driver-callback fixtures.
Status: accepted by the user; Mesa-origin allocations may have TrackRS caller frames.

## ADR 007 — Phase 1 scope and the R-10 inventory mismatch

Context: The Phase 1 list names R-07/08/09/11/12/13/14/15/16; Car Definition v2 (R-10) belongs to Phase 4.
Decision: Audit R-10 against the actual 11-vehicle snapshot, preserve its existing definitions, and implement the explicit Phase 1 list.
Reason: This checkout has nine fictional cars and two additional definitions, not eleven missing real-car asset sets; real specifications cannot be fabricated.
Consequence: Phase 1 does not claim calibrated real-car physics or introduce Phase 2/4 systems. The existing asset inventory remains authoritative.

## ADR 008 — Owned PCG streams replace libc RNG coupling

Context: Terrain tile creation reseeded the process RNG and changed subsequent wheel bumps, weather and effects according to render traversal.
Decision: Use PCG32 streams owned by each vehicle, stage, particle system and application; derive independent foliage streams from stage and signed tile coordinates.
Seed policy: Legacy levels default to the stable virtual-path FNV-1a seed; optional decimal `stage_seed` overrides it. Validation XORs its explicit `--seed` into the stage seed.
Consequence: Wheel-bump samples and foliage positions intentionally change; original golden artifacts remain immutable and a legacy-RNG A/B patch isolates their effect.

## ADR 009 — Transitional engine modules preserve the compatibility renderer

Context: Engine textures and terrain included `main.h`, and configuration downcast its host to MainApp; UI primitives shared the application menu source.
Decision: Compile Core, Platform, Renderer, UI, Audio, World and Sim separately; expose borrowed engine services and typed application states; enforce transitive engine-to-game include boundaries.
Consequence: Scene and menu composition remain application responsibilities. The fixed-function draw path remains until Phase 2; this phase makes no modern-renderer claim.
Validation: The layering check runs at configure and every game build, and as CTest; CPU-only primitives remain independent of SDL/GL.

## ADR 010 — Sanitizer and package evidence must state their limits

Context: The uninstrumented Mesa llvmpipe worker mutex produced a native TSan report; rootless SDK packages are not registered in dpkg's installed-package database.
Decision: Run TSan's real-game soak with `LP_NUM_THREADS=0`, without race suppressions; keep native ASan origin auditing unchanged. Add stable SDL_image/ALUT/PhysFS runtime package dependencies alongside shlibdeps.
Consequence: TrackRS threads remain instrumented. Soak reports distinguish wall time from fixed-step simulated time; no performance claim is made from concurrent software-rendered verification.
Packaging: All formats share the existing unlicensed-plugin exclusion list, generated project version and relative data layout; AppImage tools and runtime are checksum-pinned.

## ADR-011: staged core presentation bridge (Phase 2a)

The gates require core context/RHI infrastructure before immediate-mode migration,
while legacy content must remain playable at every gate. The visible SDL window
therefore requests a debug GL 3.3 core context and verifies its actual profile.
A hidden shared compatibility context temporarily renders the existing scene;
one GPU copy and a core shader/UBO pass present each eye. Captures read the core
framebuffer. This adds synchronization/context-switch overhead and requires
KHR_debug plus core/compatibility sharing. No silent fallback is allowed.

The alternative of running immediate mode in the visible context would not meet
the core-context gate. Rewriting geometry now would cross the 2b gate. Retire the
bridge during 2b after content and side-by-side screenshot review. No PBR, KTX2,
IBL or mesh-importer work belongs to 2a.

## ADR-012: retire the bridge with a basic core renderer (Phase 2b)

All game drawing now uses one GL 3.3 core context and a core-only glad loader.
Retained meshes and indexed terrain use explicit vertex attributes. Dynamic UI
vertices batch in order with CPU-baked transforms; original font atlas alpha is
converted to a signed-distance field and reconstructed in the shader. The original
RGB outline/metrics remain intact. Particle billboards, rain gradients and snow
use a fenced three-slot instance ring. No compatibility driver entry points or
context-sharing bridge remain.

The basic shader intentionally reproduces vertex lighting, fog, alpha cutoff and
terrain detail sampling. It is not a PBR/material-importer implementation; those
belong to 2c. Physics/input/RNG paths are unchanged. State hashes have zero
tolerance; image tolerances were declared before full comparison to permit SDF
edge reconstruction and GL arithmetic rounding while rejecting scene regressions.

## ADR-013: opt-in PBR materials and reference mesh processing (Phase 2c)

Keep the accepted legacy OBJ/ASE loader and basic shader for existing content.
Imported materials opt into PBR only with base-color, tangent-normal and packed
metallic/roughness textures. Partial texture sets retain the legacy path. The
fictional CC0 car and sphere preview prove the new path without replacing any
legacy material or starting the Phase 2d texture/IBL work.

Reference MikkTSpace runs on triangle corners before meshoptimizer deduplication,
cache, overdraw and fetch passes; mirrored UV seams and colors are retained.
Vertex layouts have fixed 56-byte/32-byte ABIs and creation-time VAO setup. Tangents
use the model linear transform, normals its inverse transpose, and bitangent
reconstruction includes both UV handedness and mirrored-model parity.

Native UBSan found shift-by-32 in two MikkTSpace pivot rotations. Keep the pinned
reference source and a marked, minimal shift-count patch, with its diff and
license/provenance included. Do not disable instrumentation or suppress the error.

The Appendix A GGX denominator uses stable arithmetic and a lower bound instead
of an additive epsilon that changes normalization at roughness 0.04. The furnace
integrates that shared production BRDF, using the PDF at the reflected light
sample; reciprocity and a mirrored-UV sphere are tested on the GPU. Full IBL furnace
validation remains at the 2d gate. CSM is a tested borrowed-resource interface,
not a cascade generator in this gate.

### Phase 2d: native BC KTX2 caches and sky IBL

KTX2 files carry native BC7 sRGB albedo, BC5 linear normals, BC7 linear ORM
(AO/R, roughness/G, metallic/B), BC4 scalar data, BC6H unsigned HDR cubes, or
RG16F integration LUTs. They are not Basis Universal payloads and do not claim
the glTF KHR_texture_basisu extension. Standard glTF images retain PNG fallbacks;
optional image extras.trackrsKtx2 selects the native cache for TrackRS. Every
level is required and colour-space/role validation runs before GPU publication.

Native BPTC is used when available. Otherwise an independent pinned bcdec CPU
decoder produces portable uncompressed uploads. Three fenced PBO slots publish
complete textures only after a zero-timeout fence poll; placeholders remain
valid meanwhile. Anisotropy applies to surface textures, not convolution cubes
or integration LUTs. Diffuse IBL samples the already-convolved base level
explicitly, avoiding derivative-dependent sampling of irradiance.

The original analytic sky is baked deterministically to diffuse irradiance/pi,
a full GGX roughness chain and a 256-square RG16F split-sum LUT. Multiple-scatter
energy compensation preserves a white rough conductor in the production shader
furnace. The local car probe captures six low-resolution faces and convolves
all roughness levels; its default refresh interval is 500 ms and the API rejects
intervals below 250 ms. No Phase 3 scheduling or LOD implementation is implied.

Opaque/masked primitives are combined by material at GPU creation; blended
primitives retain source order. Existing PNG/OBJ legacy rendering remains the
default unless an asset supplies PBR maps. GPU timestamps and logical storage
counts distinguish software-renderer measurements from hardware performance
claims and exclude driver allocation padding.

## Phase 3a: opt-in CDLOD and periodic GPU terrain

Keep the old renderer selectable and default until Phase 3d. Copy the original
height field to R32F for rendering; leave CPU physics interpolation, contacts,
terrain categories and wrapCoord unchanged. Shared indexed grids morph onto the
parent lattice, with radius eight patch spans and refinement at half that range.
Parent subdivision includes every child quadrant: the relevant bound is twice
sqrt(2) fine-patch spans. A four-span range failed edge tests and was rejected.
CPU edge coincidence and GPU raster coverage are both required; coverage alone
can conceal overlaps. Hash/list LRU bounds metadata and retained foliage tiles.

Nine KTX2 texture-array layers reuse the existing PBR BRDF; explicit gradients
outside categorical branches keep mip selection defined. Existing sky/foliage
remain until their own gates. Software Mesa decodes BPTC on the worker once;
hardware keeps native compression. Reports distinguish logical decoded/native
payload storage and software timings from hardware budgets. Probe resolution is
quality-scaled, and a frame token prevents body/wheel draws from scheduling two
refreshes in a slow frame while preserving the time-based rate limit.

## Phase 3b: one solar source and selectable stabilised CSM

Use one world solar state for direct lighting, procedural sky/IBL, fog and shadow
orientation. Preetham/Perez daylight (Preetham, Shirley and Smits, 1999,
"A Practical Analytic Model for Daylight") supplies an analytic sky distribution;
its source colour/intensity includes twilight so the attenuated direct beam can
vanish while the sky still illuminates dusk. Night uses procedural stars and a
simple full moon. These are equations and original shader code, not imported art.

Rebake sky-derived diffuse and GGX IBL on solar-preset/source changes, and reuse
the existing split-sum LUT and local-probe machinery. This is an explicit preset
update cost, not a continuous every-frame bake. The height-fog line integral has
a series limit near horizontal rays to avoid cancellation. Legacy remains a
separate selectable appearance, including its accepted image hashes.

CSM uses conservative split spheres, texel-snapped centers and an up axis that
never becomes parallel to this solar trajectory. Test both camera sub-texel
stability and whole-day orientation continuity: changing up axes near the solar
zenith would otherwise cause an abrupt basis flip. Retain alpha cutouts in the
caster pass, cross-fade overlapping cascades, and fade distant shadows. Keep the
blob when CSM is not selected. Report nominal depth payload separately from
unknown driver padding/physical allocation, and distinguish software pass times
from reference-hardware compliance. No Phase 3c vegetation/weather work is implied.

## Phase 3c (item 1): native instanced 3D vegetation

Context: PBR foliage was still the legacy crossed sprite cards; the brief asks for
instanced 3D plants with LODs, impostors, wind and alpha-to-coverage, without
unlicensed art or changes to foliage collisions. Decision: generate original
low-poly plants procedurally in code (conifer, broadleaf, bush, grass, palm,
cactus; three mesh LODs each) and keep the band's own sprite as the instanced
card impostor (LOD3). The existing deterministic per-tile PTerrainFoliage
instances are read in place, so placement, rigidity and contacts are unchanged.
`data/vegetation/species.json` maps every shipped sprite to a form, its measured
bright-half colour and silhouette width; 23 bare/sparse/silhouette sprites stay
cards at all distances rather than receiving an unfaithful form. LOD changes use
complementary screen-door dithering (no blending, no sorting, no popping). Grass
is excluded from CSM casters and probes; probes use impostors only. Consequence:
3D vegetation is the PBR default (`graphics.vegetation`, `--vegetation 3d|cards`);
Legacy rendering, legacy goldens and physics hashes are untouched. Weather, water,
road ribbon and post-FX remain later Phase 3 items.

## Phase 3c (item 2): stage weather as presentation state

Context: stages already carry rain/snowfall/fog, but the PBR world ignored them.
Decision: derive one bounded `Weather` (wetness, puddles, snow, overcast, fog
density) from the stage, overridable by a player preset; apply it in the shared
shaders in world space; attenuate the solar beam with cloud cover and rebake sky
IBL on change. Puddles are restricted to flat road-like terrain classes. Weather
never feeds physics in this phase (grip is Phase 6). Consequence: wet/snowy/
foggy stages finally look like their data; physics and legacy goldens unchanged.

## Phase 3c (item 3): world spot lights for headlights

Decision: a small fixed array (8) of world-space spot lights in the shared scene
uniforms, evaluated in pbr.frag for every PBR draw (not a clustered list yet; the
count is tiny and bounded). Lamp placement/aim is a pure function of the body
axes; on/off depends on daylight and the cloud-attenuated beam. Depth passes skip
them. A clustered/tiled list is deferred until trackside floodlights or many cars
need more than eight lights.

## Phase 3c (item 4): analytic PBR water, sky-only reflection

Decision: a camera-snapped CPU-depth grid and an analytic-wave shader reflecting
the shared world sky IBL with Fresnel, plus sun/headlight glints, Beer-Lambert
absorption and shoreline foam. Planar reflection would re-render the scene per
water plane (cost); SSR belongs with the Phase 3d depth/colour post stack, where
it can be added to this shader as a reflection source. Terrain depth comes from
the physics heightmap so water and physics agree.

## Phase 3d (core): one HDR target, GPU-only auto-exposure

Decision: render the PBR 3D scene into RGBA16F + a depth texture, reusing the
existing linearCapture switch so every pass emits linear light, then resolve
once (adapted exposure, dual-filter bloom, ACES, gamma, vignette) before the HUD.
Exposure uses a log-luminance mip chain and a 1x1 ping-pong adaptation target, so
there is no GPU->CPU read-back stall. Adaptation is driven by the deterministic
animation clock, never wall time. The CPU reference functions are the tested
specification of the shader math. Legacy stays the default renderer; switching
the default waits for TAA/SSAO and real-GPU budget data.

## Phase 4a: Car Definition v2 bridges into the legacy physics

Context: the brief's eleven real-car `.vehicle` stubs are not in this repository
(see ASSETS_TODO), reference sites were unreachable from the build session, and
the legacy physics is not SI (wheel spin integrates torque with no inertia).
Decision:
* `car.json` (schema trackrs.car/2) is the source of truth, in SI units, with a
  validator that enforces honesty: nothing is verified without an https source,
  a provenance note otherwise, a trademark notice for real marques, and a
  fictional fallback body labelled "(placeholder model)" when there is no
  licensed mesh.
* The eleven real cars are authored as unverified seeds. Their torque curves
  come from each car's own peak figures; their gearing is synthetic and
  labelled so (traction-limited first gear, top gear at the claimed top speed).
* The adapter maps gearing exactly, and power through one constant calibrated
  from lancer_evo.
* The fictional cars keep driving from their XML, so legacy hashes do not
  change, and are converted only for reports.
* Calibration is report-only until Phase 6 replaces the drivetrain.

## Phase 4b: CarRenderer derives materials from the shipped single-atlas bodies

Context: every shipped car body is one OBJ with one material and one atlas.
There are no part names, no paint mask, no glass or light materials, and the
tail lamps are only an alpha-masked emission map (some shared between sibling
car directories via `../`). Phase 4b asks for a paint mask, glass, lights,
liveries and an interior without new or ripped assets (§2.4).
Decision:
* The renderer derives a per-texel **role map** (paint, glass, rubber, metal)
  and a **light map** (tail from emission alpha, head from bright detailed
  whites) on the CPU when the car type first loads. Thresholds live in
  `engine/car/car_assets.cpp` and are unit-tested. The heuristic is
  approximate: it can misclassify, and a misclassified texel only gets another
  material response, never missing geometry.
* **Liveries** are optional `livery.json` files (`trackrs.livery/1`). They
  recolour paint near measured source colours (keeping shading), redraw race
  numbers with an original stroke font, and fill sponsor slots with
  procedurally generated fictional logos. No real team or sponsor scheme
  appears. The 11 placeholder real cars get plain fictional schemes from
  `tools/seed_liveries.py`.
* **Geometry:**
  * 3 LODs from the meshoptimizer simplifier, chosen by projected size; the
    shadow pass draws LOD ≥ 1;
  * wheels are drawn from the physics wheel frames, which already compose
    spin, steer and suspension travel;
  * the interior is procedural, kept under a roof height measured from the
    mesh, with gauge needles rotated in the vertex shader;
  * glass is a second, blended pass.
* **Scope:** PBR world only, behind `cars` (config), `--cars on|off` and an
  options entry. The Legacy renderer path is untouched and stays the default.
  Mesh imports may reach the `/vehicles` tree (`ImportOptions::assetRoot`)
  because shipped MTLs share emission maps across sibling directories.

## M0: pack-lite reuses the legacy level format

Context: amendment §A9.2 pulls content packs forward from Phase 9, while
§4.5 requires every legacy map to keep loading.
Decision:
* A pack stage is a legacy `.level` plus images inside the pack, described by
  `stage.json`. It loads through the existing level loader, so pack stages and
  `/maps` stages behave identically. The spline-road `stage.json` of §4.4
  extends this format later without breaking it.
* `engine/content/pack` is pure C++ behind a two-function file source, so the
  game (PhysFS), the CLI (std::filesystem) and the unit tests (in-memory)
  share one validator.
* The licence audit is strict: any pack file not covered by a licensed,
  attributed asset entry disables the pack.
* Discovery never throws. Broken packs are disabled with a reason shown in
  the log, the main menu and the validation JSON.
* The legacy content is a virtual pack, `classic-legacy-stages`. Its files are
  not moved.

## M1/M2: real elevation from SRTM via Tilezen, not Copernicus GLO-30

Context: amendment A2 asks for public elevation (SRTM or Copernicus GLO-30)
with the licence recorded. From the build session:
* The Copernicus GLO-30 bucket is reachable, but its licence page is not. The
  bucket readme and the AWS registry entry only point to it, so its terms
  cannot be quoted or verified.
* Tilezen's terrarium tiles (AWS Open Data) are reachable. Over Morocco they
  are built from SRTM, and each tile names its source raster in a header.
  Tilezen's attribution document, fetched and hashed, quotes the SRTM terms:
  a U.S. Government work, no copyright claimed under Title 17, with the USGS
  requesting the credit "SRTM data courtesy of the U.S. Geological Survey".

Decision:
* Use SRTM through Tilezen, with per-tile provenance written by
  `tools/fetch_dem.py`. Stages built on it are `derived`: real elevation, but
  the route and all details on top are invented.
* `verified` stays false until the maintainer confirms the source terms.
* Copernicus stays an option once its licence can be read.

## M1: the shadow pass owns its depth range

Context: while tuning A3.6, cascade-0 visibility read 0 on every terrain
fragment, on legacy stages as well as desert ones. The scene pass leaves
`glDepthRange(0, .999)` set (the sky is drawn at the back), so the CSM pass
stored every depth 0.1% closer to the light than its receivers. That is
several bias widths in every cascade. Bright scenes made it obvious: the
noon dry lake rendered blue-grey.
Decision: `world::renderShadows` sets `glDepthRange(0, 1)` for the cascades
and restores the caller's range on exit. Legacy-renderer pixels and hashes
are untouched, because the pass only runs with PBR terrain and CSM on. In
the PBR path, terrain under the sun is lit again, and shadows fall only
behind real casters.

## M1: persistent tracks are a visual-only overlay

Context: amendment A3.3 says to "reuse the tyre-mark/rut system", but TrackRS
has none. The legacy game only throws dirt particles.
Decision:
* `engine/terrain/tracks` is a pure-C++ ring of ground segments (32768,
  about 2.3 MB) fed once per simulation tick from each wheel's contact. Rock,
  hamada stone, tarmac and ice take no mark. Static camel and foot trails
  come from the stage's `props.json` `"tracks"` array and are never
  overwritten.
* `tracks_renderer` draws the segments as ground ribbons. Their fragment
  shader rebuilds the rut, lip and print relief and multiplies the already
  lit terrain by the relief's shading ratio. Albedo is shared, so no material
  lookup is needed, and the ribbons cost two draws. Only new ring slots are
  uploaded.
* Ruts are visual only. Neither physics nor the state hash reads them, so
  legacy determinism is unaffected. Wheels following earlier ruts (rut
  steering) are left for a later physics phase and recorded as not done.

## M1: desert cascades reach further

Context: A3.6 asks for CSM tuned for long views. Desert stages are seen to
the horizon, with dunes and ksar walls kilometres away.
Decision: when the level climate has heat haze (`heat > 0`), the cascade
reach is ×1.6 (819 m medium, 1229 m high). The logarithmic split keeps the
near cascade tight. Other stages keep 512 and 768 m.

## M2: bilingual signs are baked text in an atlas

Context: M2 needs bilingual (French/Arabic) signage, but the engine has no
runtime text shaping for Arabic.
Decision: `tools/bake_sahara_signs.py` shapes the text offline (PIL with
raqm) from the OFL Noto fonts in `tools/fonts/`, and bakes a 4×2 atlas plus
`signs.json` listing every text with `verified:false`. A `sign_board` prop in
`props.json` picks a cell with `"face"`. The props renderer draws the face
through the same lit PBR path, using the atlas as albedo. Runtime Arabic UI
text is still M4 (A6, string tables and RTL).

## M2: rally-raid cars are Car v2 placeholders in the pack

Context: M2 needs two rally-raid cars, but there are no rally-raid models yet.
Decision: `draa_raid_t1` and `erg_lite_t3` are `trackrs.car/2` definitions in
`morocco-sahara/vehicles/`. Every figure is invented, `verified` is all
false, and the provenance says so. They borrow the fictional legacy bodies
`cordo_super500` and `fox_kitcar` as placeholder models, with fictional
colour liveries. The game lists `car.json` files of enabled packs, and the
harness accepts `--vehicle <pack>:<car>`. Long-travel suspension is not
modelled, because the borrowed body keeps its legacy suspension (Phase 6).

## M2: the cockpit camera draws a placeholder dashboard

Context: the M2 gate asks for a cockpit capture showing the gauges, but no
car has an interior model.
Decision: `CameraMode::cockpit` places the eye at the left seat, rigid to the
body. In that mode the HUD draws a screen-space dashboard: dash and
binnacle, tachometer, speedometer, a steering-wheel rim that turns with the
input, and a rally-raid tripmaster (distance and CAP heading). It is labelled
a placeholder until car interiors exist.

## 4c: damage visuals read the existing damage model, with a visual gain

Context: Phase 4c asks for dents, decals, detachable parts, cracked glass and
smoke, "all driven by the existing damage model". That model (PDamage) holds
four corner values fed by contact impulses × 1e-7. Measured with
`tests/replays/crash_rollover.csv` (14 flat-out flicks and rolls on
`ahead`), the worst corner reached only 0.094.
Decision:
* **Read only.** Physics and state hashes do not change; the renderer reads
  through a new side-effect-free `PDamage::level()`, since `getDamage(side)`
  clears the HUD flash.
* **Visual gain.** Visual severity is `clamp(6 × simulation damage, 0, 1)`
  (`trackrs::car::VisualGain`), so a violent session reads as heavily
  damaged. Reports carry both `damage_sim` and `damage_visual`.
* **One damage field.** `engine/car/damage_visual` is a unit-tested
  function: a smooth falloff from each corner, strongest corner wins. It is
  mirrored by `car.vert` and `pbr.frag`. Lamps on a corner ≥ 0.8 go dark,
  and a front above 0.55 smokes.
* **No separate part meshes.** The borrowed fictional bodies have no
  separate parts, so "detachable parts" is a sagging bumper end at the
  heaviest damage. Real detachable parts need segmented models (4d sourcing).
* **Paint damage before dirt,** so the 4b dust and mud carry over and dirt
  clings to the scratches.
* **Smoke only in the PBR world,** with its own RNG stream, so Legacy output
  and dirt particles are byte-identical.
* **Forced captures.** `--damage-visual` forces render-only severity for
  evidence and is reported as `damage_forced`.

## 4c: livery art never repaints glass, rubber or tail lamps

Context: fixing the M2 cars' legacy lettering showed that a sponsor rectangle
can overlap mudflap or window UVs in a borrowed atlas. It also showed that
the role map, classified from the original atlas, kept white lettering as
metal or headlamp candidates under new artwork.
Decision:
* Sponsor slots may take an opaque `backing`.
* Panel and slot art is restored on glass, rubber and tail-lamp texels.
* `repaintRoles()` marks the remaining art texels as plain paint and clears
  their headlamp-candidate flag. Headlamp candidates are not protected: the
  shader already limits them to the car's front.

## 4d: class packs carry class-typical placeholder figures, not recalled specs

Context: 4d asks for class packs (rally-raid first, then heritage raid, then
the existing rally roster) with full licence and verified metadata, and no
invented sources. No reference for these cars was consulted in this
session.
Decision:
* Each class-pack car names a real car (with a trademark notice), but its
  physics figures are **class-typical placeholders for its class and era**,
  and its provenance says exactly that. This is stricter than the 4a seeds,
  which were labelled "recalled".
* Every `verified` key is false and `sources` is empty. A supplied mesh does
  not verify any figure.
* Electric rally-raid prototypes are left out: single-speed drive with a
  range extender needs the Phase 6 powertrain, and the validator's 3–8
  gears would misrepresent them.
* 37-inch tyres exceed the validator's 0.45 m radius, so those cars use
  0.45 m.
* `tools/seed_class_packs.py` generates the packs, and CTest checks they are
  fresh, as `seed_real_cars.py` does for 4a.

## 4d: the existing rally roster stays in data/vehicles

Context: the third class pack is the existing roster (11 fictional legacy
cars and the 11 real-car entries from 4a).
Decision: the cars stay where they are. `/vehicles/<id>` paths are part of
the golden replays and hashes, of the calibration tests and of saved
profiles. The roster is documented as a class (in `CARS.md` and
`ASSET_SOURCING.md`) and keeps its metadata in each `car.json` or `.vehicle`.
Moving it into a pack would only rename paths.

## 4d: a mesh checker before any mesh loader

Context: no car mesh exists yet, and the Car v2 loader still refuses own
meshes.
Decision: `docs/ASSET_SOURCING.md` defines the exact mesh contract, and
`tools/check_car_mesh.py` (stdlib only) enforces it on a `.glb`, so a
supplied asset can be checked before the loader for supplied meshes is
written. Separate part nodes (bumpers, mirrors, wheels) are required, so
that real detachable parts (Phase 4c follow-up) become possible. The loader
is written when the first conforming mesh arrives.

## 4d: cited values enter through data_overrides.json, per value

Context: the maintainer accepted class-typical figures with `verified:
false` for the class packs, and asked for a way to verify individual values.
Decision: `tools/seed_class_packs.py` reads an optional
`vehicles/<id>/data_overrides.json` (`trackrs.car-overrides/1`). Every value
carries its own https `source`. A data group (mass, engine, gearbox, tyres,
aero, brakes) turns `verified: true` only when the file covers every path of
that group, so one cited figure never verifies its neighbours. The sources
join the car's `sources` list and the provenance names the overridden paths.
Unknown paths, a missing or non-https source, or a wrong schema stop the
generator. Tested by `car-data-overrides`.

## 4d: the car selection screen says when figures are approximate

Context: unverified class figures must not read as the named car's data.
Decision: the car selection screen prints "approximate class figures" under
the stats of any Car v2 car with an unverified group, and "placeholder model"
when it has no mesh. The "Engine (BHP)" line shows the car's peak power from
its `car.json` torque curve instead of the legacy empirical estimate, which
read 71 BHP for a 150 kW Land Cruiser. Legacy `.vehicle` cars keep the old
formula, so legacy content and hashes do not change.
