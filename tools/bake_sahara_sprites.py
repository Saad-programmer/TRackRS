#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Original vegetation cards + species for pack morocco-sahara (amendment A3.5).

Draws simple seeded card sprites (RGBA PNG, 128x256, transparent background)
for the pack's desert plants and writes the matching species catalogue
(trackrs.vegetation/1) used by the 3D vegetation renderer near the camera;
the cards are the far impostors and the legacy-renderer look.
Species (sprite basename -> form):
  sahara-date-palm-tall / -mid / -young   palm, dead-frond skirt 0.6 / 0.4 / 0.1
  sahara-tamarisk                         bush (feathery grey-green)
  sahara-acacia                           acacia (umbrella crown)
  sahara-alfa-grass                       grass (tough pale tufts)
  sahara-dead-brush                       bush (dry twigs, no green)
All shapes and colours are authored here; no artwork is read.
Usage: tools/bake_sahara_sprites.py [--check]
"""
import argparse
import json
import math
import random
import sys
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
PACK = ROOT / "data/packs/morocco-sahara"
W, H = 128, 256

SPECIES = {
    "sahara-date-palm-tall.png": {"form": "palm", "foliage": "#566b2e", "bark": "#6e5a40", "width": 0.72, "dead_fronds": 0.6},
    "sahara-date-palm-mid.png": {"form": "palm", "foliage": "#5b7232", "bark": "#6e5a40", "width": 0.8, "dead_fronds": 0.4},
    "sahara-date-palm-young.png": {"form": "palm", "foliage": "#62793a", "bark": "#76624a", "width": 0.9, "dead_fronds": 0.1},
    "sahara-tamarisk.png": {"form": "bush", "foliage": "#7d8a6a", "bark": "#5a4a3a", "width": 0.95},
    "sahara-acacia.png": {"form": "acacia", "foliage": "#56632f", "bark": "#4a3a2a", "width": 0.98},
    "sahara-alfa-grass.png": {"form": "grass", "foliage": "#b3a774", "width": 0.9},
    "sahara-dead-brush.png": {"form": "bush", "foliage": "#6e5844", "bark": "#5a4636", "width": 0.85},
}


def rgb(hexcolour, k=1.0):
    return tuple(max(0, min(255, round(int(hexcolour[i:i + 2], 16) * k))) for i in (1, 3, 5))


def palm(d, rng, entry, crown_y, dead):
    bark, leaf = entry["bark"], entry["foliage"]
    # Trunk: slightly curved, ringed.
    for y in range(crown_y, H):
        t = (y - crown_y) / (H - crown_y)
        x = W / 2 + 6 * (1 - t) ** 2
        w = 4 + 3 * t
        d.line([(x - w, y), (x + w, y)], fill=rgb(bark, 0.8 + 0.25 * ((y // 5) % 2)) + (255,))
    # Dead-frond skirt.
    for i in range(int(14 * dead)):
        a = math.pi * (0.15 + 0.7 * i / max(1, int(14 * dead) - 1))
        d.line([(W / 2, crown_y), (W / 2 + math.cos(a) * 30, crown_y + 34 + math.sin(a) * 6)],
               fill=(96, 70, 40, 255), width=3)
    # Green fronds arching out of the crown.
    for i in range(16):
        a = math.pi + math.pi * i / 15
        pts = []
        for s in range(9):
            r = s / 8 * (52 + rng.random() * 10)
            pts.append((W / 2 + math.cos(a) * r, crown_y + math.sin(a) * r * 0.55 + (s / 8) ** 2 * 26))
        d.line(pts, fill=rgb(leaf, 0.8 + 0.4 * rng.random()) + (255,), width=4)


def bush(d, rng, entry, dry=False):
    leaf = entry["foliage"]
    for i in range(70 if not dry else 40):
        x0 = W / 2 + rng.gauss(0, 10)
        a = math.pi * (1.15 + 0.7 * rng.random())
        length = 40 + 70 * rng.random()
        x1, y1 = x0 + math.cos(a) * length * 0.5, H - 10 + math.sin(a) * length * 1.3
        colour = rgb(entry.get("bark", "#5a4636"), 0.9) if dry else rgb(leaf, 0.75 + 0.5 * rng.random())
        d.line([(x0, H - 6), (x1, y1)], fill=colour + (255,), width=2 if dry else 3)
        if not dry:
            for k in range(5):
                t = k / 4
                d.ellipse([x0 + (x1 - x0) * t - 4, H - 6 + (y1 - H + 6) * t - 4,
                           x0 + (x1 - x0) * t + 4, H - 6 + (y1 - H + 6) * t + 4], fill=colour + (255,))


def acacia(d, rng, entry):
    bark, leaf = entry["bark"], entry["foliage"]
    d.line([(W / 2, H), (W / 2 + 2, H - 120)], fill=rgb(bark) + (255,), width=6)
    for side in (-1, 1):
        d.line([(W / 2 + 2, H - 110), (W / 2 + side * 34, H - 168)], fill=rgb(bark) + (255,), width=4)
    for i in range(160):
        x = W / 2 + rng.gauss(0, 30)
        y = H - 175 + rng.gauss(0, 6)
        d.ellipse([x - 5, y - 3, x + 5, y + 3], fill=rgb(leaf, 0.75 + 0.45 * rng.random()) + (255,))


def grass(d, rng, entry):
    for i in range(60):
        x0 = W / 2 + rng.gauss(0, 14)
        a = math.pi * (1.2 + 0.6 * rng.random())
        length = 60 + 70 * rng.random()
        d.line([(x0, H - 2), (x0 + math.cos(a) * length * 0.6, H - 2 + math.sin(a) * length)],
               fill=rgb(entry["foliage"], 0.7 + 0.5 * rng.random()) + (255,), width=2)


def draw(name, entry, seed):
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    rng = random.Random(seed)
    form = entry["form"]
    if form == "palm":
        crown = {"tall": 40, "mid": 90, "young": 150}[name.split("-")[-1][:-4]]
        palm(d, rng, entry, crown, entry["dead_fronds"])
    elif form == "acacia":
        acacia(d, rng, entry)
    elif form == "grass":
        grass(d, rng, entry)
    else:
        bush(d, rng, entry, dry="dead" in name)
    return img


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--check", action="store_true")
    a = p.parse_args()
    out = PACK / "textures/veg"
    catalogue = {"schema": "trackrs.vegetation/1", "license": "GPL-2.0-or-later",
                 "provenance": "Desert species and card sprites authored for TrackRS by tools/bake_sahara_sprites.py; no artwork is copied.",
                 "sprites": SPECIES}
    text = json.dumps(catalogue, indent=1, sort_keys=True) + "\n"
    stale = []
    for i, (name, entry) in enumerate(sorted(SPECIES.items())):
        img = draw(name, entry, 9100 + i)
        path = out / name
        if a.check:
            if not path.exists() or Image.open(path).convert("RGBA").tobytes() != img.tobytes():
                stale.append(str(path.relative_to(ROOT)))
        else:
            out.mkdir(parents=True, exist_ok=True)
            img.save(path, optimize=True)
    species = PACK / "vegetation/species.json"
    if a.check:
        if not species.exists() or species.read_text() != text:
            stale.append(str(species.relative_to(ROOT)))
        if stale:
            print("stale (run tools/bake_sahara_sprites.py):", *stale, sep="\n  ")
            return 1
        print("PASS: sahara sprites and species are fresh")
        return 0
    species.parent.mkdir(parents=True, exist_ok=True)
    species.write_text(text)
    print("wrote", len(SPECIES), "sprites and", species.relative_to(ROOT))
    return 0


if __name__ == "__main__":
    sys.exit(main())
