# Gate register

The session proxy refuses tag pushes (HTTP 403), so tags are no longer pushed
from build sessions (amendment A0). Every gate is recorded here instead. The
maintainer creates the tags by hand from this table:

`tools/make_tags.sh` prints the exact commands for every gate below that
is not yet on the remote, using full SHAs. It only prints; it never creates
or pushes anything:

```bash
tools/make_tags.sh
```

Each printed pair has the form:

```bash
git tag -a <gate> <full-sha> -m "<gate>: see docs/GATES.md"
git push origin <gate>
```

"Commit" is the commit the gate covers. "CI run" is the green *Engine safety
net* run on `main` for that exact commit. "On remote" records whether the tag
already exists on `origin` (checked with `git ls-remote --tags`).

| Gate | Commit | CI run | Date | On remote |
|---|---|---|---|---|
| `phase-0-done` | `18c75bd` | [41406279839](https://github.com/Saad-programmer/TRackRS/actions/runs/41406279839) | 2026-11-02 | yes |
| `phase-1-done` | `e7dba16` | [41422451771](https://github.com/Saad-programmer/TRackRS/actions/runs/41422451771) | 2026-11-02 | yes |
| `phase-2a-done` | `8cbaf6b` | [41460544943](https://github.com/Saad-programmer/TRackRS/actions/runs/41460544943) | 2026-11-02 | yes |
| `phase-2b-done` | `c33c3ca` | [41502884219](https://github.com/Saad-programmer/TRackRS/actions/runs/41502884219) | 2026-11-02 | yes |
| `phase-2c-done` | `c33013d` | [41515514161](https://github.com/Saad-programmer/TRackRS/actions/runs/41515514161) | 2026-11-02 | yes |
| `phase-2d-done` | `584278d` | [41646001225](https://github.com/Saad-programmer/TRackRS/actions/runs/41646001225) | 2026-11-03 | yes |
| `phase-3a-done` | `191dcee` | [41693706606](https://github.com/Saad-programmer/TRackRS/actions/runs/41693706606) | 2026-11-03 | yes |
| `phase-3b-done` | `ec087af` | [41732484993](https://github.com/Saad-programmer/TRackRS/actions/runs/41732484993) | 2026-11-03 | **no: create by hand** |
| `phase-4a-done` | `af02f36` | [42536007052](https://github.com/Saad-programmer/TRackRS/actions/runs/42536007052) (run 109) | 2026-11-08 | **no: create by hand** |
| `phase-4b-done` | `4049bae` | [42928713175](https://github.com/Saad-programmer/TRackRS/actions/runs/42928713175) (run 111) | 2026-11-11 | **no: create by hand** |
| `phase-3e-done` | `aa95113` | [43189440337](https://github.com/Saad-programmer/TRackRS/actions/runs/43189440337) (run 122) | 2026-11-12 | **no: create by hand** |
| `m0-done` | `3579bfd` | [43189440337](https://github.com/Saad-programmer/TRackRS/actions/runs/43189440337) (run 122, on descendant `aa95113`) | 2026-11-12 | **no: create by hand** |

Notes:

* `phase-3b-done` belongs on commit `ec087af` (full SHA
  `ec087af245e5b583b16c28016fc36f406022f473`).
* `phase-4a-done` belongs on commit `af02f36` (full SHA
  `af02f367919824515d7d3a9d02783de760a5f93e`). The maintainer accepted 4a as
  done: CI run 109 is green on that commit.
* `phase-4b-done` belongs on commit `4049bae` (full SHA
  `4049baeb23807ec6e517edb7992e69b592f7b593`). The maintainer accepted it on
  2026-11-11. It predates the Moroccan amendment, and no rollback is needed.
* `phase-3e-done` belongs on commit `aa95113` (full SHA
  `aa95113e67c6e7137a15bdd258b01be3829bd93f`). Run 122 is green and includes
  the exact golden-manifest comparison.
* `m0-done` belongs on commit `3579bfd` (full SHA
  `3579bfdf7e52175323e35b3b67003ce4a7efd19d`). Its own run (120) was
  cancelled when `aa95113` superseded it. `aa95113` only adds the CI golden
  step and docs on top, so run 122 covers M0, including the pack-lite CI
  step.
* The default renderer stays Legacy until the maintainer supplies real-GPU
  numbers (A0.3).

## Work delivered without a gate stop

These steps ran between gates and have no tag. Each is covered by the
green `main` run of the next gate commit that contains it.

| Work | Last commit | Covered by |
|---|---|---|
| 3c world dressing (vegetation, weather, headlights, water) | `68c24ad` | run 109 (`af02f36`) |
| 3d post-FX core (HDR, exposure, bloom, ACES, SSAO) | `77a637f` | run 109 (`af02f36`) |
| 5a spectators, camera operators, news crew | before `bc9e0c6` | run 109 (`af02f36`) |

## Upcoming gates (amendment §A9 order)

`m1-done` (Saharan world tech),
`m2-done` (Zagora Prologue vertical slice). Each is added to the table
above when it closes.
