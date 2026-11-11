# Finding 509 — String and token helpers (G98-004) rebuilt in original shape

Issue #396. This unit was converted under the [ADR 0005](../adr/0005-original-shape-c-units.md)
recipe after the [config tree](508-config-unit-original-shape.md). It
rebuilds the gods98 string and token helpers in the original's shape:
original translation unit G98-004, text `00477440`–`004777a0`, 7 functions
([finding 505](505-original-translation-units-and-link-order.md)). Most of
the config loader and the game's parsing code sit on these helpers. All
scores are from the *matching build*. "Before" is the match baseline that
#398 accepted on `main` at `c7e7af5b`; "after" is this change.

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (byte-weighted, 839 bytes) | 33.09% | **99.03%** |
| Exact functions | 0 | **5** of 7 |
| Mapped functions | 5 | 7 |
| Game-only mean (all mapped) | 48.95% (2,119) | 49.10% (2,121) |
| Full mean (all mapped) | 53.33% (2,338) | 53.49% (2,340) |

Every function recompiles to exactly the original's length. The two that
are not exact differ only in how the comparison names one operand:

- `formatted_text_escape_decode_and_emit` (`00477590`) calls `vsprintf`, as
  the original does. The original's target, LIBC's `_vsprintf` at
  `0048e1e0`, carries the port's annotation `formatted_text_buffer_build`
  (finding 505 lists this marker as wrong). The call's target name therefore
  differs, and nothing else does.
- `game_string_hash_compute` (`00477670`) multiplies by the immediate
  `0x600937`. That value falls inside the recompiled image's address span,
  so Reccmp shows the recompiled operand as an offset and the original's as
  a number (the effect finding 507 describes for `0x800000`). The
  instructions are identical.

## The unit

[`src/gods98/utils.c`](../../src/gods98/utils.c) and its C header
[`utils.h`](../../src/gods98/utils.h) hold the 7 functions in original
order. The unit has no `.rdata`, `.data` variables or `.bss`. Its literals
are `"FALSE"`, `"NO"`, `"TRUE"` and `"YES"` (`004aabd8`–`004aabef`), and it
shares `"ON"`/`"OFF"` (`004a4124`/`004a4120`) with an earlier object. They
are annotated with Reccmp `STRING` markers in the function. The original
names are not attested in the binary, so the functions keep the port's
names, as `config.c` did. The file stem `utils` is descriptive only: the
unit's name in the spec stays empty.

| Address | Function | Bytes | before | after |
| --- | --- | ---: | ---: | ---: |
| `00477440` | `sound_variant_split` (split on a separator string) | 101 | 29.31 | **100.00** |
| `004774b0` | `whitespace_tokenize` (split on whitespace runs) | 155 | unmapped | **100.00** |
| `00477550` | `string_duplicate` (heap copy) | 64 | 83.58 | **100.00** |
| `00477590` | `formatted_text_escape_decode_and_emit` | 120 | 25.74 | 97.62 |
| `00477610` | `case_insensitive_substring_find` | 96 | 79.67 | **100.00** |
| `00477670` | `game_string_hash_compute` | 160 | unmapped | 96.72 |
| `00477710` | `block_get_bool` (YES/TRUE/ON, NO/FALSE/OFF) | 143 | 60.95 | **100.00** |

Before this change:
- `whitespace_tokenize` and `game_string_hash_compute` sat in
  `game/util.cpp` under `/* FUNCTION: */` block comments, which Reccmp does
  not read, so they were unmapped.
- The other five were spread over `sound.cpp`, `string.cpp`,
  `frontend.cpp` and `block.cpp`.

What the original does, now reproduced as written:

- **Split (`00477440`).** It tests every byte against the separator,
  the last byte included. A trailing separator therefore ends the string
  with an empty part: `"1:2:3:"` gives four parts. The port's version
  skipped the last byte and returned three parts, the last being `"3:"`.
- **Whitespace split (`004774b0`).** It keeps the first part even when
  the string starts with whitespace. That part is then empty, and a
  trailing run gives an empty last part.
- **Escape-decoding copy (`00477590`).** It formats into 256 bytes with
  `vsprintf`, without a bound. It decodes `\n` to a newline and `_` to a
  space into a second 256-byte buffer, and returns `string_duplicate` of
  the result.
- **Substring search (`00477610`).** An empty haystack or needle matches
  nothing. Each position gets a case-insensitive `_strnicmp` of the
  needle's length.
- **Hash (`00477670`).** The hash is `sum += c * step * m`, where `m`
  starts at `0x600937` and is multiplied by `0x600937` each step. With
  whitespace skipping on, a string that ends in whitespace reads past its
  terminator. The game calls it with skipping off (the SFX name table at
  `00464740`).
- **Boolean (`00477710`).** The tests run in the order YES, TRUE, ON, NO,
  FALSE, OFF. The result is 1, 0, or 2 for anything else.

**Shape notes.** The split, the whitespace split and the substring search
each match exactly only when the walking pointer is a local copy of the
argument (`char* s = text;`). Walking the argument itself compiles to the
same instructions in a different order. The escape-decoding copy matches
with the output pointer set and advanced before the input pointer.

`utils.c` uses the VC5 LIBC names `malloc`, `_strcmpi` and `_strnicmp`
through [`vc5names.h`](../../src/crt/vc5names.h), and `isspace`/`toupper`
from `<ctype.h>`.

### Callee annotations

The whitespace split and the hash use `isspace`, which in VC5 reads
LIBC's `__mb_cur_max` and `_pctype`. `crt.cpp` had annotated the original
addresses of those two (`004aff74`, `004afd68`) on port-only stand-ins
that exist only in non-MSVC builds. In the matching build, the original's
operands were therefore named after the stand-ins and the recompiled ones
after LIBC. The annotations now sit in the MSVC branch and name the LIBC
symbols by lookup comment (`// __mb_cur_max`, `// _pctype`).

This is a comment-only change, with no effect on code generation. It
raised 13 LIBC functions that read the same two variables:
- `_isspace`: 89.47 → 100.00
- `__isctype`: 96.88 → 100.00
- `_atol`: 94.44 → 100.00
- `_atof`, `_toupper`, `_tolower`, `__forcdecpt`, `__output`, `__input`,
  `___strgtold12`, `_wctomb`, `_mbtowc` and `_wcstombs`: 0.1 to 5 points
  each.

The `0048e1e0` marker is left as it is. Renaming it to LIBC's `_vsprintf`
would change the call-target name in the port's seven
`formatted_text_buffer_build` callers. Those callers keep their source, so
each would fall without being entropy drift. The marker belongs with the
CRT units.

## The boundary

The spec is **confirmed as it stands**: `00477440`–`004777a0`, one unit.
Its boundary confidence stays **low**, because no data separates it from
either neighbour. The spec note now records the evidence:

- **Literals cannot separate it.** The unit's literals fill
  `004aabd8`–`004aabef` exactly. They start at the byte where Viewports.c's
  `__FILE__` string (`004aaba4`, 52 bytes) ends, and they end at Main.c's
  first literal (`004aabf0`). So the code sits between those two objects
  in link order. However, MSVC 5.0 emits a function's literals when it
  finishes the function, in reverse order of use, and moves forward from
  function to function. Two examples:
  - `block_get_bool` uses YES, TRUE, NO, FALSE and stores them as FALSE,
    NO, TRUE, YES.
  - Main.c's option parser stores `-cleansaves` lowest and uses it last.
    WinMain's literals sit below the option parser's.

  A tail of Viewports.c, a head of Main.c and a separate object would
  therefore all place these literals at the same addresses.
- **No other data.** The unit has no `.rdata`, `.data` variables or
  `.bss`. Its only other data references are LIBC's `__mb_cur_max` and
  `_pctype`. One float, `1.0f` at `0049e8bc`, sits between Viewports.c's
  last referenced float (`0049e8b8`, read at `0047719b`) and Main.c's
  header table (`0049e8c0`). Nothing in `.text` references it, so it
  cannot be assigned.
- **Padding.** All three objects use 16-byte alignment with `0x90` fill,
  as finding 505 rule 5 expects of C, so padding gives no boundary.
- **Behaviour (the deciding evidence).** The 7 functions are stateless
  leaves. They call only the CRT (`strncmp`, `malloc`, `vsprintf`,
  `_strnicmp`, `_isctype`, `toupper`, `_strcmpi`) and each other
  (`00477590` calls `00477550`).
  - **Viewports.c side.** Viewports.c's functions work on the viewport
    commons (`0076ace0`–) or call through the viewport interface. None of
    them calls these helpers, and these helpers call none of them.
  - **Main.c side.** Main.c's functions use Main.c's `.bss`
    (`00505800`–). Their only call into the helpers is the option parser
    (`00477bf0`) calling the substring search, one of its four callers.
  - **Elsewhere.** The helpers are called from functions across the game
    and the engine (57 call the split alone). They behave like a utility
    module, not part of a viewport or start-up file.

## Callers

The callee contract changed in four ways, all to the original's:

1. **No NULL guards.** The port's versions accepted NULL and returned NULL
   (or 2 for the boolean), and the escape-decoding copy bounded its output.
   The original's do not.

   Every native call site was checked. They all pass strings that cannot
   be NULL, with one exception: `entity_tool_name_strings_load`
   (`00439950`) passed `block_get_strptr`'s result, NULL for a missing
   `ToolNames` entry, to `string_duplicate`. It now calls
   `block_get_str_copy` (`Config_GetStringValue`, `00479050`), as the
   original does. That function returns NULL for a missing entry.
2. **The split tests the last byte** (see above). `config_rgb_vector_read`
   now rejects `"1:2:3:"` as four parts, as the original does.
3. **The escape-decoding copy formats with `vsprintf` into 256 bytes.**
   Its three port callers (in `mission.cpp`, `entity_config.cpp` and
   `level.cpp`) pass config values.
4. **The C++ declarations are unchanged.** They were already inside
   `extern "C"` blocks in `sound.hpp`, `string.hpp`, `block.hpp`,
   `frontend.hpp` and `util.hpp`. `test_c_boundaries.cpp` now checks each
   against `utils.h`.

The C boundary header `block.h` declared only `block_get_bool`, so it was
removed. `sound.h` loses `sound_variant_split`. Both are now in `utils.h`.

**Caller scores.** Callers call the helpers by the same names as before, so
no caller's code changed apart from the one whose source changed:

| Address | Function | before | after |
| --- | --- | ---: | ---: |
| `00439950` | `entity_tool_name_strings_load` | 37.97 | 42.11 |

The two converted C units are unchanged:
- `config.c` swaps `block.h` and `sound.h` for `utils.h`, and all 16 of
  its functions keep their scores.
- `priorities.c` swaps `block.h` for `utils.h`, and all 14 keep their
  scores.

`priorities.c` still includes `gods98/string.hpp` even though its only use
of it, `string_duplicate`, is now in `utils.h`. Dropping that include
changes the x87 code of `priorities_move_cursor_to_up_button` (`0045d550`,
73.10 → 72.11), so it stays under entropy rule 1.

## Entropy

The headers changed as follows:
- `sound.h` loses one declaration, and `block.h` is removed. Only C units
  and `test_c_boundaries` include them.
- The C++ headers change in comments only.

The `.cpp` files that lost definitions (`sound.cpp`, `string.cpp`,
`frontend.cpp`, `util.cpp`, `block.cpp`) changed no remaining function's
score.

The first full run had one drop with unchanged source:
`ledger_stub_00444fb0_EntityTurnToVectorActivityIssue` fell from 26.04% to
25.56%. Its listing lost one `fxch st(1)`, and its branch offsets moved by
two bytes. The cause was the edit to `entity_tool_name_strings_load`
earlier in `entity.cpp`, which removed a local and shifted the symbol
numbering. Writing that edit with a local (`char* value =
block_get_str_copy(…)`) restores the numbering. The function is back to
26.04%, and its caller still gains.

**Net effect, both scopes, in the refreshed baseline:**
- game-only: 6 functions rose and none fell;
- full: 19 rose and none fell;
- 2 are newly mapped (`004774b0`, `00477670`).

`make reccmp-check` passes with no function below the baseline, so the
refresh needs no entropy classification.

## Native port

`utils.c` compiles natively as C++ (CMake `LANGUAGE CXX`). It passes
`gcc -std=gnu89 -Werror=declaration-after-statement -fsyntax-only`, as do
`config.c` and `priorities.c`.

    cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure

This builds with no errors, and **all 31 tests pass**, including
`c_boundaries`, `block`, `string` and `crt`. The tests changed in three
places:

- `test_block`'s `G::tail` case (`"1:2:3:"`) now expects the original's
  four parts, so `config_rgb_vector_read` returns 0 and leaves the outputs.
  It had pinned the old split's skipped last byte.
- `test_string` drops the NULL checks of `string_duplicate` and
  `case_insensitive_substring_find`.
- `test_crt` drops `shared_texture_directory_set(NULL)`. The original
  (`004728c0`) passes its argument straight to `string_duplicate`.

With `TEST_WADS_DIR` set to the shipped data, 29 of 31 pass, including
`block --real` (the 12,876-record digest), `entity_ae`, `texture` and
`sound`. `model` and `flic` fail with the same messages on a clean `main`
checkout built the same way, and this change does not touch them.

## Reproducing

    make reccmp-check                       # matching build + baseline gate
    make match-function SOURCE=src/gods98/utils.c
    python3 tools/translation_units.py dossier 476d50 478000
    python3 tools/translation_units.py build
