#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Measure three real-engine reference scenarios serially on the caller's display.

Owns only requested outputs and temporary profiles. No network. Run when no
other build/render workload is active; query instrumentation still adds overhead.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--frames", type=int, default=1250)
    args = parser.parse_args()
    if args.frames < 2:
        parser.error("--frames must be at least 2")
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / "summary.json").unlink(missing_ok=True)
    reports = []
    for stage, vehicle in [("ahead", "cordo_kitcar"), ("greenland", "evo_wrc"), ("mudbath", "fox_super500")]:
        output = (args.output / (stage + ".json")).resolve()
        output.unlink(missing_ok=True)
        with tempfile.TemporaryDirectory(prefix="trackrs-benchmark-") as profile:
            env = dict(os.environ, XDG_DATA_HOME=profile, ALSOFT_DRIVERS="null", LIBGL_ALWAYS_SOFTWARE="1")
            command = [str(args.binary.resolve()), "--benchmark", "--stage", stage, "--vehicle", vehicle,
                       "--data-dir", str(ROOT / "data"), "--replay", str(ROOT / "tests/replays/baseline.csv"),
                       "--frames", str(args.frames), "--json", str(output)]
            with (args.output / (stage + ".log")).open("w") as log:
                subprocess.run(command, cwd=ROOT, env=env, stdout=log, stderr=subprocess.STDOUT,
                               timeout=600, check=True)
        report = json.loads(output.read_text())
        if report["ticks"] != args.frames or report["samples"] != args.frames - 1 or report["draw_calls"] <= 0:
            raise RuntimeError("incomplete benchmark: " + stage)
        reports.append(report)
        print(f"{stage}: {report['frame_ms_avg']:.3f} ms/frame, "
              f"{report['draw_calls']:.1f} submissions/frame", flush=True)
    (args.output / "summary.json").write_text(json.dumps(reports, indent=2) + "\n")


if __name__ == "__main__":
    main()
