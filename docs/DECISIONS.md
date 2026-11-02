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
