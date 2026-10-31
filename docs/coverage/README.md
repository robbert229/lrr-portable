# Reconstruction coverage ledger

Covers all **2564** local functions in `docs/decompiles/full/manifest.csv`
(the full Ghidra corpus, PR #126).

Function-area assignments and responsibility boundaries are in
[`function-areas.md`](function-areas.md) and [`function-areas.csv`](function-areas.csv).
Revalidate the map after a corpus change with `python3 tools/function_areas.py`.

## Status counts

| Status | Count | Share |
| --- | ---: | ---: |
| ported | 2285 | 89.1% |
| stubbed | 0 | 0.0% |
| anchored | 4 | 0.2% |
| named | 2 | 0.1% |
| runtime-import | 17 | 0.7% |
| runtime-library | 254 | 9.9% |
| anonymous | 2 | 0.1% |

- **ported** — entry address or `FUN_*` name cited directly in `src/`
  (2285 functions)
- **stubbed** — entry address has a Reccmp `STUB` annotation, but the
  function remains a placeholder (0 functions)
- **anchored** — only an *interior* address (a call site / offset inside
  the body) is cited in `src/`; the function is touched but its entry is
  not named (4 functions)
- **named** — human name in the full-corpus heading / manifest but no `src/`
  citation yet (2 functions)
- **runtime-import** — CRT/runtime support or import thunks; not game
  logic, never ported (17 functions)
- **runtime-library** — original CRT/compiler-runtime implementation
  supplied by the selected link library; tracked by Reccmp `LIBRARY`
  markers, not ported as game code (254 functions)
- **anonymous** — `FUN_*` only, unreferenced in `src/`
  (2 functions)

**Game logic remaining to port: 8 (0.3%).**

## Largest anonymous regions (by 64 KB page)

| Region | Anonymous functions |
| --- | ---: |
| `0x00040xxxx` | 1 |
| `0x00045xxxx` | 1 |

## Named-but-unported corpus functions

Human-named in the corpus that have no source citation or only a `STUB`.

| Address | Name |
| --- | --- |
| `0x0049221e` | CrtFloatingPointExceptionResultDispatch |
| `0x00492225` | CrtFloatingPointExceptionResultDispatchVariant |

## Suggested next port target

The largest **named** cluster with no `src/` citation is the
**savegame / generic-list / command subsystem** in `0x00416d60–0x00417fc0`:
`Sav*` (save slot/tree/record), `List*` (named-object list), `Cmd*`
(command/request matching), `Osf*`, `Task*`, `Checkpoint*`, `Reward*`.
These already carry human names in the corpus, making them the cheapest
high-value port. The per-function pseudocode for every one of them is in
`docs/decompiles/full/LEGORR.full.c`.

## Caveats

- A `src/` citation of an entry address is a *strong* anchor (the file
  is anchored to that function) but is not proof the port is complete.
- 11 named entries preserved in `LEGORR.supplement.c` are
  absent from the full-corpus manifest at those exact entry addresses.
  They are retained for lookup but excluded from the 2,564-function
  coverage counts.

Regenerate: `python3 tools/coverage_ledger.py`
