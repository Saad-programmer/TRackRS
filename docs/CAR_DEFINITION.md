# Car Definition v2 (`car.json`, schema `trackrs.car/2`)

A car is one directory `data/vehicles/<id>/` holding `car.json`, plus its meshes
and textures once Phase 4b/4d supplies them. All units are SI. The loader,
validator and legacy adapter live in `engine/content/car_definition.{h,cpp}`.
The game lists every `car.json` next to the original `.vehicle` files, and the
CLI accepts `--vehicle <id>` for either.

```jsonc
{
  "schema": "trackrs.car/2",
  "id": "subaru_impreza_wrc97",            // = directory name, [a-z0-9_]
  "display_name": "Subaru Impreza WRC97",
  "marque": "Subaru", "year": 1997, "class": "World Rally Car", "era": "1997-2000",
  "license": {
    "data": "GPL-2.0-or-later",             // SPDX id of this file
    "model": "",                            // SPDX id of the mesh; "" = no own mesh
    "author": "...", "source_url": "",
    "trademark_notice": "..."               // required for real marques
  },
  "verified": { "mass": false, "engine": false, "gearbox": false,
                "tyres": false, "aero": false, "brakes": false },
  "sources": [],                            // https:// URLs; required if any verified=true
  "provenance": "...",                      // required while nothing is verified
  "body": { "mesh": "", "fallback": "evo_rally", "import_scale": 1.0, "unit": "m" },
  "mass": { "kerb_kg": 1230, "front_weight_dist": 0.58 },
  "engine": { "layout": "...", "idle_rpm": 1000, "redline_rpm": 7000,
              "torque_curve_nm": [[1000, 141], [2650, 338.4], [4000, 470], ...] },
  "drivetrain": { "layout": "AWD", "front_torque_share": 0.5,
                  "gearbox": { "ratios": [2.605, ...], "final_drive": 4.1, "reverse": 2.735,
                               "shift_time_s": 0.12, "derivation": "synthetic: ..." } },
  "tyres": { "radius_m": 0.32, "width_m": 0.225 },
  "aero": { "cd": 0.36, "frontal_area_m2": 2.0, "cl_front": 0, "cl_rear": 0 },
  "brakes": { "bias_front": 0.62 },
  "claims": { "top_speed_kmh": 210, "zero_to_100_s": 0 }   // 0 = unknown
}
```

## Validation (`validate()`, run at load and by `car-definition-v2`)

* **Honesty.**
  * A value may be `verified: true` only if `sources` lists at least one
    `https://` URL.
  * While nothing is verified, a `provenance` note is required.
  * A real marque requires a `trademark_notice`.
  * A mesh requires `license.model`.
  * Without a mesh, `body.fallback` must name a fictional body, and the car is
    shown as "Name (placeholder model)".
* **Ranges.**
  * mass 400–2500 kg, front weight 0.2–0.8;
  * idle 300–3000 rpm, below a redline of 3000–16000 rpm;
  * torque curve: at least 3 points, strictly increasing rpm, values in
    (0, 1500] N·m, all inside [idle/2, 1.1 × redline];
  * 3–8 strictly decreasing gear ratios, final drive 2–7;
  * FWD/RWD/AWD consistent with `front_torque_share`;
  * tyre radius 0.20–0.45 m, Cd 0.2–0.8, brake bias 0.3–0.85.
* **Warnings.** Implausible peak power; top gear at the redline below the
  claimed top speed.

## Legacy adapter (until Phase 6 replaces the physics)

* **`fromLegacyVehicle`.** Converts the original fictional `.vehicle` files
  into Car v2 for reports and calibration. Those cars still drive from their
  XML, so legacy hashes are untouched.
* **`toLegacyVehicle`.** Feeds a `car.json` into today's physics, starting from
  the fallback body's XML. It keeps the model, clips and suspension, and
  replaces:
  * name, mass and engine: torque × ω sampled every 250 rpm, because legacy
    physics interpolates *power*, with a limiter 4 % above redline;
  * gearbox: wheel revolutions per engine revolution = 1 / (ratio × final);
  * drive split, tyre radius (wheel model scaled to match) and brake bias.
* **Power scale.** Legacy power is not SI (wheels integrate torque without
  inertia), so watts are mapped with one documented constant,
  `LegacyPowerPerWatt = 26000 / 223710`. It is calibrated from the tuned
  `lancer_evo`, and a test pins it.

Regenerate the eleven real-car files with `tools/seed_real_cars.py`. A CTest
check (`car-seed-fresh`) fails if they drift from the table.

## Liveries (`livery.json`, Phase 4b)

Optional, next to the car's `.vehicle` or `car.json`. They apply only in the
PBR world's CarRenderer; Legacy draws the atlas as authored. Schema
`trackrs.livery/1`, parsed by `engine/car/car_assets.cpp`:

| key | meaning |
|---|---|
| `primary`, `secondary` | target paint colours `#rrggbb`; `null` keeps that colour |
| `primary_source`, `secondary_source` | atlas colours to replace. Omit them to use a 2-means estimate. Only paint-role texels within ~0.26 (sRGB distance) of a source change, and their shading is kept. |
| `number`, `number_colour`, `panel_colour`, `number_panels` | race number (0–999, `-1` keeps the atlas). It is redrawn in each panel with an original stroke font. |
| `number_panels[].rect` | `[u, v, w, h]`, normalised atlas coordinates with v measured from the top of the image; `rotation` is 0/90/180/270 |
| `sponsor_slots`, `sponsor_seed` | rects filled with procedurally generated **fictional** logos (seeded emblem + stroke-font name) |

Real team, sponsor or livery designs must not be used (§2.4). The 11
placeholder real cars carry plain fictional schemes, written by
`tools/seed_liveries.py`; the `car-liveries-fresh` test checks those files
are up to date.
