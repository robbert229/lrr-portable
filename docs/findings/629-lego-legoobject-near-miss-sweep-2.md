# Finding 629 — Near-miss sweep 2 of Lego.c and LegoObject.c

Issue #708, part of #388 and ADR 0005. It follows
[finding 626](626-lego-legoobject-near-miss-sweep.md). Lego.c (GAME-021,
`src/game/lego_unit.c`) and LegoObject.c (GAME-023,
`src/game/legoobject_unit.c`) had 38 functions at 95–99% and 46 at 80–94%.
This sweep brings 9 of the 84 to 100%: 4 from the 95–99% band and 5 from the
80–94% band. It also records why each of the other 75 is short.

Measured on `main` `7600fb4c` with `make reccmp-check`.

| Measure | `main` | This change |
| --- | ---: | ---: |
| New exact functions | | **9** (all in the two units) |
| Functions that rose / fell | | 9 / **0** |
| Exact-function code bytes | 244,057 | **246,809** (+2,752) |
| Reccmp mean, full / game-only | 92.71 / 92.02 | 92.73 / 92.04 |
| `opt_ref` removed / removed exact | 327 / 198 | 326 / 199 |
| Port `.bss`, image end | 1,111,760, `0x86d000` | unchanged |

No exact match was lost and no function fell below the baseline. Only the 9
functions changed score. MSVC's symbol counter (finding 538, rule B) is
unchanged at every function of both units:
`tools/experiments/699/counter_probe.py` reports no difference against
`7600fb4c`. `make match-progress` and `make unit-data-offsets` were run, and
the generated files are committed.

## What closed the gaps

### 1. Callees converted since finding 626 (3 functions)

Container.c (#697) and Building.c now serve their original functions in the
matching build under the port's link names (`container_link_names.h`,
`building_model_link_names.h`). Three adapters in the two units still went to
port models. They are renamed one for one, in the matching build only, just
before their declaration. A `#define` costs no symbol, and the target name is
not otherwise declared in the translation unit, so the counter does not move:

- `lego_container_create_adapter` → `ddiscene_object_allocate` (Container_Create, `00472950`);
- `legoobject_gs_container_is_hidden_adapter` → `scene_object_flag_bit3_get_target` (Container_IsHidden, `00473ba0`);
- `legoobject_gs_object_is_hidden_adapter` → `animation_object_visibility_get` (`004086a0`, building_model.c).

`00433770`, `0044bc30` and `00449ff0` reach 100%. Natively nothing changes.

### 2. Real corrections (2 functions)

- **`0044b4c0` returns 1** when the object is not a mini-figure or the task
  type needs no tool (the original's `mov eax, 1`). The source returned 0.
  Reccmp's original extent ended mid-instruction, so the diff showed this
  only as three surplus rebuilt instructions.
- **Detail_AddBlock's texture and direction are bytes.** detail.c's
  `0040b880` takes `unsigned char`, and Lego.c pushes the block bytes
  without zero-extending them. The matching build's declaration in
  `lego_unit.c` now uses `unsigned char` (`LEGO_DETAIL_BYTE`). The native
  build keeps `uint32_t`, which view.cpp's port entry takes. `00434480`
  reaches 100%.

### 3. Source shape (3 functions)

- **`004474f0`**: `(float)(radius * 2.0f)`. Rounding the doubled radius to
  float gives the original's `fadd st(0), st(0)`. Without the cast, MSVC 5.0
  multiplies by a constant and reverses the add (`fsubr`).
- **`004230c0`**: `width` declared before `x` and `y`, which gives the
  original's `lea edx, [edi + eax]`. This is the one case where local order
  mattered. For `00429470`, all 720 orders of its six locals score the same.
  For `0043fff0`, a hill climb over the order of its eleven locals found no
  change. So stack slots there do not follow declaration order (section 5).
- **`00435010`** (naked asm): the original encodes `mov [esp], eax` with a
  zero 8-bit displacement (`89 44 24 00`), but the inline assembler emits
  the 3-byte form. The function was one byte short, and Reccmp's original
  extent dropped its `ret` (finding 626's cause F). The original bytes are
  now emitted with `_emit`.

### 4. One `Deviation (scoring only)` (1 function)

- **`0043a960`**: the original reloads the live-object count after storing
  into the array, because both are members of `objectGlobs`. The port keeps
  two separate globals. A `volatile` increment reproduces the reload in C,
  where the unit's other two cases (finding 626) needed naked asm. The same
  form does not fix `00438310`: there the reload is in the right place, but
  MSVC schedules it before the call cleanup.

## Per-function results

Sizes are the ledger's address gaps. The letters are the causes tallied
below. `main` and After are full `make reccmp-check` scores.

### The 95–99% band

| Address | Function | Unit | Size | `main` | After | Change, or what is left |
| --- | --- | --- | ---: | ---: | ---: | --- |
| `0041fb50` | lego_game_config_init | GAME-021 | 11520 | 98.18 | 98.18 | A: 14 calls to unannotated originals (resource.cpp, BitmapSurfaceLoad, menus, MoviePlayLoop), ~30 port adapters (FrontEnd.c part 2, Images.c, Info.c, Interface.c, Lego.c's own config loaders under port names); unattributed `"Version"`/`"FrontEnd"` |
| `00430530` | lego_terrain_cell_mutate | GAME-021 | 2240 | 95.45 | 95.45 | D: block layout and operand order across 2.2 KB |
| `004288e0` | lego_level_debug_input_cell_mutator_dispatch | GAME-021 | 2176 | 98.41 | 98.41 | C: register choice (`cl`/`dl`) and jump distances |
| `00422850` | lego_misc_object_resources_load | GAME-021 | 2096 | 99.84 | 99.84 | A: pushes Encyclopedia's `"Ore"`, a C++-linkage `const` array |
| `0043bc40` | legoobject_objective_sidecar_append_gate | GAME-023 | 1472 | 99.11 | 99.11 | A: `osf_add` and `entity_cell_path_effects_apply_current` have C++ linkage; `00409ff0` unannotated |
| `00433770` | lego_scene_lights_build_from_config | GAME-021 | 1136 | 99.72 | **100.00** | Container_Create by its link name `ddiscene_object_allocate` (alias) |
| `00436c20` | legoobject_initialise | GAME-023 | 1072 | 96.67 | 96.67 | D: `rep stosd` scheduled against two immediates |
| `0044a910` | legoobject_building_placement_preview_update | GAME-023 | 944 | 99.69 | 99.69 | A: callee `004641c0` unannotated |
| `0042f350` | lego_terrain_neighbor_coordinate_check | GAME-021 | 928 | 96.26 | 96.26 | C: `ebp`/`edi` for the level and block array |
| `0042b510` | lego_map_load_pipeline | GAME-021 | 832 | 98.20 | 98.20 | C: `test eax, eax` against `cmp eax, ebx` |
| `0043af30` | legoobject_creature_spawn_from_emergence | GAME-023 | 832 | 97.89 | 97.89 | C: `ecx`/`eax` for one flag update |
| `00428020` | lego_entity_command_status_feedback_dispatch | GAME-021 | 784 | 98.89 | 98.89 | B: end-of-array pointer names the next global |
| `0044c1b0` | legoobject_heading_step_move | GAME-023 | 752 | 97.75 | 97.75 | E: x87 scheduling inside the inlined normalise |
| `004334f0` | lego_render_quality_config_apply | GAME-021 | 640 | 99.51 | 99.51 | A: `direct3d_render_preferences_apply` (`00478330`) has C++ linkage |
| `00437540` | legoobject_remove | GAME-023 | 624 | 99.47 | 99.47 | A: pushes `00472060`, unannotated |
| `004428a0` | legoobject_world_position_set | GAME-023 | 624 | 98.94 | 98.94 | E: float self-copy: `fld`/`fstp` in the original, `mov` here |
| `00447c40` | legoobject_activity_register_bookkeeping | GAME-023 | 608 | 96.85 | 96.85 | D: epilogue placement |
| `004401b0` | legoobject_terrain_cell_to_world_position | GAME-023 | 544 | 98.06 | 98.06 | D: argument pushes around a call's stack cleanup |
| `004291a0` | lego_scene_marker_update_and_draw | GAME-021 | 528 | 98.10 | 98.10 | D: stack slot assignment |
| `00431580` | lego_terrain_neighbor_work_flag_scan | GAME-021 | 512 | 96.20 | 96.20 | C: register allocation of width/height |
| `00441ed0` | legoobject_face_target_action_start | GAME-023 | 512 | 98.75 | 98.75 | E: `faddp`/`fsqrt` against the pushes |
| `0044bc30` | legoobject_motion_to_scene_node_sync | GAME-023 | 512 | 99.35 | **100.00** | Container_IsHidden by its link name `scene_object_flag_bit3_get_target` (alias) |
| `0043ad50` | legoobject_predug_spawn_coordinate_select_and_create | GAME-023 | 336 | 99.02 | 99.02 | D: load/store order of a pair copy |
| `004230c0` | lego_debug_panel_text_draw_update | GAME-021 | 304 | 98.95 | **100.00** | `width` declared before `x` and `y` (the `lea` operand order) |
| `00429470` | lego_camera_pointer_ray_route | GAME-021 | 304 | 98.80 | 98.80 | E: `fstp` slot; all 720 orders of its six locals score the same |
| `004477d0` | legoobject_path_step_lookahead_write | GAME-023 | 304 | 97.70 | 97.70 | D: `pop edi` placement |
| `0042cb70` | lego_camera_grid_expiry | GAME-021 | 288 | 96.67 | 96.67 | D: stores scheduled around an `imul` |
| `00426420` | lego_cursor_ray_to_camera_route | GAME-021 | 256 | 95.77 | 95.77 | D: stack slots of three zeroed locals |
| `004350a0` | lego_surface_type_descriptions_config_load | GAME-021 | 208 | 98.55 | 98.55 | B: end-of-array pointer names the next global |
| `004396c0` | legoobject_item_name_by_kind | GAME-023 | 208 | 96.97 | 96.97 | A: Encyclopedia's `"Ore"`/`"PowerCrystal"` (C++ linkage) |
| `00424730` | lego_sine_phase_midpoint_blend_dispatch | GAME-021 | 160 | 95.45 | 95.45 | B: `fogColourRGB - 4` names the preceding `<common>` global (linker order), and an `fxch` |
| `00426250` | lego_update_render_viewport_state | GAME-021 | 144 | 97.37 | 97.37 | A: `object_field14_set` (`0045d8a0`) has C++ linkage |
| `00435010` | terrain_cell_work_flag_circle_set_current | GAME-021 | 144 | 99.16 | **100.00** | naked asm: `mov [esp], eax` emitted in the original's 4-byte encoding (`_emit`) |
| `00438310` | legoobject_pool_chunk_allocate | GAME-023 | 144 | 97.22 | 97.22 | B: count reload scheduled before the call cleanup; a volatile read reloads but in the wrong slot |
| `00425ce0` | lego_game_speed_set | GAME-021 | 112 | 96.30 | 96.30 | A: `options_scroll_index_store` (`00414f90`) is C++ in frontend_port.cpp (#707's file) |
| `00437050` | legoobject_shutdown | GAME-023 | 96 | 96.00 | 96.00 | B: end-of-array pointer names the next global |
| `004385b0` | legoobject_shared_eligibility_check | GAME-023 | 96 | 96.00 | 96.00 | B: flag immediate inside `fileRedundantList` on both sides |
| `00434a00` | lego_advisor_tutorial_icon_image_load | GAME-021 | 80 | 96.15 | 96.15 | A: callee `0047d9d0` (BitmapSurfaceLoad) unannotated |

### The 80–94% band

| Address | Function | Unit | Size | `main` | After | Change, or what is left |
| --- | --- | --- | ---: | ---: | ---: | --- |
| `00429890` | lego_level_load | GAME-021 | 6752 | 92.56 | 92.56 | A: Objective.c, Info.c, OSF, movie and remap callees (port adapters); x87 and layout beside |
| `004250c0` | lego_main_sim_step | GAME-021 | 2688 | 89.06 | 89.06 | A: unannotated `004589c0`; adapters for LegoObject.c, Objective.c, Container lights; also camera-key block order (D) |
| `00433e80` | lego_terrain_projected_cell_state_refresh | GAME-021 | 1536 | 94.51 | 94.51 | E: x87 scheduling across the projected-cell refresh |
| `0042e8b0` | lego_entity_type_pair_lookup | GAME-021 | 1216 | 84.41 | 84.41 | B: legoGlobs catalogue counts through `legoGlobs_config + offset` |
| `004469c0` | legoobject_nearest_connected_cell_route_select | GAME-023 | 1152 | 81.79 | 81.79 | E: x87 and integer scheduling in the route selection |
| `00442b10` | legoobject_movement_accumulator_apply | GAME-023 | 1136 | 86.12 | 86.12 | E: x87 scheduling in the movement step |
| `00425d90` | lego_terrain_pointer_command_update | GAME-021 | 1072 | 89.26 | 89.26 | B: renderer/mouse words; one level load |
| `0042d600` | lego_object_names_load | GAME-021 | 1056 | 88.54 | 88.54 | B: legoGlobs name tables through `legoGlobs_config + offset` |
| `0042da20` | lego_object_the_names_load | GAME-021 | 1056 | 88.54 | 88.54 | B: legoGlobs name tables through `legoGlobs_config + offset` |
| `004403d0` | legoobject_level_debug_cell_retarget_dispatch | GAME-023 | 992 | 86.12 | 86.12 | D: block layout of a switch |
| `0043f190` | legoobject_spawn_request_queue_process | GAME-023 | 976 | 94.53 | 94.53 | B: `legoGlobs_f_556514`; a frame-slot difference |
| `00437940` | legoobject_deferred_olist_create_and_link | GAME-023 | 736 | 81.93 | 81.93 | B: `objectGlobs_recordObjs` layout; NERPs adapter |
| `0043eea0` | legoobject_simulation_queue_flush | GAME-023 | 608 | 89.26 | 89.26 | D: compare-and-branch shape (`sete`/`cmp 1`) and a hoisted load |
| `0042f0c0` | lego_level_free | GAME-021 | 544 | 85.31 | 85.31 | B: legoGlobs flag words and panel adapter |
| `0043a120` | legoobject_cell_step_command_issue_with_jitter | GAME-023 | 480 | 81.00 | 81.00 | E: x87 jitter computation |
| `0042cda0` | lego_entity_vehicle_type_table_load | GAME-021 | 432 | 93.47 | 93.47 | B: catalogue counts (`s_catalog_counts`, C++ linkage) |
| `0042cf50` | lego_entity_minifigure_type_table_load | GAME-021 | 432 | 92.83 | 92.83 | B: catalogue counts; creature-load adapter |
| `0042d100` | lego_entity_rockmonster_type_table_load | GAME-021 | 432 | 92.83 | 92.83 | B: catalogue counts; creature-load adapter |
| `0042d2b0` | lego_entity_building_types_load | GAME-021 | 432 | 93.47 | 93.47 | B: catalogue counts (`s_catalog_counts`, C++ linkage) |
| `0042d460` | lego_upgrade_types_load | GAME-021 | 416 | 93.14 | 93.14 | B: catalogue counts (`s_catalog_counts`, C++ linkage) |
| `00447210` | legoobject_adjacent_placement_cell_search | GAME-023 | 416 | 85.07 | 85.07 | C: register rotation through a lookup loop |
| `0044afb0` | legoobject_tiny_monster_spawn_and_teardown | GAME-023 | 400 | 85.71 | 85.71 | D: an `int`-to-float temporary's slot and a hoisted load |
| `0042de40` | lego_viewport_target_position_clamp | GAME-021 | 384 | 92.31 | 92.31 | D: `blockPos` load hoisted out of the two branches |
| `0043fe70` | legoobject_route_to_explicit_target | GAME-023 | 384 | 82.39 | 82.39 | D: stack slot assignment |
| `00431b20` | lego_terrain_placement_blocked_adjacent_cell_scan | GAME-021 | 336 | 88.12 | 88.12 | D: statement order in the neighbour scan |
| `00437150` | legoobject_tool_tip_icons_load | GAME-023 | 336 | 94.17 | 94.17 | A: callee `0047d9d0` unannotated; `named_keys` static (B) |
| `0043fff0` | legoobject_route_marker_setup | GAME-023 | 320 | 85.37 | 85.37 | D: stack slots (one local in the dead parameter slot); declaration order has no effect |
| `0043f6a0` | legoobject_damage_apply | GAME-023 | 304 | 94.44 | 94.44 | C: `ebx`/`edi` swapped for the damage value |
| `004492b0` | legoobject_cell_step_command_issue_with_target_mark | GAME-023 | 304 | 84.62 | 84.62 | D: stack slot assignment |
| `0042bf40` | lego_load_emerge_layer | GAME-021 | 288 | 89.72 | 89.72 | D: `(value % 2) == 1` tested as `test dl, dl` in the original |
| `00430df0` | lego_terrain_neighbor_refresh | GAME-021 | 240 | 90.17 | 90.17 | A: LegoObject.c build-placement callees reached through port names; one level load |
| `004474f0` | legoobject_linked_proximity_response | GAME-023 | 208 | 92.09 | **100.00** | `(float)(radius * 2.0f)`: the doubled radius is rounded to float, as the original's `fadd st(0), st(0)` |
| `0044bf00` | legoobject_timed_callback_list_update | GAME-023 | 208 | 87.14 | 87.14 | B: `s_entity_auxiliary_release_records` (port record) |
| `00434480` | lego_terrain_cell_surface_update_dispatch_level | GAME-021 | 176 | 88.73 | **100.00** | Detail_AddBlock's texture and direction are bytes in the matching build (no zero-extension) |
| `00427f80` | lego_level_upgrade_name_strings_load | GAME-021 | 160 | 94.74 | 94.74 | A: `"Level"` is another unit's literal (unmarked in the original listing) |
| `0044a200` | legoobject_type_component_get | GAME-023 | 160 | 91.49 | 91.49 | D: the original reloads the parameter for `return obj` |
| `0043aea0` | legoobject_emergence_random_spawn_attempt | GAME-023 | 144 | 92.00 | 92.00 | D: the original reserves one more frame slot |
| `0042ef40` | lego_stats_level_record_count_for_stat_type | GAME-021 | 128 | 90.57 | 90.57 | B: catalogue counts (`s_catalog_counts`, C++ linkage) |
| `0043a960` | legoobject_lava_crust_activity_register | GAME-023 | 128 | 82.35 | **100.00** | `Deviation (scoring only)`: volatile increment reproduces the count reload after the store |
| `00449ff0` | legoobject_type_operation_release | GAME-023 | 128 | 88.64 | **100.00** | the same alias, and building_model.c's `animation_object_visibility_get` (`004086a0`) for four calls |
| `004372a0` | legoobject_spawn_coordinate_pool_clear | GAME-023 | 96 | 91.67 | 91.67 | D: `rep stosd` scheduled after three stores |
| `0042cd50` | lego_upgrade_name_entry_find | GAME-021 | 80 | 94.12 | 94.12 | B: catalogue counts (`s_catalog_counts`, C++ linkage) |
| `00439950` | legoobject_tool_name_strings_load | GAME-023 | 64 | 92.00 | 92.00 | B: end pointer and `named_keys` static of object_names_load |
| `0044b4c0` | legoobject_type_two_target_resolve_and_apply | GAME-023 | 64 | 92.68 | **100.00** | returns 1, not 0, when the object is no mini-figure or the task needs no tool |
| `0042dfc0` | lego_object_pointer_registry_clear_match | GAME-021 | 48 | 82.35 | 82.35 | B: object pointer registry (C++ linkage) |
| `0042dff0` | lego_object_pointer_registry_get_slot | GAME-021 | 48 | 84.62 | 84.62 | B: object pointer registry (C++ linkage) |

## Why the other 75 are short

| Cause | Functions | Ledger bytes | What would close it |
| --- | ---: | ---: | --- |
| **A. Callee or literal outside these units**: unannotated originals (`004641c0`, `00472060`, `0047d9d0`, `004589c0`, resource.cpp's block-comment entries), C++-linkage port entries (`osf_add`, `direct3d_render_preferences_apply`, `object_field14_set`, `options_scroll_index_store`, Encyclopedia's `"Ore"`/`"PowerCrystal"`), and other units' literals | 15 | 28,016 | those units' conversions (Images.c #711, FrontEnd.c part 2 #707, Objective.c #710, Info.c, Interface.c); then a link-name alias here |
| **B. Data layout**: legoGlobs read through `legoGlobs_config + offset`, C++-linkage `s_catalog_counts` and registry, end-of-array pointers, `<common>` order of the fog colours, `objectGlobs` as separate globals | 24 | 10,864 | legoGlobs and objectGlobs as single structs; layout identity |
| **C. Integer register choice or operand order** | 7 | 6,000 | not found |
| **D. Scheduling, stack slots, block layout or statement shape** | 21 | 10,592 | not found; declaration order of locals does not decide the slots |
| **E. x87 scheduling and float copies** | 8 | 6,496 | not found |

The large 80–94% functions (`00429890`, `004250c0`, `0042e8b0`, `004469c0`,
`00442b10`, `004403d0`, `00437940`) were classified from their diffs, not
worked through. Each has several causes, and the table names the largest.

What was tried without success, so a later sweep need not repeat it:

- `004428a0`: `pos` assigned in four orders, as an initialiser, and before
  the heading block; the add with its operands exchanged. The self-copy
  stays `mov`.
- `0044a200`: the fallback as a `default:` case, through a temporary, a
  conditional expression, or `*(void**)&obj`. The original's parameter
  reload never appears.
- `0042bf40`: the odd test as `% 2`, `& 1`, `!= 0`, `(unsigned char)`,
  `(int)` and with the branches inverted, and `erode` as `unsigned char`.
  None gives `test dl, dl`, and most score lower.
- `00438310`: a volatile read of the count, and a volatile store into the
  array. Both reload in the wrong place.
- `0043fff0` and `00429470`: local declaration order (see section 3).

## Files

- `src/game/lego_unit.c`: the Container_Create alias, the Detail_AddBlock
  byte parameters (matching build), `004230c0`'s local order and
  `00435010`'s encoding. Only Lego.c's own declarations change.
- `src/game/legoobject_unit.c`: the two hidden-test aliases, `0044b4c0`'s
  return value, `004474f0`'s cast and `0043a960`'s volatile increment.
- `docs/coverage/`: match baseline, listing digests and binary progress.
