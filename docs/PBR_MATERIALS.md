# Native PBR materials and IBL

Build `bake-material`, `material-validator`, `ibl-baker` and `material_preview`
with CMake. Examples from the repository root:

```sh
build/linux-release/bin/bake-material --input colour.png --semantic albedo --output albedo.ktx 2
build/linux-release/bin/bake-material --input normal.png --semantic normal --output normal.ktx 2
build/linux-release/bin/bake-material --input ao.png --ao ao.png --roughness roughness.png --metallic metallic.png --semantic orm --output orm.ktx 2
build/linux-release/bin/material-validator --material surface.material.json
build/linux-release/bin/ibl-baker --output environment-cache --size 64 --samples 256
build/linux-release/bin/material_preview --data-dir data
```

A material manifest is a local JSON object:

```json
{"name":"surface","albedo":"albedo.ktx 2","normal":"normal.ktx 2","orm":"orm.ktx 2","atlas":false}
```

Albedo is BC7 sRGB; normals are BC5 linear; ORM is BC7 linear with AO in R,
roughness in G and metallic in B. The scalar bake semantic uses BC4 linear.
Every texture requires all mips. Surface sampling uses up to 8× anisotropy when
available. Atlas validation requires power-of-two dimensions. The validator
rejects wrong format/transfer roles and escaping manifest paths.

The baker creates an original analytic sky: BC6H sky, diffuse irradiance/pi and
GGX-prefiltered full cube mip chains, plus a 256² RG16F BRDF LUT with mips. Cache
files load asynchronously through fenced PBO slots with placeholders. Platforms
without BPTC decode BC6H/BC7 on the worker and upload portable uncompressed
textures. BC4/BC5 use core RGTC support.

Standard glTF images remain valid PNG fallbacks. Optional image
`extras.trackrsKtx2` points at a native cache next to the glTF image. These are
native BC payloads, not BasisU; no `KHR_texture_basisu` extension is claimed.
Legacy content continues through the existing material path.

The original demo inputs and six material sets can be regenerated with
`python 3 tools/bake_demo_materials.py --help` for its build-tool path options.
Preview controls: H headlights, I IBL, P local probe, F1–F7 material debug views.
The probe excludes its own car, captures the sky and floor/terrain, and refreshes
at most 2 Hz by default; intervals below 250 ms are rejected.
