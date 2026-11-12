# Finding 568 — Lego.c (GAME-021) part 1 rebuilt in original shape

Issue #544, part 1 of 3, under [ADR 0005](../adr/0005-original-shape-c-units.md).
GAME-021 is the original's `Lego.c`: start-up, the configuration loader, the
main loop, level loading and the per-frame world update (text
`0041f870`–`00435b20`, 216 functions). It was relocated verbatim into
`src/game/lego.cpp`. This change rebuilds its first range,
`0041f870`–`0042b500`, as the C unit
[`src/game/lego_unit.c`](../../src/game/lego_unit.c). Parts 2 and 3 convert
`0042b510`–`00435b20`.

The range is 72 functions in the unit map. The real count is **78**,
because Ghidra merged six functions into their neighbours:

- `004232e0` (Lego_MainLoop) into `004231f0`;
- `00424cf0` and `00424d00` (the two shutdowns) into `004248b0`;
- `00425b60` into `00425b40`;
- `00429110` into `004288e0`;
- `004297a0` into `004295f0`.

All six are written and annotated.

"Before" is the *match baseline* on `main` at `b141a808` (#566). "After" is
this change. Scores come from the full matching build (`make reccmp-check`).

## Result

| | before | after |
| --- | ---: | ---: |
| Range similarity (byte-weighted, 78 functions, 47,745 bytes without `0x90` fill) | 8.98% | **89.27%** |
| Exact functions in the range | 22 | **45 of 78** |
| Mapped functions in the range | 63 | 78 |
| Exact functions, whole binary | 951 | 975 |
| Accuracy, full / game-only (`binary-progress.json`) | 63.83% / 60.05% | **65.13% / 61.51%** |
| Exact-function code bytes | 80,442 | 82,289 |
| Unit-relative identity | 68,699 bytes (12.40%) | 76,185 bytes (13.75%) |
| Unmapped original functions | 181 | 171 |

Byte weights are the original's code bytes without the `0x90` fill. An
unmapped function counts as 0%.

| Address | Function | Before | After |
| --- | --- | ---: | ---: |
| `0041f870` | `global_object_field8c_increment` | **100.00** | **100.00** |
| `0041f880` | `global_object_field8c_subtract` | **100.00** | **100.00** |
| `0041f8a0` | `global_object_field98_add` | **100.00** | **100.00** |
| `0041f8c0` | `global_object_field98_clear_current` | **100.00** | **100.00** |
| `0041f8e0` | `noop_callback` | 0.00 | 0.00 |
| `0041f900` | `lego_upgrade_resource_check` | 20.00 | **100.00** |
| `0041f920` | `level_state_field9c_add` | **100.00** | **100.00** |
| `0041f940` | `lego_flags1_bit17_set` | — | **100.00** |
| `0041f970` | `lego_runtime_flag40000000_get` | 25.00 | **100.00** |
| `0041f980` | `global_object_field94_increment` | **100.00** | **100.00** |
| `0041f990` | `global_object_fielda8_increment` | **100.00** | **100.00** |
| `0041f9a0` | `lego_level_resource_balance_add_and_sound` | 40.00 | **100.00** |
| `0041f9e0` | `lego_upgrade_resource_debit` | 50.00 | **100.00** |
| `0041fa20` | `lego_game_config_callback_registry_init` | 38.60 | **100.00** |
| `0041fa80` | `lego_start_level_pilots` | 54.41 | 86.27 |
| `0041fb40` | `EngineTimeScale(1)` | **100.00** | **100.00** |
| `0041fb50` | `lego_game_config_init` | — | 93.80 |
| `00422850` | `lego_misc_object_resources_load` | 11.29 | 89.52 |
| `00423080` | `lego_mode_dependent_field24_get` | 13.33 | **100.00** |
| `004230b0` | `global_float_field_00556f00_get` | **100.00** | **100.00** |
| `004230c0` | `lego_debug_panel_text_draw_update` | 16.59 | 93.68 |
| `004231f0` | `lego_engine_pre_tick` | 17.14 | **100.00** |
| `004232e0` | `lego_engine_tick` | 10.16 | 92.01 |
| `00424560` | `lego_air_supply_amount_update` | 30.40 | 97.62 |
| `00424600` | `lego_terrain_cell_flag_cleanup` | — | 98.90 |
| `00424730` | `lego_sine_phase_midpoint_blend_dispatch` | 33.33 | 90.91 |
| `004247d0` | `lego_default_transform_apply_if_flagged` | **100.00** | **100.00** |
| `00424810` | `lego_default_transform_apply_all` | **100.00** | **100.00** |
| `00424830` | `lego_selected_object_outlines_draw` | 12.50 | **100.00** |
| `004248b0` | `lego_selected_entity_ground_marker_render` | 7.61 | 98.73 |
| `00424cf0` | `lego_engine_shutdown_alt` | — | **100.00** |
| `00424d00` | `lego_engine_shutdown` | — | 89.71 |
| `004250a0` | `lego_exit` | — | **100.00** |
| `004250c0` | `lego_main_sim_step` | 9.72 | 88.55 |
| `00425b40` | `lego_periodic_callback_dispatch` | 66.67 | **100.00** |
| `00425b60` | `lego_object_sound_restart_callback` | — | **100.00** |
| `00425c30` | `lego_transition_control` | 40.86 | **100.00** |
| `00425cd0` | `global_scroll_guard_set` | **100.00** | **100.00** |
| `00425ce0` | `lego_game_speed_set` | 43.48 | 92.59 |
| `00425d50` | `lego_render_record_set_from_input` | — | 93.33 |
| `00425d80` | `current_engine_mode_is_zero` | **100.00** | **100.00** |
| `00425d90` | `lego_terrain_pointer_command_update` | 13.33 | 86.91 |
| `004261c0` | `lego_spawn_coordinate_pool_tick` | 37.29 | 92.31 |
| `00426230` | `lego_global_triple_copy_out` | 84.21 | **100.00** |
| `00426250` | `lego_update_render_viewport_state` | — | 94.74 |
| `004262e0` | `objective_score_text_origin_set` | **100.00** | **100.00** |
| `00426320` | `lego_objective_task_text_origin_set` | 31.82 | **100.00** |
| `00426360` | `engine_global_state_flags_bits6_and_9_set_target` | **100.00** | **100.00** |
| `004263a0` | `engine_global_state_flags_bit7_set_target` | **100.00** | **100.00** |
| `004263c0` | `lego_global_subsystem_state_reset` | 34.15 | **100.00** |
| `00426420` | `lego_cursor_ray_to_camera_route` | — | 92.96 |
| `00426520` | `lego_main_sim_interaction_and_cell_update` | 9.62 | 69.79 |
| `00427e00` | `lego_tooltip_info_strings_load` | 15.38 | **100.00** |
| `00427f80` | `lego_level_upgrade_name_strings_load` | 17.17 | 94.74 |
| `00428020` | `lego_entity_command_status_feedback_dispatch` | 15.43 | 97.79 |
| `00428330` | `lego_terrain_command_feedback_dispatch` | 9.60 | 71.24 |
| `00428780` | `lego_terrain_cell_command_predicate` | 29.27 | 90.24 |
| `00428800` | `media_action_feedback_sound_dispatch` | **100.00** | **100.00** |
| `004288e0` | `lego_level_debug_input_cell_mutator_dispatch` | — | 96.90 |
| `00429110` | `lego_block_visible_callback` | — | 49.06 |
| `00429160` | `lego_terrain_cell_palette_resource_at_grid_position` | **100.00** | 82.35 |
| `004291a0` | `lego_scene_marker_update_and_draw` | 8.82 | 96.20 |
| `004293b0` | `lego_selection_rectangle_render` | — | **100.00** |
| `00429470` | `lego_camera_pointer_ray_route` | — | 98.80 |
| `004295a0` | `lego_context_selected_global_pointer_get` | 33.33 | **100.00** |
| `004295c0` | `lego_entity_matches_current_selection_or_owner` | 14.55 | **100.00** |
| `004295f0` | `lego_sound_minimum_distance_update_mode` | 9.52 | 96.75 |
| `004297a0` | `lego_cd_track_play_next_callback` | — | **100.00** |
| `004297b0` | `lego_cd_audio_watchdog_mode_update` | 26.67 | **100.00** |
| `00429810` | `lego_audio_playback_toggle` | 28.57 | **100.00** |
| `00429850` | `active_engine_field_b0_get_current` | **100.00** | **100.00** |
| `00429860` | `engine_float_field210_get_by_index_current` | **100.00** | **100.00** |
| `00429880` | `engine_float_field260_get_current_target` | **100.00** | **100.00** |
| `00429890` | `lego_level_load` | 7.92 | 90.45 |
| `0042b2f0` | `lego_entity_effect_counters_apply_from_record` | 30.30 | 91.30 |
| `0042b330` | `lego_load_tile_meshes` | 18.07 | 98.17 |
| `0042b480` | `lego_rectangular_resource_table_destroy` | 17.20 | 97.73 |
| `0042b500` | `global_state_field84_get_current_target` | **100.00** | **100.00** |

## How the conversion is split, for parts 2 and 3

`tools/convert_unit.py` refuses a relocated unit (findings 544–556). It ran in
a scratch export with the `game/lego.cpp` entry removed from the link-order
list. It scaffolds the whole unit, so its output was cut down to this range:

- **One `.c` file for the whole unit.** `lego_unit.c` is the unit file
  (`unit GAME-021`). Parts 2 and 3 append their functions to it in address
  order. The stem is `lego_unit` because `lego.h` already exists (it is the
  boundary header of the port's Lego.c entries that other C units call).
- **`lego.cpp` stays a `legacy` entry** (key `004324d0`, the lower median of
  its remaining markers) linked right after the unit. Part 3 retires it with
  `tools/link_order.py`'s `retire_legacy` once it holds no original
  functions.
- **The port's implementations stay in `lego.cpp`, unannotated.** Their
  `// FUNCTION` lines became `/* Port model of 0x… (GAME-021) */` comments.
  Finding 556 moved the port code into a new tail file instead. Doing that
  here would have removed about 2,000 lines of declarations before every
  part-2/3 function still in `lego.cpp`, re-rolling rule B (finding 538) on
  its 50-odd exact functions. Keeping the code in place changes nothing
  before them: no function in `lego.cpp` changed score. `lego.cpp` becomes
  Lego.c's port tail when part 3 drains it (then rename it, as #537 renamed
  `terrain_map.cpp` to `terrain_map_port.cpp`).
- **Shared functions.** Where the port function's contract equals the
  original's, the unit function keeps the port's name and the port
  definition is deleted from `lego.cpp`. Each deletion removes declarations
  before the later functions in `lego.cpp`; the gate shows no drift from it.

### Parallel work

The 78 functions were split into eight groups by size. Group M is the 31
accessors, done directly; groups A–G each went to an agent in its own
worktree. Each group owned:

- its function stubs;
- a state section in `lego_unit.c`;
- a section of `lego_unit.h`, `lego_unit_imports.h` and `lego_maths.h`.

Each group also added its own `extern "C"` edits and adapters. The four
unit files were assembled by section from each group's version; the other
files were cherry-picked. Overlaps were then reconciled:

- stand-ins defined twice for one address, by name and type;
- three adapters for `00416d00`, two each for `00435ebb` and `00456240`;
- stub signatures that one group had assumed differently from another.

A type chosen during deduplication can change codegen. `0x004297b0` fell
from 100% to 80.85% until `legoGlobs.CDTracks` was unsigned again, because
the original divides with `div`. **For parts 2 and 3:** deduplicate by
address and check each surviving type against the group that measured it.

## The unit

### State: legoGlobs as stand-ins

The original keeps its state in one `Lego_Globs legoGlobs`
(`005560c0`–`00556fb8`, 0x3be dwords) and in Lego.c's `.bss`
(`004de208`–`004de58c`). The port has no such struct. About 25 of its fields
are port globals annotated in other files, including `entity.cpp` and
`view.cpp`, which this change may not edit. A single `legoGlobs` object would
overlap those annotations, and Reccmp names an operand after the entity at
or below its address, so field reads would stop matching. So:

- **Fields the port models** are read through the port's globals, with C
  linkage (below). For example, `g_active_level_retail` is `currLevel`,
  `s_context_mode` is `viewMode` and `s_panel_highlight_flags` is `flags2`.
- **Fields the port does not model** are stand-ins defined in
  `lego_unit.c`: one annotated global per field (or array), named
  `legoGlobs_<field>`. With Lego.c's own data, the unit defines 161
  annotated globals, including `legoGlobs_flags1`
  (`00556ecc`, which the port keeps in `EngineContext`).
- **Lego.c's `.bss`** used only by part 1 is defined in the unit as
  `static`s (`s_lego_*`). Items parts 2 and 3 use (`004de424`, `004de548`,
  `004de588`, `004de58c`) stay in `lego.cpp`, with `extern "C"` where the
  unit reads them.
- **Key maps** are `input.cpp`'s `s_global_current_keys` (`0076ab80`) and
  `s_global_previous_keys` (`0076aa00`), from #559.
- **Zeroing loop.** `Lego_Gods_Go`'s zeroing of the whole struct is a
  `memset` from the stand-in at `005560c0` (`legoGlobs_config`), which
  gives the original's `rep stosd`.
- **Natively these stand-ins are not the live state.** The unit's functions
  that read them run only in the matching build.

`Lego_Level` (0x284) and `Lego_Block` (0x48) are typed in `lego_unit.h`. The
port's `LevelState` starts with a 32-bit word image of the original layout
on every target, so `legoGlobs_currLevel` is a cast of
`g_active_level_retail`. On 64-bit native builds, pointer fields are image
words (`LEGO_LEVEL_PTR_FIELD`), and code dereferences them through
`LEGO_LEVEL_PTR`. That keeps the shared accessors' offsets right natively.
Other original records (LegoObject, LegoCamera, …) are read by offset with
`LEGO_FIELD`. `Lego_Level`/`Lego_Block` carry the original names because
GAME-020's `interface_callees.h` (#559) already defines `LegoLevel`/
`LegoBlock` views and `test_c_boundaries.cpp` includes both.

**Three fields are read at offsets from `legoGlobs_config`:** the object-name
tables (`00556254`–`00556264`, `0055618c`) and the catalogue counts
(`0055627c`). `entity.cpp` defines them with C++ linkage, and this change
may not edit it.

- An `extern "C"` declaration appended to `entity.hpp` re-rolled rule B in
  every unit including it; it cost `0x0040e9c0` its exact match (finding
  538's case).
- LINK 5.0 rejects `/alternatename` (`LNK4044`).
- Reading them by offset gives the same instructions; only Reccmp's operand
  names differ, in `0x00424d00`.

### Shared functions (port name, port definition deleted)

Twenty functions keep the port's name, because their callers already use
the original's contract and their native behaviour is unchanged:

- `00429850`, `00429860`, `00429880`, `0042b500`: the level-field getters;
- the level counters `0041f870`–`0041f990`;
- `0041f8e0`, `0041fb40`, `004230b0`, `00425cd0`, `00425d80`, `004262e0`,
  `00426360`, `004263a0` and `00428800`.

`current_engine_mode_is_zero` now returns `int` (it returned `bool`). The
exact caller `0x00439990` (legoobject.cpp) of `0041f8c0` keeps its match.

Not shared:

- `global_triple_copy_out` (`00426230`): `enginebind`'s test pins the
  port's NULL guard.
- `scroll_scale_set_and_clamp` (`00425ce0`): the original also stores the
  options slider.
- `0x00429160`: it dereferences level pointers.

### Callers that keep the port's functions (follow-up)

These port functions have another contract: an `EngineContext` or
`GameContext` parameter, hook tables, the port's level model, NULL guards
pinned by tests, or the port's own state for `flags1`. Their native callers
keep calling them in `lego.cpp`. A caller can switch once its unit is
converted and the native port reads the original state.

| Address | Port function (stays in lego.cpp) | Unit function |
| --- | --- | --- |
| `0041f900` | `upgrade_resource_check` | `lego_upgrade_resource_check` |
| `0041f970` | `runtime_flag40000000_get` | `lego_runtime_flag40000000_get` |
| `0041f9a0` | `level_resource_balance_add_and_sound` | `lego_level_resource_balance_add_and_sound` |
| `0041f9e0` | `upgrade_resource_debit` | `lego_upgrade_resource_debit` |
| `0041fa20` | `engine_game_config_callback_registry_init` | `lego_game_config_callback_registry_init` |
| `0041fa80` | `ledger_stub_0041fa80_StartLevelPilots` | `lego_start_level_pilots` |
| `00422850` | `misc_object_resources_load` | `lego_misc_object_resources_load` |
| `00423080` | `mode_dependent_field24_get` | `lego_mode_dependent_field24_get` |
| `004230c0` | `debug_panel_text_draw_update` | `lego_debug_panel_text_draw_update` |
| `004231f0` | `EnginePreTick` | `lego_engine_pre_tick` |
| `004232e0` | `EngineTick` | `lego_engine_tick` |
| `00424560` | `air_supply_amount_update` | `lego_air_supply_amount_update` |
| `00424730` | `sine_phase_midpoint_blend_dispatch` | `lego_sine_phase_midpoint_blend_dispatch` |
| `004247d0` | `entity_default_transform_apply_if_flagged` | `lego_default_transform_apply_if_flagged` |
| `00424810` | `entity_default_transform_apply_all` | `lego_default_transform_apply_all` |
| `00424830` | `selected_object_outlines_draw` | `lego_selected_object_outlines_draw` |
| `004248b0` | `selected_entity_ground_marker_render` | `lego_selected_entity_ground_marker_render` |
| `004250c0` | `main_sim_step` | `lego_main_sim_step` |
| `00425b40` | `periodic_entity_callback_dispatch` | `lego_periodic_callback_dispatch` |
| `00425c30` | `ledger_stub_00425c30_TransitionControl` | `lego_transition_control` |
| `00425ce0` | `scroll_scale_set_and_clamp` | `lego_game_speed_set` |
| `00425d90` | `simtick_mouse_world` | `lego_terrain_pointer_command_update` |
| `004261c0` | `camera_spawn_countdown` | `lego_spawn_coordinate_pool_tick` |
| `00426230` | `global_triple_copy_out` | `lego_global_triple_copy_out` |
| `00426320` | `objective_task_text_origin_set` | `lego_objective_task_text_origin_set` |
| `004263c0` | `ledger_stub_004263c0_GlobalSubsystemStateReset` | `lego_global_subsystem_state_reset` |
| `00426520` | `simtick_vu_meter` | `lego_main_sim_interaction_and_cell_update` |
| `00427e00` | `tooltip_info_strings_load` | `lego_tooltip_info_strings_load` |
| `00427f80` | `level_upgrade_name_strings_load` | `lego_level_upgrade_name_strings_load` |
| `00428020` | `entity_command_status_feedback_dispatch` | `lego_entity_command_status_feedback_dispatch` |
| `00428330` | `ledger_stub_00428330_TerrainCommandFeedbackDispatch` | `lego_terrain_command_feedback_dispatch` |
| `00428780` | `terrain_cell_command_predicate` | `lego_terrain_cell_command_predicate` |
| `00429160` | `terrain_cell_palette_resource_at_grid_position_current` | `lego_terrain_cell_palette_resource_at_grid_position` |
| `004291a0` | `simtick_render_pass` | `lego_scene_marker_update_and_draw` |
| `004295a0` | `context_selected_global_pointer_get` | `lego_context_selected_global_pointer_get` |
| `004295c0` | `entity_matches_current_selection_or_owner` | `lego_entity_matches_current_selection_or_owner` |
| `004295f0` | `media_min_distance_update_mode` | `lego_sound_minimum_distance_update_mode` |
| `004297b0` | `media_cd_watchdog_update` | `lego_cd_audio_watchdog_mode_update` |
| `00429810` | `media_audio_playback_toggle` | `lego_audio_playback_toggle` |
| `00429890` | `level_load` | `lego_level_load` |
| `0042b2f0` | `entity_effect_counters_apply_from_record` | `lego_entity_effect_counters_apply_from_record` |
| `0042b330` | `terrain_tile_mesh_grid_load_level` | `lego_load_tile_meshes` |
| `0042b480` | `rectangular_resource_table_destroy` | `lego_rectangular_resource_table_destroy` |

`ledger_stub_00425c30_TransitionControl` and `media_action_feedback_sound_dispatch`
are also declared in `lego.h` for `panel_unit.c`. The latter is shared. For
the former, `panel_unit.c` still calls the port model, because the port
keeps `flags1` in the `EngineContext`.

### Callees

- **`extern "C"` in place** on roughly 400 existing declarations of callees and
  globals across the C++ headers. Headers that C units also include
  (`gods98/view.hpp`, `sound.hpp`, `record.hpp`, `resource.hpp`,
  `engine.hpp`, `heap.hpp`, `media.hpp`, `input.hpp`, `simtick.hpp`) use
  `#ifdef __cplusplus` / `extern "C"` / `#endif` before the declaration, so
  `priorities.c` still compiles.
- **Frozen headers** (`*_for_*`, `*_from_*`) are not edited. Where a callee
  or global is declared only there, an `extern "C"` pre-declaration sits
  directly before the frozen `#include` in each file that includes it:
  - in `lego.cpp`, `engine.cpp`, `crt.cpp`, `cmd.cpp`, `level.cpp` and
    `input.cpp`;
  - in `input_unit.cpp`, `text.cpp`, `tutorial_script.cpp`, `tutorial_hud.cpp`
    and `object_overlays.cpp`;
  - in `grid_cell_records.cpp`, `file_list_streams.cpp`, `spawn.cpp`,
    `tooltip.cpp`, `frontend.cpp` and `frontend_unit.cpp`.

  The pre-declaration is the first declaration and the frozen one becomes a
  redeclaration, so the declaration count does not change. Unnamed
  parameters keep the parameter count the same too.
- **Unit-private imports header.** C declarations go in
  [`lego_unit_imports.h`](../../src/game/lego_unit_imports.h), not the
  owners' C headers, so the other C units' symbol numbering does not change.
  `test_c_boundaries.cpp` includes it next to the C++ headers.
- **Calls into converted units**:
  - `reward.c` (#547): `reward_initialise`, `reward_load_graphics`,
    `reward_level_start`, `reward_shutdown`;
  - `lws.c` (#546): `lws_initialise`, `lws_shutdown`;
  - `interface_unit.c` (#559): nine `interface_*` entries.

  These are direct calls, so their operands match.
- **Detail_GetTexture** (`0040b850`) now takes the texture as a byte, as the
  original does. The declaration changed in `record.hpp`, `tile_variants.h`
  and the definition. The GAME-011 callee keeps its score (18.52%). The
  GAME-026 callers rise: `0x0044e6d0` from 93.88% to 97.52% and
  `0x0044f230` from 93.40% to 99.03% (finding 556 had noted the
  zero-extension).
- **Adapters.** About 120 thin unannotated original-ABI adapters, each marked
  `// PORT-ONLY:` (#558):
  - at the end of `lego.cpp`, in one block per group;
  - at the end of the callee's module, in `aitask.cpp`, `camera_control.cpp`,
    `tutorial_script.cpp`, `frontend_unit.cpp`, `objective_unit.cpp`,
    `options.cpp`, `rock_fall_styles.cpp`, `map_view.cpp`, `vehicle_model.cpp`,
    `grid_cell_records.cpp`, `tile_variants.cpp`, `object_state_file.cpp` and
    `level_id_remap.cpp`;
  - and in `reward_port.cpp`, `interface_port.cpp`, `info.cpp`,
    `bitmap_font.cpp`, `sfx.cpp`, `directdraw.cpp`, `viewports.cpp`,
    `message_unit.cpp`, `info_track_overlay.cpp`, `helpwindow.cpp` and
    `loading_progress.cpp`.

  Where no port function has the original's contract, the body is minimal
  and says so; natively nothing calls it. The call to an adapter is the
  usual remaining operand difference.

### Strings

Lego.c's literals carry a `// STRING` marker on the line directly above
them, one literal per line. Pooled literals get no marker, for example
`"Main"`, `"%s"`, `"%i"`, `"Ore"`, `"ON"`/`"OFF"`, `"Level"`, `""` (`004b8a14`)
and `"Pilot"`.

### Shape notes

- **Functions built without optimisation.** `0x0041fb50`, `0x00422850` and
  `0x004247d0`–`0x004248b0` were compiled without optimisation in the
  original. They are wrapped in `#pragma optimize("", off/on)`
  (`_MSC_VER` only).
- **Unoptimised frames are laid out by local-variable name, not declaration
  order.** Group A fitted the rule: hash
  `v = (v<<2) + (v>>4) + c` over the name's characters, bucket
  `(v ^ (v>>16)) & 15`; buckets go from 0 to 15, and within a bucket the
  later-declared local comes first. Locals in those functions are named to
  fit; do not rename them.
- **Key presses** match only as `key[k] && (prev[k] ^ key[k])`; `!=` gives
  `cmp`. `s_global_current_keys` must be declared before
  `s_global_previous_keys` (rule A).
- **No final return.** `0x0041f900` and `0x00423080` have none: the original
  falls off the end with the null level or the view mode in `eax`.
- **Uninitialised locals.** `0x004232e0` passes two locals it never sets to
  the crystal panel when there is no level; the original reads the same
  uninitialised stack slots. MSVC's `C4700` for those two names is allowed
  in `tools/reccmp_diagnostics.py`.
- **Unused parameters.** `0x004293b0` takes the level and does not use it;
  `0x00427e00` takes the game name and does not use it. Their callers push
  both.

### Near misses

| Address | Score | What is left |
| --- | ---: | --- |
| `00426520` | 69.79 | stack-slot reuse of the dead parameter slots, register choice, three store orders |
| `00428330` | 71.24 | MSVC puts the early-return block last; the frame is one dword smaller |
| `00429110` | 49.06 | register choice (edx/esi/edi) |
| `00429160` | 82.35 | see below |
| `004297b0`–`0042b480`, `004232e0`, `0041fb50` and the rest at 86–99% | | adapter call operands, pooled literals, the 0.0f that `lego.cpp` annotates as `s_panel_highlight_zero` (`0049e2b8`), rule-B register choices |

## Entropy and drops

`make reccmp-check` against `b141a808`: 46 improved, 15 newly mapped, 14 below
baseline. No instruction count changed anywhere.

| Address | Function | File | Score | Label | Instructions |
| --- | --- | --- | --- | --- | --- |
| `0x00404280` | `cmd_entity_command_chain_tick` | `game/aitask.cpp` | 10.71 → 10.46 | drift | +0 |
| `0x00415220` | `frontend_start_level` | `game/frontend_unit.cpp` | 16.67 → 13.33 | drift | +0 |
| `0x00429160` | `lego_terrain_cell_palette_resource_at_grid_position` | `game/lego_unit.c` | 100.00 → 82.35 | source-changed | +0 |
| `0x0043a650` | `entity_action_kind_entity_create_and_command_issue` | `game/legoobject.cpp` | 26.62 → 25.90 | drift | +0 |
| `0x0043a850` | `entity_action_record_consume_and_cleanup` | `game/legoobject.cpp` | 30.37 → 26.18 | drift | +0 |
| `0x0043a910` | `simulation_mode_counter_add` | `game/legoobject.cpp` | 43.75 → 37.50 | drift | +0 |
| `0x0043b270` | `simtick_scene_pass` | `game/legoobject.cpp` | 21.13 → 19.72 | drift | +0 |
| `0x00444460` | `ledger_stub_00444460_FlaggedVectorStepCommandIssue` | `game/legoobject.cpp` | 22.72 → 22.27 | drift | +0 |
| `0x004470d0` | `lego::entity_conditional_type_five_spawn` | `game/legoobject.cpp` | 72.19 → 71.01 | drift-review | +0 |
| `0x004520d0` | `cmd_queue_tick` | `game/message_unit.cpp` | 21.98 → 21.67 | drift | +0 |
| `0x004524b0` | `selection_object_detach` | `game/message_unit.cpp` | 72.22 → 66.67 | drift | +0 |
| `0x00452d60` | `selection_command_dispatch` | `game/message_unit.cpp` | 29.20 → 27.74 | drift | +0 |
| `0x004538a0` | `camera_focus_scroll_scale_commit` | `game/tutorial_script.cpp` | 42.25 → 39.44 | drift | +0 |
| `0x00458670` | `objective_announce_current` | `game/objective_unit.cpp` | 27.95 → 27.40 | drift | +0 |

- **One exact match is lost: `0x00429160`, for the owner.** Its 100%
  baseline came from a `__declspec(naked)` asm block in the port. The C
  version is 82.35%: one `imul` takes its operands in the other order, and
  the global and parameter loads are scheduled the other way. Tried:
  - index operand orders;
  - signedness casts;
  - a local level;
  - a `Point2I` parameter declared before and after `Lego_Level`;
  - the permuter (no candidates);
  - a counter sweep over a full 512 period (one code at every N).

  None moved it, so it is not rule B.
- **The other 13 are drift: same instruction count, listing differs only in
  registers or operand naming.** Most are callers whose callee's original
  address now names a unit function or stand-in while they still call the
  port model. For example, `0x004524b0` calls `media_min_distance_update_mode`,
  and the original side now reads `lego_sound_minimum_distance_update_mode`.
  This is the port-tail effect seen in finding 556.
- **`0x0040e9c0` stays exact.** It was lost and recovered (see State above).
  An `extern "C"` block appended to `cmd.hpp` was replaced the same way, by
  pre-declarations in `cmd.cpp` and `lego.cpp`.
- **Improved outside the range:**
  - `0x0045b850` reaches **100%** (the fonts are annotated now);
  - `0x0045adb0`, `0x0045b740`, `0x0045c0d0`, `0x0044e6d0` and `0x0044f230`
    rise.

## Reccmp lint ratchet

The lint baseline grows from 1,527 to 1,573 entries (46 keys), all in
`lego.cpp`:

- the 45 port models left there unannotated, under their port names;
- one `definition defined` key, the lint's misparse of a nested
  `#if defined(...)` inside the port model of `0x00429160`.

The adapters carry `// PORT-ONLY:` and add no keys (#558). `--allow-growth`
was used. These keys go when part 3 retires `lego.cpp` to a `tail` entry,
which #558 exempts.

## Follow-ups

- **Callers** in the table above switch when their units, and the port's
  state, move to the original contract.
- **One legoGlobs model.** GAME-020's `interface_port.cpp` defines its own
  unannotated stand-ins for some of the same legoGlobs fields (`lego_flags1`,
  `lego_camera_fp`, the type names, …). They should become `lego_unit.c`'s
  annotated ones. The two units also each declare their own views of
  `Lego_Level`/`LegoLevel`.
- **`0x0041baa0` Interface_DoMouse.** `interface_unit.c` declares five
  parameters. `0x00426520` pushes six (the frame time) and scores against
  the original with six. One of the two units has the wrong arity; the call
  still goes through `interface_handle_mouse_adapter`.
- **`0x00429160`** (above).
- **`textGlobs_f_503618`** (`00503618`) is Text.c's word, defined here for
  lack of a port owner. **`s_lego_f_4e8448`** (`004e8448`, the first
  mini-figure) is outside both legoGlobs and Lego.c's `.bss`.
- **Rename `lego.cpp`** to `lego_port.cpp` and retire its entry in part 3.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox): the 14 drops above, 46
  improved, 15 newly mapped. The link-order map check passes.
- `make match-progress` regenerated:
  - the baseline;
  - the listing digests;
  - `binary-progress.json` (its last step ran on the host, which has
    `pefile`).

  `tools/binary_progress_inputs.txt` was rewritten for the new files, and
  `tools/pr_match_progress.py check` passes against `main`.
- `make unit-data-offsets`, GAME-021 row:
  - `.data`: partial (244, 12 unknown);
  - `.rdata`: n/a;
  - `.bss`: partial (5, 3 unknown), gap from `004de430`;
  - commons: the stand-ins.
- `python3 tools/translation_units.py build`: 0 conflicts.
- `make tools-test` passes (lint with the growth above).
- `lego_unit.c` passes both syntax checks:
  - `gcc -std=gnu89 -Werror=declaration-after-statement -fsyntax-only`;
  - `g++ -std=c++17 -fsyntax-only`.
- `cmake -S . -B build && cmake --build build && ctest --test-dir build
  --output-on-failure`: 37/37 pass, including `c_boundaries` and
  `enginebind`.

## Reproducing

    python3 tools/convert_unit.py GAME-021 --stem lego_unit --scores baseline   # in a copy without the lego.cpp link-order entry
    make match-function SOURCE=src/game/lego_unit.c
    make reccmp-check
    make match-progress
    make unit-data-offsets
