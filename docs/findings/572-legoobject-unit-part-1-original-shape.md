# Finding 572 — LegoObject.c (GAME-023) part 1 rebuilt in original shape

Issue #575, part 1 of 3, under [ADR 0005](../adr/0005-original-shape-c-units.md).
GAME-023 is the original's `LegoObject.c`: the object pool, object creation
and removal, and the per-object activity and AI updates (text
`00436c20`–`0044c710`, 258 functions). It was relocated verbatim into
`src/game/legoobject.cpp` ([finding 551](551-relocation-game-023-legoobject.md)).
This change rebuilds its first range, `00436c20`–`0043a960`, as the C unit
[`src/game/legoobject_unit.c`](../../src/game/legoobject_unit.c). Parts 2 and 3
convert `0043a9e0`–`0044c550`. [Finding 568](568-lego-unit-part-1-original-shape.md)
(Lego.c part 1) is the template.

The range is 86 functions in the unit map. The real count is **90**, because
Ghidra merged four functions into their neighbours. A scan of the original
`.text` found every in-range address that code or data references but the
unit map does not start. A second scan for `ret` followed by `0x90` fill
before an aligned address found the same four:

- `00437510`: a RunThroughLists callback pushed by `004374f0`;
- `00438390`: a 6-byte getter of `004eadc4`, after `00438310`'s padding,
  reached through the `jmp` thunk at `004565c0`;
- `004386b0`: the callback `00438670` pushes;
- `00438fb0`: the callback the spatial queries (`004389e0`–`00438b5e`) push.

All four are written and annotated. One part-2 function, `0043c490`, is
pulled forward into the unit (see "Callers").

"Before" is the *match baseline* at `9b30b563`. "After" is this change. Scores
come from the full matching build (`make reccmp-check`).

## Result

| | before | after |
| --- | ---: | ---: |
| Range similarity (byte-weighted, 91 functions, 15,112 bytes without `0x90` fill) | 29.82% | **94.33%** |
| Exact functions in the range | 15 | **65 of 91** |
| Mapped functions in the range | 86 | 91 |
| Exact functions, whole binary | 1,026 | 1,077 |
| Accuracy, full / game-only (`binary-progress.json`) | 66.60% / 63.15% | **68.48% / 65.24%** |
| Exact-function code bytes | 88,637 | 93,917 |
| Unit-relative identity | 14.85% | 16.12% |

Byte weights are the original's code bytes without the `0x90` fill. An
unmapped function counts as 0%. The 91 functions are the range's 90 and
`0043c490`.

| Address | Function | Before | After |
| --- | --- | ---: | ---: |
| `00436c20` | `legoobject_initialise` | 82.45 | 95.83 |
| `00437050` | `legoobject_shutdown` | 52.00 | 96.00 |
| `004370b0` | `legoobject_render_config_copy_out` | 17.65 | **100.00** |
| `004370d0` | `legoobject_render_config_copy_in_or_clear` | 32.65 | **100.00** |
| `00437100` | `legoobject_grid_record_count_difference_sum` | **100.00** | **100.00** |
| `00437150` | `legoobject_tool_tip_icons_load` | 22.90 | 94.17 |
| `004372a0` | `legoobject_spawn_coordinate_pool_clear` | 13.33 | 91.67 |
| `00437300` | `legoobject_process_queued_grid_animations` | 37.95 | 98.36 |
| `004373d0` | `legoobject_activity_toggle` | 76.06 | **100.00** |
| `00437440` | `legoobject_grid_record_count_tables_clear` | **100.00** | **100.00** |
| `00437460` | `legoobject_grid_record_count_difference_get` | 20.83 | **100.00** |
| `004374a0` | `legoobject_grid_record_count_get` | 15.79 | **100.00** |
| `004374d0` | `legoobject_task_counter_increment_by_branch` | **100.00** | **100.00** |
| `004374f0` | `legoobject_callback_dispatch_unless_type_five` | 22.22 | **100.00** |
| `00437510` | `legoobject_linked_route_clear_callback` | — | **100.00** |
| `00437540` | `legoobject_remove` | 22.57 | 80.11 |
| `004377b0` | `legoobject_run_through_lists_skip_upgrade_parts` | **100.00** | **100.00** |
| `004377d0` | `legoobject_run_through_lists` | 35.22 | **100.00** |
| `00437880` | `legoobject_rename` | — | **100.00** |
| `004378e0` | `legoobject_grid_object_records_release` | 47.50 | **100.00** |
| `00437940` | `legoobject_deferred_olist_create_and_link` | 17.49 | 81.09 |
| `00437c20` | `legoobject_deferred_olist_queue_add` | 35.37 | 98.04 |
| `00437cc0` | `legoobject_flag40000_threshold_compare` | **100.00** | **100.00** |
| `00437d00` | `legoobject_create` | — | 98.80 |
| `004382c0` | `legoobject_pool_allocate` | 62.75 | **100.00** |
| `00438310` | `legoobject_pool_chunk_allocate` | 34.38 | 97.22 |
| `00438390` | `legoobject_global_animation_state_get` | — | **100.00** |
| `004383a0` | `legoobject_global_animation_state_set` | **100.00** | **100.00** |
| `004383b0` | `legoobject_rover_work_queue_update` | 19.86 | **100.00** |
| `00438460` | `legoobject_kind_record_update_dispatch` | 39.51 | 98.94 |
| `00438580` | `legoobject_flag80000000_set_or_clear` | 27.78 | **100.00** |
| `004385b0` | `legoobject_shared_eligibility_check` | 73.08 | 96.00 |
| `00438610` | `legoobject_create_in_world` | 27.78 | **100.00** |
| `00438670` | `legoobject_at_grid_position_find` | 22.95 | **100.00** |
| `004386b0` | `legoobject_grid_position_match_callback` | — | 97.50 |
| `00438720` | `legoobject_available_resource_spend` | 24.00 | **100.00** |
| `00438770` | `legoobject_building_upgrade_resource_admission_check` | 13.95 | **100.00** |
| `004387f0` | `legoobject_building_upgrade_commit` | 23.73 | 98.46 |
| `004388b0` | `legoobject_action_kind_prerequisite_check` | 18.12 | **100.00** |
| `00438960` | `legoobject_unknown_00438960` | — | 42.42 |
| `004389e0` | `legoobject_route_marker_candidate_find` | 18.82 | **100.00** |
| `00438a60` | `legoobject_query_center_mask01` | 14.04 | 97.14 |
| `00438ae0` | `legoobject_query_center_mask02` | 18.18 | 97.78 |
| `00438b80` | `legoobject_query_center_mask_from_argument` | 22.22 | **100.00** |
| `00438bf0` | `legoobject_query_center_mask80` | 23.53 | **100.00** |
| `00438c60` | `legoobject_candidate_search` | 24.24 | **100.00** |
| `00438cd0` | `legoobject_query_center_mask20` | 33.33 | **100.00** |
| `00438d30` | `legoobject_query_center_mask10` | 33.33 | **100.00** |
| `00438d90` | `legoobject_query_center_mask200` | 33.33 | **100.00** |
| `00438df0` | `legoobject_lookup_at_world_position` | 11.76 | **100.00** |
| `00438e50` | `legoobject_query_center_mask100` | 16.33 | **100.00** |
| `00438ed0` | `legoobject_flags_contain_requested_single_bit` | **100.00** | **100.00** |
| `00438f60` | `legoobject_dock_definition_matches_cell` | 30.30 | **100.00** |
| `00438fb0` | `legoobject_route_marker_candidate_visit` | — | **100.00** |
| `00439200` | `legoobject_task_kind_filtered_search` | 23.16 | **100.00** |
| `00439240` | `legoobject_task_flags_and_cell_task_match` | 73.17 | **100.00** |
| `00439280` | `legoobject_build_placement_command_batch_expand` | 47.13 | **100.00** |
| `00439340` | `legoobject_build_placement_command_pair_create` | **100.00** | **100.00** |
| `00439370` | `legoobject_build_placement_command_setup` | 17.06 | **100.00** |
| `004394b0` | `legoobject_build_placement_alternate_command_setup` | 14.68 | **100.00** |
| `004395e0` | `legoobject_display_name_lookup` | 11.38 | **100.00** |
| `004396c0` | `legoobject_item_name_by_kind` | 25.00 | 86.36 |
| `00439790` | `legoobject_the_name_get` | 75.41 | **100.00** |
| `00439860` | `legoobject_name_get` | 75.41 | **100.00** |
| `00439930` | `legoobject_copy_two_dword_pair` | **100.00** | **100.00** |
| `00439950` | `legoobject_tool_name_strings_load` | 42.11 | 92.00 |
| `00439990` | `legoobject_activity_scene_state_update` | **100.00** | **100.00** |
| `004399c0` | `legoobject_flag400_and_health_threshold_check` | 22.86 | **100.00** |
| `00439a20` | `legoobject_gated_step_command_issue` | 20.36 | **100.00** |
| `00439b80` | `legoobject_task_target_cell_eligibility_check` | 31.78 | **100.00** |
| `00439bd0` | `legoobject_accepts_target_for_task_check` | 41.67 | **100.00** |
| `00439c80` | `legoobject_indexed_flag_set_and_refresh` | 25.81 | 96.77 |
| `00439cd0` | `legoobject_animation_state_set` | **100.00** | **100.00** |
| `00439cf0` | `legoobject_stats_flag10_test` | **100.00** | **100.00** |
| `00439d10` | `legoobject_stats_flag20_test` | **100.00** | **100.00** |
| `00439d30` | `legoobject_stats_flag40_test` | **100.00** | **100.00** |
| `00439d50` | `legoobject_vehicle_unload_adjacent_cell_find` | 11.11 | 98.72 |
| `00439e10` | `legoobject_vehicle_unload_cell_find_if_eligible` | 85.00 | **100.00** |
| `00439e40` | `legoobject_record_flag20_query` | 85.00 | **100.00** |
| `00439e70` | `legoobject_attached_object_list_detach` | 13.73 | 99.59 |
| `0043a120` | `legoobject_cell_step_command_issue_with_jitter` | 8.29 | 80.37 |
| `0043a300` | `legoobject_deposit_activity_gate` | 23.29 | 70.39 |
| `0043a5f0` | `legoobject_visual_resource_get_or_create` | 16.16 | **100.00** |
| `0043a650` | `legoobject_action_kind_create_and_command_issue` | 25.90 | **100.00** |
| `0043a7c0` | `legoobject_action_kind_supported` | 20.45 | **100.00** |
| `0043a850` | `legoobject_action_record_consume_and_cleanup` | 26.18 | **100.00** |
| `0043a8f0` | `legoobject_simulation_update_counter_add` | 17.14 | **100.00** |
| `0043a910` | `legoobject_simulation_mode_counter_add` | 37.50 | **100.00** |
| `0043a930` | `legoobject_tracked_pointer_remove` | 36.36 | 45.45 |
| `0043a960` | `legoobject_lava_crust_activity_register` | 31.11 | 79.41 |
| `0043c490` | `legoobject_task_type_and_condition_check` (part 2) | **100.00** | **100.00** |

Every function in the range scores at or above its baseline.

## How the conversion is laid out

As in finding 568:

- **One `.c` file for the whole unit.** `legoobject_unit.c` (`unit GAME-023`)
  holds part 1 in address order; parts 2 and 3 append to it. The stem is
  `legoobject_unit` because `legoobject.h` is already the boundary header of
  the port's LegoObject.c entries that other C units call. Its records and
  prototypes are in `legoobject_unit.h`, the callees in
  `legoobject_unit_imports.h`.
- **`legoobject.cpp` stays a `legacy` entry**, key `00446610` (the lower
  median of its remaining markers), linked right after the unit. Part 3
  retires it with `link_order.py`'s `retire_legacy`.
- **The port's implementations stay in `legoobject.cpp`, in place,
  unannotated.** Their `// FUNCTION` lines became
  `/* Port model of 0x… (GAME-023) */` and
  `// PORT-ONLY: port model of 0x…, retires to the tail in GAME-023 part 3`.
  Fifteen of the port's helpers that only those models use carry
  `// PORT-ONLY: helper of GAME-023 part-1 port models, …`. No port code moved,
  so the declarations before parts 2 and 3 are unchanged.

### The record

`Lego_Object` (`legoobject_unit.h`) is the original's 0x40c-byte record:
`00438310` allocates the pool in strides of 0x40c and links free records
through `+0x408`. The 0x674 of [finding 565](565-entity-link-pair-2fc-300-merge.md)
is the port's `EntitySlot`, which has another layout. The name is
`Lego_Object` because GAME-020's `interface_callees.h` defines a partial
`LegoObject` view and the boundary test includes both. Members are named by
role where the port or the code shows it (`activityCont` `+0x2e4`,
`activityName1/2` `+0x2e8/+0x2ec`, `routeToObject` `+0x2fc`,
`interactObject` `+0x300`, `carriedObjects` `+0x308`, `flags1`–`flags4`
`+0x3e0`–`+0x3ec`), otherwise by offset. Struct members do not move the
symbol counter, so members found while matching were folded in after the
merge (same code as the `LEGOOBJ_FIELD` reads they replaced).

### State

The original keeps its state in one `LegoObject_Globs objectGlobs`
(`004de790`–`004eadd0`). The port models its fields as separate globals in
`legoobject.cpp`, each annotated at its address:

- **The port's globals are read in place, with C linkage.** The state block
  in `legoobject.cpp` is wrapped in `extern "C" { }`. Statics the unit reads
  (`s_task_counter_*`, `s_render_config_snapshot`, `s_type11_cell_*`) lose
  `static`; `s_type11_*` left their namespace. The frozen
  `*_from_legoobject.hpp` headers declare them with C++ linkage, so
  `entity.cpp`, `level.cpp`, `ai.cpp`, `terrain_grid.cpp` and
  `legoobject.cpp` pre-declare them `extern "C"` before the frozen include.
  The gate on this step alone showed no drift and two rises: `0x0044a390`
  53.85% → 100% and `0x0044a3d0` 44.97% → 54.44%. The namespace had kept
  Reccmp from naming `s_type11_*`.
- **Fields the port does not model** are annotated globals in the unit:
  `objectGlobs_queuedAnimationFraction` (`004ea7a8`),
  `objectGlobs_toolTipIconOre` (`004eadc0`), `objectGlobs_recordObjs`
  (`004e8544`) and its count (`004ea7a4`), and the legoGlobs word
  `legoGlobs_f_556454`.
- **`.data`**: the four static stats templates `00437d00` assigns
  (`004a97a0`, `004a98f0`, `004a9a40`, `004a9b90`; 0x54 dwords, first word
  0x30).
- **The unit's 0.0f constant** (`0049e3a4`, about 109 uses in LegoObject.c)
  is annotated in the unit as `legoobject_zero`; `legoobject.cpp` keeps the
  port's `s_eligibility_floor` unannotated.
- **legoGlobs fields** are read through `lego_unit.c`'s stand-ins and the
  port's annotated globals (`entity.cpp`'s object-name tables get the same
  pre-declarations). `legoGlobs.langToolName` (`00556228`) directly follows
  `toolName` (`005561fc`, `ai.cpp`'s `s_tool_type_names`), and the original
  addresses it from there; the unit does the same.

`make unit-data-offsets`, GAME-023 row: `.data` partial (98, 0 unknown, gap
6 from `004a5048`); `.rdata` n/a (one item); `.bss` partial (2, 28 unknown,
gap 1 from `004eadc0`); commons `objectGlobs_recordObjs`,
`objectGlobs_recordObjsCount`, `legoGlobs_f_556454`.

### Strings

98 `// STRING` markers. 91 replace `GLOBAL` lines removed from
`legoobject.cpp` (78 activity names, 6 ability names, 7 tool names; the
arrays stay). The others are the unit's own literals, among them
`0x004a48b4` `"LegoObject_ToolType_FreezerGun"`, `"ToolTipIcons"`,
`"Blank"`, `"ToolNames"`, `"Short"` and `"Normal"`. Pooled
literals other units annotate stay unmarked: `"Activity_Stand"` (`lego.cpp`),
`"Ore"`, `"PowerCrystal"` and the item names `004396c0` returns
(`encyclopedia.cpp`, `lego.cpp`).

## Callees

- **`extern "C"` in place**: `level.hpp`, `entity.hpp`, `cmd.hpp`, `ai.hpp`,
  `terrain_grid.hpp`, `upgrades.hpp`, `gods98/record.hpp`, `gods98/view.hpp`,
  `gods98/resource.hpp`, `gods98/surface.hpp`. Headers that C units include
  use the `#ifdef __cplusplus` form.
- **Pre-declarations before frozen includes** in `entity.cpp`, `level.cpp`,
  `ai.cpp`, `terrain_grid.cpp`, `weapon.cpp` and `legoobject.cpp`. **A
  function pre-declaration is not free:** the frozen redeclaration's named
  parameters are counted again. Pre-declaring `entity_building_index_set`
  (`00469c10`) in `legoobject.cpp` cost `0x00444fb0` an instruction (rule B).
  Variable pre-declarations measured free. Instead, `legoobject.cpp` wraps
  the frozen `#include "entity_from_entity_stats.hpp"` in `extern "C" { }`,
  which adds no declaration; the file uses none of its other entries.
  `entity.cpp` (which does) keeps the pre-declaration; none of its functions
  moved.
- **`00431570` Lego_GetMap** takes no argument; the port's
  `level_current_terrain_get` takes two it ignores. The unit calls it through
  a cast to `void* (*)(void)`, so the call names the annotated function.
- **`0041f8e0`** is `noop_callback(void)` in `lego_unit.c` (#544). The
  original is Lego_GetCrystalCount: it takes a flag and returns a count, and
  `0043a7c0` and `00438720` test the result. The unit calls it through
  `int (*)(int)`. `lego_unit.c` is #574's file; left as a follow-up.
- **`0043a7c0`** takes four arguments; it reads three.
- **Adapters** (unannotated, `// PORT-ONLY:`), where no port entry has the
  original contract: at the end of `legoobject.cpp` (groups B, C, G:
  `0043f560`, `0044be30`, `00409a90`, `00454880`, `0044b370`, `0044b680`,
  `00447b30`, the `00472060` callback); in `entity_stats.cpp` (`00469c10`
  for `004387f0`). `vehicle_model.cpp`'s existing GAME-021 adapter serves
  `0046cf90`. The GAME-013 `legoobject_block_position_get` serves `004424f0`,
  and is the usual remaining operand difference.
- **Part-2/3 callees with other parameter types** (`00441700`'s float arrays
  for block coordinates, `004410f0`'s float count): the unit passes the
  original's values through casts. These callees are part 2 and 3.

## Callers

### Matching-build aliases

Once an address names the unit's function, every port caller of that
address drops: the original side shows the unit's name, the rebuild still
calls the port model. The first full build after the merge lost **18 exact
matches** this way (GAME-013's electric fences, GAME-020's
`interface_any_object_can_reach*`, `lego_unit.c`'s `00424810`/`00425b40`,
`lego.cpp`'s cell-flag functions, `reward.c`, `message_unit.cpp`,
`weapons.cpp`, and `legoobject.cpp`'s `0043c490`).

Those callers already use the original's contract (they were exact against
it). They cannot call the unit natively: the unit's functions read
`Lego_Object`, and native objects are `EntitySlot`. So the switch is for the
matching build only:

- [`legoobject_port_aliases.h`](../../src/game/legoobject_port_aliases.h)
  maps each port name to the unit function at the same address, under
  `#if defined(_MSC_VER) && _MSC_VER < 1200` (MSVC 5.0 only). The list is
  generated from `legoobject.cpp`'s port-model markers, plus six adapters
  other port tails give converted C units with the original contract
  (`lego_object_can_eject`, `lego_object_create_adapter`, …), which
  `lego.cpp` excludes because it defines three of them.
- A caller includes it first, before any declaration. A macro renames a
  declaration or call one for one, so the symbol counter does not change.
  Every aliased entry has C linkage (five more port declarations got
  `extern "C"` for `lego.cpp`).
- Included by `terrain_cell_objects.c`, `interface_unit.c`, `reward.c`,
  `lego_unit.c`, `lego.cpp`, `message_unit.cpp`, `weapons.cpp`,
  `grid_cell_records.cpp`, `record_chains.cpp`, `dependencies.cpp`,
  `encyclopedia.cpp` and `objective_unit.cpp`.
- **Not** `placement.cpp`: including it there added an instruction to
  `0x004089d0` (two of its names alias to one function, so a declaration
  became a redeclaration). Its two callers stay as drift.
- **Not** `legoobject.cpp`, which defines the port models: a renamed
  definition adds a name to the TU. Its one exact caller, `0043c490`
  (part 2), is **pulled forward** into the unit instead, at the end, in a
  marked section; part 2 moves it into address order.
- **Not** `aitask.cpp` (#570's file).

Result: **no exact match is lost.**

### Callers that keep the port's functions

Natively, every caller keeps the port's functions: no unit function shares
its port name. The port models take `EntitySlot`, `LevelState` or a
`GameContext`, add NULL or bounds guards (`004370b0`, `00437460`,
`004374a0`), or walk the port's 0x410-stride pool; none has the original's
contract natively. They switch when the port reads the original record. In
the matching build, the callers above switch through the aliases;
`legoobject.cpp`'s parts 2 and 3, `aitask.cpp` and `placement.cpp` still call
the port models.

## Shape notes

- **Frames.** In optimised functions MSVC 5.0 lays scalars out in
  declaration order and aggregates after them. `0043a120` matched only with
  the block position as two `int`s, not a `Point2I`; `00439e70` needed the
  object position as a three-float vector, of which `GetPosition` fills two.
- **Dead parameter slots.** MSVC reuses a parameter's slot for locals once
  the parameter lives in a register (`00439a20`, `0043a120`, `0043a300`,
  `004387f0`'s `cost`). It is not controlled by declaration order.
- **Merged tails.** `0043a650` posts its message from two calls, which MSVC
  merges, not from a ternary. Its `switch` cases are in the order 9, 6/7, 8.
- **Loops.** `0043a8f0`/`0043a910` match only as `while (count-- > 0)` with an
  unsigned count (`jbe`). `00437460` matches only through a `[15][16]` index.
- **Late assignment.** In `0043a120`, `markPending = 1` follows the random
  side draw, in a caller-saved register.
- **Struct aliasing.** The original's `liveObjs[]` and `liveCount` are fields
  of one `objectGlobs`, so a store to the array reloads the count
  (`0043a930`, `0043a960`). The port keeps them as separate globals.
- **Return flow.** `00438720`, `00438770`, `004399c0` and `00439b80` match
  only with the success path as the fall-through.
- **`00437150`** keeps the original's checks: `if (i == 6)` sits outside
  `if (i == 11)`. Its second name is `"Ore"`; the port's model compares
  `"Cursor"` (follow-up below).
- No function in the range was compiled without optimisation.

### Near misses

| Address | Score | What is left |
| --- | ---: | --- |
| `0043a930` | 45.45 | the count reload of struct aliasing (above) |
| `00438960` | 42.42 | MSVC folds `mask = 0x2000; if (alt) mask |= 0x4000;` into one store in every form tried (plain `|=`, pointer, volatile cast, bitfield, byte write, initializer, `memset`, ternary, reordering); a counter sweep of 0–19 and the permuter changed nothing |
| `0043a300` | 70.39 | the frame is one dword larger: the original lets the uninitialised direction share the object parameter's dead slot; declaration orders and the permuter changed nothing |
| `0043a960` | 79.41 | the count reload; the `004424f0` adapter |
| `0043a120` | 80.37 | case 0 copies the jitter with integer moves where the rebuild uses the x87 stack and tail-merges the store; a struct copy gave 71% |
| `00437540`, `00437940` | 80.11, 81.09 | 97.87% and 80.25% in group B's own tree; the merged unit re-rolled their registers and scheduling (rule B); the permuter found nothing |
| `004396c0` | 86.36 | operand names only: `encyclopedia.cpp`/`lego.cpp` annotate the item names as port globals |
| `004372a0`, `00436c20` | 91.67, 95.83 | MSVC sinks the zero stores and a `push` after the inlined `memset` in every order tried (not rule B: 0–39 scratch declarations never changed it) |
| `00439950` | 92.00 | the loop end: the original names `00556254` after the next legoGlobs field |
| the rest at 94–99.6% | | the `004424f0`, `00469c10`, `0043f560` and `0046cf90` adapters, `"Activity_Stand"`/`"Ore"` port globals, the `0x700100` immediate Reccmp reads as an address on one side |

No naked asm was added. `00439930` was already naked in the port; its unit
version is C.

## Entropy and drops

`make reccmp-check` against `9b30b563`: 78 rose, 5 newly mapped,
**41 below baseline, all `drift; instructions +0`**. No exact match is lost
and no instruction count changed.

- 28 in `legoobject.cpp` (parts 2 and 3), 11 in `aitask.cpp` (#570's file),
  2 in `placement.cpp`.
- For each, the rebuilt listing was compared with the baseline build's,
  with call targets masked: **34 are identical**. The other 7 (`0x00404fc0`,
  `0x00405c10`, `0x00409470`, `0x0043bc40`, `0x0043f7d0`, `0x00444460`,
  `0x0044afb0`) differ only in data operands: unannotated addresses moved
  with the data layout, and the flag `0x800000` now falls inside another
  `.bss` symbol (`s_variant_next[106]` → `s_variant_pool[…]`), which Reccmp
  names.
- The score changes come from the original side, which now names the unit's
  functions where these callers still call port models.

Under ADR 0005's entropy rule 2 the baseline is refreshed.

## Reccmp lint ratchet

The ledger goes from 1,490 to **1,475** entries, updated without
`--allow-growth`: the fifteen `legoobject.cpp` helpers that only part-1 port
models use are marked `PORT-ONLY`. The port models and adapters carry
`PORT-ONLY` and add no keys.

## How the work was split

Seven groups by bytes (A `00436c20`–`00437510` … G `00439e70`–`0043a960`)
shared one scaffold (`f07c0aea`): the struct, the 90 prototypes and the
state's C linkage were fixed first, so each group owned only its own
sections. The shared disk ran out mid-run; the backlog manager capped
helpers at two worktrees. Groups A and D ran as helpers; B, C and E were
stopped, saved as WIP branches and finished in the main worktree with F and
G. Merging re-rolled rule B (above) and exposed the caller drops, both fixed
or recorded here.

## Follow-ups

- **`0041f8e0`** in `lego_unit.c` should become Lego_GetCrystalCount(flag)
  (`int`), not `noop_callback(void)`; remove the casts in
  `legoobject_unit_imports.h` then.
- **The port's `00437150`** compares the second icon name with `"Cursor"`;
  the original compares `"Ore"` (native gap, #580, `port` label).
- **One objectGlobs.** `liveObjs`/`liveCount` (and the rest) as one struct
  would let `0043a930`/`0043a960` match; it needs the frozen headers'
  declarations to go first.
- **Parts 2 and 3:** move `0043c490` into order; drain `legoobject.cpp`'s
  remaining aliases' callers; consolidate the groups' small record views
  (`LegoObjG*_Level`, `_Block`, `_Point2I`, the two search records) when a
  counter change is acceptable.
- **`legoobject_port_aliases.h`** retires with the port models in part 3.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox): the 41 drift drops
  above; link-order map check OK. `python3 tools/link_order.py check` OK.
- `make match-progress` regenerated the baseline, the listing digests and
  `binary-progress.json`; `tools/binary_progress_inputs.txt` was rewritten
  with `binary_progress_hash.py --write-manifest`; `pr_match_progress.py
  check` reports them fresh.
- `make unit-data-offsets`: the GAME-023 row above.
- `python3 tools/translation_units.py build`: 0 conflicts.
- `make tools-test` passes, including the lint, with the ledger shrinking.
- `legoobject_unit.c` passes `gcc -std=gnu89
  -Werror=declaration-after-statement -fsyntax-only`.
- `cmake -S . -B build && cmake --build build && ctest --test-dir build
  --output-on-failure`: 37/37 pass, including `c_boundaries`.

## Reproducing

    python3 tools/convert_unit.py GAME-023 --stem legoobject_unit --scores baseline   # in a copy without the legoobject.cpp link-order entry
    make match-function SOURCE=src/game/legoobject_unit.c
    make reccmp-check
    make match-progress
    make unit-data-offsets
