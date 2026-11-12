# Golden manifest regeneration (Phase 3e, one-off)

`manifest.json` in this directory was regenerated once, from the CI
toolchain, and is compared exactly from now on.

* **Source run:** [Legacy baselines (full run) #43184978914](https://github.com/Saad-programmer/TRackRS/actions/runs/43184978914)
  (`.github/workflows/baselines.yml`, dispatched on `main`).
* **Base commit of that run:** `f1b4015d86e9596e56d41c77bdfefa58251972b9`. Its game
  code is identical to the Phase 3e TAA commit `bc6f2a7`, because the
  commit only changed the workflow.
* **Toolchain** (as recorded by the run):
  * Ubuntu 24.04.5 LTS;
  * g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0;
  * Mesa 25.2.8-0ubuntu0.24.04.4 (llvmpipe).
* **Scope:** 13 legacy stages, 1500 ticks each, two repeats per stage, and
  framebuffers for the first eight.
* **How the file was taken:** the run prints the manifest between
  `=== BEGIN manifest.json ===` / `=== END manifest.json ===` markers in its
  job log. Build sessions cannot reach artifact storage, but they can read
  job logs. The file here is that block, byte for byte.

**Why.** The previous manifest was produced by an older toolchain during
Phase 0. The CI toolchain reproduces different legacy state hashes and
framebuffers for the *unchanged* game code (see the Phase 3c report).

* **No game code changed in this commit.** It contains only this note and
  `manifest.json`.
* **Cross-check:** a local build of the pre-3e base commit `ec087af`, in a
  container with the same toolchain, gives an identical manifest on every
  stage, both state hashes and framebuffer hashes.
* **What stays as it was:** the per-stage `*.hash` files, `screenshots/`,
  `benchmarks/` and the audit JSON remain the Phase 0 record of the older
  toolchain. They are not compared.
