# Phase 3b worklog

Base phase-3a-done191dcee. Branch wip/phase-3b. No agents authorized. Stop at3b, no3c. User will supply realGPU measurements; do not claim hardware budgets fromllvmpipe. Preserve13acceptedstatehashes/eightRGBgoldens; oldlook remainsselectable/default.

## Prerequisites completed
Added fullCC0-1.0text and per-setPROVENANCE.json: originalproceduralsource, transparentCodex/projectauthorship, SPDX,per-layerseed/recipeparameters and63assetSHA256s. Baker refreshesprovenance. New CTest terrain-license-provenance passes and runs viactest in allLinux/WindowsCIjobs; no previousterrainlicenseCIcheck existed. Includesexactassetcoverage and rejects changed/uncoveredfiles/stalegenerator. Assetsnotrebaked.

Terrain-only actualproductionGPUdraws oncontrolledperiodicfixtureHigh,60degree16:9camera towardorigin: near32m34draws69632tris;mid256m216draws442368tris;far1024m144draws294912tris. Excludesfoliage/probe/car/sky/UI/shadows. Details/reproductionreports/phase3b/TERRAIN_DISTANCE.md. ExistingterrainGPU/IBLfurnace tests pass. Next3bimplementation: sharedsunstate, proceduralday/night/stars/moon,sky-derivedIBL,analyticheightfog,3/4texel-snappedCSMwithterrain/car/alpha-cutoutfoliagecasters,optionalblobfallback; config/UI/CLI; shader/CPU/GPUtests,allgoldens/screenshotsandmetrics,CI/merge/tagthenstop.
