# Finding 662 — objectGlobs is one struct, and LegoObject.c's definition order

Issue #775, part of #388 and ADR 0005. This is the follow-up that finding 653
names. #768 made `legoGlobs` one struct. This change does the same for
LegoObject.c's state, `objectGlobs`. It also fixes LegoObject.c's
definition-order inversion (findings 657 and 658).

Measured on `main` `bf6335e3` (#771): one full matching build of `main` and
two full `make reccmp-check` runs of this change (one before the order fix
and the deviation retirements, one after), with the same scores.

| Measure | `main` | This change |
| --- | ---: | ---: |
| New exact functions | | **7** (all LegoObject.c) |
| Functions that rose / fell | | 11 / **0** |
| Exact functions | 2,408 | **2,415** |
| Exact-function code bytes | 335,096 (52.15%) | **336,831 (52.42%)** |
| Reccmp accuracy, full / game-only | 97.02 / 96.68 | **97.05 / 96.72** |
| Function-order inversions, game-only (pairs / adjacent) | 6 / 4 | **4 / 3** |
| Function-order inversions, full (pairs / adjacent) | 801 / 19 | **799 / 18** |
| `unit-data-offsets`: unit summary (partial / n/a) | 14 / 196 | 13 / 197 |
| `unit-data-offsets`: unattributed annotations | 79 | 79 |
| Image end (`make pe-audit`) | `0x7f5000` | `0x7f5000` |
| Naked asm in LegoObject.c | 1 | **0** |

No function fell below the baseline, so no drop needs a label. `make
match-progress`, `make unit-data-offsets` and `make pe-audit` were run on
the final tree, and the generated files are committed. `make tools-test`
(no `--allow-growth`), `make winapi-link-check` and native ctest (37 of 37)
pass. The native build starts and runs. `tools/binary_progress_inputs.txt`
lists the new header.

## 1. Extent and layout

LegoObject_Initialise (`00436c20`) zeroes exactly `0xc63c` bytes from
`004de790`. The struct therefore spans `004de790`–`004eadcc`, not
`004eadd0` as finding 653 estimated. The two words after it stay separate
globals:

- `004eadcc`, the drill-frame counter (`objectGlobs_f_eadcc`);
- `004eadd0`, the released-flock list (`s_entity_auxiliary_release_records`).

The whole range is past `.data`'s raw size (`004b2e00`), so the original
zero-fills all of it. The activity and ability name tables (`004decc4`,
`004ead60`) and the resource-slot table (`004dee18`) are filled at run time.

Before this change, 45 annotations covered the range, in five places:

- `legoobject_unit.c`: 8 stand-ins and 22 port-read fields;
- `lego_unit.c`: 6 (`004e8448`, `004eabc4`, `004eabc8` and the three tool
  tip icon arrays `004ead78`–`004eadbc`);
- `legoobject_port.cpp`: the two name tables.

Every field now sits at its original offset in `struct LegoObject_Globs`
(`src/game/legoobject_globs.h`). The table lists the fields with known
names; the struct names the others `field_<offset>`.

| Offset | Address | Field | Old name |
| ---: | --- | --- | --- |
| `0x0000` | `004de790` | `listSet[32]` | `s_pool_chunks` |
| `0x0084` | `004de814` | `field_84[300]` | `s_terrain_sfx_grid` |
| `0x0534` | `004decc4` | `activityName[79]` | `s_activity` |
| `0x0680` | `004dee10` | `listCount` | `s_pool_chunk_count` |
| `0x0684` | `004dee14` | `flags` | `s_entity_state_flags` |
| `0x06b4` | `004dee44` | `field_6b4[0x12c0]` | `s_grid_count_second`, with `s_task_counter_nonzero` = `[0xe10]` |
| `0x51b4` | `004e3944` | `field_51b4[0x12c0]` | `s_grid_count_first`, with `s_task_counter_zero` = `[0xe10]` |
| `0x9db4` | `004e8544` | `recordObjs[200][11]` | `objectGlobs_recordObjs` |
| `0xc034` | `004ea7c4` | `field_c034[256]` | `objectGlobs_f_4ea7c4` |
| `0xc43c` | `004eabcc` | `liveObjs[100]` | `s_idle100` |
| `0xc5d0` | `004ead60` | `abilityName[6]` | `s_ability_type_names` |
| `0xc630` | `004eadc0` | `ToolTipIcon_Ore` | `objectGlobs_toolTipIconOre` |
| `0xc638` | `004eadc8` | `field_c638` | `objectGlobs_f_4eadc8` |

The two "task counters" were annotated inside the two level-count tables.
They are element `[15][0][0]` of each `[20][15][16]` table, so they are
macros over that element.

`legoobject_unit.c` defines the struct under one annotation,
`// GLOBAL: LEGORR 0x004de790`, after every function. It is a common, as in
the original. The GAME-023 row of `unit-data-offsets` now lists
`004de790:_objectGlobs(size=50748)`. Its `.bss` status goes from `partial
(2, 9 unknown) gap 1 first 004eadc0: 618/4`, a mismatched gap, to `n/a`:
all of LegoObject.c's state is commons now. No other unit's row changes.

## 2. Method (finding 653's)

The units' and the port's code is unchanged. Each old name is a `#define`
over its field:

- **Fields only C units read** are the struct's in every build:
  `legoobject_unit.c`'s stand-ins and `lego_unit.c`'s six.
- **Fields a C++ port file reads too** are the struct's in the matching
  build only. Natively, `legoobject_unit.c` keeps them as separate
  globals, with their initial values (`legoobject_port.cpp` keeps the two
  name tables), and native behaviour does not change.

  In the matching build, the C++ files that read them include the header
  after their last declaration of the old names: `legoobject_port.cpp`,
  `entity.cpp`, `ai.cpp`, `level.cpp`, `terrain_grid.cpp` and
  `lego_port.cpp`. `legoobject_port.cpp`'s two table initialisers are
  compiled out there, as are `legoobject_unit.c`'s resource-slot
  initialiser and its natively kept globals. As with finding 653's two port
  initialisers, the matching build is scored, not played (finding 628).

The members have basic and `void*` types, and the header includes nothing,
not even `<stdint.h>`. Macros give the pointer fields their types.
`liveObjs` is `struct EntitySlot*` in C++, as the port reads it.

Two headers that C++ files share skip a declaration once the header has
made the name a macro (`#ifndef <name>`):

- `ai.h`: `s_entity_state_flags`, because `ai.cpp` includes `ai.h` after
  the header;
- `lego_unit.h`: `s_object_f_4eabc4` and `s_object_f_4eabc8`, for
  `lego_port.cpp`.

## 3. Symbol counter: every unit stays neutral

Finding 653's rule says that `legoobject_globs.h` costs 3: the new file,
the struct and the `extern`. `counter_probe.py` reports a difference of 0
at every function of every C unit touched:

- `legoobject_unit.c` (286 functions);
- `lego_unit.c`;
- `interface_unit.c`;
- `panel_unit.c`;
- `entity_stats.c`.

There are two exceptions:

- the three `s_activity` readers below, where the difference starts at the
  reader;
- the definition-order move (section 5), which shifts only the three
  functions it reorders.

- **legoobject_unit.c.** The include takes the place of two stand-in
  definitions (`004ea7a8`, `004eadc0`). Four of the other five stand-ins
  are kept as unreferenced declarations: `recordObjsCount`, `004eadc8`,
  `004dee0c` and `004e8444`. Each is written as an
  `#undef`/declaration/`#define` triple and labelled `Deviation (scoring
  only)`. The fifth (`recordObjs`) pays for the include's third symbol.
  The three mid-file declarations (`004eabc4`, `004eabc8`, `004ea7c4`) are
  triples too. The imports header's declarations of the port names are
  parsed before the include, so they stay, unreferenced.
- **lego_unit.c.** The include replaces the three tool tip icon
  definitions. `004eabc4` and `004eabc8` were redeclarations of
  `lego_unit.h`'s names (cost 0), and their definitions go.
  `s_lego_f_4e8448` stays as an unreferenced `extern`. Only the state
  block changes; no function body does, including the three #772 owns.
- **The single-declaration readers do without the header.** An `extern`
  of an undeclared struct tag, `extern struct LegoObject_Globs
  objectGlobs;`, costs 2: the tag and the variable. It replaces two old
  declarations, and offset macros reach the fields at the header's
  offsets:
  - `ai.h`, for `panel_unit.c`: `flags` and `field_c438`;
  - `interface_callees.h`, for `interface_unit.c`: the tool-tip sounds and
    `ToolTipIcon_Ore`;
  - `entity_stats.c`: the two level-count tables.

  Each one is matching-build C only and labelled `Deviation (scoring
  only)`.
- **building_model.c, upgrade.c, vehicle_model.c.** Each reads
  `s_activity[0]` in one function and declares no other field. Each one
  declares `objectGlobs` (2 symbols) immediately before that function,
  following finding 653's rule for its other units. The functions above
  keep their counter. None of the three units' functions moved.

`interface_unit.c`'s `0041baa0` used to read `lego_tooltip_ore_icon`,
`interface_port.cpp`'s unannotated stand-in. It now reads the original's
field. The tool-tip sound table, `object_tooltip_sfx`, is renamed there
instead of in `interface_unit.c`. That is one deleted `#define` in #773's
file, and no function body changes.

## 4. Per-function results (full report)

| Address | Function | `main` | After | Why |
| --- | --- | ---: | ---: | --- |
| `0043a930` | legoobject_tracked_pointer_remove | 45.45 | **100** | the count and the array are adjacent fields; the loop's bound and base are one struct's |
| `004372a0` | legoobject_spawn_coordinate_pool_clear | 91.67 | **100** | the two coordinate pools and their counts are cleared as fields of one struct |
| `00436c20` | legoobject_initialise | 96.67 | **100** | the name tables are fields (`objectGlobs+0x534`, `+0xc5d0`); the clear is now `memset(&objectGlobs, 0, sizeof objectGlobs)` |
| `00437050` | legoobject_shutdown | 96.00 | **100** | field operands |
| `00437150` | legoobject_tool_tip_icons_load | 99.17 | **100** | the icon arrays' end pointer is the next field (finding 653's left-over) |
| `00438310` | legoobject_pool_chunk_allocate | 97.22 | **100** | B closed: the count reload after the `listSet` store; the `volatile` deviation is retired |
| `0044c5f0` | legoobject_scatter_spawn_drain_callback | 93.55 | **100** | B closed: the count is reloaded after the store to the visited list |
| `00437940` | legoobject_deferred_olist_create_and_link | 84.17 | 91.36 | B partly closed (`recordObjs` and its count); the rest is scheduling (D) |
| `00428020` | lego_entity_command_status_feedback_dispatch | 98.89 | 99.26 | `objectGlobs` operands |
| `0041baa0` | interface_do_mouse | 76.38 | 76.56 | the ore icon is the original's field, not a port stand-in |
| `0045b310` | panel_button_collision_check | 57.28 | 57.76 | `flags` and `field_c438` are the original's fields; `s_lego_object_tool_tip_gate` was `ai.cpp`'s unannotated stand-in |

Of finding 653's recorded objectGlobs cause-B functions, `00438310` is
closed and `00437940` mostly closed. `00437150` and `00437050` are exact.
`0044bf00` is unchanged: its list is `004eadd0`, which lies outside the
struct.

### Deviations retired

- `00438310`: `LEGOOBJ_GC_LIST_COUNT`, a `volatile` read of the count. The
  function is now exact without it.
- `0043a960` lava_crust_activity_register: the `volatile` count increment.
  The code is identical without it, and the function stays at 100.
- `0044a390` type11_cell_coordinate_append: the **naked asm**. The plain C
  form reloads the count after each store, as the original does, and is
  exact (`make match-function` 100%, and 100 in the full report).
  LegoObject.c has no asm left.

Not retired: `0043f190`'s `volatile` read of `legoGlobs_f_556514`
(finding 645). Without it, the function falls from 100 to 94.39. The
rotated loop needs the reload on every pass, which one struct alone does
not give.

## 5. Definition order (findings 657 and 658)

`0043a120` legoobject_cell_step_command_issue_with_jitter was defined
after `0043a5f0`. It now sits between `00439e70` and `0043a300`, in the
original's order. The function is external, so it is emitted where it is
defined (finding 658), and no linkage change is needed.

Its definition costs 37 symbols, so `0043a300` and `0043a5f0` now see a
counter 37 higher, and `0043a120` one 52 lower. The functions after
`0043a650` see no change.

None of the three moved in the full report: 71.14, 100 and 81.00, as on
`main`. A sweep of 0–63 inert declarations before `0043a300` gave the
same code at every phase. In `make match-function`, `0043a300` and
`0043a5f0` show 14.90 and 94.29. This is a single-object artefact that is
the same before the move: callee names resolve differently without the
link (`stats_object_get_stats_flags1` against `entity_stats_word_get`).

Game-only function-order inversions go from 6 pairs (4 adjacent) to 4
pairs (3 adjacent). The two that remain are Lego.c's static callbacks
`00429110` and `004297a0` (finding 658), which belong to Lego.c's owner.

## Files touched in other threads' files (all mechanical)

- `src/game/lego_unit.c` (#772): the six stand-in definitions in the state
  block become the include and one kept declaration. No function body
  changes; `counter_probe.py` reports 0 at every function.
- `src/game/interface_unit.c`, `interface_callees.h` (#773): one deleted
  `#define`, and the two declarations' matching-build form (section 3).
  `counter_probe.py` reports 0.
- `src/game/panel_unit.c` (#776's area): none. Its only edit is in `ai.h`.

## Follow-up

- `0044bf00`'s list (`004eadd0`) and `004eadcc` follow the struct. They may
  be file statics of LegoObject.c rather than commons. That is not
  established here.
- Native port gaps are unchanged. Natively the port-read fields are still
  separate globals.
