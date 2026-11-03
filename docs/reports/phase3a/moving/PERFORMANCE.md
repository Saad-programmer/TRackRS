# Full moving replay

Ahead,1920×1080High,1500fixed ticks/1499samples, same baseline inputs, sequential isolated software runs. Both state hashes6c59e5730498f6bb match the accepted legacy golden.

|Path|Draws/frame|Triangles/frame|Frame ms|GPU frame ms|Terrain GPU ms|
|---|---:|---:|---:|---:|---:|
|legacy|217.23|477008|67.70|4.69|0.32|
|pbr|198.04|260663|259.35|7.90|2.33|

Raw JSON includes every pass CPU/GPU scope,load,RSS andp99/p999. Same measurement caveats and master-budget assessment as [three-stage table](../benchmark/PERFORMANCE.md). Software timing is not a reference-hardware result. [Moving comparison](comparison.png) is a matching-tick visual review, not a new legacy golden.
