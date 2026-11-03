# Phase 2d GPU timestamps and logical residency

Renderer: **llvmpipe (LLVM 19.1.7, 256 bits)**; 960×640, 120 measured frames after 3 ready warmup frames. No concurrent local build/test was running. These are software-renderer measurements, not hardware FPS predictions.

GPU time is the mean timestamp duration per material submission. Asphalt includes probe-face submissions as well as the main view. Texture bytes include full native mip payloads plus retained placeholders. Shared maps are counted per bound material here; the unique pool is listed separately. Geometry, shader binaries, query objects and driver allocation padding are not attributed to material textures.

| Material | GPU ms / draw | Samples | Resident texture B |
|---|---:|---:|---:|
| chrome | 3.2765 | 120 | 1116 |
| clear-coat paint | 11.1891 | 120 | 1116 |
| dielectric roughness 0.100000 | 10.2444 | 120 | 1116 |
| dielectric roughness 0.300000 | 10.2696 | 120 | 1116 |
| dielectric roughness 0.500000 | 10.1439 | 120 | 1116 |
| dielectric roughness 0.700000 | 12.2562 | 120 | 1116 |
| dielectric roughness 0.900000 | 12.5021 | 120 | 1116 |
| glass | 2.6536 | 120 | 1116 |
| metal roughness 0.100000 | 9.8089 | 120 | 1116 |
| metal roughness 0.300000 | 9.8260 | 120 | 1116 |
| metal roughness 0.500000 | 11.3068 | 120 | 1116 |
| metal roughness 0.700000 | 13.5701 | 120 | 1116 |
| metal roughness 0.900000 | 12.4398 | 120 | 1116 |
| rubber | 3.7270 | 120 | 1116 |
| wet asphalt | 9.3072 | 120 | 16476 |

| Probe measurement | Demo sky + wet floor | Real-race sky + terrain |
|---|---:|---:|
| Six-face capture, warm mean ms | 13.0221 | 173.4577 |
| Six-face capture, cold first ms | 354.9307 | 248.9196 |
| Full GGX mip convolution, warm mean ms | 11.2213 | 23.6586 |
| Full GGX mip convolution, cold first ms | 33.1033 | 40.5667 |

Demo probe: 64 updates; configured interval 0.5 s; minimum observed interval 0.502255 s. Cold samples include shader/JIT startup and are excluded from warm means. Per-update arrays are retained in [profile.json](demo/profile.json); scene-aware race arrays are in [game-profile.json](checkpoints/game-profile.json).

| Storage | Logical bytes |
|---|---:|
| Per 32² probe, both RGBA 16F full cube chains | 131040 |
| Per probe depth attachment | 4096 |
| Total per probe | 135136 |
| Additional texture/depth allocation per refresh | 0 |
| Shared sky, irradiance, prefilter and 256² RG 16F LUT mip chains | 417664 |
| Unique streamed texture pool, including retained placeholders | 438604 |
| Three PBO staging slots, allocated capacities | 350260 |

The shared environment is already included in the unique texture pool. PBOs are separate. The probe owns its storage independently of the texture pool. BC 7/BC 5 material sets use 1,104 payload bytes at 16²; the 64² asphalt set uses 16,464 bytes. Three 4-byte placeholders add 12 bytes per three-map set. BC 6H environments remain linear HDR; the albedo maps use hardware sRGB decoding.

Final preview: **15 material draws, 9,756 triangles**. The initial unbatched 2d scene used 23 draws for the same 9,756 triangles. The real race proves 299 draws per each of four materials over 299 rendered frames, with four wheels instanced.
