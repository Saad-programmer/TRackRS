#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Fail CI on uncovered terrain art, missing provenance or changed asset bytes."""
import hashlib
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=ROOT/'data/terrain/pbr'
m=json.loads((p/'PROVENANCE.json').read_text())
assert m['schema']=='trackrs.terrain-provenance/1'
assert m['license']=='CC0-1.0.txt'
assert 'CC0 1.0 Universal' in (p/m['license']).read_text()
assert m['generator_sha256']==hashlib.sha256((ROOT/'tools/bake_terrain_materials.py').read_bytes()).hexdigest(), 'generator provenance is stale'
names=json.loads((p/'surfaces.json').read_text())['layers']
assert len(names)==9 and [s['name'] for s in m['sets']]==names
covered=set()
for i,s in enumerate(m['sets']):
    assert s['spdx']=='CC0-1.0' and s['author'] and s['source'] and s['method']
    assert s['seed']==i+31 and s['recipe']=={'generator':'tools/bake_terrain_materials.py','layer':i}
    expected={f'{s["name"]}-{kind}.{ext}' for kind in ('albedo','normal','orm') for ext in ('png','ktx2')}|{f'{s["name"]}.material.json'}
    assert set(s['files'])==expected, 'incomplete set provenance'
    for name,sha in s['files'].items():
        assert hashlib.sha256((p/name).read_bytes()).hexdigest()==sha, 'changed/unreviewed asset: '+name
    covered.update(expected)
actual={f.name for f in p.iterdir() if f.suffix in ('.png','.ktx2') or f.name.endswith('.material.json')}
assert covered==actual, 'unlicensed/uncovered terrain assets'
print('PASS: CC0-1.0 provenance, author, source, recipe and SHA256 for all 9 terrain sets / 63 files')

# Amendment A3.1 desert set: its own directory, generator and provenance.
d=ROOT/'data/terrain/desert'
m=json.loads((d/'PROVENANCE.json').read_text())
assert m['schema']=='trackrs.terrain-provenance/1' and m['license']=='LICENSE.txt'
assert 'CC0 1.0 Universal' in (d/m['license']).read_text()
assert m['generator_sha256']==hashlib.sha256((ROOT/'tools/bake_desert_materials.py').read_bytes()).hexdigest(), 'desert generator provenance is stale'
names=json.loads((d/'surfaces.json').read_text())['layers']
assert len(names)==9 and [s['name'] for s in m['sets']]==names
covered=set()
for i,s in enumerate(m['sets']):
    assert s['spdx']=='CC0-1.0' and s['author'] and s['source'] and s['structure']
    assert s['seed']==401+i and s['recipe']=={'generator':'tools/bake_desert_materials.py','layer':i}
    expected={f'{s["name"]}-{kind}.{ext}' for kind in ('albedo','normal','orm') for ext in ('png','ktx2')}|{f'{s["name"]}.material.json'}
    assert set(s['files'])==expected, 'incomplete desert set provenance'
    for name,sha in s['files'].items():
        assert hashlib.sha256((d/name).read_bytes()).hexdigest()==sha, 'changed/unreviewed desert asset: '+name
    covered.update(expected)
actual={f.name for f in d.iterdir() if f.suffix in ('.png','.ktx2') or f.name.endswith('.material.json')}
assert covered==actual, 'unlicensed/uncovered desert terrain assets'
print('PASS: CC0-1.0 provenance for all 9 desert terrain sets / 63 files')
