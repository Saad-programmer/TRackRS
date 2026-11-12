#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Original CC0 desert terrain materials (amendment A3.1); no downloaded art.

Nine 64x64 tileable PBR sets (albedo sRGB, tangent normal, ORM = AO/rough/metal)
matching the legacy terrain array size, each built from seeded periodic noise
plus a structure of its own:
  sand_dune      warm orange grain, faint wind ripples (more ripples are
                 added in the shader, aligned with the scene wind)
  fesh_fesh      pale powdery talc-like dust, almost featureless, very rough
  hard_sand      compacted tan sand with scattered small pebbles
  hamada_stone   dark varnished stones (Voronoi cells) on lighter ground
  oued_gravel    rounded grey-beige river pebbles (Voronoi, domed normals)
  dry_lake_clay  pale clay plates separated by polygonal cracks
  palmeraie_dirt dark irrigated soil with fibrous organic specks
  rock           layered sandstone/limestone strata with fractures
  tarmac_n9      worn dark asphalt with sand drifted into the pores
Writes data/terrain/desert/{name}-{albedo,normal,orm}.{png,ktx2},
{name}.material.json, surfaces.json, PROVENANCE.json (CC0-1.0, recipe, seed,
SHA-256 of every file and of this generator). tests/check_terrain_licenses.py
verifies all of it.
Usage: tools/bake_desert_materials.py --bin <build>/bin [--output data/terrain/desert]
"""
import argparse
import hashlib
import json
import math
import subprocess
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SIZE = 64
# name, base sRGB, roughness, normal strength, albedo contrast, structure
SURFACES = [
    ("sand_dune", (224, 158, 92), 0.86, 0.10, 0.07, "ripples"),
    ("fesh_fesh", (234, 206, 162), 0.95, 0.04, 0.04, "powder"),
    ("hard_sand", (206, 156, 96), 0.88, 0.16, 0.10, "pebbles"),
    ("hamada_stone", (170, 122, 84), 0.82, 0.42, 0.10, "stones"),
    ("oued_gravel", (176, 152, 120), 0.84, 0.55, 0.12, "gravel"),
    ("dry_lake_clay", (226, 202, 166), 0.92, 0.30, 0.05, "cracks"),
    ("palmeraie_dirt", (112, 80, 56), 0.93, 0.22, 0.16, "organic"),
    ("rock", (160, 120, 92), 0.80, 0.50, 0.14, "strata"),
    ("tarmac_n9", (62, 60, 62), 0.80, 0.12, 0.05, "asphalt"),
]


def periodic_noise(rng, periods=(4, 8, 16, 32), weights=(.4, .3, .2, .1)):
    out = np.zeros((SIZE, SIZE))
    y, x = np.mgrid[0:SIZE, 0:SIZE] / SIZE
    for period, w in zip(periods, weights):
        g = rng.random((period, period))
        u, v = x * period, y * period
        i, j = np.floor(u).astype(int), np.floor(v).astype(int)
        fu, fv = u - i, v - j
        fu, fv = fu * fu * (3 - 2 * fu), fv * fv * (3 - 2 * fv)
        a, b = g[j % period, i % period], g[j % period, (i + 1) % period]
        c, d = g[(j + 1) % period, i % period], g[(j + 1) % period, (i + 1) % period]
        out += w * ((a * (1 - fu) + b * fu) * (1 - fv) + (c * (1 - fu) + d * fu) * fv)
    return out / sum(weights)


def voronoi(rng, cells):
    """Tileable Voronoi: distance to nearest and second-nearest seed (px), cell id."""
    pts = rng.random((cells, 2)) * SIZE
    y, x = np.mgrid[0:SIZE, 0:SIZE].astype(float)
    d = []
    for ox in (-SIZE, 0, SIZE):
        for oy in (-SIZE, 0, SIZE):
            d.append(np.hypot(x[..., None] - (pts[:, 0] + ox), y[..., None] - (pts[:, 1] + oy)))
    d = np.concatenate(d, axis=-1)
    order = np.argsort(d, axis=-1)
    f1 = np.take_along_axis(d, order[..., :1], -1)[..., 0]
    f2 = np.take_along_axis(d, order[..., 1:2], -1)[..., 0]
    return f1, f2, order[..., 0] % cells


def build(name, base, rough, strength, contrast, kind, seed):
    rng = np.random.default_rng(seed)
    n = periodic_noise(rng) - 0.5
    height = n.copy()
    tint = np.zeros((SIZE, SIZE, 3))
    y, x = np.mgrid[0:SIZE, 0:SIZE] / SIZE
    rough_map = np.full((SIZE, SIZE), rough) + 0.04 * n
    ao = np.ones((SIZE, SIZE))
    if kind == "ripples":
        phase = (x * 2 + y * 6) * 2 * math.pi + n * 3
        height += 0.35 * np.sin(phase)
    elif kind == "powder":
        height *= 0.3
    elif kind in ("pebbles", "stones", "gravel"):
        cells = {"pebbles": 24, "stones": 18, "gravel": 40}[kind]
        f1, f2, cid = voronoi(rng, cells)
        radius = {"pebbles": 2.2, "stones": 6.5, "gravel": 4.2}[kind]
        size_var = 0.6 + 0.8 * rng.random(cells)[cid]
        dome = np.clip(1 - f1 / (radius * size_var), 0, 1)
        keep = rng.random(cells)[cid] < {"pebbles": 0.35, "stones": 0.85, "gravel": 0.95}[kind]
        dome *= keep
        height += dome ** 0.6 * (1.4 if kind != "pebbles" else 0.8)
        shade = 0.55 + 0.45 * rng.random(cells)[cid]
        if kind == "stones":   # desert varnish: dark brown-black stones
            col = np.array([96, 62, 40]) / 255   # varnished stones: dark red-brown
        elif kind == "gravel":
            grey = 0.5 + 0.3 * rng.random(cells)          # neutral grey-beige river pebbles
            warm = 0.03 * rng.random(cells)
            col = np.stack([grey + warm, grey + 0.6 * warm, grey - 0.02], -1)[cid]
        else:
            col = np.array([150, 130, 112]) / 255
        tint += (dome[..., None] > 0.05) * (np.broadcast_to(col, (SIZE, SIZE, 3)) * shade[..., None] - np.array(base) / 255)
        ao *= 1 - 0.35 * np.clip((f2 - f1) < 1.5, 0, 1) * keep
        rough_map = np.where(dome > 0.05, rough_map - 0.12, rough_map)
    elif kind == "cracks":
        f1, f2, cid = voronoi(rng, 9)
        crack = np.clip(1 - (f2 - f1) / 1.4, 0, 1)
        height += -1.2 * crack + 0.15 * rng.random(9)[cid]
        tint -= crack[..., None] * np.array([0.32, 0.30, 0.28])
        ao *= 1 - 0.5 * crack
    elif kind == "organic":
        specks = (periodic_noise(rng, (16, 32), (.5, .5)) > 0.62).astype(float)
        tint += specks[..., None] * np.array([-0.08, -0.04, -0.06])
        height += 0.4 * specks
    elif kind == "strata":
        bands = np.sin((y * 5 + n * 0.8) * 2 * math.pi)
        height += 0.6 * np.abs(bands)
        tint += bands[..., None] * np.array([0.05, 0.04, 0.03])
        f1, f2, _ = voronoi(rng, 6)
        fracture = np.clip(1 - (f2 - f1) / 1.0, 0, 1)
        height -= fracture
        ao *= 1 - 0.4 * fracture
    elif kind == "asphalt":
        sand = np.clip((periodic_noise(rng, (8, 16), (.6, .4)) - 0.55) * 4, 0, 1)
        tint += sand[..., None] * (np.array([196, 168, 124]) / 255 - np.array(base) / 255) * 0.5
        rough_map += 0.08 * sand
    albedo = np.clip(np.array(base) / 255 * (1 + contrast * 2 * n[..., None]) + tint, 0, 1)
    dx = (np.roll(height, -1, 1) - np.roll(height, 1, 1)) * strength
    dy = (np.roll(height, -1, 0) - np.roll(height, 1, 0)) * strength
    length = np.sqrt(dx * dx + dy * dy + 1)
    normal = np.stack([-dx / length, -dy / length, 1 / length], -1) * 0.5 + 0.5
    orm = np.stack([np.clip(ao, 0, 1), np.clip(rough_map, 0.04, 1), np.zeros((SIZE, SIZE))], -1)
    return [(albedo * 255).round().astype(np.uint8), (normal * 255).round().astype(np.uint8),
            (orm * 255).round().astype(np.uint8)]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--bin", type=Path, required=True)
    p.add_argument("--output", type=Path, default=ROOT / "data/terrain/desert")
    a = p.parse_args()
    out = a.output
    out.mkdir(parents=True, exist_ok=True)
    sets = []
    for layer, (name, base, rough, strength, contrast, kind) in enumerate(SURFACES):
        seed = 401 + layer
        files = []
        for image, semantic in zip(build(name, base, rough, strength, contrast, kind, seed), ("albedo", "normal", "orm")):
            png = out / f"{name}-{semantic}.png"
            Image.fromarray(image, "RGB").save(png)
            subprocess.run([str(a.bin.resolve() / "bake-material"), "--input", str(png), "--semantic", semantic,
                            "--output", str(png.with_suffix(".ktx2"))], check=True)
            files += [png, png.with_suffix(".ktx2")]
        material = out / f"{name}.material.json"
        material.write_text(json.dumps({"name": name, "albedo": f"{name}-albedo.ktx2", "normal": f"{name}-normal.ktx2",
                                        "orm": f"{name}-orm.ktx2", "atlas": False}, indent=2) + "\n")
        files.append(material)
        sets.append({"name": name, "author": "TrackRS project (generated by tools/bake_desert_materials.py)",
                     "spdx": "CC0-1.0", "source": "Original analytic generation; no downloaded artwork",
                     "seed": seed, "structure": kind,
                     "recipe": {"generator": "tools/bake_desert_materials.py", "layer": layer},
                     "parameters": {"base_rgb_srgb8": list(base), "roughness": rough, "normal_strength": strength,
                                    "albedo_contrast": contrast},
                     "files": {f.name: digest(f) for f in files}})
    (out / "surfaces.json").write_text(json.dumps({"layers": [s[0] for s in SURFACES], "size": SIZE,
                                                   "source": "original seeded procedural desert surfaces; CC0"},
                                                  indent=2) + "\n")
    (out / "LICENSE.txt").write_text("Original TrackRS desert terrain material inputs and generated textures.\n"
                                     "Dedicated to the public domain under CC0 1.0 Universal.\n"
                                     "https://creativecommons.org/publicdomain/zero/1.0/\n")
    (out / "PROVENANCE.json").write_text(json.dumps({
        "schema": "trackrs.terrain-provenance/1", "license": "LICENSE.txt",
        "license_url": "https://creativecommons.org/publicdomain/zero/1.0/legalcode",
        "generator_sha256": digest(ROOT / "tools/bake_desert_materials.py"), "sets": sets}, indent=2) + "\n")
    print("wrote", len(sets), "desert material sets to", out)


if __name__ == "__main__":
    main()
