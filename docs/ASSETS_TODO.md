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
