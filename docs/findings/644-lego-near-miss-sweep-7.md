# Finding 644 — Near-miss sweep 7 of Lego.c, after the caller switches

Issue #740, part of #388 and ADR 0005. Follows findings
[626](626-lego-legoobject-near-miss-sweep.md),
[629](629-lego-legoobject-near-miss-sweep-2.md),
[634](634-lego-near-miss-sweep-4.md) and the caller switches of findings
[637](637-opt-ref-reevaluation.md)–[641](641-main-unit-original-shape-winmain.md).
Lego.c (GAME-021, `src/game/lego_unit.c`) had 9.7 KB at 95–99% and 22.3 KB
at 80–94% on `main` `c46f8e97`.

Eight functions reach 100% and 16 rise in all, every one in GAME-021. None
falls. Measured with `make reccmp-check` on `main` `c46f8e97` and on this
change:

| Measure | `main` `c46f8e97` | This change |
| --- | ---: | ---: |
| New exact functions | | **8** |
| Functions that rise / fall | | 16 / 0 |
| Exact-function code bytes | 303,156 (47.18%) | **306,123 (47.64%)** |
| Reccmp accuracy, full / game-only | 96.48% / 96.20% | 96.51% / 96.24% |
| Exact annotated functions (full) | 2,259 | 2,267 |
| `opt_ref` removed / removed exact | 27 / 23 | 26 / 22 |
| Image end | `0x875000` | `0x875000` |

No exact match was lost and no function fell below the baseline, so there
are no drops to label. `make match-progress`, `make unit-data-offsets` and
`make opt-ref-callers OPT_REF_ARGS=--summary` were run on the final tree and
the generated files are committed (`unit-data-offsets` changed nothing).

## What closed the gaps

### 1. legoGlobs' type counts and object registry get C linkage (7 exact)

The type counts (`0055627c`) and the object registry (`005563b8`/`005563e0`)
are entity.cpp's `s_catalog_counts`, `s_object_pointer_registry_words` and
`s_object_pointer_registry_count`. They had C++ linkage, and the frozen
`entity_for_lego.hpp` declares them without `extern "C"`. Lego.c therefore
read them at their offsets from the stand-in `legoGlobs_config`, the same
way it read the object-name tables. Reccmp named those operands after
whatever recomp global happened to sit at the offset, such as
`frontGlobs_overwriteTitle[112]`.

entity.cpp already gives the name tables C linkage with `extern "C"`
pre-declarations placed before the frozen header that redeclares them.
This change adds the same pre-declarations for the counts and the registry:

- in entity.cpp, before `entity_for_legoobject.hpp`, which pulls in
  `entity_for_lego.hpp`;
- in `lego_port.cpp` and `interface_port.cpp`, which use them, before
  `entity_for_lego.hpp`.

No frozen header changes. None of the three C++ files has annotated
functions, so their symbol counters do not matter.

`lego_unit_imports.h` then names all fifteen globals: the twelve name
tables, the counts and the two registry words.

- **Exact:** `0042d600` and `0042da20` (the two name loaders), `0042cd50`,
  `0042ef40` and `00424d00` (`lego_engine_shutdown`, which frees the
  tables).
- **The registry:** `0042dff0` is exact. `0042dfc0` became exact too, but
  only with a deviation. In the original the count and the array are
  members of one struct, so the store `recordObjects[i] = NULL` may alias
  the count, and MSVC reloads it before the loop test. As separate globals
  they cannot alias, the loop is strength-reduced, and the score falls to
  45.45%. A volatile read of the count in that one loop gives the
  original's reload: `legoGlobs_recordObjectCountReloaded`, tagged
  `Deviation (scoring only)`. `0042e020`, which also reads the count,
  scores lower with the volatile read, so it keeps the plain name.
- **Rises:** the four type-table loaders rise 3.4 points each, and
  `0042e8b0` and `0042e020` rise slightly.

**Rule B (finding 538).** The fifteen new declarations sit at the top of
the unit, so they cost the counter 15 symbols at every function. Unbalanced,
12 exact functions dropped in the single-function loop, `00431450`
(to 78.38%, as finding 634's section 3 found for one symbol) and the
terrain-cell queries `00432170`–`00433380` among them.

`counter_probe.py` confirms that each named parameter of a prototype costs
one symbol, including in a redeclaration. `lego_unit_imports.h` declared 55
functions more than once. Nine of the later duplicates were removed:

- `objective_announce_current`, `sound_handle_stop_forward`,
  `media_cd_audio_watchdog_tick`, `entity_stats_float_at7c`,
  `entity_stats_word_get`, `level_state_stored_value_set` and
  `scatter_spawn_gate_set`, with one named parameter each;
- `rect_contains_point_half_open` and `advisor_text_animation_update`,
  with three each;
- `block_get_str_copy`, with two.

That is 15 parameters, so the counter is unchanged at all 223 functions,
and `match-function` over the whole unit shows no other code change. No
filler declaration was added.

### 2. Callers switched to originals now in C units

These switches apply in the matching build only (`_MSC_VER < 1200`).
Natively the port's names are unchanged.

| Caller | Port name called | Original now called | Form |
| --- | --- | --- | --- |
| `004334f0` lego_render_quality_config_apply | `lego_main_setup3d_adapter` | `direct3d_render_preferences_apply` (`00478330`, Main_Setup3D, `gods98/main.c` since #738) | rename of the declaration; the original reads 5 of the 7 pushed arguments. **100%** |
| `00429890` lego_level_load | `incident_message_flag_set` | `info_enable` (`00419ae0`, info_unit.c) | function-like macro after both declarations |
| `00429890` lego_level_load | `renderer_fog_color_set_original` | `lego_fog_colour_set_adapter` (`00473e10`, Container_SetFogColour, container.c's link name) | function-like macro after both declarations |

All three are counter-neutral: `counter_probe.py` reports 0 everywhere.

**Not switched:** `lego_main_sim_step`'s spot penumbra and umbra calls,
which should go to Container.c's `light_interface_call_slot_40`/`_3c`. Those
two are declared later in the unit with the port's `uint32_t` value. The
exact `00433770` calls them with float bit patterns, so retyping them would
change that function's code, and a rename would turn the later declarations
into redeclarations (−2 from `00433770` on).

### 3. Tried, no gain

- `msvc_permuter.py`, 60 s each with 3 workers, on 25 functions in the two
  bands found nothing. Most had 0 to 11 candidates (`0042f350`: 34).
- `0042f0c0` lego_level_free: the original does not fold consecutive `&=`
  on one legoGlobs flag word. An address-taken form (`*&flags &= ~x`)
  folds the same way as the plain one.

## Per-function results

Sizes are the ledger's address gaps. Scores are full `make reccmp-check`
runs.

### The 95–99% band

| Address | Function | Size | `main` | After | Cause of what is left |
| --- | --- | ---: | ---: | ---: | --- |
| `00430530` | lego_terrain_cell_mutate | 2240 | 95.45 | 95.45 | D: merged work-flag tests, `imul` operands, read-modify-write `or` |
| `004288e0` | lego_level_debug_input_cell_mutator_dispatch | 2176 | 98.41 | 98.41 | C: previous-key load hoisted, `cl`/`dl` |
| `0042f350` | lego_terrain_neighbor_coordinate_check | 928 | 96.26 | 96.26 | C: `ebp`/`edi` |
| `0042b510` | lego_map_load_pipeline | 832 | 98.20 | 98.20 | D: `test eax, eax` against `cmp eax, ebx` |
| `00428020` | lego_entity_command_status_feedback_dispatch | 784 | 98.89 | 98.89 | B: end-of-array pointer; `ecx`/`eax` |
| `004334f0` | lego_render_quality_config_apply | 640 | 99.51 | **100.00** | Main_Setup3D (section 2) |
| `004291a0` | lego_scene_marker_update_and_draw | 528 | 98.10 | 98.10 | D: stack slot sharing (finding 634, section 3) |
| `00431b20` | lego_terrain_placement_blocked_adjacent_cell_scan | 336 | 96.04 | 96.04 | D: store scheduled before the block computation |
| `00429470` | lego_camera_pointer_ray_route | 304 | 98.80 | 98.80 | E: `fstp` slot |
| `0042cb70` | lego_camera_grid_expiry | 288 | 96.67 | 96.67 | D: `imul` order |
| `00426420` | lego_cursor_ray_to_camera_route | 256 | 95.77 | 95.77 | D: zero stores scheduled before the pushes |
| `004350a0` | lego_surface_type_descriptions_config_load | 208 | 98.55 | 98.55 | B: end-of-array pointer |
| `00424730` | lego_sine_phase_midpoint_blend_dispatch | 160 | 95.45 | 95.45 | B/E: `fogColourRGB - 4`; `fadd` order |

### The 80–94% band

| Address | Function | Size | `main` | After | Cause of what is left |
| --- | --- | ---: | ---: | ---: | --- |
| `00429890` | lego_level_load | 6752 | 94.80 | 94.90 | A: `osf_read`/`osf_read_dirty_flag_get` (object_state_file.cpp, C++ with a GameContext) and `0045d7e0` (GAME-035 ledger stub, another contract); D: stack slots |
| `0042f6f0` | lego_cell_init_pass | 3008 | 80.99 | 80.99 | not examined |
| `004250c0` | lego_main_sim_step | 2688 | 91.25 | 91.25 | A: the spot penumbra and umbra calls (section 2); C: register rotation in the camera-key block |
| `00433e80` | lego_terrain_projected_cell_state_refresh | 1536 | 94.51 | 94.51 | E |
| `0042e8b0` | lego_entity_type_pair_lookup | 1216 | 84.90 | 86.88 | C: `esi`/`edi` swap between `i` and `objName` |
| `00425d90` | lego_terrain_pointer_command_update | 1072 | 89.26 | 89.26 | D: one more frame word; level load placement |
| `0042d600` | lego_object_names_load | 1056 | 89.16 | **100.00** | C linkage (section 1) |
| `0042da20` | lego_object_the_names_load | 1056 | 89.16 | **100.00** | C linkage (section 1) |
| `0042f0c0` | lego_level_free | 544 | 86.01 | 86.01 | B: unfolded `&=` on legoGlobs flag words |
| `0042cda0` | lego_entity_vehicle_type_table_load | 432 | 93.47 | 96.91 | C: `eax`/`ecx` at the shared `free` join |
| `0042cf50` | lego_entity_minifigure_type_table_load | 432 | 93.52 | 96.93 | C: as `0042cda0` |
| `0042d100` | lego_entity_rockmonster_type_table_load | 432 | 93.52 | 96.93 | C: as `0042cda0` |
| `0042d2b0` | lego_entity_building_types_load | 432 | 93.47 | 96.91 | C: as `0042cda0` |
| `0042d460` | lego_upgrade_types_load | 416 | 93.14 | 96.75 | C: as `0042cda0` |
| `0042de40` | lego_viewport_target_position_clamp | 384 | 92.31 | 92.31 | D: `blockPos` load hoisted |
| `0042bf40` | lego_load_emerge_layer | 288 | 89.72 | 89.72 | D: `test dl, dl` |
| `00430df0` | lego_terrain_neighbor_refresh | 240 | 92.49 | 92.49 | C: register rotation |
| `0042ef40` | lego_stats_level_record_count_for_stat_type | 128 | 90.57 | **100.00** | C linkage (section 1) |
| `0042cd50` | lego_upgrade_name_entry_find | 80 | 94.12 | **100.00** | C linkage (section 1) |
| `0042dfc0` | lego_object_pointer_registry_clear_match | 48 | 82.35 | **100.00** | C linkage and the reloaded count (section 1) |
| `0042dff0` | lego_object_pointer_registry_get_slot | 48 | 84.62 | **100.00** | C linkage (section 1) |

### Outside the bands

| Address | Function | `main` | After |
| --- | --- | ---: | ---: |
| `00424d00` | lego_engine_shutdown | 91.18 | **100.00** |
| `0042e020` | lego_object_list_load_file | 73.47 | 73.59 |

## Why the other 27 are short

| Cause | Functions | Ledger bytes | What would close it |
| --- | ---: | ---: | --- |
| **A. Callee outside the unit**: OSF reads and `0045d7e0` (other contracts, C++); Container.c's spot setters (declaration type, section 2) | 2 | 9,440 | GAME-030/GAME-035 conversions; retype `light_interface_call_slot_3c`/`_40` together with a counter fix after `00433770` |
| **B. Data layout**: end-of-array pointers, legoGlobs flag words as separate globals | 4 | 1,696 | legoGlobs as one struct |
| **C. Integer register choice** | 9 | 6,704 | not found |
| **D. Scheduling, stack slots or statement shape** | 9 | 6,224 | not found; `004291a0` as finding 634 |
| **E. x87 scheduling** | 2 | 1,840 | not found |
| Not examined | 1 | 3,008 | a later sweep |

## Follow-ups

- **Interface.c and HelpWindow.c can now name `s_catalog_counts`.**
  Interface.c's five readers (finding 636's B cause) and HelpWindow.c's
  `helpWindow_typeCounts` still read the counts at offsets. Each unit
  needs the same counter balancing as section 1. Natively this also fixes
  finding 636's follow-up: Interface.c's vehicle and building loops would
  then read counts the port actually writes.

## Gate

- `make reccmp-check`: OK. 16 functions rise and none falls.
- `make match-progress`, `make unit-data-offsets` and
  `make opt-ref-callers OPT_REF_ARGS=--summary` were run, and the generated
  files are committed.
- `make tools-test` (without `--allow-growth`), `make winapi-link-check`
  and `make pe-audit`: OK.
- Native: `cmake --build build` succeeds and ctest passes 37/37. The game
  starts with the shipped WADs.

## Files

- `src/game/lego_unit_imports.h`:
  - the fifteen C-linkage names and the reloaded count (section 1);
  - nine duplicate prototypes removed;
  - the `info_enable` and fog-colour switches.
- `src/game/lego_unit.c`: the Main_Setup3D rename and the reloaded count in
  `0042dfc0`.
- `src/game/entity.cpp`, `src/game/lego_port.cpp`,
  `src/game/interface_port.cpp`: the `extern "C"` pre-declarations. None of
  these files is owned by another open thread.
- `docs/coverage/`: the match baseline, listing digests and binary
  progress.
