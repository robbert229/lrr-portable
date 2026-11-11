# Reccmp snapshot — 2026-11-04

Built and compared in the `developer` distrobox from source commit
`06cc0366` (documentation branch based on `9b31f55b`). Documentation and
ledger regeneration do not change the compiled source.

| Scope | Mapped functions (“Implemented”) | Accuracy | Progress |
| --- | ---: | ---: | ---: |
| Game-only (`--nolib`) | 2,116 / 2,138 (98.97%) | 48.27% | 47.77% |
| Full report | 2,335 / 2,397 (97.41%) | 52.73% | 51.36% |

These are Reccmp's printed summary values. “Implemented” measures functions
mapped for comparison; accuracy measures similarity among mapped functions,
and progress includes unmapped functions. These counts differ from the
[source citation ledger](README.md), and do not establish gameplay parity.

## Reproduce

```sh
distrobox enter developer -- bash -lc \
  'cd /var/home/johnrowl/source/lrr-portable && make reccmp-report'
```

The MSVC 5.0 profile used `/GX /ML /Zi /O2 /Ob1 /DNDEBUG`, with linker PDB
output enabled. See the [build guide](../building.md) for toolchain overrides.
The target scans `src/` for annotations. Scanning the entire repository also
picked up ignored source snapshots under `build-reccmp/runtime-evidence/`,
producing duplicate annotations and unreliable scores; those results are excluded.

Reports are generated locally at `build-reccmp/reccmp-report/index.html`
and `game-only.html`, with `results.json` and `game-only.json` alongside.
The run completed successfully but emitted duplicate-annotation and missing
function-symbol diagnostics, plus a debug-data synchronization diagnostic
near `src/game/spawn.cpp:360`, within the current source tree. Those remain
matching work; the snapshot does not claim every annotation resolved.

SHA-256:

- Original: `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`
- Rebuilt: `64683af52eee251151f1083f2b511624f0caeabe3d4f40908d7c3aeceff7dfd5`

## Whole-binary progress — 2026-11-10

The non-incremental matching build at `6db0b3c7` has **33 bytes** in
identical runs of at least 16 bytes at their original `.text` addresses
(**0.01 percentage points** above the shifted-original baseline). Raw
agreement and the chance estimate both round to **2.83%**; no fully
identical functions sit at their original addresses. Full-scope function order
has **907 adjacent inversions** and **870,576 inverted pairs**; game-only has
**848** and **856,666** respectively. Raw exact-function coverage is
**37,855 / 642,549 bytes (5.89%)**, and mean raw accuracy is **49.37%**
game-only and **53.74%** full.

The [generated report](binary-progress.json) records chance-adjusted section
metrics, sizes, header differences and the input hash. See the
[tool documentation](../../tools/README.md#whole-binary-progress) for
thresholds and definitions. These figures use the non-incremental link, then
a separate build and since #436 the gate's only build, and differ from the
incremental-link snapshot above.
