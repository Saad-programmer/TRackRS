# Phase 1 — defect purge

Status: candidate under verification. Phase 2 has not started. `main` remains at
accepted Phase 0 until the candidate's verification is complete.

Implemented: per-simulation bounded stepping and render-only body interpolation;
high-resolution timing, vsync/frame cap; portable release flags and supported LTO;
owned resources and independent PCG streams; bounded periodic map/height sampling;
data-driven edition access; in-window splash fallback; component compilation and
transitive layer checks; generated version metadata and shared CPack/AppImage layout.

R-10 remains the explicitly scheduled Phase 4 work; the snapshot inventory does
not support inventing eleven real-car meshes/specifications. See ADR 007–010.
Legacy scene composition and fixed-function GL remain application/renderer code.

Evidence already completed:

- GCC release: 7/7 CTests; Clang release: 7/7 CTests.
- ASan/UBSan: 8/8 CTests including a real application-origin leak canary.
- TSan: 7/7 CTests and a 746.7-second real-game soak, no application races.
- ASan/UBSan: 852.9-second real-game soak across Ahead/Greenland, zero application errors; only the pinned 108,124-byte Mesa origins accepted per process.
- All 13 pre-RNG physics hashes equal the untouched Phase 0 goldens.
- New core/renderer clang-tidy checks pass; a deliberate unsafe-string canary fails.
- ZIP and DEB generation pass; ZIP ran from `/tmp` using packaged content.
- Full repeated final hashes, isolated legacy-RNG A/B and distribution CI
  are still being collected; their final results will replace this candidate status.

The checked-in `phase1/legacy-rng-probe.patch` is an experiment for an isolated
source copy, never a production compatibility mode. It restores the exact libc
RNG sites while retaining the other Phase 1 fixes, to explain changed hashes.
Original `tests/baselines/linux-gcc/` files are unchanged.
