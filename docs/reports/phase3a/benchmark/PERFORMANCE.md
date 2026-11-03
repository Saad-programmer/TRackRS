# Phase 3a isolated software measurements

1920×1080, High, llvmpipe LLVM19, sequential120-tick replay pairs,119samples each. Loading frame excluded; no warmup exclusion. These samples cover the countdown/start position. GPU timestamps measure software execution and are not reference-hardware results. No concurrent local build/test was running.

| Stage | Path | Draws/frame | Triangles/frame | Frame mean ms | GPU frame ms | Main terrain GPU ms | Load ms | Peak RSS MiB |
|---|---|---:|---:|---:|---:|---:|---:|---:|
|ahead|legacy|217.87|476955|39.81|3.01|0.11|2031|547.4|
|ahead|pbr|110.20|112252|231.16|8.63|4.36|2062|569.7|
|country|legacy|149.01|135786|30.97|1.04|0.15|1268|398.2|
|country|pbr|76.47|77718|213.08|6.55|4.34|1167|427.3|
|glacier|legacy|172.70|134785|38.34|4.71|0.26|1774|475.2|
|glacier|pbr|89.45|86932|210.47|8.62|3.89|1762|499.4|

## Per-pass GPU ms/frame

|Stage/path|Sky|Terrain|Terrain probe|Mesh submission|Water|Particles|UI/text|
|---|---:|---:|---:|---:|---:|---:|---:|
|ahead/legacy|0.0000|0.1060|—|0.0002|—|2.1811|0.0156|
|ahead/pbr|0.0000|4.3591|—|0.0016|—|2.3983|0.0180|
|country/legacy|0.0000|0.1501|—|0.0601|0.0146|0.0221|0.0004|
|country/pbr|0.0001|4.3406|—|0.0010|0.0155|0.0251|0.0007|
|glacier/legacy|0.0000|0.2597|—|0.0168|—|1.9914|0.0202|
|glacier/pbr|0.0000|3.8914|—|0.0030|—|2.0502|0.0056|

Pass scopes can nest (probe work occurs during mesh submission); do not sum them. Raw JSON includes CPU scope times and p99/p999. Portable total VRAM is unavailable; no invented total is reported. Terrain sidecars give logical texture/grid storage, excluding driver padding and retained legacy buffers.

## Master prompt §7 budgets

High typical submissions must be ≤1500draws and ≤3Mtriangles; all measured pairs pass. Peak process RSS is below2.5GiB in these samples. Reference High targets90fps/≤11ms frame,≤6ms CPU/≤10ms GPU and60fps1%low: software results do not satisfy those timings and cannot certify hardware compliance. Low720p45fps and Ultra4K75fps remain hardware measurements to obtain. High3GiB/Low1.5GiB total VRAM cannot be certified by payload accounting. Load≤5s/race and≤8s/menu, and≤16ms warm hitches must be checked on target hardware; cold JIT/cache samples are not warm-hitch evidence.

## Visual review

[Side-by-side sheet](comparison.png), Legacy left/PBR right, was inspected at matching dimensions and replay ticks. Terrain colours/material response intentionally differ in the selectable new path; no pixel-equality tolerance is applied to that artistic change. Retained vehicles, HUD, sky and foliage load. Fine surface grain can remain visible in the modern materials; future art refinement is distinct from the unchanged legacy gate. All eight accepted legacy screenshots retain MAE0/SSIM1. The modern path shows no missing terrain or open LOD seams in these views; automated edge/coverage tests supply the geometric check.
