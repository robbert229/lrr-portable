# Finding 513 — key-name table (G98-018, Keys.c) rebuilt in original shape

Issue #416. This conversion follows the
[ADR 0005](../adr/0005-original-shape-c-units.md) recipe, starting from the
`tools/convert_unit.py` scaffold. It rebuilds the gods98 key-name table:
original translation unit G98-018, text `00485a20`–`00485e80`, 2 functions
([finding 505](505-original-translation-units-and-link-order.md)).

"Before" is the *match baseline* on `main` at `041ab287` (#427). "After" is
this change. `make reccmp-check` and `make match-function` give the same
per-function scores.

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (byte-weighted, 1,094 bytes) | 46.94% | **100.00%** |
| Exact functions | 1 | **2** of 2 |
| Game-only mean (2,128 mapped) | 49.64% | 49.67% |
| Full mean (2,347 mapped) | 53.97% | 53.99% |

| Address | Function (gods98) | Bytes | before | after |
| --- | --- | ---: | ---: | ---: |
| `00485a20` | `keyboard_key_names_init` (Keys_Initialise) | 1,027 | 43.48 | **100.00** |
| `00485e30` | `named_option_index_find_case_insensitive` (Key_Find) | 67 | 100.00 | **100.00** |

The means are over the committed baselines of `main` and of this change.

## The boundary

The spec is **confirmed as it stands**: `00485a20`–`00485e80`, one unit. Its
boundary confidence stays **low**, as for G98-027 (finding 511): the unit has
no `.rdata` or `.bss` that could pin its text neighbours.

- **`.data`.** The unit's 101 literals run `004ab180`–`004ab5f7`:
  - they start at the next aligned address after Flic.c's (G98-016) last
    literal, `"Flic Not Enough Memory"` at `004ab168`, ending `004ab17e`;
  - they end before Lws.c's (G98-020) first literal at `004ab5f8`;
  - G98-017 and G98-019 own no `.data`.
- **The literals are in reverse order of first use.** Keys_Initialise names
  `KEY_ESCAPE` first, and it is the last literal (`004ab5ec`). `KEY_DELETE`
  is named last and is the first (`004ab180`). This is the pooling order of
  finding 511. The rebuilt executable holds the whole `0x478`-byte block,
  padding included, byte for byte.
- **No `.rdata` or `.bss`.** The table is a common symbol (`005765e0`, 256
  pointers), so it lies in the common region in no link order.
- **Text.** All three units' functions are 16-byte aligned with `0x90` fill,
  so padding does not separate them.
  - G98-017 (`004850f0`–) is the text-window ring buffer.
  - G98-019 (`00485e80`–) is the 2D surface drawing.

  Neither calls the two functions, reads the table or uses the literals. The
  two functions call only `_strcmpi`.

The name `Keys.c` rests on the `KEY_` prefix (`string-prefix`), not on a
`__FILE__` string. It sorts between the two neighbours' likely names, but
gods98 is not linked alphabetically (finding 505 rule 7), so that is not
evidence.

## The unit

[`src/gods98/keys.c`](../../src/gods98/keys.c) and its C header
[`keys.h`](../../src/gods98/keys.h). The functions keep the port's names;
the table gives the gods98 names.

### State

- **Common.** `const char* key_names[]` at `005765e0`: the name of each
  DirectInput scan code (`DIK_*`), or NULL. The walk ends at `005769e0`,
  which is the start of Files.c's directory-token table (also common, read by
  `004802d0` and `00480570`).
- **`.data`.** The 101 literals. Each carries a Reccmp `STRING` marker on
  the line before it, as in `utils.c`.

### Shape notes

- **Keys_Initialise** clears the table with `memset` (inline `rep stosd`,
  256 dwords) and then assigns each name to its scan-code slot in the
  original's order. The port's version looped over a `{code, name}` table,
  which is why it scored 43.48%.
- **Key_Find** walks the table with an `unsigned int` index:

      for (loop = 0; loop < KEY_NAMES_COUNT; loop++)
          if (key_names[loop] && _strcmpi(key_names[loop], name) == 0) {
              *out_index = (unsigned char)loop;
              return 1;
          }

  MSVC strength-reduces it to the original's pointer walk with an `ebx`
  counter (`inc ebx`, `mov byte ptr [eax], bl`).
- **The original types** are `char*` for the names. The rebuild uses
  `const char*` because the native port compiles the file as C++, which
  rejects a string literal assigned to `char*`. The code is the same.

### Deviation: one spare slot (scoring only)

`key_names` is declared with **257** slots, not 256. Nothing reads or
writes the spare slot: the clear and the walk both cover `KEY_NAMES_COUNT`
(256) slots, and the code bytes equal the original's.

The reason is Reccmp's operand naming. Key_Find's loop ends with
`cmp esi, 0x5769e0`, one past the table. Reccmp replaces an immediate with a
name only when the address falls inside an annotated variable. The size it
uses for the original's variable comes from the rebuild's type.

- With 256 slots, neither side's end address is inside `key_names`. The
  original's `005769e0` and the rebuild's end address compare as two
  unrelated numbers, and Key_Find scores 96.77% in the full report, though
  its bytes are identical. (`match-function` gave 100%, because it renders
  offsets without type sizes; finding 507.)
- With 257 slots, both sides render `key_names[256]` and the function scores
  100%.

The port previously solved this with a separate annotated `uint32_t` end
marker at `005769e0`. A winapi-only `#pragma data_seg(".data$keynames")`
forced it to follow the table, which also took the table out of the common
region. The spare slot keeps the table common and needs no pragma. A
correct annotation of Files.c's table at `005769e0` would not help: the
rebuild's link does not place it after `key_names`.

## Callers

- The original calls Keys_Initialise once, from GameConfigInit
  (`0041fb50`), which the port has not reconstructed. As before, the two
  interface config loaders call it themselves:
  - `interface_images_config_load` (`0041ada0`);
  - `interface_build_images_config_load` (`0041ab00`).

  Both also call Key_Find. Re-initialising is harmless.
- `frontend.hpp` already declares both functions inside its `extern "C"`
  block, so **no header changed**. The `uint8_t*` there is `unsigned char*`.
  `test_c_boundaries.cpp` includes `keys.h` and checks the pair.
- From `frontend.cpp` this change deletes:
  - the old definitions;
  - the `{code, name}` entry table and its type;
  - the old `static` table and its end marker, with the `data_seg` pragma
    around them.

**Caller scores.** Unchanged: `interface_images_config_load` stays at
14.20% and `interface_build_images_config_load` at 10.22%. They are
port-shaped and call the same two functions as before.

## Entropy

**Net effect, both scopes:** one function rose (`keyboard_key_names_init`)
and none fell. No function was newly mapped or lost.

`make reccmp-check` against `main`'s baseline (`041ab287`, after #427
restored the gate) reports exactly one change, the rise above, and no
function below the baseline. No drift, so there is nothing to classify
under rule 2. `frontend.cpp` lost two functions and three statics, and no
remaining function in it changed score.

`make match-progress` regenerated the baseline from this change's build. It
differs from `main`'s only in `keyboard_key_names_init`'s two rows.

## Whole-binary progress

`make match-progress` before (`main` at `041ab287`) and after:

| | before | after |
| --- | ---: | ---: |
| Accuracy, full | 54.05% | 54.07% |
| Accuracy, game-only | 49.71% | 49.74% |
| Exact-function code bytes | 39,856 (6.20%) | 40,883 (6.36%) |
| `.text` identical at original addresses | 18,168 B | 18,260 B |
| Function-order pair inversions, full | 873,206 | 871,052 |
| `.data` identical bytes | 14,072 | 13,896 |

The `.data` comparison aligns the section at its start. The old
`data_seg` device placed the 1,024-byte table (and its marker) in the
rebuild's `.data`. The table is now common, so `.data` is 1,024 bytes shorter
and everything after it moves. The 176-byte drop comes from that shift. No
literal or initialised value changed.

`tools/pr_match_progress.py check` against `main` reports the file as fresh.
The distrobox's Python has no `pefile`. `make match-progress` ran in the
box with `BINARY_PROGRESS_PYTHON` adding a directory that links only the
host's `pefile.py` and `ordlookup` (same Python 3.14) to `PYTHONPATH`.

## Native port

`keys.c` compiles natively as C++ (CMake `LANGUAGE CXX`) and passes
`gcc -std=gnu89 -Werror=declaration-after-statement -Wall -fsyntax-only`.

    cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure

This builds with no errors, and **all 32 tests pass**, including
`c_boundaries`. `test_frontend.cpp` gains `test_key_names`, which checks:
- the first and last named codes;
- a case-insensitive match;
- an extended code above `0x80`;
- an unknown name, which fails and leaves the out byte alone.

The behaviour is unchanged: the same 101 names at the same codes, and the
same `_strcmpi` lookup.

## Reproducing

    python3 tools/translation_units.py dossier 485a00 485f00
    make reccmp-check
    make match-function SOURCE=src/gods98/keys.c
    python3 tools/translation_units.py build
