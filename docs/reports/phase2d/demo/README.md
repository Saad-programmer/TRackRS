# Material scene comparison

`before.png` is the accepted Phase 2c material preview. `after.png` is the Phase
2d preview from the same camera: sky lighting, clear-coat red paint, chrome wheel
caps, rough dark rubber and wet asphalt. The two rows sweep roughness from 0.1
to 0.9 for metallic and dielectric materials. This PBR demonstration intentionally
changes lighting; legacy-content screenshot tolerances apply separately.

Launch `material_preview --data-dir data`. H toggles headlights, I toggles IBL,
P toggles the local probe, and F1–F7 select material debug views. The local probe
captures the sky and asphalt, excluding the car to avoid self-reflection.

`profile-unbatched.json` preserves the first 120 measured-frame checkpoint.
It reports software-renderer GPU timestamps, not representative hardware FPS.
Per-material storage includes bound mip payloads and retained placeholder texels;
shared bindings can appear in multiple rows. Probe storage includes both RGBA16F
cube chains and depth; driver allocation padding is excluded. Cold shader/JIT
costs appear in initial probe samples. Final gate measurements will be recorded
separately after rendering and verification work is complete.
