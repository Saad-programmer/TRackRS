# Asset and content observations

- `data/maps/ahead/ahead.level` references `textures/sky/light-coluds.jpg`,
  which is missing. The existing loader falls back to `/textures/sky/blue.jpg`.
  Preserve this baseline until the defect-fix phase; no replacement art was invented.
- This checkout contains 11 vehicle definitions total, including the nine original
  fictional definitions; it does not contain the brief's eleven additional historical
  car stubs. Do not manufacture real-world specifications or licensing claims.
- Existing author/licence records remain under `doc/DATA_AUTHORS.txt`.
  New per-asset manifests and verified real-car licensing are future phase work.
  No new car art, marques, sponsors, or unverified specifications are bundled here.
- `tests/baselines/content-inventory.json` records the actual stage/event/vehicle/
  plugin file inventory. The generated repository dumps were removed; unique docs,
  original licences and game assets were retained.
- Phase 3c 3D vegetation is procedural (no imported meshes). Twenty-three sparse,
  bare or silhouette sprites (crooked/thin/dry-slim trees and bushes, tree-shape*,
  ivy, post, first-branch, snowy-tree, bland-birch, berry-bush, big-bush, bush-dry)
  remain instanced cards at every distance until faithful forms exist
  (`data/vegetation/species.json`, `"form": "card"`).
- Phase 4a: the eleven real cars exist only as unverified `car.json` seeds
  (`verified:false`, no sources; synthetic gearing) on fictional placeholder
  bodies. They need cited specifications and licensed meshes (Phase 4d).
- `data/vehicles/lancer_evo` names a real marque without license metadata, and
  its physics goes non-finite within ~0.4 s on every stage (pre-existing; seen
  with the pre-4a build too). It is left unchanged. Fix or retire it in Phase 4d.
