# Finding 613 — Init.c (LATE-002) rebuilt in original shape

Issue #670, under [ADR 0005](../adr/0005-original-shape-c-units.md). LATE-002's
functions were port models in two files:

- `0049d030` sat in `src/gods98/surface.cpp`, with its annotation above the
  wrong function (`directdraw_device_selection_cancelled`, 0.00%);
- `0049d5e0`–`0049d9d0` sat in `src/crt/crt.cpp`, the CRT stand-in, over the
  port's own mode and resolution lists.

This change rebuilds them as the C unit [`src/gods98/init.c`](../../src/gods98/init.c),
with the C boundary headers [`init.h`](../../src/gods98/init.h) and
[`init_imports.h`](../../src/gods98/init_imports.h). It moves the last game
code out of `crt.cpp` (#447, finding 608).

The unit is gods98's start-up display selection:

- `Init_Initialise` enumerates the DirectDraw drivers. With `best` it walks
  them from the last, and takes the first valid mode of the first driver
  that has a hardware device. Otherwise it runs the "Mode Selection" dialog.
- It then calls DirectDraw.c's `DirectDraw_Setup` (finding 596).

"Before" is `main` at `8abe8b6b`, built in this worktree. "After" is this
change on it.

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (9 functions, weighted by span, 2,688 bytes) | 11.57% | **100.00%** |
| Exact functions in the unit | 0 | **9 of 9** |
| Mapped functions in the unit | 8 | 9 |
| Exact-function code bytes (`binary-progress.json`) | 196,138 | **198,004** |
| Accuracy, full / game-only | 87.16% / 85.90% | 87.39% / 86.15% |

| Address | gods98 name | Function, before | before | Function, after | after |
| --- | --- | --- | ---: | --- | ---: |
| `0049d030` | Init_Initialise | `directdraw_device_selection_cancelled` (misplaced annotation) | 0.00 | `init_initialise` | **100.00** |
| `0049d2f0` | Init_DialogHandler | — | unmapped | `init_dialog_handler` | **100.00** |
| `0049d5e0` | Init_SetFullScreen | `display_mode_dialog_set_mode_and_refresh_choices` | 5.56 | `init_set_full_screen` | **100.00** |
| `0049d630` | Init_SetModeList | `display_mode_list_populate` | 11.01 | `init_set_mode_list` | **100.00** |
| `0049d780` | Init_SetDeviceList | `display_resolution_combo_populate` | 5.88 | `init_set_device_list` | **100.00** |
| `0049d870` | Init_AddValidMode | `display_resolution_filter_tuple_append` | 84.00 | `init_add_valid_mode` | **100.00** |
| `0049d8d0` | Init_IsValidMode | `display_mode_matches_filter_tuple` | 29.27 | `init_is_valid_mode` | **100.00** |
| `0049d950` | Init_GetMode | `display_mode_name_index_find` | 76.34 | `init_get_mode` | **100.00** |
| `0049d9d0` | Init_HandleWindowButton | `display_refresh_rate_controls_sync` | 17.65 | `init_handle_window_button` | **100.00** |

The binary attests none of these names; they are the gods98 `Init_*` names in
snake case, as DirectDraw.c's are. Every function keeps its size. No score
changed outside the unit. There is no naked asm, no filler declaration, and no
`Deviation (scoring only)` in the unit.

## Unit extent and name

The extent runs from `0049d030` to `0049dab0`, where LATE-003 (LIBC
`strnicmp`) starts. It is confirmed, and one function is added:

- **The dialog procedure, `0049d2f0`.** Its only reference in the image is
  `push 0x49d2f0` at `0049d21a`, the `DialogBoxParamA` call in `0049d030`.
  It has its own prologue after `0049d030`'s padding and returns with
  `ret 0x10` (a `DLGPROC`). Ghidra left it as a label, so the corpus manifest
  and the unit map do not list it, like DirectDraw.c's callbacks (finding
  596). Reccmp maps it through its annotation.
- **Call graph** (`docs/coverage/call-graph/edges.csv`). The only outside
  caller is WinMain (`004777a0`), which calls `0049d030`. The unit calls:
  - DirectDraw.c (`0047c1c0`, `0047c2e0`, `0047c4b0`, `0047c610`);
  - Main.c's `0x00477f80` (the desktop bit depth);
  - LIBC's `_sprintf` (`0048d9e0`), plus an inlined `strcmp`;
  - USER32 through the IAT.
- **State.** A scan of `.text` for dwords in `005580a0`–`005754e3`, checked
  by disassembly, finds only the unit's functions. The byte-scan hits
  elsewhere are not operands. The range is one block of `0x1d444` bytes with
  no gaps:
  - the three counts;
  - the driver, device and mode lists that DirectDraw.c fills (20 × `0x114`,
    20 × `0x114` and 200 × `0x110` bytes);
  - the selected driver, device and mode;
  - the full-screen and was-full-screen flags;
  - 200 valid-mode filters (`0x110`-byte mode records);
  - their count, at `005754e0`.

  So it is one common, `initGlobs`. LATE-001's commons end at `00558093`
  (finding 610).
- **Strings** `004b2d0c`–`004b2d90` follow LATE-001's `"RNC"`:
  - "Please install DirectX version 6 or later", "Window Size",
    "Screen Mode", "No supported screen modes found",
    "No supported %i bit modes found" and "Error: DirectX6 not installed.";
  - `"Error"` (`004aaf40`) is pooled with G98-014 and stays unmarked.

**The name.** The binary names no file; Main.c, Images.c and the others are
named by their `C:\Dev\SourceSafe\gods98_dx6\gods98\src\*.c` assertion
strings, and this unit has none. The evidence for gods98's **Init.c** is:

- it is the engine's start-up initialisation, called once from WinMain
  before any other display code;
- its dialog is resource 101, captioned "Mode Selection". Its controls are
  the Driver (1000), Device (1002) and Screen Modes (1003) list boxes, the
  Full Screen (1004) and Window (1005) radio buttons, and the list label
  (1006).

The stem is `init`. `translation-units.spec.csv` records this in LATE-002's
note and leaves the name column empty, as for LATE-001.

## The unit

### State

`initGlobs` (`005580a0`, 119,876 bytes), one file-scope definition of
`Init_Globs`, so it is a common as in the original (`make unit-data-offsets`:
`005580a0:_initGlobs(size=119876)`). The lists reuse DirectDraw.c's
`DirectDraw_Driver`, `DirectDraw_Device` and `DirectDraw_Mode` records
(`directdraw.h`). It is defined in every build. The native port, which does
not build the unit's code, keeps its valid-mode filters in it too (below).

The old port storage loses its annotations: `crt.cpp`'s `s_filter_tuples`
(`00568060`) and `s_filter_tuple_count` (`005754e0`).

### Imports

- The USER32 calls go through the IAT (`call [__imp_…]`), as the original's
  do; `windows.h`'s declarations give that. `MoveWindow` is newly imported
  (USER32 thunks 41 → 42), as in the original.
- `DialogBoxParamA` takes Main.c's instance and window (`00505804`,
  `00505800`):
  - The window is `g_platform_cursor_window_handle`, already annotated at
    `00505800`.
  - The instance had no definition. `g_main_instance_handle` now gets its
    `// GLOBAL` at `00505804`. It is defined at the end of
    `platform/winapi/frameloop.cpp`, after every function, so that file's
    symbol numbering does not move (finding 538).
  - The port never sets it. NULL makes `DialogBoxParamA` load the template
    from the executable, the same module.

### Shape notes

- **Temporaries for `GetDlgItem`.** Each `GetDlgItem` result goes into a local
  before it is used (`hWndButton = GetDlgItem(…); SendMessageA(hWndButton, …)`).
  Nested as an argument, the call is scheduled after the other arguments'
  pushes: `0049d9d0` is then an effective match only (87.34%). The dialog
  procedure also stores `GetDesktopWindow()` in a local before
  `GetWindowRect` (98.37% without it).
- **The dialog procedure** keeps the `LB_GETCURSEL` result in a register
  local. It passes a separate stack local (`mode`, at the frame's base) to
  `Init_GetMode`. With one variable for both, it spills. Together with the
  nested `GetDlgItem` and the width and height locals below, that gave
  86.52%.
  The `MoveWindow` arguments are written inline, `(desktop.right / 2) -
  ((dlg.right - dlg.left) / 2)`. Named width and height locals reorder the
  arithmetic.
- **`Init_IsValidMode`** returns TRUE for an empty table through the
  function's shared `return TRUE` exit: `if (count) { for … return FALSE; }
  return TRUE;`. An early `if (!count) return TRUE;` gets its own epilogue
  (85.22%).
- **`Init_SetDeviceList`** assigns the `LB_DELETESTRING` result to a
  variable (`result = …; while (result != LB_ERR) result = …;`), and sends
  `LB_SETCURSEL` once after the `if`/`else`. With `Init_SetModeList`'s bare
  `while (SendMessageA(…) != LB_ERR);` the function is an effective match
  only: the list handle, the loop counter and the zero register swap among
  `esi`, `ebx` and `edi`. These variants were also tried:
  - six declaration orders;
  - a device pointer local;
  - a separate `LB_SETCURSEL` in each branch;
  - the selection after the `if`;
  - the inverted condition;
  - the zero as a statement.

  None beat the effective match, and `tools/msvc_permuter.py` found no
  candidate. The include order, the prototype order and the place of the
  `initGlobs` definition do not move any of the nine functions.
- **`Init_Initialise`** sets the selected driver, device and mode in that
  order, then `found`. The loop leaves on `found == TRUE` (`cmp ebx, 1`), and
  the failure test is `!found`.
- **Strings.** The rebuilt `"Window Size"` and `"Screen Mode"` are pooled
  with `surface.cpp`'s identical literals (port code linked earlier), so
  `make unit-data-offsets` shows LATE-002's `.data` as partial: the four
  other literals keep the original's order (`004b2d50` at `+0x2c` against
  `+0x44`). That is the port's pooling, not the unit's order.

## Port and callers

The port never ran the original's flow. It must stay as it is:

- The WinAPI reference backend runs `directdraw_device_initialize`, an
  ops-driven model of `Init_Initialise`, from `frameloop.cpp`, and the port
  renderer does not use DirectDraw.c.
- The native backend calls none of this.

So no caller switches to `init_initialise`. As DirectDraw.c's functions
were, the unit is linked for its scores, and `/OPT:REF` removes all of it
except `Init_AddValidMode` (below). WinMain (`entry.cpp`) is not edited:
calling the unit would run the original dialog and DirectDraw setup beside
the port's.

- **`surface.cpp`** (legacy): the misplaced `0049d030` annotation is removed.
  `directdraw_device_selection_cancelled` and `directdraw_device_initialize`
  stay, as before, marked `// PORT-ONLY:`. No declaration is added or removed,
  so the file's code is unchanged.
- **`crt.cpp`** (legacy): removed are the seven annotated functions, the
  port's mode and resolution lists, their append, count and get accessors,
  and the filter-tuple helpers. The dialog model, the lists and their
  accessors had no caller outside `crt.cpp`.
- **`crt.hpp`** is not edited, so the symbol numbering of its 96 includers
  does not move (rule B, finding 538). Its superseded types and declarations
  stay and are tagged `Deviation (scoring only)`.
- **`gods98/init_port.cpp`** (new port tail) defines the four filter helpers
  that the port model and `test_crt` call. They now read and write the
  unit's `initGlobs.validModes`. In the Windows builds an append is the
  unit's `Init_AddValidMode` itself; the native build stores the same three
  fields. Natively and under WinAPI the port behaves as before. The table
  keeps its 200 rows, and an append is unchecked, as before and as in the
  original.
- **Tests.** `test_crt`'s filter-tuple case passes unchanged. `init.h` and
  `init_imports.h` join `test_c_boundaries.cpp`.

`init.c` is C in the matching build. Natively it compiles as C++ and defines
only `initGlobs`. It passes `gcc -std=gnu89 -Werror=declaration-after-statement
-fsyntax-only` and `g++ -std=c++17 -Wall -Wextra -fsyntax-only` with the
native backend. No MinGW toolchain was available to compile the Windows part
as C++, the reference build's mode.

## Port-only `.bss` (finding 602)

The port's `s_display_modes` (2 × 256 × 272 = 139,264 bytes) and
`s_display_resolutions` (256 × 264 = 67,584 bytes) are retired, not shrunk:
nothing outside `crt.cpp` read them. `s_filter_tuples` (54,400 bytes)
becomes the unit's table.

| | before | after |
| --- | ---: | ---: |
| Gate `.bss` (map group) | 3,716,992 | **3,575,632** (−141,360) |
| — legacy objects | 486,936 | 225,672 (−261,264) |
| — commons | 2,149,448 | 2,269,328 (+119,880, `initGlobs`) |
| Gate `.data` virtual size | 3,822,160 | 3,680,944 |
| Gate image end | `0x8e9000` | **`0x8c6000`** |
| Below `0x800000` by | −954,368 | −811,008 |
| `/OPT:REF` relink `.bss` | 1,668,960 | 1,527,584 |
| `/OPT:REF` relink image end | | `0x68e000` |

Since finding 602, `main` had grown from `0x8c6000` to `0x8e9000`. This change
brings the gate back to `0x8c6000`. The gate is still 811,008 bytes past
`0x800000`, so `tools/reccmp_plain_immediates.py` is still needed (finding
584).

## Late members (finding 608)

The four late LIBC members don't move. In the rebuilt map they still sit
inside LIBC, before the bulk of the import thunks:

- `strnicmp` (`libc:strnicmp.obj`);
- `xtoa` (`__itoa`);
- `strlwr`;
- `strupr`.

The order of objects, LIBC members and thunks is unchanged from `main`. Only
the addresses move, by `init.c`'s size.

Finding 608 guessed that LATE-001 and LATE-002 pulled in these members. That
guess fails for both:

- `init.c` calls no CRT routine but `_sprintf`, an early member, and an
  inlined `strcmp`;
- `rnc.c` calls only `malloc`, `free` and `strncmp` (finding 610).

The original's callers of the late members are game units:

- eleven functions call `0049dab0` (`strnicmp`), among them `00413f90`,
  `00427f80` and `00477610`;
- `0041afd0` and `0045d060` call `0049db60` (`itoa`);
- `00473c60` calls `0049dc10` (`strlwr`);
- WinMain `004777a0` calls `0049dcd0` (`strupr`).

So their late placement more likely comes from how those callers' old names
resolve through `oldnames.lib`, or from LATE-001 and LATE-002 being library
members themselves. Either way, it is #447's question.

## `/OPT:REF` (finding 579)

| | before | after |
| --- | ---: | ---: |
| `opt_ref.annotated.full`: removed / exact removed | 428 / 207 | 433 / 218 |
| `opt_ref.annotated.game_only`: removed / exact removed | 421 / 200 | 426 / 211 |
| Import thunks in the gate / removed | 163 / 153 | 164 / 154 |

The removed annotated functions, compared by address between the two
relinks:

- **The unit, +2 removed, +8 exact removed.**
  - Before, the six `crt.cpp` models at `0049d5e0`–`0049d9d0` were removed,
    none exact. `0049d870` and the misplaced `0049d030` were kept.
  - After, eight of the nine are removed, all exact.
  - `Init_AddValidMode` (`0049d870`) is kept: the port model reaches it
    through `init_port.cpp`.
  - `make opt-ref-callers OPT_REF_ARGS="--object init.c.obj"` gives
    `init_initialise` as every function's root. In the rebuild nothing calls
    it.
- **Identical-code folding, +3 removed, +3 exact removed.** LINK keeps the
  other twin of several unit/port pairs (finding 608):
  - `engine_memory_allocate`/`engine_memory_free` are kept now, and their
    twins `file_malloc`/`engine_heap_free_wrapper` are removed;
  - `lego_terrain_cell_palette_resource_at_grid_position` (`00429160`),
    `legoobject_grid_record_count_tables_clear` (`00437440`) and
    `legoobject_flags_contain_requested_single_bit` (`00438ed0`) are newly
    removed, their port twins kept.

  No code changed in them.

## Entropy

`make reccmp-check` against `main`'s baseline passes:

- 8 functions improved and 1 is newly mapped;
- no function fell, and no exact match was lost;
- no score changed outside the unit, so there are no drops to label and no
  instruction-count changes;
- the plain-immediate check, `winapi-link-check` and the link-order map check
  are OK.

The link-order keys of `surface.cpp` (`00478330` → `004781d0`) and `crt.cpp`
(`00494c70` → `00493ec0`) follow their medians. Neither entry moves.

## Reccmp lint ratchet

1,080 → 1,069 entries (−11, no growth). The retired `crt.cpp` definitions
leave the ledger, and `directdraw_device_initialize` is now `PORT-ONLY`.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox): OK.
- `make match-progress`: the three generated files.
- `make unit-data-offsets`: the LATE-002 row above.
- `make pe-audit`: imports 163 → 164 names (`MoveWindow`), 10 DLLs, common
  order equal.
- `python3 tools/translation_units.py build`: 0 conflicts.
  `tools/binary_progress_inputs.txt` is regenerated.
- `make tools-test`: OK, without `--allow-growth`. Meson lists `init.c` and
  `init_port.cpp` (`meson_source_parity.py`).
- Native: `cmake -S . -B build -DTEST_WADS_DIR=<shipped WADs> && cmake
  --build build && ctest --test-dir build`: 39/39, real-data tests included.

## Rebased onto #678 (stack #676 → #677 → #678)

The figures above were measured on `main` at `8abe8b6b`. On #678's head
(`4a5e3a19`, GAME-011 on #677 and #676), built in the same worktree, before
and after this change:

| | #678 | #678 + this change |
| --- | ---: | ---: |
| Exact-function code bytes | 201,327 | **203,193** |
| Accuracy, full / game-only | 87.75% / 86.54% | 87.97% / 86.79% |
| `opt_ref.annotated.full`: removed / exact removed | 414 / 215 | 415 / 222 |
| `opt_ref.annotated.game_only`: removed / exact removed | 407 / 208 | 408 / 215 |
| Gate `.bss` (map group) | 3,717,008 | **3,575,616** (−141,392) |
| Gate `.data` virtual size | 3,822,224 | 3,680,976 |
| Gate image end | `0x8e9000` | **`0x8c7000`** |
| `/OPT:REF` relink `.bss` | 1,668,976 | 1,527,600 |
| Lint ledger | 1,071 | 1,060 |

- **The unit** is unchanged: 9 of 9 exact, and `0x0049d2f0` newly mapped.
- **Drops:** `make reccmp-check` passes; no function fell, and no score
  outside the unit changed.
- **`opt_ref`:** the unit accounts for +2 removed and +8 exact removed, as on
  `main`. The remaining −1 / −1 is identical-code-folding or layout noise
  outside the unit (finding 591). It was not identified by address on this
  base. The three twin swaps listed above were measured on `main`.
- **Late members:** the order is unchanged.
- **The rebase** conflicted only in the shared bookkeeping, and both sides
  were kept: `test_c_boundaries.cpp`, the findings README, and the three
  generated files, which were regenerated.

## Reproducing

    python3 tools/convert_unit.py LATE-002 --stem init
    make match-function SOURCE=src/gods98/init.c
    make reccmp-check
    make match-progress
    make unit-data-offsets
    make opt-ref-link opt-ref
    make opt-ref-callers OPT_REF_ARGS="--object init.c.obj"
