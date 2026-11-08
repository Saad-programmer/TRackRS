#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Per-car GPU memory estimate and measured mesh-pass GPU time.

VRAM (estimate, uncompressed): body + wheel OBJ triangles x 3 vertices x 32 B
(position, normal, UV as uploaded by the core mesh path) + every texture the
.mtl files reference at w x h x 4 B x 4/3 (full mip chain). Car Definition v2
cars without their own mesh use their fictional fallback body, so their figures
are that body's. GPU time comes from the `mesh_submit` pass of the calibration
runs' JSON (software renderer: relative, not hardware, numbers).
Usage: tools/car_gpu_report.py --calibration <dir from tests/run_calibration.py>
"""
import argparse, json, pathlib, re
from PIL import Image
ROOT = pathlib.Path(__file__).resolve().parents[1]
VEH = ROOT / 'data/vehicles'

def body_dir(car_id):
    cj = VEH / car_id / 'car.json'
    if cj.exists():
        c = json.loads(cj.read_text())
        return (VEH / c['body']['fallback']), c['body']['fallback']
    return VEH / car_id, car_id

def estimate(directory, stem):
    xml = (directory / f'{stem}.vehicle').read_text()
    objs = set(re.findall(r'(?:model|wheelmodel)="([^"]+\.obj)"', xml))
    tris, textures, seen = 0, 0, set()
    for name in objs:
        obj = directory / pathlib.Path(name).name
        if not obj.exists():
            continue
        for line in obj.read_text(errors='replace').splitlines():
            if line.startswith('f '):
                tris += max(len(line.split()) - 3, 0)
        mtl = obj.with_suffix('.mtl')
        if mtl.exists():
            for tex in re.findall(r'map_\w+\s+(\S+)', mtl.read_text(errors='replace')):
                path = directory / pathlib.Path(tex).name
                if path.exists() and path not in seen:
                    seen.add(path)
                    w, h = Image.open(path).size
                    textures += int(w * h * 4 * 4 / 3)
    return tris, tris * 3 * 32, textures

def main():
    ap = argparse.ArgumentParser(); ap.add_argument('--calibration', type=pathlib.Path, required=True)
    ap.add_argument('--output', type=pathlib.Path); a = ap.parse_args()
    cal = json.loads((a.calibration / 'calibration.json').read_text())['cars']
    rows, data = ['| car | body | triangles | mesh MB | textures MB | mesh_submit GPU ms/frame (software) |', '|---|---|---|---|---|---|'], {}
    for car_id, r in sorted(cal.items()):
        directory, stem = body_dir(car_id)
        tris, mesh, tex = estimate(directory, stem)
        run = a.calibration / f'{car_id}-accel.csv.json'
        gpu = None
        if run.exists():
            gpu = json.loads(run.read_text()).get('passes', {}).get('mesh_submit', {}).get('gpu_ms_per_frame')
        data[car_id] = dict(body=stem, triangles=tris, mesh_bytes=mesh, texture_bytes=tex, mesh_submit_gpu_ms=gpu)
        rows.append(f"| {r['name']} | {stem} | {tris} | {mesh / 2**20:.2f} | {tex / 2**20:.2f} | {gpu if gpu is not None else '—'} |")
    text = '\n'.join(rows) + '\n'
    print(text)
    if a.output:
        a.output.write_text(json.dumps(data, indent=2) + '\n')
        a.output.with_suffix('.md').write_text(text)

if __name__ == '__main__': main()
