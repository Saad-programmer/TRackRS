#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""stage-from-heightmap: build a playable legacy-format stage for a content pack.

Input: a JSON spec (see docs/STAGE_TOOL.md) naming a height source, a route,
surface rules and dressing. Output, in the stage directory:
  h.png  8-bit heightmap (first channel), with the level's verticalscale and
         blur filter chosen so the source relief is preserved;
  c.png  colour map, t.png terrain-type map (exact TERRAINMAP_MATERIALS
         colours, read from src/include/terrainmap.h), r.png road mask
         (pure white = road), f.png foliage density, mm.png minimap, ss.png;
  <id>.level  legacy level XML (terrain, foliage bands, weather, race with
         start position and checkpoints along the route);
  stage.json  trackrs.stage/1 for the pack (id, name, seed, fidelity, length).
Height sources:
  {"kind": "terrarium", "tiles": "<dir>", ...}  Tilezen terrarium PNG tiles
        fetched by tools/fetch_dem.py (provenance recorded there);
  {"kind": "procedural", ...}  seeded ridged noise, wind-aligned dune field and
        eroded wadis (amendment A2 fallback; fidelity "procedural").
Deterministic: every random choice comes from numpy.random.default_rng(seed).
Usage: tools/stage_from_heightmap.py <spec.json> <output-stage-dir>
"""
import json
import math
import re
import sys
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]


def terrain_colours():
    """Name -> RGB from the X-macro in src/include/terrainmap.h (single source)."""
    text = (ROOT / "src/include/terrainmap.h").read_text()
    out = {}
    for name, r, g, b in re.findall(r"X\((\w+),\s*RGB\(\s*(\d+),\s*(\d+),\s*(\d+)\)", text):
        out[name] = (int(r), int(g), int(b))
    if "RacingTarmac" not in out:
        raise RuntimeError("cannot parse terrainmap.h")
    return out


# ---- height sources --------------------------------------------------------------------
def terrarium(spec, size, metres_per_px):
    """Mosaic Tilezen terrarium tiles (z, x0..x1, y0..y1) and resample a
    size x size window centred on (lat, lon) at metres_per_px."""
    tiles = Path(spec["tiles"])
    z = spec["zoom"]
    lat, lon = spec["centre"]
    n = 2 ** z
    def pixel(la, lo):  # global web-mercator pixel at zoom z (256 px tiles)
        x = (lo + 180.0) / 360.0 * n * 256
        y = (1 - math.asinh(math.tan(math.radians(la))) / math.pi) / 2 * n * 256
        return x, y
    cx, cy = pixel(lat, lon)
    ground = math.cos(math.radians(lat)) * 2 * math.pi * 6378137 / (256 * n)  # metres per source px
    half = size * metres_per_px / ground / 2
    x0, x1 = int(math.floor((cx - half) / 256)), int(math.floor((cx + half) / 256))
    y0, y1 = int(math.floor((cy - half) / 256)), int(math.floor((cy + half) / 256))
    mosaic = np.zeros(((y1 - y0 + 1) * 256, (x1 - x0 + 1) * 256), np.float64)
    for ty in range(y0, y1 + 1):
        for tx in range(x0, x1 + 1):
            p = tiles / f"{z}_{tx}_{ty}.png"
            a = np.asarray(Image.open(p).convert("RGB")).astype(np.float64)
            mosaic[(ty - y0) * 256:(ty - y0 + 1) * 256, (tx - x0) * 256:(tx - x0 + 1) * 256] = \
                a[..., 0] * 256 + a[..., 1] + a[..., 2] / 256 - 32768
    # Bilinear resample to the stage grid (row 0 = north; game +y is north).
    ys = (cy - half) - y0 * 256 + (np.arange(size) + 0.5) * (2 * half / size)
    xs = (cx - half) - x0 * 256 + (np.arange(size) + 0.5) * (2 * half / size)
    gy, gx = np.meshgrid(ys - 0.5, xs - 0.5, indexing="ij")
    ix, iy = np.clip(np.floor(gx).astype(int), 0, mosaic.shape[1] - 2), np.clip(np.floor(gy).astype(int), 0, mosaic.shape[0] - 2)
    fx, fy = np.clip(gx - ix, 0, 1), np.clip(gy - iy, 0, 1)
    h = (mosaic[iy, ix] * (1 - fx) * (1 - fy) + mosaic[iy, ix + 1] * fx * (1 - fy) +
         mosaic[iy + 1, ix] * (1 - fx) * fy + mosaic[iy + 1, ix + 1] * fx * fy)
    return h


def value_noise(rng, size, cells):
    g = rng.random((cells + 3, cells + 3))
    t = np.linspace(0, cells, size, endpoint=False)
    i = t.astype(int)
    f = t - i
    f = f * f * (3 - 2 * f)
    a = g[np.ix_(i, i)]
    b = g[np.ix_(i, i + 1)]
    c = g[np.ix_(i + 1, i)]
    d = g[np.ix_(i + 1, i + 1)]
    fy, fx = f[:, None], f[None, :]
    return (a * (1 - fx) + b * fx) * (1 - fy) + (c * (1 - fx) + d * fx) * fy


def fbm(rng, size, base, octaves, ridged=False):
    out = np.zeros((size, size))
    amp, total = 1.0, 0.0
    for o in range(octaves):
        n = value_noise(rng, size, base * 2 ** o)
        if ridged:
            n = 1 - np.abs(2 * n - 1)
            n = n * n
        out += amp * n
        total += amp
        amp *= 0.5
    return out / total


def dunes(rng, size, metres_per_px, wind_deg, wavelength, height, kind):
    """Wind-aligned dune field. Crests run across the wind; profile is
    asymmetric (gentle stoss slope upwind, steep slip face downwind)."""
    yy, xx = np.mgrid[0:size, 0:size] * metres_per_px
    yy = -yy   # rows run south; wind_deg is measured from +x towards world +y (north)
    w = math.radians(wind_deg)
    along = xx * math.cos(w) + yy * math.sin(w)           # distance downwind
    across = -xx * math.sin(w) + yy * math.cos(w)
    warp = (fbm(rng, size, 3, 3) - 0.5) * wavelength * 1.2
    phase = (along + warp) / wavelength
    saw = phase - np.floor(phase)
    profile = np.where(saw < 0.78, saw / 0.78, (1 - saw) / 0.22)  # 78 % stoss, 22 % slip face
    profile = profile ** 1.5
    if kind == "barchan":   # crescents: modulate crest length across the wind
        profile *= np.clip(0.5 + 0.5 * np.sin(across / (wavelength * 0.9) * 2 * math.pi + warp / wavelength), 0, 1)
    elif kind == "star":    # second, crossing wind adds arms
        w2 = w + math.radians(70)
        p2 = (xx * math.cos(w2) + yy * math.sin(w2) + warp) / (wavelength * 0.8)
        s2 = p2 - np.floor(p2)
        profile = np.maximum(profile, 0.7 * np.where(s2 < 0.7, s2 / 0.7, (1 - s2) / 0.3) ** 1.5)
    return profile * height


def procedural(spec, size, metres_per_px, rng):
    relief = spec.get("relief_m", 40.0)
    h = fbm(rng, size, spec.get("base_cells", 3), 5, ridged=True) * relief
    if spec.get("dunes"):
        d = spec["dunes"]
        mask = np.clip((fbm(rng, size, 2, 2) - d.get("coverage_threshold", 0.45)) * 6, 0, 1)
        h += mask * dunes(rng, size, metres_per_px, d.get("wind_deg", 60), d.get("wavelength_m", 180),
                          d.get("height_m", 25), d.get("kind", "linear"))
    if spec.get("wadis"):
        # Eroded dry riverbeds: carve along low-frequency noise valleys.
        v = fbm(rng, size, 2, 3)
        channel = np.exp(-((v - 0.5) / spec["wadis"].get("width", 0.03)) ** 2)
        h -= channel * spec["wadis"].get("depth_m", 6)
    return h


def add_detail(h, rng, metres_per_px, detail):
    """Sub-DEM relief (SRTM is ~30 m): small hummocks and, where requested,
    a dune field on top of the real elevation."""
    size = h.shape[0]
    if detail.get("hummocks_m"):
        h = h + (fbm(rng, size, max(4, int(size * metres_per_px / 120)), 4) - 0.5) * detail["hummocks_m"]
    fields = detail.get("dune_fields", [detail["dunes"]] if detail.get("dunes") else [])
    warp = (fbm(rng, size, 6, 3) - 0.5) * 2 * detail.get("zone_warp_px", 0)
    for d in fields:   # each field: its own preset (barchan / linear / star) and zones
        mask = zone_mask(size, d.get("zones", []), warp, d.get("feather_px", 40))
        h = h + mask * dunes(rng, size, metres_per_px, d.get("wind_deg", 60), d.get("wavelength_m", 160),
                             d.get("height_m", 12), d.get("kind", "barchan"))
    return h


# ---- route ------------------------------------------------------------------------------
def zone_mask(size, zones, warp, feather=1.0):
    """Soft 0..1 mask of rectangles [x0, y0, x1, y1] (stage pixels, north up)
    whose edges are pushed in and out by `warp` (px of noise displacement), so
    surface and dune zones meet along natural, irregular boundaries."""
    yy, xx = np.mgrid[0:size, 0:size].astype(np.float32)
    mask = np.zeros((size, size), np.float32)
    for x0, y0, x1, y1 in zones:
        inside = np.minimum.reduce([xx - x0, x1 - xx, yy - y0, y1 - yy]) + warp
        mask = np.maximum(mask, np.clip(inside / max(feather, 1e-3), 0, 1))
    return mask


def catmull_rom(points, step_px):
    p = [np.array(points[0], float)] + [np.array(q, float) for q in points] + [np.array(points[-1], float)]
    out = []
    for i in range(1, len(p) - 2):
        a, b, c, d = p[i - 1], p[i], p[i + 1], p[i + 2]
        seg = max(2, int(np.linalg.norm(c - b) / step_px))
        for k in range(seg):
            t = k / seg
            t2, t3 = t * t, t * t * t
            out.append(0.5 * ((2 * b) + (-a + c) * t + (2 * a - 5 * b + 4 * c - d) * t2 + (-a + 3 * b - 3 * c + d) * t3))
    out.append(p[-2])
    return np.array(out)


def distance_to_polyline(size, line):
    """Per-pixel distance (px) to the polyline and the arc-length index of the
    nearest vertex (for along-route profiles). Brute force per segment chunk."""
    yy, xx = np.mgrid[0:size, 0:size].astype(np.float32)
    best = np.full((size, size), 1e9, np.float32)
    nearest = np.zeros((size, size), np.int32)
    for i in range(len(line) - 1):
        a, b = line[i].astype(np.float32), line[i + 1].astype(np.float32)
        lo = np.floor(np.minimum(a, b) - 80).astype(int)
        hi = np.ceil(np.maximum(a, b) + 80).astype(int)
        x0, y0 = max(lo[0], 0), max(lo[1], 0)
        x1, y1 = min(hi[0], size), min(hi[1], size)
        if x0 >= x1 or y0 >= y1:
            continue
        px, py = xx[y0:y1, x0:x1], yy[y0:y1, x0:x1]
        ab = b - a
        t = np.clip(((px - a[0]) * ab[0] + (py - a[1]) * ab[1]) / max(float(ab @ ab), 1e-6), 0, 1)
        d = np.hypot(px - (a[0] + t * ab[0]), py - (a[1] + t * ab[1]))
        sub = best[y0:y1, x0:x1]
        m = d < sub
        sub[m] = d[m]
        nearest[y0:y1, x0:x1][m] = i
    return best, nearest


def blur(a, radius):
    if radius <= 0:
        return a
    k = np.ones(2 * radius + 1) / (2 * radius + 1)
    a = np.apply_along_axis(lambda r: np.convolve(np.pad(r, radius, mode="edge"), k, "valid"), 1, a)
    return np.apply_along_axis(lambda r: np.convolve(np.pad(r, radius, mode="edge"), k, "valid"), 0, a)


# ---- main ------------------------------------------------------------------------------------
def build(spec, out):
    out.mkdir(parents=True, exist_ok=True)
    size = spec["size"]
    if size & (size - 1) or size < 64:
        raise ValueError("size must be a power of two >= 64")
    mpp = spec["metres_per_px"]
    seed = spec["seed"]
    rng = np.random.default_rng(seed)
    colours = terrain_colours()
    src = spec["height"]
    if src["kind"] == "terrarium":
        h = terrarium(src, size, mpp)
        fidelity = "derived"
        # The legacy heightmap is 8-bit: a tall hill in the window would make
        # every step on the plain a metre high. "compress_above_m" squeezes
        # real elevation above a knee by "compress_ratio" (recorded in
        # stage.json, so the stage never claims the hill's true height).
        if "compress_above_m" in src:
            knee, ratio = src["compress_above_m"], src.get("compress_ratio", 0.2)
            h = np.where(h > knee, knee + (h - knee) * ratio, h)
    elif src["kind"] == "procedural":
        h = procedural(src, size, mpp, rng)
        fidelity = "procedural"
    else:
        raise ValueError("unknown height source")
    h = add_detail(h, rng, mpp, spec.get("detail", {}))
    # Legacy maps are stored north-up: PNG row 0 is the largest world y
    # (checked against the shipped "ahead" checkpoints, which lie on its road
    # only under this convention). Arrays stay north-up; only race and prop
    # coordinates are converted to world space (y_world = (size - 1 - row)).
    route = catmull_rom([(x, y) for x, y in spec["route"]["points"]], 1.0)
    width_px = spec["route"]["width_m"] / mpp
    dist, nearest = distance_to_polyline(size, route)
    # Road bed: flatten across the road by blending to a smoothed along-route
    # profile (no steps), with shoulders easing back to the terrain.
    along = np.array([h[int(np.clip(p[1], 0, size - 1)), int(np.clip(p[0], 0, size - 1))] for p in route])
    k = max(3, int(spec["route"].get("profile_smooth_m", 40) / mpp))
    along = np.convolve(np.pad(along, k, mode="edge"), np.ones(2 * k + 1) / (2 * k + 1), "valid")
    bed = along[nearest] - spec["route"].get("bed_drop_m", 0.15)
    shoulder = width_px * 0.5 + spec["route"].get("shoulder_m", 6) / mpp
    w = np.clip((shoulder - dist) / max(shoulder - width_px * 0.5, 1e-3), 0, 1)
    w = w * w * (3 - 2 * w)
    h = h * (1 - w) + bed * w
    road = dist <= width_px * 0.5
    # 8-bit quantisation with the legacy 4x4 blur filter (sum 16 * b).
    lo, hi = float(h.min()), float(h.max())
    span = max(hi - lo, 1.0)
    blur_weight = 0.0625                       # 16 taps -> unit gain
    vertical = span / 255.0 / (16 * blur_weight)
    q = np.clip(np.round((h - lo) / span * 255), 0, 255).astype(np.uint8)
    Image.fromarray(q, "L").save(out / "h.png", optimize=True)
    # Surfaces: rules ordered, first match wins; road gets the route surface.
    slope = np.hypot(*np.gradient(h, mpp))
    noise = fbm(rng, size, 8, 3)
    tmap = np.zeros((size, size, 3), np.uint8)
    cmap = np.zeros((size, size, 3), np.float32)
    rules = spec["surfaces"]
    assigned = np.zeros((size, size), bool)
    # Same warp field for every surface rule.
    zone_warp = (fbm(rng, size, 6, 3) - 0.5) * 2 * spec.get("zone_warp_px", 0)
    def rule_zones(rule):
        return zone_mask(size, rule["zones"], zone_warp) > 0.5
    for rule in rules:
        m = ~assigned
        if "max_slope" in rule:
            m &= slope <= rule["max_slope"]
        if "min_slope" in rule:
            m &= slope >= rule["min_slope"]
        if "max_height" in rule:
            m &= (h - lo) <= rule["max_height"]
        if "noise_above" in rule:
            m &= noise >= rule["noise_above"]
        if "zones" in rule:
            m &= rule_zones(rule)
        tmap[m] = colours[rule["type"]]
        base = np.array(rule["colour"], np.float32) / 255
        var = (noise[m] - 0.5)[:, None] * rule.get("variation", 0.15)
        cmap[m] = np.clip(base * (1 + var), 0, 1)
        assigned |= m
    if not assigned.all():
        raise ValueError("surface rules must cover every pixel (end with a catch-all rule)")
    rs = spec["route"]
    seglen0 = np.hypot(*np.diff(route, axis=0).T) * mpp
    arc0 = np.concatenate([[0], np.cumsum(seglen0)])
    along_m = arc0[nearest]
    road_type = np.zeros((size, size, 3), np.uint8)
    road_colour = np.zeros((size, size, 3), np.float32)
    road_type[:] = colours[rs["type"]]
    road_colour[:] = np.array(rs["colour"], np.float32) / 255
    for stretch in rs.get("spans", []):   # e.g. N9 tarmac for the first kilometre
        m = (along_m >= stretch["from_m"]) & (along_m < stretch["to_m"])
        road_type[m] = colours[stretch["type"]]
        road_colour[m] = np.array(stretch["colour"], np.float32) / 255
    tmap[road] = road_type[road]
    edge = np.clip((dist - width_px * 0.5 + 2) / 4, 0, 1)[..., None]
    cmap = cmap * edge + road_colour * (1 - edge)
    # Shade the colour map with the terrain (helps the legacy renderer and minimap).
    gy, gx = np.gradient(h, mpp)
    # Rows run southwards: light from the north-west (gy is d/d(south)).
    light = np.clip(0.75 + 0.35 * (-gx * 0.6 - gy * 0.8) / np.sqrt(1 + gx * gx + gy * gy), 0.45, 1.15)
    shaded = np.clip(cmap * light[..., None], 0, 1)
    Image.fromarray((tmap).astype(np.uint8), "RGB").save(out / "t.png", optimize=True)
    Image.fromarray((shaded * 255).astype(np.uint8), "RGB").save(out / "c.png", optimize=True)
    r = np.zeros((size, size, 3), np.uint8)
    r[road] = 255
    Image.fromarray(r, "RGB").save(out / "r.png", optimize=True)
    # Foliage density: per-rule weight, none on or near the road.
    # f.png value selects the legacy foliage band (keep it exact per surface);
    # noise only decides coverage (rule "cover", fraction 0..1, default 0.6).
    fol = np.zeros((size, size), np.float32)
    presence = fbm(rng, size, 16, 3)
    for rule in rules:
        if "foliage" in rule:
            m = np.all(tmap == np.array(colours[rule["type"]], np.uint8), axis=-1)
            if "zones" in rule:   # rules sharing a type: only this rule's zones
                m &= rule_zones(rule)
            m &= presence >= np.quantile(presence, 1 - rule.get("cover", 0.6))
            fol[m] = rule["foliage"]
    clear = spec["route"].get("foliage_clear_m", 8) / mpp + width_px * 0.5
    fol[dist < clear] = 0
    Image.fromarray((fol * 255).astype(np.uint8), "L").save(out / "f.png", optimize=True)
    mm = Image.fromarray((shaded * 255).astype(np.uint8), "RGB").resize((256, 256), Image.LANCZOS)
    mm.save(out / "mm.png", optimize=True)
    mm.save(out / "ss.png", optimize=True)
    # Race: start on the route, checkpoints every N metres, last at the end.
    seglen = np.hypot(*np.diff(route, axis=0).T) * mpp
    arc = np.concatenate([[0], np.cumsum(seglen)])
    length = float(arc[-1])
    every = spec["route"].get("checkpoint_every_m", 250)
    marks = list(np.arange(every, length - every * 0.4, every)) + [length]
    def world_px(q):   # north-up stage pixel -> race coordinates (x scale mpp)
        return np.array([q[0], size - 1 - q[1]])
    def at(s):
        i = int(np.clip(np.searchsorted(arc, s), 1, len(route) - 1))
        return world_px(route[i])
    start = world_px(route[min(3, len(route) - 1)])
    ahead = world_px(route[min(10, len(route) - 1)]) - start
    heading = math.degrees(math.atan2(ahead[1], ahead[0]))
    cps = "\n".join(f'\t\t<checkpoint coords="{at(s)[0]:.1f}, {at(s)[1]:.1f}" />' for s in marks)
    bands = "\n".join(
        f'\t\t<foliageband middle="{b["middle"]}" range="{b["range"]}" density="{b["density"]}" '
        f'sprite="{b["sprite"]}" spritecount="{b.get("count", 1)}" scale="{b.get("scale", 1)}" />'
        for b in spec.get("foliage_bands", []))
    lvl = spec["level"]
    # Desert climate (amendment A3.4): wind, wind direction, heat haze, dust.
    climate = "".join(f'\n\t\t{k}="{lvl[k]}"' for k in ("wind", "winddegrees", "heat", "dust") if k in lvl)
    xml = f'''<?xml version="1.0" ?>
<!-- Generated by tools/stage_from_heightmap.py from {spec["spec_name"]}; do not edit by hand. -->
<level
\tname="{spec["name"]}"
\tcomment="{lvl.get("comment", "")}"
\tauthor="{lvl.get("author", "TrackRS contributors")}"
\tdescription="{lvl.get("description", "")}"
\tscreenshot="ss.png"
\tminimap="mm.png">
\t<terrain
\t\theightmap="h.png"
\t\tcolormap="c.png"
\t\thudmap="c.png"
\t\tfoliagemap="f.png"
\t\troadmap="r.png"
\t\tterrainmap="t.png"
\t\ttilesize="{lvl.get("tilesize", 64)}"
\t\thorizontalscale="{mpp}"
\t\tverticalscale="{vertical:.6f}"
\t\t>
\t\t<blurfilter>
\t\t\t<row data="0.0625 0.0625 0.0625 0.0625" />
\t\t\t<row data="0.0625 0.0625 0.0625 0.0625" />
\t\t\t<row data="0.0625 0.0625 0.0625 0.0625" />
\t\t\t<row data="0.0625 0.0625 0.0625 0.0625" />
\t\t</blurfilter>
{bands}
\t</terrain>
\t<weather
\t\tcloudtexture="{lvl.get("clouds", "/textures/sky/light-clouds.jpg")}"
\t\tcloudscrollrate="0"
\t\tfogcolor="{lvl.get("fogcolor", "0.86, 0.8, 0.7")}"
\t\tfogdensity="{lvl.get("fogdensity", 0.0006)}"
\t\tfogdensitysky="1"{climate}
\t\t/>
\t<race coordscale="{mpp}, {mpp}" targettime="{lvl.get("targettime", int(length / 18))}">
\t\t<startposition pos="{start[0]:.1f}, {start[1]:.1f}" oridegrees="{(heading - 90) % 360:.1f}" />
{cps}
\t</race>
</level>
'''
    (out / f"{spec['id']}.level").write_text(xml)
    if "props" in spec:   # stage pixels (north up) -> world metres (game +y = north)
        def world(x, y):
            return round(x * mpp, 2), round((size - 1 - y) * mpp, 2)
        props = {"schema": "trackrs.props/1"}
        for key, items in spec["props"].items():
            if isinstance(items, str):   # e.g. "sign_atlas": a pack path, copied as is
                props[key] = items
                continue
            converted = []
            for item in items:
                item = dict(item)
                if "x" in item:
                    item["x"], item["y"] = world(item["x"], item["y"])
                if "points" in item:   # print trails (tracks): [[x, y], ...]
                    item["points"] = [list(world(*q)) for q in item["points"]]
                for end in ("from", "to"):
                    if end in item:
                        item[end] = list(world(*item[end]))
                for k in ("radius", "width", "depth", "spacing"):
                    if k in item:
                        item[k] = round(item[k] * mpp, 2)
                if "yaw_deg" in item:
                    item["yaw_deg"] = -item["yaw_deg"]
                converted.append(item)
            props[key] = converted
        (out / "props.json").write_text(json.dumps(props, indent=1) + "\n")
    stage = {"schema": "trackrs.stage/1", "id": spec["id"], "name": spec["name"], "region": spec.get("region", ""),
             "description": lvl.get("description", ""), "level": f"{spec['id']}.level", "stage_seed": seed,
             "geographic_fidelity": fidelity, "length_km": round(length / 1000, 3),
             "surfaces": sorted({r["type"] for r in rules} | {rs["type"]}),
             "times_of_day": spec.get("times_of_day", ["dawn", "noon", "golden-hour", "night"]),
             "terrain_source": spec.get("terrain_source", src["kind"]), "test_only": spec.get("test_only", False)}
    (out / "stage.json").write_text(json.dumps(stage, indent=2) + "\n")
    return {"length_m": length, "relief_m": span, "verticalscale": vertical, "checkpoints": len(marks),
            "road_px": int(road.sum())}


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    spec_path = Path(sys.argv[1])
    spec = json.loads(spec_path.read_text())
    spec["spec_name"] = spec_path.name
    result = build(spec, Path(sys.argv[2]))
    print(json.dumps(result))
    return 0


if __name__ == "__main__":
    sys.exit(main())
