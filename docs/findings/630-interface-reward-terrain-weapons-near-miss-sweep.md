# Finding 630 — Near-miss sweep of Interface.c, Reward.c, the terrain map and Weapons.c

Issue #709, part of #388 and ADR 0005. It follows finding 626 (the first
near-miss sweep). Four converted units had 28 functions at 95–99% and 36 at
80–94%:

- Interface.c (GAME-020, `src/game/interface_unit.c`);
- Reward.c (GAME-037, `src/game/reward.c`);
- the terrain map (GAME-026, `src/game/terrain_map.c`);
- Weapons.c (GAME-050, `src/game/weapons.c`).

This sweep brings 22 of the 64 to 100%, and two more Interface.c functions
from below the bands. It also records why each of the other 42 is short.

Measured on `main` `7600fb4c` with `make reccmp-check`.

| Measure | `main` | This change |
| --- | ---: | ---: |
| Exact functions in the 95–99% band (28) | 0 | **15** |
| Exact functions in the 80–94% band (36) | 0 | **7** |
| New exact functions, all units | | **24** (22 + `0041f3e0`, `0041f740`) |
| Functions that rose / fell | | 46 / **0** |
| Exact-function code bytes | 244,057 | **248,706** (+4,649) |
| Reccmp mean, full / game-only | 92.71 / 92.02 | 92.80 / 92.13 |
| `opt_ref` annotated (full): removed / exact removed | 327 / 198 | 318 / 197 |
| `/OPT:REF` relink `.bss` (`opt_ref.image.bss_bytes`) | 1,111,760 | 1,111,760 |
| Gate image end (`make pe-audit`) | `0x86d000` | `0x86d000` |

**Stacked on #713** (`4613161f`, Objective.c and Message.c converted): the
same 46 functions rise by the same amounts, none falls. Exact-function code
bytes go from 247,409 to 252,058; `opt_ref` from 269 / 168 to 262 / 163; relink
`.bss` stays 1,113,328; gate image end is `0x86e000`.

No exact match was lost and no function fell below the baseline.
`make match-progress` and `make unit-data-offsets` were run, and the three
progress files are committed (`unit-data-offsets` changed no tracked file).
`opt_ref` removes nine fewer annotated functions because Interface.c now calls
the original Lego.c, LegoObject.c and AITask.c functions; one of them was
exact, hence 198 to 197.

## What closed the gaps

Finding 626 found that the operands, not the code, held back most near
misses. That held for Interface.c and Reward.c. In the terrain map and
Weapons.c, and in Interface.c's 80–94% band, source shape mattered more than
in Lego.c.

### 1. Operands (11 functions exact)

- **Interface.c's adapters and stand-ins.** 36 adapters and stand-ins of
  `interface_callees.h` now have an original-shape target with the original's
  contract, since Lego.c, LegoObject.c, AITask.c and GAME-013
  (`terrain_cell_objects.c`) are converted.
  A block of matching-build `#define`s at the top of `interface_unit.c` gives
  each declaration its target's name:
  - functions, such as `lego_get_map` to `level_current_terrain_get`
    (`00431570`) and `lego_object_detach_all` to
    `legoobject_attached_object_list_detach` (`00439e70`);
  - `legoGlobs` fields, such as `lego_flags1` to `legoGlobs_flags1`
    (`00556ecc`) and the teleporters to `legoGlobs_f_55644c`/`_556450`/
    `_556454`;
  - Input.c's key maps, as `s_global_current_keys` and
    `s_global_previous_keys`.

  None of the target names was declared in the unit before. The block
  therefore renames declarations one for one, and `counter_probe.py` shows the
  counter unchanged at every function. The block is in the unit, not in
  `interface_callees.h`: `interface_port.cpp` includes that header and defines
  the stand-ins, so a rename there would define Lego.c's globals twice.
  Natively nothing changes.
- **Reward.c's Lego.c names.** `g_front_end_avi1`/`_avi2` become
  `legoGlobs_endGameAVI1`/`2`, and `lego_reset_level_and_exit` becomes
  `lego_exit` (`004250a0`). The same kind of block is at the top of
  `reward.c`.
- **config.c's pooled literals.** Reward.c's seven `""` become
  `s_config_angle_empty`. Interface.c's five `":"` and Weapons.c's two become
  `s_config_rgb_separator` (finding 581). Each unit declares the extern once,
  just before its first user, so the functions before it keep their counter.
  A pooled literal costs one symbol at its first use only, so in Interface.c
  the extern and the dropped literal cancel after `0041ab00`.
- **Main.c's frame interval.** The original's TextWindow_Update in Reward.c's
  scroll passes `0050593c`. `platform/winapi/entry.cpp` kept that global
  `static`, so finding 560 could not name it. It is now defined with C
  linkage (`extern "C" { float s_frame_interval_override; }`: still a
  definition, at the same counter cost). `reward.c` reads it in the matching
  build only; natively the port's `1.0f` stays.

### 2. Source shape (13 functions exact, and most of the rises)

- **`argv` arrays are 20 entries.** Interface.c's four config loaders
  (`0041ab00`, `0041ada0`, `0041afd0`, `0041b1b0`) declared `argv[4]`,
  `argv[5]` or `argv[10]`, and `parts[2]`. The original's frames
  (`sub esp, 0x54`, `0x50`, `0x64`, `0x480`) fit `argv[20]` and `parts[10]`
  exactly. With them, `0041ada0` goes from 85.96% to 98.26% and `0041afd0`
  from 78.52% to 97.16%.
- **Local tables are aggregate initialisers.** Some functions built their
  constant tables one store at a time:
  - `terrain_map_shape_tolerance_check` (`0044e8c0`): `vertexOffsets[4]` and
    `edges[6][2]`. With `= { … }` initialisers MSVC schedules the stores as
    the original does: 83.33% to 96.15%, then 98.72% with the comparison
    direction below.
  - `interface_can_lay_path` (`0041f740`): `Point2I dirs[4] = { … }` and
    indexed loops. With the mask test the right way round it goes from
    61.54% to 100%.
- **Loop forms.**
  - `interface_set_menu_items`: `for (i = 0; count > 0; count--)
    items[i++] = …`. The original decrements the address-taken `count` and
    indexes with a separate counter.
  - `interface_object_menu_check` and `interface_driven_object_menu_check`:
    `for (i = 0; i < count; i++) list[i]` in place of a walking pointer.
  - `interface_enumerate_menu_items`: its last loop has no `i = 0`; the
    function's opening `i = 0` serves it, as in the original.
- **Signedness.** `unsigned int i` in `interface_get_menu_item_type` and
  `interface_load_item_panels` (`jb`, not `jl`).
- **Branch order and statement order.**
  - `interface_handle_item_press`: object branch first, then `else return 1`.
  - `interface_draw`: the reset in the then-branch.
  - `interface_update_advisor` (87.54% to 98.58%): `found = 1` after the
    `if`/`else`.
  - `interface_do_action` case 16: cancel-path in the then-branch.
  - `interface_update`: the timer store before the state flip.
  - `interface_set_dependency_area`: `y` before `width`.
- **Comparison and conversion forms.**
  - `reward_prepare_level`: `if (0.0f == (timer = atof(…)))`, which gives the
    original's `fld 0; … fcompp`.
  - `reward_load_item_importance` and `_quota`: ternaries ending in the pooled
    `""`.
  - `reward_scroll_update`: `(cond) ? (float)-k : (float)k`, converted in each
    arm.
  - `reward_draw_item`: two `flic_animate` calls, not one with `setl`.
  - `terrain_map_shape_tolerance_check`: `z > highest`.
  - `weapons_object_center_proximity_check`: a `dist` local, so the `fsqrt`
    result is stored before the stale `dy` is popped.
- **Other.**
  - `resource_list_update_matching_key`: the formatted part name in a local.
    Container_SearchTree's constant arguments are then pushed after
    Container_FormatPartName returns.
  - `reward_do_buttons`: Reward_GetLevel's result in a variable, which gives
    `cmp eax, edi` and `xor eax, eax`.

### 3. A correction

`reward_draw_score` (`00461c90`) scans the whole score text,
`i < strlen(text)`, as the original's `dec ecx` shows. The old bound was
`strlen(text) - 1`, which skipped the last character.

### 4. The counter

`counter_probe.py` (finding 626) checked every change. Two things moved the
counter:

- **`interface_handle_item_press`'s 100% shape costs one counter step.** Of
  six equivalent forms, only the one with an `else` matches, and it advances
  the counter by one for every later function. On its own it moved
  `interface_can_lay_path` from 61.54% to 59.17% (drift).
  `interface_can_lay_path`'s own shape, found while looking at that drop,
  then reached 100%. Nothing else in the unit moved.
- **The Reward.c extern** for `s_frame_interval_override` is the last
  declaration before `reward_scroll_update`. Only that function and the ones
  after it shift.

### 5. Rule B in these units

A 0–63 inert-declaration sweep over 20 of the short functions was run as a
diagnostic only; nothing is committed. Only one has the original's code at
another phase: `terrain_map_world_z_unmodified_get` (`0044fe70`) reaches 100%
at +4. `terrain_map_block_texture_fade_set`'s `imul` order does not change
with the counter at all. `tools/msvc_permuter.py` ran for 100 s on each of 14
functions and improved none.

## Per-function results

The 64 functions of the two bands. Sizes are the ledger's address gaps.
Letters in the last column are the causes tallied below.

| Address | Function | Unit | Size | `main` | After | Change, or what is left |
| --- | --- | --- | ---: | ---: | ---: | --- |
| `00461070` | reward_format_values | GAME-037 | 928 | 99.64 | **100.00** | pooled `""` |
| `00461410` | reward_loop | GAME-037 | 896 | 98.28 | 99.31 | A: Images.c `0047dcb0` (#711), FrontEnd.c MoviePlayWait `00415700` (#707) |
| `0045f7f0` | reward_load_graphics | GAME-037 | 768 | 98.68 | 99.56 | A: Images.c `0047d9d0` (#711) |
| `0045fda0` | reward_prepare_level | GAME-037 | 768 | 96.65 | **100.00** | pooled `""`; assignment inside the `0.0f ==` test |
| `00461dd0` | reward_do_buttons | GAME-037 | 608 | 96.12 | 97.95 | A: Images.c `0047dcb0` (#711); result variable fixed the zero compare |
| `00450f30` | terrain_map_block_vertex_normals_update | GAME-026 | 592 | 98.86 | 98.86 | E: `fld`/`fadd` order of an indexed element (Reccmp: effective match) |
| `00471630` | weapons_ray_block_sweep | GAME-050 | 560 | 97.93 | 97.93 | E: `fld`/`fadd` order of two pointer operands (effective match) |
| `0044fe70` | terrain_map_world_z_unmodified_get | GAME-026 | 496 | 97.99 | 97.99 | R: rule B, the original's code at counter +4 |
| `00471d20` | weapons_model_fire_update | GAME-050 | 480 | 95.60 | 95.60 | D: stack slots of `i` and `discharge`; declaration order does not move them |
| `0045f030` | reward_initialise | GAME-037 | 448 | 98.11 | 98.11 | A: `"Crystals"`/`"Ore"` are C++-linkage globals (`mission.cpp`, `encyclopedia.cpp`) |
| `0044e6d0` | terrain_map_block_texture_fade_set | GAME-026 | 400 | 98.35 | 98.35 | C: `imul` operand order; one code at every counter value |
| `0045f290` | reward_load_item_box_image | GAME-037 | 336 | 97.98 | 97.98 | A: Images.c BitmapSurfaceLoad, SetupTrans (#711) |
| `0045f3e0` | reward_load_item_image | GAME-037 | 336 | 97.98 | 97.98 | A: as `0045f290` |
| `0044f700` | terrain_map_world_to_block_centre | GAME-026 | 272 | 96.55 | 96.55 | E: `fdiv`/`fst` scheduling |
| `0041c800` | interface_update_object_item_enabled | GAME-020 | 240 | 98.82 | **100.00** | `0041f8e0` called by its name |
| `00470540` | weapons_object_center_proximity_check | GAME-050 | 240 | 98.78 | **100.00** | `dist` local |
| `00462390` | reward_debug_keys | GAME-037 | 208 | 98.44 | **100.00** | `lego_exit` |
| `0041b300` | interface_set_block_highlight | GAME-020 | 192 | 95.16 | **100.00** | Lego_GetMap called directly |
| `0045f530` | reward_load_item_text | GAME-037 | 192 | 98.36 | 98.36 | A: `"Text"` is pooled into FrontEnd.c part 2 (#707, finding 569) |
| `0041c950` | interface_is_object_item_disabled | GAME-020 | 160 | 98.18 | **100.00** | `legoGlobs_flags1` |
| `004600a0` | reward_load_item_importance | GAME-037 | 160 | 96.36 | **100.00** | ternary with the pooled `""` |
| `00460140` | reward_load_item_quota | GAME-037 | 160 | 96.36 | **100.00** | ternary with the pooled `""` |
| `0041a7d0` | interface_set_menu_items | GAME-020 | 128 | 97.37 | **100.00** | `count > 0; count--` with an index |
| `0041b930` | interface_update | GAME-020 | 128 | 96.67 | **100.00** | store order |
| `00451bc0` | resource_list_update_matching_key | GAME-026 | 112 | 95.56 | **100.00** | part name in a local |
| `0041c8f0` | interface_is_item_disabled | GAME-020 | 96 | 96.67 | **100.00** | `legoGlobs_flags1` |
| `0041f6c0` | interface_callback_detach | GAME-020 | 96 | 96.77 | **100.00** | LegoObject.c `00439e70` called directly |
| `0041a9c0` | interface_get_menu_item_type | GAME-020 | 64 | 96.55 | **100.00** | `unsigned int i` |
| `0041d040` | interface_is_item_available | GAME-020 | 3168 | 93.72 | 94.92 | D: one inline epilogue; splitting the test moves another one (adapters fixed) |
| `0041dca0` | interface_do_action | GAME-020 | 2736 | 84.43 | 86.90 | D: MSVC cross-jumps returns the original duplicates (adapters and case 16 fixed) |
| `00460950` | reward_calculate_score | GAME-037 | 1456 | 90.86 | 90.86 | E: score loop loads `total` before the value; no statement order does |
| `0046eb80` | weapons_initialise | GAME-050 | 1360 | 86.64 | 87.14 | C: `esi`/`edi` swapped; an end-of-array pointer (`s_surface_type_names`) |
| `00461790` | reward_draw_item | GAME-037 | 1280 | 94.17 | 98.87 | A: Images.c and Flic.c (#711); two `flic_animate` calls fixed |
| `00462ad0` | reward_scroll_update | GAME-037 | 720 | 85.16 | 96.02 | A: Images.c (#711); E: one `fld`/`fadd` order |
| `0041b680` | interface_enumerate_menu_items | GAME-020 | 688 | 90.28 | 96.20 | B: the type counts are entity.cpp's C++ `s_catalog_counts` |
| `0045faf0` | reward_load_buttons | GAME-037 | 688 | 93.75 | 93.75 | A: Images.c (#711) |
| `0041ab00` | interface_load_object_items | GAME-020 | 672 | 87.17 | 91.44 | A: Images.c (#711); B: type counts; `argv[20]` fixed |
| `004502e0` | terrain_map_screen_pick | GAME-026 | 640 | 82.38 | 82.38 | E: x87 scheduling and stack slots throughout |
| `0041ada0` | interface_load_menu_items | GAME-020 | 560 | 85.96 | 98.26 | A: Images.c (#711); `argv[20]`, `parts[10]` fixed |
| `0044e8c0` | terrain_map_shape_tolerance_check | GAME-026 | 528 | 83.33 | 98.72 | D: the base register of `shape[i + 1].y`, one reload |
| `0041b490` | interface_draw | GAME-020 | 496 | 93.71 | 97.90 | A: Images.c (#711) |
| `0041c4f0` | interface_update_advisor | GAME-020 | 496 | 87.54 | 98.58 | E: MSVC rewrites `+ h * 0.5f` as `- h * -0.5f` once `-0.5f` is in the function |
| `004512d0` | terrain_map_block_vertex_normal_get | GAME-026 | 384 | 91.37 | 91.37 | E: x87 scheduling |
| `0046faf0` | weapons_projectile_create | GAME-050 | 384 | 84.50 | 84.50 | C: register choice in two struct copies |
| `00461c90` | reward_draw_score | GAME-037 | 320 | 93.64 | **100.00** | `i < strlen(text)` (a correction) |
| `00462270` | reward_advance | GAME-037 | 288 | 83.84 | 83.84 | A: FrontEnd.c part 2's save calls take no GameContext (#707) |
| `0044fab0` | terrain_map_world_normal_get | GAME-026 | 224 | 91.78 | 91.78 | E: `fsub` scheduling |
| `004629d0` | reward_scroll_add_label | GAME-037 | 224 | 93.67 | 93.67 | D: `labels` loaded before the count store |
| `0041c6e0` | interface_handle_item_press | GAME-020 | 208 | 93.51 | **100.00** | object branch first |
| `0041b1b0` | interface_load_back_button | GAME-020 | 192 | 89.09 | 96.36 | A: Images.c (#711); `argv[20]` fixed |
| `00471960` | weapons_see_through_walls_check | GAME-050 | 192 | 94.74 | 94.74 | E: subtraction scheduled around the pushes |
| `0041b3c0` | interface_get_menu_item_count | GAME-020 | 176 | 93.33 | 93.33 | B: type counts |
| `0041c440` | advisor_callback_schedule_gate | GAME-020 | 176 | 93.33 | **100.00** | `s_global_current_keys` |
| `00472080` | weapons_object_in_range | GAME-050 | 176 | 90.16 | 90.16 | E: x87 scheduling in the inlined modulus |
| `00472130` | weapons_object_in_touch_range | GAME-050 | 176 | 90.16 | 90.16 | E: as `00472080` |
| `004718d0` | weapons_object_pair_ray_hit | GAME-050 | 144 | 88.00 | 88.00 | E: subtraction scheduled around the pushes |
| `0041cea0` | interface_object_menu_check | GAME-020 | 128 | 91.11 | **100.00** | indexed loop |
| `0041a920` | interface_reset | GAME-020 | 112 | 85.19 | 85.19 | B: type counts |
| `0041ec30` | interface_draw_object_count | GAME-020 | 112 | 86.49 | 89.19 | B: the request arrays' base register; one `legoGlobs` struct (count fixed) |
| `0041b270` | interface_load_dependency_images | GAME-020 | 96 | 89.19 | 89.19 | A: Images.c (#711) |
| `0041cf20` | interface_driven_object_menu_check | GAME-020 | 96 | 94.87 | **100.00** | indexed loop |
| `0041cce0` | interface_set_dependency_area | GAME-020 | 80 | 90.00 | **100.00** | store order |
| `00462460` | reward_restart_flic | GAME-037 | 64 | 94.74 | 94.74 | A: Flic.c Flic_Setup (#711) |
| `0041f840` | interface_callback_remove_spider | GAME-020 | 48 | 92.86 | **100.00** | LegoObject.c `004489a0` called directly |

Other functions that rose:

- reached 100%: `interface_callback_can_enter` (`0041f3e0`, 92.31) and
  `interface_can_lay_path` (`0041f740`, 61.54);
- `interface_load_item_panels` (78.52 to 97.16);
- `interface_callback_hotkey` (88.61 to 91.14);
- `interface_do_mouse` (74.21 to 75.66);
- `interface_update_slide` (70.59 to 72.73);
- `interface_callback_can_pick_up` (85.86 to 86.87);
- `position_tracker_box_draw` (25.53 to 26.60).

## Why the other 42 are short

| Cause | Functions | Ledger bytes | What would close it |
| --- | ---: | ---: | --- |
| **A. Callee or operand outside these units**: Images.c and Flic.c (#711) in 14; FrontEnd.c part 2 (#707) in `00461410`, `00462270`, `0045f530`; C++-linkage literals in `0045f030` | 17 | 8,640 | those conversions; then a rename here |
| **B. Data layout**: entity.cpp's C++ `s_catalog_counts` for the vehicle and building counts (`0041b680`, `0041b3c0`, `0041a920`), the request arrays outside one `legoGlobs` struct (`0041ec30`) | 4 | 1,088 | `legoGlobs` as one struct (as finding 626) |
| **R. Rule B**: `0044fe70`, original code at counter +4 | 1 | 496 | the unit's real declaration set |
| **C. Register choice or integer operand order**: `0044e6d0`, `0046eb80`, `0046faf0` | 3 | 2,144 | not found |
| **D. Scheduling, stack slots or block layout**: `00471d20`, `0041d040`, `0041dca0`, `0044e8c0`, `004629d0` | 5 | 7,136 | not found |
| **E. x87 scheduling, operand order, constant folding**: `00450f30`, `00471630`, `0044f700`, `00460950`, `004502e0`, `0041c4f0`, `004512d0`, `0044fab0`, `00471960`, `00472080`, `00472130`, `004718d0` | 12 | 5,312 | not found |

Two near misses take more than one letter. `00462ad0` has an E residue after
its A callee, and `0041ab00` has A and B. Each is counted once, under A.

What was tried for C–E, so a later sweep need not repeat it:

- `tools/msvc_permuter.py` for 100 s on each of 14 functions: no
  improvement.
- The 0–63 phase sweep (section 5): only `0044fe70` moves to the original.
- `00471d20`: `i` and `discharge` declared in the other order. The slots
  do not move.
- `0044e6d0`: `by * map->gridWidth` in place of `map->gridWidth * by`. The
  code is unchanged.
- `00460950`: the score loop's two statements in either order, with
  `actual + value`, `+= value`, `total += 100.0f` and `100.0f + total`. The
  original order loses 5 bytes; the others are unchanged.
- `0041c4f0`: `pos.y += h * 0.5f`, `0.5f * h`, `h / 2.0f`, the back-button
  sums written with `+ 0.5f`, and the image branch first. MSVC keeps
  `fsubr` with `-0.5f` in all of them.
- `0041d040`: the can-enter test split from the `||` chain. That test then
  gets the original's inline epilogue, but MSVC takes it from case 56, and
  the score falls to 89%.
- `0041dca0`:
  - the switch wrapped in `if (!disabled)`, which cross-jumps far more
    (28%);
  - a `goto` to a trailing `return 0`, which is unchanged;
  - cases 20/21/27/28 with swapped, uncast or in-expression masks, which are
    unchanged.
- `004629d0`: four orders of the count increment and the `realloc`. None
  matches; one is worse.
- `004718d0`: the subtraction as three statements, not the inline helper.
  The code is unchanged.
- `0041ec30`: the comparison order and the declaration order. The base
  register is unchanged.
- `terrain_map_shape_flatten` (`0044ead0`, below the bands): its
  `vertexOffsets` as an initialiser. The code is unchanged.

## Follow-ups

- **`0041f8e0` is not a no-op.** `lego_unit.c` names it `noop_callback` with
  an empty body (0%). The original reads `legoGlobs.currLevel`'s crystals
  (`+0x8c`), less `+0x98` when the argument is 0. Interface.c's
  `0041c800` calls it with an argument and uses the result. Interface.c now
  calls it by name, so the matching build calls the empty body there. Fixing
  the body is a Lego.c change (#708's file).
- **#711 (Images.c, Flic.c)** owns 14 of the 17 A functions. After that
  conversion, the image and flic calls here are one rename each.
- **#707 (FrontEnd.c part 2)** owns `"Text"`, MoviePlayWait and the save
  calls in `00462270`.

## Files

- `src/game/interface_unit.c`: the alias block, `s_config_rgb_separator`, the
  shapes above. `interface_callees.h` and other shared headers are
  unchanged.
- `src/game/reward.c`: its alias block, `s_config_angle_empty`,
  `s_frame_interval_override` (matching build only), the shapes above.
- `src/game/terrain_map.c`, `src/game/weapons.c`: the shapes above and
  Weapons.c's `s_config_rgb_separator`.
- `src/platform/winapi/entry.cpp`: `s_frame_interval_override` gets C
  linkage.
- `docs/coverage/match-baseline.txt`, `match-listing-digests.txt`,
  `binary-progress.json`: regenerated by `make match-progress`.
