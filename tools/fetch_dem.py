#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Fetch Tilezen terrarium elevation tiles for a stage, with provenance.

Downloads the zoom-z tiles covering a square of `extent_m` metres centred on
(lat, lon) from the public AWS Open Data bucket `elevation-tiles-prod`, and
writes `provenance.json` with, per tile: URL, SHA-256, Last-Modified and the
`x-amz-meta-x-imagery-sources` header naming the source rasters (e.g.
`srtm/N30W006.tif`). It also stores the SHA-256 and the exact SRTM passage of
Tilezen's attribution document, so the licence statement used by the pack is
traceable. Tiles are intermediate data (not shipped); the stage heightmap
derived from them is.

Usage: tools/fetch_dem.py --centre LAT LON --extent-m 8192 --zoom 13 --out <dir>
"""
import argparse
import hashlib
import json
import math
import sys
import urllib.request
from pathlib import Path

BUCKET = "https://s3.amazonaws.com/elevation-tiles-prod/terrarium"
ATTRIBUTION = "https://raw.githubusercontent.com/tilezen/joerd/master/docs/attribution.md"


def get(url):
    req = urllib.request.Request(url, headers={"User-Agent": "trackrs-content-tools"})
    with urllib.request.urlopen(req, timeout=60) as r:
        return r.read(), {k.lower(): v for k, v in r.headers.items()}


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--centre", nargs=2, type=float, required=True, metavar=("LAT", "LON"))
    p.add_argument("--extent-m", type=float, required=True)
    p.add_argument("--zoom", type=int, default=13)
    p.add_argument("--out", type=Path, required=True)
    a = p.parse_args()
    a.out.mkdir(parents=True, exist_ok=True)
    lat, lon = a.centre
    n = 2 ** a.zoom
    cx = (lon + 180) / 360 * n * 256
    cy = (1 - math.asinh(math.tan(math.radians(lat))) / math.pi) / 2 * n * 256
    ground = math.cos(math.radians(lat)) * 2 * math.pi * 6378137 / (256 * n)
    half = a.extent_m / ground / 2
    tiles = []
    for ty in range(int((cy - half) // 256), int((cy + half) // 256) + 1):
        for tx in range(int((cx - half) // 256), int((cx + half) // 256) + 1):
            url = f"{BUCKET}/{a.zoom}/{tx}/{ty}.png"
            data, headers = get(url)
            (a.out / f"{a.zoom}_{tx}_{ty}.png").write_bytes(data)
            tiles.append({"url": url, "sha256": hashlib.sha256(data).hexdigest(),
                          "last_modified": headers.get("last-modified"),
                          "imagery_sources": headers.get("x-amz-meta-x-imagery-sources")})
            print("fetched", url, tiles[-1]["imagery_sources"], flush=True)
    doc, _ = get(ATTRIBUTION)
    text = doc.decode("utf-8")
    start = text.find("### SRTM")
    end = text.find("###", start + 5)
    provenance = {
        "dataset": "Tilezen terrarium elevation tiles (AWS Open Data: elevation-tiles-prod)",
        "centre": [lat, lon], "extent_m": a.extent_m, "zoom": a.zoom,
        "ground_resolution_m_per_px": ground,
        "tiles": tiles,
        "sources": sorted({s.strip() for t in tiles for s in (t["imagery_sources"] or "").split(",") if s.strip()}),
        "attribution_document": {"url": ATTRIBUTION, "sha256": hashlib.sha256(doc).hexdigest(),
                                 "srtm_section": text[start:end].strip() if start >= 0 else None},
        "credit": "SRTM data courtesy of the U.S. Geological Survey",
    }
    (a.out / "provenance.json").write_text(json.dumps(provenance, indent=2) + "\n")
    print("sources:", ", ".join(provenance["sources"]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
