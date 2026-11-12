#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Bilingual (French / Arabic) event sign atlas for pack morocco-sahara (M2).

Writes textures/signs/signs.png (4 x 2 cells of 256 x 128, one per sign face)
and textures/signs/signs.json (the texts, for review). Text is shaped with
raqm (PIL) from the OFL Noto fonts in tools/fonts. Everything belongs to the
fictional event "Rallye Sahara Draa". No real organiser, sponsor or official
road-sign design is reproduced.

Every Arabic line is marked "verified": false until a human reviewer confirms
it (amendment A10, cultural review). Digits are Latin by default (A10).
Usage: tools/bake_sahara_signs.py [--check]
"""
import argparse
import json
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont, features

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "data/packs/morocco-sahara/textures/signs"
CELL_W, CELL_H, COLS, ROWS = 256, 128, 4, 2

# face index -> (French, Arabic). Order is the "face" id used in props.json.
FACES = [
    ("RALLYE SAHARA DRAÂ", "رالي صحراء درعة"),
    ("DÉPART", "الانطلاق"),
    ("ATTENTION COURSE", "انتباه سباق"),
    ("ZAGORA 4 km", "زاكورة 4 كلم"),
    ("ARRIVÉE", "الوصول"),
    ("ZONE SPECTATEURS", "منطقة المتفرجين"),
]
PANEL, BORDER, INK = (238, 228, 208), (168, 84, 48), (40, 34, 30)


def fit(draw, text, path, size, width, direction=None):
    while size > 8:
        font = ImageFont.truetype(str(path), size, layout_engine=ImageFont.Layout.RAQM)
        box = draw.textbbox((0, 0), text, font=font, direction=direction)
        if box[2] - box[0] <= width:
            return font, box
        size -= 1
    raise ValueError(f"cannot fit {text!r}")


def bake():
    if not features.check("raqm"):
        raise SystemExit("PIL lacks raqm: Arabic cannot be shaped")
    atlas = Image.new("RGB", (CELL_W * COLS, CELL_H * ROWS), PANEL)
    d = ImageDraw.Draw(atlas)
    latin = ROOT / "tools/fonts/NotoSans-Bold.ttf"
    arabic = ROOT / "tools/fonts/NotoNaskhArabic-Bold.ttf"
    for i in range(COLS * ROWS):
        x0, y0 = (i % COLS) * CELL_W, (i // COLS) * CELL_H
        d.rectangle([x0, y0, x0 + CELL_W - 1, y0 + CELL_H - 1], fill=PANEL)
        d.rectangle([x0 + 4, y0 + 4, x0 + CELL_W - 5, y0 + CELL_H - 5], outline=BORDER, width=6)
        if i >= len(FACES):
            continue
        fr, ar = FACES[i]
        font, box = fit(d, fr, latin, 34, CELL_W - 36)
        d.text((x0 + (CELL_W - (box[2] - box[0])) / 2 - box[0], y0 + 22 - box[1]), fr, font=font, fill=INK)
        font, box = fit(d, ar, arabic, 40, CELL_W - 36, "rtl")
        d.text((x0 + (CELL_W - (box[2] - box[0])) / 2 - box[0], y0 + 64 - box[1]), ar, font=font, fill=INK,
               direction="rtl")
    meta = {"schema": "trackrs.signs/1", "cell": [CELL_W, CELL_H], "grid": [COLS, ROWS],
            "faces": [{"face": i, "fr": fr, "ar": ar, "verified": False} for i, (fr, ar) in enumerate(FACES)],
            "note": "Fictional event signage. The Arabic text awaits human review (verified:false)."}
    return atlas, json.dumps(meta, ensure_ascii=False, indent=1) + "\n"


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--check", action="store_true")
    a = p.parse_args()
    atlas, meta = bake()
    png, js = OUT / "signs.png", OUT / "signs.json"
    if a.check:
        stale = [str(x.relative_to(ROOT)) for x, ok in (
            (png, png.exists() and Image.open(png).convert("RGB").tobytes() == atlas.tobytes()),
            (js, js.exists() and js.read_text(encoding="utf-8") == meta)) if not ok]
        if stale:
            print("stale (run tools/bake_sahara_signs.py):", *stale)
            return 1
        print("PASS: sign atlas is fresh")
        return 0
    OUT.mkdir(parents=True, exist_ok=True)
    atlas.save(png, optimize=True)
    js.write_text(meta, encoding="utf-8")
    print("wrote", png.relative_to(ROOT), js.relative_to(ROOT))
    return 0


if __name__ == "__main__":
    sys.exit(main())
