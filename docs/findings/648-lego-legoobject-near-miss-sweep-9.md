# Finding 648 — Near-miss sweep 9 of Lego.c and LegoObject.c under the original link mode

Issue #749, part of #388 and ADR 0005. Follows the earlier sweeps (findings
[626](626-lego-legoobject-near-miss-sweep.md),
[629](629-lego-legoobject-near-miss-sweep-2.md),
[630](630-interface-reward-terrain-weapons-near-miss-sweep.md),
[634](634-lego-near-miss-sweep-4.md),
[635](635-frontend-near-miss-sweep-5.md),
[636](636-interface-weapons-near-miss-sweep-6.md),
[644](644-lego-near-miss-sweep-7.md) and
[645](645-legoobject-near-miss-sweep-8.md)), the link-mode switch (finding
[642](642-gate-takes-the-original-link-mode.md)) and the CRT switch (finding
[643](643-crt-from-libc-and-late-members.md)).

On `main` `5f251885`, Lego.c (GAME-021, `src/game/lego_unit.c`) had 17
functions (11,184 bytes) at 95–99% and 10 (17,728 bytes) at 80–94%.
LegoObject.c (GAME-023, `src/game/legoobject_unit.c`) had 13 (7,264 bytes)
at 95–99% and 18 (8,000 bytes) at 80–94%.

The sweep re-checked first what earlier sweeps had recorded as blocked by
callees, literals or data layout. The callees were the gain. Under
`/OPT:REF` and LIBC, two of the units' callers still called port adapters
of ObjectRecall.c (GAME-030), and one called a C++ name that a C unit
cannot reach. Seven ObjectRecall.c functions are now in original shape, so
all eight of its functions score 100% (`004592a0` already did, and
`00459360` is newly mapped). The callers now call them. Measured with full `make reccmp-check` runs on `main` `5f251885` and on
this change:

| Measure | `main` `5f251885` | This change |
| --- | ---: | ---: |
| Exact functions, full / game-only | 2,348 / 2,088 | **2,356 / 2,096** (+8) |
| Mapped functions, full / game-only | 2,827 / 2,536 | 2,828 / 2,537 (`00459360`) |
| Exact-function code bytes | 319,635 (49.74%) | **321,904 (50.10%)** |
| Reccmp accuracy, full / game-only | 96.72% / 96.42% | **96.86% / 96.57%** |
| Functions that rise / fall | | 13 (one newly mapped) / **0** |
| Function-order inversions, full: adjacent / pairs | 44 / 12,029 | 43 / 12,027 |
| Function-order inversions, game-only: adjacent / pairs | 13 / 44 | 12 / 42 |
| `opt_ref` annotated removed (gate against `--noref`) | 0 | 0 |
| `.text` virtual size | 842,053 | 841,557 (the unused port adapters leave) |
| Gate link / image end | `/DEBUG /OPT:REF /OPT:NOICF`, `0x7f5000` | the same |

No exact match was lost and no function fell below the baseline, so there
are no drops to label.

Stacked on #753 (`tools/748-retire-plain-immediates`, `e699736f`, on #752),
which changes no score: its figures equal `main`'s above. Measured again
there with plain Reccmp, the same 13 functions rise, none fall, and every
figure in the table is unchanged. `make match-progress`, `make unit-data-offsets`
(no change), `make opt-ref-callers OPT_REF_ARGS=--summary` (0 removed) and
`make pe-audit` were run on the final tree, and the three generated files
are committed.

## What closed the gaps

### 1. ObjectRecall.c in original shape (GAME-030, `object_state_file.cpp`)

The port models ObjectRecall.c on the game context (`GameContext::osf*`).
The original works on a live table (`004ffe50` table, `004ffe54` count,
`004ffe58` capacity) and a shadow table (`004ffe5c`, `004ffe60`, dirty flag
`004ffe64`). Finding 642 had already given the live table its addresses and
rebuilt `004593e0` (ObjectRecall_Save) in original shape. The rest still
had the port's contract, so their callers called `PORT-ONLY` adapters:

| Address | Function | Port score | Called through |
| --- | --- | ---: | --- |
| `00459190` | ObjectRecall_StoreMiniFigure | 35.05 | LegoObject.c `0043bc40` → `legoobject_gi_osf_add_adapter` |
| `00459240` | ObjectRecall_RecallMiniFigure | 21.05 | Lego.c `0042e020`, LegoObject.c → the port's LIFO model by its own name |
| `004592e0` | ObjectRecall_Snapshot | 40.96 | Objective.c `00458be0`, `004593e0` → `objective_osf_store_adapter` |
| `00459360` | ObjectRecall_Restore | not annotated | Objective.c `00458be0` → `objective_osf_restore_adapter` (does nothing) |
| `004593d0` | ObjectRecall_IsLoaded | 10.53 | Lego.c `00429890` → `osf_read_dirty_flag_get_original` |
| `00459470` | ObjectRecall_LoadRROSFile | 20.28 | Lego.c `00429890` → `osf_read_original` |

All seven are now written in original shape in one matching-build block at
the end of `object_state_file.cpp`, in address order (`00459190` …
`00459470`, with `004593e0` moved into place). The shadow table gets
`GLOBAL` markers at `004ffe5c` and `004ffe60`. The port's context-based
functions stay the native models; their `FUNCTION` markers become
`PORT-ONLY`. Every one of the seven scores 100% at the first or second
source shape. `00459470` needed its count and capacity stores as two
statements: the chained `a = b = n` stores them in the other order.

The callers switch with object-like aliases placed before each name's only
declaration, in the matching build and in C only (`!defined(__cplusplus)`),
as finding 645 did for `00472060`. The counter does not move:
- `legoobject_unit_imports.h`: `legoobject_gi_osf_add_adapter` and
  `entity_state_override_pop`;
- `lego_unit_imports.h`: `osf_read_dirty_flag_get_original`,
  `osf_read_original` and `entity_state_override_pop`;
- `objective_unit_imports.h`: `objective_osf_store_adapter` and
  `objective_osf_restore_adapter`.

`object_recall_save` (`004593e0`) calls the new `004592e0` directly. Its
forward declaration replaced the old adapter's one for one.

`object_state_file.cpp` was compared function by function with
`match-function --source` before and after. The block stays after the
port's `osf_read`, which finding 642 found to be sensitive to the counter.
`osf_read_state_reset` (`004592a0`) and the unchanged port functions keep
their code.

**Rises:** `0043bc40` 99.33 → **100**. `00429890` lego_level_load
94.94 → 95.04 (the two OSF calls; what is left is stack slots). Objective.c's
`00458be0` 59.94 → 60.56.

FrontEnd.c also calls `00459360` through its own adapter
(`front_object_recall_restore_adapter`). That declaration is in
`frontend_unit.c`, which #750 owns, so the alias is left as a follow-up.

### 2. `00409d50` gets C linkage in the matching build (`0043bc40`)

Finding 642 rebuilt GAME-008's `00409d50` (LegoObject_DestroyBuilding's cell
pass) in original shape. However, it kept the C++ name
`unported_ledger_stubs::entity_cell_path_effects_apply_current`, which a C
unit cannot call. LegoObject.c's `0043bc40` therefore still called the
`PORT-ONLY` adapter `legoobject_gi_cell_path_effects_apply_adapter` twice.

In the matching build the original-shape definition is now `extern "C"`.
Two headers (`level.hpp`, `port_loop_ports.hpp`) also declared a global
C++-linkage function of that name, which nothing defines or calls. MSVC
rejects the namespace-scope `extern "C"` against them (C2732), so they are
declared `extern "C"` too. The native model stays C++ in its namespace.
`entity_cell_effects.cpp`'s three functions keep their code
(`match-function --source`). `legoobject_unit_imports.h` aliases the adapter
to the original. With section 1, `0043bc40` reaches 100%.

### 3. `legoobject_zero` in compares (`0043a300`, `00447ea0`)

LegoObject.c's `0.0f` compares in `0043a300` (two) and `00447ea0` (four)
compiled to an unnamed literal. The original reads `legoobject_zero`
(`static const float`, which the unit already uses elsewhere). This change
writes the variable at those six sites. `0043a300` 70.39 → 71.14 and
`00447ea0` 67.46 → 68.05, below the bands. A scan of both units' diffs
found no other `legoobject_zero` operand left unmatched.

### 4. `0042e020`'s heading keeps two multiplies (finding 645's lead)

Lego.c's `heading * 0.15915494f * 8.0f` folded into one multiply.
`(float)(heading * 0.15915494f) * 8.0f` keeps both, as in finding 645's
`00437940`. 73.59 → 73.91. Moving the computation after the
`task_field84_get_and_field88_out` call, where the original schedules it,
scores lower (69.33 in `match-function`).

## Tried, not applied

- **Spot penumbra/umbra in `004250c0`** (finding 644's open item). The
  matching build can declare `light_interface_call_slot_40/_3c` with a float
  value in place of the two adapters' declarations, at the same counter
  cost. `lego_unit.c`'s later declarations then become redeclarations: −1
  symbol each from line 8962 on. The four `00433770` calls can pass
  `0.85f`/`0.4f`/`0.0f`, which MSVC pushes as the same bit patterns. In
  `match-function` over the whole unit, `00434530` fell from 95.65 to 78.26
  and `00433380` from 100 to an effective match, for three call operands in
  a 2,688-byte function. Not applied, because balancing would need a count
  change with no source reason.
- **The type-table loaders' `ecx` at the shared `free`** (`0042cda0`,
  `0042cf50`, `0042d100`, `0042d2b0`, `0042d460`). A finding 538 phase
  sweep (`sweep.py`, N = 0–256 step 4) shows one code throughout, so the
  register is not a rule B phase. Local order, the `malloc` assignment
  outside the `if`, `(void*)` casts, and the frees moved under the
  `arrayItem` test all keep `ecx`, or score lower. Recorded as B: in the
  original both frees load members of one `legoGlobs`.
- **`00430530`'s `imul` operand order.** It is not a phase (one code over
  N = 0–128, step 1). Swapping `LEGO_2F_BLOCK`'s `width * by` to
  `by * width` changes no function in the unit: MSVC normalises the
  product.
- **`0042f6f0`'s merged work-flag tests** (`test dh, 0x84` against the
  original's separate `flags1 & 0x400`, `flags2 & 4` and reloaded
  `flags1 & 0x8000`). Finding 644 had not examined this function. The
  original reloads the block address for the third test, as if it were an
  inlined helper. A `static __inline` helper taking the position pointer
  falls to 55% (the frame and block layout change). Not pursued.
- **`004428a0`'s float self-copy** (`fld`/`fstp` against `mov`/`mov` for
  `field_2a8 + 0`): `pos.z` as `0.0`, `-0.0f`, `(float)0` and
  `legoobject_zero` leave it as `mov` or score lower.
- **`tools/msvc_permuter.py`** on `0042cda0` (180 s, 3 workers) managed 2
  attempts: one `lego_unit.c` compile takes about a minute, so the
  permuter is no use on this unit.

## Per-function results

Sizes are the ledger's address gaps. Scores are full `make reccmp-check`
runs. The letters are the causes tallied below.

### Lego.c, 95–99%

| Address | Function | Size | `main` | After | Cause of what is left |
| --- | --- | ---: | ---: | ---: | --- |
| `00430530` | lego_terrain_cell_mutate | 2240 | 95.45 | 95.45 | D: merged work-flag tests, `imul` operand order (not a phase), read-modify-write `or` |
| `004288e0` | lego_level_debug_input_cell_mutator_dispatch | 2176 | 98.41 | 98.41 | C: `cl`/`dl` |
| `0042f350` | lego_terrain_neighbor_coordinate_check | 928 | 96.26 | 96.26 | C: `ebp`/`edi` |
| `0042b510` | lego_map_load_pipeline | 832 | 98.20 | 98.20 | D: `test eax, eax` against `cmp eax, ebx` |
| `00428020` | lego_entity_command_status_feedback_dispatch | 784 | 98.89 | 98.89 | B: end-of-array pointer |
| `004291a0` | lego_scene_marker_update_and_draw | 528 | 98.10 | 98.10 | D: stack slot sharing |
| `0042cda0` | lego_entity_vehicle_type_table_load | 432 | 96.91 | 96.91 | B: `ecx` at the shared `free` (section "Tried") |
| `0042cf50` | lego_entity_minifigure_type_table_load | 432 | 96.93 | 96.93 | B: as `0042cda0` |
| `0042d100` | lego_entity_rockmonster_type_table_load | 432 | 96.93 | 96.93 | B: as `0042cda0` |
| `0042d2b0` | lego_entity_building_types_load | 432 | 96.91 | 96.91 | B: as `0042cda0` |
| `0042d460` | lego_upgrade_types_load | 416 | 96.75 | 96.75 | B: as `0042cda0` |
| `00431b20` | lego_terrain_placement_blocked_adjacent_cell_scan | 336 | 96.04 | 96.04 | D: store scheduling |
| `00429470` | lego_camera_pointer_ray_route | 304 | 98.80 | 98.80 | E: `fstp` slot |
| `0042cb70` | lego_camera_grid_expiry | 288 | 96.67 | 96.67 | D: `imul` order |
| `00426420` | lego_cursor_ray_to_camera_route | 256 | 95.77 | 95.77 | D: zero stores scheduled before the pushes |
| `004350a0` | lego_surface_type_descriptions_config_load | 208 | 98.55 | 98.55 | B: end-of-array pointer |
| `00424730` | lego_sine_phase_midpoint_blend_dispatch | 160 | 95.45 | 95.45 | B/E: `fogColourRGB - 4`; `fadd` order |

### Lego.c, 80–94%

| Address | Function | Size | `main` | After | Cause of what is left |
| --- | --- | ---: | ---: | ---: | --- |
| `00429890` | lego_level_load | 6752 | 94.94 | **95.04** | ObjectRecall originals (section 1); D: stack slots; B: `dragBoxRGB - 4` end pointer |
| `0042f6f0` | lego_cell_init_pass | 3008 | 80.99 | 80.99 | D: merged work-flag tests (section "Tried"), register choice |
| `004250c0` | lego_main_sim_step | 2688 | 91.25 | 91.25 | A: the spot penumbra/umbra adapters (section "Tried"); C: register rotation |
| `00433e80` | lego_terrain_projected_cell_state_refresh | 1536 | 94.51 | 94.51 | E |
| `0042e8b0` | lego_entity_type_pair_lookup | 1216 | 86.88 | 86.88 | C: `esi`/`edi` |
| `00425d90` | lego_terrain_pointer_command_update | 1072 | 89.26 | 89.26 | D: one more frame word |
| `0042f0c0` | lego_level_free | 544 | 86.01 | 86.01 | B: unfolded `&=` on legoGlobs flag words |
| `0042de40` | lego_viewport_target_position_clamp | 384 | 92.31 | 92.31 | D: `blockPos` load hoisted |
| `0042bf40` | lego_load_emerge_layer | 288 | 89.72 | 89.72 | D: `test dl, dl` |
| `00430df0` | lego_terrain_neighbor_refresh | 240 | 92.49 | 92.49 | C: register rotation |

### LegoObject.c, 95–99%

| Address | Function | Size | `main` | After | Cause of what is left |
| --- | --- | ---: | ---: | ---: | --- |
| `0043bc40` | legoobject_objective_sidecar_append_gate | 1472 | 99.33 | **100.00** | sections 1 and 2 |
| `00436c20` | legoobject_initialise | 1072 | 96.67 | 96.67 | D: `rep stosd` scheduling (effective match) |
| `0043f190` | legoobject_spawn_request_queue_process | 976 | 99.67 | 99.67 | B: the count reload scheduled one store early |
| `0044c1b0` | legoobject_heading_step_move | 752 | 97.75 | 97.75 | E |
| `004428a0` | legoobject_world_position_set | 624 | 98.94 | 98.94 | E: float self-copy (section "Tried") |
| `004401b0` | legoobject_terrain_cell_to_world_position | 544 | 98.06 | 98.06 | D: float argument slot |
| `00441ed0` | legoobject_face_target_action_start | 512 | 98.75 | 98.75 | E |
| `00437150` | legoobject_tool_tip_icons_load | 336 | 98.33 | 98.33 | B: end-of-array pointers |
| `0043ad50` | legoobject_predug_spawn_coordinate_select_and_create | 336 | 99.02 | 99.02 | D: pair copy order (effective match) |
| `004477d0` | legoobject_path_step_lookahead_write | 304 | 97.70 | 97.70 | D: `pop edi` scheduling |
| `00438310` | legoobject_pool_chunk_allocate | 144 | 97.22 | 97.22 | B: volatile reload scheduling (effective match) |
| `00437050` | legoobject_shutdown | 96 | 96.00 | 96.00 | B: end-of-array pointer |
| `004385b0` | legoobject_shared_eligibility_check | 96 | 96.00 | 96.00 | B: flag immediate inside `fileRedundantList` |

### LegoObject.c, 80–94%

No function in this band changed. The causes are finding 645's, which this
sweep re-checked against the new link mode: no callee, literal or layout
change applies to them.

| Address | Function | Size | `main` = After | Cause |
| --- | --- | ---: | ---: | --- |
| `004469c0` | legoobject_nearest_connected_cell_route_select | 1152 | 82.72 | D |
| `00442b10` | legoobject_movement_accumulator_apply | 1136 | 86.12 | E |
| `004403d0` | legoobject_level_debug_cell_retarget_dispatch | 992 | 86.12 | D |
| `00437940` | legoobject_deferred_olist_create_and_link | 736 | 84.17 | B |
| `0043eea0` | legoobject_simulation_queue_flush | 608 | 89.26 | D |
| `0043a120` | legoobject_cell_step_command_issue_with_jitter | 480 | 81.00 | E |
| `00447210` | legoobject_adjacent_placement_cell_search | 416 | 85.07 | C |
| `0044afb0` | legoobject_tiny_monster_spawn_and_teardown | 400 | 85.71 | D |
| `0043fe70` | legoobject_route_to_explicit_target | 384 | 82.39 | D |
| `0043fff0` | legoobject_route_marker_setup | 320 | 85.37 | D |
| `0043f6a0` | legoobject_damage_apply | 304 | 94.44 | C |
| `004492b0` | legoobject_cell_step_command_issue_with_target_mark | 304 | 84.62 | D |
| `0044bf00` | legoobject_timed_callback_list_update | 208 | 87.14 | B |
| `0044a200` | legoobject_type_component_get | 160 | 91.49 | D |
| `0043aea0` | legoobject_emergence_random_spawn_attempt | 144 | 92.00 | D |
| `004372a0` | legoobject_spawn_coordinate_pool_clear | 96 | 91.67 | D |
| `0043c3e0` | legoobject_type26_command_completion_gate | 96 | 84.85 | C |
| `00439950` | legoobject_tool_name_strings_load | 64 | 92.00 | B |

### Outside the bands and the units

| Address | Function | `main` | After |
| --- | --- | ---: | ---: |
| `00459190` | object_recall_store_minifigure (ObjectRecall.c) | 35.05 | **100.00** |
| `00459240` | object_recall_recall_minifigure | 21.05 | **100.00** |
| `004592e0` | object_recall_snapshot | 40.96 | **100.00** |
| `00459360` | object_recall_restore | not mapped | **100.00** |
| `004593d0` | object_recall_is_loaded | 10.53 | **100.00** |
| `004593e0` | object_recall_save | 97.56 | **100.00** |
| `00459470` | object_recall_load_rrosfile | 20.28 | **100.00** |
| `00458be0` | objective_tick_target (Objective.c) | 59.94 | 60.56 |
| `0042e020` | lego_object_list_load_file (Lego.c) | 73.59 | 73.91 |
| `0043a300` | legoobject_deposit_activity_gate | 70.39 | 71.14 |
| `00447ea0` | legoobject_type_behavior_update | 67.46 | 68.05 |

## Why the other 57 are short

Both units' bands, after this change (one function left them at 100%):

| Cause | Lego.c | LegoObject.c | Ledger bytes | What would close it |
| --- | ---: | ---: | ---: | --- |
| **A. Callee outside the unit**: Container.c's spot setters (`004250c0`) | 1 | 0 | 2,688 | retype `light_interface_call_slot_3c`/`_40` together with a counter change that has a source reason |
| **B. Data layout**: separate globals for the members of one `legoGlobs`/`objectGlobs` (the loaders' `free` register, flag words, count reloads), end-of-array pointers, a port record | 9 | 8 | 6,496 | legoGlobs and objectGlobs as single structs; layout identity |
| **C. Integer register choice** | 4 | 3 | 5,376 | not found |
| **D. Scheduling, stack slots, block layout or statement shape** | 11 | 14 | 22,800 | not found |
| **E. x87 scheduling and float copies** | 2 | 5 | 5,344 | not found |

Ledger bytes for the 57 functions: Lego.c 28,912 and LegoObject.c 13,792.
`00429890` (6,752 bytes) is counted under D and `00424730` under B.

Five LegoObject.c functions remain effective matches: `00436c20`,
`0043ad50`, `00438310`, `004372a0` and `0043c3e0`.

## Follow-ups

- **FrontEnd.c's `00459360` call.** `frontend_unit.c` declares
  `front_object_recall_restore_adapter`. An object-like alias to
  `object_recall_restore` before that declaration switches it, with no
  counter change. Left for #750's owner of that file.
- **The spot setters** need a counter change with a source reason before
  they can switch (above).

## Gate

- `make reccmp-check`: OK. 13 functions rise, one is newly mapped, none
  falls, on plain Reccmp (#753 retired the plain-immediate wrapper).
  The image-end guard, winapi link and link-order checks pass.
- `make match-progress`, `make unit-data-offsets`,
  `make opt-ref-callers OPT_REF_ARGS=--summary` and `make pe-audit` were
  run. The three generated files are committed (`unit-data-offsets`
  changed nothing).
- `make tools-test` (without `--allow-growth`) and
  `make winapi-link-check`: OK.
- Native: `cmake --build build` succeeds and ctest passes 37/37. The game
  starts with the shipped WADs and runs until the timeout.

## Files

- `src/game/object_state_file.cpp`: ObjectRecall.c's seven original-shape
  functions and the shadow table's markers (matching build); the port's
  models are tagged `PORT-ONLY`.
- `src/game/entity_cell_effects.cpp`: `00409d50`'s original-shape
  definition gets C linkage (matching build).
- `src/game/level.hpp`, `src/game/port_loop_ports.hpp`: the uncalled
  global declaration of `entity_cell_path_effects_apply_current` is
  `extern "C"`.
- `src/game/lego_unit_imports.h`, `src/game/legoobject_unit_imports.h`,
  `src/game/objective_unit_imports.h`: the aliases.
- `src/game/lego_unit.c`: `0042e020`'s heading.
- `src/game/legoobject_unit.c`: `legoobject_zero` in `0043a300` and
  `00447ea0`.
- `docs/coverage/`: the match baseline, listing digests and binary
  progress.

None of these files is owned by #748, #750, #751 or #560. Through the
headers they include, `mesh_unit.c` (#751's) and a few other C units now
see the renamed `lego_unit_imports.h` declarations. Each alias renames one
declaration for one, and those units do not call the renamed names, so
their code does not change.
