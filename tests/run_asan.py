#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Audit one real-engine ASan run without hiding native sanitizer reports.

Owns its output directory and temporary profile. Single child process; no network.
Driver callers never exempt application-origin allocations. Unknown/incomplete
reports fail closed. Raw stderr, child status and accepted bytes remain auditable.
"""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
LEAK = re.compile(r"^(Direct|Indirect) leak of (\d+) byte\(s\) in (\d+) object\(s\) allocated from:$", re.M)
SUMMARY = re.compile(r"SUMMARY: AddressSanitizer: (\d+) byte\(s\) leaked in (\d+) allocation\(s\)\.")
FRAME = re.compile(r"^\s*#(\d+)\s+(.*)$", re.M)
MODULE = re.compile(r"\((/[^()]+)\+0x([0-9a-fA-F]+)\)")
BUILD_ID = re.compile(r"\(BuildId: ([0-9a-fA-F]+)\)")


def read_policy(path):
    allowed = set()
    for line in path.read_text().splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        rule = re.fullmatch(r"driver-origin:(libGLX_mesa\.so\.0)\+0x([0-9a-f]+)@([0-9a-f]{40})", line)
        if not rule:
            raise ValueError("unaudited suppression rule: " + line)
        allowed.add((rule.group(1), int(rule.group(2), 16), rule.group(3)))
    return allowed


def audit(text, allowed):
    if "runtime error:" in text or re.search(r"ERROR: (?:AddressSanitizer|UndefinedBehaviorSanitizer)", text):
        raise ValueError("application sanitizer error")
    if "suppression" in text.lower() and "Suppressions used" in text:
        raise ValueError("native suppression was active")
    summaries = SUMMARY.findall(text)
    leaks = list(LEAK.finditer(text))
    if len(summaries) != 1 or not leaks:
        raise ValueError("missing or incomplete leak report")
    accepted, total_bytes, total_objects = [], 0, 0
    for i, leak in enumerate(leaks):
        section = text[leak.end():leaks[i + 1].start() if i + 1 < len(leaks) else len(text)]
        frames = FRAME.findall(section)
        if len(frames) < 3 or [int(n) for n, _ in frames] != list(range(len(frames))) or len(frames) >= 64:
            raise ValueError("incomplete allocation stack")
        if not re.search(r"asan_(?:malloc_linux|new_delete)\.cpp", frames[0][1]):
            raise ValueError("unrecognized allocator frame")
        module = MODULE.search(frames[1][1])
        if not module:
            raise ValueError("allocation origin is unresolved or application-symbolized")
        origin = Path(module.group(1))
        build_id = BUILD_ID.search(frames[1][1])
        site = (origin.name, int(module.group(2), 16), build_id.group(1).lower() if build_id else "")
        # A same-named library under the checkout or /tmp is never trusted.
        resolved = origin.resolve()
        if site not in allowed or not resolved.is_relative_to(Path("/usr/lib").resolve()):
            raise ValueError("non-driver allocation origin: " + str(origin))
        size, objects = int(leak.group(2)), int(leak.group(3))
        total_bytes += size
        total_objects += objects
        accepted.append({"kind": leak.group(1), "bytes": size, "objects": objects, "origin": str(origin),
                         "allocation_pc": hex(site[1]), "build_id": site[2]})
    if (total_bytes, total_objects) != tuple(map(int, summaries[0])):
        raise ValueError("leak totals do not cover the entire report")
    return accepted


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--stage", default="ahead")
    parser.add_argument("--frames", type=int, default=1500)
    args = parser.parse_args()
    if args.frames < 2:
        parser.error("--frames must be at least 2")
    args.output.mkdir(parents=True, exist_ok=True)
    output = args.output.resolve()
    for name in ("audit.json", "state.json", "state.hash"):
        (output / name).unlink(missing_ok=True)
    with tempfile.TemporaryDirectory(prefix="trackrs-asan-") as profile:
        # Native LSan remains fully enabled. In particular, lsan.supp is NOT
        # supplied to LSAN_OPTIONS; an any-frame rule could hide a callback leak.
        env = dict(os.environ, XDG_DATA_HOME=profile, LIBGL_ALWAYS_SOFTWARE="1", ALSOFT_DRIVERS="null",
                   ASAN_OPTIONS="detect_leaks=1:fast_unwind_on_malloc=0:malloc_context_size=64:halt_on_error=1",
                   LSAN_OPTIONS="exitcode=23:max_leaks=0",
                   UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
        command = [str(args.binary.resolve()), "--headless", "--data-dir", str(ROOT / "data"),
                   "--stage", args.stage, "--frames", str(args.frames),
                   "--replay", str(ROOT / "tests/replays/baseline.csv"),
                   "--json", str(output / "state.json"), "--dump-state", str(output / "state.hash")]
        with (output / "raw.log").open("w") as log:
            result = subprocess.run(command, cwd=ROOT, env=env, stdout=log, stderr=subprocess.STDOUT, timeout=900)
    text = (output / "raw.log").read_text()
    if "Shutdown complete" not in text or "STATE_HASH " not in text:
        raise RuntimeError("engine did not complete; inspect raw.log")
    state = json.loads((output / "state.json").read_text())
    if state["ticks"] != args.frames or state["samples"] != args.frames - 1:
        raise RuntimeError("incomplete integration run")
    if (output / "state.hash").read_text().strip() != state["state_hash"]:
        raise RuntimeError("inconsistent state output")
    accepted = []
    if result.returncode == 23:
        accepted = audit(text, read_policy(ROOT / "lsan.supp"))
    elif result.returncode != 0 or re.search(r"ERROR:|runtime error:|SUMMARY: .*Sanitizer", text):
        raise RuntimeError("sanitizer run failed; inspect raw.log")
    report = {"native_exit_code": result.returncode, "state_hash": state["state_hash"],
              "ticks": args.frames, "accepted_driver_allocations": accepted,
              "accepted_driver_bytes": sum(v["bytes"] for v in accepted),
              "application_errors": 0, "native_suppressions": False}
    (output / "audit.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report))


if __name__ == "__main__":
    main()
