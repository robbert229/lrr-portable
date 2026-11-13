# Finding 633 — Loader.c, Encyclopedia.c, ObjInfo.c and SelectPlace.c rebuilt in original shape; Advisor.c's Main_GetStartLevel written

Issue #719, part of #388 under [ADR 0005](../adr/0005-original-shape-c-units.md).
Four small units had been relocated verbatim and never converted, and
Advisor.c's `00401c00` had no source. Two of their functions (`0044dde0`,
`00401c00`) were the last unannotated calls holding Lego.c's
`lego_game_config_init` (`0041fb50`) below its other callees' level
(findings 629 and 632). This change:

- rebuilds GAME-025 as **Loader.c** ([`src/game/loader.c`](../../src/game/loader.c));
- rebuilds GAME-014 **Encyclopedia.c** ([`src/game/encyclopedia.c`](../../src/game/encyclopedia.c));
- rebuilds GAME-031 as **ObjInfo.c** ([`src/game/objinfo.c`](../../src/game/objinfo.c));
- rebuilds GAME-040 as **SelectPlace.c** ([`src/game/selectplace.c`](../../src/game/selectplace.c));
- writes Main_GetStartLevel (`00401c00`, a Main.h inline) in
  [`src/game/advisor.c`](../../src/game/advisor.c).

Each unit has a `.h`, an `_imports.h` and, where callers use port names, a
`_link_names.h`. The relocated files move, unannotated, to the port tails
`loader_port.cpp`, `encyclopedia_port.cpp`, `objinfo_port.cpp` and
`selectplace_port.cpp` (`git mv`, `tail` entries in `src/link_order.cmake`).

"Before" is the *match baseline* at `main` `7835ce86`. "After" is this
change, from the full matching build (`make reccmp-check`).

## Result

| | before (`7835ce86`) | after |
| --- | ---: | ---: |
| Exact-function code bytes (`binary-progress.json`) | 279,887 (43.56%) | **285,588 (44.45%)** |
| Accuracy, full / game-only | 95.38 / 94.98 | **95.77 / 95.40** |
| `opt_ref` annotated (full): removed / exact removed | 164 / 127 | **157 / 123** |
| `opt_ref` annotated (game-only) | 158 / 121 | 152 / 118 |
| `/OPT:REF` relink `.bss` (`opt_ref.image.bss_bytes`) | 1,115,696 | 1,116,688 |
| Gate image end (`make pe-audit`) | `0x873000` | `0x875000` |
| Mapped functions (full) | 2,656 | 2,663 |
| `0041fb50` lego_game_config_init | 99.29% | **99.45%** |
| Reccmp lint ledger (entries) | 946 | 931 |

`make reccmp-check`: 27 functions improved and 7 newly mapped, **no
function below the baseline, no exact match lost**, so there is no drop to
label. No naked asm and no `Deviation (scoring only)` in any of the units.
The added `.bss` (992 bytes) is the units' own state: `loaderGlobs`
(0x290), `objInfoGlobs` (0x78), `selectPlaceShape` (0xa0) and
`encyclopediaGlobs` (0x24), with alignment.

`lego_game_config_init` now calls Loader.c and Advisor.c's
Main_GetStartLevel at their own addresses (`0044db50`, `0044dde0` twice,
`00401c00`). Its remaining diffs are calls to other units' functions under
port names (Info.c, the Lego.c config loaders, Panel.c, Text.c), outside
this issue.

Names are this repository's snake case of the original's (`Loader_*`,
`Encyclopedia_*`, `ObjInfo_*`, `SelectPlace_*`, after OpenLRR where known).
"Link name" is the name the definition takes in the matching build.

### Loader.c (GAME-025)

| Address | Function | Link name | Before | After |
| --- | --- | --- | ---: | ---: |
| `0044db50` | `loader_initialise` | (own) | 11.05 | **100.00** |
| `0044dd40` | `loader_shutdown` | (own) | 9.09 | **100.00** |
| `0044dde0` | `loader_display_loading_bar` | (own) | unmapped | **100.00** |
| `0044dec0` | `loader_file_load_callback` | (own) | unmapped | **100.00** |
| `0044e0a0` | `loader_display_shutdown` | (own) | unmapped | **100.00** |

### Advisor.c (GAME-001)

| Address | Function | Link name | Before | After |
| --- | --- | --- | ---: | ---: |
| `00401c00` | `main_get_start_level` | (own) | unmapped | **100.00** |

### Encyclopedia.c (GAME-014)

| Address | Function | Link name | Before | After |
| --- | --- | --- | ---: | ---: |
| `0040e490` | `encyclopedia_initialise` | `encyclopedia_config_files_open` | 9.93 | 91.79 |
| `0040e700` | `encyclopedia_select_object` | `terrain_choice_resource_select` | 22.40 | **100.00** |
| `0040e7e0` | `encyclopedia_disable_update` | `global_flag_bit0_clear` | 100.00 | 100.00 |
| `0040e7f0` | `encyclopedia_update` | (own) | unmapped | **100.00** |
| `0040e8d0` | `encyclopedia_draw_select_box` | (own) | unmapped | **100.00** |
| `0040e910` | `encyclopedia_remove_current_reference` | `global_value_clear_if_equal` | 100.00 | 100.00 |

### ObjInfo.c (GAME-031)

| Address | Function | Link name | Before | After |
| --- | --- | --- | ---: | ---: |
| `00459530` | `objinfo_initialise` | `frontend_render_three_pass_update` | 100.00 | 100.00 |
| `00459560` | `objinfo_load_health_bar` | (own) | 4.24 | 99.27 |
| `00459900` | `objinfo_load_hunger` | (own) | 17.58 | 98.41 |
| `004599c0` | `objinfo_load_hunger_images` | (own) | 31.50 | **100.00** |
| `00459a50` | `objinfo_load_bubble` | (own) | 36.21 | 98.21 |
| `00459b00` | `objinfo_draw_health_bar` | `legacy_entity_screen_ring_render` | 9.66 | 26.72 |
| `00459f50` | `objinfo_draw_hunger` | `objinfo_draw_hunger_image_adapter` | 27.12 | 88.57 |
| `00459fd0` | `objinfo_draw_bubble` | `panel_resource_position_apply_if_enabled` | 24.14 | 88.00 |

### SelectPlace.c (GAME-040)

| Address | Function | Link name | Before | After |
| --- | --- | --- | ---: | ---: |
| `00463f00` | `selectplace_create` | `select_place_create_adapter` | 15.83 | **100.00** |
| `00464110` | `selectplace_transform_shape` | `rotated_pairs_scratch_fill` | 10.85 | 81.89 |
| `004641c0` | `selectplace_check_and_update` | `legoobject_gs_placement_footprint_check_adapter` | unmapped | 51.14 |
| `00464720` | `selectplace_hide` | `surface_visibility_set` | 9.76 | **100.00** |

Callers that rose with no source change: Bubbles.c `00407960` (96.30 to
98.15), Lego.c `0041fb50`, `00422850` (to 100), `004232e0`, `00424d00`,
`00429890`, `0042d600`, `0042da20` and `0042e8b0`, LegoObject.c `00437150`,
`004396c0` (to 100) and `0044a910` (to 100), and Reward.c `0045f030`.
Several of these gain only because "Ore" and "PowerCrystal"
(`004a1e98`/`004a1e9c`) are now marked as Encyclopedia.c's strings.

## Unit extents

Findings 599 and 603 apply: MSVC 5.0 pools float literals per object, and a
constant used only as an immediate still gets a pool entry. All four
extents hold. A scan of `.text` for absolute references (a throwaway
script over `reference/LegoRR.exe`) gives the data ownership below.
`translation-units.spec.csv` records it; `python3
tools/translation_units.py build`: 0 conflicts.

**Loader.c, `0044db50`-`0044e0c0`, 5 functions (name inferred).** No
`__FILE__` or identifier names it; OpenLRR calls it Loader.c.

- The commons `00555e00`-`00556090` (`loaderGlobs`: the loading and
  shutdown screens, the font, 50 sections of name, current and expected
  size, the current section, the bar image and window, its direction, the
  loading text and its width, the last drawn share and the flags) are
  referenced only from the five functions.
- `.rdata` `0049e4d0`-`0049e4e0` (-1.0 1.0 0.0 -0.002 0.5) follows
  LightEffects.c's pool (`0049e4b0`-`0049e4cc`). Only `0044dec0` reads it;
  -1.0 and 0.0 are stored as immediates (`0044de92`, `0044df1e`) and their
  entries are otherwise unreferenced, as finding 611 saw for SelectPlace.c.
- Strings `004a52ac`-`004a52b8` ("=", "%s%s%i\n", "w"). "w" is also
  Files.c's (pooled, marked there), and "r" (`004a1eac`) is Encyclopedia.c's.
- `0044e0c0` after it opens the terrain map, whose pool starts at
  `0049e4e4`.

**Encyclopedia.c, `0040e490`-`0040e930`, 6 functions.** The `.bss`
`004c7ea4`/`004c7ea8` words the port had annotated are the end of one
record, `encyclopediaGlobs` (`004c7e88`-`004c7eac`: the vehicle,
minifigure, rock monster and building file arrays, the crystal and ore
files, the current file, the current object and the flags). All of it is
referenced only from the six functions. Its strings `004a1e98`-`004a1ee1`
are the "Ore", "PowerCrystal", "r", "Encyclopedia" and "Object has no
encyclopedia file." it uses; other units reuse the pooled "Ore",
"PowerCrystal" and "r".

**ObjInfo.c, `00459530`-`0045a030`, 8 functions.** It reads Lego.cfg's
`ObjInfo` block (the config key names it: strings `004a8290`-`004a8375`).

- `.bss` `004ffe68`-`004ffee0` (`objInfoGlobs`, 0x78 bytes) is referenced
  only from the range. It is one record: the flags, the health bar's offset,
  size, border width, border colour with its light and dark shades (stored
  by channel), bar and background colours, the hunger images' offset and
  five images, and the bubble offset.
- `.rdata` `0049e5b8`-`0049e5d4` (0.4 1.0 0.0 2.0 -1.0 100.0 0.01 5.0) is
  read only by `00459560`, `00459b00` and `00459f50`. The four floats before
  it (`0049e5a8`-`0049e5b4`: 470 315 130 1.0) are referenced by no code.
  They come after Objective.c's pool (`0049e56c`-`0049e5a4`), and this
  change does not attribute them.
- Panel.c starts at `0045a030` (its pool `0049e5d8`).

**SelectPlace.c, `00463f00`-`00464740`, 4 functions (name inferred).** The
boundary was `low`.

- The start is finding 611's: `00463f00` stores 0.0f, 1.0f and -1.0f as
  immediates, and their pool entries `0049e718`-`0049e720` follow
  ScrollInfo.c's 0.5 double.
- `004641c0` reads `0049e724`-`0049e738` (0.7 0.9 0.1 0.6 0.5 0.2).
- `.bss` `005013c8`-`00501468` (the 20-point shape buffer) is read only by
  `00464110`. SFX.c's `.bss` starts at `00501468`, and SFX.c's text at
  `00464740`.

**`00401c00` in Advisor.c.** Main_GetStartLevel reads Main.c's flags
(`00505f84`) and returns its start-level buffer (`00505f8c`). Only
`00401c00` and Main.c (`00477bf0`) reference the buffer, so this is one of
the Main.h inlines that close Advisor.c (finding 505). Its pool and
neighbours are unchanged.

## How the conversion is done

### State and data

Every unit defines its state as one record at its original address:
`loaderGlobs` is a common (no `static`, finding 611), and the others are
`static`. The port's globals that carried those addresses lose their
`GLOBAL` annotations but stay in the tails, because port code reads them:

- `s_panel_enable_word`, `s_panel_position_offset_x`/`_y` and
  `s_panel_position_slots` (ObjInfo.c);
- `s_global_value_word` and `s_ui_flag_word` (Encyclopedia.c);
- `s_rot_pairs` (SelectPlace.c);
- `s_item_name_ore` and `s_item_name_power_crystal`.

The literals are marked once each, in the unit that owns them.

`make unit-data-offsets`:

| Unit | `.bss` | `.data` | `.rdata` |
| --- | --- | --- | --- |
| GAME-025 | gaps-only (3, 0 unknown) | n/a | n/a |
| GAME-031 | gaps-only (12, 0 unknown) | n/a | n/a |
| GAME-014 | n/a (3, 2 unknown) | n/a | n/a |
| GAME-040 | n/a | n/a | n/a |

`platform/winapi/entry.cpp`'s start-level buffer (`s_start_level_name`,
annotated at `00505f8c`) takes C linkage, so Advisor.c can return it. It is
the same pattern as `s_frame_interval_override` (finding 630). Natively the
port has no such buffer, so `main_get_start_level` returns NULL there.

### Callers

- **Link names.** Encyclopedia.c, ObjInfo.c and SelectPlace.c give the
  functions whose callers already pass the original's arguments the port's
  names in the matching build (`*_link_names.h`). The tails compile their
  definitions of those names out of that build
  (`#if !(defined(_MSC_VER) && _MSC_VER < 1200)`). The callers are
  lego_unit.c, legoobject_unit.c, interface_unit.c, bubbles.c,
  terrain_cell_objects.c and the port's C++ callers (`frameloop.cpp`,
  `engine.cpp`, `tutorial_hud.cpp`). All these names have C linkage.
  `legoobject_port.cpp`'s PORT-ONLY adapter for `004641c0` is compiled out
  the same way.
- **Imports-header macros (one header edit in another thread's area).**
  [`lego_unit_imports.h`](../../src/game/lego_unit_imports.h) gains one
  matching-build, C-only block. It maps `lego_unit.c`'s port-name and
  adapter declarations to the units, as finding 631 did for Objective.c:

  | Declaration in `lego_unit_imports.h` | Unit function |
  | --- | --- |
  | `loader_initialise_adapter` | `loader_initialise` |
  | `loader_profile_entries_release_adapter` | `loader_shutdown` |
  | `string_table_entry_set_current` | `loader_display_loading_bar` |
  | `global_resource_handle_release` | `loader_display_shutdown` |
  | `main_get_start_level_adapter` | `main_get_start_level` |
  | `effects_update_original` | `encyclopedia_update` |
  | `screen_space_effect_dispatch_original` | `encyclopedia_draw_select_box` |

  A `#define` adds no declaration, so lego_unit.c's symbol numbering does
  not move. `lego_unit.c` itself is not edited.
- `global_resource_handle_release` keeps its port body in `resource.cpp`
  for `engine.cpp` and `rock_fall_styles_port.cpp`, which use the port's
  contract. `resource.cpp`'s block-comment "FUNCTION" notes for `0044dde0`,
  `0044dec0` and `0044e0a0` (which Reccmp never read) now name the unit.

Natively nothing changes: the port serves every name, and Loader.c compiles
to nothing (it blits to the DirectDraw back buffer and flips, as Flic.c and
Images.c do, finding 632).

### Source shapes worth noting

- **Loader_Initialise.** `file` is assigned in an `if`/`else`, not
  initialised at its declaration: initialised, it loses `ebx` to a
  zero-constant register (75% against 100%). The token array is
  `char* argv[10]`, which sets the frame size. The section names are copied
  with an inline `malloc(strlen + 1)` and `strcpy`; the loading text uses
  Util_StrCpy.
- **Loader_FileLoadCallback.** The bar's two `Point2F`s and the new edge are
  declared inside the draw block, so the stack temporary of the unsigned
  division overlaps them (87% against 61%). The bar edge is
  `newPos = (size + pos) - size * percent; size = (size + pos) - newPos;
  pos = newPos;`. The share test is
  `percent > (progressLast - -0.002f)`, which uses the pool's -0.002. The
  text's x is `(int)(x + (width / 2)) - (loadingWidth / 2)`.
- **Loader_display_loading_bar.** `progressLast` is stored before `flags`.
- **ObjInfo_LoadHealthBar.** Each shade is written
  `light = (c * 0.4f) + c`, `dark = c - (c * 0.4f)`, with the light shades
  first. `c + c * 0.4f` folds to `c * 1.4f` (78% against 99%). The border
  width uses Config_GetIntValue's double `Config_GetTempStringValue` call.
- **Encyclopedia_SelectObject.** The chosen file goes in a local and is
  stored after the flags and the object.
- **SelectPlace_Create.** `memset` clears the record. The u and v tables are
  set first, then the normals, then the face indices (96.7%; the rest is
  operand names).
- **SelectPlace_TransformShape.** A struct copy of the point and an explicit
  `case 0:` make the original's four-entry jump table.

## What is left

- **`00459b00` ObjInfo_DrawHealthBar (26.72%).** The original copies the
  bar's offset and size globals into the draw rectangle before adding the
  screen position, then builds the four border line lists in one
  interleaved x87 loop. Two copy shapes and three statement orders did not
  reproduce its stack layout.
- **`004641c0` SelectPlace_CheckAndUpdate (51.14%).** The two passes over
  the shape (the path check and the colour/vertex pass) differ in register
  allocation and in how the doubled-block branch joins the loop. The
  permuter (240 s) found nothing. It is also removed by `/OPT:REF`: its
  caller, LegoObject.c `0044a910`, is not reached from a live root (finding
  579).
- **`00459f50`/`00459fd0` (88%).** The original adds the screen position to
  the local copy of the offset (`fadd [esp]`). This build adds it to the
  global it was copied from. Five copy shapes gave 80% or less.
- **`00464110` (81.89%).** Register choice and the source pointer's bias
  (`sub esi, shape+4`).
- **`0040e490` (91.79%).** Only operand names differ. Like lego_unit.c,
  the unit reads legoGlobs' type counts at their offset from the
  `legoGlobs_config` stand-in, because the port's counts are C++
  (`s_catalog_counts`). Reccmp names the original's operand after that
  annotation and the rebuild's after whatever the stand-in layout puts at
  `+0x1bc`.

Port gap (`port`, not fixed here): the port's `rotated_pairs_scratch_fill`
negates y in its 90° case, while the original (`00464161`-`00464180`)
keeps the old x unchanged as the new y.

## Checks

- `make reccmp-check`: no function below the baseline; 27 improved and 7
  newly mapped; winapi link check and link-order `check-map` pass.
- `make match-progress`: the baseline, the listing digests and
  `binary-progress.json` regenerated, then restamped after `git add` with
  `binary_progress_hash.py --write-manifest`.
- `make unit-data-offsets`: the rows above.
- `make tools-test` (no `--allow-growth`): pass. Lint ledger 946 to 931
  (`make reccmp-lint-update`).
- Native build and `ctest`: 37 of 37 pass.
- `gcc -std=gnu89 -Werror=declaration-after-statement -fsyntax-only` passes
  on the four units.
- `make opt-ref-callers`: only `selectplace.c.obj` loses a function
  (`004641c0`, above). `loader.c.obj`, `encyclopedia.c.obj`,
  `objinfo.c.obj` and `advisor.c.obj` lose none.
