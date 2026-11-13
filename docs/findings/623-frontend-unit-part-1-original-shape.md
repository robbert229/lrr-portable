# Finding 623 — FrontEnd.c (GAME-017) part 1 rebuilt in original shape

Issue #696, part 1 of 2, under [ADR 0005](../adr/0005-original-shape-c-units.md).
GAME-017 is the original's `FrontEnd.c`: the menu sets loaded from
`Lego.cfg`'s `Menu::*` blocks, the menu loop with its rock wipe and
overlay animations, the save-slot screens and the front-end callbacks. It
was relocated verbatim into `src/game/frontend_unit.cpp` and never
converted. This change rebuilds its first range, `004102b0`-`00414de0`
(66 functions, as the issue proposed; see [Split](#the-split)), as the C
unit [`src/game/frontend_unit.c`](../../src/game/frontend_unit.c). Part 2
(`00414df0`-`00418190`, 66 functions) is still the relocated port code.

"Before" is the *match baseline* of `main` `d4c4cf5c`. "After" is this
change, from the full matching build (`make reccmp-check`).

## Result

| | before | after |
| --- | ---: | ---: |
| Part 1 similarity (66 functions, weighted by span, 19,264 bytes) | 23.46% | **71.70%** |
| Exact functions in part 1 | 26 | **38** |
| Mapped functions in part 1 | 61 | **66** |
| Exact-function code bytes (`binary-progress.json`) | 208,965 | **210,467** |
| Accuracy, full / game-only | 89.03% / 87.97% | **89.77% / 88.78%** |
| `opt_ref` annotated (full): removed / exact removed | 398 / 220 | 422 / 238 |
| `opt_ref` annotated (game-only) | 391 / 213 | 415 / 231 |
| `/OPT:REF` relink `.bss` (`opt_ref.image.bss_bytes`) | 1,304,096 | 1,319,984 |
| Gate image end (`make pe-audit`) | `0x892000` | `0x89b000` |
| Reccmp lint ledger (entries) | 1,042 | **1,023** |

Spans run to the next function, padding included; an unmapped function
counts as 0%. Four functions are naked asm (below); everything else is C.
There is no `Deviation (scoring only)` and there are no filler declarations.

The names follow OpenLRR's `Front_*` functions in the repository's snake
case (`inferred`). Five keep the port's names because their callers and
contracts are the original's: `main_menu_text_item` (004103d0, in the
matching build only; see [Link names](#link-names)),
`renderer_word_a_get`/`_b_get`/`_c_get` and `input_key_context_check`, and
`mission_replace_char`.

| Address | Function | Span | Before | After |
| --- | --- | ---: | ---: | ---: |
| `004102b0` | `front_util_replace_text_spaces` | 112 | 100.00 | **100.00** |
| `00410320` | `front_util_print_f` | 176 | 29.03 | 98.21 |
| `004103d0` | `main_menu_text_item` | 32 | 100.00 | **100.00** |
| `004103f0` | `front_rock_wipe_play` (naked asm) | 80 | 84.21 | 89.47 |
| `00410440` | `front_rock_wipe_stop` | 16 | 100.00 | **100.00** |
| `00410450` | `front_cache_find_by_name` | 64 | 100.00 | **100.00** |
| `00410490` | `front_cache_create` | 144 | 88.29 | 98.21 |
| `00410520` | `front_cache_load_image` | 64 | 96.43 | 96.43 |
| `00410560` | `front_cache_load_font` | 64 | 38.24 | **100.00** |
| `004105a0` | `front_util_string_copy` | 80 | 100.00 | **100.00** |
| `004105f0` | `front_menu_item_create_select` | 160 | 100.00 | **100.00** |
| `00410690` | `front_menu_item_free_select` | 176 | 100.00 | **100.00** |
| `00410740` | `front_menu_item_add_select_item` | 720 | 10.07 | 80.74 |
| `00410a10` | `front_menu_item_create_cycle` (naked asm) | 144 | 100.00 | **100.00** |
| `00410aa0` | `front_menu_item_add_cycle_item` | 64 | 32.43 | **100.00** |
| `00410ae0` | `front_menu_item_free_cycle` | 80 | 100.00 | **100.00** |
| `00410b30` | `renderer_word_a_get` | 16 | 100.00 | **100.00** |
| `00410b40` | `renderer_word_b_get` | 16 | 100.00 | **100.00** |
| `00410b50` | `renderer_word_c_get` | 16 | 100.00 | **100.00** |
| `00410b60` | `front_menu_item_create_trigger` | 48 | 100.00 | **100.00** |
| `00410b90` | `front_menu_item_create_real_slider` | 96 | 16.36 | **100.00** |
| `00410bf0` | `front_menu_item_create_slider` | 128 | 100.00 | **100.00** |
| `00410c70` | `front_menu_item_free_menu_item` | 224 | 44.07 | 82.61 |
| `00410d50` | `front_menu_item_create_banner_item` (naked asm) | 208 | 100.00 | **100.00** |
| `00410e20` | `front_menu_item_create_image_item` | 272 | 27.72 | 90.00 |
| `00410f30` | `front_menu_free_menu` | 128 | 23.33 | **100.00** |
| `00410fb0` | `front_menu_load_menu_image` | 336 | 27.20 | 98.32 |
| `00411100` | `front_menu_create_menu` | 352 | 15.09 | 73.53 |
| `00411260` | `front_menu_add_menu_item` | 128 | 13.92 | **100.00** |
| `004112e0` | `front_maths_is_point_inside_rect` | 64 | 100.00 | **100.00** |
| `00411320` | `front_maths_is_point_inside_rect_centered` | 64 | 100.00 | **100.00** |
| `00411360` | `front_menu_item_select_test_string_collision` | 400 | 21.12 | 86.14 |
| `004114f0` | `front_menu_is_level_item_under_mouse` | 64 | 24.69 | **100.00** |
| `00411530` | `front_menu_get_item_bounds` | 608 | 7.71 | 59.50 |
| `00411790` | `front_menu_find_item_under_mouse` | 176 | 75.52 | **100.00** |
| `00411840` | `front_menu_was_mouse_released` | 48 | 100.00 | **100.00** |
| `00411870` | `front_menu_item_slider_handle_input` | 352 | 18.00 | 46.49 |
| `004119d0` | `front_menu_item_check_not_in_tuto_any_tutorial_flags` | 48 | 34.48 | **100.00** |
| `00411a00` | `front_menu_update_menu_items_input` | 1280 | unmapped | 77.40 |
| `00411f00` | `input_key_context_check` | 16 | 100.00 | **100.00** |
| `00411f10` | `front_menu_item_draw_slider` | 608 | 13.42 | 44.38 |
| `00412170` | `front_menu_get_overlay_count` | 32 | 100.00 | **100.00** |
| `00412190` | `front_menu_should_random_play` | 32 | 100.00 | **100.00** |
| `004121b0` | `front_menu_update_overlays` | 672 | 9.03 | 74.94 |
| `00412450` | `front_menu_item_draw_select_item` | 160 | 10.98 | 67.92 |
| `004124f0` | `front_menu_item_draw_save_image` | 608 | 14.36 | 45.73 |
| `00412750` | `front_menu_item_draw_select_text_window` | 640 | 11.74 | 60.62 |
| `004129d0` | `front_menu_draw_load_save_text` | 288 | 8.96 | 77.84 |
| `00412af0` | `front_menu_draw_menu_image` | 272 | 20.69 | 67.27 |
| `00412c00` | `front_menu_update` | 3440 | 8.28 | 45.88 |
| `00413970` | `front_menu_update_mouse_position` | 240 | unmapped | **100.00** |
| `00413a60` | `front_menu_load_slot_images` | 240 | unmapped | 71.01 |
| `00413b50` | `front_menu_free_slot_images` | 48 | 10.96 | 92.31 |
| `00413b80` | `front_run_screen_menu` | 672 | unmapped | 77.20 |
| `00413e20` | `front_screen_menu_loop` | 64 | 11.36 | **100.00** |
| `00413e60` | `front_menu_item_parse_type_string` | 160 | 100.00 | **100.00** |
| `00413f00` | `mission_replace_char` | 48 | 100.00 | **100.00** |
| `00413f30` | `front_menu_overlay_get_type` | 96 | 26.39 | **100.00** |
| `00413f90` | `front_menu_create_overlay` (naked asm) | 128 | 100.00 | **100.00** |
| `00414010` | `front_menu_item_load_slider_images` | 96 | 24.44 | **100.00** |
| `00414070` | `front_menu_set_create` | 80 | 38.55 | 84.06 |
| `004140c0` | `front_load_menu_set` | 3024 | 18.71 | 69.79 |
| `00414c90` | `front_get_menu_id_by_name` | 80 | 100.00 | **100.00** |
| `00414ce0` | `front_is_intros_enabled` | 80 | 25.81 | 95.00 |
| `00414d30` | `front_callback_slider_brightness` | 176 | unmapped | 77.11 |
| `00414de0` | `front_callback_slider_sound_volume` | 16 | 100.00 | **100.00** |

Fifty-three functions rose in the full run: 34 of the unit's and 19
callers in Interface.c (`0041ada0`), Lego.c (`0041fb50`, `004250c0`,
`00429890`, `0042d600`, `0042da20`, `0042e8b0`, `004334f0`, `00433770`,
`00434a00`), Priorities.c (`0045cdc0`) and Reward.c (eight from `0045f290`
to `00462270`); Reward.c's `0045f5f0` is now exact.
The callers rose because `0x004a2360` ".bmp", `0x004a24e4` "Main" and the
other front-end strings are now `STRING`-marked in their own unit and
Reccmp names them in every user (finding 569), and because `00413f00` and
`004103d0` keep their exact definitions under the callers' names.

### What is left

- **The three draw-heavy functions** — `00412c00` Front_Menu_Update
  (45.88%), `00411f10` Front_MenuItem_DrawSlider (44.38%) and `004124f0`
  Front_MenuItem_DrawSaveImage (45.73%) — have the original's structure,
  calls and branch layout. They differ in register and stack-slot choice,
  and Image_DisplayScaled (`0047dcb0`) has no annotated definition, so every
  call to it misses. A hill-climb over the declaration order of
  `00412c00`'s locals raised it from 34% to 49% in `match-function`; the
  same search found nothing in `004140c0`, `00411a00`, `004121b0` or
  `004124f0`.
- **`00411a00` (77.40%)** keeps `menu` in `edi` and the item in `esi`; the
  original swaps them. Declaration order does not move it.
- **Unannotated callees**: Image_LoadBMP (`0047d9d0`), Image_DisplayScaled
  (`0047dcb0`), Image_SaveBMP (`0047e440`), Flic_Setup (`00483c80` is
  annotated on the port's `flic_open`, another contract) and Flic_Animate
  (`00484070`). The unit calls the port's adapters (`images.h`, `flic.h`)
  and an unannotated no-op for Image_SaveBMP in `frontend_port.cpp`.

## Unit extent

The issue marked the boundary low-confidence. **The extent holds:**
`004102b0`-`00418190`, 132 functions, one object.

- **The literal pool.** Findings 599 and 603 apply: MSVC 5.0 emits each
  float literal once per object. `.rdata` `0049e170`-`0049e22c` holds 47
  floats and doubles with no repeated value, and both halves of the unit
  share it: `0.0f` (`0049e170`) is loaded by `00411870`, `00413a60` and
  `004157c0`; `0.5f` (`0049e1b0`) by `00412c00` and `00414e10`; `25.0f`
  (`0049e1a4`) by `00412c00` and `00417430`; `0.025f` (`0049e18c`) by
  `004121b0` and `00413b80`. GAME-016's pool ends at `0049e16c` with its
  own `0.0f`/`1.0f` (`0049e148`/`0049e14c`, used by `0040fcc0` and
  `0040f920`), and HelpWindow.c's starts with another `0.0f` at `0049e230`
  (`00418a00`). A second object would have pooled its own `0.0f` and
  `1.0f`.
- **`.bss`.** `004d78c8`-`004db8e8` sits between Fallin.c's `004d78c4`
  (finding 599) and HelpWindow.c's `004db8e8`. Every immediate reference
  into it is from a function inside the unit (the last, `004db8c4`, from
`00417430`). Part 1 uses `004d78c8`-`004db8dc`;
  `004db8e0`/`004db8e4` are part 2's (`00415370`). `004d7904`-`004d7b08`
  (0x204 bytes) has no immediate reference.
- **`.data`.** `004a1f00` ("Menu::SaveImage") to `004a2794`, the `-1`
  words `004a1f48`-`004a1f5c` and the key-character table `004a1f60`; each
  item's referencing functions are all inside the unit.
- **Common data.** `frontGlobs` (`00556fc0` onwards) is read by the unit,
  by Reward.c (`00556fc4`, `00556ff8`, `00557418`, `00557504`, `00557508`)
  and by Objective.c (`0055750c`, `00557510`).

`translation-units.spec.csv`: GAME-017 keeps `FrontEnd.c` with this
evidence. `python3 tools/translation_units.py build`: 0 conflicts.

### The split

The issue proposed `004102b0`-`00414de0`. Part 1 keeps it: `00414de0`
(Front_Callback_SliderSoundVolume) and the brightness callback `00414d30`
close the loader block, and `00414df0` starts the options callbacks
(VolumeSet, music, scroll and slot callbacks) that part 2's
`LoadMenuBuild` (`004161f0`) pushes.

## How the conversion is laid out

As in findings 568 and 572:

- **`frontend_unit.c` is the unit file** (`unit GAME-017`). Part 2 appends
  to it in address order. The records and prototypes are in
  [`frontend_unit.h`](../../src/game/frontend_unit.h), the callees in
  [`frontend_unit_imports.h`](../../src/game/frontend_unit_imports.h).
- **The relocated file becomes `frontend_port.cpp`** (`git mv`) and stays a
  `legacy` entry (key `00416ed0`, its lower median) linked right after the
  unit, because it still holds part 2's annotated functions. The brief asked
  for a `tail` entry; it can only become one when part 2 drains it. Its
  part-1 models stay in place, unannotated, so no declaration before a
  part-2 function changes (finding 538): their `// FUNCTION` lines became
  `// PORT-ONLY: port model of 0x… (GAME-017 part 1, now frontend_unit.c);
  retires to the tail in part 2`. Nineteen static helpers that only those
  models use carry `// PORT-ONLY: helper of the GAME-017 part-1 port
  models`.
- **Annotations moved to the unit**: `GLOBAL`s for `004a1f60`, `004db748`,
  `004db8cc`, `004db8d4` and the six item-type tokens and suffixes
  (`004a2360`-`004a23f4`) leave `frontend_port.cpp`; `frontend.cpp`'s
  `005574dc` and `005574ec`-`005574f8` (the port's wipe and pan state, with
  other types) leave too. The port keeps its variables. Nine of those
  strings are now `// STRING` markers on the unit's literals; `"Null"`
  (`004a23f4`) stays a named array because `00413f90`'s asm pushes it.

### Records

`Front_Menu` (0xa0), `Front_MenuItem` (0x3c), the item data
(`Front_CycleData` 0x1c, `Front_TriggerData` 0xc, `Front_SliderData` 0x38,
`Front_RealSliderData` 0x1c, `Front_TextInputData`, `Front_SelectData`
0x5c with its 0x20-byte `Front_SelectItem`), `Front_MenuOverlay` (0x20),
`Front_MenuSet` (8) and `Front_Cache` (0x10), from the allocations and the
field accesses. The `Front_` prefix keeps them apart from the port's C++
types in the boundary test.

### State

- **FrontEnd.c's `.bss`** is the unit's `static`s, one annotated object
  each (the print queue, the replace-spaces buffer, the slot images and
  completions, the overlay and overwrite flags, the cache list, the mouse
  edge and the text-input blink counter).
- **`.data`**: the six `-1` words and the 256-entry key-character table.
- **`frontGlobs`**: one annotated stand-in per field part 1 uses
  (`frontGlobs_*`), as Lego.c's stand-ins (finding 568). Three keep the
  names Reward.c already reads through `frontend_api.h`:
  `g_front_main_menu_set` (`00556fc4`), `g_front_save_menu_result`
  (`00557504`) and `g_front_save_menu_open` (`00557508`); their port
  definitions leave `frontend_port.cpp`.

### What the shapes needed

- **Ghidra's item-token offsets are wrong** in `004140c0`
  Front_LoadMenuSet: from the pushes, an item line is `type:x:y:...` with
  the banner at token 5 for cycles and sliders (token 3 for 5- and 6-part
  triggers and "next" items), the slider range at tokens 6-7 and the
  real-slider range and step at 6-8. The varargs pointer runs on across
  menus. The config read is `legoGlobs.config`, not the parameter. The
  original passes the item type and the centring flag the other way round
  in the 5-part "next" item with an empty target; the unit keeps that.
- **Front_Menu_Update draws with Font_PrintF directly**, prints the title
  from `fullName`, uses the item's hi font while switching on the focused
  item's type, passes Front_MenuItem_DrawSlider a sixth float (1.0 focused,
  0.25 otherwise; unused), and draws select entries with image 0/1 for
  selectable and 2 for unselectable ones. The wipe clock is Main_GetTime
  (`00478000`), not the import.
- **Front_Menu_UpdateOverlays** reads the flic's size through `004897d0`
  and `004850e0` (now `extern "C"` in `record.hpp`, in place) and tests
  the playing branch first: 39% to 75% (82% in `match-function`).
- **Front_Menu_GetItemBounds** is a switch (slider, select, default): 25%
  to 60%.
- **Key edges** are `cur[k] && (prev[k] ^ cur[k])`, which gives the
  original's `xor`.

### Naked asm

Four functions use the port's naked asm under `#if defined(_MSC_VER) &&
defined(_M_IX86)`, with C in `#else` (the native build):

| Address | Why |
| --- | --- |
| `00410a10`, `00410d50`, `00413f90` | Exact before (the port's asm). The C stores, their order and the loads are the original's, but MSVC picks other registers: 92.98%, 93.90% and 90.70%. Tried: a hill-climb over every adjacent swap and move of the stores, the `msvc_permuter.py` (no candidates for pointer-field stores), separate locals and the original statement order from the listing. |
| `004103f0` | MSVC folds `flags |= 1; flags |= 2` into `or al, 3`; the original keeps both. Tried: every order around the timer store, one combined expression and an enum flag type. The asm calls Main_GetTime, as the original does (the port's asm called the import), so it rises from 84.21% to 89.47%. |

## Link names

`frontend_unit_link_names.h` includes the callees' existing link-name
headers (Pointer.c, Credits.c, SFX.c, ToolTip.c, Viewports.c), so in the
matching build the unit calls the definitions annotated at the original
addresses, as Reward.c does. It also gives `004103d0`
(Front_Callback_TriggerPlayCredits) the port's name `main_menu_text_item`
in the matching build; `frontend_port.cpp` compiles its own definition out
of that build. Natively the port's definition stays live and the unit's
version calls it.

Callees whose port declarations type a float argument as its bits
(`view.hpp`: `00473460`, `00473480`) or name the frame time unused
(`savegame.hpp`: `004150b0`, now `extern "C"` in place) are declared with
the float in `frontend_unit.c` for the matching build; natively they get the
port's prototype.

### Callers switched

| Caller | Before | After |
| --- | --- | --- |
| Lego.c `0041fb50` | `movie_avis_enabled_original` (adapter in `frontend_port.cpp`) | `front_is_intros_enabled` (`lego_unit_imports.h`, matching build only): 97.18% to 97.21% |
| Reward.c `00461410`, Lego.c `0041fb50`/`004231f0`, GAME-050 `00471d20` | the port's definitions | the unit's definitions under the same names (no source change) |

## Entropy and drops

`make reccmp-check` against `main`'s baseline: 53 improved, 5 newly mapped
(`00411a00`, `00413970`, `00413a60`, `00413b80`, `00414d30`), no exact
match lost, and **one drop, flagged for review**:

- `00416c80` `menu_enter` (part 2, still the port's model), 24.24% to
  22.22% (drift; instruction count unchanged). Its call to the port's
  `frontend_open_submenu` was scored against `00413e20`'s annotation, which
  the unit now owns. The port's function has another contract (`FrontEnd*`,
  menu, submenu), so the call is not switched; part 2 converts the caller.

The listing digests change at 36 addresses, all inside part 1. No function
outside the unit changed instruction count.

## `opt_ref` (finding 579, `make opt-ref-callers`)

`opt_ref` rises (398/220 to 422/238) because most of part 1 is reached only
from part 2, whose functions are still the port's: Front_RunScreenMenu,
Front_LoadMenuSet and their callees are roots only through `00416c80`,
`004161f0`, `004161a0` and `004150b0`, which call the port's models.
`make opt-ref-callers OPT_REF_ARGS="--object frontend_unit.c.obj --min 1"`
lists them. The one caller with the original's contract, Lego.c's
`0041fb50`, is switched.

## `.bss` and the image

The relink `.bss` grows by 15,888 bytes and the gate image end moves from
`0x892000` to `0x89b000`: the unit adds FrontEnd.c's own `.bss` (16,416
bytes, mostly the 15 × 1,024-byte print queue), while the port's models in
`frontend_port.cpp` keep their copies (its string queue, 15,540 bytes)
until part 2 retires them.

## Native behaviour

Native behaviour is unchanged. Natively the port's front end still runs;
the unit compiles as C++ and only its shared trivial functions
(`renderer_word_*_get`, `input_key_context_check`, `mission_replace_char`)
are live. One port bug surfaced: `message_test` read
`s_msg_catalog[MESSAGE_MAX_ID]`, one past the 40-record array in
`info_port.cpp`, and the new globals changed what lay there. The port's
`incident_message_text_catalog_get` now guards the index, as the test's
message ("guarded") expects.

## Reccmp lint ratchet

The ledger goes from 1,042 to 1,023 entries (−19): the nineteen port
helpers that only part 1's models use are now `PORT-ONLY`. The unit's one
native-only helper (`front_float_bits`) is `PORT-ONLY`.

## Files outside the unit

No file owned by #697 (`container_unit.cpp`), #698 (`mesh_unit.cpp`), #699
(function bodies in `lego_unit.c`, `legoobject_unit.c`) or #700
(`resource.cpp`, the tutorial state, `roofGlobs`) is edited, and no frozen
`_for_`/`_from_` header. Outside the unit:

- `frontend.cpp`: five `GLOBAL` annotations removed;
- `savegame.hpp`, `record.hpp`: `extern "C"` in place on one declaration
  each;
- `lego_unit_imports.h`: the `0041fb50` mapping;
- `info_port.cpp`: the index guard;
- `test_c_boundaries.cpp`, `link_order.cmake`, `src/game/meson.build`,
  `translation-units.spec.csv` and the generated files,
  `tools/binary_progress_inputs.txt`.

## Validation

- `make reccmp-check`: one flagged drop (above), no exact match lost; the
  plain-immediate check, the winapi link check and the link-order map
  check pass.
- `make match-progress`, `make unit-data-offsets`: run; the three generated
  files are committed. GAME-017's row: `.data` partial (45, 5 unknown),
  `.bss` partial (15, 0 unknown), commons as listed.
- `make tools-test` (no `--allow-growth`): passes. `make winapi-link-check`:
  OK.
- Native: `cmake --build build` and `ctest --test-dir build`: 37 of 37.
- C subset: `gcc -std=gnu89 -Werror=declaration-after-statement
  -fsyntax-only` on `frontend_unit.c`.

## Reproduce

    python3 tools/convert_unit.py GAME-017 --stem frontend_unit --out <scratch>   # with the GAME-017 link-order entry removed: the unit was relocated
    make match-function SOURCE=src/game/frontend_unit.c
    make reccmp-check
    make opt-ref-callers OPT_REF_ARGS="--object frontend_unit.c.obj --min 1"
