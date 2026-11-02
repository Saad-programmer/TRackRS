# Build and validate the legacy engine

The 0.7 Reality migration starts with a safety net around the working 0.6.7
engine. Read [the phase report](reports/PHASE_0.md) for completed and pending
checks. This branch does not yet implement the modern renderer or new physics.

## Linux

Install CMake 3.20+, Ninja, Python 3.9+ (for mandatory layering checks), a C++17 compiler, and the development packages listed
in the README (SDL2, SDL2_image, glad (vendored), OpenGL, OpenAL, ALUT, PhysFS, TinyXML-2).
For Debian/Ubuntu:

```sh
sudo apt-get install build-essential cmake ninja-build libsdl2-dev libsdl2-image-dev \
  libopenal-dev libalut-dev libtinyxml2-dev libphysfs-dev \
  libgl1-mesa-dev libglu1-mesa-dev libglm-dev nlohmann-json3-dev xvfb xauth
cmake --preset linux-release
cmake --build --preset linux-release -j4
ctest --preset linux-release
cd build/linux-release/bin
./trackrs
```

The existing Make build remains supported. The CMake release uses portable
compiler defaults without fast-math or native-only instructions. Opt into
machine-specific instructions with `-DTRACKRS_NATIVE=ON`. ASan+UBSan and TSan
have separate configure/build/test presets; never combine their runtimes.
Legacy warnings are enabled and recorded, not treated as clean.
If an existing profile has only the old relative `../data` path, the CMake
build falls back to this checkout's data after all configured paths fail.
Valid user content paths keep their priority.

The cloud machine uses a rootless prefix: first source
`/workspace/trackrs-env/activate.sh`, add
`/workspace/trackrs-env/python/cmake/data/bin` and
`/workspace/trackrs-env/python/bin` to PATH, then configure with
`-DCMAKE_PREFIX_PATH=/workspace/trackrs-env/root/usr`.
Those paths are local environment setup, not repository dependencies.

## Windows

For MSVC 2022, use a developer terminal with Ninja and the vcpkg toolchain:

```powershell
cmake --preset win-msvc-release -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build --preset win-msvc-release
ctest --preset win-msvc-release
```

The manifest pins its registry baseline. For MinGW-w64, use an MSYS2 UCRT64
terminal with the dependencies in `.github/workflows/build.yml`, then run
the `win-mingw-release` configure/build/test presets. Windows jobs are defined;
the Linux cloud machine cannot establish Windows build or runtime success.
CI build artefacts are not standalone release installers.

## Deterministic integration checks

From the repository root on Linux:

```sh
LIBGL_ALWAYS_SOFTWARE=1 ALSOFT_DRIVERS=null xvfb-run -a \
  python3 tests/run_baselines.py --binary build/linux-release/bin/trackrs \
  --output build/baselines
```

This runs each scenario twice with fresh profiles and the same seed and replay.
It checks the actual 250 Hz update count, rolling body-state hash, output files,
and exact framebuffer equality on the same renderer. It does not substitute a
toy simulation. The selected terrain maps collectively cover legacy material
types; the short scripted route does not contact every material.
When Python is available, CTest also verifies all baseline files remain present,
parses the 256 legacy XML definitions and CRC-checks all 25 plugin archives.
That static content check is distinct from driving every stage.

Use `--quick` for the Ahead smoke check, `--frames N` for a longer run, or
`--compare <previous manifest.json>` to detect a change against a previous
run on the same build/platform. Cross-compiler bitwise equality is not assumed.
PPM captures are read before swap and converted losslessly to PNG using Python's
standard library. Physics hashes cover the trajectory's position, quaternion,
linear velocity and angular velocity (13 IEEE754 floats per tick), FNV-1a with
explicit little-endian byte ordering. They do not hash padding or pointers.

The application also accepts a single benchmark scenario:

```sh
LIBGL_ALWAYS_SOFTWARE=1 ALSOFT_DRIVERS=null xvfb-run -a \
  build/linux-release/bin/trackrs --benchmark --data-dir "$PWD/data" \
  --stage ahead --vehicle cordo_kitcar --frames 1500 \
  --replay tests/replays/baseline.csv --json build/ahead-benchmark.json
```

Benchmark JSON reports frame mean/p99/p99.9, CPU update time, whole-frame GPU
query time, stage load time, and per-scope CPU/GPU timing (terrain, meshes,
sky, water, particles and text). Submitted draw calls/triangles include
degenerates; Linux peak process RAM comes from getrusage. The first
stage-loading frame is excluded. Queries synchronize the GPU, so these
are instrumented software-renderer baselines, not reference-hardware
performance claims. Portable VRAM measurement is unavailable here and
is explicitly `null`; missing GPU timings are also `null`.

To collect all three reference scenarios serially, with no competing local
build/render work:

```sh
LIBGL_ALWAYS_SOFTWARE=1 ALSOFT_DRIVERS=null xvfb-run -a \
  python3 tests/run_benchmarks.py --binary build/linux-release/bin/trackrs \
  --output build/benchmarks
```

LeakSanitizer on this cloud image reports 108,124 bytes in 481 Mesa GLX
allocations, reproduced by `tests/driver_probe.cpp` without TrackRS.
Run the audited integration check with:

```sh
xvfb-run -a -s '-screen 0 640x480x24' python3 tests/run_asan.py \
  --binary build/linux-debug-asan/bin/trackrs --output build/asan-audit
```

Native leak detection remains enabled with full allocation stacks. The runner
retains raw reports and accepts only the two allocation PCs in the exact system
Mesa build listed in `lsan.supp`. TrackRS-origin allocations remain fatal even
when a driver is a caller; unknown driver builds/callsites also fail closed.
`lsan.supp` is an origin-policy file for this runner, **not** a native LSan
suppression file. Do not pass it to `LSAN_OPTIONS`: native LSan matches any
frame and cannot enforce the allocation-origin restriction. CTest includes
adversarial callback fixtures and a real intentional application-leak canary.
See ADR-006 in `docs/DECISIONS.md` for the user-approved policy clarification.

Invalid input, missing stages/vehicles, non-finite state, output-write errors,
and incomplete runs return nonzero. The runner enforces a timeout, preserves
logs and underlying command failures, and removes stale result files.

The current legacy simulation uses process-global state; the runner deliberately
starts one process per scenario until Phase 1 fixes simulation ownership and RNG.
Interactive play retains its existing timing and input paths.

## Phase 1 runtime and distributions

`project(TrackRS VERSION ...)` is the canonical version. CMake generates the
runtime header, appdata metadata, CPack versions and `generated/version.svg`.
When bumping it, copy the generated badge to `docs/version.svg` for the README.
The old GNUmakefile entry points forward to CMake; use the presets above.
Release uses x86-64-v2 on x86-64, `-O3 -ffp-contract=off`, and LTO when supported.
Use `-DTRACKRS_NATIVE=ON` only for a local machine; `-DTRACKRS_LTO=OFF` disables LTO.

The `<video>` element accepts `vsync="off|on|adaptive"` and `framecap="120"`.
Defaults are vsync on and 120 FPS cap; cap zero is unlimited. Unsupported adaptive
vsync falls back to ordinary vsync. Validation runs remain uncapped, fixed-step
and non-interpolated for reproducible state/image capture. Interactive vehicle
rendering interpolates body snapshots without changing simulation state.

`data/policy/edition.json` chooses demo/full/dev. Demo uses virtual-path allowlists;
full respects championship locks; dev bypasses them. The bundled splash plays
inside the SDL window; any key skips its remaining delay after loading. No
external player is required. Legacy levels derive a deterministic seed from their
virtual path; add decimal `stage_seed` on `<level>` to override it. Validation
combines this with `--seed`. `PSim` also accepts a fixed-step duration at construction;
legacy game/replay runs retain 0.004 seconds.

After a release build, create `.deb` or `.zip` with:

```sh
cpack --config build/linux-release/CPackConfig.cmake -G 'DEB;ZIP'
```

DEB resolves platform ABI dependencies with shlibdeps. Linux ZIP requires the same
runtime libraries as a source build; its binary/content layout is relocatable.
Windows CPack supports ZIP and NSIS and includes resolved non-system DLLs.
Run installers from the platform they were built for. Packages exclude the same
17 unlicensed plugin ZIPs as the previous Debian script; source content is retained.

For AppImage, supply the verified linuxdeploy tool and runtime described in
[tools/README.md](../tools/README.md), then run:

```sh
LINUXDEPLOY=/absolute/path/linuxdeploy-x86_64.AppImage \
LDAI_RUNTIME_FILE=/absolute/path/runtime-x86_64 \
cmake --build --preset linux-release --target appimage
```

The host still supplies graphics drivers and glibc; build on the oldest supported
distribution when targeting older systems. FUSE-free testing uses
`APPIMAGE_EXTRACT_AND_RUN=1 ./TrackRS-x86_64.AppImage --help`.

For the ten-minute wall-clock sanitizer soak (separate builds), use:

```sh
LP_NUM_THREADS=2 xvfb-run -a -s '-screen 0 640x480x24' python3 tests/run_soak.py \
  --binary build/linux-debug-asan/bin/trackrs --output build/soak-asan --sanitizer asan
LP_NUM_THREADS=0 xvfb-run -a -s '-screen 0 640x480x24' python3 tests/run_soak.py \
  --binary build/linux-debug-tsan/bin/trackrs --output build/soak-tsan --sanitizer thread
clang-tidy tests/core_lint.cpp -- -std=c++17 -I.
```

Each cycle runs 2,500 real-game ticks (10 simulated seconds) including load,
rendering, replay and teardown. The loop runs until at least 600 wall seconds;
slow cycles may exceed that target. Mesa worker threading is disabled only for
TSan; no TrackRS race suppressions are used. Raw native diagnostics are retained.


Optional intro playback uses the vendored MIT pl_mpeg decoder, with MPEG-1 video
and MP2 audio in `data/intro.mpg` (64 MiB input limit). Playback stays inside the
SDL window, queues PCM through SDL audio, and any key/mouse click skips. Missing
or invalid media falls back to the existing splash. The player launches no
external process; MP4/ffplay are not used. Test with `movie-decoder` and, under
Xvfb with xdotool, `python3 tests/test_intro.py --binary build/linux-release/bin/trackrs --output build/intro-test`.

### Phase 2b core rendering requirements

The visible window requires OpenGL 3.3 core and KHR_debug. glad 2.0.8 is vendored;
no GLEW or loader generator is needed to build. The game creates only one core
context; compatibility-profile support and context sharing are no longer required.
Startup fails with a diagnostic if core 3.3 or KHR_debug is unavailable.

Legacy-look shaders live in `data/shaders/basic`. Interactive runs poll edits
every 250 ms, retain the last valid linked program on errors, and watch included
files. Includes must stay inside the shader root; compile-time define permutations
are supported by the RHI API. Validation runs disable hot reload.

Run `xvfb-run -a python3 tests/run_rhi.py --binary build/linux-release/bin/rhi_tests
--output build/rhi-gate` for the core integration test (put the command on one
line). Add `--asan` with the ASan build; the existing exact-origin Mesa audit
policy remains unchanged. See [the report](reports/PHASE_2B.md).

`python3 tests/check_core_only.py` rejects removed GL calls in active production
code and the generated loader. `tests/run_rhi.py` also proves batched UI output,
retained meshes, basic lighting/fog, SDF coverage, and particle-ring reuse.

Golden images use the declared limits in `docs/reports/phase2b/tolerance.json`
with `tests/review_gate_images.py --tolerance ...`; physics hashes remain exact.
The 2b report records software-Mesa worker settings for each validation run.
