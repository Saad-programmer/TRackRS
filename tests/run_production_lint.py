#!/usr/bin/env python3
"""Lint production seams and surface actionable diagnostics in CI annotations."""
import argparse
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--database', type=Path, required=True)
parser.add_argument('--compiler', default='clang-tidy')
args = parser.parse_args()
command = [args.compiler, 'engine/rhi/rhi.cpp', 'engine/rhi/core_device.cpp',
           'engine/media/movie.cpp', 'engine/render/basic.cpp', 'engine/mesh/mesh_importer.cpp',
           'engine/mesh/tangents.cpp', 'engine/pbr/renderer.cpp',
           'engine/pbr/texture_stream.cpp', 'engine/pbr/ibl.cpp',
           'engine/assets/ktx2.cpp', 'engine/assets/ibl_bake.cpp',
           'engine/assets/material_manifest.cpp',
           'engine/world/atmosphere.cpp', 'engine/world/state.cpp', 'engine/world/renderer.cpp', 'engine/world/water.cpp', 'engine/post/exposure.cpp', 'engine/post/post.cpp', 'engine/dressing/people.cpp', 'engine/dressing/renderer.cpp', 'engine/content/car_definition.cpp', 'engine/car/car_assets.cpp', 'engine/car/car_renderer.cpp', 'engine/content/pack.cpp',
           'engine/terrain/cdlod.cpp', 'engine/terrain/renderer.cpp', 'engine/terrain/tracks.cpp', 'engine/terrain/tracks_renderer.cpp', 'engine/car/damage_visual.cpp', 'engine/dressing/props.cpp', 'engine/dressing/props_renderer.cpp',
           'engine/vegetation/vegetation.cpp', 'engine/vegetation/renderer.cpp', '-p', str(args.database)]
result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
print(result.stdout, end='')
if result.returncode:
    lines = [line for line in result.stdout.splitlines() if 'error:' in line or 'warning:' in line]
    for line in (lines or result.stdout.splitlines()[-10:])[:20]:
        escaped = line.replace('%', '%25').replace('\r', '%0D').replace('\n', '%0A')
        print('::error::' + escaped)
raise SystemExit(result.returncode)
