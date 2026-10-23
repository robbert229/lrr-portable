# Reconstruction coverage ledger

Covers all **2564** local functions in `docs/decompiles/full/manifest.csv`
(the full Ghidra corpus, PR #126).

## Status counts

| Status | Count | Share |
| --- | ---: | ---: |
| ported | 293 | 11.4% |
| anchored | 10 | 0.4% |
| named | 39 | 1.5% |
| runtime-import | 46 | 1.8% |
| anonymous | 2176 | 84.9% |

- **ported** — entry address or `FUN_*` name cited directly in `src/`
  (293 functions)
- **anchored** — only an *interior* address (a call site / offset inside
  the body) is cited in `src/`; the function is touched but its entry is
  not named (10 functions)
- **named** — human name in `engine-region.c` / corpus but no `src/`
  citation yet (39 functions)
- **runtime-import** — CRT/runtime support or import thunks; not game
  logic, never ported (46 functions)
- **anonymous** — `FUN_*` only, unreferenced in `src/`
  (2176 functions)

**Game logic remaining to port: 2225 (86.8%).**

## Largest anonymous regions (by 64 KB page)

| Region | Anonymous functions |
| --- | ---: |
| `0x00047xxxx` | 343 |
| `0x00040xxxx` | 303 |
| `0x00048xxxx` | 255 |
| `0x00046xxxx` | 243 |
| `0x00043xxxx` | 231 |
| `0x00041xxxx` | 219 |
| `0x00049xxxx` | 193 |
| `0x00045xxxx` | 168 |
| `0x00044xxxx` | 160 |
| `0x00042xxxx` | 61 |

## Named-but-unported corpus functions

Human-named in the corpus but with no `src/` citation yet.

| Address | Name |
| --- | --- |
| `0x00402000` | CmdMarkMatching |
| `0x00402060` | CmdSetKind |
| `0x00402100` | CmdFlushAll |
| `0x00402cd0` | CmdCancelRef |
| `0x00403700` | CmdCancelByCoords |
| `0x00416d60` | IsInGame |
| `0x00416e00` | ActiveObjectList |
| `0x00416e20` | ListFindName |
| `0x00416e70` | SavClearVisited |
| `0x00416ed0` | ListAddName |
| `0x00416f40` | ListHasItem |
| `0x00416fb0` | ListSetNode |
| `0x00417020` | ListGetNode |
| `0x00417090` | LevelNodeGet |
| `0x004171c0` | SavTreeWalk |
| `0x004172f0` | SavTreeWalk2 |
| `0x004173e0` | SavTreeLoad |
| `0x004179b0` | SavMenuList |
| `0x00417df0` | SavRefreshSlots |
| `0x00417e50` | SavSlotCtx |
| `0x00417e70` | SavBuildCtx |
| `0x00417e90` | SavGetSlot |
| `0x00417ea0` | SavSetSlot |
| `0x00417f40` | SavWriteRecord |
| `0x00417f90` | SavRecordPtr |
| `0x00417fc0` | SavCommit |
| `0x00425c30` | TransitionControl |
| `0x004515a0` | MapFileReadHeader |
| `0x004515c0` | MapFileGetCell |
| `0x00456240` | SystemStateFlags |
| `0x00456550` | SetSystemStateFlags |
| `0x004593e0` | OsfWrite |
| `0x0045f750` | TaskBlockFinish |
| `0x0045f7b0` | TaskFlushImages |
| `0x0045f7d0` | TaskFlushFont |
| `0x004601e0` | CheckpointBlobFree |
| `0x004601f0` | CheckpointBlobGet |
| `0x00461790` | RewardDisplayTick |
| `0x00462800` | RewardScoreAdd |

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
- 8 functions named in `engine-region.c` (`EngineTick`, `EngineShutdown`,
  `SelectionClusterA/B`, `SelectionGateHelper`, `SavObjCallbackB`,
  `SavObjSerialize`, `EngineShutdownAlt`) are **absent from the full
  corpus**: Ghidra auto-analysis never created them as function entries
  (they live in the engine's indirect-call region — see
  `tools/ghidra/CreateEngineFunctions.java`). They are tracked here only
  as named entries in the engine-region corpus.

Regenerate: `python3 tools/coverage_ledger.py`
