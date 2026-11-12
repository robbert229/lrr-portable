# Finding 577 — Lego.c (GAME-021) part 3 rebuilt in original shape; `lego.cpp` retired to the port tail

Issue #587, part 3 of 3, under [ADR 0005](../adr/0005-original-shape-c-units.md).
It follows [finding 568](568-lego-unit-part-1-original-shape.md) (part 1) and
[finding 571](571-lego-unit-part-2-original-shape.md) (part 2). This change
appends Lego.c's last range, `004324d0`–`00435aa0`, to
[`src/game/lego_unit.c`](../../src/game/lego_unit.c). It then renames
`lego.cpp`, which now holds only port code, to
[`lego_port.cpp`](../../src/game/lego_port.cpp), a `tail` entry.

The range is 72 functions in the unit map. The real count is **73**:
Ghidra merged `00434450` (the `qsort` comparator of the projected-cell list)
into `00433e80`. Four functions had no port definition (`004334f0`,
`00433e80`, `00435300`, `00435940`); all 73 are written and annotated.

"Before" is main at `3791f0dd` (#590, finding 579). "After" is this change.
Scores come from the full matching build (`make reccmp-check`).

## Result

| | before | after |
| --- | ---: | ---: |
| Range similarity (byte-weighted, 73 functions, step 0's sizes) | 35.70% | **88.28%** |
| Exact functions in the range | 35 | **52 of 73** |
| Mapped functions in the range | 68 | 73 |
| Exact-function code bytes (share of `.text`) | 105,485 (16.42%) | **108,678 (16.91%)** |
| Accuracy, full / game-only (`binary-progress.json`) | 71.59% / 68.70% | **72.36% / 69.56%** |
| `/OPT:REF`: annotated functions removed, full (exact) | 1,303 (585) | **773 (339)** |
| ⤷ game-only (exact) | 1,266 (556) | 738 (312) |
| ⤷ `lego_unit.c` (exact) | 123 (58) | 80 (54) |
| ⤷ `lego.cpp` (exact) | 34 (22) | retired (no annotated code) |
| `/OPT:REF`: `.text` removed | 414,654 bytes | 275,486 bytes |

`lego_unit.c` now holds 73 more annotated functions, so its 80 removed
functions are out of 223 (123 of 150 before).

| Address | Function | Before | After |
| --- | --- | ---: | ---: |
| `004324d0` | `terrain_cell_secondary_flag_8_set_current` | **100.00** | **100.00** |
| `00432520` | `terrain_cell_secondary_flag_8_current` | **100.00** | **100.00** |
| `00432550` | `always_accept_place_command_cell` | **100.00** | **100.00** |
| `00432560` | `terrain_cell_flag40000000_clear_or_set_current` | **100.00** | **100.00** |
| `004325d0` | `terrain_cell_secondary_bit0_set_current` | **100.00** | **100.00** |
| `00432600` | `terrain_cell_variant_flag_propagate_current` | **100.00** | **100.00** |
| `00432710` | `terrain_cell_flag_80000_clear_or_set_current` | **100.00** | **100.00** |
| `00432770` | `terrain_cell_walkability_query_current` | 21.62 | **100.00** |
| `00432950` | `lego_entity_move_destination_cell_usable` | 36.52 | **100.00** |
| `004329d0` | `level_cell_flag8_check_target` | **100.00** | **100.00** |
| `00432a20` | `lego_terrain_cell_allows_placement` | 38.60 | **100.00** |
| `00432aa0` | `terrain_cell_occupied_or_flag_current` | **100.00** | **100.00** |
| `00432b00` | `terrain_cell_flag_800_bounded_current` | **100.00** | **100.00** |
| `00432b50` | `terrain_cell_flag_8_bounded_current` | **100.00** | **100.00** |
| `00432b90` | `terrain_cell_type_is_8_or_10_or_11_current` | **100.00** | **100.00** |
| `00432bd0` | `terrain_cell_walkable_flag_current` | **100.00** | **100.00** |
| `00432c20` | `terrain_cell_flag_wave_current` | **100.00** | **100.00** |
| `00432c50` | `terrain_cell_speed_class_current` | **100.00** | **100.00** |
| `00432c90` | `terrain_cell_state_update_current` | 19.18 | **100.00** |
| `00432d90` | `terrain_cell_flag_20_bounded_current` | **100.00** | **100.00** |
| `00432dd0` | `terrain_cell_flag_reserved_current` | **100.00** | **100.00** |
| `00432e00` | `level_tile_mark_current_target` | **100.00** | **100.00** |
| `00432e60` | `terrain_cell_mask_c0_current` | **100.00** | **100.00** |
| `00432e90` | `terrain_cell_edge_wall_current` | **100.00** | **100.00** |
| `00432ec0` | `terrain_cell_state_equals_one_current` | **100.00** | **100.00** |
| `00432f00` | `terrain_cell_state_equals_six_current` | **100.00** | **100.00** |
| `00432f40` | `terrain_cell_mask18_clear_current` | **100.00** | **100.00** |
| `00432f90` | `level_cell_flag_4_current` | **100.00** | **100.00** |
| `00432fd0` | `terrain_cell_flag_2000_current` | **100.00** | **100.00** |
| `00433000` | `terrain_cell_flag_80_current` | **100.00** | **100.00** |
| `00433030` | `terrain_cell_flag_8000_current` | **100.00** | **100.00** |
| `00433060` | `terrain_cell_secondary_bit0_current` | **100.00** | **100.00** |
| `00433090` | `level_cell_occupied_at_current` | **100.00** | **100.00** |
| `004330e0` | `terrain_cell_mask_180000_current` | 92.86 | 92.86 |
| `00433120` | `terrain_cell_flag_10000000_set_current` | **100.00** | **100.00** |
| `00433180` | `entity_cell_type_speed_multiplier_resolve_current` | 96.97 | **100.00** |
| `004332c0` | `terrain_cell_record_byte3_current` | **100.00** | **100.00** |
| `004332f0` | `terrain_cell_work_flag_set_current` | **100.00** | **100.00** |
| `00433330` | `terrain_cell_type_get_at_current` | **100.00** | **100.00** |
| `00433380` | `terrain_cell_mutation_progress` | 13.95 | 99.09 |
| `004334c0` | `level_map_dimensions_get` | **100.00** | **100.00** |
| `004334f0` | `lego_render_quality_config_apply` | — | 94.09 |
| `00433770` | `lego_scene_lights_build_from_config` | 7.99 | 96.35 |
| `00433be0` | `lego_current_level_terrain_grid_pick` | 24.00 | **100.00** |
| `00433c10` | `lego_entity_neighbor_blocked_cell_step_probe` | 18.59 | 65.45 |
| `00433e30` | `lego_grid_cell_entity_eligibility_check` | 13.43 | 96.00 |
| `00433e80` | `lego_terrain_projected_cell_state_refresh` | — | 93.84 |
| `00434450` | `lego_projected_cell_area_compare` | — | **100.00** |
| `00434480` | `lego_terrain_cell_surface_update_dispatch_level` | 33.33 | 88.73 |
| `00434530` | `terrain_grid_entry_resources_release_all_current` | 16.00 | 95.65 |
| `00434570` | `terrain_cell_drill_work_add_current` | 11.11 | **100.00** |
| `004345f0` | `lego_panels_config_entry_apply` | 37.50 | **100.00** |
| `00434710` | `lego_panel_button_layout_config_load` | 33.86 | 39.47 |
| `00434a00` | `lego_advisor_tutorial_icon_image_load` | 30.99 | 92.31 |
| `00434a50` | `lego_sound_samples_config_load` | 47.48 | **100.00** |
| `00434af0` | `lego_text_messages_config_load` | 32.50 | 99.02 |
| `00434c10` | `lego_message_load_infomessages` | 17.75 | 99.29 |
| `00434da0` | `lego_advisor_tooltip_and_sound_config_load` | 67.84 | 98.78 |
| `00434e80` | `lego_interface_spawn_request_enqueue` | 26.33 | **100.00** |
| `00435010` | `terrain_cell_work_flag_circle_set_current` | 99.16 | 99.16 |
| `004350a0` | `lego_surface_type_descriptions_config_load` | 18.45 | 98.55 |
| `00435170` | `lego_terrain_surface_type_name_get` | 40.00 | **100.00** |
| `00435190` | `lego_surface_type_configured_value_at_index` | 37.50 | **100.00** |
| `004351a0` | `media_action_code_to_sound_alias` | **100.00** | **100.00** |
| `00435230` | `lego_random_variant_cell_scatter` | 33.78 | 72.87 |
| `00435300` | `lego_tutorial_block_icons_update` | — | 58.24 |
| `00435550` | `ledger_stub_00435550_GlobalStateZeroTransitionDetect` | 75.00 | **100.00** |
| `00435580` | `lego_global_handle_position_apply_if_present` | 13.79 | 71.79 |
| `004355c0` | `lego_menu_option_panel_texture_update` | 10.53 | 58.69 |
| `00435940` | `lego_keep_running` | — | 67.11 |
| `00435a20` | `global_state_low_byte_clear_and_refresh` | 30.00 | **100.00** |
| `00435a50` | `lego_camera_registry_probe` | 33.33 | **100.00** |
| `00435aa0` | `lego_panel_selection_highlight_toggle` | 84.38 | **100.00** |

## How the work was split

Step 0 ran as in parts 1 and 2: `tools/convert_unit.py GAME-021 --stem
lego_p3 --scores baseline` in a scratch export without the unit's two
link-order entries. Its stubs were cut into three groups, each with marker
pairs in the shared files:

| Group | Range | Functions | Done by |
| --- | --- | ---: | --- |
| 3A | `004324d0`–`00433330` | 39 | parent |
| 3B | `00433380`–`00434530` | 11 | agent (stopped on an API rate limit with every function written; its work-in-progress was committed and merged) |
| 3C | `00434570`–`00435aa0` | 23 | agent (same) |

## Rule B: part 3's declarations sit after parts 1 and 2

A function's code depends on MSVC's symbol counter at the function
(finding 538, rule B), and declarations after a function do not affect it.
The groups first added their imports, declarations and stand-ins to
`lego_unit.h`, `lego_unit_imports.h` and the state area at the top of
`lego_unit.c`. The first merged build lost part 2's exact `0042c120`
(98.31%) and moved `0042c330` from 99.03% to 66.98%.

Each group's state, declarations and imports now sit directly before its
own functions in `lego_unit.c`, so the counter at every part-1/2 function
is what it was (both recovered). `lego_unit.h` gained nothing, so
`aitask.c`, which includes it, is not moved either. Natively the file builds
as C++, so part 3 opens its own `extern "C"` block, under `#ifdef
__cplusplus` so MSVC's C build does not see it. Five Lego.c data items moved
out of `lego.cpp` are defined at the very end of the file, for the same
reason.

Changes to the existing import lines are one for one. For example, `0041fb50`'s
calls to the port's name loaders became the unit's `0042d600`/`0042da20`,
and the two import lines now redeclare the unit's names. Finding 571 showed
that redeclarations count, and deleting the two lines instead cost two exact
functions.

## The block accessors (3A)

- **Shared names.** 35 of 3A's 39 functions keep the port's name: the port's
  declarations already have the original's contract. The converted callers
  reach them unchanged: `aitask.c`, `interface_unit.c`, `legoobject_unit.c`
  and `terrain_cell_objects.c`. They are compiled for MSVC x86 only, as part
  2's shared accessors are, because the level's pointer words are not
  pointers on 64-bit. Other targets keep the port models, now in
  `lego_port.cpp` under the opposite `#if`.
  - The 15 shared declarations in `level.hpp` without C linkage got
    `extern "C"` in place.
  - `level_cell_flag8_check_target` is declared only in the frozen
    `level_from_lego.hpp`. It gets a C-linkage pre-declaration in
    `level.cpp`, and the port model in `lego_port.cpp` is `extern "C"`.
  - `terrain_cell_record_byte3_current` returns `uint32_t`: the original
    zero-extends the byte.
- **`lego_` names.** `00432950` and `00432a20`: the port's functions also
  take the level. Part 2's `00431b20` called `00432a20` through an adapter;
  it now calls the unit's function, and the adapter is gone.
- **`00432770` and `00433180` take a LegoObject.** They are shared in the
  matching build only. The port's callers pass its `EntitySlot` (0x674
  bytes, not 0x40c), which that build reads at the original's offsets, as
  the port's own MSVC bodies already did.
- **Shape notes:**
  - `00432e60` matches only as `(flags & 0x40) || (flags & 0x80)`. Any
    single-mask test gives MSVC's `neg/sbb`.
  - `004330e0` has no value when the mask is set, like part 1's `0041f900`.
    The original falls off the end with the block index in `eax`.
  - `00432c90` writes the 2-bit stage as `((stage - 1) ^ flags) & 3 ^ flags`,
    the original's bit-field store.
  - `00432770` initialises the second offset table before the first (the
    original's store order). Its literal `0.0f` matches once 0049e2b8 is no
    longer annotated (see below).
  - `00432950` and `00432a20` read the level once into a local and test
    `bx > 0`, which gives the original's `jbe`.
- **Naked bodies.** Eight tiny accessors keep the port's naked code:
  `004325d0`, `00432dd0`, `00432e90`, `00432ec0`, `00433060`, `00433090`,
  `004330e0` and `00433120`.
  - In every C form tried, MSVC swaps the two base registers (`ecx`/`edx`)
    or the order of two loads. The forms were: the level read directly or
    through a local, both `imul` operand orders, a block-pointer local and a
    block-array local. A tuner tried each form top-down at the final counter
    position.
  - The closest C form is kept in a comment above each body.
  - The whole 3A block is MSVC-x86-only, so the C form for other targets is
    the port model.
  - `004325d0` matched in C until the `0041fb50` import swap moved its
    phase (to an effective match only).

## Groups 3B and 3C

Shared names (MSVC x86 only, as above): `00433380`, `004334c0`, `00434530`,
`00434570`, `004351a0`, `00435550` and `00435a20`. The rest are `lego_`
functions; their port models stay in `lego_port.cpp`.

- `00435a20` clears eight one-bit fields of `legoGlobs.flags3`: the original
  has one `and` per bit, which MSVC folds when the masks are written as
  integers.
- `00435010` keeps the port's naked body (99.16%, as before). The original
  keeps the level in a stack slot across the call and reloads the radius;
  both C forms keep the level in a register (46.73% and 28.04%).
- `00433c10` inlines `00432bd0`'s test. A `static __inline` helper scores
  67.07%, but the lint takes it for port code. The macro form with the
  coordinates in locals gives 65.45%.
- 3C's `// STRING` markers for the `"Main", "key"` paths sat above the
  pooled `"Main"`. Each now sits above its own key literal; Reccmp's
  diagnostics had failed on 14 of them.
- 3C's callees `sound_alias_registration_set`, `sound_sample_marker_parse`
  (`sound.hpp`) and the two `ui_message_text_slot_assign*`
  (`message.hpp`) got C linkage in place.

### Near misses

| Address | Score | What is left |
| --- | ---: | --- |
| `00434710` | 39.47 | the original's frame is 0x2dec bytes (0x2b74 here) and its first array 11 dwords (12): the local arrays are laid out differently |
| `00435300` | 58.24 | not tuned |
| `004355c0` | 58.69 | not tuned |
| `00433c10` | 65.45 | the inlined `00432bd0` test (above); register choice |
| `00435940` | 67.11 | not tuned |
| `00435580` | 71.79 | not tuned |
| `00435230` | 72.87 | not tuned |
| `00434480` | 88.73 | operand names (adapters) and register choice |
| `004330e0` | 92.86 | naked; operand name only, as before |
| the rest at 93–99.3 | | adapter operands and pooled literals |

## Retiring `lego.cpp`

- `git mv src/game/lego.cpp src/game/lego_port.cpp`. Its entry went from
  `legacy 004330e0` to `tail -` with `link_order.py`'s `retire_legacy`.
  `link_order.py check` and `check-map` pass. Meson and
  `tools/binary_progress_inputs.txt` follow.
- **No annotated function stays in it.** Every port model of a Lego.c
  function is unannotated (`PORT-ONLY`, as parts 1 and 2 left them). The
  tail is lint-exempt, so the ledger loses those keys.
- **Its Lego.c data moved to `lego_unit.c`**, defined at the end of the
  file:
  - `s_scroll_guard` (`004de548`) and `s_neighbor_pick_rotation`
    (`004de588`);
  - `s_lego_text_window_font`, `s_lego_crystal_count_font` and
    `s_lego_radar_zoom` (`00556124`, `00556128`, `005563f4`).

  All already had C linkage in their users' declarations.
- **The pooled `0.0f` at `0049e2b8` is no longer annotated.** It is the
  original's compiler constant, not a variable. The unit's literal `0.0f`
  now matches it: `00432770` goes from 99.35% to 100%. No other function
  moved.
- **Still annotated in `lego_port.cpp`: the eight item-name and activity
  strings** (`004a3f40`–`004a46f8`). `legoobject.cpp` (#588) reads them
  through the frozen `entity_from_lego.hpp` and `ai_from_lego.hpp`, with
  C++ linkage. Moving them needs `extern "C"` pre-declarations in
  `legoobject.cpp`, which this change does not edit. Follow-up below.
- **Rule B across the move.** `lego.cpp` holds no annotated function, so no
  function in it can drift. `0x004302b0`, `0x00430320`, `0x00432600`,
  `0x00424810` and `0x00425b40` stay exact. `legoobject_port_aliases.h`
  stays included, with `LEGORRR_LEGOOBJECT_ALIASES_NO_ADAPTERS`.

## Callers

### The original's call graph

Finding 579 measured that most original-shape code is dead under
`/OPT:REF` because WinMain reaches Lego.c only through port models:
`frameloop.cpp` calls the port's `engine_game_config_callback_registry_init`.
[`lego_port_aliases.h`](../../src/game/lego_port_aliases.h) follows #581's
pattern: matching-build-only (MSVC 5.0) macros rename 22 port entry points
to the unit's functions, in the callers that include it first. Each entry
has the original's contract. `LevelState` starts with the word image of the
original `Lego_Level`, and `BlockFile` is `struct BlockRec`. Each aliased
declaration got C linkage in place.

| Port name → unit function | Caller files |
| --- | --- |
| `engine_game_config_callback_registry_init` → `0041fa20` | `gods98/frameloop.cpp` |
| `ledger_stub_004263c0_GlobalSubsystemStateReset` → `004263c0` | `objective_unit.cpp` |
| `level_upgrade_name_strings_load` → `00427f80` | `gods98/frameloop.cpp` |
| `entity_command_status_feedback_dispatch` → `00428020` | `entity.cpp` |
| `media_min_distance_update_mode`, `media_cd_watchdog_update`, `media_audio_playback_toggle` → `004295f0`, `004297b0`, `00429810` | `message_unit.cpp`, `gods98/engine.cpp`, `objective.cpp` |
| `terrain_tile_mesh_grid_load_level`, `terrain_texture_set_config_load_level`, `terrain_placement_blocked_adjacent_cell_scan`, `terrain_cell_edge_flag_set_and_update` → `0042b330`, `0042bb60`, `00431b20`, `0042f2e0` | `terrain_grid.cpp` |
| `upgrade_name_entry_find` and the four type-table loaders → `0042cd50`, `0042cda0`–`0042d2b0` | `entity_config.cpp` |
| `terrain_neighbor_flag_set`, `terrain_cell_mutate`, `terrain_cell_initialize`, `terrain_stamp_impact_handle_current`, `terrain_cell_material_get_current` → `00430470`, `00430530`, `00430ee0`, `00431780`, `004319b0` | `level.cpp`, `ai.cpp`, `message_unit.cpp` |
| `terrain_cell_route_and_occupancy_check_current` → `00431da0` | `select.cpp` |

The first row reaches the unit's config-callback registry from WinMain,
which roots `lego_game_config_init` (`0041fb50`) and with it most of the
original's tree. That one alias accounts for most of the `/OPT:REF` drop
from 1,303 to 773. Natively nothing changes: the macros exist only under
MSVC 5.0.

### Direct switches

- `aitask.c` → `lego_first_free_neighbor_cell_pick` (`00431c70`): its
  import line is gone, and `lego_unit.h` declares the function. `0x00404280`,
  `0x00404fc0` and `0x00405c10` rise; nothing else in the TU moves.
- `panel_unit.c` → `lego_panel_selection_highlight_toggle` (`00435aa0`)
  with the original's single argument. `lego.h`'s declaration was swapped
  one for one. `0x0045adb0` rises from 80.13% to 81.47%.
- Inside the unit: `0041fb50` calls `0042d600`/`0042da20`, and `00428330`
  calls `00435170`/`00435190` (67.28% → 71.24%).

### Not switched (gaps)

`make opt-ref-callers` lists `lego_unit.c`'s 80 remaining removed functions.
Their roots:
- **`legoobject.cpp` and `legoobject_unit.c` (#588's files):**
  `entity_activity_counters_advance`, `simtick_scene_pass`, the movement
  ledger stubs, and `legoobject_remove` → `0042dfc0`;
- **`placement.cpp`:** the visited-list functions. #581 kept the alias
  header out of it (a redeclaration added an instruction to `0x004089d0`);
- **`interface_unit.c` (#589's):** `interface_do_action` calls the port's
  `interface_spawn_request_enqueue` (`00434e80`);
- **port callers with another contract:** `EngineContext`/`GameContext`
  (`camera_grid_timers`, `camera_spawn_countdown`, `object_list_load_file`,
  `ledger_stub_00425c30_TransitionControl`), and `frontend.cpp`'s
  `selected_entity_ground_marker_render`;
- **functions nothing live calls:** about 30 of the 3A accessors and part 2's
  terrain helpers.

## Entropy and drops

Against `3791f0dd`, `make reccmp-check` reports 52 improved and 5 newly
mapped. **No exact match is lost** and no drop changes an instruction count.

| Address | Function | File | Score | Label | Instructions |
| --- | --- | --- | --- | --- | --- |
| `0x0041dca0` | `interface_do_action` | `game/interface_unit.c` | 84.32 → 84.10 | drift: calls the port's `00434e80`, whose address now names the unit's function (#589's file) | +0 |
| `0x0041fb50` | `lego_game_config_init` | `game/lego_unit.c` | 93.74 → 93.64 | source changed (calls the unit's name loaders); flagged for review | +0 |
| `0x00448800` | `ledger_stub_00448800_EntityMoveToCellCoordinates` | `game/legoobject.cpp` | 31.01 → 29.46 | drift: calls the port model of `00432950` | +0 |
| `0x00449240` | `ledger_stub_00449240_EntityWalkableTargetCellRouteIssue` | `game/legoobject.cpp` | 46.15 → 41.03 | drift: same, `00432a20` | +0 |
| `0x00449980` | `entity_activity_terrain_eligibility_transition` | `game/legoobject.cpp` | 26.90 → 25.73 | drift: same | +0 |

Improved outside the range include part 2's `0x00431ab0` (98.08% → 100%,
it calls the shared `00432770`) and `0x00431450` (83.78% → 100%, the phase
after the `0041fb50` import swap). Also improved: `0x004524b0` (66.67% →
72.22%), `0x00452d60`, `0x00458670`, `0x00428330` and the three `aitask.c`
callers above.

## Reccmp lint ratchet

The ledger goes from 1,442 to 1,390 entries, updated without
`--allow-growth`. The 52 keys removed were `lego.cpp`'s unannotated
helpers, now in a tail file.

## Native gaps (`port` label)

The native port is deferred (#552). New gaps:
- **The alias header is matching-build-only.** Natively every caller above
  keeps the port's functions.
- **Group 3B's minimal adapters, which create nothing:**
  `lego_main_setup3d_adapter`, `lego_container_create_adapter`,
  `lego_roof_block_vertices_set_adapter`, the viewport getters,
  `lego_projected_quad_area_adapter` and
  `lego_entity_random_cell_step_route_adapter`.

## Follow-ups

- **The eight item-name and activity strings** move to `lego_unit.c` once
  `legoobject.cpp` (#588) declares them with C linkage; then `lego_port.cpp`
  holds no annotation at all.
- **`legoobject.cpp`'s callers of `00432950`/`00432a20`** switch with #588
  (three drift drops above).
- **`interface_unit.c`'s call to `00434e80`** switches with #589.
- **Near misses** `00434710` (local-array layout), `00435300`, `004355c0`,
  `00435940`.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox): the five labelled
  drops above against `3791f0dd`. The link-order map check passes.
- `make match-progress` regenerated the baseline, the listing digests and
  `binary-progress.json`.
- `make winapi-link-check`: OK.
- `make unit-data-offsets`, GAME-021 row:
  - `.data`: partial (298, 12 unknown);
  - `.rdata`: n/a;
  - `.bss`: partial (5, 0 unknown), gap from `004de430`.
- `make tools-test` passes, including the lint (no growth) and
  `link_order.py check`.
- `lego_unit.c` passes `gcc -std=gnu89 -Werror=declaration-after-statement
  -fsyntax-only`.
- `cmake -S . -B build && cmake --build build && ctest --test-dir build
  --output-on-failure`: 37/37 pass.

## Reproducing

    python3 tools/convert_unit.py GAME-021 --stem lego_p3 --scores baseline   # in a copy without the two GAME-021 link-order entries
    make match-function SOURCE=src/game/lego_unit.c
    make reccmp-check
    make opt-ref-callers OPT_REF_ARGS="--object lego_unit.c.obj"
    make match-progress
    make unit-data-offsets
