# TrackRS 0.7 "Reality" — progress against the master prompt

Snapshot after the 2026-11-08 session. Each line links to the phase report that
holds its evidence. "Done" means implemented, tested and on `main` with CI;
it does not mean certified against the §7 hardware budgets. Every measurement
so far comes from software rendering (Mesa llvmpipe).

| Phase | State | Report |
|---|---|---|
| 0 Safety net (CMake, CI, golden runs, benchmark) | Done | [PHASE_0](PHASE_0.md) |
| 1 Defect purge (R-07…R-16) | Done | [PHASE_1](PHASE_1.md) |
| 2 Modern renderer core: GL 3.3 core, RHI, PBR, vertex formats, IBL, no immediate mode | Done (2a–2d) | [2A](PHASE_2A.md) [2B](PHASE_2B.md) [2C](PHASE_2C.md) [2D](PHASE_2D.md) |
| 3a CDLOD terrain, PBR splat | Done | [PHASE_3A](PHASE_3A.md) |
| 3b Sky, time of day, height fog, CSM | Done | [PHASE_3B](PHASE_3B.md) |
| 3c World dressing: 3D vegetation, weather surfaces, headlights, water | Done except the road ribbon | [PHASE_3C](PHASE_3C.md) |
| 3d Post-FX: HDR target, auto-exposure, bloom, ACES, SSAO | Core done; TAA, motion blur, DoF, LUT and SSR open | [PHASE_3D](PHASE_3D.md) |
| 4 Real rally cars (Car Definition v2, liveries, damage visuals, roster) | Not started | — |
| 5 Stage dressing (signage, spectators, camera crews, marshals) | Not started | — |
| 6 Physics realism (Pacejka, drivetrain, aero, setup) | Not started | — |
| 7 Audio, cameras, HUD, replay, photo mode, co-driver | Not started | — |
| 8 Performance gate (profiler overlay, jobs, zero-alloc frames) | Partly done (pass timers, budgets in runners) | — |
| 9 Content tools, packs, release engineering | Not started | — |

## Defaults and switches

Legacy remains the default world renderer. PBR is opt-in through
**Options → World renderer** or `--renderer pbr`, and the PBR features each have
their own switch (README, "`<graphics>`"). The default should flip once TAA lands
and real-GPU measurements confirm §7.

## Open items carried forward

* **Road ribbon (3c):** spline road mesh, markings, berms, tyre marks.
* **3d remainder:** TAA (it also resolves the vegetation LOD dither), motion
  blur, DoF, colour LUT, and SSR (the water shader is ready to take an SSR source).
* **Real-GPU timings:** use the hand-off commands in PHASE_3B / 3C, with and
  without `--vegetation`, `--post` and `--weather`.
* **ASan in this container:** LSan reports SDL/dbus/driver allocations that
  the repository's audit policy rejects here. CI's audited ASan gate is
  authoritative.
* **Golden manifest:** the committed `tests/baselines/linux-gcc` manifest came
  from a different toolchain. Regression evidence in this session is an A/B
  against the base commit built with the same toolchain.
