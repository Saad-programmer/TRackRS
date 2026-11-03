# Before/after world rendering — software measurements

Measured production source `c7cc37e`, including offscreen IBL capture/filter helper draws.

Ahead,1920×1080High,120ticks/119samples, identical baseline inputs and vehicle. All states equal3eebeb4e28341a5d. Sequential isolated llvmpipe LLVM19 runs; no concurrent local build/test during the after captures. The accepted3a before record is from its earlier isolated run on this environment. Loading frame excluded; no further warmup exclusion. Cold program/JIT and first-ready sky-bake costs are not representative warm hardware costs; the engine stage-load timer does not include all lazy GPU/JIT work. Software GPU queries are not reference-hardware results.

|Path/time|Draws/frame|Triangles/frame|Frame mean ms|GPU frame ms|p99 ms|Peak RSS MiB|Stage-load ms|
|---|---:|---:|---:|---:|---:|---:|---:|
|3a PBR fixed lighting|110.20|112252|231.16|8.63|329.24|569.7|2062|
|legacy|217.87|476955|40.32|3.05|67.13|549.2|1954|
|dawn-csm|815.46|1022427|369.58|365.54|508.56|746.4|2106|
|noon-csm|640.97|778389|375.17|370.77|585.85|718.2|2028|
|golden-hour-csm|833.45|1047739|428.33|424.32|558.30|741.3|2117|
|dusk-csm|820.87|1034185|372.88|368.55|480.73|733.8|1996|
|night-csm|696.83|853369|341.27|336.97|458.60|720.1|2008|
|midnight-csm|640.97|778389|308.85|304.39|412.28|648.3|1904|
|noon-blob|110.71|111454|229.49|227.51|315.35|574.7|1904|

## GPU ms per frame

|Path/time|sky|terrain|shadow_cascade_0|shadow_cascade_1|shadow_cascade_2|shadow_cascade_3|shadow_terrain|sky_ibl_bake|mesh_submit|water|particles|ui_text|
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
|3a PBR fixed lighting|0.0000|4.3591|—|—|—|—|—|—|0.0016|—|2.3983|0.0180|
|legacy|0.0000|0.1118|—|—|—|—|—|—|0.0285|—|2.1188|0.1277|
|dawn-csm|0.0065|276.5676|14.7683|14.5524|17.2330|37.9515|11.7211|0.4105|0.0024|—|2.3586|0.0229|
|noon-csm|0.0067|288.1446|6.4856|11.2062|12.3000|48.2577|27.2044|0.4807|0.1044|—|2.2684|0.3455|
|golden-hour-csm|0.0057|283.1371|31.4561|23.4804|24.1835|57.1313|38.3454|0.4598|0.0245|—|2.2009|0.0239|
|dusk-csm|0.0058|277.2931|19.6084|15.3374|16.8699|35.7962|6.7746|0.3929|0.0292|—|2.1817|0.0134|
|night-csm|0.0057|277.0667|10.9873|9.8497|10.5904|24.8495|5.9007|0.5338|0.0411|—|2.1801|0.0059|
|midnight-csm|0.0073|272.7815|2.0740|2.0302|4.4000|19.4394|0.0007|0.4051|0.0050|—|2.1612|0.0065|
|noon-blob|0.0054|222.7505|—|—|—|—|—|0.3867|0.0038|—|2.1087|0.3813|

Scope queries can nest: shadow_terrain is inside cascade scopes, and probes can be inside mesh submission. Do not sum overlapping pass times. Raw records contain CPU scope times and all sample counts. A dash means no measured scope, not a claim of zero execution cost.

## Last-frame geometry and allocated logical shadow payload

These sidecars count the final rendered frame, while the table above averages119frames. Terrain excludes retained foliage. Cascade counts include terrain, vehicles and alpha-cutout foliage. Nominal depth payload excludes driver padding; D24 may occupy32bits physically. Total VRAM is unavailable portably.

|Time|Main terrain draws/tris|Cascade draws|Cascade triangles|Blob draws|CSM payload bytes|
|---|---:|---|---|---:|---:|
|dawn-csm|117/239616|[57, 85, 137, 402]|[47816, 82022, 153412, 593354]|0|50331648|
|noon-csm|117/239616|[24, 48, 86, 355]|[13510, 38024, 77124, 518052]|0|50331648|
|golden-hour-csm|117/239616|[63, 87, 144, 409]|[52738, 86118, 159256, 607690]|0|50331648|
|dusk-csm|117/239616|[58, 86, 139, 408]|[49864, 84070, 157508, 605642]|0|50331648|
|night-csm|117/239616|[28, 50, 110, 370]|[21702, 42120, 105986, 548772]|0|50331648|
|midnight-csm|117/239616|[24, 48, 86, 355]|[13510, 38024, 77124, 518052]|0|50331648|
|noon-blob|117/239616|[0, 0, 0, 0]|[0, 0, 0, 0]|1|0|

## Budgets and review

All after cases meet the master prompt's High geometry limits of≤1500draws and≤3Mtriangles. High90fps/≤11msframe,≤6msCPU/≤10msGPU and60fps1%low, total3GiBVRAM, and warm-hitch/load targets still require the user's real-GPU measurements. Software frame times do not establish those targets. RSS/payload are process/logical metrics, not complete physical GPU allocation. Low720p45fps and Ultra4K75fps are also unverified on reference hardware.

[Six preset thumbnails](times-of-day.png), [3a versus3b Noon](before-after.png), and each full-resolution PNG are actual engine captures. New appearance intentionally changes; unchanged-golden requirements apply to Legacy. Screenshot review and exact legacy comparison are recorded in the gate report.
