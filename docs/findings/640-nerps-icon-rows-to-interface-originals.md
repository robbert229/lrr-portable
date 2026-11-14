# 640. NERPs's icon and block-pointer rows rebuilt: Interface.c's click, flash and selection functions get their original callers

Issue #732, part of #388 and #562. Follows findings
[637](637-opt-ref-reevaluation.md) (the `/OPT:REF` caller table) and
[634](634-lego-near-miss-sweep-4.md) (switching calls to annotated originals).
Measured on `main` `db760978` with full `make reccmp-check` runs before and
after.

| Measure | `main` `db760978` | This change |
| --- | ---: | ---: |
| Annotated functions in `tutorial_script.c` | 31 | **133** |
| New exact functions | | **102** (2,802 original code bytes) |
| Functions that rise / fall | | 1 / 0 (`00453da0`, 92.62 to **100**) |
| Exact-function code bytes (`binary-progress.json`) | 289,516 (45.06%) | 290,085 (45.15%) |
| `opt_ref` removed / removed exact | 124 / 102 | **115 / 92** |
| Annotated mapped / exact (full) | 2,663 / 2,146 | 2,765 / 2,249 |
| Port `.bss` (relink) | 1,116,672 | 1,116,672 |
| Image end | `0x875000` | `0x875000` |
| Rows of `tutorial_script_port_functions.h` | 276 | 184 |

No function fell below the match baseline, and the baseline diff is exactly
the 102 new functions plus `00453da0`. `make match-progress`,
`make unit-data-offsets`, `make opt-ref-callers OPT_REF_ARGS=--summary`,
`make tools-test`, `make winapi-link-check`, `make reccmp-lint` (931, unchanged)
and native ctest (37 of 37) pass.

**Why exact-function code bytes rise by only 569.** `binary_progress.py`
sums the spans of the original's function list (`load_functions`). That list
has no entries for these small NERP functions: they fall inside larger spans
(for example the ledger's `00455300` covers `00455300`, `00455340` and
`00455370`). So the 102 new exact functions add nothing to the measure, and
the +569 is `00453da0` alone (0x239 bytes). Reccmp's own counts (mapped and
exact, above) see them.

## The live callers were an attribution artefact

Finding 637 listed the NERPs caller as four annotated functions
(`nerps_object_count` `00455300`, `nerpfunc_set_level_fail` `00454b80`,
`nerps_runtime_update_camera` `00453da0`, `nerps_monsters_destroyed_reset`
`00454890`) that "call instead: no call (hook tables)". These four were never
the callers. `opt_ref_link.py callers` assigns each original call site to the
nearest annotated function at or below it. The real callers were unannotated
functions after those four:

- **NERP functions of the table** (`nerpsFunctions`, `.data` `004a5948`) that
  the matching build still served through `tutorial_script_port.cpp`'s
  `nerps_port_<name>` adapters, so they ran the port's interpreter, not
  Interface.c;
- **helpers outside the table** (`00454710`, `00454bd0`, `00454c10`,
  `00455340`, `00455370`) and **block-pointer callbacks** (`00454c50`,
  `00454ca0`, `00454d30`, `00455130`, `00455190`) that only those functions
  call or pass.

So "routing the hook-table calls" meant rebuilding those functions in
`tutorial_script.c` and pointing the table's rows at them. A rebuilt row
deletes its line from `tutorial_script_port_functions.h`, and with it its
port adapter (the adapters are generated from that X-macro). Natively nothing
changes: the native tutorial still runs `tutorial_script_port.cpp`, and the
native compile of the unit only gains the new definitions.

The call sites were found by disassembling the original's NERPs range
(`00452df0`-`004574e0`) for direct calls to the 12 dropped callees, then
mapping each to the table row whose function contains it.

## Per call site

Each row is one original call (or one family of identical calls) to a
function `/OPT:REF` dropped. "Rows" are NERP functions of the table.

| Original callee | Original callers (rebuilt here) | Before | After |
| --- | --- | --- | --- |
| `0041e7f0` interface_set_item_clicks | 19 `Set*IconClicked` rows (`004548c0`, `00454920`, `00455d50`-`00455f50`) | port adapters | direct call |
| `0041e810` interface_get_item_clicks | helper `00454710` nerps_get_icon_clicks, called by 19 `Get*IconClicked` rows (`004548e0`, `00454940`, `00455b30`-`00455d30`) | port adapters | direct call |
| `0041e890` interface_set_advisor_item, `0041e770` interface_set_item_flash | helper `00455370` nerps_flash_icon, called by 19 `Flash*Icon` rows (`00454900`, `00454960`, `00455f70`-`00456170`) | port adapters | direct calls |
| `0041e8d0` interface_get_advisor_type | `0041e890` (Interface.c) | dropped with its caller | live |
| `0041e820` interface_set_object_item_clicks | helper `00454bd0` nerps_set_object_icon_clicks, called by 8 building `Set*IconClicked` rows (`00455890`-`00455970`) | port adapters | direct call |
| `0041e7a0` interface_set_object_item_flash | helper `00454c10` nerps_flash_object_icon, called by 8 building `Flash*Icon` rows (`00455990`-`00455a70`) | port adapters | direct call |
| `0041e860` interface_get_object_item_clicks | helper `00455340` nerps_get_object_icon_clicks, called by 8 building `Get*IconClicked` rows (`00455810`-`00455880`) | port adapters | direct call |
| `0041e7e0` interface_get_primary_selected | 6 `Get*Selected` rows (`00454e20`-`00455000`) | port adapters | direct call |
| `00456cb0` nerps_block_pointer_enumerate | `SetTutorialBlockClicks` `00454c80`, `SetTutorialCrystals` `00454d10`, `SetOreAtIconPositions` `00454d90`, `GetTutorialCrystals` `00455160`, `GetTutorialBlockClicks` `004551c0`, with their callbacks; `SetTutorialCrystals`'s and `SetOreAtIconPositions`'s callbacks call LegoObject.c's `00439370`/`004394b0` by their unit names | port adapters | direct call |

All 92 rebuilt rows are single-row functions (no other table row names them).
In all, 102 functions are new: 92 rows, 5 helpers and 5 callbacks. Every one
is exact.

- **String literals.** The building and vehicle names (`"SmallTruck"`
  through `"Upgrade"`, `004a7f8c`-`004a8034`) carry `// STRING:` annotations
  at their first use. Without them Reccmp leaves the original's literal
  unnamed and the rows score 75% to 97%. `"Barracks"` is pooled with
  Interface.c's `004a354c`, already annotated there.
- **Declarations.** The table needs a prototype for each rebuilt row; each
  replaces the row's `nerps_port_<name>` prototype one for one. The new
  imports are 8 Interface.c and 2 LegoObject.c declarations in
  `tutorial_script_imports.h`, and two struct fields each in `NERPs_Block`
  (`+0x28`-`+0x30`) and `NERPs_Object` (`+0`, `+4`).
- **Rule B (finding 538/558).** `make match-function` over every function of
  the unit before and after shows no change in any pre-existing function. No
  counter shift had to be resolved.
- No filler declaration, no frozen `_for_`/`_from_` header edited, no
  `Deviation (scoring only)`, no naked asm. No other thread's file was edited.

## Per function: NERPs's near-miss bands

| Address | Function | Size | Before | After | Change, or what is left |
| --- | --- | ---: | ---: | ---: | --- |
| `00453da0` | nerps_runtime_update_camera | 569 | 92.62 | **100.00** | The rotation's end test compares against `(float)(amount - 1.0f)`: with the cast, MSVC loads the threshold first and compares with `fcompp`, as the original does |
| `004532e0` | nerps_runtime_resolve_argument | 62 | 95.00 | 95.00 | Reccmp names `call [ecx + nerpsFunctions+4]` on the rebuilt side only (`0x4a594c` stays a raw displacement on the original's). The code is identical |
| `00453bb0` | nerps_runtime_update_timers | 496 | 96.97 | 96.97 | The x87 operand order of `timers[i] + delta` (finding 558's table: not rule A). `match-function` calls it an effective match |
| `00454600` | nerps_draw_tutorial_icon | 256 | 97.37 | 97.37 | x87 scheduling: the original stores `bounce` after both `iconBounce` products, and emits `faddp` for the icon's y offset. 9 expression forms tried; none moves it |
| `00453220` | nerps_shutdown | 188 | 90.60 | 90.60 | `eax`/`ecx`/`edx` rotation in the sound-release loop. Four loop shapes tried. The callee `0046aa90` is the same function on both sides in the full run |

## `opt_ref` before and after

`make opt-ref-callers OPT_REF_ARGS=--summary`, default relink (`/OPT:REF` with
LINK's implicit ICF):

| Unit | Before: removed (exact) | After |
| --- | ---: | ---: |
| GAME-020 Interface.c | 9 (9) | **0** |
| GAME-028 [tutorial_script.c] | 4 (3) | 2 (1) |
| GAME-002 AITask.c | 4 (4) | 4 (4) |
| **All** | **124 (102)** | **115 (92)** |

All 9 Interface.c functions are now live, and so are NERPs's
`nerps_block_pointer_enumerate` and `flh_magic_uu_noop`.

## Remaining blockers

| Removed | Hangs below | Owner |
| --- | --- | --- |
| `004033f0` aitask_coordinate_pair_task_record_queue, `00403430` aitask_task_registry_remove_type0_entry_by_pair (AITask.c, exact) | NERPs's ~1 KB block-pointer callback `00456d00`. It is not rebuilt, and the original reaches it only through the `.data` pointer `004a676c` that the `MakeSomeoneOnThisBlockPickUpSomethingOnThisBlock` / `SetCongregationAtTutorial` / `...AtTutorial` rows (`00457130`-`004574a0`) pass to `nerps_block_pointer_enumerate` | NERPs (this unit), follow-up: rebuild `00456d00`, its pointer and those rows |
| `00454600` nerps_draw_tutorial_icon | `lego_engine_tick` `004232e0` calls `lego_port.cpp`'s `world_render_pass_b_original` | #731 (`lego_unit.c`, `lego_port.cpp`) |
| `00454880` nerps_monster_destroyed (exact) | `legoobject_deferred_olist_create_and_link` `00437940` calls `legoobject_port.cpp`'s `legoobject_gb_tutorial_counter_increment_adapter` | #730 (`legoobject_unit.c`) |
| `004021e0`, `00402210` (AITask.c) | folded by `/OPT:ICF` (finding 637) | `/OPT:NOICF` in the eventual gate link |

**Literal range.** `make unit-data-offsets` lists the 13 new `STRING`
addresses as unattributed. `translation-unit-ranges.csv` ends NERPs's string
range at `004a7f7c`, and Objective.c's starts at `004a8088`. The gap holds
only NERPs's object names (`"Func Call %s"` and `"SmallTruck"` through
`"LavaMonster"`). Regenerating the CSV (`tools/translation_units.py build`)
does not change it, because the range comes from the binary evidence, not
from annotations. Widening NERPs's range is a follow-up for that tool.

## Files

- `src/game/tutorial_script.c`: 102 functions in original shape, the table's
  rows pointed at them, the `(float)` threshold in `00453da0`, struct fields
  and the header comment.
- `src/game/tutorial_script_imports.h`: Interface.c's eight and LegoObject.c's
  two declarations.
- `src/game/tutorial_script_port_functions.h`: 92 rows deleted.
- `docs/coverage/`: match baseline, listing digests and binary progress.
