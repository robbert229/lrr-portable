# Finding 600 — TextWindow.c (G98-017) rebuilt in original shape, with a port tail

Issue #639, part of #388 under [ADR 0005](../adr/0005-original-shape-c-units.md).
G98-017 is gods98's `TextWindow.c` (text `004850f0`-`00485a20`). A text
window is a 0x830-byte record that holds:

- a font and a screen area;
- a ring buffer of printed text, with the start of each wrapped line;
- an overlay string, shown for a while instead of the ring buffer.

TextWindow_Update wraps the ring buffer at word breaks and draws the lines
that fit, newest at the bottom. HelpWindow.c, FrontEnd.c, Panel.c, Lego.c,
Reward.c, Text.c and Objective.c use it for the help text, the scrolling
message and info panels, the dialogs and the objective text.

#496 relocated the unit verbatim into `src/gods98/text_window.cpp`. This
change rebuilds it as the C unit [`src/gods98/textwindow.c`](../../src/gods98/textwindow.c),
with:

- the unit-private imports header
  [`textwindow_imports.h`](../../src/gods98/textwindow_imports.h);
- the matching-build link names
  [`textwindow_link_names.h`](../../src/gods98/textwindow_link_names.h).

The port's implementation moves, unannotated, to the port tail
[`text_window_port.cpp`](../../src/gods98/text_window_port.cpp). The
existing boundary header [`textwindow.h`](../../src/gods98/textwindow.h)
keeps its six declarations.

"Before" is the *match baseline* on `main` at `8601d8ab` (#635). "After" is
this change, from the full matching build (`make reccmp-check`).

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (13 functions, weighted by function span, 2,352 bytes) | 29.99% | **94.79%** |
| Exact functions in the unit | 5 (one naked asm) | **9** (no asm) |
| Exact-function code bytes (`binary-progress.json`) | 178,866 | **179,079** |
| Accuracy, full / game-only | 83.30% / 81.67% | 83.49% / 81.87% |

| Address | gods98 name | Function (matching build) | Span | before | after |
| --- | --- | --- | ---: | ---: | ---: |
| `004850f0` | TextWindow_Create | `text_buffer_create` | 112 | 50.00 | **100.00** |
| `00485160` | TextWindow_EnableCentering | `text_output_field82c_bit2_set_clear` | 48 | 64.29 | **100.00** |
| `00485190` | TextWindow_Remove | `text_buffer_destroy_target` | 32 | 100.00 | **100.00** |
| `004851b0` | TextWindow_ChangePosition | `float_pair_at_offset4_set_from_ints` | 32 | 19.35 | **100.00** |
| `004851d0` | TextWindow_ChangeSize | `float_pair_at_offsetc_set_from_uints` | 64 | 32.65 | **100.00** |
| `00485210` | TextWindow_PagePrintF | `formatted_text_append_to_ring_buffer` | 32 | 100.00 | **100.00** |
| `00485230` | TextWindow_PrintF | `text_buffer_append_format` | 32 | 100.00 | **100.00** |
| `00485250` | TextWindow_VPrintF | `text_buffer_append_vformat` | 176 | 25.00 | 98.21 |
| `00485300` | TextWindow_PrintFOverlay | `objective_advisor_text_copy` | 144 | 82.22 | 97.62 |
| `00485390` | TextWindow_Update | `text_buffer_draw_wrapped` | 896 | 5.96 | 87.80 |
| `00485710` | TextWindow_UpdateOverlay | `text_wrap_measure_and_draw_lines` | 672 | 24.94 | 99.01 |
| `004859b0` | TextWindow_Clear | `text_buffer_reset_target` | 80 | 100.00 (naked asm) | **100.00** (C) |
| `00485a00` | TextWindow_GetInfo | `object_state_pair_get_target` | 32 | 100.00 | **100.00** |

The binary attests none of the gods98 names. HelpWindow.c's
`"TextWindowArea"` key names the window area, and the stem follows
finding 595, which already calls the unit TextWindow.c. Each span runs to
the next function, padding included.

### What is left

- **`00485250`, `00485300`**: the `vsprintf` call operand only. Reccmp names
  the original's `0048e1e0` after the port's `formatted_text_buffer_build`,
  as in `utils.c` and `tooltip.c` (finding 597).
- **`00485710`** (99.01%): the Font_OutputChar call operand only (see
  [Callees](#callees)). Every instruction matches.
- **`00485390`** (87.80%). Three differences remain:
  - The scan loop uses a rotated set of scratch registers (`eax`/`ecx`/`edx`).
  - The font pointer is loaded into `ebp` before the `__ftol` call that
    computes `maxLines`. The original loads it into `ecx` after the call.
  - The original compares `sub` with a zeroed register (`xor ebx, ebx;
    cmp ecx, ebx; jbe`) where the rebuild uses `inc`'s flags (`je`).

  Tried without effect, all at 87.43% in the single-function loop:
  - every order of the locals' declarations;
  - 1 to 32 inert declarations before the function (a diagnostic only, not
    committed). The function does not depend on counter phase (finding 538,
    rule B).
  - a `windowBuf` of `unsigned char`;
  - other forms of the `maxLines` division (casts, a temporary, `/=`);
  - other forms of `if (c == '\a') break;` (folded into the loop condition,
    `else if`);
  - other orders of the word-break statements;
  - reusing `y`, `width` or a function-scope `diff` for `closest` and the
    distance;
  - other placements of the overlay `return`;
  - seven forms of the `sub` test. A signed `(int)sub > 0` gives the
    original's `cmp`, with `jle` (80.22%), but it changes behaviour, so it
    is not used.
  - Removing `c` in favour of repeated `windowBuf[loop]` reads scored
    lower (58-68%).

  The bounded permuter (`tools/msvc_permuter.py`, 420 s) found nine
  rewrites and no improvement. For UpdateOverlay it found three.

## Unit extent

The map gave 11 of the 13 functions low confidence. The extent is
confirmed, and no function moves:

- **Call graph** (`docs/coverage/call-graph/edges.csv`). The thirteen
  functions call only each other, Font.c (`0047a470` Font_OutputChar,
  `0047a520` Font_GetCharWidth) and the C library (`malloc`, `free`,
  `vsprintf`, `__ftol`, `_isctype`). Their callers are HelpWindow.c,
  FrontEnd.c (`00412750`, `004129d0`, `00415d00`), Lego.c,
  Panel.c, Objective.c (`00458be0`), Reward.c (`00462ad0`) and Text.c
  (`0046a9e0`, `0046aa90`, `0046ad00`). Font_GetHeight is read inline
  (`[font + 0xbe4]`), not called.
- **The lower edge.** `004850e0` (ObjectField10Get) is Flic.c's. Its
  three callers pass it a FLIC handle:
  - `0045a730` and `0045cc70` pass the same handle to Flic.c's `00484070`;
  - `004121b0` passes `DAT_0055706c`, whose FLIC header fields `+0x646`
    and `+0x624` it also reads.

  Each caller pairs it with `004897d0` (`+0xc`), so the two read the
  handle's width and height. `004850c0` is called only by Flic.c's
  `004848d0` and `00484ca0`.
- **The upper edge.** `00485a20` is Keys.c's (its `KEY_*` strings, finding
  513). `004859b0` and `00485a00` read the window record (`+0x14`,
  `+0x418`, `+0x820`, `+0x824`, `+0x82c`), and Panel.c's `0045a630` calls
  `00485a00` on a text window.
- **Data.** The unit owns no `.data` or `.bss`. A scan of `.text` for
  operands in `0049e9e0`-`0049ea20` finds `0049ea04` (`0.5f`) only in
  `00485390` and `00485710`, and `0049ea0c` (`0.0f`) only in `00485710`.
  `0049ea00` and `0049ea08` (`1.0f`) have no code reference, so they are
  left unattributed. The only other data the unit reads is LIBC's
  `__mb_cur_max` (`004aff74`) and `_pctype` (`004afd68`), through the
  `isspace` macro.

`translation-units.spec.csv` records this evidence in G98-017's note, and
names the unit `TextWindow.c` (`inferred`). The boundary confidence is
computed by `tools/translation_units.py` and stays `low`, because padding
cannot separate C objects (finding 505).

## How the conversion is done

### State

None of its own. The record is the original's `TextWindow` (0x830 bytes):

- the font (`+0x000`) and the area `x, y, width, height` (`+0x004`);
- the ring buffer (`+0x014`), the overlay string (`+0x018`, 1,024 bytes),
  the buffer's length and fill (`+0x418`, `+0x41c`);
- the 256 line starts (`+0x420`), the line count and the lines that fit
  (`+0x820`, `+0x824`);
- the overlay's display time (`+0x828`) and the flags (`+0x82c`: wrapped,
  overlay, centred).

The unit declares Font.c's record as far as it reads it: the height at
`+0xbe4`, which `Font_GetHeight` reads inline. `0.5f` and `0.0f` are
literals. MSVC pools them, and the rebuilt operands match.

`make unit-data-offsets`, G98-017 row:

| Unit | .data | .rdata | .bss | Commons |
| --- | --- | --- | --- | --- |
| G98-017 | n/a (0, 0 unknown) fewer than two attributable items | n/a (0, 0 unknown) fewer than two attributable items | n/a (0, 0 unknown) fewer than two attributable items | - |

Unit summary: full 0, gaps-only 6, partial 27, n/a 192.

### The port tail

The port's text windows are another contract:

- a host-width font sidecar for 64-bit font handles;
- NULL guards;
- drawing through the `TextBufferDrawOps` hooks (`heap.hpp`), which the
  software renderer installs.

Native code calls them (`frontend.cpp`, `frontend_unit.cpp`, `text.cpp`,
`tutorial_hud.cpp`, `info_port.cpp`, `lego_port.cpp`), and so do
`test_heap.cpp` and `test_record.cpp`. As in findings 591, 595 and 597,
`text_window.cpp` is renamed `text_window_port.cpp` and becomes a `tail`
entry, also in Meson. Each `// FUNCTION` becomes a `/* Port model of 0x… */`
comment. The port's naked-asm TextBufferReset is deleted: TextWindow_Clear
matches in C, and the port keeps its C body.

### Link names

In the matching build, `textwindow_link_names.h` gives all thirteen unit
functions the port's names (the table above). On a 32-bit build the port's
record has the original's layout, and every caller passes the original's
arguments. So these callers reach the unit with no caller edit:

- C: `helpwindow.c` (Create, Clear, PrintF, Update), `panel_unit.c`
  (Create, VPrintF, ChangePosition, Update, Clear, GetInfo), `lego_unit.c`
  (Create, PrintF, Remove, Clear, Update) and `reward.c` (ChangePosition,
  Update);
- C++: `frontend.cpp`, `frontend_unit.cpp`, `text.cpp`, `tutorial_hud.cpp`,
  `info_port.cpp`, and the wrappers `heap.cpp` (`text_buffer_destroy`,
  `text_buffer_reset`) and `record.cpp` (`object_state_pair_get`).

`text_window_port.cpp` compiles its definitions of these names out of the
matching build behind `LRR_TEXT_WINDOW_PORT_NAMES`, set by an explicit
`#if` (finding 590). Natively the unit keeps its own names
(`text_window_*`) and nothing calls it.

Five of the port names had C++ linkage in `heap.hpp`, `record.hpp` and
`string.hpp`. They gain `extern "C"` in place:

- `text_output_field82c_bit2_set_clear`
- `formatted_text_append_to_ring_buffer`
- `text_wrap_measure_and_draw_lines`
- `float_pair_at_offsetc_set_from_uints`
- `objective_advisor_text_copy`

None of them is declared in a frozen `_for_`/`_from_` header, and no
declaration is added.

### Caller switches

None was needed. Every caller with the original's contract already called
the port names that the unit now serves:

- **HelpWindow.c** (finding 595) calls Create, Clear, PrintF and Update by
  those names.
- **ToolTip.c** (finding 597) does not call TextWindow.c.

The one original caller the unit cannot reach is Objective.c's `00458be0`
(PagePrintF, Clear, Update). Its port model, `objective_unit.cpp`'s
`objective_tick_target`, has no text-window calls. Objective.c's
conversion can call the unit (see [Follow-ups](#follow-ups)).

No file owned by #637 (`bubbles.cpp`), #638 (`lava_erosion.cpp`), #640
(`gods98/lwo.cpp`) or #641 (port `.bss` sizing) is edited.

### Callees

- **Font_GetCharWidth** (`0047a520`) is `bitmap_font.cpp`'s
  `bitmap_font_glyph_measure_width`, which has the original's contract and
  C linkage. The operand matches.
- **Font_OutputChar** (`0047a470`). The port's entry at that address,
  `fe_font_glyph_draw_and_measure`, takes a render target. A `PORT-ONLY`
  adapter, `font_output_char`, has the original's five arguments and is
  added at the end of `bitmap_font.cpp`. Like `font_print_f`, it measures
  only, because the port's renderer owns the back buffer. It is declared
  only in `textwindow_imports.h`, so no shared header's declaration count
  changes. This is the operand left in `00485390` and `00485710`. Font.c's
  conversion closes it.
- **The C library.** `malloc`/`free` (through `crt/vc5names.h`),
  `vsprintf`, `strcpy`, `memset` and `isspace` (`<ctype.h>`). The `isspace`
  macro reads `__mb_cur_max` and `_pctype`, as the original does
  (finding 509).

### Source shapes that matched

- **TextWindow_Create** (`004850f0`): `if (window = malloc(...))` with
  `memset` and a struct copy of the area. The port's guarded byte copies
  scored 50%.
- **TextWindow_Update** (`00485390`, 5.96% → 87.80%):
  - The `!(flags & OVERLAY)` body comes first, so the overlay `return`
    lands at the end of the function as in the original.
  - `closest` is assigned before the wrapped test, as the original loads
    `windowBufLen` before `test al, 1`.
  - The pen's x **reuses `start`**. Ghidra shows one slot for both, and
    with a separate `x` MSVC pairs x with `wordWidth`, and every stack slot
    of the function moves (78.00% → 87.43%). An inner-block `x` scored
    78.80%.
  - `y` is `if (wrapped && in view) … else if (lineCount <= maxLines) …
    else 0`, which gives the original's block order.
  - `posFromEnd`'s zero case stores `sub`, which is 0 there (86.51% →
    87.43%; the generated code is the original's apart from `je`).
  - The draw loop wraps at `windowBufLen + 1`, as the original's does.
- **TextWindow_UpdateOverlay** (`00485710`, 24.94% → 99.01%):
  - The pen's x reuses `lineWidth` (66.67% → 76.19%).
  - `c = secondaryBuf[sub++]` keeps `c` in memory in the draw loop, as the
    original does (94.06%).
  - `lineWidth = 0; wordWidth = 0;` as two statements (96.04%).
  - `loop` is initialised at its declaration, and `lineCount = 1` follows
    `lineList[0] = 0` (98.02%). In the full build only the callee operand
    is left.
- **TextWindow_Clear** (`004859b0`): two `memset`s. The port used naked asm
  for it.

No naked asm and no `Deviation (scoring only)` are in the unit.

## Entropy and drops

`make reccmp-check` against the baseline at `8601d8ab` passes:

- 8 improved, all in the unit;
- no function below baseline, and no exact match lost;
- the plain-immediate check (finding 584), `winapi-link-check` and the
  link-order map check pass.

No score changed outside the unit. Instruction counts changed only in the
unit (listing digests):

| Address | before | after |
| --- | ---: | ---: |
| `004850f0` | 55 | 38 |
| `00485160` | 13 | 11 |
| `004851b0` | 12 | 6 |
| `004851d0` | 21 | 15 |
| `00485250` | 69 | 56 |
| `00485300` | 43 | 42 |
| `00485390` | 296 | 267 |
| `00485710` | 214 | 202 |

The other five (`00485190`, `00485210`, `00485230`, `004859b0`, `00485a00`)
have identical listings. `004859b0`'s C body produces the naked asm's
instructions.

## `/OPT:REF` (finding 579, `make opt-ref-callers`)

| | before (`main` `8601d8ab`) | after |
| --- | ---: | ---: |
| `opt_ref.annotated.full`: removed / exact removed | 498 / 239 | 494 / 235 |
| `opt_ref.annotated.game_only` | 479 / 226 | 475 / 222 |
| The unit's object: removed | `text_window.cpp.obj`: `00485210` | `textwindow.c.obj`: `00485210` |

`make opt-ref-callers OPT_REF_ARGS="--object textwindow.c.obj"` lists one
function, the same one as on `main`. `00485210` TextWindow_PagePrintF is
exact but removed: its only original caller, Objective.c's `00458be0`, is
live but its port model does not call it. The other twelve are kept.

**The change of four is measurement noise, not this unit.** Both builds
were relinked and diffed by address:

- removed only on `main`: `004021e0`, `00402210`, `00431560`, `00439cd0`,
  `00439cf0`, `00439d10`, `00439d30`, `00472a40`, `0047fe20`, `00484c90`;
- removed only after: `00437440`, `00438ed0`, `00456210`, `00473680`,
  `0047ff30`, `0047ff40`.

These are the duplicate-name swaps finding 591 describes. Names such as
`legoobject_stats_flag10_test` and `file_get_system` are defined in two
objects, and which copy the original-mode relink keeps depends on layout.
The by-object caller lists (`opt_ref_link.py callers`) of the two builds
are identical, 286 functions each.

## Reccmp lint ratchet

1,267 → 1,258 entries (−9, no growth). These are the port's eight static
helpers (`text_buffer_font_binding_add`, `_get`, `text_buffer_font_get`,
`text_buffer_read_u32`, `text_buffer_line_height`, `text_buffer_is_break`,
`text_buffer_glyph_width`, `text_buffer_draw_glyph`) and the
`text_buffer_reset_target` fallback. They were unannotated code in a unit
file and are now in a tail file.

## Native behaviour

Unchanged:

- The port tail serves every port name natively, with the font sidecar and
  the draw hooks.
- The unit compiles natively as C++ under its own names. Nothing native
  calls it.
- The `extern "C"` edits change only the five names' linkage. Their
  definitions and callers see the same declarations.

There is no new native gap.

**Matching-build runtime** (`make reccmp-run` only). The port's text-buffer
records and the unit's are the same record on a 32-bit build, so mixing
port creators and unit users is safe. The unit reads the font height at
`+0xbe4`, as HelpWindow.c's Font_GetHeight call already does. The port's
`FontAtlasRecord` keeps its line height at `+0xbe8`, so text-window line
spacing in `reccmp-run` follows the last glyph cell's stored height until
Font.c is converted. Font_OutputChar measures and does not draw (above).

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox): OK (above).
- `make match-progress` (baseline, listing digests, `binary-progress.json`)
  and `make unit-data-offsets`. `tools/binary_progress_inputs.txt` is
  regenerated (`binary_progress_hash.py --write-manifest`).
- `python3 tools/translation_units.py build`: 0 conflicts.
- `make tools-test`, without `--allow-growth`: lint (ledger shrinks), link
  order, winapi list, Meson parity and the tool tests are OK.
- `textwindow.c` passes `gcc -std=gnu89
  -Werror=declaration-after-statement -fsyntax-only`.
- Native: `cmake -S . -B build -DTEST_WADS_DIR=<shipped WADs> && cmake
  --build build && ctest --test-dir build`: 39/39, real-data tests
  included.

## Follow-ups

- **`00485390`**: the register rotation, the font preload and the `jbe`
  compare above.
- **Font.c (G98-008)**: its conversion gives Font_OutputChar its contract,
  so `00485390` and `00485710` call it directly and the `font_output_char`
  adapter retires. It also gives the font record its `+0xbe4` height in the
  matching build.
- **Objective.c**: its conversion makes `00458be0` call PagePrintF, Clear
  and Update, so `00485210` stops being removed.

## Rebased onto stack (#644)

This section gives the figures after rebasing onto #644 (`0db44a71`,
Bubbles.c), which sits on #643 (`77c9ede5`, lwo.cpp) and `main` `8601d8ab`.
Every per-function score above is unchanged by the rebase. Neither base
change touches TextWindow.c or its callers.

`make reccmp-check` against #644's baseline improves the same 8 unit
functions and nothing else. No function falls below baseline, and no exact
match is lost. The listing digests differ from #644's only at those 8
addresses.

| | #644 head | this change |
| --- | ---: | ---: |
| Exact-function code bytes | 181,048 | **181,261** |
| Accuracy, full / game-only | 83.86% / 82.29% | 84.05% / 82.49% |
| `opt_ref` full: removed / exact removed | 503 / 246 | 504 / 247 |
| `opt_ref` game-only | 484 / 233 | 485 / 234 |
| Lint ledger | 1,249 | **1,240** |

`make opt-ref-callers` still lists one removed unit function, `00485210`,
as on `main`. The extra removed exact function is outside the unit. I did
not relink #644's head to name it. It is of the same kind as the
duplicate-name swaps measured against `main` above.

## Reproducing

    git mv src/gods98/text_window.cpp src/gods98/text_window_port.cpp   # and make it a tail entry
    python3 tools/convert_unit.py G98-017 --stem textwindow   # with textwindow.h moved aside
    make match-function SOURCE=src/gods98/textwindow.c
    python3 tools/msvc_permuter.py 0x00485390 --seconds 420 --workers 2 --output build-match/search-485390
    make reccmp-check
    make opt-ref-link opt-ref
    make opt-ref-callers OPT_REF_ARGS="--object textwindow.c.obj"
    make match-progress
    make unit-data-offsets
