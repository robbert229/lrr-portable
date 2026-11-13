# Finding 597 — ToolTip.c (GAME-046) rebuilt in original shape, with a port tail; Upgrade.c's part functions split off

Issue #632, part of #388 under [ADR 0005](../adr/0005-original-shape-c-units.md).
GAME-046 is the original's `ToolTip.c`: one record per tool tip type, each
holding a text or an image plus rows of icons. A caller sets a tip's content
and activates it every frame the mouse is over its owner. Once the tip has
been active for the hover time, ToolTip_Update draws it beside the mouse and
plays its sound once.

#496 relocated the unit verbatim into `src/game/tooltip.cpp`. This change
rebuilds it as the C unit [`src/game/tooltip.c`](../../src/game/tooltip.c),
with:

- the unit's C header [`tooltip.h`](../../src/game/tooltip.h);
- the unit-private imports header
  [`tooltip_imports.h`](../../src/game/tooltip_imports.h);
- the link names [`tooltip_link_names.h`](../../src/game/tooltip_link_names.h).

The port's implementation moves, unannotated, to the port tail
[`tooltip_port.cpp`](../../src/game/tooltip_port.cpp).

The unit map gave GAME-046 16 functions, `0046b1d0`-`0046c160`. **The last
five (`0046c030`-`0046c130`) are not ToolTip.c's**: they are Upgrade.c's part
functions (GAME-047; see [Unit extent](#unit-extent)). They are rebuilt in
original shape at the top of GAME-047's relocated file,
[`upgrade_levels.cpp`](../../src/game/upgrade_levels.cpp), and the unit map
now starts GAME-047 at `0046c030`. This change covers all 16 addresses that
issue #632 listed.

"Before" is the *match baseline* on `main` at `12a0e950` (#627). "After" is
this change, from the full matching build (`make reccmp-check`).

## Result

| | before | after |
| --- | ---: | ---: |
| ToolTip.c similarity (byte-weighted, 11 functions, 3,680 bytes) | 12.79% | **77.12%** |
| Upgrade.c part functions (byte-weighted, 5 functions, 304 bytes) | 34.18% | **96.26%** |
| All 16 addresses of issue #632 (3,984 bytes) | 14.42% | **78.58%** |
| Exact functions of the 16 | 1 | **11** |
| Mapped functions of the 16 | 15 | 16 |
| Exact functions, full / game-only (`binary-progress.json`) | 1,490 / 1,290 | **1,503 / 1,303** |
| Exact-function code bytes | 169,567 (26.39%) | **172,164 (26.79%)** |
| Accuracy, full / game-only | 81.32% / 79.49% | 81.66% / 79.87% |
| `opt_ref` annotated (full, by address): removed / exact removed | 498 / 229 | **497 / 230** |
| `opt_ref` annotated (game-only, by address) | 481 / 218 | 480 / 219 |
| Reccmp lint ledger (entries) | 1,290 | **1,286** |

The weights are the functions' spans up to the next function, padding
included. An unmapped function counts as 0%.

The unit's names follow OpenLRR's `ToolTip_*` and `Upgrade_Part_*`
functions, in the repository's snake case. "Link name" is the name the
unit's definition takes in the matching build (see
[Link names](#link-names)).

| Address | Function | Link name | Bytes | Before | After |
| --- | --- | --- | ---: | ---: | ---: |
| `0046b1d0` | `tooltip_initialise` | `tooltip_overlay_state_initialize` | 768 | 9.52 | **100.00** |
| `0046b4d0` | `tooltip_get_type` | `case_insensitive_name_table_lookup` | 80 | 55.42 | **100.00** |
| `0046b520` | `tooltip_set_content` | `tooltip_set_content` | 320 | unmapped | 96.74 |
| `0046b660` | `tooltip_add_icon` | `command_sample_stats_accumulate` | 160 | 10.00 | **100.00** |
| `0046b700` | `tooltip_set_sfx` | `timed_effect_cue_request_set_corpus` | 48 | 10.00 | **100.00** |
| `0046b730` | `tooltip_set_flag10` | (unit name) | 64 | 37.04 | **100.00** |
| `0046b770` | `tooltip_activate` | `timed_effect_update_request_set_corpus` | 48 | 75.00 | **100.00** |
| `0046b7a0` | `tooltip_reset_timer` | (unit name) | 32 | 100.00 | **100.00** |
| `0046b7c0` | `tooltip_update` | `timed_effect_runtime_update` | 240 | 21.05 | 57.14 |
| `0046b8b0` | `tooltip_draw_box` | `tooltip_draw_box` | 896 | 7.97 | 34.47 |
| `0046bc30` | `tooltip_draw` | (unit name) | 1024 | 11.59 | 86.16 |
| `0046c030` | `upgrade_part_load` | `render_resource_binding_initialize` | 128 | 14.49 | 91.11 |
| `0046c0b0` | `upgrade_part_clone` | `render_resource_binding_copy_and_convert` | 64 | 57.14 | **100.00** |
| `0046c0f0` | `upgrade_part_remove` | `render_resource_binding_release` | 32 | 11.76 | **100.00** |
| `0046c110` | `upgrade_part_move_animation` | (unit name; caller switched) | 32 | 20.69 | **100.00** |
| `0046c130` | `upgrade_part_set_activity` | `scene_object_set_animation_with_fallback` | 48 | 80.00 | **100.00** |

`0046b520` (ToolTip_SetContent) had no annotated source before. frontend.cpp
had an unannotated text-only adapter under the same name.

Callers outside the unit that rose, with no source change to them unless
noted:

| Address | Function | Before | After |
| --- | --- | ---: | ---: |
| `0041a300` | `interface_initialise` (caller switch, below) | 99.08 | **100.00** |
| `0041b1b0` | `interface_load_back_button` | 87.27 | 89.09 |
| `0041baa0` | `interface_do_mouse` (caller switch) | 73.30 | 74.21 |
| `00425d90` | `lego_terrain_pointer_command_update` | 87.92 | 88.26 |
| `00428020` | `lego_entity_command_status_feedback_dispatch` | 98.52 | 98.89 |
| `00428330` | `lego_terrain_command_feedback_dispatch` | 71.50 | 72.03 |
| `00434da0` | `lego_advisor_tooltip_and_sound_config_load` | 98.78 | **100.00** |
| `00447b30` | `legoobject_activity_tick` (caller switch) | 98.86 | **100.00** |
| `0045d110` | `priorities_handle_cursor` | 50.16 | 50.81 |
| `00461790` | `reward_draw_item` | 92.46 | 93.03 |

### What is left

| Function | Score | Remaining difference and what was tried |
| --- | ---: | --- |
| `0046b7c0` ToolTip_Update | 57.14 | The loop's induction pointer. The original strides a pointer to each record's `flags` (`+0x278`) and reaches the timer and sound at `-0x278`/`-4`. This compile strides the record's start, which frees a register for `mouseX` and shifts every other register. The same code came from five source shapes: indexed `toolTipGlobs.toolTip[i]` throughout, a record pointer at the loop top, one in the update branch only, a pointer loop, and `unsigned` `i`. `msvc_permuter.py` (120 s) had no candidates. Dropping the timer references moves the base to the lowest other offset, so MSVC seems to base the pointer on the lowest offset the loop reads; the original does not. |
| `0046b8b0` ToolTip_DrawBox | 34.47 | x87 scheduling of the border loop. The original keeps 13 scalar slots (frame `0x174`) where this compile keeps 15 (`0x17c`), and it computes the `-1` edges before the `-2` ones. Four store orders scored 31.91-34.47: the decompile's order, line by line, high and low pairs, and copies from the first point (kept). `msvc_permuter.py` (240 s) made one attempt, with no gain. |
| `0046bc30` ToolTip_Draw | 86.16 | MSVC adds the two `unsigned` border conversions together first (`fild; fiadd`). The original keeps both on the x87 stack and adds them to `box.x` and `box.y` one at a time. Explicit casts, parentheses and both operand orders compile alike. Two other changes took it from 54.58: `height = width = …` (83.36), and `box.width` assigned after the off-screen test (74.39 from 60.35). The call operands of `font_print_f` and `image_display_scaled` are adapters (below). `msvc_permuter.py` (180 s): 4 attempts, no gain. |
| `0046b520` ToolTip_SetContent | 96.74 | Call operands only: `vsprintf` (Reccmp names the original's `0048e1e0` `formatted_text_buffer_build`, as in `utils.c`), and the `image_load_bmp_scaled` and `font_get_string_info` adapters. |
| `0046c030` Upgrade_Part_Load | 91.11 | Call operands only: the three Container_Load calls go to `gods98/entity.cpp`'s `container_load` adapter, as in Building.c and Vehicle.c, and `"MESH"` (`004a4014`, pooled with GAME-021) has no marker yet. |

No naked asm, and no `Deviation (scoring only)`. No unit function uses the
flag `0x800000`, and #608's plain-immediate check passes.

## Unit extent

The unit map's high-confidence boundary at `0046c160` is one object too
late. ToolTip.c is the 11 functions `0046b1d0`-`0046c030`. `0046c030`-
`0046c130` belong to Upgrade.c (GAME-047), which runs to `0046c6f0`.

- **The literal pool decides it.** MSVC 5.0 emits each float literal once
  per object. ToolTip.c's functions share theirs:
  - `1.0f` at `0049e7c8` is used by `0046b1d0`, `0046b8b0` and `0046bc30`;
  - ToolTip_Initialise clamps against `0.0f` at `0049e7cc`.

  `0046c110` returns `0.0f` from a second copy at `0049e7dc`, so it was
  compiled in another object. Building.c (`0049e084`) and Vehicle.c
  (`0049e7e4`) show the same per-object copies.
- **The record.** `0046c030`-`0046c130` work on a 0x10-byte record: the
  object type at `+0x00` and the container at `+0x04`. That is Upgrade.c's
  part model:
  - Lego.c's UpgradeTypes loader (`0042d460`) fills it at a 0x10 stride
    through `0046c030`;
  - LegoObject.c clones it (`00437d00` → `0046c0b0`), removes it
    (`00437540` → `0046c0f0`) and animates it (`00447b30` → `0046c110`);
  - Building.c (`004083a0`) and Upgrade.c's own `0046c510` set its
    activity (`0046c130`).

  No tool tip function calls them, and they touch no ToolTip.c state.
- **ToolTip.c's own data** is unchanged:
  - `.rdata` `0049e7c0`-`0049e7d8` (`25.0f`, `0.4f`, `1.0f`, `0.0f`,
    `2.0f`, `-1.0f`, `-32.0f`);
  - `.data` `004aa64c` (the sound gate);
  - the 38 `"ToolTip_*"` names, `004aa650`-`004aa9c8`;
  - the state, `0054bf20`-`005520e4`.

  Upgrade.c's strings (`004aa9dc`-`004aa9e8`) follow the names.

`translation-units.spec.csv` records this evidence. GAME-047's start moves
to `0046c030`. `python3 tools/translation_units.py build`: 0 conflicts.
GAME-046 keeps `high` confidence. The five Upgrade.c functions are no
longer the medium-confidence "boundary:GAME-047" row.

## How the conversion is done

### State

ToolTip.c's state is one `ToolTip_Globs toolTipGlobs` (`0054bf20`, 0x61c4
bytes) in the common region:

- the font and its height;
- the border size and the offset;
- the screen size and the y offset;
- the hover time, `25 * scale`;
- the colours;
- the 39 name pointers (`0054bf64`; index 0 is unused, so
  ToolTip_GetType's walk starts at `0054bf68`);
- 39 `ToolTip` records of 0x27c bytes from `0054c000`.

ToolTip_Initialise clears the 39 records (`0x1839` dwords).

The colours are stored by channel: red, green and blue, each with its
normal, highlight (`+40%`) and shadow (`-40%`) value. ToolTip_Draw reads
the box's three colours across the channels. The `.data` word `004aa64c`
(`toolTip_sfxReady`, initially 1) gates the hover sound.

`frontend.cpp`'s `// GLOBAL` annotations at `0054c000`
(`s_timed_effect_records`) and `0054bf3c` (`s_timed_effect_tick`) are
removed, and so is `tooltip_port.cpp`'s at `004aa64c`. Those are the port's
own copies; the unit now owns the addresses.

`make unit-data-offsets`, GAME-046 row:

| Unit | .data | .rdata | .bss | Commons |
| --- | --- | --- | --- | --- |
| GAME-046 | gaps-only (39, 0 unknown) | n/a (0, 0 unknown) fewer than two attributable items | n/a (0, 0 unknown) fewer than two attributable items | `0054bf20`:`_toolTipGlobs` (size 25028) |

Unit summary: full 0, gaps-only 6, partial 29, n/a 190.

The `.rdata` literals are literals in the code. The 38 names are marked
`// STRING` where ToolTip_Initialise stores them. `"%s"` (`004a1d60`,
GAME-009) and Upgrade.c's `"LWO"`, `"ACT"` and `"MESH"` are pooled with
other units and stay unmarked.

### The port tail

The port's tool tips are another contract:

- the `TimedEffectRecord` table and `TooltipOverlayState` snapshot that
  `frontend.cpp` owns;
- the runtime and render hook tables;
- bounds and NULL guards.

Native code (`frontend.cpp`, `effects.cpp`, `frameloop.cpp`, `mission.cpp`,
`lego_port.cpp`, `reward_port.cpp`) and `test_cmd.cpp` and
`test_frontend.cpp` use it. As in findings 585, 590 and 592,
`tooltip.cpp` is renamed `tooltip_port.cpp` and becomes a `tail` entry.
Each `// FUNCTION` becomes a `/* Port model of 0x… */` comment.

### Link names

In the matching build, `tooltip_link_names.h` gives six unit functions the
port's names:

- `0046b1d0` `tooltip_overlay_state_initialize`;
- `0046b4d0` `case_insensitive_name_table_lookup`;
- `0046b660` `command_sample_stats_accumulate`;
- `0046b700` `timed_effect_cue_request_set_corpus`;
- `0046b770` `timed_effect_update_request_set_corpus`;
- `0046b7c0` `timed_effect_runtime_update`.

Every C caller of these names passes the original's arguments, so the C
callers and the C++ ones reach the unit with no caller edit:

- `lego_unit.c` (tip types, the font and colours, `Image`s, sound ids);
- `interface_unit.c`, `panel_unit.c`, `priorities.c`;
- `reward.c` (the mouse position and frame time).

`tooltip_port.cpp` compiles its definitions of these names out of the
matching build behind `LRR_TOOLTIP_PORT_NAMES`, set by an explicit `#if`
(finding 590).

Upgrade.c's part functions do the same inside `upgrade_levels.cpp`, which
takes four port names (`render_resource_binding_initialize`, `_copy_and_convert`,
`_release`, `scene_object_set_animation_with_fallback`). Their callers
(`lego_unit.c`, `legoobject_unit.c`, `building_model.c`) pass a part of the
original's layout. The four definitions sit in an `extern "C"` block and use
`view.hpp`'s `RenderResourceBinding`, which has the part's layout, so the
C++ redeclarations agree.

**Two unit names were already port names.** ToolTip_SetContent's name,
`tooltip_set_content`, was `frontend.cpp`'s text-only adapter, which the C
units call. ToolTip_DrawBox's name, `tooltip_draw_box`, was
`tooltip_port.cpp`'s adapter, which drew nothing; Reward.c calls it.

- In the matching build both adapters are compiled out, and the unit's
  definitions serve the names.
- Natively the adapters keep the names, and `tooltip_link_names.h`'s `#else`
  renames the unit's two definitions (`tooltip_unit_set_content`,
  `tooltip_unit_draw_box`). The unit's state is not the port's, so native
  callers must not reach it.

**Three functions keep the unit's names in every build.**

- `tooltip_draw` (`0046bc30`): its port name,
  `timed_effect_slot_render_record`, takes the port's record and hooks.
- `tooltip_set_flag10` (`0046b730`) and `tooltip_reset_timer` (`0046b7a0`):
  their port names (`timed_effect_persistence_set_corpus`,
  `timed_effect_timestamp_reset_corpus`) have C++ linkage in the frozen
  `frontend_for_*.hpp` headers. The unit's C definitions cannot serve
  `frontend.cpp`'s C++ calls, so the port tail keeps serving them in every
  build.

### Caller switches

| File | Edit | Effect |
| --- | --- | --- |
| `interface_unit.c` (no owner) | `tooltip_set_persistent(4, 1)` becomes `tooltip_set_flag10(4, 1)`. `tooltip_reset_timer` stays: it now names the unit's function. | `0041a300` 99.08 → 100, `0041baa0` 73.30 → 74.21 |
| `interface_callees.h` | The two declarations are renamed one for one. The declaration count does not change (finding 538). | — |
| `interface_port.cpp` | Its forwarders `tooltip_set_persistent` and `tooltip_reset_timer` are deleted. They forwarded to the port names, and natively the second would collide with the unit's definition. The native port does not reach `interface_unit.c`'s functions. | — |
| `legoobject_unit.c` (no owner) | `00447b30` calls `upgrade_part_move_animation` instead of `legoobject_gq_upgrade_move_animation_adapter`, and its declaration is renamed one for one. The contract is the same: part, elapsed time, repeat count. | `00447b30` 98.86 → **100** |
| `legoobject_port.cpp` | The `PORT-ONLY` adapter for `0046c110`, which returned 0, is deleted. | — |
| `frontend.cpp` | Its `tooltip_set_content` adapter is compiled out of the matching build, and two `// GLOBAL` annotations are removed (above). | — |

No Lego.c file is edited. No file owned by #624/#630
(`gods98/viewports.cpp`), #625 (`lighteffects.cpp`), #628
(`helpwindow.cpp`) or #629 (`gods98/directdraw.cpp`) is edited.

### Callees

- **Font_GetStringInfo** (the Font.h inline at `00401c80`). Advisor.c's
  entry at that address keeps the port's render-target contract. A
  `PORT-ONLY` adapter, `font_get_string_info`, with the original's four
  arguments, is added at the end of `gods98/bitmap_font.cpp`, after
  `font_print_f`. It is declared only in `tooltip_imports.h`, so no shared
  header's declaration count changes.
- **Font_PrintF, Image_LoadBMPScaled and Image_DisplayScaled.** The unit
  uses the existing original-ABI adapters `font_print_f`,
  `image_load_bmp_scaled` and `image_display_scaled`.
- **Container_Load.** Upgrade_Part_Load uses `gods98/entity.cpp`'s
  `container_load` adapter.
- **Exact callees.** The unit calls Draw_RectListEx and Draw_LineListEx
  (`draw.c`), SFX_Random_SetAndPlayGlobalSample (`sfx.c`, by its link name)
  and Font_GetHeight (`object_field_be4_dword_get`). Upgrade.c's part
  functions call Container_Clone, Container_Remove,
  Container_ForwardAnimation and Container_SetActivity by their port names.
- **`legoGlobs.flags2`** (`00556ed0`) is read through `lego.h`'s
  `s_panel_highlight_flags`. Its `0x100000` bit mutes the tool tip sound.

No `extern "C"` was added to any existing declaration.

### Source shapes that matched

- **ToolTip_Initialise** (`0046b1d0`, 97.16% → 100%): the hover time is
  computed after the five integer settings. The compiler then reuses
  `borderSize`'s dead argument slot for the blue `0.4f` product, as the
  original does. The colour loop counter is `unsigned`.
- **ToolTip_GetType** (`0046b4d0`): an `unsigned` counter gives the
  original's `jb` on the strided name pointer.
- **ToolTip_AddIcon** (`0046b660`): `iconsHeight += iconRowHeight` after
  `iconRowHeight = image->height`, not `+= image->height`.
- **ToolTip_SetContent** (`0046b520`): `for (t = textBuffer, s = buff; …;
  t++, s++)`.
- **Upgrade_Part_Load** (`0046c030`): `memset` of the part. The original
  keeps the four zero stores that separate assignments let MSVC drop.
- **Upgrade_Part_SetActivity** (`0046c130`): the fallback activity name in a
  block-scoped local. Without it the two loads swap `eax` and `edx`
  (75.68%, below the port's 80%). `== 0`, braces, an unsized array and
  0-16 inert declarations (a throwaway finding 538 sweep) did not change
  that.

## Entropy and drops

`make reccmp-check` against the baseline at `12a0e950`:

- 24 improved: 14 of the 16 addresses and the ten callers above.
  `0046b520` is newly mapped, and `0046b7a0` stays at 100%;
- **none below baseline**, and no exact match lost.

Instruction counts changed only at the 16 addresses (listing digests):

| Address | before | after |
| --- | ---: | ---: |
| `0046b1d0` | 47 | 141 |
| `0046b4d0` | 39 | 29 |
| `0046b520` | — | 92 |
| `0046b660` | 50 | 39 |
| `0046b700`, `0046b730`, `0046b770` | 11, 15, 9 | 11, 15, 9 |
| `0046b7c0` | 147 | 74 |
| `0046b8b0` | 148 | 251 |
| `0046bc30` | 436 | 286 |
| `0046c030` | 37 | 45 |
| `0046c0b0` | 31 | 22 |
| `0046c0f0` | 13 | 6 |
| `0046c110` | 17 | 10 |
| `0046c130` | 20 | 18 |

Upgrade.c's other relocated functions (`0046c340`-`0046c510`) keep their
scores. The single-function loop scores `0046c3f0` at 18.41% with and
without this change, against 22.59% in the full build: a single-function
parity difference, not a drop.

## `opt_ref` (finding 579, `make opt-ref-callers`)

| | before (`main` `12a0e950`) | after |
| --- | ---: | ---: |
| `opt_ref.annotated.full` (by address): removed / exact removed | 498 / 229 | **497 / 230** |
| `opt_ref.annotated.game_only` (by address) | 481 / 218 | 480 / 219 |

`make opt-ref-callers` for the three objects:

- `tooltip.c.obj`: one removed. `0046b7a0` ToolTip_ResetTimer is exact now,
  but its only original caller, `0041baa0` `interface_do_mouse`, is removed
  itself (finding 579: Interface.c's roots are in Lego.c). That is why
  exact-removed rises by one.
- `upgrade_levels.cpp.obj`: one removed. `0046c3f0` Vehicle_SetActivity is
  GAME-047's own relocated function, and its live caller `00447c40` calls
  `legoobject_port.cpp`'s adapter instead.
- `tooltip_port.cpp.obj`: none.

All five part functions and the other ten tool tip functions are kept.

## Reccmp lint ratchet

1,290 → 1,286 entries (−4, no growth). `frameloop.cpp`'s four software
renderer hooks for the port's tool-tip overlay (`frame_tooltip_pixel`,
`_fill`, `_lines`, `_text`) are port-only and are now marked
`// PORT-ONLY`. `make reccmp-lint-update` removed their entries.

## Native behaviour

Unchanged:

- The port tail and `frontend.cpp` serve every port name natively.
- The two adapters keep their names (above).
- The unit compiles natively as C++ under its own names. Nothing native
  calls them.
- The `interface_port.cpp` forwarders and the `legoobject_port.cpp` adapter
  that were deleted had only `interface_unit.c` and `legoobject_unit.c`
  callers. Those calls now reach the unit's functions, and the native port
  does not run them.

There is no new native gap.

**Matching-build runtime** (`make reccmp-run` only). The unit's state and the
port's are separate. In the matching build:

- callers of the six link names write `toolTipGlobs`;
- `frontend.cpp`'s peeks and `tooltip_port.cpp`'s ToolTip_SetFlag10 and
  ToolTip_ResetTimer models read and write the port's
  `s_timed_effect_records`.

This is the same split that findings 590 and 592 accept until the callers'
units are converted.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox): OK. None below
  baseline and no exact match lost. The plain-immediate check (finding
  584), `winapi-link-check` and the link-order map check pass.
- `make match-progress` (baseline, listing digests, `binary-progress.json`)
  and `make unit-data-offsets`. `tools/binary_progress_inputs.txt` is
  regenerated (`binary_progress_hash.py --write-manifest`).
- `python3 tools/translation_units.py build`: 0 conflicts.
- `make tools-test`, without `--allow-growth`.
- `tooltip.c` passes `gcc -std=gnu89 -Werror=declaration-after-statement
  -fsyntax-only`.
- Native: `cmake -S . -B build -DTEST_WADS_DIR=<shipped WADs> && cmake
  --build build && ctest --test-dir build`: 39/39, real-data tests
  included.

## Follow-ups

- **`0046b7c0`, `0046b8b0`, `0046bc30`**: the induction-pointer, x87
  scheduling and `fiadd` gaps above.
- **GAME-047 (Upgrade.c)**: its conversion takes the five part functions
  from `upgrade_levels.cpp` into the unit's `.c` file with the rest of
  Upgrade.c. It also gives Vehicle_SetActivity (`0046c3f0`) a live caller.
- **Container.c (G98-002)**: its conversion can replace the
  `container_load` adapter (Upgrade_Part_Load's operands).
- **The frozen `frontend_for_*.hpp` headers**: once they are retired, the
  port names of `0046b730` and `0046b7a0` can take C linkage, and the unit
  can serve them as it serves the other six.

## Rebased onto stack #636

This section gives the figures after rebasing onto #633 (`8e25f30e`, HelpWindow.c). That base contains #630 (Viewports.c) and #631 (LightEffects.c). All the per-function scores above are unchanged on the rebase.

`make reccmp-check` against #633's baseline improves 25 functions and newly maps 1 (`0046b520`). The extra improved function is #633's `004185f0` `help_window_load_buttons` (96.03% to 97.35%), a ToolTip_SetContent caller. No function falls below baseline, and no exact match is lost.

| | #633 head | this change |
| --- | ---: | ---: |
| Exact-function code bytes | 173,989 (27.08%) | **176,586 (27.48%)** |
| Exact functions, full / game-only | 1,526 / 1,326 | 1,539 / 1,339 |
| Accuracy, full / game-only | 82.56% / 80.86% | 82.90% / 81.24% |
| `opt_ref` full: removed / exact removed | 497 / 233 | **490 / 228** |
| `opt_ref` game-only | 480 / 222 | 473 / 217 |
| Lint ledger | 1,276 | **1,272** |

`make opt-ref-callers` lists the same two removed functions as on `main`: `0046b7a0` and `0046c3f0`.

## Reproducing

    git mv src/game/tooltip.cpp src/game/tooltip_port.cpp   # and make it a tail entry
    python3 tools/convert_unit.py GAME-046 --stem tooltip
    make match-function SOURCE=src/game/tooltip.c
    make match-function SOURCE=src/game/upgrade_levels.cpp
    make reccmp-check
    make opt-ref-link opt-ref
    make opt-ref-callers OPT_REF_ARGS="--object tooltip.c.obj --object upgrade_levels.cpp.obj --min 1"
    make match-progress
    make unit-data-offsets
