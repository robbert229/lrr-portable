# Finding 554 — Panel unit (GAME-032) rebuilt in original shape

Issue #523, under [ADR 0005](../adr/0005-original-shape-c-units.md). #515
(finding 546) relocated GAME-032 (Panel.c) verbatim into
`src/game/panel_unit.cpp`. This change rebuilds it as the C unit
[`src/game/panel_unit.c`](../../src/game/panel_unit.c), with the C boundary
header [`panel_unit.h`](../../src/game/panel_unit.h), and deletes the `.cpp`.
The unit is text `0045a030`–`0045c830`.

- **The panels.** Twelve interface panels (radar, messages, crystal side bar,
  top panel, information, priority list, camera control, info dock,
  encyclopedia, ...). Each slides between an "out" and an "in" position and
  owns a list of buttons with normal, highlight and pressed images.
- **Other state in the unit.** The panel text windows, the camera-control
  rotation widget, the crystal side bar, the air meter and the ore meter.

"Before" is the *match baseline* at `e326140e`, the head of #529 (G98-009),
which contains #526 (GAME-023) and #528 (GAME-017, GAME-020, G98-011,
G98-014). This change is stacked on #529. "After" is this change. `make reccmp-check` and `make match-function` give the same
per-function scores, except where noted below.

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (byte-weighted, all 52 functions, 9,823 bytes) | 30.65% | **90.17%** |
| Exact functions | 12 | **31 of 52** |
| Mapped functions | 50 | 52 |
| Accuracy, full (`binary-progress.json`) | 58.07% | 59.11% |
| Accuracy, game-only | 53.59% | 54.75% |
| Exact-function code bytes | 70,358 | 72,434 |

Byte weights are the original's code bytes, without the `0x90` fill. An
unmapped function counts as 0%.

| Address | Function | Bytes | before | after |
| --- | --- | ---: | ---: | ---: |
| `0045a030` | `ui_panel_name_tables_init` (Panel_Initialise) | 521 | 100.00 | **100.00** |
| `0045a240` | `legacy_scroll_panel_controls_initialize` | 39 | 63.41 | **100.00** |
| `0045a270` | `legacy_panel_category_interaction_state_reset` | 101 | 17.72 | **100.00** |
| `0045a2e0` | `legacy_overlay_image_record_load` (Panel_LoadImage) | 129 | 13.27 | 93.88 |
| `0045a370` | `ui_panel_name_table_lookup` (Panel_GetPanelType) | 63 | 52.05 | **100.00** |
| `0045a3b0` | `legacy_panel_slot_coordinate_hit_test` | 94 | 28.89 | 97.14 |
| `0045a410` | `ui_panel_indexed_name_lookup` (Panel_GetButtonType) | 75 | 47.73 | **100.00** |
| `0045a460` | `panel_print_f` (new) | 148 | unmapped | 96.08 |
| `0045a500` | `ui_text_output_record_allocate` (Panel_TextWindow_Create) | 141 | 34.29 | **100.00** |
| `0045a590` | `panel_text_window_print_f` | 28 | 100.00 | **100.00** |
| `0045a5b0` | `ui_text_output_position_and_draw` | 99 | 32.56 | **100.00** |
| `0045a620` | `ui_text_output_object_release_target` | 16 | 100.00 | **100.00** |
| `0045a630` | `ui_record_callback_forward` | 26 | 32.26 | **100.00** |
| `0045a650` | `panel_category_bounds_set` (Panel_SetArea) | 139 | 15.62 | 39.44 |
| `0045a6e0` | `indexed_pair_set` | 31 | 76.19 | **100.00** |
| `0045a700` | `indexed_pair_get` (Panel_GetPosition) | 34 | 26.67 | **100.00** |
| `0045a730` | `simtick_manager_tick` (Panel_Update) | 642 | 16.74 | 90.86 |
| `0045a9c0` | `panel_texture_slots_blit_to` (buttons display) | 241 | 13.33 | 98.97 |
| `0045aac0` | `panel_item_flag16_set` | 56 | 19.35 | **100.00** |
| `0045ab00` | `panel_state_flags_toggle` (Panel_Toggle) | 39 | 42.11 | **100.00** |
| `0045ab30` | `request_record_ready_flag4_target` | 35 | 100.00 | **100.00** (C, was asm) |
| `0045ab60` | `request_record_ready_flag2_target` | 35 | 100.00 | **100.00** (C, was asm) |
| `0045ab90` | `request_record_flag8_is_set_target` | 20 | 100.00 | **100.00** (C, was asm) |
| `0045abb0` | `panel_item_flag32_set` | 54 | 25.81 | **100.00** |
| `0045abf0` | `panel_item_flag8_set_if_enabled` | 56 | 27.78 | **100.00** |
| `0045ac30` | `panel_texture_slots_allocate` (button loader) | 381 | 16.55 | 95.97 |
| `0045adb0` | `panel_cursor_interaction_update` (was a ledger stub) | 1368 | 20.09 | 78.63 |
| `0045b310` | `panel_button_collision_check` (was a ledger stub) | 629 | 19.64 | 57.28 |
| `0045b590` | `panel_button_point_inside` (new) | 113 | unmapped | **100.00** |
| `0045b610` | `simtick_global_ui_flags_clear_bits12` | 13 | 100.00 | **100.00** |
| `0045b620` | `ui_list_priority_button_action_dispatch` | 276 | 100.00 | **100.00** |
| `0045b740` | `ui_scroll_info_panel_initialize` | 268 | 20.59 | 92.19 |
| `0045b850` | `ui_overlay_create_configured_element` | 71 | 12.50 | 93.75 |
| `0045b8a0` | `point_distance_within_radius` | 93 | 100.00 | **100.00** (asm kept) |
| `0045b900` | `integer_pair_divide_to_float_pair` | 35 | 100.00 | **100.00** |
| `0045b930` | `float_pair_to_integer_pair_x87` (circle clamp) | 153 | 20.00 | 75.93 |
| `0045b9d0` | `frontend_panel_rotation_control_config_load` | 760 | 27.27 | 95.20 |
| `0045bcd0` | `panel_rotation_control_drag` (was a ledger stub) | 583 | 15.04 | **100.00** |
| `0045bf20` | `camera_zoom_step_adjust` | 72 | 100.00 | **100.00** |
| `0045bf70` | `ui_entry_bounds_get` | 62 | 25.00 | **100.00** |
| `0045bfb0` | `three_surface_resources_load` (crystal images) | 142 | 11.27 | 89.09 |
| `0045c040` | `mission_crystal_quota_setup` | 137 | 35.00 | 85.42 |
| `0045c0d0` | `panel_crystals_draw` (was a ledger stub) | 613 | 17.11 | 90.03 |
| `0045c340` | `panel_resource_layout_store` (air meter) | 167 | 37.89 | 94.34 |
| `0045c3f0` | `advisor_progress_bar_fill_draw` (air meter) | 172 | 15.25 | 93.02 |
| `0045c4a0` | `global_value_0050080c_set` | 10 | 100.00 | **100.00** |
| `0045c4b0` | `panel_resource_position_update` | 97 | 17.07 | 96.67 |
| `0045c520` | `panel_resource_handle_and_dimensions_store` (ore meter) | 90 | 25.00 | 95.45 |
| `0045c580` | `ui_overlay_animation_advance` | 112 | 37.93 | **100.00** |
| `0045c5f0` | `panel_side_bar_draw` (was a ledger stub) | 119 | 22.22 | 96.88 |
| `0045c670` | `panel_advisor_animation_play` (was a ledger stub) | 54 | 91.89 | **100.00** |
| `0045c6b0` | `advisor_animation_slot_id_resolve` | 371 | 21.21 | **100.00** |

19 functions are newly exact; none falls. Most keep the port's names. The
seven ledger stubs get descriptive names, and the two functions the port
never wrote (`0045a460`, `0045b590`) are mapped for the first time. The
names in parentheses are descriptive; they do not appear in the source.

## The unit

### State: one external, zero-initialised struct

The unit's `.bss` (`005000e0`–`00500898`, 0x7b8 bytes) is one
`PanelGlobs panelGlobs`, declared in `panel_unit.h`:

- the twelve panel names;
- twelve 0x30-byte `PanelData` records (image or flic, out/in/current
  position, button list and count, flags);
- the button-name table, `[12][24]`;
- the selected panel and button;
- the two text windows (`005007d8`, `005007dc`);
- the crystal images and quota, the air-meter and ore-meter layouts;
- the rotation widget (centre, radius, four arrow images and positions);
- the flag word `00500860` and the 2×6 crystal history.

`PanelButton` (0x2c bytes) and `PanelTextWindow` (0x10) are typed too.

**Other original units read these fields directly.** Scanning `.text` for
absolute references into the range finds them in GAME-019 (`005007d8`),
GAME-020 (`0050013c`, `005002bc`, `00500860`), GAME-034 Priorities.c
(`00500860`), GAME-039 (`00500110`) and others. So the original struct is
external, yet it sits in Panel.obj's own `.bss`, not the common region.

**MSVC 5.0 C puts a zero-initialised external in `_BSS`.** Compiling
`G g_tentative; G g_zero = { 0 }; static G g_static;` with the matching
flags gives `COMM _g_tentative`, `PUBLIC _g_zero` in `_BSS`, and
`_g_static$S83` in `_BSS`. The unit therefore defines
`PanelGlobs panelGlobs = { 0 };`. That is the original's shape, and it lets
port code and other units read the state without accessor functions.
Earlier conversions used a `static` struct; this is the first external one.

`make unit-data-offsets`, GAME-032 row:

| Unit | .data | .rdata | .bss | Commons |
| --- | --- | --- | --- | --- |
| GAME-032 | partial (54, 7 unknown) gap 4 first 004a89b8: 640/644 | n/a (0, 0 unknown) fewer than two attributable items | n/a (1, 0 unknown) fewer than two attributable items | - |

Before, `.bss` was partial (20 items, a 19-item gap from `00500110`); it is
now the one struct. `.data` holds the unit's string literals.

### Strings

- Each literal the unit owns has a `// STRING` marker on the line directly
  above it. The rotation-control keys are split one literal per line for
  that.
- Pooled literals get no marker, following findings 549 and 550:
  `"Reward"`, `"Quota"`, `"Crystals"` (Reward.c reuses all three), `","`,
  `"%i"` (Info.c's range) and the empty string `004b8a14` (config.c). Their
  operands are the remaining difference in `0045c040` and part of it in
  `0045b9d0` and `0045c0d0`.
- `s_task_name_crystals` (`004a8a08`, finding 546 fix-up 2) goes back to
  `mission.cpp`, its owner before #516, with its annotation: Reward.c still
  names it.

### Contracts and adapters

The unit calls the port's entries where their contracts fit, with
`extern "C"` added in place (ADR 0005 step 6). New C boundary headers, each
included only by `panel_unit.c` and `test_c_boundaries.cpp`:
`game/ai.h`, `camera_control.h`, `info.h`, `interface.h`, `lego.h`,
`message.h`, `tutorial_script.h`, `gods98/flic.h`, `image_adapters.h` and
`textwindow.h`. `frontend.h`, `cmd.h`, `images.h`, `font.h` and `message.h`
are not extended: other C units, or many C++ files, include them.

Contracts used as they are:

- `0047a150`/`0047a1b0`, `0047dbc0`, `0047dbf0`, `0047dfa0`, `00483c80`,
  `00484070`: the port's Images.c, Font.c and Flic.c entries take the
  `TextureSurface` an `Image` wraps, a software render target, or return
  the flic rather than store it. Calling them with `image->surface` costs
  more than it saves (`0045bfb0` 89 → 54%), so thin unannotated adapters
  stand in: `image_setup_trans`, `image_get_pixel`,
  `image_set_pen_zero_trans`, `font_v_get_string_info`, `font_v_print_f`,
  `flic_setup`, `flic_animate`. Each costs one call operand.
- `004636b0` and `004638a0` (GAME-039 InfoPanel) have no port entries;
  `info_panel_initialise` and `info_panel_check_collision` are empty
  adapters.
- `00456240` and `00435aa0` take one argument less or more in the port
  (`system_state_flags_get()`, `panel_selection_highlight_toggle(ctx, on)`).
- `legoGlobs` fields: `flags1` (`00556ecc`) lives in the port's
  `EngineContext`, so the unit reads it through `engine_shared_state_word()`;
  the fonts (`00556124`, `00556128`), the radar zoom (`005563f4`) and
  LegoObject.c's `004eabc8` have no port owner, so port globals stand in
  (end of `lego.cpp` and `ai.cpp`). These operands differ.
- `004369f4` CameraTiltAxisUpdate had no port entry. It is now written and
  annotated in `camera_control.cpp` (GAME-022), in original order: newly
  mapped at 10.53%, like its frame-pointer sibling `00436900`. With the
  call mapped, `0045bcd0` is exact. `00436953`'s range now ends where it
  should, and its score rises from 35.59 to 36.84.
- Panel positions are floats, as in the original (`indexed_pair_get/set`).
  `priorities.c` and `advisor.c` switch from integer bits to floats.
- `ui_entry_bounds_get`, the text-window entries and the button loader keep
  the port's pointer spellings (`float*`, `void*`) in the shared C++
  headers: an elaborated `struct Area2F*` or `struct PanelTextWindow*`
  there declares a new tag in every includer and drifted three functions
  (below).

### Shape notes

- **`0045a270`**: exact only with the flags read into a local first.
- **`0045a650`, 39%**: the original converts each corner once, stores it
  twice (a local and a dead parameter slot) and copies the fields through
  integer moves. Point2F locals give the best of 11 shapes.
- **`0045a730`**: `flicArea.x = position.x = floor(...)` (that order);
  `4.0f / elapsed + 1.0f` compiles to the original's `fsub -1.0`.
- **`0045c580`**: `0.05263158f * ((level - 0.05f) * 20.0f)`. The other
  operand order folds the two constants.
- **`0045c6b0`, `0045adb0`**: the switch cases follow the original's case
  order (from the jump table's targets), not the case values.
- **`0045c0d0`**: `= { 0 }` initialisers reproduce the first-element store
  plus `rep stosd`; the quota loop is `n = quota; while (n--)`. The first
  `usedCrystals` crystals use `005007e0`'s image and the rest `005007e8`'s.
- **`0045b930`**: Maths.h's 2D normalise and scale, as `static __inline`
  helpers in [`panel_maths.h`](../../src/game/panel_maths.h).
- **`0045b8a0`** keeps its `__declspec(naked)` body under MSVC (C for other
  compilers), as `vec_cross` does (finding 550). The C body loads the radius
  after the second difference; no expression shape or counter phase moves
  it. It was exact before and stays exact.
- **`0045ab30`/`0045ab60`/`0045ab90`** were naked asm; plain C is exact.

### Near misses

`tools/msvc_permuter.py` found no applicable rewrite for `0045a270` (which
was then fixed by hand). Rule-B sweeps (finding 538: 0–64 inert
declarations) are flat for `0045b8a0`, `0045c4b0`, `0045c040`, `0045b310`,
`0045adb0` and `0045a270`, so none is a phase effect.

| Address | Score | What is left |
| --- | ---: | --- |
| `0045a2e0`, `0045a3b0`, `0045a460`, `0045a9c0`, `0045ac30`, `0045b740`, `0045b850`, `0045bfb0`, `0045c340`, `0045c3f0`, `0045c4b0`, `0045c520`, `0045c5f0`, `0045b9d0` | 89–99 | call operands of the adapters above and of the unmapped `0047dcb0` Image_DisplayScaled, the font globals, pooled `","`; the code is otherwise the original's |
| `0045c040` | 85.42 | pooled string operands only |
| `0045c0d0` | 90.03 | `"%i"`, the font global, the display adapter; register order in the zeroing |
| `0045a730` | 90.86 | `flags1` through `engine_shared_state_word()`, adapter operands |
| `0045adb0` | 78.63 | `flags1` (four calls), the one-argument contracts, and one early return the compiler tail-merges |
| `0045b930` | 75.93 | the radius conversion is scheduled after the squares, not before |
| `0045b310` | 57.28 | register allocation of the outer loop (panel index kept in a stack slot) |
| `0045a650` | 39.44 | see shape notes |

## Native port

The port's panel model (`s_legacy_panels`, `s_panel_records`, the
32-bit-word draw records with a pointer sidecar, the hook sinks) is
replaced by `panelGlobs`:

- `crt.cpp`'s request-record and draw-record accessors, `tutorial_hud.cpp`'s
  panel-image getters, and `frontend.cpp`'s pointer, activation and render
  code read the typed records. The frozen `*_from_panel_unit.hpp` private
  headers map the port's old global names onto `panelGlobs` fields.
- Panels and buttons load through `image_load_bmp_scaled`, so the native HUD
  still draws them; `panel_texture_slots_render` draws a panel's buttons
  with Panel_Button_DisplayAll's state choice.
- `panel_texture_slots_allocate_named` checks the button table and frees a
  list it built before the original loader replaces it (the original
  leaks).
- Two guards the native port needs (a NULL text window in
  `ui_text_output_position_and_draw` and `ui_record_callback_forward`) are
  compiled out of the matching build (`PANEL_PORT`, ADR 0006).
- `tickers.cpp` calls Panel_Update directly; it already draws the buttons.
- The superseded slot-table model of ManagerTick in `simtick.cpp` is
  deleted; its declarations stay (entropy rule 1).

Tests: `test_crt`, `test_frontend`, `test_simtick` and `test_engine_bind`
move to the typed records and the original behaviour (convergence, the
camera-control arrows, the button state choice drawn into a 16×16 target,
the loader's corners and tool tips). The old seam and guard checks are
gone.

## Entropy

Every shared C++ header edit keeps its includers' symbol counts:
`extern "C"` in place, the port's parameter spellings, and renamed
superseded declarations (`simtick_manager_tick_superseded`,
`float_pair_to_integer_pair_x87_superseded`,
`ui_scroll_info_panel_initialize_superseded`,
`ui_overlay_create_configured_element_superseded`). Three things showed up
on the way and were fixed:

| Cause | Effect | Fix |
| --- | --- | --- |
| `advisor.c` including `panel_unit.h` (+4 once the struct moved into it) | `0x004016f0` `vec_dot` 100 → 62.07 | `advisor.c` includes a restored, count-identical `panel.h` |
| `struct Area2F*`/`struct PanelTextWindow*` in `frontend.hpp`, `crt.hpp`, `heap.hpp`, `record.hpp` | `0x00444fb0`, `0x0047d800`, `0x004882e0` −0.2 to −0.5 | the port's pointer spellings |
| `simtick.hpp` losing one parameter | `0x004089d0`, `0x0044fb90` (as in finding 550) | old declaration kept, renamed; `tickers.cpp` (no annotated functions) includes `panel_unit.h` |

Final `make reccmp-check` against `e326140e`:

- 42 improved, 3 newly mapped, link-order map OK;
- **one function below baseline**, outside the unit:

| Address | Function | File | Score | Instructions | Label |
| --- | --- | --- | --- | --- | --- |
| `0x0045d110` | `priorities_handle_cursor` | `priorities.c` | 53.99 → 50.16 | 164 → 158 | `source-changed` |

**For the owner.** Priorities.c now reads the panel position as floats, as
the original does. Its three siblings rise (`0x0045d550` 75.86 → 85.29,
`0x0045d640` 52.27 → 62.50, `0x0045d6d0` 52.87 → 61.94);
`priorities_handle_cursor` falls by register allocation. Its 24 declaration
orders all give 50.16; the original keeps `type` and `i` in other
registers. The baseline is refreshed with this drop.

The only other listing change outside the unit is `0x00463d20`
(`info_track_overlay.cpp`, source-changed: it reads `panelGlobs` instead of
the old record words), at equal score and instruction count. (On `main`
before the stack, `0x004078c0`, `0x00411360`, `0x00411870` and `0x0041c1c0`
also changed at equal score; on #529's base they do not.) No exact match is
lost: 843 exact functions (full scope) become 862, all 19 new ones in the
unit.

`priorities_change_priority` (`0x0045d3c0`) reads `00500860` as
`panelGlobs.flags` now, as the original does. Through the old
`simtick_timer_word` name, with that global gone, it fell to 93.33; it stays
at 100.

## Reccmp lint ratchet

4,118 → 4,110 entries. Removed (17): the relocated file's static helpers
and asm bodies (7), the `simtick.cpp` slot-table model (9) and the ledger
row for `004369f4`. Added (9): the adapters above (`image_setup_trans`,
`image_get_pixel`, `image_set_pen_zero_trans`, `font_v_get_string_info`,
`font_v_print_f`, `flic_setup`, `flic_animate`, `info_panel_initialise`,
`info_panel_check_collision`). ADR 0005 step 6 prescribes unannotated
adapters, so the update needed `--allow-growth` for these keys although
the total shrinks. **For the owner:** please confirm.

## Tooling notes

- `convert_unit.py` still refuses a relocated unit (findings 544, 549,
  550). The `.cpp` entry was removed from the link-order list by hand. The
  default stem `panel` names the existing `panel.h`, so `--stem panel_unit`.
- `match_function.py --candidate` cannot score a function whose body
  changes length (it finds the function by line); edit in place instead.
- `pr_match_progress.py render --base` wants a checkout path, not a ref.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox): the one flagged drop
  above; `link order check-map: OK`. `make reccmp-baseline-update`, then
  `make match-progress` regenerated the three generated files;
  `pr_match_progress.py check --base origin/convert/524-g98-009-sound3d
  --head .` reports them
  fresh.
- `make unit-data-offsets`: the GAME-032 row above; summary 0 full / 2
  gaps-only / 25 partial / 198 n/a.
- `python3 tools/translation_units.py build`: 0 conflicts.
  `tools/binary_progress_inputs.txt` regenerated.
- `make tools-test` passes (Meson parity, link order, lint ratchet).
- `panel_unit.c` passes `gcc -std=gnu89 -Werror=declaration-after-statement
  -fsyntax-only`.
- `cmake -S . -B build && cmake --build build && ctest --test-dir build
  --output-on-failure`: 37/37 pass, including `c_boundaries`.

## Reproducing

    python3 tools/convert_unit.py GAME-032 --stem panel_unit   # after removing the .cpp entry
    make match-function SOURCE=src/game/panel_unit.c
    make reccmp-check
    make match-progress
    make unit-data-offsets
