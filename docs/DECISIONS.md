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
