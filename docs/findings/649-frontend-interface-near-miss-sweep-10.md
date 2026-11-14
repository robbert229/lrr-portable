# Finding 649 — Near-miss sweep 10 of FrontEnd.c and Interface.c

Issue #750, part of #388 and ADR 0005. This sweep follows the earlier
sweeps of the two units (findings 630, 635 and 636) and the link-mode and
CRT switches (#745, #746; findings 642 and 643). The units are:

- FrontEnd.c: GAME-017, `src/game/frontend_unit.c`;
- Interface.c: GAME-020, `src/game/interface_unit.c`.

Measured on `main` `5f251885`, with one full `make reccmp-check` before the
change and one after.

| Measure | `main` | This change |
| --- | ---: | ---: |
| New exact functions | | **7** (3 FrontEnd.c, 4 Interface.c) |
| Functions that rose / fell | | 14 / **0** |
| Exact-function code bytes | 319,635 (49.74%) | **321,128 (49.98%)** |
| Reccmp accuracy, full / game-only | 96.72 / 96.42 | **96.77 / 96.47** |
| Naked asm left in FrontEnd.c | 1 (`004103f0`) | **0** |
| Gate link | `/DEBUG /OPT:REF /OPT:NOICF` | unchanged |
| Image end (`make pe-audit`) | `0x7f5000` | `0x7f5000` |
| `opt_ref` annotated removed (exact) | 0 (0) | 0 (0) |

No exact match was lost, and no function fell below the baseline. `make
match-progress`, `make unit-data-offsets`, `make opt-ref-callers
OPT_REF_ARGS=--summary` and `make pe-audit` were run, and the generated
files are committed (`unit-data-offsets` changed no tracked file). `make tools-test` (no `--allow-growth`), `make
winapi-link-check` and native ctest (37 of 37) pass. The native build
starts and runs with the shipped data.

## What closed the gaps

### 1. Interface.c's type counts are legoGlobs' words (5 functions, 4 exact)

Finding 636 recorded five Interface.c functions as blocked by data layout.
They read the vehicle and building type counts as `lego_vehicle_type_count`
and `lego_building_type_count`, port globals that nothing writes. The
original reads legoGlobs at `0055627c` and `00556288`. entity.cpp keeps
that storage as `s_catalog_counts[5]`, and since #740 (finding 644) the
array has C linkage. In the matching build, `interface_callees.h` now
declares `s_catalog_counts` and maps `LEGO_VEHICLE_TYPE_COUNT` and
`LEGO_BUILDING_TYPE_COUNT` to `[0]` and `[3]`.

This removes one declaration before every function of the unit (two
externs become one). A whole-unit pass with `make match-function` showed
that no other function moved. Natively the port globals stay, so native
behaviour does not change. Finding 636's follow-up still applies there:
natively the counts read zero.

| Function | Before | After |
| --- | ---: | ---: |
| `interface_reset` `0041a920` | 85.19 | **100** |
| `interface_load_object_items` `0041ab00` | 94.65 | **100** |
| `interface_get_menu_item_count` `0041b3c0` | 93.33 | **100** |
| `interface_enumerate_menu_items` `0041b680` | 96.20 | **100** |
| `interface_init_object_items` `0041aa00` | 77.78 | 91.67 |

`0041aa00` still differs in register choice (C). The original loads the
building count for the `F2keys` `malloc` into `eax`, and ours uses
`edx`/`ecx`. One `rep stosd` is also scheduled after the next load. The
permuter (90 s) found no candidate.

### 2. frontGlobs is one struct (FrontEnd.c, 3 exact)

Finding 635 named this the largest lever left in FrontEnd.c. frontGlobs
(`00556fc0`, `0x890` bytes) was 61 separate annotated globals. It is now
the original's one `Front_Globs` struct:

- every field sits at its original offset;
- the six gaps are named fields (`field_40[4]`, `field_518`, `field_524[2]`,
  `field_860`, `field_86c[2]`, `field_884[3]`);
- one `// GLOBAL: LEGORR 0x00556fc0` annotation covers the whole struct.

The unit's code is unchanged: each old name is a `#define` to its field.
The part-2 record types that the struct holds (`Front_LevelLink`,
`Front_LevelSet`, `Front_RewardLevel`, `Front_SaveReward`,
`Front_SaveStruct18` and `Front_SaveData`) move up to part 1, before the
struct.

The unit's own readers are not the only ones. Reward.c reads
`g_front_main_menu_set`, `g_front_save_tree`, `g_front_save_menu_result`
and `g_front_save_menu_open` through `frontend_api.h`. Objective.c reads
`saveImageBigWidth`/`Height` through `objective_unit_imports.h`. These
names are now macros over `extern struct Front_Globs frontGlobs`, at the
fields' offsets, so neither header needs the struct's types. Natively
`g_front_save_tree` stays the port's own global, as before.
`frontGlobs` is defined in an `extern "C"` block for the native C++
build.

The single-function loop shows no fall in either unit: Reward.c's
`00462030` and `00462270` and Objective.c's `00458670` rise slightly
there. In the full report they are unchanged, because the gain was only
in naming.

| Function | Before | After | Why |
| --- | ---: | ---: | --- |
| `front_rock_wipe_play` `004103f0` | 89.47 (asm) | **100** | the C form gives `or al, 1; or al, 2`, as finding 635 predicted; the naked asm and its `#if` are deleted |
| `sav_refresh_slots_target` `00417df0` | 96.97 | **100** | the loop's end pointer `&saveData[6]` is now `frontGlobs+0x510` (`saveNumber`), as in the original |
| `front_level_link_callback_load_menu_item` `004177e0` | 69.33 | **100** | rule B: the new counter phase is one of the original's (finding 635, section 6) |
| `front_menu_item_select_test_string_collision` `00411360` | 86.14 | 92.45 | counter phase |
| `front_menu_item_slider_handle_input` `00411870` | 89.92 | 90.70 | counter phase |
| `front_menu_get_item_bounds` `00411530` | 59.50 | 59.92 | counter phase |

FrontEnd.c has no naked asm left.

### 3. Switch case order (3 functions rise)

MSVC 5.0 lays out case bodies in source order. Three FrontEnd.c switches
had the right cases in the wrong order. The original's order was read from
its call sequence. Each was then confirmed by scoring every order with the
single-function loop: all permutations where a switch has five blocks or
fewer, insertion moves otherwise.

| Function | Switch | Original's order | Before | After |
| --- | --- | --- | ---: | ---: |
| `front_load_menu_set` `004140c0` | item type | Cycle, Slider, Trigger, Next, RealSlider | 69.79 | **83.84** |
| `front_menu_update_menu_items_input` `00411a00` | focus input; other input | TextInput, Slider, RealSlider, Cycle, Select; Slider, TextInput, RealSlider, Cycle | 77.40 | **84.59** |
| `front_menu_update` `00412c00` | both draw switches | Cycle, Slider, RealSlider, TextInput, Select | 46.06 | **49.10** |

For `00411a00`, two orders of the first switch tie at 80.60 and two of the
second at 80.98 when scored alone. The pair committed scores 84.38 in the
single-function loop and 84.59 in the full report.

`004140c0` is still 0x50 bytes longer than the original. The original
computes `atoi(...) - 1` with a separate `dec eax` and cross-jumps the
`Next` case's `numParts == 6` and `*stringParts[4]` branches. Ours folds
the `- 1` into the displacement (`[ecx+eax*4-4]`) and keeps both calls.

## Per-function results

These are the functions of the two units' 95–99% and 80–94% bands, plus
the largest functions below 80%, from the full report. Letters are the
causes tallied below. "n.t." means not tried in this sweep.

| Address | Function | `main` | After | Change, or what is left |
| --- | --- | ---: | ---: | --- |
| **FrontEnd.c 95–99%** | | | | |
| `004157c0` | front_play_intro_splash | 97.72 | 97.72 | E: `fxch` before the `seconds` store; shared epilogue |
| `00413a60` | front_menu_load_slot_images | 97.59 | 97.59 | B: end pointers into FrontEnd.c `.bss` (an unannotated static at `004db8a4`; the rebuilt `.bss` order differs) |
| `00415220` | front_load_selected_level | 97.83 | 97.83 | A: ObjectRecall.c `00459360` is still a port adapter |
| `00410490` | front_cache_create | 98.21 | 98.21 | D: one store scheduled (effective match) |
| `00417eb0` | front_save_set_level_completed | 95.45 | 95.45 | C: `lea` operand order (effective match) |
| `00416d70` | front_get_selected_level | 96.67 | 96.67 | D: the original's `je` skips `xor eax, eax` (known zero); 5 shapes tried |
| `00417df0` | sav_refresh_slots_target | 96.97 | **100** | frontGlobs struct |
| `00417f40` | save_write_record_target | 96.43 | 96.43 | C: as `00417eb0` (effective match) |
| **FrontEnd.c 80–94%** | | | | |
| `00417430` | float_to_long_x87 | 90.91 | 90.91 | C: as `00417eb0` (effective match) |
| `00411360` | front_menu_item_select_test_string_collision | 86.14 | 92.45 | counter phase; C: the coordinate sums |
| `00411870` | front_menu_item_slider_handle_input | 89.92 | 90.70 | R: rule B |
| `00417090` | front_level_set_load_level_links | 90.32 | 90.32 | C: `levelName`/`index` in swapped registers; the original compares `count` against the known-zero `esi` |
| `00410c70` | front_menu_item_free_menu_item | 82.61 | 82.61 | D: every single insertion move of its 6 case blocks ties; MSVC still merges two `optional_resource_destroy` cases |
| `004103f0` | front_rock_wipe_play | 89.47 | **100** | frontGlobs struct; asm deleted |
| `00413b50` | front_menu_free_slot_images | 92.31 | 92.31 | B: as `00413a60` |
| `00417f90` | sav_record_ptr_target | 94.44 | 94.44 | C: as `00417eb0` (effective match) |
| **FrontEnd.c below 80%** | | | | |
| `00412c00` | front_menu_update | 46.06 | 49.10 | case order; C/D: `esi`/`edi`/`ebx` roles and stack slots throughout |
| `004140c0` | front_load_menu_set | 69.79 | 83.84 | case order; D: the folded `- 1` and the missing cross-jump; slots |
| `00411a00` | front_menu_update_menu_items_input | 77.40 | 84.59 | case order (two switches; the third ties at every order) |
| `00411530` | front_menu_get_item_bounds | 59.50 | 59.92 | C: all 6 case orders tie |
| `004177e0` | front_level_link_callback_load_menu_item | 69.33 | **100** | counter phase after the struct |
| **Interface.c 95–99%** | | | | |
| `0041b680` | interface_enumerate_menu_items | 96.20 | **100** | type counts |
| `0041c4f0` | interface_update_advisor | 98.58 | 98.58 | E: MSVC emits `+ h * 0.5f` as `- h * -0.5f`; 19 shapes tried (below); one code at counter offsets 0–63 |
| **Interface.c 80–94%** | | | | |
| `0041d040` | interface_is_item_available | 94.92 | 94.92 | D: both case orders tried (91.87 the other way) |
| `0041dca0` | interface_do_action | 86.90 | 86.90 | D: three of its four switches searched without gain (below) |
| `0041ab00` | interface_load_object_items | 94.65 | **100** | type counts |
| `0041b3c0` | interface_get_menu_item_count | 93.33 | **100** | type counts |
| `0041a920` | interface_reset | 85.19 | **100** | type counts |
| `0041ec30` | interface_draw_object_count | 89.19 | 89.19 | B: the loop's base register; the request arrays outside one legoGlobs struct |
| **Interface.c below 80%** | | | | |
| `0041aa00` | interface_init_object_items | 77.78 | 91.67 | type counts; C: register choice |
| `0041a850` | interface_shutdown | 70.31 | 70.31 | C: finding 636 |
| `0041baa0` | interface_do_mouse | 75.84 | 75.84 | permuter (90 s): no candidate |
| `0041ee80` | interface_get_dependency_at_point | 63.57 | 63.57 | permuter (90 s): no candidate |
| others below 80% | | | | n.t. (finding 636's causes) |

## Why the others are short

Each function is counted once, under its main cause. The count covers the
rows above that are still short of 100%.

| Cause | Functions | What would close it |
| --- | ---: | --- |
| **A. Callee outside the unit**: `00415220` (ObjectRecall.c `00459360`) | 1 | GAME-030's conversion |
| **B. Data layout**: FrontEnd.c `.bss` end pointers (`00413a60`, `00413b50`); legoGlobs request arrays (`0041ec30`) | 3 | the unit's static order and its missing static at `004db8a4`; one legoGlobs struct |
| **R. Rule B**: `00411870` | 1 | the unit's real declaration set |
| **C. Register choice or integer operand order**: `00417eb0`, `00417f40`, `00417430`, `00417f90`, `00411360`, `00417090`, `00411530`, `0041aa00`, `0041a850` | 9 | not found (four are effective matches) |
| **D. Scheduling, stack slots, block layout**: `00410490`, `00416d70`, `00410c70`, `00412c00`, `004140c0`, `00411a00`, `0041d040`, `0041dca0`, `0041baa0`, `0041ee80` | 10 | not found |
| **E. x87**: `004157c0`, `0041c4f0` | 2 | not found |

## What was tried and failed

So that a later sweep need not repeat it:

- **`0041c4f0`.** The original keeps `fmul 0.5f; fadd` for the image
  branch's `pos.y`, and uses `-0.5f` only in the back-button branch. Every
  shape tried gives `fmul -0.5f; fsubr`:
  - `+=`, `-= h * -0.5f` and `pos.y + h * 0.5f`;
  - `0.5f * h` and `h / 2.0f`;
  - the back-button sums without any `-0.5f` literal, or with `/ 2.0f`;
  - `height` as a ternary local.

  Three other shapes score differently:
  - `(float)(height * 0.5)` gives `fadd` but a `qword` constant (99.29%);
  - the ternary written inline in the expression scores 95.04%.

  A 0–63 inert-declaration sweep shows one code at every offset, so the
  fold is not rule B.
- **`00416d70`.** Five shapes give the same code: an early `return NULL`
  in each branch, an `else { return NULL; }`, `== -1` returns, `return 0`,
  and assignment in the condition.
- **`msvc_permuter.py`, 90 s each.** `0041aa00` (0 attempts), `0041ec30`
  (2), `0041b3c0` (5), `0041baa0` (15), `0041c470` (3), `0041f4d0` (5) and
  `0041ee80`: no improvement.
- **Case-order searches with no gain.**
  - `00411530`: 6 orders, all equal.
  - `00410c70`: every single insertion move of its 6 blocks, all at 82.61.
  - `00411a00`'s third switch: 6 orders, all equal.
  - `0041d040`: 2 orders.
  - `0041dca0`'s menu-type switch (line 2239, 2 orders) and object-type
    switch (line 2340, 6 orders).

### Unfinished: `0041dca0`'s outer switch

The search of `interface_do_action`'s outer `switch (type)`
(`interface_unit.c` line 2237 on this tree) was stopped after about 25
minutes, at the backlog manager's request. That switch has more than five
case blocks, so the search used single insertion moves from the source
order and reported none better than 86.90. It had not finished one pass
over all moves. The fourth switch (`switch (type)` at line 2449) was not
searched.

To resume, run the scratch searcher on that line (and on 2449). It splits
the switch at its own indentation into case blocks (adjacent labels stay
together), writes each order to the real source path, and scores it with
`tools/match_function.py 0x41dca0 --json`. Restore the file afterwards.
The searcher is not committed: it is about 60 lines, and the method above
is enough to rebuild it. It took about 4 s per compile.

## Files

- `src/game/interface_callees.h`: the type counts (matching build only).
- `src/game/frontend_unit.c`: `Front_Globs` and the field macros, the
  part-2 record types moved up, `004103f0` as C, and the case orders.
- `src/game/frontend_api.h`, `src/game/objective_unit_imports.h`: the
  external readers' names as frontGlobs offsets.
- `src/game/frontend_port.cpp`: a comment.
- `docs/coverage/match-baseline.txt`, `match-listing-digests.txt`,
  `binary-progress.json` and the other generated files: regenerated.
