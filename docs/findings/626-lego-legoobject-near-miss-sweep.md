# Finding 626 — Near-miss sweep of Lego.c and LegoObject.c

Issue #699, part of #388 and ADR 0005. Lego.c (GAME-021, `src/game/lego_unit.c`)
and LegoObject.c (GAME-023, `src/game/legoobject_unit.c`) are converted, but 52
of their functions scored 95–99%. This sweep brings 19 of the 52 to 100%, and
two more from the 80–94% band. It also records why each of the other 33 is
short, as a guide for later sweeps (Interface.c, Reward.c, the terrain map,
Weapons.c).

Measured on `main` `d4c4cf5c` with `make reccmp-check`.

| Measure | `main` | This change |
| --- | ---: | ---: |
| Exact functions in the 95–99% band (52) | 0 | **19** |
| New exact functions, all units | | **22** (19 + `0042b2f0`, `004330e0`, Reward.c's `00460220`) |
| Functions that rose / fell | | 47 / **0** |
| Exact-function code bytes | 208,965 | **216,621** (+7,656) |
| Reccmp mean, full / game-only | 89.03 / 87.97 | 89.07 / 88.01 |

No exact match was lost and no function fell below the baseline. `make
match-progress` and `make unit-data-offsets` were run and their files
committed.

## What closed the gaps

Almost every gain came from the call and data operands, not from code shape.
Of the 21 functions brought to 100% in the two units, 19 needed no change to
their own code.

### 1. Literals: Lego.c's own, and config.c's pooled `""` and `":"`

- **STRING markers for Lego.c's literals** (finding 569's rule: a pooled
  literal gets one marker, in the unit that owns its address). Lego.c owns
  `"Pilot"` (`004a356c`), `"MESH"` (`004a4014`) and `"%s%s%s"` (`004a4024`),
  which the GAME-021 conversion left unmarked. It also owns `"ElectricFence"`,
  `"ProcessedOre"`, `"Barrier"`, `"OohScary"`, `"Dynamite"`, `"tvcamera"` and
  `"Activity_Stand"` (`004a3f90`–`004a46f8`). `lego_port.cpp` annotated those
  seven addresses as `GLOBAL`s for its `s_item_name_*` copies, so every use of
  the literal in an original-shape unit compared against a port symbol. The
  `GLOBAL` annotations are removed (the port's copies stay, unannotated, as in
  finding 620) and `lego_unit.c` marks the literals. Reward.c's
  `reward_level_start` (`00460220`) reaches 100% from these markers alone.
- **config.c's pooled literals.** Lego.c's `""` and `":"` resolve to
  `004b8a14` and `004a1cd4`, config.c's `s_config_angle_empty` and
  `s_config_rgb_separator` (finding 581). `LEGO_A_CFG_STR`, `LEGO_D_STR`, the
  rename-cursor quotes and the 11 `sound_variant_split(…, ":")` calls now name
  them, declared in `lego_unit_imports.h`.
- Literals that other units own stay unmarked: `"Main"` (`004a24e4`), `"|"`,
  `"Version"`, `"FrontEnd"`, `"Level"`, `"Saves"`, `"%s\\%i.osf"`,
  `"FullName"`, `"Path"`, and Encyclopedia's `"Ore"` and `"PowerCrystal"`.
  They hold back `lego_game_config_init`, `lego_render_quality_config_apply`,
  `lego_scene_lights_build_from_config`, `lego_level_load` and the name
  loaders until their units are converted.

### 2. Adapters whose target is now converted

The two units still called 21 port adapters whose original targets are now
original-shape functions with the original's contract: Lego.c's own
`00431570` (Lego_GetMap, six call sites), `00433e80`, `00433be0`, `00433c10`
and `00435230`; LegoObject.c's `00438960`, `0043a8f0`, `0043a910`,
`00437d00`, `0043f6a0`, `004424f0`, `004471b0`, `0044a5d0` and `0044c150`;
Info.c's `00419b80`; Stats.c's `00469c10`; Weapons.c's `0046faf0` and
`0046fc70`; Lego.c's `00435aa0`; the shared no-op `00484c90`; and the getter
at `004897d0`. In the matching build they now call the original functions.
Natively the adapters are unchanged.

Each switch is one of three counter-neutral forms (see section 4):

- **an alias** (`legoobject_port_aliases.h`, or a matching-build `#define`
  just before the declaration) when the target is not otherwise declared in
  the translation unit: the declaration is renamed one for one;
- **a post-declaration macro** when the target is already declared (Info_Send,
  Stats.c's `00469c10`, LegoObject_Create through `lego_object_create_adapter`):
  the adapter's declaration keeps its name, and only the calls change;
- **an `()` function-pointer cast** where the original calls with other
  arguments than the declaration's: Lego_GetMap with none (lego_unit.c keeps
  the port's two ignored cells), the no-op with the block, and
  LegoObject_GetBlockPos for its on-map result. A cast to an unprototyped
  pointer costs no symbol; a prototyped one (`(void (*)(const int*))`) does.

One is tagged `Deviation (scoring only)`: `004897d0` is a one-line getter the
linker folded; the tree annotates it as animclone.c's `anim_clone_is_lws`,
which LegoObject.c's flock test now calls by that name.

### 3. Three real corrections

- **`0048e090` is `__CIacos`, not `__CIasin`** (`src/crt/crt.cpp`). The body
  exchanges the `fpatan` operands (`fxch st(1)`), answers `pi` and `0` at −1
  and +1 (`fldpi`, `fldz`) and reports math error 13. LegoObject.c's two
  callers (`00442480`, `00444fb0`) compute an `acos`. The `LIBRARY`
  annotations and the `/include:` pragma now name acos.obj; `00442480`
  reaches 100% and `_acos` (`0048e0ad`) rises from 68.63% to 88.46%.
- **`terrain_cell_mask_180000_current` (`004330e0`) returns 1**, not the
  block index, when the mask is set: its naked asm lacked the original's
  `mov eax, 1` path. It reaches 100%.
- **Lego.c's level-start pilot count** is Reward.c's
  `rewardGlobs.level.itemsArray[5].startCount` (`00555118`), not a separate
  word. In the matching build `s_start_level_pilot_grid_count` names it
  (`Deviation (scoring only)`: the extern views `rewardGlobs` as words, since
  naming its struct tag costs one symbol). Natively spawn.cpp's word stays.

### 4. The symbol counter decided which forms were allowed

Lego.c's counter is fragile in one region. Sweeping inert declarations
(an experiment only; nothing is committed) showed that 13 exact functions,
`004311d0`–`00433380`, keep their code at exactly one counter value: one
more or one less already costs two of them, and a shift of 12 costs 12
(rule B, finding 538). So every
change before them must leave the counter where it was. Two facts make that
practical:

- **String literals cost a symbol each** (`$SG`). Replacing `""` with
  `s_config_angle_empty` lowered the counter by one in each function that had
  used it; the two new externs raised it by two at the top. The net, measured
  per function, is +2, +1, 0 and −1 across the file, with 0 in the fragile
  region and −1 after it, which the later functions tolerate.
- **Renaming a declaration into an existing one costs a symbol** (the
  redeclaration is free). A first attempt aliased `entity_block_pos_get_adapter`
  onto a name already declared, and the region lost 12 exact functions; the
  post-declaration macro form restored them.

`tools/experiments/699/counter_probe.py` (new) prints the counter before
every function, at a git revision and in the working tree, in about four
seconds. It was the check for every change here, ahead of `make
match-function`. LegoObject.c's counter is unchanged at every function.

The same sweep, over 0–64 inert declarations in each unit, brought **none**
of the remaining 33 functions to 100%. LegoObject.c's band functions do not
move with the counter at all. Rule B is not what holds them back.

## Per-function results

The 52 functions of the band, and the two from the 80–94% band that reached
100%. Sizes are the ledger's address gaps. Letters in the last column are the
causes tallied below.

| Address | Function | Unit | Size | `main` | After | Change, or what is left |
| --- | --- | --- | ---: | ---: | ---: | --- |
| `00430530` | lego_terrain_cell_mutate | GAME-021 | 2240 | 95.45 | 95.45 | D: block layout and operand order across a 2.2 KB body (`imul` order, `or [mem]`, merged flag tests) |
| `004288e0` | lego_level_debug_input_cell_mutator_dispatch | GAME-021 | 2176 | 97.51 | 98.41 | C: register choice (`cl`/`dl`) and jump distances; its adapters are fixed |
| `004248b0` | lego_selected_entity_ground_marker_render | GAME-021 | 2032 | 99.68 | **100.00** | Lego_GetMap called directly (adapter) |
| `004438b0` | legoobject_path_step_advance_and_arrival_check | GAME-023 | 2032 | 99.66 | **100.00** | Lego.c `00433be0`/`00433c10` called directly (adapters) |
| `0043bc40` | legoobject_objective_sidecar_append_gate | GAME-023 | 1472 | 99.11 | 99.11 | A: `osf_add` has C++ linkage; `00409ff0` is unannotated |
| `00433770` | lego_scene_lights_build_from_config | GAME-021 | 1136 | 96.35 | 96.35 | A: Container.c callees (#697); `"Main"` is an earlier unit's literal |
| `00436c20` | legoobject_initialise | GAME-023 | 1072 | 95.83 | 96.67 | D: `rep stosd` scheduled against two immediates; `"Activity_Stand"` fixed |
| `0044a910` | legoobject_building_placement_preview_update | GAME-023 | 944 | 99.69 | 99.69 | A: the callee is unannotated |
| `0042f350` | lego_terrain_neighbor_coordinate_check | GAME-021 | 928 | 96.26 | 96.26 | C: register allocation (`ebp`/`edi` for the level and block array) |
| `00431da0` | lego_terrain_cell_route_and_occupancy_check | GAME-021 | 864 | 99.69 | **100.00** | LegoObject_GetBlockPos called directly (adapter) |
| `0042b510` | lego_map_load_pipeline | GAME-021 | 832 | 98.20 | 98.20 | C: `test eax, eax` against `cmp eax, ebx` (zero register) |
| `0043af30` | legoobject_creature_spawn_from_emergence | GAME-023 | 832 | 97.89 | 97.89 | C: register choice (`ecx`/`eax`) for one flag update |
| `0044b7a0` | legoobject_matrix_rotate_z_with_offset | GAME-023 | 832 | 97.83 | 97.83 | A: Container_SetMatrix has C++ linkage (#697) |
| `00428020` | lego_entity_command_status_feedback_dispatch | GAME-021 | 784 | 98.89 | 98.89 | B: end-of-array pointer names the next global; one register choice |
| `0044c1b0` | legoobject_heading_step_move | GAME-023 | 752 | 97.75 | 97.75 | E: x87 scheduling inside the inlined Vector2D normalise |
| `0042c760` | lego_load_fallin | GAME-021 | 624 | 99.45 | **100.00** | the shared no-op called with the block (adapter) |
| `00437540` | legoobject_remove | GAME-023 | 624 | 99.47 | 99.47 | A: pushes `00472060`, which nothing annotates |
| `004428a0` | legoobject_world_position_set | GAME-023 | 624 | 98.94 | 98.94 | E: a float self-copy is `fld`/`fstp` in the original, `mov` here |
| `00447c40` | legoobject_activity_register_bookkeeping | GAME-023 | 608 | 96.85 | 96.85 | D: block layout (epilogue placement) |
| `004401b0` | legoobject_terrain_cell_to_world_position | GAME-023 | 544 | 98.06 | 98.06 | D: argument pushes scheduled around a call's stack cleanup |
| `004291a0` | lego_scene_marker_update_and_draw | GAME-021 | 528 | 96.84 | 98.10 | D: stack slot assignment; its adapters are fixed |
| `00431580` | lego_terrain_neighbor_work_flag_scan | GAME-021 | 512 | 96.20 | 96.20 | C: register allocation and load scheduling of width/height |
| `00441ed0` | legoobject_face_target_action_start | GAME-023 | 512 | 98.75 | 98.75 | E: x87 scheduling (`faddp`/`fsqrt` against the pushes) |
| `0044bc30` | legoobject_motion_to_scene_node_sync | GAME-023 | 512 | 98.71 | 99.35 | A: Container_IsHidden adapter (#697); the flock getter is fixed |
| `0042bb60` | lego_texture_set_config_load | GAME-021 | 448 | 98.72 | **100.00** | pooled `""` (config.c) |
| `00434c10` | lego_message_load_infomessages | GAME-021 | 400 | 99.29 | **100.00** | pooled `":"` (config.c) |
| `0043abf0` | legoobject_terrain_cell_variant_update_queue | GAME-023 | 352 | 98.15 | **100.00** | Lego.c `00435230` called directly (adapter) |
| `0043ad50` | legoobject_predug_spawn_coordinate_select_and_create | GAME-023 | 336 | 99.02 | 99.02 | D: load/store order of a pair copy (Reccmp: effective match) |
| `004420d0` | legoobject_kind_effect_slot_init_dispatch | GAME-023 | 320 | 96.08 | **100.00** | Weapons.c `0046faf0`/`0046fc70` called directly (adapters) |
| `004230c0` | lego_debug_panel_text_draw_update | GAME-021 | 304 | 95.79 | 98.95 | C: `lea` operand order; its strings are fixed |
| `00429470` | lego_camera_pointer_ray_route | GAME-021 | 304 | 98.80 | 98.80 | E: x87 store scheduling (`fstp` slot) |
| `004477d0` | legoobject_path_step_lookahead_write | GAME-023 | 304 | 97.70 | 97.70 | D: epilogue scheduling (`pop edi` placement) |
| `0042cb70` | lego_camera_grid_expiry | GAME-021 | 288 | 96.67 | 96.67 | D: store scheduling around an `imul` |
| `00434af0` | lego_text_messages_config_load | GAME-021 | 288 | 99.02 | **100.00** | pooled `":"` (config.c) |
| `00426420` | lego_cursor_ray_to_camera_route | GAME-021 | 256 | 95.77 | 95.77 | D: stack slots of three zeroed locals |
| `004350a0` | lego_surface_type_descriptions_config_load | GAME-021 | 208 | 98.55 | 98.55 | B: end-of-array pointer names the next global |
| `0041fa80` | lego_start_level_pilots | GAME-021 | 192 | 96.08 | **100.00** | `"Pilot"` marked; the count stored in `rewardGlobs` |
| `004387f0` | legoobject_building_upgrade_commit | GAME-023 | 192 | 98.46 | **100.00** | Stats.c `00469c10` called directly (adapter) |
| `004313b0` | lego_block_activity_update_all | GAME-021 | 160 | 98.33 | **100.00** | `"Activity_Stand"` marked |
| `00437c20` | legoobject_deferred_olist_queue_add | GAME-023 | 160 | 98.04 | **100.00** | Lego_GetMap called directly (adapter) |
| `00438ae0` | legoobject_query_center_mask02 | GAME-023 | 160 | 97.78 | **100.00** | Lego_GetMap called directly (adapter) |
| `0043bb20` | legoobject_building_deposit_state_monitor | GAME-023 | 160 | 96.08 | **100.00** | comparison written as the original (`stat > carry`) |
| `00426250` | lego_update_render_viewport_state | GAME-021 | 144 | 97.37 | 97.37 | A: RadarMap_SetZoom is a C++-linkage port model |
| `00435010` | terrain_cell_work_flag_circle_set_current | GAME-021 | 144 | 99.16 | 99.16 | F: naked asm; Reccmp's original extent ends before the last `ret` |
| `00438310` | legoobject_pool_chunk_allocate | GAME-023 | 144 | 97.22 | 97.22 | B: the count is reloaded after the store: one `objectGlobs` struct |
| `00438a60` | legoobject_query_center_mask01 | GAME-023 | 128 | 97.14 | **100.00** | Lego_GetMap called directly (adapter) |
| `00425ce0` | lego_game_speed_set | GAME-021 | 112 | 96.30 | 96.30 | A: FrontEnd.c `00414f90` takes no context in the original (#696) |
| `004261c0` | lego_spawn_coordinate_pool_tick | GAME-021 | 112 | 96.15 | **100.00** | `00438960` called directly (adapter) |
| `00442480` | legoobject_vector_direction_angle_radians_get | GAME-023 | 112 | 97.14 | **100.00** | CRT label: `0048e090` is `__CIacos`, not `__CIasin` |
| `00437050` | legoobject_shutdown | GAME-023 | 96 | 96.00 | 96.00 | B: end-of-array pointer names the next global |
| `004385b0` | legoobject_shared_eligibility_check | GAME-023 | 96 | 96.00 | 96.00 | B: a flag immediate lands inside `fileRedundantList` on both sides |
| `00433e30` | lego_grid_cell_entity_eligibility_check | GAME-021 | 80 | 96.00 | **100.00** | Lego_GetMap called directly (adapter) |
| `0042b2f0` | lego_entity_effect_counters_apply_from_record | GAME-021 | 64 | 91.30 | **100.00** | LegoObject.c `0043a8f0`/`0043a910` called directly (adapters) |
| `004330e0` | terrain_cell_mask_180000_current | GAME-021 | 64 | 92.86 | **100.00** | naked asm: the original returns 1 on the true path |

Other functions that rose without reaching 100%: `lego_game_config_init`
(93.87 to 94.81), `lego_misc_object_resources_load` (94.76 to 96.19),
`lego_level_load` (90.82 to 92.42), the two name loaders (86.69 to 88.24),
`lego_entity_type_pair_lookup` (82.67 to 84.16), `lego_engine_tick` (97.87
to 98.15), `legoobject_item_name_by_kind` (87.88 to 96.97),
`legoobject_damage_apply` (93.33 to 94.44), and six functions in other units
(Reward.c's `00460360`, `004519b0`, `0046c030`, `004089d0`, `00479630`,
`_acos`), all from the literals and the acos label.

## Why the other 33 are short

| Cause | Functions | Ledger bytes | What would close it |
| --- | ---: | ---: | --- |
| **A. Callee or operand outside these units**: Container.c (#697: `00433770`, `0044b7a0`, `0044bc30`), FrontEnd.c (#696: `00425ce0`), RadarMap_SetZoom and `osf_add` with C++ linkage (`00426250`, `0043bc40`), callees nothing annotates (`0044a910`, `00437540`) | 8 | 5,776 | those conversions; then a one-line switch here |
| **B. Data layout**: an end-of-array pointer that names whichever global follows (`00428020`, `004350a0`, `00437050`), a flag immediate inside `fileRedundantList` on both sides (`004385b0`), and separate globals where the original has one `objectGlobs` struct, so a store no longer forces the count's reload (`00438310`) | 5 | 1,328 | layout identity; `objectGlobs` as one struct |
| **C. Register choice or operand order (integer)**: `004288e0`, `0042f350`, `0042b510`, `0043af30`, `00431580`, `004230c0` | 6 | 5,584 | not found: several shapes each, the permuter and a 0–64 counter sweep |
| **D. Scheduling, stack slots or block layout**: `00430530`, `00436c20`, `00447c40`, `004401b0`, `004291a0`, `0043ad50`, `004477d0`, `0042cb70`, `00426420` | 9 | 6,176 | not found |
| **E. x87 scheduling and float copies**: `0044c1b0`, `004428a0`, `00441ed0`, `00429470` | 4 | 2,192 | not found; no swap traces to declaration order (rule A) |
| **F. Naked asm against Reccmp's extent** (`00435010`) | 1 | 144 | an extent fix in the ledger |

What was tried for C–E, so a later sweep need not repeat it:

- `tools/msvc_permuter.py`, 120 s on each of 21 functions: no improvement
  (its rewrites are few: zero to nine candidates per function).
- `0043ad50`: the pair copy written as a `Point2I` struct copy; the code did
  not change.
- `0043aea0` (92%, the original reserves one more frame slot): declaration
  order, a `Point2I` or `int[2]` block position, a result variable; none adds
  the slot.
- `0042b510`: dropping the result variable from the loader tests (`if
  (f())`), with and without `return 1`/`return 0`; both score lower.
- `004477d0`: the target position stored field by field; lower.
- `0044c1b0`: the 2D normalise through a local reciprocal (the M-group
  inline); lower.
- `004428a0`: the vector add with its operands exchanged; unchanged.

### Blocked in the 80–94% band

Ten Lego.c functions (`0042cd50` and `0042cda0`–`0042d460`, the two name
loaders, `0042dfc0`, `0042dff0`, `0042e8b0`, `0042ef40`) read legoGlobs' catalogue counts,
object-name tables and record registry through `legoGlobs_config + offset`.
On the rebuilt side that lands in `bubbleGlobs`, and Reccmp names it so. The
original-address globals are entity.cpp's `s_catalog_counts`,
`s_object_name_kind*` and `s_object_pointer_registry_*`. The registry and
the counts have C++ linkage and a frozen `entity_for_lego.hpp` declares the
counts, so lego_unit.c cannot name them. Declaring the name tables (which are
`extern "C"`) would add a dozen symbols before the fragile region. These
functions recover when legoGlobs becomes one struct.

## Files

- `src/game/lego_unit.c`, `lego_unit.h`, `lego_unit_imports.h` (Lego.c's
  declarations only), `legoobject_unit.c`, `legoobject_unit_imports.h`,
  `legoobject_port_aliases.h`: the switches above.
- `src/game/lego_port.cpp`: seven `GLOBAL` annotations removed.
- `src/crt/crt.cpp`: the acos label.
- `tools/experiments/699/counter_probe.py`: the counter probe.
