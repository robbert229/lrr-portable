# Finding 605 — Font.c (G98-008) rebuilt in original shape, with a port tail

Issue #650, part of #388 under [ADR 0005](../adr/0005-original-shape-c-units.md).
G98-008 is gods98's `Font.c` (text `00479ee0`-`0047a640`). A font is a
0xbf4-byte record that holds:

- the bitmap (`+0x000`, an Images.c `Image`);
- the source rectangle of each of the 10 x 19 glyph cells (`+0x004`), with
  each width trimmed to the glyph's ink;
- the line height and the tab width (`+0xbe4`, `+0xbe8`);
- the free-list link (`+0xbf0`). `+0xbec` is never read or written.

Fonts come from a pool of doubling blocks (`.bss` `00506528`-`005065b0`).
Font_VPrintF2 expands `%b` into an inline-image token (`@[0x%0.8x]`),
formats the text and draws it one glyph at a time, returning the widest
line. TextWindow.c, HelpWindow.c, ToolTip.c, Panel.c, Reward.c, Lego.c,
Info.c, Interface.c and Advisor.c's Font.h inlines call it.

#496 relocated the unit verbatim into `src/gods98/bitmap_font.cpp`. This
change rebuilds it as the C unit [`src/gods98/font.c`](../../src/gods98/font.c),
with:

- the unit-private imports header
  [`font_imports.h`](../../src/gods98/font_imports.h);
- the link names [`font_link_names.h`](../../src/gods98/font_link_names.h).

The port's renderer moves, unannotated, to the port tail
[`bitmap_font_port.cpp`](../../src/gods98/bitmap_font_port.cpp) (a `git mv`
and a `tail` entry, also in Meson). The existing boundary header
[`font.h`](../../src/gods98/font.h) keeps its declaration and gains one.

"Before" is the *match baseline* on `main` at `18cc9483` (#646). "After" is
this change, from the full matching build (`make reccmp-check`).

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (11 functions, weighted by function span, 1,888 bytes) | 11.46% | **89.46%** |
| Exact functions in the unit | 1 (naked asm) | **7** (no asm) |
| Exact-function code bytes (`binary-progress.json`) | 183,830 | **184,352** |
| Exact functions, full | 1,582 | 1,591 |
| Accuracy, full / game-only | 84.26% / 82.73% | 84.57% / 83.07% |

| Address | gods98 name | Function (matching build) | Span | before | after |
| --- | --- | --- | ---: | ---: | ---: |
| `00479ee0` | Font_Load | `font_atlas_record_create` | 624 | 9.93 | 75.43 |
| `0047a150` | Font_VGetStringInfo | `font_v_get_string_info` | 48 | 14.16 | **100.00** |
| `0047a180` | Font_PrintF | `font_print_f` | 48 | 25.00 | **100.00** |
| `0047a1b0` | Font_VPrintF | `font_v_print_f` | 48 | 20.34 | **100.00** |
| `0047a1e0` | Font_VPrintF2 | `font_v_print_f2` | 656 | 9.73 | 97.26 |
| `0047a470` | Font_OutputChar | `font_output_char` | 176 | 12.05 | 98.36 |
| `0047a520` | Font_GetCharWidth | `bitmap_font_glyph_measure_width` | 32 | 11.94 | **100.00** |
| `0047a540` | Font_GetHeight | `object_field_be4_dword_get` | 16 | 100.00 (naked asm) | **100.00** (C) |
| `0047a550` | Font_Remove | `font_atlas_record_release` | 48 | 43.75 | **100.00** |
| `0047a580` | Font_Create | `font_create` | 64 | unmapped | 61.11 |
| `0047a5c0` | Font_AddList | `font_add_list` | 128 | unmapped | **100.00** |

The binary attests none of the gods98 names. The stem follows finding 600,
which already calls the unit Font.c, and the `Interface\Fonts\*.bmp` names
passed to `00479ee0`. Each span runs to the next function, padding
included. Unmapped functions count as 0% in the "before" figure.

### What is left

- **`0047a470`** (98.36%) and **`0047a1e0`** (97.26%): the Image_DisplayScaled
  (`0047dcb0`) call operand. Images.c is not converted, and nothing in the
  port is annotated at that address.
- **`0047a1e0`**: also the `vsprintf` operand, which Reccmp names after the
  port's `formatted_text_buffer_build` (as in finding 600). The original
  also reloads `*msg` at the head of the `%b` copy loop, where the rebuild
  reuses the value its loop test loaded (one instruction). Tried without
  effect:
  - `while`, `for` and `if (...) do ... while` forms;
  - `msg[0]`, `*(msg + 1)`, reversed comparisons, unsigned-char casts;
  - writing `'%'` or `*msg`;
  - an index loop, which scored 76.77%.

  The bounded permuter (300 s) found no rewrite.
- **`00479ee0`** (75.43%). The Images.c call operands (`0047d9d0`,
  `0047dbc0`, `0047def0`, `0047df50`, `0047de60`, `0047ded0`) differ. The
  rest is one x87 allocation choice:
  - the original keeps `(float)width` and `(float)(y * height)` on the FPU
    stack and spills `(float)height`;
  - the rebuild keeps `(float)height` and spills the row's y.

  Tried without effect: all four orders of the declaration groups, six
  orders of the four rectangle stores (74.45% to 76.10%; the original's
  order is kept), and `int` casts. The permuter (420 s) found no rewrite.
- **`0047a580`** (61.11%). The original holds `&newFont->nextFree` in `esi`
  across the `rep stosd`, where the rebuild addresses `[edx + 0xbf0]`. An
  explicit pointer local is folded away by the compiler (61.11%). Testing
  the free list once (`if ((newFont = freeList) == NULL) { AddList(); ... }`)
  raised it from 45.71%. The permuter had no candidate.

## Unit extent

The map gave the unit high confidence. The extent is confirmed, and no
function moves:

- **Call graph** (`docs/coverage/call-graph/edges.csv` and a scan of
  `.text`). The 11 functions call only each other, Images.c (`0047d470`,
  `0047d9d0`, `0047dbc0`, `0047dcb0`, `0047de60`, `0047ded0`, `0047def0`,
  `0047df50`) and the C library (`malloc`, `vsprintf`, `__ftol`,
  `_isctype`). Only `00479ee0` calls `0047a580`, and only `0047a580` calls
  `0047a5c0`.
- **The edges.** `00479eb0` is Maths.c's Matrix4Copy and `0047a640` is
  Sound3D.c's first function.
- **Data.**
  - `.rdata` `0049e948` (`1.0f`) has one reference in `.text`, `0047a08c`
    (the width decrement). It is written as the literal, which MSVC pools,
    so the scaffold's `GLOBAL` for it is dropped.
  - The pool `.bss` `00506528`-`005065b0` (32 block pointers, the free list
    and the block count) is referenced only by `0047a550`-`0047a638`. It is
    one `fontGlobs`, annotated at `00506528`.
  - The unit also reads LIBC's `__mb_cur_max` and `_pctype` through
    `isdigit`.

`translation-units.spec.csv` records this evidence in G98-008's note, and
names the unit `Font.c` (`inferred`).

`make unit-data-offsets`, G98-008 row:

| Unit | .data | .rdata | .bss | Commons |
| --- | --- | --- | --- | --- |
| G98-008 | n/a (0, 0 unknown) fewer than two attributable items | n/a (0, 0 unknown) fewer than two attributable items | n/a (0, 0 unknown) fewer than two attributable items | `00506528`: `_fontGlobs` (size 136) |

Unit summary: full 0, gaps-only 7, partial 26, n/a 193.

## How the conversion is done

### The port tail

The port's fonts are another contract:

- software `TextureSurface` atlases with an ink-box glyph advance;
- drawing into a render target (`px`, `pitch`, `w`, `h`);
- the `@[0x…]` handles resolved through `s_formatted_text_texture_resolve`.

Native code calls these entries (`frontend.cpp`, `frontend_unit.cpp`,
`frameloop.cpp`, `mission.cpp`, `reward_port.cpp`, `lego_port.cpp`,
`game.cpp`), and so do `test_frontend.cpp` and `test_record.cpp`. As in
findings 595, 597 and 600, `bitmap_font.cpp` is renamed
`bitmap_font_port.cpp` and becomes a `tail` entry. Each `// FUNCTION`
becomes a `/* Port model of 0x… */` comment. The port's naked-asm
`object_field_be4_dword_get` is deleted: Font_GetHeight matches in C, and
the port keeps its NULL-guarded C body.

### Link names

In the matching build, `font_link_names.h` gives the unit's functions the
port's names wherever every caller of that name passes the original's
arguments:

- Font_Load, Font_GetCharWidth, Font_GetHeight and Font_Remove take
  `font_atlas_record_create`, `bitmap_font_glyph_measure_width`,
  `object_field_be4_dword_get` and `font_atlas_record_release`;
- Font_VGetStringInfo, Font_PrintF, Font_VPrintF and Font_OutputChar keep
  the names of the port's original-ABI adapters (`font_v_get_string_info`,
  `font_print_f`, `font_v_print_f`, `font_output_char`).

`bitmap_font_port.cpp` compiles its definitions of those eight names out of
the matching build behind `LRR_FONT_PORT_NAMES`, set by an explicit `#if`
(finding 590). The adapters `font_get_string_info` (below) and the port's
names for the other addresses (`formatted_text_format_draw`,
`reward_text_draw`, `formatted_text_draw`,
`formatted_text_draw_and_measure`, `fe_font_glyph_draw_and_measure`) keep
their port definitions in every build: their contract is the port's.

Natively, the four adapter names would collide, so the unit's definitions
take `font_unit_*` names there (the `#else` branch, as in
`tooltip_link_names.h`), and nothing calls the unit.

So these callers reach the unit with no caller edit:

- C: `textwindow.c` (Font_OutputChar, Font_GetCharWidth; finding 600's
  operand left in `00485390`/`00485710` is closed), `helpwindow.c`,
  `tooltip.c`, `reward.c`, `lego_unit.c`, `info_unit.c`,
  `interface_unit.c` (Font_PrintF, Font_GetHeight, Font_Load, Font_Remove)
  and `panel_unit.c` (Font_VGetStringInfo, Font_VPrintF; `0045a460` becomes
  exact);
- C++: the callers of `font_atlas_record_create`,
  `font_atlas_record_release` and `object_field_be4_dword_get`
  (`frontend.cpp`, `frontend_unit.cpp`, `reward_port.cpp`, `mission.cpp`,
  `game.cpp`).

### Caller switches

Two caller functions are edited, both in `advisor.c` (GAME-001), whose two
Font.h inlines had the port's contract:

- **`00401c50` Font_GetStringWidth** (`formatted_text_printf_width`) now
  calls `font_v_get_string_info(font, &width, NULL, msg, args)`, the
  original's call. Its signature is unchanged, and natively the adapter does
  what the old `formatted_text_format_draw` call did. 29.17% to **100%**.
- **`00401c80` Font_GetStringInfo** (`formatted_text_printf`) had no caller.
  It takes the original's signature `(font, width, lineCount, msg, ...)` and
  calls `font_v_get_string_info`. 26.92% to **100%**. Its declarations in
  `advisor.h`, `frontend.hpp` and `tooltip_imports.h` change to match.
- **ToolTip.c** (`tooltip.c`, `0046b520`) now calls Font_GetStringInfo by
  that name (`formatted_text_printf`), as the original calls `00401c80`.
  The `PORT-ONLY` adapter `font_get_string_info` in the port tail retires.
  `0046b520` goes from 96.74% to 97.83%.

**HelpWindow.c** and **TextWindow.c** needed no edit (above). No file owned
by #648 (`placement.cpp`), #649 (`gods98/avi_player.cpp`), #651
(`map_view.cpp`) or #652 (`text.cpp`) is edited.

### Callees

- **Images.c** is not converted. Each of its entries that the unit calls
  either has a port contract on a `TextureSurface` or is not written. The
  unit therefore calls:
  - Image_LoadBMPScaled and Image_DisplayScaled through the existing
    adapters in `images.h` (`texture.cpp`);
  - Image_SetPenZeroTrans through the existing `image_set_pen_zero_trans`
    adapter, not the port's `resource_record_add_ref` at `0047dbc0`;
  - four new `PORT-ONLY` adapters added to `texture.cpp`: Image_GetPen255,
    Image_GetPixelMask, Image_LockSurface and Image_UnlockSurface (which
    takes the place of the port's `surface_directdraw_unlock` at
    `0047ded0`). They are declared only in `font_imports.h`.

  The unit reads each glyph cell's top row as four bytes, most significant
  first. So the adapters return the pen and the mask in that byte order
  over the 32-bit `0xAARRGGBB` surface, and the matching build's fonts trim
  as the original's do.

  Image_Remove calls `image_surface_release` (`0047d470`), as `reward.c`
  already does. Converting Images.c closes the six operands in `00479ee0`.
- **The C library.** `malloc` (through `crt/vc5names.h`), `vsprintf`,
  `memset` and `isdigit` (`<ctype.h>`).

### Source shapes that matched

- **Font_Load** (`00479ee0`, 9.93% to 75.43%). The pixel offset is computed
  from `(float)(x * width)` and `(float)(y * height)`, not read back from
  `posSet`. This lets MSVC keep the converted values, hoist the row's
  `__ftol` out of the trim loop and base the cell pointer on `.width` as the
  original does. Reading the rectangle back scored 35.39%. Mixing the two
  scored 41.97% and 53.26%. The pixel is
  `((((b0 << 8) | b1) << 8 | b2) << 8 | b3)`, the original's byte order.
- **Font_VPrintF2** (`0047a1e0`, 9.73% to 97.26%):
  - `return lineWidth > maxWidth ? lineWidth : maxWidth;` gives the
    original's stack layout and tail. An `if` assignment before `return`
    scored 84.94%.
  - `for (hex = 0, shift = 28; ...)` gives the original's register setup.
- **Font_OutputChar** (`0047a470`): `yPos = c / 10` before `xPos = c % 10`
  (60.66% to 98.36%).

No naked asm and no `Deviation (scoring only)` are in the unit.

## Entropy and drops

`make reccmp-check` against `main`'s baseline at `18cc9483`:

- 26 improved and 2 newly mapped (`0047a580`, `0047a5c0`);
- no exact match lost;
- one drop, flagged for owner review (below).

The baseline is refreshed with `make match-progress`, after which
`make reccmp-compare` passes. The plain-immediate check (finding 584),
`winapi-link-check` and the link-order map check pass.

**The drop: `00462ad0` Reward.c `reward_scroll_update`, 85.59% to 85.16%**
(`drift-review`, instruction count unchanged at 234; the classifier marks
it OWNER REVIEW). It is finding 538's rule B:

- `reward.c` includes `advisor.h`, and `00401c80`'s prototype there now
  has the original's four parameters instead of the port's ten. That moves
  the counter phase at `00462ad0`.
- Measured in the single-function loop as a diagnostic only, not committed:
  - with the old prototype restored, 85.16%;
  - with the new one, 84.30%;
  - with an inert prototype of N `int` parameters, 84.30% for N ≤ 6 and
    85.16% for N ≥ 8;
  - with no prototype, 84.30%.
- The cost depends only on the parameter count, so no spelling of the
  original's four-parameter signature restores the phase without filler.
- In the first full build, before that prototype changed, the same function
  rose to 86.02%: the Font_PrintF operand it calls now matches.
- Keeping the port's contract for `00401c80` instead drops `00401c80` itself
  (26.92% to 23.08%), because its callee `0047a150` no longer carries the
  port's name. The switch is kept, which makes `00401c80` and `00401c50`
  exact. The drop is reported for owner review and has not been reviewed.

`frontend.hpp`'s copy of the same prototype moves two C++ functions'
listings the same way. Neither drops:

- `00411360` (`frontend_unit.cpp`): 314 instructions, score unchanged;
- `00459470` `osf_read` (`object_state_file.cpp`): 9.71% to 20.28%, 136 to
  147 instructions.

### Instruction counts (listing digests)

| Address | before | after |
| --- | ---: | ---: |
| `00401c50` | 22 | 13 |
| `00401c80` | 25 | 13 |
| `00411360` | 314 | 314 (sequence changed) |
| `00459470` | 136 | 147 |
| `00462ad0` | 234 | 234 (sequence changed) |
| `00479ee0` | 154 | 202 |
| `0047a150` | 92 | 18 |
| `0047a180` | 23 | 15 |
| `0047a1b0` | 34 | 15 |
| `0047a1e0` | 240 | 201 |
| `0047a470` | 46 | 61 |
| `0047a520` | 50 | 10 |
| `0047a550` | 14 | 11 |
| `0047a580` | — | 18 |
| `0047a5c0` | — | 36 |

`0047a540`'s C body produces the naked asm's listing. Every other function
has an identical listing. The other caller gains (`00418a00`, `00419f30`,
`0041ec30`, `0041eca0`, `0041f740`, `004230c0`, `004232e0`, `004248b0`,
`0045a460`, `00461790`, `00461c90`, `0046bc30`, `00485390`, `00485710`)
come from call operands that now match.

## `/OPT:REF` (finding 579, `make opt-ref-callers`)

| | before (`main` `18cc9483`) | after |
| --- | ---: | ---: |
| `opt_ref.annotated.full`: removed / exact removed | 500 / 243 | 497 / 241 |
| `opt_ref.annotated.game_only` | 481 / 230 | 478 / 228 |

`make opt-ref-callers OPT_REF_ARGS="--object font.c.obj"` lists no
function: all 11 unit functions are kept. Their callers are the converted C
units and the port's C++ callers of Font_Load, Font_Remove and
Font_GetHeight.

## Reccmp lint ratchet

1,239 → 1,236 entries (−3, no growth). These are the port's three static
helpers (`fe_font_key_hit`, `fe_font_ink_box`, `fe_blit_glyph`). They were
unannotated code in a unit file and are now in a tail file.

## Native behaviour

Unchanged:

- The port tail serves every port name natively, including the four
  original-ABI adapters.
- The unit compiles natively as C++ under its own names. Nothing native
  calls it.
- `advisor.c`'s `00401c50` now reaches `formatted_text_format_draw` through
  the `font_v_get_string_info` adapter with the same arguments.
  `00401c80` and `tooltip.c` call the same adapter that `tooltip.c` called
  before, now through `00401c80`.

There is no new native gap.

**Matching-build runtime** (`make reccmp-run` only). Every font there is
now the original's record, made by Font_Load from the Image adapters. So
Font.c's C callers, TextWindow.c (line height at `+0xbe4`) and HelpWindow.c
read it correctly. The port's C++ drawing entries (`formatted_text_draw`
and the others) still expect a `TextureSurface`, as they did before this
change with the port's `FontAtlasRecord`. Image_DisplayScaled is still the
port's no-op adapter, so nothing is drawn.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox): one drop, flagged
  above. After `make match-progress`, `make reccmp-compare`: OK.
- `make match-progress` (baseline, listing digests, `binary-progress.json`)
  and `make unit-data-offsets`. `tools/binary_progress_inputs.txt` is
  regenerated (`binary_progress_hash.py --write-manifest`).
- `python3 tools/translation_units.py build`: 0 conflicts.
- `make tools-test`, without `--allow-growth`: lint (ledger shrinks), link
  order, winapi list, Meson parity and the tool tests are OK.
- `make winapi-link-check`: OK.
- `font.c` passes `gcc -std=gnu89 -Werror=declaration-after-statement
  -fsyntax-only`.
- Native: `cmake -S . -B build -DTEST_WADS_DIR=<shipped WADs> && cmake
  --build build && ctest --test-dir build`: 39/39, real-data tests
  included.

## Follow-ups

- **Images.c (G98-011)**: its conversion gives the unit's eight Images.c
  calls their operands and retires the four new adapters in `texture.cpp`.
- **`00479ee0`**: the x87 spill choice; **`0047a580`**: the `esi` address;
  **`0047a1e0`**: the `*msg` reload (above).
- **`00462ad0`**: the rule B drift above. Owner review.

## Rebased onto stack (#655)

This section gives the figures after rebasing onto #655 (`229d12be`,
Text.c), which sits on `main` `321fdbf1` (#653). Every per-function score
above is unchanged by the rebase.

Text.c (`game/text.c`) makes no Font.c call: it reads the font height at
`+0xbe4` inline, and the call graph has no edge from it into the unit. So no
caller switch is added.

`make reccmp-check` against #655's baseline gives the same 26 improvements
and 2 newly mapped functions. No exact match is lost. The same one drop
remains, `00462ad0` 85.59% to 85.16%, flagged for owner review (above). The
listing digests differ from #655's at the same 15 addresses as on `main`.

| | #655 head | this change |
| --- | ---: | ---: |
| Exact-function code bytes | 184,977 | **185,499** |
| Accuracy, full / game-only | 84.47% / 82.95% | 84.77% / 83.29% |
| `opt_ref` full: removed / exact removed | 497 / 243 | 497 / 244 |
| `opt_ref` game-only | 478 / 230 | 478 / 231 |
| Lint ledger (file lines) | 1,239 | **1,236** |

`make opt-ref-callers OPT_REF_ARGS="--object font.c.obj"` still lists no
function. `make match-progress`, `make unit-data-offsets`, `make tools-test`
(no `--allow-growth`), `make winapi-link-check` and native ctest (39/39,
real-data tests included) pass on the rebased tree.

## Reproducing

    git mv src/gods98/bitmap_font.cpp src/gods98/bitmap_font_port.cpp   # and make it a tail entry
    python3 tools/convert_unit.py G98-008 --stem font   # with font.h moved aside
    make match-function SOURCE=src/gods98/font.c
    python3 tools/msvc_permuter.py 0x00479ee0 --seconds 420 --workers 3 --output build-match/search-479ee0
    make reccmp-check
    make opt-ref-link opt-ref
    make opt-ref-callers OPT_REF_ARGS="--object font.c.obj"
    make match-progress
    make unit-data-offsets
