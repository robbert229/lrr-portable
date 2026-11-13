# Finding 610 — RNC expander unit (LATE-001) rebuilt in original shape

Issue #661, under [ADR 0005](../adr/0005-original-shape-c-units.md). The 14
functions at `0049c7c0`–`0049d030` were port models in
`src/gods98/resource.cpp`, the legacy file that also holds the port's WAD
reader. This change rebuilds them as the C unit
[`src/gods98/rnc.c`](../../src/gods98/rnc.c), with the C boundary header
[`rnc.h`](../../src/gods98/rnc.h). The unit is the RNC (Rob Northen)
ProPack expander that Wad.c's `_Wad_FileOpen` (`0048be40`) calls for packed
entries. `resource.cpp`, left with no annotated function, becomes a port-tail
file.

"Before" is the *match baseline* at #666's head (`bfdea59a`, stack #668:
#664 → #665 → #666, over `main` `4862ceaa`). "After" is this change on that
base.

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (14 functions, weighted by span, 2,160 bytes) | 45.08% | **97.93%** |
| Exact functions in the unit | 1 | **13 of 14** |
| Exact-function code bytes (`binary-progress.json`) | 193,084 | **195,022** |
| `opt_ref`, full: removed / exact removed | 475 / 244 | 477 / 246 |

| Address | Corpus name | Function | before | after |
| --- | --- | --- | ---: | ---: |
| `0049c7c0` | ExpandWadPayload | `wad_expand_payload` | 58.27 | **100.00** |
| `0049c840` | WadPayloadDecodeDispatch | `wad_payload_decode_dispatch` | 25.71 | **100.00** |
| `0049c8e0` | DecodeWadPayloadType1 | `wad_payload_decode_type1` | 52.13 | **100.00** |
| `0049ca60` | DecodeWadPayloadType2 | `wad_payload_decode_type2` | 34.19 | **100.00** |
| `0049cc70` | WadBitstreamOutputInitialize | `wad_bitstream_output_initialize` | 26.09 | 44.00 |
| `0049ccc0` | WadReadBitsLsbFirst | `wad_read_bits_lsb_first` | 27.16 | **100.00** |
| `0049cd30` | WadReadBitsMsbFirst | `wad_read_bits_msb_first` | 50.00 | **100.00** |
| `0049cd90` | WadHuffmanTableRead | `wad_huffman_table_read` | 56.76 | **100.00** |
| `0049ce00` | WadHuffmanSymbolDecode | `wad_huffman_symbol_decode` | 34.34 | **100.00** |
| `0049ce70` | WadRunLengthDecode | `wad_run_length_decode` | 84.21 | **100.00** |
| `0049ceb0` | WadBackreferenceOffsetDecode | `wad_backreference_offset_decode` | 74.23 | **100.00** |
| `0049cf50` | WadHuffmanTableClear | `wad_huffman_table_clear` | 22.86 | **100.00** |
| `0049cf90` | WadHuffmanCanonicalCodesBuild | `wad_huffman_canonical_codes_build` | 36.36 | **100.00** |
| `0049d000` | WadReverseLowBits | `wad_reverse_low_bits` | 100.00 | **100.00** |

Every function keeps its size, and no function outside the unit changed
score. The function names are the port's; no name in the binary attests the
originals. The stem `rnc` comes from the `"RNC"` magic, as G98-030's `wad`
came from `"WWAD"` (finding 591).

## Unit extent

The map gave 12 of the 14 functions low confidence. The extent is
confirmed, and no function moves:

- **Call graph** (`docs/coverage/call-graph/edges.csv`). The 14 functions
  call only each other and three C library functions: `malloc` (`0048dbd0`),
  `free` (`0048db80`) and `strncmp` (`0048f270`). The only caller from
  outside is Wad.c's `_Wad_FileOpen` (`0048be40`), which calls `0049c7c0`.
  `0049d000` is called only by `0049cf90`.
- **`.bss`.** A scan of the image for dword operands in
  `00557c00`–`005580c0` finds only the unit's functions on `00557d68` and
  `00557d80`–`00558093`: the three 256-byte Huffman tables and the six
  scalars of the decoder state. The neighbours are other units':
  `00557d54`/`00557d58` belong to code at `00407402`–`004078ae`, and
  `005580a0`–`005580bc` to `0049d0a9`–`0049d952`, the display-mode dialog
  (LATE-002).
- **Strings.** `"RNC"` (`004b2d08`) is read only by `0049c840`. LATE-002's
  `"Please install DirectX…"` follows it at `004b2d0c`.
- **The end.** The last function is `0049d000` (48 bytes, padding to
  `0049d030`). `0049d030` is the first function of LATE-002, which shares
  no call, string or `.bss` with this unit. LATE-002 reaches `crt.cpp`'s
  display-dialog functions (`0049d5e0`–`0049d9d0`); this unit stops well
  before them.
- **Kind.** The unit's code follows the import thunks
  (`0049c7a8`–`0049c7bf`), so the original linked this object after the
  libraries. Its state is commons (`00557d68`–`00558093`).

`translation-units.spec.csv` records this evidence in LATE-001's note. The
boundary confidence stays as `tools/translation_units.py` computes it.

## Step 0 for a late unit

`tools/convert_unit.py` refused the unit, because only `game` and `gods98`
units were convertible. This change adds the `late` kind:

- `conversion_footprint.py`: `late` is convertible and converts into
  `src/gods98`; `convert_unit.py` puts it in the `gods98` library.
- `link_order.insert_unit` takes the CRT segment's start. A unit that starts
  at or after it goes among the CRT-range entries after the port tail, in
  address order (here after `crt/crt.cpp`). `link_order.check` accepts a
  `late` unit entry, and the segment check is unchanged.
- Tests: `tools/test_convert_unit.py` (wiring) and
  `tools/tests/test_link_order.py` (check).

`resource.cpp` no longer holds an annotated function, so its legacy entry
(`0049cd30`) became a `tail` entry (`link_order.retire_legacy`).

## The unit

### State

Nine commons, each annotated:

| Address | Name | Type |
| --- | --- | --- |
| `00557d68` | `rnc_bitCount` | `unsigned char`: bits left in the buffer |
| `00557d80` | `rnc_rawTable` | `RNC_HuffEntry[16]`: method 1's literal-count table |
| `00557e80` | `rnc_distTable` | `RNC_HuffEntry[16]`: its offset table |
| `00557f80` | `rnc_lenTable` | `RNC_HuffEntry[16]`: its length table |
| `00558080` | `rnc_input` | `unsigned char*` |
| `00558084` | `rnc_output` | `unsigned char*` |
| `00558088` | `rnc_byteBuffer` | `unsigned char`: method 2's bit byte |
| `0055808c` | `rnc_bitBuffer` | `unsigned int`: method 1's bit word |
| `00558090` | `rnc_outputEnd` | `unsigned char*` |

`RNC_HuffEntry` is 16 bytes: a dword at `+0` and a word at `+4` that are
cleared (to 0 and `0xffff`) and never read, the canonical code at `+8`, and
its bit length at `+0xc`. `"RNC"` has its `// STRING` marker at its use.

`make unit-data-offsets`, LATE-001 row:

| Unit | .data | .rdata | .bss | Commons |
| --- | --- | --- | --- | --- |
| LATE-001 | n/a (0, 1 unknown) fewer than two attributable items | n/a (0, 0 unknown) | n/a (0, 0 unknown) | the nine above (1, 256, 256, 256, 4, 4, 1, 4, 4 bytes) |

Unit summary: full 0, gaps-only 9, partial 23, n/a 195.

### Shape notes

- **Return types.** Both bit readers, the two match decoders and the symbol
  decoder return `unsigned short` (callers test `ax`). Methods 1 and 2 return
  `short` 0 (`xor ax, ax`, with whatever pointer is in `eax` above it; the
  old port modelled that pointer). The dispatch returns `short`: `-1`
  (`or ax, 0xffff`) for a missing magic, and an `unsigned short` result
  local for the switch, which gives `mov eax, 0xfffe` for an unknown method.
- **The big-endian length** has two source forms. `0049c7c0` uses the mask
  form, `((len & 0xff) << 24) + ((len & 0xff00) << 8) + ((len >> 8) & 0xff00)
  + (len >> 24)`. The dispatch and the initializer use a Horner form on a
  signed 32-bit word, `RNC_LENGTH`: `((l * 256 + ((l >> 8) & 0xff)) * 256 +
  ((l >> 16) & 0xff)) * 256 + ((unsigned)l >> 24)`. The third byte is read
  from the stack slot (`mov cl, [esp+0xa]`) in both. An unsigned word, or
  the shift form, moves the registers (82–98%).
- **Typed tables.** The Huffman functions take `RNC_HuffEntry*`. With a
  `void*` parameter copied into a typed local, MSVC biases the clear loop's
  pointer by `+8` (70.83%); with the typed parameter it does not. The symbol
  decoder still walks a local copy of the parameter (with the parameter
  itself, `ecx`/`edx` swap: effective match only).
- **Method 2** tests the output end only after the end marker: the original
  jumps back to the loop body after every other token. The source is an
  inner `for (;;)` that the end marker leaves with `break`. The match branch
  comes first, and the run branch has its own copy loop. With one shared
  copy loop the function is 84–85%.
- **Canonical codes** (`0049cf90`): the length starts at 1 before the code
  and the divisor are set (`do … while (bitDepth <= 16)`).
- **Method 1** adds the three-byte lookahead into the bit word with `+`, not
  `|`.

No naked asm, no filler declaration and no `Deviation (scoring only)` is in
the unit. `tools/msvc_permuter.py` found no candidate for `0049cc70` (its
rewrites do not apply to that function).

### The near miss: `0049cc70` (44.00%)

The initializer's source matches exactly by itself. In the unit it does
not: MSVC 5.0 keeps `dest` in `ecx` (the original: `esi`) and borrows `ebx`
for the length's bytes. What was measured, with `make match-function`:

- Removing any one of the four functions above it (or replacing method 2's
  body with a shorter one) makes it 100.00%. Replacing `0049c7c0`'s body
  with `return 0;`, renaming it, or removing a function below it (two
  tried) does not.
- It does not follow the symbol counter (finding 538): 1 to 64 extra
  declarations, 1 to 8 extra functions (with or without branches, calls or
  new pointer types), and 1 to 40 blank lines before it all leave it at
  44.00%.
- Other shapes, in the unit: the stores in either order, the length read
  before them, `long`/`unsigned long`/`int`, the mask, Horner and cast
  forms, `unsigned char*` parameters, the end from `rnc_output`, named
  source and destination locals, a separate size local, `static` with a
  local prototype, and eight orders of the commons. None goes above 44.00%.
- Equivalent exact forms of methods 1 and 2 (27 of method 2) and of
  `0049c7c0` leave it at 44.00%.

So the choice depends on the code generated for earlier functions, not on
anything this function's source controls. No C form matches in the unit,
but a C form does match on its own, so it is left in C (no naked asm) and
documented at the definition.

## Port and callers

The function names are unchanged, so no link-name header is needed. The
unit's functions have C linkage through `rnc.h`.

- **`resource.cpp`** (now a port-tail file): the 14 port models, their
  state and the `WadCodecTableEntry` type are deleted. Its two readers of
  packed entries call `wad_payload_decode_dispatch`, now the unit's. Like
  the original, the expander reads up to four bytes past a packed stream (a
  32-bit refill of the bit word and method 1's three-byte lookahead), so
  both readers allocate their stored-bytes buffer with four zeroed bytes of
  slack (`RNC_INPUT_SLACK`, `calloc`).
- **`resource.hpp`**, in place, with the declaration count unchanged:
  `wad_payload_decode_dispatch` gets `extern "C"` and returns `short`; the
  C++ declaration of `wad_huffman_table_clear(void*, uint8_t)`, which now has
  no definition, is kept and tagged `Deviation (scoring only)` so that the
  symbol numbering of the files that include the header does not move. No
  caller uses it.
- **`wad_payload.h`**: comment only. Wad.c keeps including it, so its
  symbol numbering does not move.
- **`test_c_boundaries.cpp`** includes `rnc.h` beside `resource.hpp` and
  `wad_payload.h`.
- **Native.** `rnc.c` is compiled as C++. On LP64 a `long` is 64 bits, so
  the decoder's words are `unsigned int`, which MSVC 5.0 compiles the same
  as `unsigned long`. The signed length is `RNC_LONG`: `long` under MSVC 5.0
  on x86, `unsigned int` elsewhere, so the native shifts wrap without
  overflow. Method 1's lookahead is shifted as `unsigned int` for the same
  reason (same code under MSVC).
- **Behaviour.** The old port read the bit word 16 bits at a time; the
  original reads 32 and keeps the upper half as lookahead, and the unit does
  too. The two can differ only on packed method-1 data, and no shipped entry
  is packed: all 5,631 entries of `LegoRR0.wad` and 214 of `LegoRR1.wad`
  have flags `1`. Native gameplay is unchanged. The unit has no NULL checks,
  as the original; the old port's three guards and their tests are gone,
  and no caller passes NULL.
- **Tests** (`test_resource.cpp`): the stored method, the verdicts, a
  method-2 stream (literals, a short match, the end marker), a method-1
  stream (a one-code literal table), the expander, and the table clear.

## Port-only tables (finding 602)

The scene-availability and name tables in `resource.cpp` (about 30 KB) are
not this unit's state. They model the rock-fall styles and scene
availability, and converted units read them through the frozen
`resource_for_rock_fall_styles.hpp` and `resource_for_vehicle_model.hpp`.
The RNC unit's original state (commons spanning 812 bytes) replaces none of them,
so they are unchanged. They go when their readers' contract changes.

`.bss`: the codec's state moves from `resource.cpp`'s statics to the unit's
commons, at the same sizes. The `/OPT:REF` relink's `.bss` goes from
1,668,544 to 1,668,608 bytes (+64, alignment of the new commons); no
port-only table changes.

## Stray annotations

- **`0048bd50`.** #618 (finding 591) already turned the `/* FUNCTION */`
  comment that finding 583 flagged into a plain comment that names Wad.c.
  Nothing was left to do.
- Seven more `/* FUNCTION: LEGORR … */` comments in `resource.cpp` named
  addresses that converted units now own. Reccmp never read them. They are
  now plain comments naming the owner: `004629d0` (reward.c), `00440bf0`
  (legoobject_unit.c), `004519b0` (terrain_map.c), `0040c3a0`
  (rock_fall_styles.c), `004800c0` and `004800a0` (files.c), and `0048be00`
  (wad.c). Eight comments for unconverted units (`0044e0a0`, `00463810`,
  `0044dde0`, `00451cd0`, `0044dec0`, `00462da0`, `00462e60`, `00462ed0`)
  are left for their conversions.

## Link order and the late members (finding 608)

`rnc.c.obj` is the last explicit object, after `crt.cpp` (where
`resource.cpp`'s legacy entry sat), so its code precedes LIBC: Reccmp
matches by annotation, so the placement does not affect scores. The late
LIBC members keep their order and their place inside LIBC, before the
thunks: `strnicmp` (`libc:strnicmp.obj`), `xtoa`, `strlwr`, `strupr`.
Finding 608's hypothesis (LATE-001 and LATE-002 linked as library members
whose references pull those members in later) does not apply to this unit's
references: `rnc.c` calls only `malloc`, `free` and `strncmp`. Linking it as a
library member is left to that work.

## `crt.cpp` (owned by #447)

Nothing here edits `src/crt/`. For #447: LATE-002 (`0049d030`–`0049dab0`)
starts in `surface.cpp` (`0049d030`) and continues in `crt.cpp`
(`0049d5e0`–`0049d9d0`); its `.bss` (`005580a0`–`005580bc`) and its strings
(`004b2d0c`–`004b2d90`) follow this unit's.

## `/OPT:REF` (finding 579)

| | before (#666) | after |
| --- | ---: | ---: |
| `opt_ref.annotated.full`: removed / exact removed | 475 / 244 | 477 / 246 |
| `opt_ref.annotated.game_only`: removed / exact removed | 449 / 224 | 451 / 226 |
| The unit's object: removed | (in `resource.cpp.obj`) | `rnc.c.obj`: 0 |

`make opt-ref-callers OPT_REF_ARGS="--object rnc.c.obj"` lists nothing: all
14 functions are kept, because Wad.c's `_Wad_FileOpen` and the port's
packed-entry readers are live. The two more exact functions removed are
not the unit's. Measured on `main` before the rebase, the same change went
the other way (477 / 246 to 475 / 243), so this is the duplicate-name layout
noise of finding 591; the two functions were not identified, which would
need a build of #666. The legacy category's removed functions fall because
`resource.cpp` is now counted as port tail.

## Entropy

`make reccmp-check` against the baseline at #666's head passes:

- 13 functions improved; no function fell, and no exact match was lost;
- the plain-immediate check, the link-order map and `winapi-link-check` are
  OK.

No score changed outside the unit, so there are no drops to label and no
instruction-count changes.

## Reccmp lint ratchet

1,218 → 1,080 entries (−138, no growth). `resource.cpp` is now a port-tail
file, so its unannotated definitions no longer need ledger entries.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox): OK.
- `make match-progress`: the three generated files.
- `make unit-data-offsets`: the LATE-001 row above.
- `python3 tools/translation_units.py build`: 0 conflicts.
  `tools/binary_progress_inputs.txt` is regenerated
  (`binary_progress_hash.py --write-manifest`).
- `make tools-test`: OK, without `--allow-growth`.
- `rnc.c` passes `gcc -std=gnu89 -Werror=declaration-after-statement
  -fsyntax-only` and `g++ -std=c++17 -Wall -Wextra -fsyntax-only`.
- Native: `cmake -S . -B build -DTEST_WADS_DIR=<shipped WADs> && cmake
  --build build && ctest --test-dir build`: 39/39, real-data tests
  included.

## Reproducing

    python3 tools/convert_unit.py LATE-001 --stem rnc
    make match-function SOURCE=src/gods98/rnc.c
    make reccmp-check
    make match-progress
    make unit-data-offsets
    make opt-ref-link opt-ref
    make opt-ref-callers OPT_REF_ARGS="--object rnc.c.obj"
