# `stage-from-heightmap` (tools/stage_from_heightmap.py)

This tool builds a playable stage in the legacy level format from a JSON spec.
The stage then ships inside a content pack (see
[CONTENT_PACKS.md](CONTENT_PACKS.md)). It is deterministic: the same spec and
seed produce byte-identical files.

```bash
python3 tools/stage_from_heightmap.py <spec.json> data/packs/<pack>/stages/<id>
```

## Spec

| Key | Meaning |
|---|---|
| `id`, `name`, `region`, `seed` | Stage identity. `seed` drives every random choice and becomes `stage_seed`. |
| `size`, `metres_per_px` | Heightmap side (a power of two) and horizontal scale. `size × metres_per_px` is the map width. |
| `height` | `{"kind":"terrarium", "tiles":dir, "zoom":z, "centre":[lat,lon]}` for real elevation, or `{"kind":"procedural", "relief_m", "base_cells", "dunes":{…}, "wadis":{…}}` |
| `detail` | Relief below the DEM resolution on top of the source: `hummocks_m`, plus `dunes` (`zones` as `[x0,y0,x1,y1]` stage pixels, `wind_deg`, `wavelength_m`, `height_m`, `kind` = `linear`, `barchan` or `star`). |
| `route` | `points` (stage pixels, north up) run through a Catmull-Rom spline. Also: `width_m`, `type` (a TerrainType name), `colour`, `checkpoint_every_m`, `profile_smooth_m`, `shoulder_m`, `foliage_clear_m`. |
| `surfaces` | An ordered rule list; the first match wins and the last rule must catch everything. Each rule has `type`, `colour`, `variation` and `foliage`, plus filters: `max_slope`, `min_slope`, `max_height`, `noise_above`, `zones`. |
| `foliage_bands` | Legacy foliage bands (`middle`, `range`, `density`, `sprite`, `count`, `scale`). |
| `level` | Level metadata: `comment`, `author`, `description`, `fogcolor`, `fogdensity`, `targettime`. |
| `times_of_day`, `terrain_source`, `test_only` | Copied into `stage.json`. |

## What it does

1. **Height.**
   * **Source:** real elevation is resampled bilinearly from terrarium
     tiles; procedural terrain is ridged fBm, a wind-aligned dune field and
     wadis.
   * **Dunes:** the profile is asymmetric (78 % stoss slope, 22 % slip
     face), with crests across the wind.
   * **Road bed:** the road is flattened onto a smoothed along-route
     profile, and smoothstep shoulders blend it back into the terrain.
2. **Quantisation.**
   * The heightmap is 8-bit, as the legacy loader requires.
   * `verticalscale` is chosen so 0–255 spans the stage's relief through
     the level's 4×4 blur filter (unit gain).
   * The step is `relief / 255`, about 0.1 m for a 25 m prologue.
3. **Maps.**
   * `t.png` uses the exact `TERRAINMAP_MATERIALS` colours, parsed from
     `src/include/terrainmap.h`.
   * `c.png` is the surface colour with hill shading.
   * `r.png` is pure white on the road.
   * `f.png` holds per-surface foliage, cleared near the road.
   * `mm.png` and `ss.png` are previews.
4. **Race.** The start is on the route, facing along it, with checkpoints
   every `checkpoint_every_m` and one at the end.
5. **`stage.json`.**
   * `geographic_fidelity` is `derived` for real elevation and
     `procedural` otherwise. The tool never writes `surveyed`.
   * `length_km` is measured along the spline.

## Real elevation: `tools/fetch_dem.py`

* **Source:** Tilezen terrarium tiles (AWS Open Data, `elevation-tiles-prod`).
  Over Morocco at zoom 10–13 they are built from **SRTM**.
* **Provenance:** each tile's `X-Imagery-Sources` header names its source
  rasters, and the fetch tool records them in a provenance JSON.
* **Licence:** SRTM is a U.S. Government work (no copyright claimed under
  Title 17 U.S.C.). The USGS requests the credit "SRTM data courtesy of the
  U.S. Geological Survey". This is quoted from Tilezen's attribution
  document, which the tool records too.
* **Fidelity:** the stage route and every detail on top of the DEM are
  invented, so these stages are `derived`, never `surveyed`.
