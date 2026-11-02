#!/usr/bin/env python3
"""Run real legacy stages twice in isolated profiles; fail on errors or hash drift.

Owns only the requested output directory and temporary profiles. Runs scenarios
serially on the caller's GL display; no network and no third-party Python modules.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import zlib

ROOT = Path(__file__).resolve().parents[1]
STAGES = ["ahead", "aegyptian", "greenland", "mudbath", "april", "ancient",
          "coastline", "country", "darkbushes", "barren", "altitude", "arizonachase", "glacier"]
CARS = ["cordo_kitcar", "evo_wrc", "fox_super500"]


def png_from_ppm(source, target):
    with source.open("rb") as f:
        if f.readline() != b"P6\n":
            raise ValueError("unexpected framebuffer format")
        width, height = map(int, f.readline().split())
        if f.readline() != b"255\n":
            raise ValueError("unexpected framebuffer depth")
        rgb = f.read()
    if len(rgb) != width * height * 3 or len(set(rgb)) < 16:
        raise ValueError("empty, truncated or blank framebuffer")
    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))
    rows = b"".join(b"\0" + rgb[y * width * 3:(y + 1) * width * 3] for y in range(height))
    target.write_bytes(b"\x89PNG\r\n\x1a\n" +
                       chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)) +
                       chunk(b"IDAT", zlib.compress(rows, 9)) + chunk(b"IEND", b""))
    return hashlib.sha256(rgb).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--quick", action="store_true")
    parser.add_argument("--frames", type=int)
    parser.add_argument("--stages", nargs="+", choices=STAGES, help="run a selected subset")
    parser.add_argument("--compare", type=Path, help="previous run manifest, same build/platform only")
    parser.add_argument("--repeats", type=int, choices=(1, 2), default=2, help="one run is a regression probe, two also prove repeatability")
    parser.add_argument("--state-only", action="store_true", help="compare simulation records while retaining new framebuffer evidence")
    args = parser.parse_args()
    args.binary = args.binary.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / "manifest.json").unlink(missing_ok=True)
    frames = args.frames or (1250 if args.quick else 1500)
    if frames < 2:
        parser.error("--frames must be at least 2")
    cases = args.stages or (STAGES[:1] if args.quick else STAGES)
    reports = []
    for i, stage in enumerate(cases):
        car = CARS[STAGES.index(stage) % len(CARS)]
        hashes, images = [], []
        for repeat in range(args.repeats):
            stem = args.output / f"{stage}-{repeat}"
            with tempfile.TemporaryDirectory(prefix="trackrs-profile-") as profile:
                env = dict(os.environ, XDG_DATA_HOME=profile, ALSOFT_DRIVERS="null", LIBGL_ALWAYS_SOFTWARE="1")
                command = [str(args.binary), "--headless", "--data-dir", str(ROOT / "data"),
                           "--stage", stage, "--vehicle", car, "--frames", str(frames),
                           "--replay", str(ROOT / "tests/replays/baseline.csv"),
                           "--dump-state", str(stem.with_suffix(".hash").resolve()),
                           "--json", str(stem.with_suffix(".json").resolve())]
                if STAGES.index(stage) < 8:
                    command += ["--screenshot", str(stem.with_suffix(".ppm").resolve())]
                # Never accept stale output after a failed or zero-work run.
                for ext in (".hash", ".json", ".ppm", ".png"):
                    stem.with_suffix(ext).unlink(missing_ok=True)
                with stem.with_suffix(".log").open("w") as log:
                    subprocess.run(command, cwd=ROOT, env=env, stdout=log, stderr=subprocess.STDOUT,
                                   timeout=600, check=True)
            report = json.loads(stem.with_suffix(".json").read_text())
            if report["ticks"] != frames or report["samples"] != frames - 1:
                raise RuntimeError(f"wrong work count: {stage}")
            if (report["draw_calls"] <= 0 or report["triangles"] <= 0 or
                    report["passes"]["terrain"]["calls"] != frames - 1 or
                    report["passes"]["mesh_submit"]["calls"] <= 0):
                raise RuntimeError(f"missing render measurements: {stage}")
            hashes.append(stem.with_suffix(".hash").read_text().strip())
            if hashes[-1] != report["state_hash"]:
                raise RuntimeError("hash/report mismatch")
            if STAGES.index(stage) < 8:
                images.append(png_from_ppm(stem.with_suffix(".ppm"), stem.with_suffix(".png")))
                stem.with_suffix(".ppm").unlink()
        if len(set(hashes)) != 1:
            raise RuntimeError(f"non-deterministic physics: {stage}: {hashes}")
        if images and len(set(images)) != 1:
            raise RuntimeError(f"non-deterministic framebuffer: {stage}: {images}")
        reports.append({"stage": stage, "vehicle": car, "ticks": frames, "state_hash": hashes[0],
                        "framebuffer_sha256": images[0] if images else None})
        print(f"PASS {stage}/{car}: {frames} ticks, hash {hashes[0]}", flush=True)
    summary = {"schema": "trackrs.legacy-baselines/1", "repeat_count": args.repeats, "scenarios": reports}
    (args.output / "manifest.json").write_text(json.dumps(summary, indent=2) + "\n")
    if args.compare:
        previous = json.loads(args.compare.read_text())
        keys = ("stage", "vehicle", "ticks", "state_hash") if args.state_only else tuple(reports[0])
        old = {r["stage"]: r for r in previous["scenarios"]}
        changed = [r["stage"] for r in reports if any(r[k] != old.get(r["stage"], {}).get(k) for k in keys)]
        if changed:
            raise RuntimeError("baseline regression in: " + ", ".join(changed))


if __name__ == "__main__":
    main()
