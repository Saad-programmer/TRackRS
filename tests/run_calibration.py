#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Phase 4a car calibration — REPORT-ONLY (becomes blocking in Phase 6).

For every car (Car Definition v2 `car.json` and every legacy `.vehicle`):
  static  peak power / peak torque from the data (dyno sweep of the torque
          curve; legacy cars through the adapter constant), mass, layout
  dynamic real headless runs on the flat tarmac calibration stage with
          `--telemetry`: 0-100 km/h, highest speed within the run, and the
          100->0 km/h braking distance (second run brakes at the measured
          100 km/h tick).
Claims in car.json are unverified unless sourced, so deltas are informative.
Exit status is non-zero only if a run crashes or produces no telemetry.
"""
import argparse, concurrent.futures, csv, json, math, os, pathlib, subprocess, tempfile
import xml.etree.ElementTree as ET
ROOT = pathlib.Path(__file__).resolve().parents[1]
VEHICLES = ROOT / 'data/vehicles'
STAGE = '/testing/calibration/flat.level'
COUNTDOWN = 750                      # 3 s at 250 Hz: input ignored, brakes on
LEGACY_POWER_PER_WATT = 26000.0 / 223710.0  # engine/content/car_definition.h

def omega(rpm): return rpm * math.pi / 30

def interp(curve, rpm):
    if rpm < curve[0][0] or rpm > curve[-1][0]: return 0.0
    for (r0, t0), (r1, t1) in zip(curve, curve[1:]):
        if rpm <= r1: return t0 + (t1 - t0) * (rpm - r0) / (r1 - r0)
    return curve[-1][1]

def static(car_id):
    path = VEHICLES / car_id / 'car.json'
    if path.exists():
        c = json.loads(path.read_text())
        curve = c['engine']['torque_curve_nm']; redline = c['engine']['redline_rpm']
        curve = [p for p in curve if p[0] <= redline]
        info = dict(kind='car.json', name=c['display_name'] + (' (placeholder model)' if not c['body']['mesh'] else ''),
                    mass_kg=c['mass']['kerb_kg'], layout=c['drivetrain']['layout'],
                    verified=c['verified'], claims=c.get('claims', {}))
    else:
        root = ET.parse(VEHICLES / car_id / f'{car_id}.vehicle').getroot()
        engine = root.find('drivesystem/engine'); scale = float(engine.get('powerscale', 1))
        curve = [[float(p.get('rpm')), float(p.get('power')) * scale / LEGACY_POWER_PER_WATT / omega(float(p.get('rpm')))]
                 for p in engine.findall('powerpoint') if float(p.get('power')) > 0]
        drives = [(w.get('pos'), float(w.get('drive', 0))) for w in root.find('part').findall('wheel')]
        front = sum(d for p, d in drives if float(p.split(',')[1]) > 0); rear = sum(d for p, d in drives if float(p.split(',')[1]) <= 0)
        info = dict(kind='legacy .vehicle (fictional unless named)', name=root.get('name'),
                    mass_kg=float(root.find('genparams').get('mass')),
                    layout='FWD' if rear == 0 else 'RWD' if front == 0 else 'AWD',
                    verified={}, claims={})
    sweep = [(rpm, interp(curve, rpm)) for rpm in range(int(curve[0][0]), int(curve[-1][0]) + 1, 10)]
    p_rpm, p_t = max(sweep, key=lambda s: s[1] * omega(s[0]))
    t_rpm, t_nm = max(curve, key=lambda p: p[1])
    info.update(peak_power_kw=round(p_t * omega(p_rpm) / 1000, 1), peak_power_rpm=p_rpm,
                peak_torque_nm=round(t_nm, 1), peak_torque_rpm=t_rpm)
    return info

aborted = set()
def run(binary, car_id, frames, replay, telemetry, log):
    env = dict(os.environ, LIBGL_ALWAYS_SOFTWARE='1', ALSOFT_DRIVERS='null')
    with tempfile.TemporaryDirectory(prefix='trackrs-cal-') as profile:
        cmd = [str(binary), '--headless', '--data-dir', str(ROOT / 'data'), '--renderer', 'legacy', '--stage', STAGE,
               '--vehicle', car_id, '--frames', str(frames), '--width', '320', '--height', '200',
               '--replay', str(replay), '--telemetry', str(telemetry), '--json', str(telemetry) + '.json']
        with open(log, 'w') as out:
            r = subprocess.run(cmd, env=dict(env, XDG_DATA_HOME=profile), stdout=out, stderr=subprocess.STDOUT, timeout=1800)
    if not pathlib.Path(telemetry).exists():
        raise RuntimeError(f'{car_id}: run produced no telemetry, see {log}')
    if r.returncode:  # e.g. the legacy physics went non-finite: keep what was measured
        aborted.add(car_id)
    rows = [dict(tick=int(row['tick']), speed=float(row['speed_ms']), x=float(row['x']), y=float(row['y']))
            for row in csv.DictReader(open(telemetry))]
    # Drop samples after the physics went non-finite (aborted legacy runs).
    return [r for r in rows if all(math.isfinite(r[k]) for k in ('speed', 'x', 'y'))]

def dynamic(binary, car_id, out, frames):
    cached = out / f'{car_id}.result.json'
    if cached.exists():  # resume an interrupted sweep
        return json.loads(cached.read_text())
    accel = run(binary, car_id, frames, ROOT / 'tests/replays/calibration_accel.csv', out / f'{car_id}-accel.csv', out / f'{car_id}-accel.log')
    t100 = next((r['tick'] for r in accel if r['tick'] >= COUNTDOWN and r['speed'] >= 100 / 3.6), None)
    result = dict(zero_to_100_s=round((t100 - COUNTDOWN) / 250, 2) if t100 else None,
                  max_speed_kmh=round(max(r['speed'] for r in accel) * 3.6, 1),
                  run_seconds=round((frames - COUNTDOWN) / 250, 1), braking_100_0_m=None)
    if t100:
        replay = out / f'{car_id}-brake.csv'
        replay.write_text(f'# tick, throttle, steering, foot brake, handbrake\n0,0,0,0,0\n{COUNTDOWN},1,0,0,0\n{t100},0,0,1,0\n')
        brake = run(binary, car_id, t100 + 2500, replay, out / f'{car_id}-brake.csv.tel', out / f'{car_id}-brake.log')
        start = next(r for r in brake if r['tick'] >= t100)
        stop = next((r for r in brake if r['tick'] > t100 and r['speed'] < .3), None)
        if stop:
            result['braking_100_0_m'] = round(math.hypot(stop['x'] - start['x'], stop['y'] - start['y']), 1)
            result['braking_from_kmh'] = round(start['speed'] * 3.6, 1)
    if car_id in aborted:
        result['aborted'] = 'run stopped early (legacy physics went non-finite); figures cover the run up to that point'
    cached.write_text(json.dumps(result) + '\n')
    return result

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--binary', type=pathlib.Path, required=True); ap.add_argument('--output', type=pathlib.Path, required=True)
    ap.add_argument('--frames', type=int, default=8250, help='ticks per acceleration run (incl. 750-tick countdown)')
    ap.add_argument('--jobs', type=int, default=4); ap.add_argument('--cars', nargs='*')
    a = ap.parse_args(); a.output.mkdir(parents=True, exist_ok=True)
    cars = a.cars or sorted({p.parent.name for p in VEHICLES.glob('*/car.json')} | {p.stem for p in VEHICLES.glob('*/*.vehicle')})
    results = {c: static(c) for c in cars}
    with concurrent.futures.ThreadPoolExecutor(a.jobs) as pool:
        futures = {pool.submit(dynamic, a.binary, c, a.output, a.frames): c for c in cars}
        for f in concurrent.futures.as_completed(futures):
            results[futures[f]].update(f.result()); print('measured', futures[f], flush=True)
    report = {'schema': 'trackrs.calibration/1', 'mode': 'report-only (blocking in Phase 6)', 'stage': STAGE,
              'note': 'Legacy physics (no wheel inertia, non-SI power scale) drives every car; claims are unverified seeds.',
              'cars': results}
    (a.output / 'calibration.json').write_text(json.dumps(report, indent=2) + '\n')
    lines = ['| id | car | data | mass kg | layout | peak power kW @ rpm | peak torque N·m @ rpm | 0–100 s | max km/h (run s) | claimed top km/h | 100→0 m |',
             '|---|---|---|---|---|---|---|---|---|---|---|']
    for c in cars:
        r = results[c]
        lines.append(f"| `{c}` | {r['name']} | {r['kind']} | {r['mass_kg']:.0f} | {r['layout']} | {r['peak_power_kw']} @ {r['peak_power_rpm']} | "
                     f"{r['peak_torque_nm']} @ {r['peak_torque_rpm']:.0f} | {r['zero_to_100_s'] or '—'} | {r['max_speed_kmh']} ({r['run_seconds']}) | "
                     f"{r['claims'].get('top_speed_kmh') or '—'} | {r['braking_100_0_m'] or '—'}{' ⚠ aborted' if r.get('aborted') else ''} |")
    (a.output / 'calibration.md').write_text('\n'.join(lines) + '\n')
    print('\n'.join(lines))

if __name__ == '__main__': main()
