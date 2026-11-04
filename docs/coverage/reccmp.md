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
