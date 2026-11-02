#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Wall-clock sanitizer soak of real stage load/replay/render/unload cycles.

Owns only output and temporary profiles. No sanitizer errors are ignored. ASan
uses the reviewed allocation-origin audit; TSan runs natively with halt-on-error.
Elapsed time includes startup/teardown; reported simulated time is separate.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
ROOT = Path(__file__).resolve().parents[1]

def main():
    p = argparse.ArgumentParser()
    p.add_argument('--binary', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--sanitizer', choices=('asan', 'thread'), required=True)
    p.add_argument('--seconds', type=int, default=600)
    args = p.parse_args()
    if args.seconds < 1:
        p.error('seconds must be positive')
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    (output / 'summary.json').unlink(missing_ok=True)
    start = time.monotonic()
    runs = []
    while time.monotonic() - start < args.seconds:
        stage = ('ahead', 'greenland', 'mudbath')[len(runs) % 3]
        case = output / f'{len(runs):03d}-{stage}'
        case.mkdir(exist_ok=True)
        if args.sanitizer == 'asan':
            command = [sys.executable, str(ROOT / 'tests/run_asan.py'), '--binary', str(args.binary.resolve()),
                       '--stage', stage, '--frames', '2500', '--output', str(case)]
            subprocess.run(command, check=True, timeout=900)
            state = json.loads((case / 'state.json').read_text())
        else:
            with tempfile.TemporaryDirectory(prefix='trackrs-tsan-') as profile:
                env = dict(os.environ, XDG_DATA_HOME=profile, TSAN_OPTIONS='halt_on_error=1',
                           LIBGL_ALWAYS_SOFTWARE='1', ALSOFT_DRIVERS='null')
                command = [str(args.binary.resolve()), '--headless', '--data-dir', str(ROOT / 'data'),
                           '--stage', stage, '--frames', '2500', '--replay', str(ROOT / 'tests/replays/baseline.csv'),
                           '--json', str(case / 'state.json'), '--dump-state', str(case / 'state.hash')]
                with (case / 'raw.log').open('w') as log:
                    subprocess.run(command, env=env, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=900)
            text = (case / 'raw.log').read_text()
            if 'ThreadSanitizer' in text or 'Shutdown complete' not in text:
                raise RuntimeError('thread sanitizer run failed')
            state = json.loads((case / 'state.json').read_text())
        if state['ticks'] != 2500 or state['samples'] != 2499:
            raise RuntimeError('incomplete soak cycle')
        runs.append({'stage': stage, 'ticks': state['ticks'], 'state_hash': state['state_hash']})
        print(f'PASS {args.sanitizer} cycle {len(runs)}; elapsed {time.monotonic()-start:.1f}s', flush=True)
    summary = {'sanitizer': args.sanitizer, 'requested_wall_seconds': args.seconds,
               'elapsed_wall_seconds': time.monotonic()-start, 'simulated_seconds': sum(r['ticks'] for r in runs)*.004,
               'cycles': runs, 'application_errors': 0}
    (output / 'summary.json').write_text(json.dumps(summary, indent=2)+'\n')
if __name__ == '__main__':
    main()
