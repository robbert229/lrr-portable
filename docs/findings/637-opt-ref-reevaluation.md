# 637. `/OPT:REF` re-evaluated: 164 dropped functions, `/OPT:NOICF`, and when the gate can switch

Issue #562, a follow-up of findings 576 and 579. Measured on `main` at
`7835ce86`, from one full matching build in one worktree. ADR 0006 decision 2
lets the gate take the original's link mode (no `/DEBUG`, so `/OPT:REF`) only
when no score drops. When #562 was filed, a scratch `/OPT:REF` relink lost
1,102 annotated functions (420 exact). This finding measures what it loses
now, estimates what the gate would score, and recommends what to do. **The
gate is not changed.**

## Summary

1. **`/OPT:REF` now drops 164 annotated functions, 127 of them exact**
   (`binary-progress.json`'s `opt_ref`), 33,324 bytes. 16 of them (15 exact)
   are not unreferenced at all.
   - LINK 5.0 applies `/OPT:ICF` along with `/OPT:REF`, and folds them into
     byte-identical functions of other objects. An example is
     `engine_memory_free` (`0x402210`), folded into a `radarmap.c` copy.
   - **The original did no folding.** It keeps byte-identical functions
     apart: `0x402210` and `0x47ff40` are both `push [esp+4]`, `call free`,
     `add esp, 4`, `ret`, and `0x47ff30` is the same shape around `malloc`.
     This fits finding 576's reading that its objects had no `/Gy`, so there
     were no COMDATs to fold.
   - `/OPT:REF /OPT:NOICF` drops **148 (112 exact)**. That is the figure that
     matters for a switch.
2. **Why the 148 have no live caller.** Each is traced to its *roots*, the
   topmost removed callers, and then to the live original caller above them.
   - **146 (110 exact) hang below a live original function whose rebuild
     calls a port substitute instead.**
   - 2 (2 exact) are unreferenced in the original as well. This is the
     original's own floor.
   - None is caused by an unconverted or unmapped caller, and none is reached
     only from a data table.
   - Six live callers account for 122 of the 146:
     - `WinMain`: 35;
     - `lego_game_config_init`: 23;
     - `lego_engine_tick`: 23;
     - `legoobject_update_callback` and its sibling: 25;
     - `lego_main_sim_interaction_and_cell_update`: 11;
     - the NERPs functions: 11.
3. **Estimated score of the gate in the original's mode.** These are real
   Reccmp runs on `/DEBUG` relinks of the same objects. They are not
   estimates.

   | | Gate (`/DEBUG`) | `/DEBUG /OPT:REF` | `/DEBUG /OPT:REF /OPT:NOICF` |
   | --- | ---: | ---: | ---: |
   | Mapped (full) | 2,656 | 2,490 | **2,508** |
   | Exact (full) | 2,112 | 1,941 | **2,003** |
   | Exact-function code bytes | 279,887 (43.56%) | 255,637 (39.78%) | **267,443 (41.62%)** |
   | Accuracy, full / game-only (mean over mapped) | 95.38 / 94.98 | 95.59 / 95.19 | 95.66 / 95.27 |
   | Annotated functions lost | | 166 (129 exact) | 148 (112 exact) |
   | Surviving functions lower / higher | | 77 / 3 | **4 / 3** |

   - The mean accuracy rises only because the lost functions leave the
     average. If they count as 0, it is about 90.33% (full) and 89.66%
     (game-only) under `/OPT:NOICF`. That figure is estimated from the gate's
     scores.
   - `/OPT:ICF` also lowers 73 more functions, the callers of the folded
     ones, because their call targets now resolve to the other copy.
     `/OPT:NOICF` avoids this.
4. **The two-link decision of finding 576 still holds.** At equal `/OPT`
   settings, the `/DEBUG` and no-`/DEBUG` links have the same section RVAs
   and sizes, the same entry point, and the same symbol set. Only 110
   `.rdata` symbols move, by 80 or 88 bytes, which is the 84-byte debug
   directory.
5. **Recommendation: do not switch now.** A switch would cost 112 exact
   functions and 1.94 points of exact-function code bytes, against ADR 0006
   decision 2.
   - When the gate does switch, it should take `/DEBUG /OPT:REF /OPT:NOICF`,
     not plain `/OPT:REF`.
   - The switch condition is `opt_ref` under `/OPT:NOICF` reaching the floor,
     2 functions today, with no survivor scoring lower. The six caller
     rewires in [What would bring it to the floor](#what-would-bring-it-to-the-floor)
     cover 122 of the 146.
6. **Tooling:** `make opt-ref-callers OPT_REF_ARGS=--summary` produces the
   per-unit and per-cause table below in about 2 s. Two other changes to the
   `callers` report:
   - It now counts removal by (name, object), as `opt_ref` does. Before, it
     missed the 16 folded functions, which keep their name in the relink's
     map.
   - It counts a function whose code holds a pointer to the callee as a
     caller. That traces callbacks: the DirectDraw enumeration callbacks, the
     init dialog procedure, and the weapons and interface search callbacks.

## The 164, by owning unit and cause

`make opt-ref-callers OPT_REF_ARGS=--summary` on `7835ce86`. That is the
default relink, so `/OPT:REF` with LINK's implicit `/OPT:ICF`. Each cell
gives functions (exact). A function counts once, under the first cause that
applies to it, in column order.
- **folded:** `/OPT:ICF` folded it into another object's identical function.
- **live caller:** a root's original caller is live, but the rebuilt caller
  calls a port substitute.
- **unmapped caller:** a root's original caller is unannotated or not
  mapped.
- **data reference:** a root is referred to only from a data table.
- **unreferenced:** nothing in the original calls a root or points at it.

Units in brackets have no original file name; the rebuilt source is shown instead.

| Unit | Removed | Exact | Bytes | Folded | Live caller | Unmapped caller | Data reference | Unreferenced |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| GAME-050 [weapons.c] | 27 | 19 | 8,096 | 0 | 27 (19) | 0 | 0 | 0 |
| GAME-021 Lego.c | 19 | 13 | 5,424 | 3 (2) | 16 (11) | 0 | 0 | 0 |
| GAME-020 Interface.c | 15 | 10 | 3,792 | 0 | 15 (10) | 0 | 0 | 0 |
| GAME-023 LegoObject.c | 14 | 13 | 2,528 | 2 (2) | 11 (10) | 0 | 0 | 1 (1) |
| GAME-019 Info.c | 11 | 11 | 1,424 | 0 | 11 (11) | 0 | 0 | 0 |
| G98-010 DirectDraw.c | 9 | 9 | 2,192 | 0 | 9 (9) | 0 | 0 | 0 |
| G98-014 Files.c | 8 | 8 | 928 | 3 (3) | 5 (5) | 0 | 0 | 0 |
| LATE-002 [init.c] | 8 | 8 | 2,592 | 0 | 8 (8) | 0 | 0 | 0 |
| G98-005 Main.c | 4 | 1 | 784 | 0 | 4 (1) | 0 | 0 | 0 |
| GAME-026 [terrain_map.c] | 4 | 3 | 1,072 | 0 | 4 (3) | 0 | 0 | 0 |
| GAME-028 [tutorial_script.c] | 4 | 3 | 368 | 1 (1) | 3 (2) | 0 | 0 | 0 |
| GAME-002 AITask.c | 3 | 3 | 192 | 1 (1) | 2 (2) | 0 | 0 | 0 |
| G98-002 Container.c | 2 | 2 | 64 | 1 (1) | 1 (1) | 0 | 0 | 0 |
| G98-006 [config.c] | 2 | 2 | 112 | 0 | 2 (2) | 0 | 0 | 0 |
| G98-013 Input.c | 2 | 0 | 352 | 0 | 2 (0) | 0 | 0 | 0 |
| G98-015 Mesh.c | 2 | 2 | 176 | 1 (1) | 1 (1) | 0 | 0 | 0 |
| G98-016 Flic.c | 2 | 2 | 32 | 2 (2) | 0 | 0 | 0 | 0 |
| G98-028 [file_list_streams.cpp] | 2 | 1 | 176 | 0 | 2 (1) | 0 | 0 | 0 |
| GAME-013 [terrain_cell_objects.c] | 2 | 1 | 384 | 0 | 2 (1) | 0 | 0 | 0 |
| GAME-025 [loading_progress.cpp] | 2 | 0 | 544 | 0 | 2 (0) | 0 | 0 | 0 |
| GAME-052 Construction.c | 2 | 0 | 896 | 0 | 2 (0) | 0 | 0 | 0 |
| IMP-002 ddraw.lib | 2 | 2 | 12 | 0 | 2 (2) | 0 | 0 | 0 |
| CRT-046 vfprintf.c | 1 | 1 | 64 | 0 | 1 (1) | 0 | 0 | 0 |
| CRT-049 fscanf.c | 1 | 1 | 32 | 0 | 1 (1) | 0 | 0 | 0 |
| CRT-053 cvt.c | 1 | 1 | 48 | 0 | 1 (1) | 0 | 0 | 0 |
| G98-007 [maths.c] | 1 | 1 | 16 | 1 (1) | 0 | 0 | 0 | 0 |
| G98-012 [avi_player.cpp] | 1 | 1 | 16 | 0 | 1 (1) | 0 | 0 | 0 |
| G98-019 [draw.c] | 1 | 1 | 32 | 0 | 1 (1) | 0 | 0 | 0 |
| G98-024 [g98_024.c] | 1 | 0 | 32 | 0 | 1 (0) | 0 | 0 | 0 |
| G98-025 AnimClone.c | 1 | 1 | 16 | 1 (1) | 0 | 0 | 0 | 0 |
| GAME-007 Collision.c | 1 | 0 | 144 | 0 | 1 (0) | 0 | 0 | 0 |
| GAME-024 LightEffects.c | 1 | 1 | 80 | 0 | 1 (1) | 0 | 0 | 0 |
| GAME-027 Message.c | 1 | 1 | 64 | 0 | 1 (1) | 0 | 0 | 0 |
| GAME-030 [object_state_file.cpp] | 1 | 0 | 176 | 0 | 1 (0) | 0 | 0 | 0 |
| GAME-032 Panel.c | 1 | 1 | 32 | 0 | 1 (1) | 0 | 0 | 0 |
| GAME-035 [level_id_remap.cpp] | 1 | 0 | 32 | 0 | 1 (0) | 0 | 0 | 0 |
| GAME-043 [entity_stats.c] | 1 | 1 | 48 | 0 | 1 (1) | 0 | 0 | 0 |
| GAME-046 ToolTip.c | 1 | 1 | 32 | 0 | 1 (1) | 0 | 0 | 0 |
| IMP-003 avifil32.lib | 1 | 1 | 128 | 0 | 0 | 0 | 0 | 1 (1) |
| LATE-006 strupr.c | 1 | 1 | 192 | 0 | 1 (1) | 0 | 0 | 0 |
| **Total** | **164** | **127** | **33,324** | **16 (15)** | **146 (110)** | **0** | **0** | **2 (2)** |

By link-order kind, the annotated functions removed are 153 in `unit` objects
(32,064 bytes) and 5 in `legacy` objects (832 bytes). The rest are LIBC
members and import thunks. Under `/OPT:REF /OPT:NOICF` the table is the same
without the folded column, so 148 (112 exact).

### The folded 16

Each is a 16- or 32-byte function that has a byte-identical twin in another
object. The relink's map lists the name under the twin's object.

| Function | Original | Folded into |
| --- | --- | --- |
| `engine_memory_free`, `engine_heap_free_wrapper` | `0x402210`, `0x47ff40` | `radarmap.c` (`ui_render_record_free`) |
| `file_malloc` | `0x47ff30` | `aitask.c` (`engine_memory_allocate`) |
| `file_get_system`, `mesh_get_group_count` | `0x47fe20`, `0x482210` | `container.c` |
| `noop_callback`, `lego_terrain_cell_palette_resource_at_grid_position`, `always_accept_place_command_cell`, `flic_opcode25_success_stub`, `flic_opcode27_fallback_noop` | | `lego_port.cpp` |
| `legoobject_grid_record_count_tables_clear`, `legoobject_flags_contain_requested_single_bit`, `flh_magic_uu_noop` | | `legoobject_port.cpp` |
| `Container_Frame_GetName` | `0x476310` | `container_port.cpp` |
| `rng_thunk_draw15` | `0x4798a0` | `level.cpp` |
| `anim_clone_frame_count_callback` | `0x4898e0` | `savegame.cpp` |

### The floor: 2

- `legoobject_global_animation_state_get` (`0x438390`, 16 bytes, exact).
  The original has no direct call to it and no 32-bit pointer to it anywhere
  in the image.
- `_AVIFileInit@0` (`0x49b6ec`, the avifil32 import thunk). The original has
  no call or pointer to the thunk.

These two can only stay with `/OPT:NOREF`, or with objects compiled without
`/Gy` (#553). The original presumably kept them that way. They are the only
removed functions that the original itself never references. Finding 576's
242 unreferenced original functions are the wider floor; the rest of them
are not removed today.

## Where the 146 hang: the live callers

These are grouped by the live original caller above the roots, under
`/OPT:REF /OPT:NOICF`. A function can hang under more than one caller.
"New" counts the functions that an earlier row has not already counted.

| Live original caller (rebuilt file) | What the rebuild calls instead | Functions (exact) | New | Roots |
| --- | --- | ---: | ---: | --- |
| `WinMain` `0x4777a0` (`platform/winapi/entry.cpp`) | `winmain_application_run`, the port's start-up | 35 (29) | 35 (29) | `init_initialise` (Init.c: the mode dialog, DirectDraw enumeration and its 3 callbacks, `directdraw_setup`, the ddraw thunks, `direct3drm_device_initialize`), `directdraw_shutdown`, `block_text_token_pool_init`/`destroy`, `startup_config_stream_load` (Files.c's directory scan and `fscanf`), `input_direct_initialize`/`shutdown`, `avi_initialise`, `surface_render_setup`, `level_map_slots_reset`, `__strupr` |
| `lego_game_config_init` `0x41fb50` (`lego_unit.c`) | `lego_port.cpp`'s `*_original` copies and `info_port.cpp` | 23 (18) | 23 (18) | `lego_render_quality_config_apply`, `lego_scene_lights_build_from_config` (and `light_effects_initialise`), the panel, sound, text, advisor and surface config loaders, `lego_message_load_infomessages` (and Info.c's setters), `info_initialise`, `ledger_stub_0044db50_KeyValueEqualsFileLoad` |
| `lego_engine_tick` `0x4232e0` (`lego_unit.c`) | `lego_port.cpp` and `info_port.cpp` | 23 (17) | 23 (17) | `weapons_update` (and its projectile chain), `lego_tutorial_block_icons_update`, `lego_camera_registry_probe`, `nerps_draw_tutorial_icon`, `info_draw`, `info_draw_panel`, `info_update_timers`, `electric_fence_update` |
| `legoobject_update_callback` `0x43c810` and `legoobject_type26_command_completion_gate` `0x43c3e0` (`legoobject_unit.c`) | `weapons_port.cpp` | 25 (17) | 19 (13) | `weapons_object_update_weapons` (the weapon-model fire and aim chain, the lazer mesh), `weapons_object_pair_ray_hit`, `weapons_object_can_be_hit`, `weapons_object_in_range` |
| `lego_main_sim_interaction_and_cell_update` `0x426520` (`lego_unit.c`) | `interface_port.cpp` and `lego_port.cpp` adapters | 11 (5) | 11 (5) | `interface_do_mouse` (and its hit-test callbacks, `tooltip_reset_timer`), `legoobject_building_placement_preview_update` (`construction_spawn_barriers`, `construction_zone_start_building`, `stats_table_word_at_d0`) |
| NERPs: `nerps_object_count` `0x455300`, `nerpfunc_set_level_fail` `0x454b80`, `nerps_runtime_update_camera` `0x453da0`, `nerps_monsters_destroyed_reset` `0x454890` (`tutorial_script.c`) | no call at the site (hook tables) | 11 (9) | 11 (9) | Interface.c's item flash and click setters and getters, `interface_set_advisor_item`, `nerps_block_pointer_enumerate` (and two AITask.c task records) |
| 16 smaller callers, 1 to 3 functions each, and the floor | | | 26 (21) | `entity_placement_commands_build_once` → three `legoobject_query_center_mask*`; `entity_cell_path_effects_apply_current` → two `lego_terrain_cell_*` clears; `rotated_pairs_scratch_fill` (`placement_footprint.cpp`) → terrain-placement checks; `entity_type_config_resource_row_load` and `lego_vehicle_load_adapter` → `legoobject_poly_variant_section_model_build` (and two `terrain_map.c` model records); `lego_engine_shutdown` → `0x44dd40` (and `game_file_print_format`, `_vfprintf`); `lego_level_load` → `0x45d7e0` (and `message_lookup_physical_name`); `lego_main_sim_step` → `legoobject_movement_input_accumulate`; two AITask.c adapters; `legoobject_deferred_olist_create_and_link` → `nerps_monster_destroyed`; `renderer_window_mode_apply` → `window_runtime_adjust_rect`; `front_save_write_save_file` → `osf_write`; `__cftoe`/`__cftof` → `string_prefix_reserve`; and the 2 of the floor |

## What would bring it to the floor

In order of yield. Each step is a caller rewire or conversion. None needs a
link change, apart from the last.

1. **Main.c's `WinMain` (G98-005), 35 (29 exact).** `entry.cpp` runs the
   port's start-up and shutdown. The original start-up runs Init.c's mode
   selection, the DirectDraw enumeration, Config.c's token pool, the start-up
   streams, the input, AVI and draw set-up, and `__strupr`. Converting
   `WinMain`'s body to the original's call sequence revives all of these.
   It also brings back Files.c's `fileRedundantList`
   ([below](#interaction-with-the-0x800000-image-end-and-447)).
2. **Lego.c's `lego_game_config_init` and `lego_engine_tick`, 46 (35 exact).**
   Both are in `lego_unit.c`, which #720's sweep owns, so this change does not
   touch them. They call `lego_port.cpp`'s `*_original` copies, and
   `info_port.cpp`'s, at the original's call sites. The unit already has its
   own original-shape versions; switching the calls to them revives them.
   Rows where the substitute is an adapter with another contract (finding
   579's class (c)) wait for that adapter's own unit.
3. **LegoObject.c's `legoobject_update_callback`, 19 new (13 exact).** It
   calls `weapons_port.cpp`'s models instead of Weapons.c's
   `weapons_object_*`. This is in `legoobject_unit.c`; the target functions
   are in `weapons.c`, which #722 owns.
4. **`lego_main_sim_interaction_and_cell_update`, 11 (5 exact)**: Interface.c's
   `interface_do_mouse` and LegoObject.c's placement preview. Also in
   `lego_unit.c` (#720).
5. **NERPs, 11 (9 exact).** `tutorial_script.c` drives the port interface's
   hook tables. Finding 579 lists these as class (c).
6. **The rest, 26 (21 exact)**, in 16 callers that are mostly port models
   (finding 579's (c) list).
7. **`/OPT:NOICF`** in the gate's eventual link, for the 16 folded functions.

Steps 1 to 5 cover 122 of the 146. After all of them, the floor is 2, and
the switch can then be made in a link-only PR (ADR 0006 entropy rule 1).

## The four survivors that score lower

Under `/DEBUG /OPT:REF /OPT:NOICF`, four functions score lower:
- `screen_effects_tick` (`0x4075a0`): 98.32 → 95.38;
- `lego_level_load` (`0x429890`): 93.96 → 93.87;
- `legoobject_flag_gated_callback_b` (`0x445df0`): 91.44 → 87.84;
- `legoobject_flag_gated_callback_a` (`0x4460f0`): 89.69 → 85.57.

In `screen_effects_tick`, the original's
`test dword ptr [esi+0x3e0], 0x600000` stays a plain constant. The rebuild's
names a global instead (`s_call_rect1_set (DATA)`): the relinked `.data`
starts at `0x513000`, so a port `.bss` symbol now covers `0x600000`. This is
finding 584's artifact, but at a value *inside* the original image
(`0x49e000`–`0x771000`). `tools/reccmp_plain_immediates.py` leaves such
values alone on purpose, because it only keeps values past the original's
image end plain.
- The two `legoobject_flag_gated_callback_*` functions lose points the same
  way: `test ..., 0x600000` reads as `s_call_rect1_set (DATA)`.
- `lego_level_load` loses 0.09 points for another reason. Its original call
  to the removed `0x45d7e0` (`ledger_stub_0045d7e0_LevelIdentifierRemapTableLoad`)
  no longer has a rebuilt name and becomes an `<OFFSETn>` placeholder. It
  recovers when that function does.

Three functions score higher:
- `legoobject_shared_eligibility_check` (`0x4385b0`): 96 → 100;
- `__cftoe` (`0x490400`): 99.22 → 100;
- `__cftof` (`0x490540`): 97.20 → 100.

The two CRT functions call LIBC's own static `_shift`, not `crt.cpp`'s
annotated stand-in `string_prefix_reserve`
([below](#interaction-with-the-0x800000-image-end-and-447)).

## The two links compared

These are the original-mode relink (no `/DEBUG`) and the same objects with
`/DEBUG`, both stripped by `strip_pe_debug.py`, at equal `/OPT` settings.
The comparison was made for both `/OPT:REF` and `/OPT:REF /OPT:NOICF`.

| | `.text` | `.rdata` | `.data` | `.idata` | `.rsrc` |
| --- | --- | --- | --- | --- | --- |
| RVA and virtual size | equal | equal RVA; 22,672 vs 22,592 | equal | equal | equal |
| Bytes that differ | 3,883 (absolute operands into `.rdata`) | 14,657 | 195 | 0 | 10 (timestamp) |

- Every one of the 10,366 map symbols is in both maps.
- 110 symbols move, all in `.rdata`: 92 by 88 bytes and 18 by 80.
- The entry point is equal (`0xfcd50`).
- `Characteristics` is `0x10B` with `/DEBUG` and `0x10F` without.

This is finding 576's result, unchanged. The decision stands: **two links**.
The gate keeps `/DEBUG` for Reccmp's PDB, and `make opt-ref-link` gives the
original mode. The alternative of relocating the debug directory (finding
576's option c) still needs the second link's map, so it gains nothing.

## Interaction with the `0x800000` image end and #447

- **Image end.**
  - The original-mode relink ends at `0x640000` today. The gate ends at
    `0x873000` and the original at `0x771000`.
  - Most of the difference is one global. Files.c's `fileRedundantList`
    (2,048,000 bytes, `<common>`) goes because all its users are removed.
    They hang below the start-up streams under `WinMain`.
  - Step 1 of the plan brings it back. The original-mode `.data` would then
    span about 3,268,240 bytes from `0x513000`, ending near `0x834000`. That
    is still about 213 KB past `0x800000`, so finding 584's wrapper stays
    necessary after a switch.
  - What keeps the image past `0x800000` is unchanged:
    - `.text` is too long: 1,096,677 bytes against 642,549, so `.data` starts
      `0x72000` later than the original's;
    - the port's `.bss`, 1,115,696 bytes (finding 627).
  - Separately, once the image is smaller than the original's, the wrapper's
    rule ("past the original image end") no longer covers every plain
    constant. [Above](#the-four-survivors-that-score-lower),
    `0x600000` costs points.
- **#447 (the CRT from LIBC, finding 608).**
  - In the original mode, `crt.cpp`'s annotated `string_prefix_reserve`
    (`0x4907c0`, a stand-in for LIBC's static `_shift`) is removed, and its
    two callers reach 100%. This is the state #447 wants.
  - The annotation itself cannot map to LIBC's static, which has no PDB
    symbol, so a switch loses that exact function either way.
  - `__strupr` (`0x49dcd0`) is removed because nothing live in the rebuild
    calls it; the original's `WinMain` does, so step 1 revives it.
  - `_vfprintf` and `_fscanf` hang below Files.c's removed print and scan
    functions, so steps 1 and 2 revive them.

## Recommendation

- **Do not switch the gate now.** Losing 148 mapped functions (112 exact) and
  1.94 points of exact-function code bytes breaks ADR 0006 decision 2, and
  so do the four lower survivors.
- **When the gate switches, use `/DEBUG /OPT:REF /OPT:NOICF`.** Plain
  `/OPT:REF` folds 16 more annotated functions and lowers 73 of their
  callers. The original did not fold.
- **Switch condition:** a `/OPT:REF /OPT:NOICF` relink removes only the floor
  (2 today, both unreferenced in the original), and a Reccmp run on the
  `/DEBUG` variant shows no survivor lower.
  - The `0x600000` case needs either a wrapper extension or a layout where
    the constant is not covered.
  - Track the condition with
    `python3 tools/opt_ref_link.py relink --flag /OPT:REF --flag /OPT:NOICF --out DIR`
    and `callers --out DIR --summary`.
- **Highest-yield work:**
  1. convert `WinMain`'s start-up and shutdown sequence (G98-005; 35);
  2. switch `lego_unit.c`'s config-init and engine-tick call sites to the
     unit's own functions (#720's file; 46);
  3. switch `legoobject_update_callback`'s weapon calls to `weapons.c`
     (19 more).

  These three reach 100 of the 146.
- **Not changed here.** `binary-progress.json`'s recorded `opt_ref` keeps
  LINK's default `/OPT:REF` (with ICF), so it stays comparable with findings
  576 and 579. Recording `/OPT:NOICF` instead is a one-line change to
  `relink`'s default flags. It would lower the recorded counts by the 16
  folded functions, and is left as a follow-up for the issue.

## Reproducing

```sh
make reccmp-check
make opt-ref-link && make opt-ref                     # 164 / 127
make opt-ref-callers OPT_REF_ARGS=--summary           # the per-unit table
python3 tools/opt_ref_link.py relink --out build-reccmp/opt-ref-noicf --flag /OPT:REF --flag /OPT:NOICF
python3 tools/opt_ref_link.py measure --out build-reccmp/opt-ref-noicf     # 148 / 112
python3 tools/opt_ref_link.py callers --out build-reccmp/opt-ref-noicf --summary --json summary.json
```

**Scoring a `/DEBUG` variant** (finding 576):
1. Run `relink --keep-debug --flag /OPT:REF [--flag /OPT:NOICF] --out DIR`.
2. Run `strip_pe_debug.py DIR/LegoRR.exe`.
3. Point `reccmp-build.yml` at `DIR`'s executable and PDB.
4. Run `tools/reccmp_full_diff.py` with and without `--nolib`.
5. Restore `reccmp-build.yml`.

Exact-function code bytes are `binary_progress.py`'s measure applied to that
report.
