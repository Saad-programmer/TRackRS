#!/usr/bin/env python3
"""Run real PBR probes, preview and optional game integration.
Native sanitizers use the allocation-origin audit, never broad suppressions.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile
from run_asan import audit, read_policy
ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
p.add_argument('--binary', type=Path, required=True)
p.add_argument('--game-binary', type=Path)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--asan', action='store_true')
args = p.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
(args.output/'result.json').unlink(missing_ok=True)
env = dict(os.environ, LIBGL_ALWAYS_SOFTWARE='1', ALSOFT_DRIVERS='null',
           ASAN_OPTIONS='detect_leaks=1:fast_unwind_on_malloc=0:malloc_context_size=64:halt_on_error=1',
           LSAN_OPTIONS='exitcode=23:max_leaks=0', UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
cases = [('probes', args.binary, ['--test', '--report', str((args.output/'gpu.json').resolve())], 'gpu.json'),
         ('preview', args.binary, ['--frames', '2', '--output', str((args.output/'preview.png').resolve())], 'preview.png')]
if args.game_binary:
    cases.append(('game', args.game_binary, ['--headless', '--stage', 'ahead', '--vehicle', 'pbr_test',
                  '--frames', '300', '--replay', str(ROOT/'tests/replays/baseline.csv'),
                  '--json', str((args.output/'game.json').resolve()),
                  '--screenshot', str((args.output/'game.ppm').resolve())], 'game.json'))
results = []
for name, binary, options, artifact in cases:
    target = args.output/artifact
    target.unlink(missing_ok=True)
    logfile = args.output/f'{name}.log'
    with tempfile.TemporaryDirectory(prefix='trackrs-pbr-profile-') as profile:
        with logfile.open('w') as log:
            result = subprocess.run([str(binary.resolve()), '--data-dir', str(ROOT/'data'), *options],
                                    env=dict(env, XDG_DATA_HOME=profile), stdout=log, stderr=subprocess.STDOUT, timeout=600)
    text = logfile.read_text()
    accepted = []
    if args.asan and result.returncode == 23:
        accepted = audit(text, read_policy(ROOT/'lsan.supp'))
    elif result.returncode or 'runtime error:' in text or 'ERROR: AddressSanitizer' in text:
        raise RuntimeError(f'{name} failed; inspect {logfile}')
    if not target.is_file():
        raise RuntimeError(f'{name} did not create fresh evidence')
    if name == 'game':
        report = json.loads(target.read_text())
        if report['ticks'] != 300 or report['vehicle'] != 'pbr_test' or report['passes']['mesh_submit']['calls'] <= 0:
            raise RuntimeError('PBR game integration did not submit the test car')
        from run_baselines import png_from_ppm
        png_from_ppm(args.output/'game.ppm', args.output/'game.png')
        (args.output/'game.ppm').unlink()
    results.append({'name': name, 'passed': True, 'native_exit': result.returncode,
                    'accepted_driver_allocations': accepted})
(args.output/'result.json').write_text(json.dumps({'passed': True, 'runs': results}, indent=2)+'\n')
print('PASS: PBR GPU probes, material preview and requested game integration')
