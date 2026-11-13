# Finding 591 — WAD archive unit (G98-030) rebuilt in original shape

Issue #618, under [ADR 0005](../adr/0005-original-shape-c-units.md). #496
relocated G98-030 verbatim into `src/gods98/wad.cpp`. Those functions were
port models of the WAD reader (`Resource` records over the port's `s_wads`),
and six of the unit's fifteen functions had no source at all. This change
rebuilds the unit as the C unit [`src/gods98/wad.c`](../../src/gods98/wad.c),
with the C boundary header [`wad.h`](../../src/gods98/wad.h). The port's
reader moves, unannotated, to the port tail
[`src/gods98/wad_port.cpp`](../../src/gods98/wad_port.cpp). The native port
keeps running it. The unit is text `0048b4a0`–`0048c030`: gods98's Wad.c.

- **Wad_Load** takes the first free of ten archive records, checks the
  `WWAD` magic and reads the entry count. It then reads two tables of
  NUL-terminated names: the archive-relative names, which lookups search,
  and the data-directory names. Last it reads a 16-byte record per entry
  (flags, stored length, uncompressed length, offset). Each name is echoed
  through File_ErrorFile (`"%s\n"`).
- **Wad_FileOpen** reads an entry into the first free of 100 handles. Given
  wad `-1`, it tries every active archive from newest to oldest. When the
  entry's flag bit 1 is set, it expands the data with the RNC expander
  (`0049c7c0`). Wad_hLength and Wad_hData give a handle's uncompressed
  length and data. Files.c's WAD streams read that data in memory.

"Before" is the *match baseline* on `main` at `984a1213`. "After" is this
change.

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (15 functions, weighted by function span, 2,976 bytes) | 25.42% | **99.80%** |
| Exact functions in the unit | 3 | **13 of 15** |
| Mapped functions in the unit | 9 | 15 |
| Files.c functions with Wad.c call operands (7) exact | 0 | **6** |
| Exact-function code bytes (`binary-progress.json`) | 148,281 | **151,806** |
| Accuracy, full / game-only | 77.67% / 75.46% | 77.89% / 75.70% |

| Address | gods98 name | Function (matching build) | before | after |
| --- | --- | --- | ---: | ---: |
| `0048b4a0` | GetFileName | `wad_get_file_name` | 31.88 | **100.00** |
| `0048b4e0` | Wad_Load | `wad_load` | 27.21 | **100.00** |
| `0048bce0` | GetWad | `wad_get` | unmapped | **100.00** |
| `0048bcf0` | Wad_GetFileLength | `wad_get_file_length` | 13.33 | **100.00** |
| `0048bd10` | Wad_GetFileCompressedLength | `wad_get_file_compressed_length` | unmapped | **100.00** |
| `0048bd30` | Wad_FindFreeFileHandle | `wad_find_free_file_handle` | unmapped | 90.00 |
| `0048bd50` | Wad_IsFileInWad | `wad_is_file_in_wad` | unmapped | **100.00** |
| `0048bda0` | Wad_GetFileIndex | `wad_get_file_index` | 43.33 | **100.00** |
| `0048be00` | GetFreeWad | `wad_get_free_wad` | unmapped | 95.65 |
| `0048be40` | _Wad_FileOpen | `wad_file_open_in_wad` | unmapped | **100.00** |
| `0048bf70` | Wad_FileOpen | `wad_file_open` | 22.22 | **100.00** |
| `0048bfc0` | Wad_FileClose | `wad_file_close` | 100.00 | **100.00** |
| `0048bff0` | Wad_FileGetPointer | `wad_file_get_pointer` | 100.00 | **100.00** |
| `0048c010` | Wad_hLength | `wad_file_length` | 60.00 | **100.00** |
| `0048c030` | Wad_hData | `wad_file_data` | 100.00 | **100.00** |

The gods98 names are descriptive. The binary attests only `"WWAD"`, and the
stem comes from that magic. The weights are the functions' spans up to the
next function, padding included. The "before" figure counts the six unmapped
functions as 0.

The two near misses are end-of-array bounds, which depend on the commons'
layout (see [State](#state)). Neither can be reached by changing source.

## Unit extent

The map gave 14 of the 15 functions low confidence, and six had no current
source. The extent is confirmed, and no function moves:

- **Call graph.** The fifteen functions call only each other and the C
  library. Their only callers outside the unit are seven Files.c functions
  (`0047f640`, `0047f850`, `0047f980`, `0047fb60`, `0047fc20`, `0047fe30`,
  `0047fea0`), plus `0047ff50` for Wad_FileClose (`docs/coverage/call-graph/edges.csv`).
  `0048b4a0` is called only by `0048b4e0`. `0048c040` belongs to the next
  unit: its only caller is LWO's `0048c820`, and it calls `__strnicmp`.
- **`.bss`.** A scan of `.text` for operands in `00575400`–`00575e00` finds
  only the unit's functions on `00575500`–`0057566c` and
  `005756a0`–`00575ce4`. `00575ce0`–`00575cf0` belong to G98-028's
  `0048b263`–`0048b31b` (`s_startup_streams`), and `005754e0` to code at
  `0049d06c`–`0049d8d5`. Nothing in the image refers to `00575668`–`0057569f`.
- **Strings.** `"%s\n"` (`004af9e8`) and `"WWAD"` (`004af9ec`) are
  referenced only from `0048b4e0`. They follow G98-029, which has no data,
  and come before LWO's `"SPHERICAL"` (`004af9f4`). The `.data` word at
  `004af9e4` (value `0x10`) has no code reference, so it is left
  unattributed and unannotated, and no storage is added for it.
- **Registry (G98-029)** ends at `0048b4a0`. Its three functions call only
  each other and the registry API.

`translation-units.spec.csv` records this evidence in G98-030's note. The
boundary confidence is computed by `tools/translation_units.py` and stays
`low`, because padding cannot separate C objects (finding 505). The unit's
functions and ranges are unchanged.

## The unit

### State

- **Commons**: `Wad wads[10]` (`00575500`, 0x24 bytes each) and
  `WadFile wadFiles[100]` (`005756a0`, 16 bytes each: data, active, wad
  number, entry number). Both are file-scope definitions, so they are
  communal, as the original's are, and both are annotated. Words `+0x00`,
  `+0x08` and `+0x0c` of a `Wad` are never read; they are kept for the
  layout.
- **Strings**: `"%s\n"` and `"WWAD"` each have a `// STRING` marker. `"rb"`
  (`004a2770`) is pooled with GAME-017 and is already marked in Files.c
  (finding 583).

`make unit-data-offsets`, G98-030 row:

| Unit | .data | .rdata | .bss | Commons |
| --- | --- | --- | --- | --- |
| G98-030 | gaps-only (2, 0 unknown) | n/a (0, 0 unknown) fewer than two attributable items | n/a (0, 0 unknown) fewer than two attributable items | `00575500` `_wads` (360), `005756a0` `_wadFiles` (1,600) |

Unit summary: full 0, gaps-only 4, partial 27, n/a 194.

**Commons layout and the two near misses.** The matching build places
`wadFiles` (`008e69c0`) directly before `wads` (`008e7000`). Swapping the
definitions in the source does not change that, so common order is not the
source order (finding 580, S6b).

- Wad_FindFreeFileHandle ends its walk at `&wadFiles[100].active`. The
  original names that address `s_startup_streams[1]` (G98-028, the next
  `.bss`). The rebuild names it `wads[0].active`: 90.00%.
- GetFreeWad ends at `&wads[10].active`, `0x57566c` in the original, which
  falls in the unreferenced gap after the table and so stays a plain
  constant. The rebuild's `0x8e716c` is also plain, but its value differs:
  95.65%.

Both are layout identity, not source shape.

### Port names

Native code calls the port's WAD reader by its own names:

- `resource.cpp` calls `wwad_archive_index_load`, `resource_wad_lookup`,
  `ares_open_wad_path` and `fixed_record_slot_resource_get_target`.
- `test_resource.cpp` calls `fixed_record_slot_resource_release` and
  `wad_memory_stream_buffer_get`.
- `files_port.cpp` calls `wad_entry_find_across_mounted_archives`.

None of these has the original's contract. Archives are `ResourceWad`
records in `resource.cpp` with a different layout, and entries are
`Resource` handles read lazily. The stream slots are the port's sixteen
`s_fixed_slots`. So no port name is given to a unit function, and every
port function keeps its name in every build. They move, unannotated, to
the port tail:

- `wad_port.cpp` (`git mv` of `wad.cpp`; a `tail` link-order entry and a
  Meson source) keeps `wwad_archive_index_load`, `resource_wad_lookup`,
  `ares_open_wad_path`, `fixed_record_slot_resource_release`,
  `fixed_record_slot_resource_get_target`, `wad_memory_stream_buffer_get`
  and the static `ares_open_wad`.
- Deleted, because nothing calls them: the ledger stub
  `unported_ledger_stubs::buffered_char_stream_read_to_string`, plus
  `wad_entry_length_get` and `wad_stream_length_get`. Their only callers
  were each other. `port_loop_ports.hpp`'s global declaration of
  `buffered_char_stream_read_to_string` was never defined, because the
  definition sat in a namespace. It is kept, tagged
  `Deviation (scoring only)`, so the symbol numbering of the 67 files that
  include the header does not move.
- `resource.cpp`: `s_fixed_slots` loses its `// GLOBAL: LEGORR 0x005756a0`,
  which the unit's `wadFiles` now holds. The storage stays for the port.
  The four `/* FUNCTION: LEGORR … */` comments, which Reccmp never read
  (`0048bce0`, `0048bd10`, `0048bd30`, `0048bd50`), become plain comments
  that name the unit's function. This resolves the `0048bd50` annotation
  that finding 583 flagged.

`wad_expand_payload` (`0049c7c0`) gains `extern "C"` in place in
`resource.hpp`. The new C boundary header
[`wad_payload.h`](../../src/gods98/wad_payload.h), which only `wad.c`
includes, declares it, and `test_c_boundaries.cpp` checks the pair.

### Files.c coupling (finding 583)

`wad.h` keeps #598's six entry points, in the same order and with the same
count, so `files.c`'s symbol numbering does not move. Two names change to
the unit's:

| Original | Address | Before (#598) | After |
| --- | --- | --- | --- |
| Wad_Load | `0048b4e0` | adapter `wad_load` → `wwad_archive_index_load` | `wad_load`, the unit's |
| Wad_IsFileInWad | `0048bd50` | port `wad_entry_find_across_mounted_archives` (unannotated) | `wad_is_file_in_wad` |
| Wad_FileOpen | `0048bf70` | adapter `wad_file_open` → `ares_open_wad_path` | `wad_file_open`, the unit's |
| Wad_FileClose | `0048bfc0` | port `fixed_record_slot_resource_release` | `wad_file_close` |
| Wad_hLength | `0048c010` | adapter `wad_file_length` | `wad_file_length`, the unit's |
| Wad_hData | `0048c030` | adapter `wad_file_data` | `wad_file_data`, the unit's |

Caller switches in `files.c`, three calls in all:

- `0047fb60` and `0047fe30` call `wad_is_file_in_wad`, not
  `wad_entry_find_across_mounted_archives`.
- `0047ff50` calls `wad_file_close`, not
  `fixed_record_slot_resource_release`.

`files_port.cpp` loses the four adapters and their length table, and no
longer includes `wad.h`.

What happens to the seven Files.c functions whose call operands depended on
this conversion:

| Address | Function | before | after |
| --- | --- | ---: | ---: |
| `0047f640` | File_LoadWad | 87.50 | **100.00** |
| `0047f850` | File_Seek | 95.69 | **100.00** |
| `0047f980` | File_Read | 95.18 | 97.59 |
| `0047fb60` | File_Exists | 98.48 | **100.00** |
| `0047fc20` | File_GetC | 96.15 | **100.00** |
| `0047fe30` | _File_CheckSystem | 97.96 | **100.00** |
| `0047fea0` | _File_OpenWad | 94.44 | **100.00** |

File_Read keeps the two differences finding 583 lists that have nothing to
do with Wad.c: `""` is `config.c`'s static `004b8a14`, and `lea edi,
[edx+ecx]` has its operands in the other order.

### Runtime

- **Native**: unchanged. The native port reads its data through
  `resource.cpp` and `wad_port.cpp`. `wad.c` is compiled natively as C++,
  but only `files.c` calls it, and nothing native calls `files.c`.
- **Matching build**: files.h gives `files.c` the port's file-layer names
  (finding 583), so the port's file opens (`game_file_open_dispatch`, which
  is File_Open there) now read Wad.c's tables, as the original does.
  `resource_open` (22 call sites, `read_whole_file` among them) still
  reads the port's `s_wads`. The two tables are filled at different points:
  - the retail startup mounts through File_Initialise into Wad.c's tables;
  - an explicit `WAD_DIR` mounts through `resource_mount` into the port's.

  Without a fix, `frameloop_application_init` would count 0 mounted
  archives after the retail startup and quit, and `make reccmp-run` would
  break. `wad_port.cpp` therefore defines two port-tail functions, which
  `frameloop.cpp` calls after each mount path. Both are compiled only
  under MSVC 5.0 (`_MSC_VER < 1200`):
  - `wad_port_mount_startup_archives` mounts `<prefix>0.wad`–`<prefix>9.wad`
    into the port's tables;
  - `wad_port_mount_unit_archives` mounts the port's archives into Wad.c's.

  Each call uses the names the #598 adapters used. Native builds and the
  MinGW reference build do not compile either function and keep one table.
  The second table goes away when the `resource_open` callers' own units
  are converted to File_*.

## Shape notes

- **`0048b4e0`**: `i` is initialised at its declaration, and the first
  name loop has no initialiser (`xor ebp, ebp` before GetFreeWad). The four
  failure paths are written out as the original has them:
  - After the first table, it frees the names and the second table's array.
  - After the second table, it tests the second table's pointer before
    freeing the *first* table. That is a bug in the original, and it is
    kept.
  - After the entry-table allocation and after a short read, the same
    tests and frees as the second table's path, plus the entry table on a
    short read.

  The success return follows the short-read test, so the cleanup falls
  through.
- **`0048be40`**: `dest` is declared in the decompression block, so MSVC
  gives it the dead `wadNo` argument slot, as the original does (`push ecx`
  frame, one local). With `dest` declared at function scope, the frame
  grows to `sub esp, 8` (82.87%).
- **`0048bd50`/`0048bf70`**: `if (wadNo == -1) { loop } return f(...)`
  gives the original's fall-through order.

No naked asm, no permuter run was needed, and no `Deviation (scoring only)`
is in the unit.

## Entropy

`make reccmp-check` against the baseline at `984a1213` passes:

- 13 functions improved and 6 newly mapped;
- no function fell, and no exact match was lost;
- the link-order map is OK, and `winapi-link-check` is OK.

No score changed outside Wad.c and Files.c, so there are no drops to label
and no instruction-count changes. Header edits:

- in place: `extern "C"` (`resource.hpp`) and comments (`resource.cpp`,
  `port_loop_ports.hpp`);
- `frameloop.cpp`: two declarations and two calls, matching build only,
  placed after the file's last annotated function;
- `wad.h`: two names renamed, the count unchanged;
- new header: `wad_payload.h`, which only `wad.c` includes.

No file owned by #615, #616 or #617 is edited.

## `/OPT:REF` (finding 579)

| | before (`main`) | after |
| --- | ---: | ---: |
| `opt_ref.annotated.full`: removed / exact removed | 585 / 275 | **582 / 278** |
| `opt_ref.annotated.game_only`: removed / exact removed | 568 / 264 | 565 / 267 |
| The unit's object: removed | `wad.cpp.obj` | `wad.c.obj`: 0 |

`make opt-ref-callers OPT_REF_ARGS="--object wad.c.obj"` lists nothing:
all fifteen functions are kept, because `files.c`'s File_Initialise and
File_Open are live in the matching build. `files.c.obj` still has 13
removed (13 exact). These are the same class (c) and removed-root cases
that finding 583 lists.

**Measurement noise.** Some names are defined in two objects. For example,
`legoobject_stats_flag10_test` is in both `legoobject_unit.c.obj` and
`legoobject.cpp.obj`, and `file_get_system` is in both `files.c.obj` and
`container_unit.cpp.obj`. The original-mode relink keeps one copy of each,
and which copy it keeps depends on layout. The same tree without the
matching-build mirror above measures 576 / 272. Diffing the two lists of
removed functions shows only these duplicate-name swaps, which are
unrelated to Wad.c:

- removed only with the mirror: `00432200`, `00438ed0`, `00439cd0`,
  `00439cf0`, `00439d10`, `00439d30`, `0043c440`, `0045cc60`, `0045daf0`,
  `0047fe20`;
- removed only without it: `00472a40`, `00473680`, `004798a0`,
  `0047ff40`.

## Reccmp lint ratchet

1,314 → 1,313 entries (−1, no growth): `ares_open_wad`, a static in the old
unit file, is now in a tail file.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox): OK; link-order map
  OK; `winapi-link-check` OK; the plain-immediate check is OK.
- `make match-progress`: the three generated files.
- `make unit-data-offsets`: the G98-030 row above.
- `python3 tools/translation_units.py build`: 0 conflicts.
  `tools/binary_progress_inputs.txt` is regenerated
  (`binary_progress_hash.py --write-manifest`).
- `make tools-test`: lint (ledger shrinks), link order, winapi list, Meson
  parity and the tool tests are OK.
- `wad.c` passes `gcc -std=gnu89 -Werror=declaration-after-statement
  -fsyntax-only`.
- Native: `cmake -S . -B build -DTEST_WADS_DIR=<shipped WADs> && cmake
  --build build && ctest --test-dir build`: 39/39, real-data tests
  included.

## Reproducing

    python3 tools/convert_unit.py G98-030 --stem wad   # in a copy without the .cpp entry and wad.h
    make match-function SOURCE=src/gods98/wad.c
    make reccmp-check
    make match-progress
    make unit-data-offsets
    make opt-ref-link opt-ref
    make opt-ref-callers OPT_REF_ARGS="--object wad.c.obj"
