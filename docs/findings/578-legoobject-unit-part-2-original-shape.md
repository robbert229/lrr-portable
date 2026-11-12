# Finding 578 — LegoObject.c (GAME-023) part 2 rebuilt in original shape

Issue #588, part 2 of 3, under [ADR 0005](../adr/0005-original-shape-c-units.md).
It follows [finding 572](572-legoobject-unit-part-1-original-shape.md) (part 1)
in layout and method. This change rebuilds `0043a9e0`–`00445830` (functions
87–172 of the unit map) in [`src/game/legoobject_unit.c`](../../src/game/legoobject_unit.c),
and moves part 1's pulled-forward `0043c490` back into address order.

"Before" is `main` at `31da2ab6` (the *match baseline* and `binary-progress.json`
there). "After" is this change. Scores come from the full matching build
(`make reccmp-check`).

## Result

| | before | after |
| --- | ---: | ---: |
| Range similarity (byte-weighted, 101 functions, 45,218 bytes without `0x90` fill) | 15.25% | **88.39%** |
| Exact functions in the range | 4 | **60 of 101** |
| Mapped functions in the range | 82 | 101 |
| Exact functions in part 1 (`00436c20`–`0043a960`) | 64 of 90 | 69 of 90 |
| Exact functions, full / game-only | 1,240 / 1,040 | **1,301 / 1,101** |
| Accuracy, full / game-only (`binary-progress.json`) | 73.97% / 71.35% | **76.07% / 73.68%** |
| Exact-function code bytes | 128,629 (20.02%) | **139,499 (21.71%)** |
| `opt_ref` annotated removed, full (exact) | 762 (346) | **604 (279)** |
| `opt_ref` annotated removed, game-only (exact) | 743 (333) | 586 (267) |
| `legoobject_unit.c`: annotated / removed by `/OPT:REF` (exact) | 91 / 24 (18) | 191 / 27 (23) |
| `legoobject.cpp`: annotated / removed by `/OPT:REF` (exact) | 165 / 54 (0) | 84 / 9 (0) |
| Reccmp lint ledger | 1,366 | 1,337 |

The unit map lists 86 functions in the range. The real count is **101**:
Ghidra merged fifteen into their neighbours. A scan of the original `.text`
for in-range addresses that code or data references (`push` of a callback)
but the map does not start, and for `ret` followed by `0x90` fill before an
aligned address, found the same fifteen: `0043ac90`, `0043b350`, `0043b3b0`,
`0043b770`, `0043b8d0`, `0043c2f0`, `0043c810`, `0043f130`, `0043f580`,
`00440800`, `00440920`, `00442560`, `00445340`, `00445770` and `00445830`.
Five more had no annotation: `0043eea0`, and `0043f5b0`, `0043fb40`,
`0043fdc0`, `0043fe70`, `0043fff0`, whose port models in `entity.cpp` carry
their markers as comments. All 101 are written and annotated.

`0043c810` is LegoObject_Callback_Update, the 9,864-byte per-object frame
that LegoObject_UpdateAll (`0043b270`) pushes to LegoObject_RunThroughLists;
it is 22% of the range's bytes. `00442540` and `00442560` are not object
callbacks: they are the world-Z callbacks `float cb(float x, float y, Map3D*)`
that `004425f0`–`00442f80` push to the container functions.

Byte weights are the original's code bytes without the `0x90` fill. An
unmapped function counts as 0%.

| Address | Function | Bytes | Before | After |
| --- | --- | ---: | ---: | ---: |
| `0043a9e0` | `legoobject_queue_for_rockfall_processing_if_eligible` | 14 | 42.86 | **100.00** |
| `0043a9f0` | `legoobject_pair_interaction_gate` | 192 | 27.91 | **100.00** |
| `0043aab0` | `legoobject_proximity_activity_raise` | 320 | 37.23 | **100.00** |
| `0043abf0` | `legoobject_terrain_cell_variant_update_queue` | 153 | 23.60 | 98.15 |
| `0043ac90` | `legoobject_terrain_cell_variant_visit_callback` | 188 | — | 98.44 |
| `0043ad50` | `legoobject_predug_spawn_coordinate_select_and_create` | 328 | 15.77 | 98.04 |
| `0043aea0` | `legoobject_emergence_random_spawn_attempt` | 132 | 21.88 | 92.00 |
| `0043af30` | `legoobject_creature_spawn_from_emergence` | 832 | 19.35 | 96.49 |
| `0043b270` | `legoobject_update_all` | 168 | 15.49 | 97.67 |
| `0043b320` | `legoobject_remove_all` | 43 | 45.45 | **100.00** |
| `0043b350` | `legoobject_remove_all_callback` | 14 | — | **100.00** |
| `0043b360` | `legoobject_pick_at_screen_position` | 77 | 10.17 | **100.00** |
| `0043b3b0` | `legoobject_pick_at_screen_position_callback` | 776 | — | 87.29 |
| `0043b6c0` | `legoobject_select_in_screen_rect` | 171 | 19.77 | 77.69 |
| `0043b770` | `legoobject_select_in_screen_rect_callback` | 162 | — | **100.00** |
| `0043b820` | `legoobject_first_person_meshes_swap` | 47 | 43.90 | **100.00** |
| `0043b850` | `legoobject_lod_meshes_update` | 121 | 19.13 | **100.00** |
| `0043b8d0` | `legoobject_lod_meshes_update_callback` | 529 | — | 96.55 |
| `0043baf0` | `legoobject_flags_mask107c33de_or_1c00203_any_test` | 40 | **100.00** | **100.00** |
| `0043bb20` | `legoobject_building_deposit_state_monitor` | 152 | 28.15 | 96.08 |
| `0043bbc0` | `legoobject_detach_carried_object_and_reset_state` | 120 | 63.89 | **100.00** |
| `0043bc40` | `legoobject_objective_sidecar_append_gate` | 1472 | 39.16 | 98.88 |
| `0043c200` | `legoobject_state_pair_predicate` | 125 | 26.36 | 88.37 |
| `0043c280` | `legoobject_nested_float_add_unless_blocked` | 46 | 85.71 | **100.00** |
| `0043c2b0` | `legoobject_periodic_timer25_advance` | 58 | 14.41 | **100.00** |
| `0043c2f0` | `legoobject_periodic_timer25_callback` | 240 | — | 96.86 |
| `0043c3e0` | `legoobject_type26_command_completion_gate` | 82 | 31.33 | 78.79 |
| `0043c440` | `legoobject_can_do_activity_classify` | 69 | **100.00** | **100.00** |
| `0043c490` | `legoobject_task_type_and_condition_check` | 36 | **100.00** | **100.00** |
| `0043c4c0` | `legoobject_finish_state_1a` | 101 | 31.03 | **100.00** |
| `0043c530` | `legoobject_mark_and_finish_state_1a` | 58 | 26.09 | **100.00** |
| `0043c570` | `legoobject_kind_four_post_create_resolve` | 222 | 30.27 | 97.26 |
| `0043c650` | `legoobject_capacity_or_state_predicate` | 86 | 37.33 | **100.00** |
| `0043c6b0` | `legoobject_activity_counters_advance` | 339 | 43.88 | **100.00** |
| `0043c810` | `legoobject_update_callback` | 9864 | — | 88.60 |
| `0043eea0` | `legoobject_simulation_queue_flush` | 598 | — | 89.26 |
| `0043f100` | `legoobject_linked_route_flag_clear` | 40 | 69.23 | **100.00** |
| `0043f130` | `legoobject_trigger_frame_callback` | 28 | — | **100.00** |
| `0043f150` | `legoobject_spawn_request_queue_add` | 58 | 26.09 | **100.00** |
| `0043f190` | `legoobject_spawn_request_queue_process` | 969 | 15.78 | 94.53 |
| `0043f560` | `legoobject_pool_find_with_local_predicate` | 19 | 14.81 | **100.00** |
| `0043f580` | `legoobject_pool_find_local_predicate_callback` | 37 | — | **100.00** |
| `0043f5b0` | `legoobject_linked_activity_event_dispatch` | 232 | — | **100.00** |
| `0043f6a0` | `legoobject_damage_apply` | 304 | 65.22 | 93.33 |
| `0043f7d0` | `legoobject_energy_low_target_route_command_tick` | 872 | 28.47 | **100.00** |
| `0043fb40` | `legoobject_damage_hurt_sound_submit` | 223 | — | **100.00** |
| `0043fc20` | `legoobject_record_pair_pose_copy_and_activity_raise` | 402 | 22.53 | **100.00** |
| `0043fdc0` | `legoobject_action_target_cell_is_valid` | 168 | — | **100.00** |
| `0043fe70` | `legoobject_route_to_explicit_target` | 373 | — | 82.39 |
| `0043fff0` | `legoobject_route_marker_setup` | 320 | — | 85.37 |
| `00440130` | `legoobject_update_and_unregister_at_tile` | 117 | 61.90 | **100.00** |
| `004401b0` | `legoobject_terrain_cell_to_world_position` | 530 | 17.18 | 98.06 |
| `004403d0` | `legoobject_level_debug_cell_retarget_dispatch` | 984 | 8.70 | 86.12 |
| `004407b0` | `legoobject_spatial_callback_query` | 76 | 12.00 | **100.00** |
| `00440800` | `legoobject_spatial_callback_query_visit` | 186 | 64.18 | 98.41 |
| `004408c0` | `legoobject_spatial_callback_query_at_optional_coordinate` | 83 | 21.92 | **100.00** |
| `00440920` | `legoobject_spatial_callback_query_at_optional_coordinate_visit` | 179 | 40.82 | **100.00** |
| `004409e0` | `legoobject_set_activity` | 42 | 12.50 | **100.00** |
| `00440a10` | `legoobject_attached_direction_vector_update` | 468 | 22.71 | 61.95 |
| `00440bf0` | `legoobject_object_list_field_assign_all` | 61 | 53.52 | **100.00** |
| `00440c30` | `legoobject_grid_route_search_adapter` | 53 | 95.24 | **100.00** |
| `00440c70` | `legoobject_terrain_grid_path_search` | 1150 | 16.79 | 51.91 |
| `004410f0` | `legoobject_terrain_neighbor_path_check` | 1538 | 13.49 | 62.94 |
| `00441700` | `legoobject_route_setup_and_marker_associate` | 571 | 23.22 | **100.00** |
| `00441940` | `legoobject_action_sequence_reset` | 491 | 44.52 | **100.00** |
| `00441b30` | `legoobject_state_reset` | 870 | 41.26 | **100.00** |
| `00441ea0` | `legoobject_type5_finalize_if_allowed` | 39 | 37.04 | **100.00** |
| `00441ed0` | `legoobject_face_target_action_start` | 502 | 18.72 | 98.75 |
| `004420d0` | `legoobject_kind_effect_slot_init_dispatch` | 308 | 7.36 | 96.08 |
| `00442210` | `legoobject_action_route_state_initialize` | 66 | 41.67 | **100.00** |
| `00442260` | `legoobject_transform_position_get` | 56 | 23.19 | **100.00** |
| `004422a0` | `legoobject_surface_vector_normalize` | 89 | 38.81 | **100.00** |
| `00442300` | `legoobject_velocity_vector_dispatch` | 377 | 16.16 | **100.00** |
| `00442480` | `legoobject_vector_direction_angle_radians_get` | 109 | 47.06 | 97.14 |
| `004424f0` | `legoobject_position_to_terrain_cell` | 70 | 25.53 | **100.00** |
| `00442540` | `legoobject_world_z_callback` | 24 | 29.63 | **100.00** |
| `00442560` | `legoobject_world_z_callback_lake` | 138 | — | **100.00** |
| `004425f0` | `legoobject_subobject_pose_delta_normalize_and_dispatch` | 677 | 17.42 | 99.52 |
| `004428a0` | `legoobject_world_position_set` | 611 | 13.95 | 98.40 |
| `00442b10` | `legoobject_movement_accumulator_apply` | 1132 | 14.73 | 85.60 |
| `00442f80` | `legoobject_pose_orientation_dispatch` | 1415 | 14.56 | 73.26 |
| `00443510` | `legoobject_scene_object_pose_position_sync` | 346 | 10.13 | **100.00** |
| `00443670` | `legoobject_random_activity_attempt` | 113 | 89.58 | **100.00** |
| `004436f0` | `legoobject_type3_flag04000000_test` | 30 | **100.00** | **100.00** |
| `00443710` | `legoobject_adjacent_position_candidate_find` | 219 | 23.53 | **100.00** |
| `004437f0` | `legoobject_dispatch_event31_for_pending_flag` | 70 | 87.50 | **100.00** |
| `00443840` | `legoobject_route_cell_candidate_accept` | 112 | 20.31 | **100.00** |
| `004438b0` | `legoobject_path_step_advance_and_arrival_check` | 2024 | 26.66 | 99.33 |
| `004440a0` | `legoobject_linked_flag_and_command_update` | 70 | 31.82 | **100.00** |
| `004440f0` | `legoobject_activity_timeout_accumulate_and_raise` | 358 | 11.31 | **100.00** |
| `00444260` | `legoobject_random_cell_step_route_issue` | 509 | 34.37 | **100.00** |
| `00444460` | `legoobject_flagged_vector_step_command_issue` | 434 | 19.15 | **100.00** |
| `00444620` | `legoobject_enter_event21_state_once` | 91 | 87.10 | **100.00** |
| `00444680` | `legoobject_step_movement_vector_resolve` | 2344 | 4.88 | 64.85 |
| `00444fb0` | `legoobject_turn_to_vector_activity_issue` | 556 | 26.04 | 70.66 |
| `004451e0` | `legoobject_path_step_vector_normalize` | 351 | 17.54 | **100.00** |
| `00445340` | `legoobject_path_step_vector_callback` | 599 | — | 74.81 |
| `004455a0` | `legoobject_move_vector_surface_probe` | 308 | 16.54 | 75.83 |
| `004456e0` | `legoobject_behavior_callback_tick` | 133 | 17.65 | **100.00** |
| `00445770` | `legoobject_behavior_callback_a` | 177 | — | 92.68 |
| `00445830` | `legoobject_behavior_callback_b` | 1342 | — | 75.52 |


### What is left

| Address | Score | What is left |
| --- | ---: | --- |
| `00440c70` | 51.91 | size and control flow match; the original keeps `step` in `ebp` and a zero in `esi`, and packs the frame differently. 22 permuter rewrites found nothing |
| `00440a10` | 61.95 | register roles swapped (object in `edi`); the original subtracts the vectors on the x87 stack where ours stores and reloads |
| `004410f0` | 62.94 | same size and control flow; stack slots and registers, and the original keeps a separate copy of the step count for `*outCount` that MSVC removes in every form tried; 300 s of permuter |
| `00444680` | 64.85 | frame 0x80 against 0x74: in the original `facing`'s components sit in the packed-scalar area; as scalars it falls to 40–54%; the `switch` byte is spilled where the original keeps `al` |
| `00444fb0` | 70.66 | x87 scheduling of the normalise/dot/cross code; `acos` is `__CIacos` on our side and unnamed on the original's |
| `00442f80` | 73.26 | the type-2 path keeps `z` in the elapsed slot, and `len > half` is `fcom st1` against `fld st0; fcomp st2` |
| `00445830`, `00445340`, `004455a0` | 73–76 | frame slots (the original reuses the dead `obj` parameter slot for `dist`), x87 scheduling, the loop base `&points[i].y` |
| `0043b6c0` | 77.69 | the y bounds are compared on the x87 stack (`fld; fld; fcompp`); every form tried moves them as integers |
| `0043fe70`, `0043fff0`, `004403d0` | 82–86 | stack-slot layout only (the original keeps the route lists in the dead parameter slots); 400 declaration orders moved nothing |
| `0043c810` | 88.60 | the frame size (0x3c) and control flow match; locals land in other slots (the original's scoping is not recovered), and 14 bytes of x87/test-byte forms: `fld/fstp` against `mov` for `+0x2c8`, `fld; fcomp st(1)`, and `and edx,3; test dl,dl`, which MSVC folds into `test byte ptr` in about fifteen forms |
| `0043b3b0`, `0043eea0` | 86–89 | block order of a skip loop; `sete; cmp ecx,1`, which MSVC folds |
| `0043c200`, `0043f6a0`, `0043af30` | 88–97 | symbol-counter ties (rule B): `0043c200` scored 100% in two of the group trees and flips with the declaration count before it |
| the rest at 91–99.6% | | adapter operands (`0046d380` Vehicle_SetPosition, `00446610`, `004477d0`, `00435aa0`, placement), one `fxch` (`00445770`) |

## How the conversion is laid out

As finding 572:

- The functions are in address order in `legoobject_unit.c`, after part 1,
  in sections H–O (H `0043a9e0`–`0043bbc0`, I `0043bc40`–`0043c6b0` and
  `0043eea0`–`0043f7d0`, J `0043c810`, K `0043fb40`–`00440c70`, L
  `004410f0`–`00442560`, M `004425f0`–`00443840`, N `004438b0`–`00444620`,
  O `00444680`–`00445830`). Prototypes and group record views are in
  `legoobject_unit.h`, callees in `legoobject_unit_imports.h`, both in the
  same group sections.
- The port's implementations stay in `legoobject.cpp`, in place,
  unannotated: each `// FUNCTION` became `/* Port model of 0x… (GAME-023) */`
  and `// PORT-ONLY: port model of 0x…, retires to the tail in GAME-023 part
  3`. 29 helpers that only those models use are `// PORT-ONLY: helper of
  GAME-023 part-2 port models` (the lint ledger shrinks by 29). The legacy
  entry's key is now `00449ac0`, the lower median of its remaining markers.
- **`Lego_Object`** gained the route curve (`routeCurveCount` `+0x030`,
  `routeCurvePoints[50][2]` `+0x034`, `routeCurveDistances[50]` `+0x1c4`;
  the original's BezierCurve, replacing `pad_030`), `weaponDirection`
  (`+0x39c`) and `weaponTarget` (`+0x3a8`); `field_298`/`field_29c` are
  floats. Struct members do not move the symbol counter.
- **Maths.h inline helpers.** The Vector2D/3D helpers the original's Maths.h
  expands in place are `static __inline` functions in the unit's headers
  (`/Ob1` expands them). They reproduce the original's interleaved x87 code
  where macros or explicit statements do not (`00441940`, `004438b0`,
  `00444460`, `0043c810`: 85.04% as macros). Helpers in the `.c` file are
  lint violations, and the move to the headers changed one tie (`0043af30`).
- **State** the port does not model, annotated in the unit:
  `objectGlobs_f_4dee0c` (`004dee0c`, the 25-second timer; the port keeps an
  unannotated `s_periodic_timer25`), `objectGlobs_f_4e8444`,
  `objectGlobs_f_4eadc8` (the 125-second timer), `objectGlobs_f_eadcc` (the
  drill-frame counter), `legoGlobs_f_556090` (the object being updated),
  `legoGlobs_objectRequests` (`00556474`) and the `.data` 1.0f at `004a48b0`.
  `make unit-data-offsets`, GAME-023 row: `.data` gaps-only (105, 0 unknown);
  `.rdata` n/a (one item); `.bss` partial (2, 30 unknown, gap 1 from
  `004eadc0`); commons `objectGlobs_f_4dee0c`, `objectGlobs_f_4e8444`,
  `objectGlobs_recordObjs`, `objectGlobs_recordObjsCount`,
  `objectGlobs_f_4eadc8`, `objectGlobs_f_eadcc`, `legoGlobs_f_556090`,
  `legoGlobs_f_556454`, `legoGlobs_objectRequests`.
- **Strings**: `"SND_Hurt"`, `"Lazer"`, `"RockMonster"`, `"Lavamonster"`,
  `"Icemonster"`, `"BatAttack"`, `"Scorpion"`; pooled literals other units
  annotate (`"Barracks"`) stay unmarked.
- **Callees.** `extern "C"` in place on `ai.hpp`, `entity.hpp`, `spawn.hpp`,
  `gods98/record.hpp`, `gods98/resource.hpp`, `gods98/view.hpp` and
  `gods98/simtick.hpp` (the last two partly in the `#ifdef __cplusplus` form,
  because `sound3d.c` includes them); the `level.hpp`, `weapon.hpp` and
  `gods98/sound.hpp` entries the groups needed already have C linkage on
  `main`. No frozen header is edited and no
  function is pre-declared before one. Original-ABI adapters, unannotated and
  `PORT-ONLY`, are in the group sections at the end of `legoobject.cpp`
  where the port's entry is a namespaced ledger stub or has another
  contract (`0046d380`, `00446610`, `004477d0`, `00459190`, `00409d50`,
  `004093b0`, `00409450`, `00435230`, `0044a4f0`, `0044bf00`, `004474f0`,
  `004475c0`, `00449700`, `0044c1b0`, `0044bfd0`, `0044bc30`, `0044c110`,
  `0044b250`). Part 1's `0043f560` adapter is gone.
- **Two declarations per build.** `004332c0` returns the block's byte 3; the
  port declares a `uint32_t` (`level.hpp`), so the unit calls it through a
  `uint8_t` cast (the original uses only `al`). Part 1's
  `entity_command_unlink_by_kind` names `00403d10` in the matching build with
  a `const void*` target, `aitask.h` with a `struct EntitySlot*`; group I's
  redeclaration takes the matching build's type there and `aitask.h`'s
  natively, so neither build sees a conflict.

## Callers

### Part 1 calls the unit

Part 1 called thirteen part-2 addresses through port names or adapters
(`0043b320`, `004409e0`, `00441940`, `0043f100`, `0043f5b0`, `0043f560`,
`0043c570`, `00442260`, `004424f0` through GAME-013's adapter, `00441700`,
`004410f0`, `0043baf0`, `00441ea0`, `004428a0`). It now calls the unit's
functions, and their imports are removed. Exact again: `00437300`,
`00438460`, `004386b0`, `00439d50`, `00439e70` (below 100% in part 1's own
baseline), and `00437540` goes from 79.58% to 98.94% (#590's `0042dfc0`
switch: `legoobject_remove` calls `lego_unit.c`'s registry clear).

### Matching-build aliases

[`legoobject_port_aliases.h`](../../src/game/legoobject_port_aliases.h) gains
71 entries (82 to 153):

- every plain port name in the range, mapped to the unit's function at the
  same address, except `simtick_scene_pass` and `periodic_timer25_advance`
  (`lego_port.cpp` calls them as C++ with a context) and
  `emergence_random_spawn_attempt` (a namespaced ledger stub with four
  arguments); the namespaced `ledger_stub_*` entries cannot be renamed by a
  macro;
- `entity.cpp`'s `0043fdc0`, `0043fe70`, `0043fff0`;
- in the adapter block (which `lego_port.cpp` skips with
  `LEGORRR_LEGOOBJECT_ALIASES_NO_ADAPTERS`), nine original-ABI adapters whose
  contract is the original's: `simtick_scene_pass_original` (`0043b270`),
  `periodic_timer25_advance_original` (`0043c2b0`),
  `level_debug_cell_retarget_adapter` (`004403d0`),
  `entity_position_to_terrain_cell_adapter` (`004424f0`),
  `aitask_grid_route_search_adapter` (`00440c30`),
  `aitask_cell_step_command_issue_adapter` (`0043a120`),
  `lego_object_pair_requirement_met` (`0043c200`), `lego_object_drop_carried`
  (`0043fc20`), `lego_object_go_to_block` (`004401b0`).

The interface was not changed: macros only, under `_MSC_VER < 1200`.
`lego_unit.c`, `lego_port.cpp`, `interface_unit.c`, `aitask.c`,
`terrain_cell_objects.c` and the other includers see the new names. Two
files include it for the first time: `tutorial_script.cpp` (keeps
`00454890` exact) and `entity_stats.c` (keeps `0046a330` exact; first, before
`entity_stats_link_names.h`; no name collides). No macro collides with
`lego_port_aliases.h`, `weapons_port_aliases.h` or `aitask_port_aliases.h`.

`weapons.c` already includes `legoobject_unit.h` and calls part 1 by the
unit's names through `WEAPONS_LOBJ()`. Its five calls into the range
(`0043b360`, `0043c440`, `0043f6a0`, `00442260`, `004436f0`) now do the same;
its declarations of the port names stay, so its symbol count is unchanged.
This keeps `0046f270`, `0046f380`, `0046f770` and `00470630` exact.

### `opt_ref`

`lego_engine_tick` (`004232e0`) now reaches LegoObject_UpdateAll and its
callback, and AITask.c's and Interface.c's live callers reach the unit
through the adapter aliases. `/OPT:REF` removes **604 annotated functions
(279 exact)**, against 762 (346) on `main`. The unit's own count barely moves
(24 to 27 removed, while it grows from 91 to 191 functions);
`legoobject.cpp`'s falls from 54 to 9.

Gaps (`make opt-ref-callers`), left as they are because the contracts differ:

- **`0046cf90`** (Vehicle_SetUpgradeLevel(vehicle, level)): `legoobject_create`
  and `0043bc40` call `vehicle_model.cpp`'s no-op adapter; the annotated port
  entry takes `(list, channels, value)`. It needs GAME-048's original-ABI
  function (#590's follow-up; not done here).
- **`0043a5f0`**: the original returns the container; part 1's prototype is
  `void` and is exact, so `aitask_object_container_get_adapter` and
  `lego_object_get_container` are not aliased.
- **`00443710`**: `aitask_adjacent_position_find_adapter` takes a fourth
  argument.
- **`00438960`**: `spawn_registry_probe_adapter`'s arguments are not
  verified against the original's.
- `placement.cpp`, `entity_cell_effects.cpp` and `legoobject.cpp`'s part 3
  call namespaced ledger stubs or port entries with other contracts (as
  findings 570 and 572 recorded); they follow their own conversions.

## Entropy and drops

`make reccmp-check` against `main` (`31da2ab6`): 99 improved, 19 newly mapped,
**21 below baseline, all `drift; instructions +0`**, and no exact match lost.
For each, the rebuilt listing's normalised digest (`match-listing-digests.txt`,
registers, names and addresses masked) equals `main`'s: our code is unchanged.
They are 19 part-3 functions in `legoobject.cpp`, plus `00409f20`
(`entity_cell_effects.cpp`) and `004526e0` (`message_unit.cpp`), that still call
port models at addresses that now name the unit's functions (as in finding
572). None is owner-reviewed. Under ADR 0005's entropy rule 2 the baseline is
refreshed.

On `05cd8938`, before #604, twelve more functions dropped on data operands,
among them two byte-identical exact ones (`00402510`, `004667e0`): finding
580's Cause A, where the original's `0x800000` immediate fell inside the port's
`.bss` array `s_entity_path_motion_runtime`. #604 (finding 584) removed them;
on `31da2ab6` none remains.

## How the work was split

The scaffold (`ce3240a9`) fixed the prototypes and group sections first. The
eight groups ran as Claude Opus children in their own worktrees with
`match_function.py` only (no full matching build there), three at a time
after a first launch of eight hit the API rate limit. Each was merged into the
main worktree as it finished. Merging made three kinds of fix: two groups
defined Maths.h helpers under the same names (renamed `legoobject_m_*`,
`legoobject_n_*`), two named the route curve differently (one member set
kept), and casts written against the scaffold's prototypes were dropped once
the callee's group had settled its signature. A counter sweep (0–33 inert
declarations) in group O moved none of its functions, so its gaps are source
shape; the group-I and group-H trees each had `0043c200` at 100%, which the
merged tree loses to rule B.

## Shape notes

- MSVC 5 rewrites `x*K + y` as `y - x*(-K)` (`fsubr`), even for `3.0f`;
  `(float)(x*K) + y` keeps the original's `fadd` (`0043ad50`, `0043af30`; the
  same miss is open in GAME-020's `0041c4f0`).
- MSVC 5 moves a constant multiplier to the end of a product:
  `elapsed * (x * 0.04f)` keeps the original's order.
- `int[2]` against `Point2I`, and scalars against aggregates, move whole
  frames: aggregates go after scalars (`0043fe70`, `004403d0`, `00444460`,
  `00445830`).
- Comparisons written in the original's operand order raised several
  functions (`legoobject_zero != rate`, `004440f0`).
- Switch cases in the binary's code order, not numeric order (`004438b0`).
- No naked asm was added.

## Follow-ups

- Vehicle_SetUpgradeLevel (`0046cf90`) with the original's contract (GAME-048),
  then `legoobject_create` and `0043bc40` call it.
- `0043a5f0`'s prototype returns the container once a form keeps it exact;
  then alias the two container adapters.
- One `Lego_Level`/`Lego_Block` and one Vector view for the groups' small record
  views (`LegoObj{H,J,K,L,M,O}_*`), when a counter change is acceptable.
- Part 3 (`00445d70`–`0044c550`) retires `legoobject.cpp` and the alias
  header's part-2 entries with it.

## Validation

On `31da2ab6`:

- `make reccmp-check`: the 21 drops above (accepted, not owner-reviewed); link
  order map check OK; `make winapi-link-check` OK.
- `make match-progress` regenerated the baseline, the listing digests and
  `binary-progress.json`; `tools/binary_progress_inputs.txt` rewritten with
  `binary_progress_hash.py --write-manifest`; `pr_match_progress.py check`
  reports them fresh.
- `make unit-data-offsets`: the GAME-023 row above;
  `python3 tools/translation_units.py build`: 0 conflicts.
- `make tools-test` (no `--allow-growth`): passes; the lint ledger goes from
  1,366 to 1,337.
- Native: `cmake -S . -B build && cmake --build build && ctest --test-dir
  build --output-on-failure`: 37/37 pass, including `c_boundaries`.
- `legoobject_unit.c` passes `gcc -std=gnu89 -Werror=declaration-after-statement
  -fsyntax-only`.

## Reproducing

    make match-function SOURCE=src/game/legoobject_unit.c
    make reccmp-check
    make match-progress
    make opt-ref-link opt-ref
    python3 tools/opt_ref_link.py callers --build build-reccmp --object legoobject_unit.c.obj
    make unit-data-offsets
