#!/usr/bin/env python3
"""Validate shipping KTX2 with Khronos tooling and complete material manifests."""
import argparse
import json
from pathlib import Path
import subprocess
ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
p.add_argument('--ktx', required=True)
p.add_argument('--material-validator', required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
results = []
for path in sorted([*ROOT.glob('data/**/*.ktx2'), *ROOT.glob('tests/fixtures/**/*.ktx2')]):
    subprocess.run([a.ktx, 'validate', str(path)], check=True)
    results.append({'file': str(path.relative_to(ROOT)), 'khronos': 'pass'})
for path in sorted(ROOT.glob('data/**/*.material.json')):
    subprocess.run([a.material_validator, '--material', str(path)], check=True)
    results.append({'file': str(path.relative_to(ROOT)), 'material': 'pass'})
a.output.parent.mkdir(parents=True, exist_ok=True)
a.output.write_text(json.dumps(results, indent=2)+'\n')
print(f'PASS: {len(results)} texture/material validation checks')
