# Content packs (pack-lite, M0)

New maps and championships ship as content packs, with no recompile
(master prompt §4.4, amendment §A9.2). Legacy content is untouched: today's
`/maps` and `/events` keep loading through the legacy loaders and appear as
the virtual pack `classic-legacy-stages`.

## Layout

```
data/packs/<pack-id>/            (or <userdir>/packs, or <pack-id>.trpack zip)
  pack.json                       trackrs.pack/1
  stages/<stage-id>/stage.json    trackrs.stage/1
  stages/<stage-id>/<stage-id>.level + h.png c.png t.png r.png f.png mm.png ss.png
  championships/<id>.championship.json   trackrs.championship/1
```

A `<pack-id>.trpack` zip placed in `/packs` is mounted as `/packs/<pack-id>/`.

## `pack.json`

```json
{
  "schema": "trackrs.pack/1",
  "id": "morocco-sahara",                 // lower-case letters, digits, '-', '_'; equals the directory
  "name": "Rallye Sahara Draâ",
  "version": "0.1.0",                     // semver
  "requires_engine": ">=0.6.7 <1.0",      // comparators: >= <= > < = ^ ~, space = AND, * = any
  "content_api": 1,
  "depends": [{"id": "other-pack", "version": "^1.0"}],
  "authors": ["…"],
  "license": {"spdx": "GPL-2.0-or-later", "attribution": "…"},
  "notices": ["shown in CREDITS.md"],
  "assets": [
    {"path": "stages/zagora/*.png", "kind": "texture",
     "license": {"spdx": "…", "attribution": "…", "author": "…", "source_url": "…", "verified": false}}
  ]
}
```

**Licence audit (amendment A10).** Every file in the pack must match an
`assets` glob (`*` within a path segment, `**` across segments) whose licence
has an SPDX id and an attribution. Otherwise the pack is invalid. Set
`verified: true` only after a maintainer has checked the source.

## `stage.json`

```json
{
  "schema": "trackrs.stage/1",
  "id": "zagora-prologue", "name": "Zagora Prologue", "region": "Zagora, Draâ valley",
  "level": "zagora-prologue.level",       // relative to the stage directory, never escaping it
  "stage_seed": 20260, "length_km": 3.4,
  "geographic_fidelity": "derived",       // procedural | derived | surveyed
  "surfaces": ["PressedEarth", "FineGravel"], "times_of_day": ["dawn", "noon", "golden-hour", "night"],
  "terrain_source": "SRTM 1 arc-second via Tilezen terrarium tiles", "test_only": false
}
```

The fidelity values mean:

| Value | Meaning |
|---|---|
| `procedural` | Generated terrain with no real elevation. This is the amendment A2 fallback. |
| `derived` | Real public elevation under a fictional route and invented details. |
| `surveyed` | A real course replica. It triggers a validator warning and needs a maintainer-verified survey source. |

Never claim a stage is a surveyed replica unless it is one.

## `*.championship.json`

```json
{
  "schema": "trackrs.championship/1", "id": "draa-sahara-cup", "name": "Draâ Sahara Cup",
  "rounds": [{"stage": "zagora-prologue", "day": 1, "kind": "special", "time_of_day": "dawn",
              "weather": "clear", "service_after": true}],
  "points": [25, 18, 15, 12, 10, 8, 6, 4, 2, 1],
  "penalties": {"missed_waypoint": 300}
}
```

* `kind` is one of `special`, `liaison`, `service` or `marathon`.
* A round's `stage` is a local stage id, or `<pack>:<stage>` for another
  pack's stage.
* Local references must exist.

## Discovery, failure policy and edition

* **Discovery:** packs are discovered at startup and validated for schema,
  references, licence audit, engine range and dependencies.
* **Broken packs:** a broken pack is **disabled with a reason**. The reason
  goes to the log, to a line on the main menu, and to `<json>.packs.json` in
  validation runs. It is never a crash.
* **Dependencies:** a disabled or missing dependency disables its dependents.
* **Edition policy:** `data/policy/edition.json` may list `demo_packs`,
  which are allowed in the demo edition. Stages with `test_only` are listed
  only in the dev edition and the validation harness.
* **Running a pack stage:** `--stage <pack>:<stage>` runs a pack stage in the
  harness, and `--pack-root <dir>` mounts an extra packs directory there (for
  tests).

## Tools

* `trackrs-content validate <data-dir>` validates every pack and exits 1 on
  any invalid pack. `--json` writes the catalogue.
* `trackrs-content credits <data-dir> --output docs/CREDITS.md [--check]`
  generates the credits from the manifests. CI fails if the file is stale.
* `tools/stage_from_heightmap.py <spec.json> <stage-dir>` builds a stage
  (see [STAGE_TOOL.md](STAGE_TOOL.md)).
