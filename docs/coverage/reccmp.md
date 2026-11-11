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

The separate non-incremental matching build measured on the source tree at
`cf3bfa0d` reports **2.80%** of original `.text` bytes identical at their
original addresses, **872,200** inverted pairs of mapped functions, and
**32,883 / 642,549 bytes (5.12%)** covered by raw 100%-matching functions.
The mapped-function accuracy means are **49.30%** game-only and **53.68%**
full. Section-relative identical shares are `.text` **2.80%**, `.rdata`
**12.28%**, `.data` **15.04%**, import descriptors **32.08%**, and resources
**16.31%**. The [generated report](binary-progress.json) records the sizes,
header differences and input hash. These figures use the definitions and
reproduction command in the [tool documentation](../../tools/README.md#whole-binary-progress);
they are distinct from the incremental-link snapshot above.
