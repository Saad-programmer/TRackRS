#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Writes data/vehicles/<id>/livery.json for the placeholder real-car entries.

Phase 4b. Every livery here is FICTIONAL: plain original colour schemes and
race numbers, chosen to avoid any real team or sponsor livery (no Martini,
555, Castrol, Marlboro, Total, Rothmans, ... schemes or colours). Sponsor
slots receive procedurally generated fictional logos (engine/car/car_assets).

Source colours were measured from the fallback atlases (12-colour quantised
histogram mode of the paint) and number-panel rects from the white panels in
the fox_wrc atlas (connected components); the evo_rally panels are split by
the UV layout, so evo-based cars keep the authored number.

Usage: tools/seed_liveries.py [--check]
"""
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ATLAS = 1024.0

def rect(x, y, w, h, rotation=0):
    return {"rect": [round(x / ATLAS, 5), round(y / ATLAS, 5), round(w / ATLAS, 5), round(h / ATLAS, 5)], "rotation": rotation}

FALLBACK = {
    "fox_wrc": {"primary_source": "#a18a2b", "secondary_source": "#d6d1b5",
                "number_panels": [rect(191, 703, 100, 65), rect(263, 899, 100, 65)],
                "sponsor_slots": [rect(330, 712, 120, 30), rect(400, 908, 120, 30)]},
    "evo_rally": {"primary_source": "#bf121f", "secondary_source": "#e3e3e3",
                  "number_panels": [], "sponsor_slots": []},
}
# id: (fallback, primary, secondary, number, seed)
CARS = {
    "alpine_a110_1800":         ("fox_wrc",   "#e8731a", None,      8,  101),
    "bmw_m3_e30_group_a":       ("fox_wrc",   "#e9d36a", "#2b2b30", 21, 102),
    "lancia_stratos_hf":        ("fox_wrc",   "#8cbf26", "#1d1f22", 5,  103),
    "porsche_911_sc_rs":        ("fox_wrc",   "#e6dfc4", "#7a1f2e", 11, 104),
    "audi_quattro_a2":          ("evo_rally", "#1d5a35", "#b9bcc0", -1, 105),
    "ford_escort_rs_cosworth":  ("evo_rally", "#7f1420", "#a7abb0", -1, 106),
    "lancia_delta_hf_integrale":("evo_rally", "#5a3590", None,      -1, 107),
    "peugeot_205_t16_e2":       ("evo_rally", "#1a2766", "#d8dde3", -1, 108),
    "peugeot_206_wrc":          ("evo_rally", "#5a9bd9", None,      -1, 109),
    "subaru_impreza_wrc97":     ("evo_rally", "#9a6633", "#2a2a2a", -1, 110),
    "toyota_celica_gt4_st185":  ("evo_rally", "#1a8080", None,      -1, 111),
}

def livery(car_id):
    fallback, primary, secondary, number, seed = CARS[car_id]
    f = FALLBACK[fallback]
    out = {"schema": "trackrs.livery/1", "id": car_id + "_fictional",
           "note": "Fictional livery (placeholder model); not a real team or sponsor scheme.",
           "primary": primary, "primary_source": f["primary_source"],
           "secondary": secondary, "secondary_source": f["secondary_source"],
           "number": number, "number_colour": "#111111", "panel_colour": "#f2f2f2",
           "number_panels": f["number_panels"] if number >= 0 else [],
           "sponsor_slots": f["sponsor_slots"], "sponsor_seed": seed}
    return json.dumps(out, indent=2) + "\n"

def main():
    check = "--check" in sys.argv
    stale = []
    for car_id in CARS:
        path = ROOT / "data/vehicles" / car_id / "livery.json"
        text = livery(car_id)
        if check:
            if not path.exists() or path.read_text() != text:
                stale.append(str(path.relative_to(ROOT)))
        else:
            path.write_text(text)
    if stale:
        print("stale liveries (run tools/seed_liveries.py):", *stale, sep="\n  ")
        return 1
    print("PASS: liveries" if check else f"wrote {len(CARS)} liveries")
    return 0

if __name__ == "__main__":
    sys.exit(main())
