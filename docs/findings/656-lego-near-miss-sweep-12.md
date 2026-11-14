# Finding 656 — Near-miss sweep 12 of Lego.c's 95–99% band

Issue #762, part of #388 and ADR 0005. Follows the Lego.c sweeps (findings
[626](626-lego-legoobject-near-miss-sweep.md),
[629](629-lego-legoobject-near-miss-sweep-2.md),
[634](634-lego-near-miss-sweep-4.md),
[644](644-lego-near-miss-sweep-7.md) and
[648](648-lego-legoobject-near-miss-sweep-9.md)). On `main` `1e84f9da`,
Lego.c (GAME-021, `src/game/lego_unit.c`) had 19 functions at 95–99%. Their
ledger address gaps total 22,592 bytes. The issue's 17.9 KB was an earlier
count; `004232e0` lego_engine_tick (4,736 bytes, 99.93%) has entered the band
since.

This change is meant to be stacked on #758 (the GAME-030/035/008 conversions,
cause A) and #759 (the original `legoGlobs` struct, cause B). It therefore
works on causes C, D and E, and on the two functions that had not been
examined before. Two functions reach 100%. None falls. Measured with full
`make reccmp-check` runs on `main` `1e84f9da` and on this change:

| Measure | `main` `1e84f9da` | This change |
| --- | ---: | ---: |
| New exact functions | | **2** |
| Functions that rise / fall | | 2 / 0 |
| Exact annotated functions (full) / mapped | 2,397 / 2,833 | **2,399** / 2,833 |
| Exact-function code bytes | 329,171 (51.23%) | **329,992 (51.36%)** |
| Reccmp accuracy, full / game-only | 96.98% / 96.64% | 96.99% / 96.64% |
| `opt_ref` annotated removed | 0 | 0 |
| Image end | `0x7f5000` | `0x7f5000` |

No exact match was lost and no function fell below the baseline, so there
are no drops to label. `make match-progress`, `make unit-data-offsets` (no
change), `make opt-ref-callers OPT_REF_ARGS=--summary` (0 removed) and
`make pe-audit` were run on the final tree. The three generated files are
committed.

## What closed the gaps

### 1. `004232e0` lego_engine_tick: the oxygen volume's two products (99.93 → 100)

The only difference was one x87 multiply. The original computes
`constant_negative_10000_get() / 10.0f` first (`fild; fmul K`), then
`oxygenLevel / 10.0f`, then their product. MSVC 5.0 reassociated the
unparenthesised `c / 10.0f * (oxygen / 10.0f)` into `(c * (oxygen * K)) * K`.
A `(float)` cast on the first quotient keeps it a separate product, as
finding 645 did for `00437940`. Without the cast, `(c / 10.0f) * ...` and
swapping the operands give the same reassociated code. `* 0.1f` scores lower.

### 2. `0042b510` lego_map_load_pipeline: nested tests, success return, falls off the end (98.20 → 100)

The port stored every test in `result` (`if ((result = f()))`) and returned
`result`. MSVC then compared the loaders' results with `cmp eax, ebx`. The
original uses `test eax, eax`, and `cmp edi, ebx` for the `malloc` result.
It has one epilogue: the failure branches jump to it with the failed test's
zero still in `eax`, and success sets `eax = 1`. The function's source shape:

- each loader's result is tested directly (`if (f())`), with no `result`
  variable;
- success is `return 1;` inside the innermost block;
- there is no failure `return`: the function falls off the end. With a
  `return 0;` the code matches up to an extra `xor eax, eax` epilogue
  (95.25%). With `result` kept for the map only, it scores 93.06%.

Natively the port needs the return, so it sits under
`#if !(defined(_MSC_VER) && _MSC_VER < 1200)`, as in `message_unit.c` and
`images_unit.c`. Native behaviour is unchanged: every failure still returns
0.

**Rule B (finding 538).** Removing `int result;` costs the symbol counter
one, from `0042b850` to the end of the unit (`counter_probe.py`). Over the
whole unit, `match-function` showed one function sensitive to that:
`00432360` lego_terrain_cell_primary_flags_mask_clear_and_refresh. It fell
from 100% to 84.85%, its `imul` operands swapped. Its phase has a 32-symbol
period (`sweep.py`, N = 0–47: `A` 16, `B` 16, `A` …), so writing the index
the other way round cannot fix it: MSVC normalises the product.

The balance is a real local in `0042cb70` lego_camera_grid_expiry: the block
pointer for the first test (`block = &blocks[y * width + x]` after the
`blockPos` stores, `if (block->f_10[11] != 0)`). It scores the same as
before (95.56% in `match-function`, 96.67% full). The other placements
tried all score lower:

- the pointer computed before the `blockPos` stores: 66.33%;
- every access through `block`: 34.62%.

`counter_probe.py` then shows −1 only from `0042b850` to `0042cb70`. Over
that range `match-function` finds no other code change, and from `0042cc90`
on the counter equals `main`'s. Across the whole unit the only score changes
are the two rises.

## Tried, no gain

- **`tools/msvc_permuter.py`**, 75 s with 3 workers on each of the 12 other
  band functions. Unlike in finding 648, a `lego_unit.c` compile now takes
  about a second, so the permuter enumerates its whole candidate set:
  `0042f350` 34, `004288e0` 9, `00429890` 8, `00430530` 7, `004291a0` 4,
  `0042cb70` and `00431b20` 3, `0042cda0` 2, `00424730`, `00426420` and
  `00428020` 1, and `00429470` 0. None scores higher. The search finished
  within its budget.
- **Rule B phases** (`sweep.py`, N = 0–47 inert declarations, scratch copies
  only): `0042cb70`, `00426420`, `00429470` and `004291a0` have one code
  throughout, so they are not phases. `00431b20` has a new code at every N,
  only from a numbered label in its body.
- **`00426420`** (zero stores scheduled before the pushes): split stores,
  `centre.z = wPos.z`, and the inlined subtract written out in three orders.
  It stays at 95.77%.
- **`0042cb70`** (`imul` before the blocks load): `blockPos` order, a comma
  expression, and the index as `width * y`, `x + y * width` and
  `(&blocks[y * width])[x]`. Every variant gives the same code.
- **`004288e0`** (the previous-key load hoisted, `cl`/`dl`): the expanded
  macro in both operand orders, in either of the two `0x2e` tests, and a
  nested `if`. Every variant gives the same code.
- **`0042f350`** (`ebp`/`edi`): four local orders give the same code. An
  unsigned bounds compare and the index written the other way round give
  the same code or score lower.

## Per-function results (the 95–99% band)

Sizes are the ledger's address gaps. Scores are full `make reccmp-check`
runs.

| Address | Function | Size | `main` | After | Cause of what is left |
| --- | --- | ---: | ---: | ---: | --- |
| `00429890` | lego_level_load | 6752 | 95.04 | 95.04 | A: `0045d7e0` (GAME-035, #758); D: stack slots |
| `004232e0` | lego_engine_tick | 4736 | 99.93 | **100.00** | section 1 |
| `00430530` | lego_terrain_cell_mutate | 2240 | 95.45 | 95.45 | B: the block reloaded for the third work-flag test (legoGlobs, #759); D: `imul` operands, read-modify-write `or` |
| `004288e0` | lego_level_debug_input_cell_mutator_dispatch | 2096 | 98.41 | 98.41 | C: previous-key load hoisted, `cl`/`dl` |
| `0042f350` | lego_terrain_neighbor_coordinate_check | 928 | 96.26 | 96.26 | C: `ebp`/`edi` |
| `0042b510` | lego_map_load_pipeline | 832 | 98.20 | **100.00** | section 2 |
| `00428020` | lego_entity_command_status_feedback_dispatch | 784 | 98.89 | 98.89 | B: end-of-array pointer (#759); C: `ecx`/`eax` |
| `004291a0` | lego_scene_marker_update_and_draw | 528 | 98.10 | 98.10 | D: stack slot sharing (finding 634) |
| `0042d2b0` | lego_entity_building_types_load | 432 | 96.91 | 96.91 | B: `ecx` at the shared `free` (finding 648, #759) |
| `0042d100` | lego_entity_rockmonster_type_table_load | 432 | 96.93 | 96.93 | B: as `0042d2b0` |
| `0042cf50` | lego_entity_minifigure_type_table_load | 432 | 96.93 | 96.93 | B: as `0042d2b0` |
| `0042cda0` | lego_entity_vehicle_type_table_load | 432 | 96.91 | 96.91 | B: as `0042d2b0` |
| `0042d460` | lego_upgrade_types_load | 416 | 96.75 | 96.75 | B: as `0042d2b0` |
| `00431b20` | lego_terrain_placement_blocked_adjacent_cell_scan | 336 | 96.04 | 96.04 | D: `imul` and the `blockPos` store scheduled before the blocks load |
| `00429470` | lego_camera_pointer_ray_route | 304 | 98.80 | 98.80 | E: `fstp` scheduled before the last pushes (same slot) |
| `0042cb70` | lego_camera_grid_expiry | 288 | 96.67 | 96.67 | D: `imul` before the blocks load; not a phase |
| `00426420` | lego_cursor_ray_to_camera_route | 256 | 95.77 | 95.77 | D: zero stores scheduled before the pushes |
| `004350a0` | lego_surface_type_descriptions_config_load | 208 | 98.55 | 98.55 | B: end-of-array pointer (#759) |
| `00424730` | lego_sine_phase_midpoint_blend_dispatch | 160 | 95.45 | 95.45 | B/E: `legoGlobs_HPBlocks - 4` (#759); `fadd` order |

## Why the other 17 are short

| Cause | Functions | Ledger bytes | What would close it |
| --- | ---: | ---: | --- |
| **A. Callee outside the unit** | 1 | 6,752 | #758 (GAME-035), then the stack slots |
| **B. Data layout** (legoGlobs as one struct) | 9 | 5,536 | #759; re-check after the stack rebase |
| **C. Integer register choice** | 2 | 3,024 | not found |
| **D. Scheduling, stack slots or statement shape** | 4 | 1,408 | not found |
| **E. x87 scheduling** | 1 | 304 | not found |

`00430530` is counted under B: its listed D differences sit in the same
reloaded-block sequence. `00428020` and `00424730` are counted under B too:
the struct base fixes their data operands, and `00424730`'s `fadd` order
may follow.

## Follow-ups

- After this change is rebased onto #758 and #759, re-check `00429890` and
  the nine cause-B functions, per the issue.

## Gate

- `make reccmp-check`: OK. Two functions rise and none falls.
- `make match-progress`, `make unit-data-offsets`,
  `make opt-ref-callers OPT_REF_ARGS=--summary` and `make pe-audit`: run,
  generated files committed.
- `make tools-test` (without `--allow-growth`) and `make winapi-link-check`:
  OK.
- Native: `cmake --build build` succeeds, ctest passes 37/37, and the game
  starts with the shipped WADs.

## Files

- `src/game/lego_unit.c`: `004232e0`'s cast, `0042b510`'s shape with its
  native-only `return 0`, and `0042cb70`'s block pointer.
- `docs/coverage/`: the match baseline, listing digests and binary progress.

No file owned by #758–#761 is touched.
