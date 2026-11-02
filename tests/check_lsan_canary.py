#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Prove a real instrumented application allocation cannot pass the driver audit."""
import os
from pathlib import Path
import subprocess
import sys
from run_asan import audit, read_policy, ROOT

env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:fast_unwind_on_malloc=0:malloc_context_size=64",
           LSAN_OPTIONS="exitcode=23:max_leaks=0")
result = subprocess.run([str(Path(sys.argv[1]).resolve())], env=env, text=True,
                        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
if result.returncode != 23 or "LeakSanitizer" not in result.stdout:
    raise RuntimeError("canary did not produce its required native leak failure")
try:
    audit(result.stdout, read_policy(ROOT / "lsan.supp"))
except ValueError as error:
    print("Application-origin leak remains fatal:", error)
else:
    raise RuntimeError("driver policy incorrectly accepted an application leak")
