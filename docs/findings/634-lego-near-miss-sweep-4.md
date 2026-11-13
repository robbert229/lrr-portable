# Finding 634 — Near-miss sweep 4 of Lego.c

Issue #720, part of #388 and ADR 0005. Follows findings
[626](626-lego-legoobject-near-miss-sweep.md),
[629](629-lego-legoobject-near-miss-sweep-2.md) and
[630](630-interface-reward-terrain-weapons-near-miss-sweep.md). Lego.c
(GAME-021, `src/game/lego_unit.c`) had 16 functions (23,616 ledger bytes) at
95–99% and 21 (19,616 bytes) at 80–94%. Two reach 100%, and 16 functions of the
unit rise in all, 10 of them below 80%. None falls.

Measured on `main` `7835ce86` with `make reccmp-check`.

| Measure | `main` | This change |
| --- | ---: | ---: |
| New exact functions | | **2** (`00426250`, `00431580`) |
| Functions that rise / fall | | 16 / **0** (all in GAME-021) |
| Exact-function code bytes | 279,887 (43.56%) | **280,529 (43.66%)** |
| Reccmp mean, full / game-only | 95.38 / 94.98 | 95.40 / 94.99 |
| `opt_ref` removed / removed exact | 164 / 127 | 152 / 121 |
| Port `.bss`, image end | 1,115,696, `0x873000` | 1,115,680, `0x873000` |

No exact match was lost and no function fell below the baseline. The
`opt_ref` counts fall because the aliased callees below are now reached from
Lego.c, so `/OPT:REF` keeps them. `make match-progress` and
`make unit-data-offsets` were run and the generated files are committed.

## What closed the gaps

### 1. Link-name aliases to originals that are now in C units (10 functions)

Finding 629 aliased three adapters. Since then LegoObject.c, Info.c,
Interface.c, animation_object.c, Flic.c and Lego.c itself serve their
originals under C linkage, but 15 of Lego.c's calls still went to port
adapters. In the matching build only (`_MSC_VER < 1200`), each adapter now
takes the original's name:

| Adapter | Original | Owner |
| --- | --- | --- |
| `interface_handle_mouse_adapter` | `interface_do_mouse` (`0041baa0`) | interface_unit.c |
| `info_check_click_adapter` | `ui_entry_hit_test_and_dispatch` (`0041a1a0`) | info_unit.c |
| `building_placement_update_adapter` | `legoobject_building_placement_preview_update` (`0044a910`) | legoobject_unit.c |
| `lego_object_custom_name_set_adapter` | `legoobject_rename` (`00437880`), via `entity_custom_name_set_adapter` | legoobject_unit.c |
| `lego_object_movement_input_adapter` | `legoobject_movement_input_accumulate` (`0044a310`) | legoobject_unit.c |
| `lego_keep_running_adapter` | `lego_keep_running` (`00435940`) | lego_unit.c |
| `lego_creature_load_adapter` | `entity_type_config_resource_row_load` (`00406990`; its sixth parameter is unused) | animation_object.c |
| `entity_linked_activity_event_dispatch` | `legoobject_linked_activity_event_dispatch` (`0043f5b0`) | legoobject_unit.c |
| `deferred_olist_entity_queue_add_original` | `legoobject_deferred_olist_queue_add` (`00437c20`) | legoobject_unit.c |
| `entity_linked_route_flag_clear_original` | `legoobject_linked_route_flag_clear` (`0043f100`) | legoobject_unit.c |
| `lego_panel_selection_highlight_toggle_adapter` | `lego_panel_selection_highlight_toggle` (`00435aa0`) | lego_unit.c |
| `lego_noop_point_adapter` | `flic_opcode27_fallback_noop` (`00484c90`) | flic_unit.c |
| `entity_build_placement_setup_original`, `..._alt_...` | `legoobject_build_placement_command_setup` (`00439370`), `..._alternate_...` (`004394b0`) | legoobject_unit.c |
| `lego_entity_random_cell_step_route_adapter` | `legoobject_random_cell_step_route_issue` (`00444260`) | legoobject_unit.c |
| `grid_cell_entity_eligibility_check` | `lego_grid_cell_entity_eligibility_check` (`00433e30`) | lego_unit.c |
| `radar_map_zoom_set_adapter` | `object_field14_set` (`0045d8a0`, RadarMap_SetZoom) | level_id_remap.cpp |

The last row needed C linkage. `object_field14_set` was C++ only because its
declaration in `gods98/record.hpp` lacked the `extern "C"` guard that its
neighbours have. Adding the guard changes no code. Its matching-build body
is the original's naked store of the float, and `00426250` reaches 100%.

**Watch the symbol counter (finding 538, rule B).** An object-like `#define`
is free only when the target is not otherwise declared in the translation
unit. Two targets were already declared:

- `legoobject_rename`, through the existing alias of
  `entity_custom_name_set_adapter`;
- `lego_grid_cell_entity_eligibility_check`, at its forward declaration.

As object-like macros, these two cost a symbol from the first function on,
and `00432360` (100% to 84.85%) and `00433380` drifted.

For both, a function-like macro placed after the adapter's declaration keeps
every declaration, and with it the counter. The two same-unit targets
declared only near the end (`lego_keep_running` and
`lego_panel_selection_highlight_toggle`, forward-declared before `00434570`)
cost one symbol each from `00434570` on. `counter_probe.py` reports −2 there
and no difference before. No function in that range changes code.

Natively nothing changes: the adapters are still declared and defined there.

### 2. Statement order of independent stores (6 functions)

Finding 629 found that declaration order rarely matters. The order of
independent assignments does: MSVC 5.0 allocates registers and schedules
stores from it. Two sweeps with `make match-function` swapped each adjacent
pair of simple assignments in Lego.c's non-exact functions, one swap at a
time (50 `x[0]`/`x[1]` pairs, then 521 general pairs, 4 workers). Each
improving swap was checked for a data dependency by hand: two had one and
one was a declaration list, so those three were dropped. The rest hold
together:

- **`00431580`**: `listSize` assigned after `counts[0]` and `counts[1]` and
  before `curr` gives the original's `ebp`/`ebx` use. **100%**. Six of the
  orders score 96.20 to 96.84%. Computing it after the three zeroes is only
  an effective match (`eax` for `ebp`).
- **`00431b20`**: `blockPos[1]` stored before `blockPos[0]`: 88.12% to
  96.04%.
- `004250c0`, `00426520`, `00429890`, `0042e020`, `0042f6f0`, `00431780`,
  `00433c10` and `004355c0` gain from 0.6 to 6.4 points from one or two
  swaps each: `004355c0` from the two font getters, `0042e020` from the
  shape lookup before the heading.
- `0042cb70`: `blockPos[0]` before `blockPos[1]` puts the two stores in the
  original's order. The `imul` is then scheduled before the blocks load
  instead of after it, so the score stays the same.

The permuter (`msvc_permuter.py`, 120 s each) found nothing on `004291a0`,
`00429470`, `0042cb70`, `00431580`, `0042f350`, `004288e0`, `00428020` or
`0042de40`. Its assignment-swap pass is narrower than this sweep.

### 3. Tried and reverted: `004291a0` (block-scoped vectors)

The original's track branch reads the camera position into the first
branch's *direction* slot. Moving the two vectors into the first branch's
block and giving the track branch its own block-scoped vector reproduces
that exactly. The single-function loop then shows only callee-name
artefacts. But it is a third variable, so the counter rises by one from
`004293b0` on, and `00431450` (exact) drops to 78.38% and `004311d0` to
63.50%.

The two-variable forms are counter-neutral but lose the sharing (86.71%):
with a function-scope `dir` used by the track branch, the more-used variable
takes the lower slot. Folding `dist` into its only use to pay for the third
variable changes the mode-0 branch (67%). Not applied. It would fit a later
change that removes a symbol between `004293b0` and `00431450`.

## Per-function results

Sizes are the ledger's address gaps. Scores are full `make reccmp-check`
runs. The letters are the causes tallied below.

### The 95–99% band

| Address | Function | Size | `main` | After | Change, or what is left |
| --- | --- | ---: | ---: | ---: | --- |
| `0041fb50` | lego_game_config_init | 11520 | 99.29 | 99.29 | A: skipped (re-checked after #719's stack rebase). Left: `0044db50` and `0044dde0` (#719's `loading_progress.cpp`); `info_initialise` and `info_set_overflow_image_file` (`00419430`, info_unit.c) under port names; `legoobject_trigger_frame_callback` (legoobject_unit.c) pushed as `lego_object_trigger_frame_callback_adapter`; ten of Lego.c's own config loaders still under port names (`sound_samples_config_load` → `00434a50`, `surface_type_descriptions_config_load` → `004350a0`, `scene_lights_build_from_config_original` → `00433770`, `render_quality_config_apply_original` → `004334f0`, `advisor_tooltip_and_sound_config_load` → `00434da0`, `message_load_infomessages`, `text_messages_config_load_original` → `00434af0`, `panels_config_entry_apply_original`, `panel_button_layout_config_load_original` → `00434710`, `advisor_tutorial_icon_image_load` → `00434a00`); `select_place_create_adapter`; `lego_noop_load_adapter` → `00484c90`; `":|"` against `","` (another unit's literal) |
| `00430530` | lego_terrain_cell_mutate | 2240 | 95.45 | 95.45 | D: the inlined work-flag clear's tests merged (finding 629's `00430320`); `imul` operands for `by + 1` (local macro order tried); a read-modify-write `or` |
| `004288e0` | lego_level_debug_input_cell_mutator_dispatch | 2176 | 98.41 | 98.41 | C: previous-key load hoisted before `test al, al` and a `cl`→`dl` copy around `LEGO_KEY_PRESSED(0x2e)` |
| `00422850` | lego_misc_object_resources_load | 2096 | 99.84 | 99.84 | A: Encyclopedia's `s_item_name_ore` (`encyclopedia.cpp`, #719's file, C++ linkage) |
| `0042f350` | lego_terrain_neighbor_coordinate_check | 928 | 96.26 | 96.26 | C: `ebp`/`edi` for the level; 48 declaration orders, a conditional expression and `lego_C_block_flag20000000` all score the same |
| `0042b510` | lego_map_load_pipeline | 832 | 98.20 | 98.20 | D: the original tests the loaders' results with `test eax, eax` and falls into the shared epilogue with eax 0. Unassigned tests give `test` but need an explicit `xor eax, eax` (95.25%); `!= 0`, a comma test and three result types change nothing |
| `00428020` | lego_entity_command_status_feedback_dispatch | 784 | 98.89 | 98.89 | B: end-of-array pointer names the next global; `ecx`/`eax` |
| `004334f0` | lego_render_quality_config_apply | 640 | 99.51 | 99.51 | A: `direct3d_render_preferences_apply` (`00478330`, surface.cpp) has C++ linkage and a different contract |
| `004291a0` | lego_scene_marker_update_and_draw | 528 | 98.10 | 98.10 | D: stack slot sharing; the block-scoped form matches but costs a counter symbol (section 3) |
| `00431580` | lego_terrain_neighbor_work_flag_scan | 512 | 96.20 | **100.00** | `listSize` assigned after the counts are cleared |
| `00429470` | lego_camera_pointer_ray_route | 304 | 98.80 | 98.80 | E: `fstp` slot (finding 629) |
| `0042cb70` | lego_camera_grid_expiry | 288 | 96.67 | 96.67 | D: `imul` before the blocks load; the two stores are now in the original's order |
| `00426420` | lego_cursor_ray_to_camera_route | 256 | 95.77 | 95.77 | D: three zero stores scheduled before the pushes; six local orders, three store forms and two placements tried |
| `004350a0` | lego_surface_type_descriptions_config_load | 208 | 98.55 | 98.55 | B: end-of-array pointer names the next global |
| `00424730` | lego_sine_phase_midpoint_blend_dispatch | 160 | 95.45 | 95.45 | B/E: `fogColourRGB - 4` names another global, and `half * s` is computed after `half + fog` (eight expression forms tried) |
| `00426250` | lego_update_render_viewport_state | 144 | 97.37 | **100.00** | RadarMap_SetZoom by `object_field14_set` (C linkage) |

### The 80–94% band

| Address | Function | Size | `main` | After | Change, or what is left |
| --- | --- | ---: | ---: | ---: | --- |
| `00429890` | lego_level_load | 6752 | 93.96 | 94.66 | swap of two separator splits. A: `00459470` OSF read and `004593d0` (object_state_file.cpp, C++ with a GameContext), `0044dde0` (#719), `0045d7e0` unannotated, `00419ae0` Info_Enable against `incident_message_flag_set` |
| `004250c0` | lego_main_sim_step | 2688 | 89.70 | 91.25 | aliases (keep-running, rename, movement input) and two swaps. A: `light_interface_call_slot_40`/`_3c` reached through `lego_light_penumbra_set_adapter`/`lego_light_umbra_set_adapter`; D: camera-key block order |
| `00433e80` | lego_terrain_projected_cell_state_refresh | 1536 | 94.51 | 94.51 | E |
| `0042e8b0` | lego_entity_type_pair_lookup | 1216 | 84.41 | 84.41 | A: Encyclopedia's `"Ore"`/`"PowerCrystal"` (#719's file); B: catalogue counts |
| `00425d90` | lego_terrain_pointer_command_update | 1072 | 89.26 | 89.26 | B/D: the original has one more frame word and loads the level inside the branch; moving the load and the `diff` scope score lower |
| `0042d600` | lego_object_names_load | 1056 | 88.54 | 88.54 | A: Encyclopedia literals |
| `0042da20` | lego_object_the_names_load | 1056 | 88.54 | 88.54 | A: Encyclopedia literals |
| `0042f0c0` | lego_level_free | 544 | 85.31 | 86.01 | alias (panel toggle). B: the original does not fold consecutive `&=` on its flag words (members of legoGlobs); three forms tried |
| `0042cda0` | lego_entity_vehicle_type_table_load | 432 | 93.47 | 93.47 | B: `s_catalog_counts` (entity.cpp, C++; its declaration is in frozen `entity_for_lego.hpp`) |
| `0042cf50` | lego_entity_minifigure_type_table_load | 432 | 92.83 | 93.52 | alias (Creature_Load). B: catalogue counts |
| `0042d100` | lego_entity_rockmonster_type_table_load | 432 | 92.83 | 93.52 | alias (Creature_Load). B: catalogue counts |
| `0042d2b0` | lego_entity_building_types_load | 432 | 93.47 | 93.47 | B: catalogue counts |
| `0042d460` | lego_upgrade_types_load | 416 | 93.14 | 93.14 | B: catalogue counts |
| `0042de40` | lego_viewport_target_position_clamp | 384 | 92.31 | 92.31 | D: `blockPos` load hoisted out of the two branches |
| `00431b20` | lego_terrain_placement_blocked_adjacent_cell_scan | 336 | 88.12 | 96.04 | `blockPos[1]` first. D: the block computation scheduled after the stores |
| `0042bf40` | lego_load_emerge_layer | 288 | 89.72 | 89.72 | D: `test dl, dl` (finding 629) |
| `00430df0` | lego_terrain_neighbor_refresh | 240 | 90.17 | 92.49 | aliases (build placement setup). C: register rotation of the level and block loads |
| `0042ef40` | lego_stats_level_record_count_for_stat_type | 128 | 90.57 | 90.57 | B: catalogue counts |
| `0042cd50` | lego_upgrade_name_entry_find | 80 | 94.12 | 94.12 | B: catalogue counts |
| `0042dfc0` | lego_object_pointer_registry_clear_match | 48 | 82.35 | 82.35 | B: object pointer registry (C++ linkage) |
| `0042dff0` | lego_object_pointer_registry_get_slot | 48 | 84.62 | 84.62 | B: object pointer registry (C++ linkage) |

### Below 80%

| Address | Function | `main` | After | Change |
| --- | --- | ---: | ---: | --- |
| `00426520` | lego_main_sim_interaction_and_cell_update | 70.50 | 71.10 | aliases (Interface, Info, placement preview) and two swaps |
| `0042f6f0` | lego_cell_init_pass | 77.84 | 80.99 | alias (`00484c90`) and a swap |
| `0042e020` | lego_object_list_load_file | 69.33 | 73.47 | three LegoObject.c aliases and two swaps |
| `004355c0` | lego_menu_option_panel_texture_update | 59.07 | 65.42 | font getters swapped |
| `00435300` | lego_tutorial_block_icons_update | 59.89 | 60.44 | alias (`00433e30`) |
| `00431780` | lego_terrain_stamp_impact_handle | 68.90 | 69.91 | a swap |
| `00433c10` | lego_entity_neighbor_blocked_cell_step_probe | 66.67 | 72.12 | alias (`00444260`) and a swap |

## Why the other 35 are short

| Cause | Functions | Ledger bytes | What would close it |
| --- | ---: | ---: | --- |
| **A. Callee or literal outside the unit**: `0044db50`/`0044dde0` and Encyclopedia's literals (#719's files); `00459470`/`004593d0` (OSF, C++ with another contract); `00478330` (surface.cpp, C++); `0045d7e0` unannotated; Lego.c's own config loaders called under port names from `0041fb50` | 8 | 27,024 | #719's annotations, then aliases like section 1; for `0041fb50`, the same aliasing of the unit's own loaders after the rebase |
| **B. Data layout**: `s_catalog_counts` and the object registry with C++ linkage (frozen headers), end-of-array pointers, legoGlobs flag words as separate globals | 14 | 5,216 | legoGlobs as one struct; C linkage for `s_catalog_counts` once its `_for_` header can change |
| **C. Integer register choice** | 3 | 3,344 | not found |
| **D. Scheduling, stack slots or statement shape** | 8 | 5,152 | `004291a0`: the block-scoped form, if a symbol is removed before `00431450` |
| **E. x87 scheduling** | 2 | 1,840 | not found |

## Files

- `src/game/lego_unit_imports.h`, `src/game/lego_unit.c`: matching-build
  aliases (section 1) and the statement orders (section 2). Only Lego.c's own
  declarations change.
- `src/gods98/record.hpp`: `object_field14_set` gets C linkage.
- `docs/coverage/`: match baseline, listing digests and binary progress.
