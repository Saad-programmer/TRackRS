#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Phase 4b: measured per-car cost of the PBR CarRenderer.

Runs every vehicle (legacy .vehicle and Car Definition v2 car.json) headless
with the PBR renderer on the same stage/replay and collects, per car:
  - VRAM: the renderer's own accounting of what it uploaded for that car type
    (body + wheel VBO/IBO incl. 3 LODs, interior mesh, albedo/role/light
    textures with full mip chains) from <json>.cars.json;
  - triangles per LOD and the draws of the main view;
  - time of the `cars` (main view) and `shadow_cars` (all cascades) passes:
    CPU submission ms/frame and GL timer-query ms/frame. Under llvmpipe the
    timer queries do not measure rasterisation cost (near zero); real-GPU
    numbers must come from a hardware run of this same tool.
Usage: tools/car_render_report.py --binary build/.../trackrs --output <dir>
"""
import argparse, json, os, pathlib, subprocess, sys, tempfile
ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests'))

def vehicles():
    out = []
    for d in sorted((ROOT / 'data/vehicles').iterdir()):
        if d.name == 'pbr_test':
            continue
        if (d / 'car.json').exists() or (d / f'{d.name}.vehicle').exists():
            out.append(d.name)
    return out

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--binary', type=pathlib.Path, required=True)
    ap.add_argument('--output', type=pathlib.Path, required=True)
    ap.add_argument('--frames', type=int, default=24)
    ap.add_argument('--time-of-day', default='noon')
    ap.add_argument('--only', nargs='*')
    a = ap.parse_args()
    a.output.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, LIBGL_ALWAYS_SOFTWARE=os.environ.get('LIBGL_ALWAYS_SOFTWARE', '1'), ALSOFT_DRIVERS='null')
    rows, data = [], {}
    for car in (a.only or vehicles()):
        target = (a.output / f'{car}.json').resolve()
        cmd = [str(a.binary.resolve()), '--headless', '--data-dir', str(ROOT / 'data'), '--renderer', 'pbr', '--quality', 'medium',
               '--width', '640', '--height', '480', '--time-of-day', a.time_of_day, '--shadows', 'csm', '--stage', 'ahead',
               '--vehicle', car, '--frames', str(a.frames), '--replay', str(ROOT / 'tests/replays/baseline.csv'), '--json', str(target),
               '--screenshot', str(target.with_suffix('.ppm'))]
        with tempfile.TemporaryDirectory(prefix='trackrs-cars-') as profile:
            log = a.output / f'{car}.log'
            with log.open('w') as stream:
                r = subprocess.run(cmd, env=dict(env, XDG_DATA_HOME=profile), stdout=stream, stderr=subprocess.STDOUT, timeout=900)
        if r.returncode:
            raise RuntimeError(f'{car} failed; see {log}')
        if 'Car renderer:' in log.read_text():
            raise RuntimeError(f'{car} fell back to the legacy path; see {log}')
        report = json.loads(target.read_text())
        cars = json.loads(pathlib.Path(str(target) + '.cars.json').read_text())
        t, v = cars['types'][0], cars['vehicles'][0]
        main, shadow = report['passes']['cars'], report['passes']['shadow_cars']
        try:
            from run_baselines import png_from_ppm
            png_from_ppm(target.with_suffix('.ppm'), target.with_suffix('.png'))
            target.with_suffix('.ppm').unlink()
        except Exception:
            pass
        data[car] = {'name': t['name'], 'gpu_bytes': t['gpu_bytes'], 'texture_bytes': t['texture_bytes'],
                     'body_triangles_per_lod': t['body_triangles_per_lod'], 'wheel_triangles_per_lod': t['wheel_triangles_per_lod'],
                     'interior_triangles': t['interior_triangles'], 'lod_drawn': v['lod'], 'draws': v['draws'], 'triangles_drawn': v['triangles'],
                     'cars_cpu_ms': main['cpu_ms_per_frame'], 'cars_gpu_ms': main['gpu_ms_per_frame'],
                     'shadow_cars_cpu_ms': shadow['cpu_ms_per_frame'], 'shadow_cars_gpu_ms': shadow['gpu_ms_per_frame'],
                     'cabin_measured': t['cabin']['measured']}
        d = data[car]
        rows.append(f"| `{car}` | {d['name']} | {d['gpu_bytes'] / 2**20:.2f} | {d['texture_bytes'] / 2**20:.2f} | "
                    f"{'/'.join(map(str, d['body_triangles_per_lod']))} | {d['draws']} | {d['triangles_drawn']} | "
                    f"{d['cars_cpu_ms']:.3f} | {d['cars_gpu_ms']:.5f} | {d['shadow_cars_cpu_ms']:.3f} |")
        print('measured', car, flush=True)
    head = ['| id | car | VRAM MB (total) | of which textures MB | body tris LOD0/1/2 | draws | tris drawn | cars CPU ms/frame | cars GL-timer ms/frame (llvmpipe) | shadow_cars CPU ms/frame |',
            '|---|---|---|---|---|---|---|---|---|---|']
    (a.output / 'cars.md').write_text('\n'.join(head + rows) + '\n')
    (a.output / 'cars.json').write_text(json.dumps({'renderer': 'GL timer queries under llvmpipe (software); CPU = submission wall time',
                                                    'frames': a.frames, 'time_of_day': a.time_of_day, 'cars': data}, indent=2) + '\n')
    print((a.output / 'cars.md').read_text())

if __name__ == '__main__':
    main()
