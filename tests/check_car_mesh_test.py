#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Tests for tools/check_car_mesh.py with synthetic .glb files (boxes named as parts)."""
import json
import pathlib
import struct
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from check_car_mesh import PARTS_REQUIRED, check  # noqa: E402


def box(cx, cy, cz, sx, sy, sz):
    xs, ys, zs = (cx - sx / 2, cx + sx / 2), (cy - sy / 2, cy + sy / 2), (cz - sz / 2, cz + sz / 2)
    verts = [(x, y, z) for x in xs for y in ys for z in zs]
    idx = [0, 1, 2, 1, 3, 2, 4, 6, 5, 5, 6, 7, 0, 4, 1, 1, 4, 5, 2, 3, 6, 3, 7, 6, 0, 2, 4, 2, 6, 4, 1, 5, 3, 3, 5, 7]
    return verts, idx


def glb(parts, materials=('paint',), png=(1024, 1024), extra_tris=None):
    """parts: name -> (centre, size) in glTF axes (x right, y up, z backwards), metres."""
    blob, views, accessors, meshes, nodes = b'', [], [], [], []
    for name, (centre, size) in parts.items():
        lod_centre = (0.0, 0.0, 0.0) if name.startswith('wheel_') else centre
        verts, idx = box(*lod_centre, *size)
        vb = b''.join(struct.pack('<fff', *v) for v in verts)
        ib = b''.join(struct.pack('<H', i) for i in idx)
        views += [{'buffer': 0, 'byteOffset': len(blob), 'byteLength': len(vb)},
                  {'buffer': 0, 'byteOffset': len(blob) + len(vb), 'byteLength': len(ib)}]
        blob += vb + ib
        while len(blob) % 4:
            blob += b'\0'
        lo = [min(v[i] for v in verts) for i in range(3)]
        hi = [max(v[i] for v in verts) for i in range(3)]
        count = (extra_tris or {}).get(name, len(idx) // 3) * 3
        accessors += [{'bufferView': len(views) - 2, 'componentType': 5126, 'count': 8, 'type': 'VEC3', 'min': lo, 'max': hi},
                      {'bufferView': len(views) - 1, 'componentType': 5123, 'count': count, 'type': 'SCALAR'}]
        meshes.append({'primitives': [{'attributes': {'POSITION': len(accessors) - 2}, 'indices': len(accessors) - 1,
                                       'material': 0}]})
        node = {'name': name, 'mesh': len(meshes) - 1}
        if name.startswith('wheel_'):
            node['translation'] = list(centre)
        nodes.append(node)
    images = []
    if png:
        header = b'\x89PNG\r\n\x1a\n' + struct.pack('>I', 13) + b'IHDR' + struct.pack('>II', *png) + b'\x08\x06\0\0\0'
        views.append({'buffer': 0, 'byteOffset': len(blob), 'byteLength': len(header)})
        blob += header
        while len(blob) % 4:
            blob += b'\0'
        images.append({'bufferView': len(views) - 1, 'mimeType': 'image/png'})
    doc = {'asset': {'version': '2.0'}, 'scene': 0, 'scenes': [{'nodes': list(range(len(nodes)))}], 'nodes': nodes,
           'meshes': meshes, 'accessors': accessors, 'bufferViews': views, 'buffers': [{'byteLength': len(blob)}],
           'materials': [{'name': m} for m in materials], 'images': images}
    js = json.dumps(doc).encode()
    while len(js) % 4:
        js += b' '
    body = struct.pack('<II', len(js), 0x4E4F534A) + js + struct.pack('<II', len(blob), 0x004E4942) + blob
    return b'glTF' + struct.pack('<II', 2, 12 + len(body)) + body


def car(length=4.4, width=1.95, height=1.8, radius=0.45, along_x=False):
    """A plausible raid car in glTF axes: front towards -Z, resting on y = 0."""
    L, W = length, width
    size = (L, height, W) if along_x else (W, height, L)   # along_x: the car's length lies on X
    p = {'body': ((0, height / 2, 0), size)}
    for name in PARTS_REQUIRED:
        if name == 'body':
            continue
        if name.startswith('wheel_'):
            x = (W / 2 - .2) * (1 if name[-1] == 'r' else -1)
            z = (L / 2 - .7) * (-1 if name[-2] == 'f' else 1)
            p[name] = ((x, radius, z), (.3, 2 * radius, 2 * radius))
        else:
            p[name] = ((0, height / 2, 0), (.5, .3, .3))
    return p


def run(blob, cls='rally-raid', radius=0.45):
    with tempfile.TemporaryDirectory() as d:
        path = pathlib.Path(d) / 'm.glb'
        path.write_bytes(blob)
        return check(path, cls, radius)[0]


def test():
    assert run(glb(car())) == [], run(glb(car()))
    parts = car()
    del parts['mirror_left']
    assert any('mirror_left' in p for p in run(glb(parts)))                       # separate mirror required
    assert any('-Z' in p or 'outside' in p for p in run(glb(car(along_x=True))))  # front along X, not -Z
    cm = {k: (tuple(c * 100 for c in v[0]), tuple(s * 100 for s in v[1])) for k, v in car().items()}
    assert any('outside' in p for p in run(glb(cm), radius=45))                    # centimetres, not metres
    assert any('not in the allowed list' in p for p in run(glb(car(), materials=('Material.001',))))
    assert any('triangles >' in p for p in run(glb(car(), extra_tris={'body': 90000})))
    assert any('tyre radius' in p for p in run(glb(car()), radius=0.40))
    assert any('power-of-two' in p for p in run(glb(car(), png=(1000, 1024))))
    assert any('exceeds' in p for p in run(glb(car(), png=(4096, 4096))))
    unknown = car()
    unknown['wing_left'] = ((0, 1, 0), (.1, .1, .1))
    assert any('unknown part' in p for p in run(glb(unknown)))
    print('PASS: check_car_mesh accepts a conforming mesh and rejects 9 kinds of defect')


if __name__ == '__main__':
    test()
