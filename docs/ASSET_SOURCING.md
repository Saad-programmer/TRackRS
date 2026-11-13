# Asset sourcing: car meshes (Phase 4d)

This is the specification for every car mesh the maintainer supplies. Each
mesh replaces a car's **placeholder model**, a borrowed fictional legacy
body. Until a car has a mesh that passes this specification, it stays
listed and shown as "Name (placeholder model)" ([CARS.md](CARS.md)).

`tools/check_car_mesh.py` checks a mesh against this page. Standard
library only:

```bash
python3 tools/check_car_mesh.py model.glb --class rally-raid --radius 0.45
```

* `--class` is `rally-raid`, `heritage-raid` or `rally`.
* `--radius` is the car's `tyres.radius_m` from `car.json`.

Class packs are delivered in this order: **1. rally-raid**, **2. heritage
raid**, **3. the existing rally roster**.

## 1. File format

* **Format:** **glTF 2.0 binary (`.glb`)**, one file per car.
* **Textures:** embedded, or as PNG/JPEG files next to the `.glb`.
* **Materials:** glTF *metallic-roughness*. `KHR_materials_clearcoat` is
  accepted on `paint`. No other extensions are required; Draco and meshopt
  compression are **not** accepted.
* **Transforms:** apply every rotation and scale before export. Part nodes
  carry a **translation only**; wheels are the exception (§3).
* **Delivery:** `data/packs/<class-pack>/vehicles/<car id>/model.glb`. In
  that car's `car.json`, set `body.mesh: "model.glb"` and `license.model` to
  the SPDX id, and fill `license.author` and `license.source_url`.

## 2. Axes, origin and scale

| | Requirement |
|---|---|
| Units | **Metres**, real-world size within ±2 % of the real car. |
| Axes (in the `.glb`) | glTF convention, **+Y up**. The car's **front faces −Z**, its **right side faces +X**. The importer turns this into the engine frame (+X right, +Y forward, +Z up). |
| Origin | On the **ground plane** (lowest tyre point at y = 0), at the **centre of the wheelbase and track**. |
| Size ranges (checked) | rally-raid: length 3.2–5.6 m, width 1.6–2.4 m, height 1.4–2.2 m. heritage-raid: 3.6–5.2, 1.6–2.2, 1.2–2.0 m. rally: 3.4–4.8, 1.6–2.0, 1.1–1.6 m. |
| Ride height | As raced, with the car at rest on its wheels. |

## 3. Part nodes (exact names)

Each part is its **own node and mesh**, so damage can bend, hang or detach
it (Phase 4c today shows a sagging bumper only because the borrowed
bodies have no separate parts).

* **Required:**
  * `body`, `glass`;
  * `bumper_front`, `bumper_rear`;
  * `mirror_left`, `mirror_right`;
  * `wheel_fl`, `wheel_fr`, `wheel_rl`, `wheel_rr`;
  * `light_head_left`, `light_head_right`, `light_tail_left`,
    `light_tail_right`.
* **Optional:**
  * panels: `bonnet`, `boot`, `door_fl`, `door_fr`, `door_rl`, `door_rr`,
    `spoiler`, `exhaust`, `skid_plate`, `roll_cage`;
  * mudflaps: `mudflap_fl`, `mudflap_fr`, `mudflap_rl`, `mudflap_rr`;
  * indicators: `light_indicator_left`, `light_indicator_right`;
  * brake discs: `brake_disc_fl`, `brake_disc_fr`, `brake_disc_rl`,
    `brake_disc_rr`;
  * cockpit: `interior`, `dashboard`, `steering_wheel`, `seat_left`,
    `seat_right`, `needle_tacho`, `needle_speed`;
  * rally-raid extras: `spare_wheel_1`, `spare_wheel_2`, `roof_scoop`,
    `snorkel`, `sand_ladder_left`, `sand_ladder_right`.
* **Left and right** are the car's own sides, seen from the driver's seat.
* **Wheels:**
  * Each node's origin is at the **hub centre**, and the rolling axis is
    along X.
  * The tyre radius must match `tyres.radius_m` within ±2 cm.
  * A wheel node may carry a rotation (for example a mirrored right-hand
    wheel). Brake discs (optional) share their wheel's origin, so the 4b
    glow sits on them.
* **Steering wheel:** origin on the column axis.
* **Gauge needles:** the dashboard's tachometer and speedometer needles are
  separate nodes, `needle_tacho` and `needle_speed`. Each node's origin is
  at the needle's pivot, it points to the gauge's zero, and it turns about
  the dial's axis. (The 4b procedural cockpit animates needles by a UV tag.
  A supplied cockpit uses these nodes instead; the loader for supplied
  meshes is not written yet.)
* **Nothing else:**
  * no other node names (`check_car_mesh.py` rejects unknown parts);
  * no cameras or lights;
  * no manufacturer badges, emblems or logos modelled in geometry (§7).

## 4. Levels of detail

* LOD0 nodes have the plain name.
* Lower LODs repeat **every exterior part** with the suffix `_lod1` or
  `_lod2`, e.g. `bumper_front_lod1`. Interior parts are LOD0 only.
* If a car comes without LOD1 and LOD2, the renderer's simplifier makes them
  (as for the legacy bodies in 4b). Supplied LODs are preferred.

Triangle budgets (checked; triangles after triangulation):

| Class | Exterior without wheels, LOD0 / LOD1 / LOD2 | Each wheel, LOD0 / LOD1 / LOD2 | Interior (LOD0) |
|---|---|---|---|
| rally-raid (T1+, T3, T4) | 80 000 / 25 000 / 6 000 | 6 000 / 2 000 / 400 | 25 000 |
| heritage-raid | 60 000 / 20 000 / 5 000 | 5 000 / 1 500 / 300 | 20 000 |
| rally (existing roster) | 60 000 / 20 000 / 5 000 | 5 000 / 1 500 / 300 | 20 000 |

* **Dents (4c):** they displace vertices, so panels need an even density.
  Keep edges on body panels **≤ 15 cm**, and keep panels closed where they
  meet; open seams show as cracks when dented.

## 5. Materials (exact names)

| Name | Use | Notes |
|---|---|---|
| `paint` | Livery-painted body panels | The livery recolours it; clear-coat |
| `paint_secondary` | Second paint colour, if the design needs it | Optional |
| `glass` | Windows and lamp lenses | Gets the 4c crack web |
| `rubber` | Tyres, seals, mudflaps | Never repainted by liveries |
| `metal` | Bare metal, rims, cage | |
| `chrome` | Trim | Optional |
| `plastic_trim` | Black plastic, grilles | |
| `carbon` | Carbon panels | Optional |
| `light_head`, `light_tail`, `light_indicator` | Lamp reflectors | Emissive texture marks the lit area |
| `interior` | Cockpit | |
| `decal` | Alpha-masked stickers | Fictional only (§7) |

* **UV0 on `paint`:** a unique, non-overlapping layout inside 0..1. The
  livery system writes numbers and sponsor marks into rectangles of that
  atlas.
* **`livery_template.json`:** supply it next to the mesh. It lists the
  number-panel and sponsor-slot rectangles (`[u, v, w, h]`, v from the top)
  for both sides, the bonnet and the roof. The format is
  [CAR_DEFINITION.md](CAR_DEFINITION.md#liveries-livery-json-phase-4b).

## 6. Textures

| Map | Size (max) | Colour space | Notes |
|---|---|---|---|
| Base colour, body atlas | **2048²** | sRGB | Neutral paint (mid grey or white) under the livery |
| Normal, body | 2048² | linear | OpenGL convention (+Y up) |
| Occlusion-roughness-metallic, body | 2048² | linear | R occlusion, G roughness, B metallic |
| Emissive, lamps | 512² | sRGB | Lit lamp areas only |
| Interior set | 1024² | as above | |
| Wheel set | 1024² | as above | Shared by the four wheels |

* Sides must be powers of two, and no class allows more than 2048 (checked).
* Images are PNG or JPEG. Mip maps are built by the engine.
* Budget: about **48 MiB** of textures per car at full resolution, with
  mips. The 4b legacy bodies use about 13 MiB.

## 7. Licences and trademarks

**Accepted mesh and texture licences:**

* `CC0-1.0`;
* `CC-BY-4.0` (attribution recorded in the pack manifest and
  [CREDITS.md](CREDITS.md));
* `CC-BY-SA-4.0`;
* `GPL-2.0-or-later` or `GPL-3.0-or-later`;
* the contributor's own work, with a written statement placing it under one
  of the above.

**Not accepted:**

* non-commercial (NC) or no-derivatives (ND) terms;
* "editorial use only";
* "free for personal use";
* store licences without a redistribution right;
* models extracted from other games;
* unknown or unstated origin.

**Trademarks:**

* A real car's shape may represent the real car named in `car.json`, under
  that file's trademark notice.
* **No manufacturer badges, emblems or wordmarks** in geometry or textures.
* **No real team liveries, sponsors or numbers**: liveries are fictional
  (generated sponsor marks).
* The amendment forbids "Dakar" and any real event name or branding.

**Provenance, recorded in the class pack's `pack.json` asset entry:**

* author;
* source URL;
* licence (SPDX);
* date obtained.

The entry stays `verified: false` until the maintainer has checked the
licence text, then `true`. `trackrs-content validate` disables any pack with
an uncovered file, so a mesh without an asset entry never loads.

## 8. Class packs and the cars waiting for meshes

| Order | Class pack | Where | Cars |
|---|---|---|---|
| 1 | Rally-raid | `data/packs/class-rally-raid` | Toyota GR DKR Hilux T1+, Prodrive Hunter T1+, Ford Raptor T1+, Can-Am Maverick XRS (T4). The fictional `draa_raid_t1` and `erg_lite_t3` (pack `morocco-sahara`) use the same specification. |
| 2 | Heritage raid | `data/packs/class-heritage-raid` | Porsche 959 (rally-raid, 1986), Peugeot 405 T16 Grand Raid, Citroën ZX Rallye-Raid, Mitsubishi Pajero Evolution (raid, 1998), Range Rover (raid, 1981) |
| 3 | Existing rally roster | `data/vehicles` (kept in place: legacy paths are part of the golden hashes and of saved profiles) | the 11 real-car entries from Phase 4a, plus the 9 fictional legacy cars, which keep their own bodies |

* **Performance figures** in the class packs are **class-typical
  placeholders, not the named cars' data**. All are `verified: false`; see
  each `car.json` provenance.
* **Supplying a mesh** does not verify any figure. Figures are verified
  separately, with a cited source in `sources` and the matching `verified`
  key.
