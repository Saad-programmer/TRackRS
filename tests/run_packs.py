#!/usr/bin/env python3
"""M0 pack-lite runtime gate.

1. With the fixture packs mounted (--pack-root), the game starts, disables the
   broken packs with a readable reason (never a crash), lists the good pack's
   stage and plays it by its global id (fixture-good:mini) twice with the same
   state hash (determinism).
2. Legacy content still loads alongside packs: a legacy stage with the same
   packs mounted reproduces the legacy-only state hash.
3. The shipped data validates (trackrs-content) and CREDITS.md is current.
Software GL; no timing claims.
"""
import argparse, json, os, subprocess, tempfile
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
p.add_argument('--binary', type=Path, required=True)
p.add_argument('--content-tool', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
a.output.mkdir(parents=True, exist_ok=True)
env = dict(os.environ, LIBGL_ALWAYS_SOFTWARE='1', ALSOFT_DRIVERS='null')


def run(name, stage, packs=True, frames=120, renderer='legacy'):
    target = (a.output / f'{name}.json').resolve()
    for suffix in ('', '.packs.json'):
        Path(str(target) + suffix).unlink(missing_ok=True)
    cmd = [str(a.binary.resolve()), '--headless', '--data-dir', str(ROOT / 'data'), '--renderer', renderer,
           '--stage', stage, '--vehicle', 'cordo_kitcar', '--frames', str(frames),
           '--replay', str(ROOT / 'tests/replays/baseline.csv'), '--json', str(target)]
    if packs:
        cmd += ['--pack-root', str(ROOT / 'tests/fixtures/packs')]
    with tempfile.TemporaryDirectory(prefix='trackrs-packs-') as profile:
        log = a.output / f'{name}.log'
        with log.open('w') as stream:
            r = subprocess.run(cmd, env=dict(env, XDG_DATA_HOME=profile), stdout=stream, stderr=subprocess.STDOUT, timeout=600)
    text = log.read_text()
    if r.returncode or 'runtime error:' in text:
        raise RuntimeError(f'{name} failed (exit {r.returncode}); see {log}')
    report = json.loads(target.read_text())
    assert report['ticks'] == frames, f'{name}: wrong tick count'
    packs_report = json.loads(Path(str(target) + '.packs.json').read_text())
    return report, packs_report, text


first, catalog, log = run('pack-stage', 'fixture-good:mini')
second, _, _ = run('pack-stage-repeat', 'fixture-good:mini')
assert first['state_hash'] == second['state_hash'], 'pack stage is not deterministic'
assert 'Loading level "/packs/fixture-good/stages/mini/mini.level"' in log, 'pack stage did not load through the level loader'
packs = {p['id']: p for p in catalog['packs']}
assert packs['classic-legacy-stages']['enabled'], 'legacy pack missing'
assert packs['fixture-good']['enabled'] and packs['fixture-good']['stages'][0]['id'] == 'fixture-good:mini'
assert packs['fixture-good']['championships'][0]['rounds'] == 1
for broken, needle in (('fixture-broken-json', 'parse error'), ('fixture-unlicensed', 'licence audit')):
    assert not packs[broken]['enabled'] and needle in packs[broken]['disabled_reason'], f'{broken} not disabled correctly'
    assert f'Content pack {broken} DISABLED' in log, f'{broken} not reported'
assert packs['morocco-sahara']['enabled'], 'shipped morocco-sahara pack disabled'
legacy_only, _, _ = run('legacy-only', 'ahead', packs=False)
legacy_with_packs, _, _ = run('legacy-with-packs', 'ahead', packs=True)
assert legacy_only['state_hash'] == legacy_with_packs['state_hash'], 'mounting packs changed a legacy stage'
subprocess.run([str(a.content_tool), 'validate', str(ROOT / 'data')], check=True)
bad = subprocess.run([str(a.content_tool), 'validate', str(ROOT / 'tests/fixtures')], capture_output=True, text=True)
assert bad.returncode == 1 and 'FAIL fixture-unlicensed' in bad.stdout, 'validator accepted the broken fixtures'
subprocess.run([str(a.content_tool), 'credits', str(ROOT / 'data'), '--output', str(ROOT / 'docs/CREDITS.md'), '--check'], check=True)
result = {'pack_stage_hash': first['state_hash'], 'legacy_hash': legacy_only['state_hash'],
          'packs': {k: {'enabled': v['enabled'], 'reason': v['disabled_reason']} for k, v in packs.items()}}
(a.output / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
print('PASS: pack-lite runtime gate', json.dumps(result))
