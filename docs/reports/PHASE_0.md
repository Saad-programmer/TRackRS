# Phase 0 — safety-net implementation (gate open)

Status: implementation and current-instance validation in progress. This is
not a completed 0.7 engine and no phase-completion or release tag is warranted.
Phases 1–9 have not started because the supplied operating protocol requires
the Phase 0 exit gate first.

## Changes

- Preserved the full engineering brief at `docs/CODEX_MASTER_PROMPT.md`.
- Created `reality/0.7` and local `v0.6.7-baseline` at
  `7ef9c95b6747a17ea1e39ad56ca893ae50519cbf`.
- Added C++17 CMake builds, Linux/Windows presets, separate ASan+UBSan/TSan
  configurations, and GCC/Clang/MSVC/MinGW GitHub Actions jobs.
- Added opt-in replay, stage selection, state hashing, framebuffer capture and
  benchmark output around the actual legacy engine. Interactive timing,
  controls, simulation and compatibility rendering remain in place.
- Added pinned, unchanged MIT doctest source and explicit Release-active checks.
- Removed two generated repository dumps. This checkout has no tracked `.d`
  files; original game assets and unique historical docs/licences were retained.
- Fixed the C++17 `throw()` spelling and a missing `<iostream>` include exposed
  only when debug score logging is compiled.

## Evidence recorded so far

- GCC 14.2 and Clang 19.1 release builds and unit/CLI checks passed.
- GCC ASan+UBSan and TSan builds and CPU-only unit/CLI checks passed outside the
  restricted sandbox. LeakSanitizer requires access unavailable under its ptrace
  sandbox; rerunning outside that sandbox succeeded.
- Ahead's 1,000-tick replay produced `53d7f7b88992cf4e` in both Release and the
  sanitized game; the repeated Release framebuffer matched exactly.
- The full scenario suite records fresh profiles, repeat hashes, and screenshots.
  Final scenario counts and measurements will be added after it completes.
- The sanitizer driving run completed, then failed LeakSanitizer at teardown:
  108,124 bytes / 481 allocations in `libGLX_mesa.so.0`. The minimal
  `tests/driver_probe.cpp` reproduces the same leak with no TrackRS code.
  This is a confirmed external-driver limitation; no suppressions were added.
- The inventory is 132 standalone stages, 97 event stages, 16 events, 25 plugin
  archives, and 11 vehicle definitions total. The brief's additional historical
  vehicle roster does not match this revision.

## Gate still open

- Windows jobs are configured, not yet verified on Windows.
- The final baseline/benchmark capture and code review are still running.
- No real reference GPU is attached. llvmpipe correctness/measurement data does
  not establish the GTX 1060/UHD 620/RTX 3070 performance budgets.
- A universally clean rendering sanitizer run is blocked by the reproduced Mesa
  allocation leak. VRAM has no reliable portable query on this software renderer.
- The baseline runner drives a short fixed-input route; it does not yet prove
  the later all-content 20-second compatibility gate or long-duration soak.

See `docs/BUILDING.md` for exact reproduction commands and `docs/DECISIONS.md`
for scope and evidence decisions. Later phases must not bypass this gate.
