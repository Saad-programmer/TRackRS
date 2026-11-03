# Car probe quality setting

The persisted `graphics.renderquality` setting and Render quality menu offer Low, Medium, High and Ultra. Medium remains the default. Low/Medium use32²; High/Ultra use128². Preview accepts `--quality low|medium|high|ultra`. Changing quality recreates the probe on the GL thread on the next PBR draw. Refresh remains capped at2Hz by default.

Measurements: llvmpipe software rendering,960×640,30 measured frames after3 warmup frames; not reference-hardware results. This refresh uses software-only worker BPTC decoding; hardware keeps native compressed uploads. Each preset captured three updates: the first cold update is excluded, and the table averages the remaining two. The small sample is a cost illustration, not a hardware benchmark.

| Presets | Probe | Logical resident bytes | Warm capture ms/update | Warm filter ms/update |
|---|---:|---:|---:|---:|
| Low / Medium | 32² | 135136 | 1.4253 | 10.1323 |
| High / Ultra | 128² | 2162656 | 3.8173 | 75.9992 |

Raw samples are in probe-quality/medium.json and high.json. Residency includes both RGBA16F full cubemap chains and depth; driver padding and query objects are excluded. The larger probe is an explicit quality cost, not a hardware performance claim. CPU tests cover all four mappings, name round trips and invalid values.

This checkpoint is the prerequisite for Phase3a, not the terrain gate. No Phase3b–3d work has begun.
