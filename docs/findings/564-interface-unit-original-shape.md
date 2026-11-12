# Finding 564 — Interface unit (GAME-020) rebuilt in original shape, with a port tail

Issue #545, under [ADR 0005](../adr/0005-original-shape-c-units.md). #528
relocated GAME-020 (Interface.c, text `0041a300`–`0041f870`) verbatim into
`src/game/interface.cpp`. This change rebuilds it as the C unit
[`src/game/interface_unit.c`](../../src/game/interface_unit.c), with the C
boundary header [`interface_unit.h`](../../src/game/interface_unit.h).
Interface.c is the in-game icon menu on the right of the screen: 24 menus over
74 menu items (plus one icon per vehicle or building type in the three object
menus), the sliding surround and back button, the tool tips, the advisor hint
and the dependency icons of the item under the cursor.

The port's interface is a different model: hook tables, its own state words
(shared with `frontend.cpp`, `input.cpp`, `resource.cpp` and `simtick.cpp`
through the frozen per-unit headers) and the native HUD. As with GAME-026
(finding 556) and GAME-037 (finding 560), it stays in a **port tail**: the old
file is renamed [`interface_port.cpp`](../../src/game/interface_port.cpp)
(`tail` in the link-order list) with every annotation removed, so native
callers and tests keep their behaviour.

"Before" is the *match baseline* of #558 (`887b5c56`, on `main` at
`63da6269`; #558 changes no score); "after" is this change. Scores are from the full matching build (`make reccmp-check`).

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (byte-weighted, all 98 functions, 20,583 bytes; unmapped = 0) | 20.52% | **83.71%** |
| Exact functions in the unit | 13 | **45 of 98** |
| Mapped functions in the unit | 73 | 98 |
| Accuracy, game-only (`binary-progress.json`) | 58.20% | 60.05% |
| Accuracy, full | 62.20% | 63.83% |
| Exact functions, full scope | 919 | 951 |
| Exact-function code bytes | 79,550 | 80,442 |
| Unit-relative identity | 11.95% | 12.40% |

Byte weights are the original's code bytes as the single-function loop
reports them.

| Address | Function | Bytes | Before | After |
| --- | --- | ---: | ---: | ---: |
| `0041a300` | `interface_initialise` | 854 | 44.07 | 99.08 |
| `0041a660` | `interface_init_menus` | 355 | **100.00** | **100.00** |
| `0041a7d0` | `interface_set_menu_items` | 122 | 27.45 | 97.37 |
| `0041a850` | `interface_shutdown` | 208 | 38.60 | 70.31 |
| `0041a920` | `interface_reset` | 112 | 24.59 | 85.19 |
| `0041a990` | `interface_reset_menu` | 33 | **100.00** | **100.00** |
| `0041a9c0` | `interface_get_menu_item_type` | 63 | 93.10 | 96.55 |
| `0041aa00` | `interface_init_object_items` | 250 | 8.00 | 77.78 |
| `0041ab00` | `interface_load_object_items` | 669 | 10.22 | 87.17 |
| `0041ada0` | `interface_load_menu_items` | 545 | 14.20 | 84.80 |
| `0041afd0` | `interface_load_item_panels` | 435 | 12.53 | 77.04 |
| `0041b1b0` | `interface_load_back_button` | 177 | 8.62 | 87.27 |
| `0041b270` | `interface_load_dependency_images` | 95 | 14.55 | 89.19 |
| `0041b2d0` | `command_context_post_to_hud` | 38 | **100.00** | **100.00** |
| `0041b300` | `interface_set_block_highlight` | 184 | 11.29 | 95.16 |
| `0041b3c0` | `interface_get_menu_item_count` | 175 | 27.07 | 93.33 |
| `0041b470` | `interface_menu_has_back_button` | 22 | **100.00** | **100.00** |
| `0041b490` | `interface_draw` | 482 | — | 93.71 |
| `0041b680` | `interface_enumerate_menu_items` | 378 | 48.21 | 90.28 |
| `0041b800` | `interface_callback_draw_item` | 304 | — | 47.86 |
| `0041b930` | `interface_update` | 0 | 10.00 | 96.67 |
| `0041b9b0` | `sim_status_banner_word_get` | 6 | **100.00** | **100.00** |
| `0041b9c0` | `interface_resolve_teleporter_item` | 72 | 25.45 | **100.00** |
| `0041ba10` | `interface_update_hover_sfx` | 117 | 63.77 | **100.00** |
| `0041ba90` | `simulation_global_word_set` | 10 | **100.00** | **100.00** |
| `0041baa0` | `interface_do_mouse` | 1824 | 15.26 | 72.76 |
| `0041c1c0` | `interface_get_item_at_point` | 297 | 11.49 | 49.28 |
| `0041c310` | `interface_callback_hit_item` | 289 | — | **100.00** |
| `0041c440` | `advisor_callback_schedule_gate` | 44 | 31.11 | 93.33 |
| `0041c470` | `interface_callback_hotkey` | 119 | — | 88.61 |
| `0041c4f0` | `interface_update_advisor` | 495 | 13.10 | 87.54 |
| `0041c6e0` | `interface_handle_item_press` | 206 | 84.77 | 93.51 |
| `0041c7b0` | `interface_update_item_enabled` | 73 | 19.75 | 75.00 |
| `0041c800` | `interface_update_object_item_enabled` | 239 | 22.08 | 98.82 |
| `0041c8f0` | `interface_is_item_disabled` | 79 | — | 83.64 |
| `0041c950` | `interface_is_object_item_disabled` | 144 | — | 90.38 |
| `0041c9f0` | `interface_set_item_pressed` | 41 | **100.00** | **100.00** |
| `0041ca20` | `interface_set_object_item_pressed` | 55 | 41.03 | 64.86 |
| `0041ca60` | `interface_is_item_pressed` | 15 | **100.00** | **100.00** |
| `0041ca70` | `interface_is_object_item_pressed` | 50 | 21.43 | **100.00** |
| `0041cab0` | `interface_get_item_image` | 60 | 66.67 | **100.00** |
| `0041caf0` | `interface_get_object_item_image` | 145 | 37.62 | **100.00** |
| `0041cb90` | `input_binding_default_value_get` | 227 | 16.67 | **100.00** |
| `0041cc80` | `interface_set_dependency_item` | 81 | **100.00** | **100.00** |
| `0041cce0` | `interface_set_dependency_area` | 75 | 37.21 | 90.00 |
| `0041cd30` | `position_tracker_box_draw` | 368 | 19.35 | 25.53 |
| `0041cea0` | `interface_object_menu_check` | 116 | 16.51 | 91.11 |
| `0041cf20` | `interface_driven_object_menu_check` | 95 | 69.05 | 94.87 |
| `0041cf80` | `selection_mode7_cancel_at_point` | 42 | 15.38 | **100.00** |
| `0041cfb0` | `cancel_action_for_matching_coordinates` | 48 | 33.33 | **100.00** |
| `0041cfe0` | `selection_mode5_cancel_at_point` | 42 | 10.53 | **100.00** |
| `0041d010` | `interface_back_to_main_if_selected` | 34 | 31.11 | **100.00** |
| `0041d040` | `interface_is_item_available` | 3160 | 13.14 | 93.35 |
| `0041dca0` | `interface_do_action` | 2608 | 4.73 | 84.10 |
| `0041e750` | `selection_clear_request_and_hud_reset` | 29 | **100.00** | **100.00** |
| `0041e770` | `interface_set_item_flash` | 41 | — | **100.00** |
| `0041e7a0` | `interface_set_object_item_flash` | 55 | — | 64.86 |
| `0041e7e0` | `interface_get_primary_selected` | 5 | — | **100.00** |
| `0041e7f0` | `interface_set_item_clicks` | 23 | 7.41 | **100.00** |
| `0041e810` | `interface_get_item_clicks` | 12 | 37.50 | **100.00** |
| `0041e820` | `interface_set_object_item_clicks` | 61 | 21.62 | **100.00** |
| `0041e860` | `interface_get_object_item_clicks` | 44 | 15.38 | **100.00** |
| `0041e890` | `interface_set_advisor_item` | 64 | — | **100.00** |
| `0041e8d0` | `interface_get_advisor_type` | 31 | — | 30.00 |
| `0041e990` | `cell_action_permission_get` | 44 | 42.42 | 83.33 |
| `0041e9d0` | `interface_set_slide_positions` | 117 | **100.00** | **100.00** |
| `0041ea50` | `interface_set_menu` | 106 | **100.00** | **100.00** |
| `0041eac0` | `interface_update_slide` | 368 | 16.06 | 70.59 |
| `0041ec30` | `interface_draw_object_count` | 97 | — | 83.78 |
| `0041eca0` | `interface_draw_dependencies` | 403 | 12.23 | 48.61 |
| `0041ee60` | `interface_reset_dependency_timer` | 24 | **100.00** | **100.00** |
| `0041ee80` | `interface_get_dependency_at_point` | 628 | 23.04 | 63.11 |
| `0041f100` | `interface_object_type_present` | 138 | 24.66 | 68.42 |
| `0041f190` | `interface_callback_object_search` | 149 | — | 97.14 |
| `0041f230` | `interface_any_object_of_type2` | 55 | 35.71 | 29.63 |
| `0041f270` | `interface_any_object_with_flags3` | 55 | 33.33 | 41.38 |
| `0041f2b0` | `interface_any_object_with_flags2_100000` | 55 | 35.71 | 37.04 |
| `0041f2f0` | `interface_any_object_of_type` | 63 | 48.78 | 76.47 |
| `0041f340` | `interface_any_object_with_ability` | 59 | 43.75 | 64.52 |
| `0041f390` | `interface_callback_can_reach` | 35 | — | **100.00** |
| `0041f3c0` | `interface_any_object_can_reach` | 19 | 14.29 | **100.00** |
| `0041f3e0` | `interface_callback_can_enter` | 31 | — | 92.31 |
| `0041f400` | `interface_any_object_can_reach_alt` | 19 | 9.52 | **100.00** |
| `0041f420` | `interface_enumerate_selected` | 73 | 75.27 | **100.00** |
| `0041f470` | `interface_callback_can_unload` | 29 | — | **100.00** |
| `0041f490` | `interface_callback_hungry` | 32 | — | **100.00** |
| `0041f4b0` | `interface_callback_damaged` | 32 | — | **100.00** |
| `0041f4d0` | `interface_callback_can_pick_up` | 285 | — | 85.86 |
| `0041f5f0` | `interface_callback_delete` | 32 | — | **100.00** |
| `0041f610` | `interface_callback_go_feed` | 35 | — | **100.00** |
| `0041f640` | `interface_callback_repair` | 37 | — | **100.00** |
| `0041f670` | `interface_callback_lacks_tool` | 24 | — | **100.00** |
| `0041f690` | `interface_callback_get_tool` | 36 | — | **100.00** |
| `0041f6c0` | `interface_callback_detach` | 83 | 65.57 | 93.55 |
| `0041f720` | `interface_primary_has_ability` | 24 | 40.00 | **100.00** |
| `0041f740` | `interface_can_lay_path` | 196 | 16.81 | 59.17 |
| `0041f820` | `interface_callback_has_tool` | 25 | — | **100.00** |
| `0041f840` | `interface_callback_remove_spider` | 33 | 53.66 | 92.86 |

The unit's functions are named after what they do (`interface_*`); the header
comments give the original's `Interface_*` name where it is known. Eleven keep
the port's name (below).

### 25 newly mapped functions

The scaffold listed the 73 functions of `translation-units.csv`. The
disassembly has 25 more that Ghidra merged into their neighbours:

- **16 callbacks** passed by address (`push offset`): the menu-item callbacks
  `0041b800` (draw), `0041c310` (hit test) and `0041c470` (F2 hot keys), and
  the object-pool and selection callbacks `0041f190`, `0041f390`, `0041f3e0`,
  `0041f470`–`0041f690` and `0041f820`. `0041f6c0` and `0041f840` were already
  in the manifest.
- **The item flag and click helpers** `0041e770`–`0041e8d0` inside
  `0041e750`'s range (`0041e7f0`–`0041e860` were annotated in the port but not
  in the manifest; `0041e770`, `0041e7a0`, `0041e7e0`, `0041e890` and the
  switch `0041e8d0` were not written at all).
- `0041b490` (the menu draw), `0041c8f0`, `0041c950` and `0041ec30`, which were
  in the manifest but had no port definition.

## The unit

### State

- **`.bss`**: `interfaceGlobs` (`004dcd58`, 0x14ac bytes, typed in
  `interface_unit.h`). Lego.c reads the menu type and the flashing state
  (`00430320`) and Info.c reads the font (`0041a2f0`), so it is external,
  zero-initialised (`= { 0 }`) to stay in this object's `.bss`, as
  `panelGlobs` does (finding 554). The surround, plus-surround and back-button
  position arrays are indexed by item count − 1, which MSVC folds into the
  address (`(&font)[count]` in Ghidra).
- **Statics**: `0041eca0`'s last dependency type (`.data` `004a2b5c`, 20) is a
  function-scope static; its last index is the file static `004dcd50` before
  the struct; the hover-sound timer is the `.data` static `004a2b58` (25.0).
- **`.rdata`** `0049e274`–`0049e2a8`: float literals, pooled by MSVC, not
  annotated (the relocation's `0049e298` annotation is gone).
- **Strings**: the 74 `Interface_MenuItem_*` names and the unit's own literals
  carry `// STRING` markers on the line above them; the pooled `NULL`, `:`,
  `%s`, `|` and `%i` carry none. `"InterfaceBuildImages"` is used twice and
  marked once (Reccmp drops a duplicate address).

`make unit-data-offsets`, GAME-020 row:

| Unit | .data | .rdata | .bss | Commons |
| --- | --- | --- | --- | --- |
| GAME-020 | partial (82, 1 unknown) gap 7 first 004a34d8: 980/984 | n/a (0, 0 unknown) fewer than two attributable items | gaps-only (2, 0 unknown) | - |

### Entries that keep the port's name

Annotated code outside the unit calls eleven of these functions (Panel.c,
Priorities.c, Message.c, Lego.c, LegoObject.c, the construction and help
units). Their declarations are in editable headers, so the unit function takes
the port's name and spelling of the arguments, the port's implementation is
renamed `*_port` in the tail, and the unit function delegates to it in native
builds inside `#ifdef INTERFACE_PORT` (defined unless MSVC 5.0, as
`PANEL_PORT`). The matching build compiles only the original bodies; every
caller now calls the annotated address, and native behaviour is unchanged:
`command_context_post_to_hud` (`0041b2d0`), `sim_status_banner_word_get`
(`0041b9b0`), `simulation_global_word_set` (`0041ba90`),
`advisor_callback_schedule_gate` (`0041c440`), `input_binding_default_value_get`
(`0041cb90`), `position_tracker_box_draw` (`0041cd30`),
`selection_mode7_cancel_at_point`, `cancel_action_for_matching_coordinates`,
`selection_mode5_cancel_at_point` (`0041cf80`–`0041cfe0`),
`selection_clear_request_and_hud_reset` (`0041e750`) and
`cell_action_permission_get` (`0041e990`).

`info.cpp`'s `0x0041a2f0` reads `interfaceGlobs.font` instead of the port's
unannotated `s_incident_state` (both are zero natively). The include sits after
every other annotated function in that file, so their counters do not move.

Not shared: `0041baa0` (Lego.c calls a namespaced ledger stub through a frozen
header), `0041cf20` and `0041d010` (the port's entries take a `GameContext` or
`EngineContext`). Their callers' operands did not change score.

### Callees

- **`extern "C"` in place** (no counter change): 21 entries in
  `game/entity.hpp`, 10 in `cmd.hpp`, 16 in `level.hpp`, 9 in `select.hpp`,
  4 in `frontend.hpp`, 2 in `ai.hpp`, and one each in `crt/crt.hpp`,
  `gods98/engine.hpp`, `simtick.hpp` and `sound.hpp`.
- **New boundary header**
  [`interface_callees.h`](../../src/game/interface_callees.h), included only by
  the unit, the tail and `test_c_boundaries.cpp`: the port entries above with C
  linkage, the original-layout views of the LegoObject, Lego_Level/Lego_Block
  and camera fields the unit reads, and the adapters. It is separate from
  `lego.h`, `cmd.h`, `entity.h`, `message.h` and `frontend.h` because other C
  units include those (finding 538, rule B). `test_c_boundaries.cpp` cannot
  include `entity.hpp` beside `steering.h` (finding 549), so the LegoObject.c
  and Stats.c pairs are not checked there.
- **Thin unannotated adapters** with the original signatures, at the end of the
  port tail (the port's entries take x and y instead of a block pointer, an
  extra `GameContext` or `LevelState`, live in a namespace, or are declared only
  in frozen headers): `lego_ai_create_dig`, `lego_ai_create_dynamite`,
  `lego_ai_create_reinforce`, `lego_ai_create_dam`, `aitask_list_contains`,
  `lego_path_remove`, `dependencies_get_requirements`,
  `lego_block_has_lava_neighbour`, `lego_get_resource_count`,
  `lego_get_resource`, `lego_set_tracked_object`, `lego_get_min_health`,
  `lego_object_type_for_name`, `lego_level_block_update`, `lego_get_map`,
  `lego_object_can_enter_block`, `lego_object_set_primary_teleporter`,
  `lego_object_can_eject`, `lego_object_pair_requirement_met`,
  `lego_object_get_container`, `lego_object_drop_carried`,
  `lego_object_go_to_block`, `lego_object_get_block`,
  `lego_object_find_trainer`, `lego_object_detach_all`,
  `entity_kind6_tick_down_marker_spawn_obj`, `container_get_position`,
  `tooltip_set_persistent` and `tooltip_reset_timer`. Each costs one call
  operand. `Font_PrintF` uses `font_print_f` from #547.
- **Stand-ins** (unannotated, in the tail) for the legoGlobs fields with no
  C-visible owner (`legoGlobs.flags1`/`flags2`, the type names and counts, the
  first-person camera and object, the primary teleporters, the request queue,
  the upgrade costs and texts, the level names), LegoObject.c's tool-tip sounds
  and Input.c's key maps. Their data operands differ until #544 converts
  Lego.c; `s_global_low_word` (`00556ed4`) is annotated in `engine.cpp` but
  declared with C++ linkage only in a frozen header.

### Shape notes

- **One `return 0` per condition group.** In `0041d040` (the item availability
  switch, 3,168 bytes) the original returns through shared epilogues: each
  `if (!a || !b || !c) return 0;` group has one inline epilogue and earlier
  tests jump to a shared one. Writing each case's tests as the original's
  groups (read off the inline epilogues) took it from 56% to 93%. Cases 49–56
  are two statements each so that their tail merges with case 48.
- **Jump-table order.** Case bodies follow the order of the jump table's
  targets, not the case values, in `0041d040`, `0041dca0` and `0041cb90`.
- **`0041baa0`**: the cursor-over-item path is the `then` branch and the
  surround pixel test the `else`; the tool tip and the displayed text are two
  variables; the dependency and item paths set `result = 1` and share the
  final `return result`.
- **`0041cd30`** draws the box as one five-point polyline (`list`, `list + 1`,
  4), not two arrays.
- **`0041c1c0`** takes unsigned coordinates (`fild qword`) and compares
  `(float)x >= left`.
- **`0041c6e0`**: the button-down branch comes first (84.77 → 93.51%).

### Near misses

`tools/msvc_permuter.py` (150 s each) found no applicable rewrite for
`0041cd30`, `0041ee80`, `0041eca0`, `0041c1c0`, `0041b800`, `0041eac0`,
`0041f740`, `0041a850` or `0041aa00`.

| Address | Score | What is left |
| --- | ---: | --- |
| `0041baa0` | 72.76 | register choice (`y` in `ebx`, the text in a stack slot); stand-in operands |
| `0041dca0` | 84.10 | the disabled check's `return 0` sits at the end in the original; shared tails of the menu changes |
| `0041ee80`, `0041eca0` | 63.11, 48.61 | x87 scheduling of the dependency layout; the list adapter |
| `0041c1c0` | 49.28 | `x` and `y` kept in `edi`/`ebx` across the back-button test |
| `0041b800` | 47.86 | the original's flash flag shares `objType`'s register; MSVC duplicates the pressed-reset call into each branch |
| `0041cd30` | 25.53 | the original keeps corners in dead parameter slots |
| `0041f230`–`0041f340` | 30–76 | the original interleaves the search-record stores with the call's pushes |

## Entropy and drops

`make reccmp-check` against `887b5c56`: 60 improved, 25 newly mapped, link
order OK, **one function below baseline**, inside the unit:

| Address | Function | Score | Instructions | Label |
| --- | --- | --- | --- | --- |
| `0x0041f230` | `interface_any_object_of_type2` | 35.71 → 29.63 | −1 | source-changed |

**For the owner:** it is the original-shape search-record setup; the port's
version happened to place the stores closer to the original's. No variant of
the field order changes the code.

Outside the unit nothing fell, and no listing digest changed. One function
rose: `0x0045c0d0` `panel_crystals_draw` (90.03 → 92.07), whose call to
`0041b9b0` now reaches the annotated address. No instruction count changed
outside the unit. No exact match is lost (919 → 951 exact, full scope).

## Reccmp lint ratchet

Stacked on #558, which drops definitions in link-order `tail` files and
definitions marked `// PORT-ONLY: <reason>` from the ledger (baseline 1,558).
`interface_port.cpp` is a `tail` file, so the port's relocated functions, the
eleven `*_port` implementations and the adapters need no key. The ledger falls
to **1,527** (−31: the keys of the old `interface.cpp`); `make
reccmp-lint-update` needed no `--allow-growth`. The unit file adds no
port-only definition: its native delegations sit inside annotated functions.

## Tooling notes

- `convert_unit.py` still refuses a relocated unit; the `.cpp` entry was
  removed from the link-order list by hand and `--stem interface_unit` used
  (`interface.h` exists and `panel_unit.c` includes it).
- The scaffold's boundary-test includes added `game/entity.hpp`, which
  conflicts with `steering.h` (as in finding 549); it was dropped.
- The scaffold's stub list misses functions Ghidra merged into neighbours; a
  scan for `push offset` targets and 16-byte-aligned starts after `ret` found
  the 25 above.
- A full `make reccmp-check` that runs while the unit is edited reports "Debug
  data out of sync" and attaches annotations to the wrong functions: it reads
  current line numbers against the object compiled earlier.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox): the one flagged drop
  above; link-order map OK. `make reccmp-baseline-update` and
  `make binary-progress-compare` (host Python, for `pefile`) regenerated the
  three files; `pr_match_progress.py check` reports them fresh.
- `make unit-data-offsets`: the GAME-020 row above; summary 0 full / 2
  gaps-only / 31 partial / 192 n/a.
- `python3 tools/translation_units.py build`: 0 conflicts;
  `tools/binary_progress_inputs.txt` regenerated.
- `make tools-test` passes; `make reccmp-lint`: 1,527 existing entries, none to remove.
- `interface_unit.c` passes `gcc -std=gnu89 -Werror=declaration-after-statement
  -fsyntax-only`.
- `cmake -S . -B build && cmake --build build && ctest --test-dir build
  --output-on-failure`: 37/37 pass, including `c_boundaries`.

## Reproducing

    python3 tools/convert_unit.py GAME-020 --stem interface_unit   # after removing the .cpp entry
    make match-function SOURCE=src/game/interface_unit.c
    make reccmp-check
    make match-progress
    make unit-data-offsets
