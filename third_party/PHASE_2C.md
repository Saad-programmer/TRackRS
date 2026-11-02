# Phase 2c vendored dependencies

Exact upstream repositories, release labels and peeled commits are recorded in
[phase2c-dependencies.json](phase2c-dependencies.json). Builds require no downloads.

- cgltf v1.15: unmodified `cgltf.h`, MIT notice at the end of the header; the small implementation translation unit is ours.
- MikkTSpace reference implementation: reference `mikktspace.c` and `.h`, zlib-style notice at the top of each file. Our marked two-site rotation patch masks the right-shift count to 0–31; upstream can otherwise shift by 32 when the pivot seed has five zero low bits. No tangent equations or grouping rules are changed. See `mikktspace/trackrs-rotate.patch`.
- meshoptimizer v0.23: unmodified upstream source and MIT `LICENSE.md`. Only allocator, index generation, cache, overdraw and vertex-fetch passes are linked.

Upstream notices travel in distribution packages. Original test art has its own
CC0 notice in `data/vehicles/pbr_test/LICENSE.txt`.
