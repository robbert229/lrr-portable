# Finding 611 — ScrollInfo.c (GAME-039) rebuilt in original shape, with a port tail

Issue #662, part of #388 under [ADR 0005](../adr/0005-original-shape-c-units.md).
GAME-039 is the information panel's scroll bar. OpenLRR names it
`ScrollInfo.c`, and the unit map now uses that name (`inferred`). Each scroll
record sits on a panel and has:

- an image;
- two arrow buttons, each with a highlight image and a pressed image;
- a slider over a range of positions.

ScrollInfo_Update moves the position when a button is pressed or the slider
is dragged, and then lays the slider out again. ScrollInfo_Draw draws the
bar, the highlighted or pressed buttons and the slider. Panel.c creates the
one record the game uses, for the scrolling message text. Info.c sets the
record's range and reads it back.

The unit map listed 12 functions (`004636b0`-`00464110`), 7 of them
low-confidence and 4 with no source. **The unit is 11 functions,
`004636b0`-`00463f00`.** `00463f00` (SelectPlace_Create) opens GAME-040
(see [Unit extent](#unit-extent)).

This change rebuilds the unit as the C unit
[`src/game/scrollinfo.c`](../../src/game/scrollinfo.c), with:

- the C header [`scrollinfo.h`](../../src/game/scrollinfo.h);
- the unit-private imports header
  [`scrollinfo_imports.h`](../../src/game/scrollinfo_imports.h);
- the matching-build link names
  [`scrollinfo_link_names.h`](../../src/game/scrollinfo_link_names.h).

The port's model moves, unannotated, to the port tail
[`scrollinfo_port.cpp`](../../src/game/scrollinfo_port.cpp). That file is
`info_track_overlay.cpp` under `git mv`, without SelectPlace_Create.

"Before" is the *match baseline* at `0064d345` (#665, stacked on #664). "After" is this
change, from the full matching build (`make reccmp-check`).

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (11 functions, weighted by function span, 2,128 bytes) | 12.74% | **94.70%** |
| Exact functions in the unit | 1 | **6** (no asm) |
| Mapped functions in the unit | 7 | **11** |
| Exact-function code bytes (`binary-progress.json`) | 192,532 | **193,084** |
| `opt_ref` annotated (full, by address): removed / exact removed | 480 / 248 | 475 / 244 |
| `opt_ref` annotated (game-only, by address) | 454 / 228 | 449 / 224 |
| Reccmp lint ledger (entries) | 1,219 | **1,218** |

The weights are each function's span up to the next function, padding
included. An unmapped function counts as 0%.

The names follow OpenLRR's `ScrollInfo_*` functions in the repository's
snake case. They are `inferred`. The binary attests only the bitmap names
`scrollInfo.bmp` and `scrollInfoSld.bmp`, which Panel.c passes in.
"Link name" is the name the definition takes in the matching build (see
[Link names](#link-names)).

| Address | Function | Link name | Span | Before | After |
| --- | --- | --- | ---: | ---: | ---: |
| `004636b0` | `scroll_info_initialise` | `info_panel_initialise` | 128 | unmapped | 97.92 |
| `00463730` | `scroll_info_set_xy_offset` | `indexed_float_pair_set` | 64 | 39.02 | **100.00** |
| `00463770` | `scroll_info_add_structure` | `scroll_rect_resource_record_create` | 160 | 29.41 | 96.15 |
| `00463810` | `scroll_info_add_create_structure` | (its own) | 144 | unmapped | 97.73 |
| `004638a0` | `scroll_info_update` | `info_panel_check_collision` | 864 | 6.06 | 94.76 |
| `00463c00` | `scroll_info_mouse_over_structure` | (its own) | 144 | unmapped | 94.85 |
| `00463c90` | `scroll_info_mouse_over_create_structure` | (its own) | 144 | unmapped | **100.00** |
| `00463d20` | `scroll_info_draw` | `view_blit_rect_positions_apply` | 288 | 9.43 | 83.33 |
| `00463e40` | `scroll_info_round` | `float_round_to_nearest_int` | 96 | 100.00 | **100.00** |
| `00463ea0` | `scroll_info_get_range` | `indexed_text_metrics_pair_get` | 48 | 19.51 | **100.00** |
| `00463ed0` | `scroll_info_set_range` | `indexed_text_metrics_pair_set` | 48 | 29.63 | **100.00** |

**The four functions with no source** (`004636b0`, `00463810`, `00463c00`
and `00463c90`) are written and mapped. Before this change:

- Panel.c called a no-op `PORT-ONLY` adapter for ScrollInfo_Initialise.
- `gods98/resource.cpp` held an unannotated model of `00463810`.
- `gods98/view.cpp` held port helpers named after `00463c00` and
  `00463c90`, with other contracts.

Two Panel.c callers rose with no source change:

- `0045b740` `ui_scroll_info_panel_initialize`, from 96.88% to **100.00%**;
- `0045adb0` `panel_cursor_interaction_update`, from 82.26% to 82.48%.

Both call the unit, which is now mapped. GAME-038's `00463590` also rose,
from 33.99% to 36.60%, with no source change.

### What is left

- **Images.c callees** (`004636b0`, `00463770`, `00463810`): every
  instruction matches except the calls to Image_LoadBMPScaled (`0047d9d0`).
  The unit calls `images.h`'s unannotated adapter, as Text.c does.
  `00463d20` has the same gap for Image_DisplayScaled (`0047dcb0`).
  Converting Images.c (G98-011) closes them.
- **`004638a0`** (94.76%): ScrollInfo_Update stores the slider's dirty flag
  with an immediate `1`. The rebuild reuses `ebx`, which already holds the
  handled flag, so the function is 4 bytes shorter and the later branch
  offsets shift. The statement orders, the types of `handled`, and a shared
  tail written with `goto` all give the same code.
- **`00463c00`** (94.85%): the panel's `xCurr` is read through
  `panelGlobs.panelTable[...]` with its displacement folded in. The original
  first computes the panel pointer. With a panel pointer local, MSVC emits the
  record's `xOffset` first instead. Ten source shapes were tried. The same
  shape in `00463c90` matched once the slider was read inline.
- **`00463d20`** (83.33%): two `fadd` operand swaps (`xOffset` against
  `xCurr`), and the slider kept in `eax` rather than `esi`. The order of the
  source operands, the local declaration order (24 permutations), inline or
  local `slider` and `panel`, and the order of the slider's sums all give the
  same code. This is finding 538's rule B (the counter's phase), not
  declaration order.

`tools/msvc_permuter.py` found no improving rewrite for `00463c00`,
`00463c90` or `00463d20` (90 s each). No naked asm, and no
`Deviation (scoring only)`.

## Unit extent

The issue asked for the extent to be confirmed, as in #618 (finding 591).
Findings 599 and 603 apply: MSVC 5.0 emits each float literal once per
object, so a function's literal pool shows which object it was compiled in.

- **The literal pool.** The `.rdata` run `0049e700`-`0049e73c` is:

  | Address | Value | Used by |
  | --- | --- | --- |
  | `0049e6f8`, `0049e700` | `0.1f`, `1.0f` | GAME-038 (`00462d77`, `00463428`, `0046366c`) |
  | `0049e710` | `0.5` (double) | `00463e40` only: **GAME-039's pool** |
  | `0049e718` | `0.0f` | no code reference |
  | `0049e71c` | `1.0f` | no code reference |
  | `0049e720` | `-1.0f` | no code reference |
  | `0049e724`-`0049e730` | `0.7f`, `0.9f`, `0.1f`, `0.6f` | no code reference |
  | `0049e734` | `0.5f` | `004641c0` |
  | `0049e738` | `1.0f` | no code reference |

  `00463f00` stores `0x00000000`, `0x3f800000` and `0xbf800000` (0, 1 and -1)
  as immediates, in that order of first use. `004641c0` stores `0x3f333333`,
  `0x3f666666`, `0x3dcccccd` and `0x3f19999a` (0.7, 0.9, 0.1 and 0.6) as
  immediates. So the pool `0049e718`-`0049e738` belongs to one object that
  starts with `00463f00`'s literals and continues with `004641c0`'s: GAME-040
  (OpenLRR's SelectPlace.c). GAME-039's only literal is the double at
  `0049e710`.
- **The data.** The unit's state is:
  - the common block `00552140`-`00552164` (`scrollInfoGlobs`, 0x24 bytes):
    one 0x20-byte scroll record and the selection word `00552160`;
  - the `.bss` word `005013c0`.

  Only the unit's functions reference them. `00463f00` touches neither.
  GAME-038 owns the commons from `00552180`, and GAME-040 owns `.bss` from
  `005013c8`. The `.bss` `00500110` that the unit map attributed to GAME-039
  is Panel.c's `panelTable`, which the unit reads.
- **The calls.** The unit's functions call each other (`004638a0` calls
  `00463c00`, `00463c90` and `00463e40`), Images.c, Panel.c's
  `0045ab30` and the C library. Their callers are:
  - Panel.c (`0045b740`, `0045adb0`);
  - Info.c (`0041a250`, `0041a290`);
  - Lego.c (`00423bf1` calls `00463d20`; the call graph misses it).

  `00463f00` is called only by Lego.c, and only for the placement grid.
- **The near edge.** `004636b0` writes `00552140` onwards. GAME-038's last
  function, `00463590`, reads its own commons (`00552180`) and its own
  `1.0f`.

`translation-units.spec.csv`:

- GAME-039 becomes `ScrollInfo.c`, `004636b0`-`00463f00`, 11 functions.
- GAME-040 starts at `00463f00`, 4 functions.

`python3 tools/translation_units.py build`: 0 conflicts.

`scene_quad_grid_emit` (`00463f00`), its static vertex helper and
`select_place_create_adapter` move verbatim from `info_track_overlay.cpp` to
GAME-040's `placement_footprint.cpp`, before `00464110`. That file gains one
include, `port_loop_ports.hpp`, for the hook table. GAME-040's listings are
unchanged.

## How the conversion is done

### State

| Offset | Address | Field |
| ---: | --- | --- |
| `0x00` | `00552140` | `structs[0].image` |
| `0x04` | `00552144` | `xOffset`, `yOffset`: the bar's place on its panel |
| `0x0c` | `0055214c` | `buttons[2]`: 0x1c-byte records (`x1, y1, x2, y2`, highlight and pressed images, flags) |
| `0x14` | `00552154` | `slider`: a 0x28-byte record (`x1, y1, x2, y2`, offsets, image, `range`, `position`, `dirty`) |
| `0x18` | `00552158` | `panel` |
| `0x1c` | `0055215c` | `flags`: 1 enabled, 4 image loaded, 8 vertical, 0x10 horizontal |
| `0x20` | `00552160` | `selection`: button 0 or 1, the slider (2), or none (3) |

`ScrollInfo_Globs scrollInfoGlobs` is a plain file-scope definition, so C
places it in the common region (`00552140`). `static int
scrollInfoOverSlider` is `.bss` `005013c0`. The port's
`s_ui_track_pointer_outer_gate` (`005013c0`, read by `ui_list.cpp` and
`pointer.cpp`) stays in the port tail, unannotated.

`make unit-data-offsets`, GAME-039 row:

| Unit | .data | .rdata | .bss | Commons |
| --- | --- | --- | --- | --- |
| GAME-039 | n/a (0, 0 unknown) | n/a (0, 0 unknown) | n/a (1, 0 unknown) | 00552140:_scrollInfoGlobs(size=36) |

The unit has one `.bss` item, so no anchored offsets can be checked.
`scroll_info_round`'s `0.5` is pooled by MSVC.

The image's `.bss` grows by 96 bytes (1,668,448 to 1,668,544): the unit's
state, beside the port's own. The plain-immediate check passes.

### The port tail

The port's model is another contract. It stores into the port's `crt.cpp`
arrays (`s_fpair_a`, `s_view_rect_a`, `s_metric_records`), with guards and
NULL checks, and has no-op adapters for `004636b0` and `004638a0`. As in
findings 597-607, `info_track_overlay.cpp` is renamed `scrollinfo_port.cpp`
and becomes a `tail` entry, in Meson too:

- each `// FUNCTION` becomes a `/* Port model of 0x… */` comment;
- the `// GLOBAL` becomes a `/* Port state of 0x… */` comment;
- the `unported_ledger_stubs` forwarder for `004638a0` is deleted.

### Link names

In the matching build, `scrollinfo_link_names.h` gives eight unit functions
the port's names (see the table above). All eight port names have C linkage
(`info.h`, `info_api.h`, `lego_unit_imports.h`, `crt.hpp`,
`frontend.hpp`), and every matching-build caller passes the original's
arguments:

- **Panel.c** (`panel_unit.c`): the image name, index, orientation and panel
  (a NULL name ends the list); the offset; the button rectangles; the mouse
  and buttons.
- **Info.c** (`info_unit.c`): the range and position.
- **Lego.c** (`lego_unit.c`): the draw index.

So these callers reach the unit with no caller edit. `scrollinfo_port.cpp`
compiles its definitions of the eight names out of the matching build with
`LRR_SCROLLINFO_PORT_NAMES`, set by an explicit `#if` (finding 590).

ScrollInfo_AddCreateStructure (`00463810`) keeps the unit's name. The port's
name, `scroll_rect_payload_record_create`, is defined by
`gods98/resource.cpp`, which #661 owns. So `info.h` points Panel.c at the
unit's name in the matching build instead, with a `#define` under the
MSVC 5.0 `#if`. `resource.cpp` is not edited. The two mouse-over tests keep
the unit's names, because only the unit calls them.

Natively, the unit compiles as C++ under its own names and nothing calls it.
The port tail serves every port name, so native behaviour is unchanged.

### Callees

| Original | Called as | Unit |
| --- | --- | --- |
| `0045ab30` (the panel-collapsed test) | `request_record_ready_flag4_target` | Panel.c (`panel_unit.c`) |
| `panelGlobs.panelTable[...].xCurr/yCurr` | read inline | Panel.c |
| Image_LoadBMPScaled `0047d9d0`, Image_DisplayScaled `0047dcb0` | `images.h`'s adapters | not converted |
| `malloc`, `floor`, `ceil`, `_ftol` | `crt/vc5names.h`, `<math.h>` | LIBC |

`scrollinfo_imports.h` includes `panel_unit.h`, as `info_unit.c` and
`priorities.c` do. No existing declaration was edited.

### Source shapes that matched

- **ScrollInfo_Initialise** (`004636b0`, 41.76% at first): two stores,
  `flags = ENABLED;` then `flags = ENABLED | (horizontal ? HORIZONTAL :
  VERTICAL);`. The original keeps both stores. `flags |= …` folds the first
  store away, as does every one-statement form, five forms in all. This gave
  97.92%, and every instruction matches.
- **ScrollInfo_SetXYOffset** (`00463730`, 70.59%): a record pointer local.
  Indexing the array twice folds the base into the displacement.
- **ScrollInfo_Update** (`004638a0`, 69.10% at first):
  - a `PanelData*` local for the drag (81.59%);
  - the slider layout written as `position * ((x2 - x1 - width) / range)`
    (90.60%; four other shapes gave 79.92-84.59%);
  - the call to ScrollInfo_Round in each branch of the vertical test, which
    MSVC tail-merges (95.13% single-function, 94.76% linked).
- **ScrollInfo_MouseOverCreateStructure** (`00463c90`, 85.42%): a panel
  pointer local, and the slider read inline as `scrollInfo->slider->x1`. A
  `slider` local stores and reloads `(float)mouseX` in another order.

## Entropy and drops

`make reccmp-check` against the baseline at `0064d345` (#665):

- 9 improved: 7 of the unit's addresses, plus `0045adb0`, `0045b740`
  (Panel.c) and `00463590` (GAME-038);
- 4 newly mapped: `004636b0`, `00463810`, `00463c00`, `00463c90`;
- **none below baseline**, and no exact match lost.

Instruction counts changed only at the unit's addresses (listing digests):

| Address | before | after |
| --- | ---: | ---: |
| `004636b0` | — | 48 |
| `00463730` | 19 | 17 |
| `00463770` | 70 | 52 |
| `00463810` | — | 44 |
| `004638a0` | 17 | 268 |
| `00463c00` | — | 48 |
| `00463c90` | — | 50 |
| `00463d20` | 102 | 96 |
| `00463ea0` | 21 | 12 |
| `00463ed0` | 14 | 12 |

`00463e40` keeps its listing and its exact match. `00463f00` and the rest of
GAME-040 keep their listings after the move.

## `opt_ref` (finding 579, `make opt-ref-callers`)

| | before (#665 `0064d345`, committed) | after |
| --- | ---: | ---: |
| `opt_ref.annotated.full` (by address): removed / exact removed | 480 / 248 | 475 / 244 |
| `opt_ref.annotated.game_only` (by address) | 454 / 228 | 449 / 224 |

`make opt-ref-callers OPT_REF_ARGS="--object scrollinfo.c.obj"` lists no
removed function, so all 11 are kept. #665 was not rebuilt in this
worktree, so part of the difference may be the layout noise that finding 591
describes.

## Reccmp lint ratchet

1,219 → 1,218 entries (−1, no growth). The port's static `float_bits` is
now in a tail file, which the lint exempts.

## Files outside the unit

The change does not edit any file owned by #447 (`src/crt/`), #660
(`screen_effect_quads.cpp`), #661 (`gods98/resource.cpp`) or #663
(`dependencies.cpp`, `pointer.cpp`). No frozen `_for_` or `_from_` header is
edited. Outside the unit and its port tail, the change edits:

- `info.h`: comments, and the matching-build `#define` for `00463810`;
- `lego_unit_imports.h`: a comment (SelectPlace_Create's new file);
- `placement_footprint.cpp`: SelectPlace_Create moved in (GAME-040);
- `test_c_boundaries.cpp`: `scrollinfo.h` and `scrollinfo_imports.h`;
- `link_order.cmake` and `src/game/meson.build`: the unit and the tail.

## Native behaviour

Native behaviour is unchanged. There is no new native gap. The port tail
still serves every name, and Panel.c's native widget is still the no-op
adapter pair (#552 defers native gameplay).

**Matching-build runtime** (`make reccmp-run` only): Panel.c, Info.c and
Lego.c now drive the real scroll bar. The port's `crt.cpp` arrays are left
unused there.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox): OK. None below
  baseline, and no exact match lost. The plain-immediate check, the winapi
  link check and the link-order map check pass.
- `make match-progress`, `make unit-data-offsets`: run, and the generated
  files are committed.
- `make winapi-link-check`: OK.
- `make tools-test`: passes (no `--allow-growth`).
- Native: `cmake --build build` and `ctest --test-dir build`: 37 of 37
  pass.
- C subset: `gcc -std=gnu89 -Werror=declaration-after-statement
  -fsyntax-only src/game/scrollinfo.c`.
