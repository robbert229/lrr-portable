# Finding 595 — HelpWindow.c (GAME-018) rebuilt in original shape, with a port tail

Issue #628, part of #388 under [ADR 0005](../adr/0005-original-shape-c-units.md).
GAME-018 is the original's `HelpWindow.c` (text `00418190`-`00419230`): the
help window. When the player first makes a vehicle, minifigure or building
available, it shows that object's `HelpWindowInfo` text from Lego.cfg and the
list of object types it unlocks, each with its icon and name. It has two
buttons (Continue, Cancel Future) and scrolls when the list is longer than
the text area. The unit was relocated verbatim into `src/game/helpwindow.cpp`
(#496). This change rebuilds it as the C unit
[`src/game/helpwindow.c`](../../src/game/helpwindow.c), with:

- the C boundary header [`helpwindow.h`](../../src/game/helpwindow.h);
- the unit-private imports header
  [`helpwindow_imports.h`](../../src/game/helpwindow_imports.h);
- the matching-build link names
  [`helpwindow_link_names.h`](../../src/game/helpwindow_link_names.h).

The port's implementation moves, unannotated, to the port tail
[`helpwindow_port.cpp`](../../src/game/helpwindow_port.cpp).

Stacked on #631 (GAME-024, `9d8a27a0`), which sits on #630 (G98-003) and
`main` `12a0e950`. "Before" is #631's head; "after" is this change rebased
onto it, from fresh full matching builds. The change was first measured on
`2dcc7dec` and then on `12a0e950`; no GAME-018 score differs between the
three bases, and the earlier bases' figures are given where they help.

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (byte-weighted, all 19 functions, 4,127 code bytes) | 25.79% | **95.20%** |
| Exact functions in the unit | 6 | **11 of 19** |
| Exact functions, full / game-only (`binary-progress.json`) | 1,521 / 1,321 | **1,526 / 1,326** |
| Exact-function code bytes | 173,540 (27.01%) | **173,989** (27.08%) |
| Accuracy, full / game-only | 82.26% / 80.53% | 82.56% / 80.86% |
| `opt_ref` annotated (full, by address): removed / exact removed | 497 / 233 | 497 / 233 (see [below](#opt_ref-finding-579-make-opt-ref-callers)) |
| `opt_ref` by symbol (`make opt-ref-callers --min 1`) | 479 / 215 | **477 / 214** |
| `.bss` of the unit's objects | 742,472 (`helpwindow.cpp`) | **11,500** (3,436 `helpwindow.c` + 8,064 `helpwindow_port.cpp`) |
| Image `.bss` / `.data` virtual size (`/OPT:REF` relink) | 3,831,232 / 3,932,144 | **3,100,256 / 3,201,360** |
| Reccmp lint ledger | 1,283 | **1,276** |

Byte weights are the single-function loop's original sizes. Scores are the
full matching build's (`make reccmp-check`).

The names are the unit's own (OpenLRR's where known, in the repository's
snake case). "Port name" is the name the port and the callers use; in the
matching build the unit's definition takes it where marked "link name" (see
[Link names](#link-names-and-caller-switches)).

| Address | Function | Port name | Bytes | Before | After |
| --- | --- | --- | ---: | ---: | ---: |
| `00418190` | `help_window_set_font` | `selection_mode_state_set` (link name) | 10 | 100.00 | **100.00** |
| `004181a0` | `help_window_hide` | `selection_state_flag_clear_bit0` (link name) | 13 | 100.00 | **100.00** |
| `004181b0` | `help_window_initialise` | `help_window_initialise_adapter` (link name) | 658 | 8.09 | 98.17 |
| `00418450` | `help_window_load_infos` | `help_window_info_config_strings_load` | 408 | 8.21 | 96.08 |
| `004185f0` | `help_window_load_buttons` | `help_window_button_layout_load` | 521 | 16.67 | 96.03 |
| `00418800` | `help_window_show_text` | `mode_flag_pair_apply_if_bit2` (link name) | 37 | 100.00 (naked asm) | **100.00** (C) |
| `00418830` | `help_window_recall_dependencies` | `building_upgrade_state_changed_dispatch` (link name) | 232 | 29.91 | 95.71 |
| `00418920` | `help_window_list_dependencies` | `dependency_grid_category_candidates_build` | 175 | 17.09 | 96.00 |
| `004189d0` | `help_window_close` | `help_render_transition_cleanup_if_active` | 48 | 93.75 | **100.00** |
| `00418a00` | `help_window_update` | `overlay_text_buffer_refresh` (caller switched) | 921 | 11.45 | 91.51 |
| `00418da0` | `help_window_check_buttons` | `help_window_button_hit_test` (link name) | 460 | 35.49 | 88.75 |
| `00418f80` | `help_window_button_tooltip` | `selection_mode_set_by_index` | 56 | 26.09 | **100.00** |
| `00418fc0` | `help_window_press_button` | `ui_mode_transition_request` | 110 | 34.67 | **100.00** |
| `00419030` | `help_window_button_action` | `help_window_button_feedback_update` | 200 | 71.15 | **100.00** |
| `00419100` | `help_window_draw_buttons` | `ui_mode_resource_positions_apply` | 74 | 71.64 | 96.55 |
| `00419150` | `help_window_get_button_image` | `selection_state_handle_get` | 77 | 100.00 | **100.00** |
| `004191a0` | `help_window_is_showing` | `main_sim_state_admission_predicate` (link name) | 71 | 51.85 | **100.00** |
| `004191f0` | `help_window_is_enabled` | `ui_mode_resource_bit40_get` | 9 | 100.00 | **100.00** |
| `00419200` | `help_window_toggle` | `ui_mode_resource_bit40_set_clear` | 47 | 100.00 | **100.00** |

`00418800` was naked asm and is now C (see [Source shapes](#source-shapes-that-matched)).
No unit function is naked asm.

### What is left

| Function | Score | Remaining difference and what was tried |
| --- | ---: | --- |
| `004181b0` HelpWindow_Initialise | 98.17 | Operands: Image_LoadBMPScaled (`0047d9d0`) has no annotated port entry, so the call goes to `texture.cpp`'s `image_load_bmp_scaled` adapter. One code difference in each of the three underscore loops: the original reads the first character through the copied pointer (`mov ecx,eax; mov al,[ecx]`), the rebuild through the original (`mov al,[eax]`). Five loop forms (`for`, `while`, `c = *++s`, comma `for`, declaration order) give the same code |
| `00418450` HelpWindow_LoadInfos | 96.08 (98.69 single-function) | Operands only: legoGlobs' type counts (`0055627c`, `00556280`, `00556288`) are read at `legoGlobs_config` +0x1bc, as `lego_unit.c` does, because the port's counts are `entity.cpp`'s C++ `s_catalog_counts`. In the full build the rebuilt address falls in `__pioinfo`, so the operand name differs |
| `004185f0` HelpWindow_LoadButtons | 96.03 | Operands only: Image_LoadBMPScaled and ToolTip_SetContent (`0046b520`) go to unannotated adapters |
| `00418830` HelpWindow_RecallDependencies | 95.71 | Operands only: the three Dependencies.c calls go to `PORT-ONLY` adapters (see [Callees](#callees)) |
| `00418920` HelpWindow_ListDependencies | 96.00 | Operands only: the two Dependencies.c calls go to adapters |
| `00418a00` HelpWindow_Update | 91.51 | Operands: Image_DisplayScaled (`0047dcb0`) and Font_PrintF (`0047a180`) go to adapters. Code: the first message's x coordinate is read from the global `textX`, which the rebuild propagates into `pos.x`; the original reads `pos.x`. That adds two bytes to each of the three tail-merged calls, so their jumps differ. Tried: `pos.x` before and after `pos.y`, after the counting loop, as a struct copy of the text position (80.51), and the permuter (no candidates) |
| `00418da0` HelpWindow_CheckButtons | 88.75 | The original loads the Next button's width and height before the first comparison and spills the height; it also keeps the two flag clears as two `and`s. The rebuild folds them into `and al, 0xcf` and loads the sizes later. Tried: size locals (76-85), an inlined point-in-rectangle helper (37-43), the conversion of `mouseX` as an expression instead of a local, reversed and negated comparisons, and the permuter (no candidates) |
| `00419100` HelpWindow_DrawButtons | 96.55 | Operands only: Image_DisplayScaled |

## Link names and caller switches

### Link names

As in findings 581, 585 and 590, `helpwindow_link_names.h` renames seven of
the unit's definitions to the port's names, under `#if defined(_MSC_VER) &&
_MSC_VER < 1200`. Only `helpwindow.c` includes it, first. Every matching-build
caller of these names passes the original's arguments:

- `lego_unit.c` (Lego.c): `selection_mode_state_set` (the briefing font),
  `selection_state_flag_clear_bit0`, `help_window_initialise_adapter`
  (config, game name), `mode_flag_pair_apply_if_bit2`,
  `help_window_button_hit_test` (mouse, buttons, elapsed) and
  `main_sim_state_admission_predicate`;
- `legoobject_unit.c` (LegoObject.c), `lego_port.cpp` and
  `legoobject_port.cpp`: `building_upgrade_state_changed_dispatch`
  (type, index, level, no-display). The port's definition is in namespace
  `lego` but declared `extern "C"` (`upgrades.hpp`), so the C++ callers
  reach the same symbol.

`helpwindow_port.cpp` compiles its definitions of the seven names out of the
matching build behind `LRR_HELPWINDOW_PORT_NAMES` (set by an explicit `#if`,
finding 590). Natively the port tail keeps serving them.
`help_window_initialise_adapter` was the `PORT-ONLY` adapter that called the
port's `help_window_resources_init(config, NULL)`.

The other twelve functions keep the unit's names. Their only callers outside
the unit are port models with another contract (`frameloop.cpp`'s
`help_window_resources_init(cfg, NULL)`, `frontend.cpp`'s
`ledger_stub_00418a00_OverlayTextBufferRefresh`) or `test_frontend.cpp`,
which tests the port's model.

### Caller switches

Two `lego_unit.c` calls are switched, with their declarations in
`lego_unit_imports.h` replaced one for one:

- `004232e0` `lego_engine_tick` calls `help_window_update()` instead of
  `overlay_text_buffer_refresh()`. The original HelpWindow_Update returns
  nothing; the port's returns `int`, which `frontend.cpp` passes on, so a
  link name would have handed that caller an undefined `eax`. The caller's
  score is unchanged (97.02).
- `0042e020` `lego_object_list_load_file` calls
  `building_upgrade_state_changed_dispatch` instead of
  `building_upgrade_state_changed_dispatch_original`, the `PORT-ONLY`
  adapter in `lego_port.cpp` that forwarded to the namespaced port entry.
  The adapter is deleted.

Natively nothing calls `lego_unit.c`'s entry points (`lego_port.cpp`, "natively
nothing calls lego_game_config_init"), so neither switch changes native
behaviour. Two Lego.c functions rose in the matching build, the config
loader through the `help_window_initialise_adapter` link name and the
object-list loader through the switch:

| Address | Function | Before | After |
| --- | --- | ---: | ---: |
| `0041fb50` | `lego_game_config_init` | 93.74 | 93.77 |
| `0042e020` | `lego_object_list_load_file` | 68.44 | 68.59 |

## How the conversion is done

### The port tail

The port's help window is another contract:

- resource-op and overlay hook tables (`FeHelpWindowResourceOps`,
  `OverlayTextBufferOps`);
- the selection band's state words, which `frontend.cpp` reads through the
  frozen `frontend_from_helpwindow.hpp` (`s_sel_state`, `s_sel_handles_a`,
  `s_help_info_strings`, …);
- module-owned copies of the config text.

Native code (`frameloop.cpp`, `frontend.cpp`) and `test_frontend.cpp` use it.
As in findings 570, 582, 585 and 590, `helpwindow.cpp` is renamed
`helpwindow_port.cpp` and becomes a `tail` entry. Each `// FUNCTION` and
`// GLOBAL` becomes a `/* Port model of 0x… */` comment. The port's naked-asm
copy of `00418800` is dropped; its C form remains.

### State

The unit's `.bss` (`004db8e8`-`004dc654`, 3,436 bytes) is one
`static HelpWindow_Globs helpwindowGlobs`, annotated at `004db8e8`:

- the font `+0x000`; the text area's x, y and bottom `+0x004`-`+0x00c`;
- the background image and its position `+0x010`-`+0x018`;
- two 0x1c-byte buttons `+0x01c` (area, highlight and pressed images,
  tool tip);
- 0x38 bytes at `+0x054` (`004db93c`) that no code reads or writes, kept as
  padding (both button loops end at `004db93c` and `004db940`);
- the text window `+0x08c`;
- the three `[15][16]` info-text tables `+0x090`, `+0x450`, `+0x810`;
- the three messages, the current text, the object `+0xbd0`-`+0xbe8`;
- the dependency lists `[30]` and count `+0xbec`-`+0xd54`;
- the scroll position and limit, the highlighted and pressed buttons and the
  flags `+0xd58`-`+0xd68`.

The `.rdata` floats (`0049e230`-`0049e258`) are literals. The unit's nine
strings (`004a2798`-`004a283c`) are marked `// STRING` where used. `","`
(`004a13e4`) and `"%s"` (`004a1d60`) are pooled with GAME-001 and GAME-009
and stay unmarked.

`make unit-data-offsets`, GAME-018 row: `.data` gaps-only (9, 0 unknown),
`.rdata` n/a, `.bss` n/a (one item).

### Callees

- **Existing entries with C linkage**: Config.c (`string_path_join`,
  `block_get_str_copy`), Utils.c (`sound_variant_split`,
  `formatted_text_escape_decode_and_emit`), TextWindow.c (`004850f0`,
  `00485230`, `00485390`, `004859b0`), Font_GetHeight (`0047a540`),
  Image_SetupTrans (`0047dbf0`), Interface.c's `0041cb90`, Lego.c's
  `00425c30`, `004262e0`, `00428800`, `legoobject_the_name_get`
  (`00439790`), NERPs' `00456240` and ToolTip_Activate (`0046b770`).
- **Existing unannotated adapters**: Image_LoadBMPScaled and
  Image_DisplayScaled (`texture.cpp`), Font_PrintF (`bitmap_font.cpp`'s
  `font_print_f`), ToolTip_SetContent (`frontend.cpp`'s
  `tooltip_set_content`).
- **Five new `PORT-ONLY` adapters** at the end of `helpwindow_port.cpp`, for
  Dependencies.c (GAME-010, `dependencies.cpp`), whose entries have C++
  linkage. `0040b000` and `0040b1b0` are in namespace
  `unported_ledger_stubs`, and the three flag accessors (`0040b250`,
  `0040b2a0`, `0040b2e0`) are declared without `extern "C"` in the frozen
  `level_for_*.hpp` headers, so `extern "C"` cannot be added in place. The
  first two adapters forward to the annotated entries. The flag adapters
  forward to `level.hpp`'s public wrappers, which add the dependency grid's
  override and bounds checks, as the port's own model of `00418830` does.
  GAME-010's conversion can replace them.

### Source shapes that matched

- **`00418800`** (64.00% → 100% in C). The original sets or clears the
  active bit, then clears the showing bit after the `if`:
  `if (show) flags |= 2; else flags &= ~2; flags &= ~1;`. MSVC duplicates
  the shared clear into both arms, which is the original's `or al,2; and
  al,0xfe` / `and al,0xfd; and al,0xfe`. Two separate statements per arm
  (the port's form) fold the false arm into `and al,0xfc`; that is why the
  port used naked asm.
- **`00418f80`** (43.48% → 100%). The original's jump table has entries
  for buttons 2 and 3 that do nothing. MSVC 5.0 drops empty cases before it
  chooses a jump table, so `case 2: case 3: break;` compiles to a compare
  chain. A statement in those cases keeps them. The unit stores 0 to the
  parameter there, marked `Deviation (scoring only)`. A dead local and an
  `if (button) break;` also match; the store adds no declaration.
- **`004191a0`** (88.24% → 100%). The flag locals in the original's order:
  `active = flags & 2` before `showing = flags & 1`.
- **`004181b0`** (84.51% → 98.17%). The underscore loops test a `char`
  local (Ghidra's `cVar2`): `for (s = msg; (c = *s) != '\0'; s++)`.
- **`00418a00`** (84.56% → 91.51%):
  - the pointer position for `004262e0` is block-scoped, so it shares the
    stack slot of the unsigned-to-float conversion (frame `0x14`);
  - the count tests are `> 0` / `<= 0` on the unsigned count (`ja`/`jbe`);
  - the counting loop is a `switch` (`dec eax; je; sub eax,3`);
  - `pos.x` is set before `pos.y`.
- **`00418da0`** (84.04% → 88.75%). An unsigned loop index (`jb`), and the
  button sums parenthesised: `msx <= (area.x + area.width)`. MSVC 5.0 then
  loads `msx` after the sum and compares with `fcompp`, as the original
  does; unparenthesised, it compares the sum with `fcomp [msx]`.
- **`00419100`** (93.10% → 96.55%). An unsigned loop index (`jb`).

## `.bss`

Finding 584 listed `helpwindow.cpp` as the second-largest `.bss` object:
742,472 bytes, of which `s_help_info_string_storage[3][240][1024]`
(737,280 bytes) held a 1 KB copy of each `HelpWindowInfo` line.

The port's loader (`help_window_info_config_strings_load_native`) now keeps
an exact-length heap copy of each line, with the same 1,023-character limit
and the same underscore replacement, and frees the previous copies when it
loads again. `s_help_info_strings` and its readers are unchanged. The only
difference is when an allocation fails: that line has no text, where the
fixed slot always had room. The original keeps a heap copy per line too
(Config_GetStringValue, `00479050`).

| Object | before | after |
| --- | ---: | ---: |
| `helpwindow.cpp.obj` / `helpwindow_port.cpp.obj` | 742,472 | 8,064 |
| `helpwindow.c.obj` (the original's `.bss`) | — | 3,436 |
| Image `.bss` (relink) | 3,831,232 | 3,100,256 (−730,976) |

The rest of the port tail's `.bss` is the 256-byte label and button-text
copies and the pointer tables (`s_help_info_strings`, 2,880 bytes; its copy
table, 2,880 bytes).

## `opt_ref` (finding 579, `make opt-ref-callers`)

| | before (#631 `9d8a27a0`) | This change |
| --- | ---: | ---: |
| `opt_ref.annotated.full` (by address) | 497 / 233 | 497 / 233 |
| `opt_ref.annotated.game_only` (by address) | 480 / 222 | 480 / 222 |
| By symbol (`callers --min 1`) | 479 / 215 | **477 / 214** |
| GAME-018's object | `helpwindow.cpp.obj`: 1 removed (`004185f0`) | `helpwindow.c.obj`: 0 removed |
| By address, on `12a0e950` | 498 / 229 | 493 / 224 |

By symbol, two functions are newly kept: the unit's `004185f0`, which the
port's `help_window_resources_init` never called, and `legoobject_unit.c`'s
exact `00439790` `legoobject_the_name_get`, which HelpWindow_Update calls.
Nothing is newly removed.

The by-address count does not move on this base. It also counts alias rows,
whose rebuilt address happens to equal a removed port symbol's address
(finding 579's caveat). They shift with layout: 18 such rows on #631, 20
here. Four rows are new (`0041f8e0` `noop_callback`, `00431560`,
`00437440`, `0047fe20`) and two are gone (`0043baf0`, `00473680`). None of
them is a GAME-018 function. On `12a0e950` the same change moved the
by-address count from 498 / 229 to 493 / 224.

## Entropy and drops

`make reccmp-check` against #631's baseline: 15 improved, **none below
baseline**, no exact match lost, no function newly mapped or unmapped. No
function outside the unit changed its score other than the two Lego.c
callers above, and none changed its instruction count. One
`Deviation (scoring only)` is added (`00418f80`); it changes no storage.

## Files outside the unit

| File | Change |
| --- | --- |
| `src/game/lego_unit.c` | the two caller switches above |
| `src/game/lego_unit_imports.h` | the two declarations replaced one for one |
| `src/game/lego_port.cpp` | `building_upgrade_state_changed_dispatch_original` deleted |
| `src/game/test_c_boundaries.cpp` | includes `helpwindow.h` and `helpwindow_imports.h` |
| `src/link_order.cmake`, `src/game/meson.build` | `unit GAME-018 game/helpwindow.c`; `tail game/helpwindow_port.cpp` |

No file another thread owns is edited (`building_model.cpp` #623, now merged;
`gods98/viewports.cpp` #624; `lighteffects.cpp` #625;
`gods98/directdraw.cpp` #629). `frontend_for_helpwindow.hpp`,
`frontend_from_helpwindow.hpp` and `upgrades_for_helpwindow.hpp` are frozen
and unchanged.

## Native behaviour

Unchanged, apart from the storage of the info-text copies above:

- the port tail serves every port name natively, and no definition other
  than the info-text loader changed;
- the unit compiles natively as C++ under its own names; its callers,
  `lego_unit.c` and `legoobject_unit.c`, are not reached natively.

There is no new native gap.

## Validation

On #631's head (`9d8a27a0`) plus this change, from fresh build directories:

- `make reccmp-check`: OK. None below baseline and no exact match lost. The
  plain-immediate check (finding 584), `winapi-link-check` and the link-order
  map check pass.
- `make match-progress` (baseline, listing digests, `binary-progress.json`),
  `make unit-data-offsets`, `python3 tools/translation_units.py build` and
  `tools/binary_progress_hash.py --write-manifest`.
- `make tools-test`, without `--allow-growth`. The lint ledger shrinks from
  1,283 to 1,276: the seven static helpers of the relocated file are now in
  the exempt tail.
- Native build and `ctest` pass (37/37, synthetic fixtures).
- `gcc -std=gnu89 -Werror=declaration-after-statement -fsyntax-only` on
  `helpwindow.c`: clean.

## Follow-ups

- **Dependencies.c (GAME-010).** Its conversion can give `0040b000`-`0040b2e0`
  C linkage and replace the five adapters, which removes the operand
  differences in `00418830` and `00418920`.
- **Images.c (G98-011), Fonts.c (G98-008), ToolTip.c (GAME-046).** Annotated
  original-ABI entries for Image_LoadBMPScaled, Image_DisplayScaled,
  Font_PrintF and ToolTip_SetContent would remove the rest of the operand
  differences.
- **`entity.cpp`'s type counts.** With C linkage, `00418450` (and
  `lego_unit.c`) could read `s_catalog_counts` directly.

## Reproducing

    git mv src/game/helpwindow.cpp src/game/helpwindow_port.cpp   # and make it a tail entry
    python3 tools/convert_unit.py GAME-018 --stem helpwindow
    make match-function SOURCE=src/game/helpwindow.c
    make reccmp-check
    make opt-ref-link opt-ref
    make opt-ref-callers OPT_REF_ARGS="--min 1"
    make match-progress
    make unit-data-offsets
