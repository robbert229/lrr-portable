# Finding 511 — DirectX error decoder (G98-027) rebuilt in original shape

Issue #411. This conversion follows the [ADR 0005](../adr/0005-original-shape-c-units.md)
recipe, starting from the `tools/convert_unit.py` scaffold (#405). It
rebuilds the gods98 DirectX HRESULT decoder: original translation unit
G98-027, text `00489d90`–`0048b260`, 2 functions
([finding 505](505-original-translation-units-and-link-order.md)). The unit
is small in code but large in data: 338 literals and a module-name table,
`004ab788`–`004af9e1`.

"Before" is the *match baseline* on `main` at `2afc7287` (#408). "After" is
this change. `make reccmp-check` and `make match-function` give the same
per-function scores.

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (byte-weighted, 5,319 bytes) | 18.34% | **100.00%** |
| Exact functions | 0 | **2** of 2 |
| Game-only mean (2,121 mapped) | 49.21% | 49.29% |
| Full mean (2,340 mapped) | 53.59% | 53.66% |

| Address | Function | Bytes | before | after |
| --- | --- | ---: | ---: | ---: |
| `00489d90` | `directx_error_message_format` (format the report) | 62 | 18.87 | **100.00** |
| `00489dd0` | `directx_result_diagnostic_dispatch` (record and decode) | 5,257 | 18.33 | **100.00** |

Byte counts are Reccmp's. The dispatcher's include its six jump tables
(`0048b098`–`0048b25d`). The original names are not attested in the binary,
so the functions keep the port's names. The file stem `dxerror` is
descriptive only, and the unit's name in the spec stays empty.

## The unit

[`src/gods98/dxerror.c`](../../src/gods98/dxerror.c) and its C header
[`dxerror.h`](../../src/gods98/dxerror.h).

### State

- **`.bss`** (`005488a8`–`005490c0`), one `DxerrorGlobs` struct:

  | Address | Field | Use |
  | --- | --- | --- |
  | `005488a8` | `err` | the last HRESULT |
  | `005488ac` | `line` | the caller's `__LINE__` |
  | `005488b0` | `module` | the module, 0 when outside 1..7 |
  | `005488b4` | `count` | results reported |
  | `005488b8` | `file` | the caller's `__FILE__` |
  | `005488bc` | `reserved` | never referenced |
  | `005488c0` | `message[2048]` | the report |

  Only the `sprintf` call references the buffer, so its size is inferred.
  The next referenced `.bss` address is the CRT's `005490c8`, and 2,048
  bytes end at `005490c0`.
- **`.data`.** The module-name table at `004ab788` holds 8 pointers. Its
  literals follow it in reverse (`004ab7a8`–`004ab824`). The order differs
  from the port's: 4 is `"Direct Sound"`, 5 `"Direct Play"`, 6
  `"Direct3D Immediate mode"` and 7 `"IUknown Interface"` (sic).

### Literals in original order

The original compiles with string pooling (`/Gf`). Each literal is a COMDAT,
and the link places them in the object's order:
1. the table's literals, in reverse;
2. then, function by function, each one's literals in reverse order of
   their first use in its code.

A literal used more than once is stored once. `"Unknown"`, for example, is
used four times and stored at `004af9ac`.

The rebuilt object reproduces this layout. Laying out its `.data` COMDATs
from `004ab788` with their alignment puts **every one of the 338 literals at
its original address**, ending at `004af9e1`. The check reads the sections
of the object that `match_function.py --keep` leaves.

Every literal carries a Reccmp `STRING` marker on the line before it, as in
`utils.c`. Each use of a pooled literal carries the same marker.

### The dispatcher

The original's shape, recovered from the disassembly:

- **Record.** It stores `err`, then the module (0 when outside 1..7), then
  `file`, `line` and the count. Only that statement order gives the
  original's stores. The other five orders score 99.68–99.75%.
- **Outer switch on the raw module.**
  - 4, 5 and 6 (DirectSound, DirectPlay, Direct3D IM) each report
    `"Unknown"`/`"Error handling is unsupported in this module"`. There is
    no `break`, so module 4 reports it three times. Then all three fall
    through to the Retained Mode table.
  - 1, 2 and 3 each switch on the HRESULT: Retained Mode (21 results),
    DirectInput (33) and DirectDraw (107).
  - 7 reports `QUERY_INTERFACE`.
  - Anything else reports `"Unknown"`/`"An error in an unknown DirectX
    module/object was raised"`.
- **One exit.** Every case ends in `break`, and the function has a single
  `return err`. MSVC copies the epilogue into each case as `mov eax, esi`.
  - With `return err` in each case, MSVC returns the case's constant
    (`mov eax, 0x80004001`). The first draft did that and scored 84.39%,
    0x89 bytes longer than the original.
  - Each inner switch's `default:` reports `"UNKNOWN ERROR"`/`"Invalid error
    code specified"`. MSVC merges the three identical defaults into the one
    block at `0048b012`, as in the original.
- **Case order.** MSVC lays out the case bodies that its binary search
  reaches next to the tests itself, in an order fixed by the values.
  - Where bodies keep their source order, they are alphabetical: each
    jump-table run, and `D3DRM_OK` after the `D3DRMERR_` codes. This is the
    order the DirectX documentation lists them in.
  - The rebuilt tables are therefore alphabetical.
  - Writing them in the original's code order instead compiles to the same
    code and the same literal layout. The bytes fix the layout but not
    every case's position in the source.

The names and descriptions are the original's, including its typing errors
(`DDERR_NOTLOADE`, `DDERR_MOREDAT`, `DDERR_VIDEONOTACTIV`, "availabe"). All
161 result entries match the port's former table.

## The boundary

The spec is **confirmed as it stands**: `00489d90`–`0048b260`, one unit.
Its boundary confidence stays **low**, because the text neighbours own no
data that could separate them. The spec note records the evidence:

- **`.data`.** The unit's data starts at `004ab788`, the first 8-byte
  boundary after G98-023's last literal (`"stop cdaudio"`, `004ab774`).
  G98-024 to G98-026 have no `.data`. It ends at `004af9e1`, before:
  - an unreferenced dword (`004af9e4`);
  - Wad.c's (G98-030) first literal (`004af9e8`).

  In the rebuilt object, the unit's only non-COMDAT `.data` is the table,
  which the link places before the literals. A dword of this object would
  therefore not follow its last literal, so `004af9e4` belongs to a later
  object.
- **`.bss`.** `005488a8`–`005490c0` follows G98-024's `.bss` directly (it
  ends with `005488a4`, used at `00489400`). The CRT's `.bss` comes next
  (`005490c8`).
- **Text.**
  - The dispatcher's jump tables end at `0048b25d`, with `0x90` fill to
    `0048b260`.
  - G98-026 (`00489c30`, `00489d70`) are BMP palette helpers called from
    `00474050` and `0047d9d0`.
  - G98-028 (`0048b260`–) resets file-stream commons and is called from
    WinMain.

  Neither neighbour calls the decoder or touches its state. The decoder
  calls only `sprintf` and its own formatter.

## Callers

The original's callers are `ddiscene_draw_end` (`00477150`),
`direct3drm_device_initialize` (`004781d0`), `0047e300` and
`input_direct_initialize` (`0047ed90`). Only `ddiscene_draw_end` calls the
decoder in the port. The other three are port-shaped hook-table code, and
adding the call belongs to their own units' conversions.

The two declarations in `resource.hpp` stay in place inside
`namespace unported_ledger_stubs` and gain `extern "C"` (entropy rule 1).
The caller in `view.cpp` is therefore unchanged, and
`test_c_boundaries.cpp` checks them against `dxerror.h`.
`ddiscene_draw_end` stays at 23.75%.

### `_sprintf` annotation (comment only)

The formatter's one remaining difference was its call target. `crt.cpp`
annotated `0048d9e0` with the LIBC symbol `_vsprintf`, but the routine is
`_sprintf`: it passes the address of its own variadic arguments
(`lea eax, [esp+0x2c]`) to `_output`. The lookup comment now names
`_sprintf`.

That raised `_sprintf` itself from 97.67% to 100% and nine of its original
callers, each of which calls `_sprintf`:

| Address | Function | before | after |
| --- | --- | ---: | ---: |
| `00401530` | `advisor_position_config_load` | 28.67 | 29.33 |
| `0040a400` | `preload_numbered_bitmaps` | 12.33 | 19.18 |
| `00434710` | `panel_button_layout_config_load` | 33.07 | 33.46 |
| `004599c0` | `legacy_hunger_images_load` | 29.92 | 31.50 |
| `0045cdc0` | `priorities_load_images` | 91.84 | 92.86 |
| `0045f030` | `mission_task_init_target` | 12.70 | 13.49 |
| `00461070` | `reward_stats_text_format_bound` | 24.29 | 27.62 |
| `004653a0` | `transient_effect_textures_preload` | 22.06 | 23.53 |
| `004777a0` | `WinMain` | 28.49 | 30.81 |

The `vsprintf` callers are unaffected. The original's `_vsprintf`
(`0048e1e0`) still carries the port's `formatted_text_buffer_build` marker
(finding 509).

## Entropy

**Net effect, both scopes:**
- game-only: 11 functions rose and none fell;
- full: 12 rose and none fell.

No function was newly mapped or lost. `make reccmp-check` passes with no
function below the baseline, so the refresh needs no entropy
classification.

- `resource.cpp` lost the two functions and seven statics, and no remaining
  function in it changed score.
- `resource.hpp` changed only by `extern "C"` on two declarations.

## Native port

`dxerror.c` compiles natively as C++ (CMake `LANGUAGE CXX`) and passes
`gcc -std=gnu89 -Werror=declaration-after-statement -Wall -fsyntax-only`.

    cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure

This builds with no errors, and **all 31 tests pass**, including
`c_boundaries`. No test exercised the decoder, and the original exposes no
reader of its report.

Behaviour changes in the native port, all to the original's:

- **Module names.** 4 to 7 now name the original's modules (above).
- **Report.** The report is formatted with `sprintf` into 2,048 bytes and
  ends in a newline. The port bounded it at 1,023 bytes and substituted
  defaults for a NULL name, description, file or module name.
- **Strings.** The unknown-result strings are `"UNKNOWN ERROR"` and
  `"Invalid error code specified"`, where the port had `"UNKNOWN_ERROR"`
  and a trailing period. The QUERY_INTERFACE description loses the port's
  period, and the unsupported-module one reads `"…in this module"`, not
  `"…in this DirectX module."`.

`make reccmp-lint` reports the same 4,315 unannotated functions as `main`.

## Reproducing

    make reccmp-check                          # matching build + baseline gate
    make match-function SOURCE=src/gods98/dxerror.c
    python3 tools/translation_units.py dossier 489c30 48b330
    python3 tools/translation_units.py build
