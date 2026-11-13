#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Check a supplied car mesh (.glb, glTF 2.0 binary) against docs/ASSET_SOURCING.md.

Standard library only (no Pillow), so CI and contributors can run it as is:

    tools/check_car_mesh.py model.glb --class rally-raid [--radius 0.45] [--json report.json]

Checks, in the asset's own glTF axes (+Y up, front towards -Z, right towards +X):
  * required part nodes (separate bumpers, mirrors, wheels, lamps, glass) and
    that every node name is a known part, optionally with a _lod1/_lod2 suffix;
  * material names from the fixed list;
  * triangle budgets per LOD for the class (TRIANGLES primitives only);
  * scale in metres: overall length/width/height within the class ranges, car
    resting on the ground (min Y about 0) and centred on the origin;
  * wheels: hub at the node origin, radius matching --radius (car.json) +-2 cm;
  * textures: PNG or JPEG, power-of-two sides, at most the class maximum.
Exit status 0 when every check passes, 1 otherwise (each problem is printed).
"""
import argparse
import json
import math
import pathlib
import struct
import sys

PARTS_REQUIRED = ['body', 'glass', 'bumper_front', 'bumper_rear', 'mirror_left', 'mirror_right',
                  'wheel_fl', 'wheel_fr', 'wheel_rl', 'wheel_rr',
                  'light_head_left', 'light_head_right', 'light_tail_left', 'light_tail_right']
PARTS_OPTIONAL = ['bonnet', 'boot', 'door_fl', 'door_fr', 'door_rl', 'door_rr', 'spoiler', 'exhaust',
                  'mudflap_fl', 'mudflap_fr', 'mudflap_rl', 'mudflap_rr', 'light_indicator_left',
                  'light_indicator_right', 'interior', 'dashboard', 'steering_wheel', 'seat_left', 'seat_right',
                  'needle_tacho', 'needle_speed',
                  'brake_disc_fl', 'brake_disc_fr', 'brake_disc_rl', 'brake_disc_rr',
                  # rally-raid extras
                  'spare_wheel_1', 'spare_wheel_2', 'roof_scoop', 'snorkel', 'sand_ladder_left', 'sand_ladder_right',
                  'skid_plate', 'roll_cage']
MATERIALS = {'paint', 'paint_secondary', 'glass', 'rubber', 'metal', 'chrome', 'plastic_trim', 'carbon',
             'light_head', 'light_tail', 'light_indicator', 'interior', 'decal'}
WHEELS = ['wheel_fl', 'wheel_fr', 'wheel_rl', 'wheel_rr']
INTERIOR = {'interior', 'dashboard', 'steering_wheel', 'seat_left', 'seat_right', 'roll_cage', 'needle_tacho',
            'needle_speed'}
# Per class: triangle budgets (LOD0, LOD1, LOD2) for the exterior without wheels and interior, per wheel,
# the interior (LOD0 only), size ranges in metres and the largest texture side.
CLASSES = {
    'rally-raid': {'body': (80000, 25000, 6000), 'wheel': (6000, 2000, 400), 'interior': 25000,
                   'length': (3.2, 5.6), 'width': (1.6, 2.4), 'height': (1.4, 2.2), 'texture': 2048},
    'heritage-raid': {'body': (60000, 20000, 5000), 'wheel': (5000, 1500, 300), 'interior': 20000,
                      'length': (3.6, 5.2), 'width': (1.6, 2.2), 'height': (1.2, 2.0), 'texture': 2048},
    'rally': {'body': (60000, 20000, 5000), 'wheel': (5000, 1500, 300), 'interior': 20000,
              'length': (3.4, 4.8), 'width': (1.6, 2.0), 'height': (1.1, 1.6), 'texture': 2048},
}
COMPONENT = {5120: 1, 5121: 1, 5122: 2, 5123: 2, 5125: 4, 5126: 4}
TYPE = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4, 'MAT4': 16}


def read_glb(path):
    data = path.read_bytes()
    if len(data) < 20 or data[:4] != b'glTF':
        raise ValueError('not a binary glTF (.glb) file')
    version, length = struct.unpack_from('<II', data, 4)
    if version != 2:
        raise ValueError(f'glTF version {version}, expected 2')
    offset, doc, binary = 12, None, b''
    while offset + 8 <= min(length, len(data)):
        size, kind = struct.unpack_from('<II', data, offset)
        chunk = data[offset + 8:offset + 8 + size]
        if kind == 0x4E4F534A:
            doc = json.loads(chunk.decode('utf-8'))
        elif kind == 0x004E4942:
            binary = chunk
        offset += 8 + size
    if doc is None:
        raise ValueError('no JSON chunk')
    return doc, binary


def split_lod(name):
    for level in (1, 2):
        if name.endswith(f'_lod{level}'):
            return name[:-5], level
    return name, 0


def image_size(blob):
    if blob[:8] == b'\x89PNG\r\n\x1a\n':
        return struct.unpack('>II', blob[16:24])
    if blob[:2] == b'\xff\xd8':  # JPEG: walk the markers to a start-of-frame
        i = 2
        while i + 9 < len(blob):
            if blob[i] != 0xFF:
                i += 1
                continue
            marker = blob[i + 1]
            size = struct.unpack('>H', blob[i + 2:i + 4])[0]
            if marker in (0xC0, 0xC1, 0xC2):
                h, w = struct.unpack('>HH', blob[i + 5:i + 9])
                return w, h
            i += 2 + size
    return None


def check(path, cls, radius=None):
    spec = CLASSES[cls]
    doc, binary = read_glb(path)
    problems, notes = [], {}
    accessors, views = doc.get('accessors', []), doc.get('bufferViews', [])
    meshes, nodes = doc.get('meshes', []), doc.get('nodes', [])

    def triangles(mesh_index):
        total = 0
        for prim in meshes[mesh_index].get('primitives', []):
            if prim.get('mode', 4) != 4:
                problems.append(f'mesh {mesh_index}: primitive mode {prim.get("mode")} (only TRIANGLES)')
                continue
            idx = prim.get('indices')
            count = accessors[idx]['count'] if idx is not None else accessors[prim['attributes']['POSITION']]['count']
            total += count // 3
        return total

    def bounds(mesh_index):
        lo, hi = [math.inf] * 3, [-math.inf] * 3
        for prim in meshes[mesh_index].get('primitives', []):
            acc = accessors[prim['attributes']['POSITION']]
            if 'min' in acc and 'max' in acc:
                lo = [min(a, b) for a, b in zip(lo, acc['min'])]
                hi = [max(a, b) for a, b in zip(hi, acc['max'])]
        return lo, hi

    # Names and per-LOD triangle totals.
    known = set(PARTS_REQUIRED) | set(PARTS_OPTIONAL)
    present = {0: set(), 1: set(), 2: set()}
    body_tris, interior_tris, wheel_tris = [0, 0, 0], 0, {}
    lo, hi = [math.inf] * 3, [-math.inf] * 3
    for node in nodes:
        name = node.get('name', '')
        if 'mesh' not in node:
            continue
        part, level = split_lod(name)
        if part not in known:
            problems.append(f'node "{name}": unknown part name (see ASSET_SOURCING.md, "Part nodes")')
            continue
        if any(k in node for k in ('rotation', 'scale', 'matrix')) and part not in WHEELS:
            problems.append(f'node "{name}": apply rotation/scale before export (translation only)')
        present[level].add(part)
        t = triangles(node['mesh'])
        if part in WHEELS:
            wheel_tris.setdefault(part, [0, 0, 0])[level] += t
            mlo, mhi = bounds(node['mesh'])
            if level == 0 and radius is not None and math.isfinite(mlo[1]):
                r = (mhi[1] - mlo[1]) / 2
                if abs(r - radius) > .02:
                    problems.append(f'{part}: tyre radius {r:.3f} m, car.json says {radius:.3f} m (+-0.02)')
                centre = [(a + b) / 2 for a, b in zip(mlo, mhi)]
                if max(abs(c) for c in centre) > .02:
                    problems.append(f'{part}: hub must be at the node origin (mesh centre {centre})')
            continue
        if part in INTERIOR:
            if level == 0:
                interior_tris += t
            continue
        body_tris[level] += t
        if level == 0:
            mlo, mhi = bounds(node['mesh'])
            offset = node.get('translation', [0, 0, 0])
            lo = [min(a, b + o) for a, b, o in zip(lo, mlo, offset)]
            hi = [max(a, b + o) for a, b, o in zip(hi, mhi, offset)]
    for part in PARTS_REQUIRED:
        if part not in present[0]:
            problems.append(f'missing required part node "{part}" (LOD0)')
    for level in (1, 2):
        if present[level] and present[level] != {p for p in present[0] if p not in INTERIOR}:
            missing = sorted({p for p in present[0] if p not in INTERIOR} - present[level])
            if missing:
                problems.append(f'LOD{level} lacks parts present at LOD0: {", ".join(missing)}')
    for level in range(3):
        if body_tris[level] > spec['body'][level]:
            problems.append(f'exterior LOD{level}: {body_tris[level]} triangles > {spec["body"][level]}')
    for part, counts in wheel_tris.items():
        for level in range(3):
            if counts[level] > spec['wheel'][level]:
                problems.append(f'{part} LOD{level}: {counts[level]} triangles > {spec["wheel"][level]}')
    if interior_tris > spec['interior']:
        problems.append(f'interior: {interior_tris} triangles > {spec["interior"]}')
    notes['triangles'] = {'exterior_per_lod': body_tris, 'wheels': wheel_tris, 'interior_lod0': interior_tris}

    # Scale and placement (glTF axes: x right, y up, z backwards).
    if math.isfinite(lo[0]):
        width, height, length = hi[0] - lo[0], hi[1] - lo[1], hi[2] - lo[2]
        notes['dimensions_m'] = {'length': round(length, 3), 'width': round(width, 3), 'height': round(height, 3)}
        for key, value in (('length', length), ('width', width), ('height', height)):
            a, b = spec[key]
            if not a <= value <= b:
                problems.append(f'{key} {value:.2f} m outside {a}-{b} m: wrong scale (metres) or axes?')
        if abs(lo[1]) > .15:
            problems.append(f'lowest point at y = {lo[1]:.2f} m: the car must rest on the ground plane (y = 0)')
        if abs((lo[0] + hi[0]) / 2) > .05 or abs((lo[2] + hi[2]) / 2) > .3:
            problems.append('exterior is not centred on the origin (x and z)')
        if width > length:
            problems.append('wider than long: the front must point towards -Z (glTF), not along X')

    # Materials.
    for m in doc.get('materials', []):
        if m.get('name') not in MATERIALS:
            problems.append(f'material "{m.get("name")}": not in the allowed list')

    # Textures.
    sizes = []
    for i, img in enumerate(doc.get('images', [])):
        blob = None
        if 'bufferView' in img:
            v = views[img['bufferView']]
            blob = binary[v.get('byteOffset', 0):v.get('byteOffset', 0) + v['byteLength']]
        elif 'uri' in img and not img['uri'].startswith('data:'):
            p = path.parent / img['uri']
            blob = p.read_bytes() if p.exists() else None
            if blob is None:
                problems.append(f'image {i}: missing file {img["uri"]}')
        size = image_size(blob) if blob else None
        if blob and size is None:
            problems.append(f'image {i}: not PNG or JPEG')
        if size:
            w, h = size
            sizes.append([w, h])
            if w & (w - 1) or h & (h - 1):
                problems.append(f'image {i}: {w}x{h} is not power-of-two')
            if max(w, h) > spec['texture']:
                problems.append(f'image {i}: {w}x{h} exceeds {spec["texture"]}')
    notes['textures'] = sizes
    return problems, notes


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('mesh', type=pathlib.Path)
    ap.add_argument('--class', dest='cls', choices=sorted(CLASSES), required=True)
    ap.add_argument('--radius', type=float, help='tyres.radius_m from the car.json')
    ap.add_argument('--json', type=pathlib.Path)
    a = ap.parse_args()
    try:
        problems, notes = check(a.mesh, a.cls, a.radius)
    except (ValueError, KeyError, IndexError, struct.error) as error:
        problems, notes = [f'unreadable: {error}'], {}
    if a.json:
        a.json.write_text(json.dumps({'mesh': str(a.mesh), 'class': a.cls, 'ok': not problems,
                                      'problems': problems, **notes}, indent=2) + '\n')
    for p in problems:
        print('FAIL:', p)
    print(('PASS' if not problems else f'{len(problems)} problem(s)') + f': {a.mesh} ({a.cls})')
    return 0 if not problems else 1


if __name__ == '__main__':
    sys.exit(main())
