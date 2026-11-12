# Finding 583 — File layer unit (G98-014) rebuilt in original shape

Issue #593, under [ADR 0005](../adr/0005-original-shape-c-units.md). #496
relocated G98-014 verbatim into `src/gods98/files.cpp`. This change rebuilds
it as the C unit [`src/gods98/files.c`](../../src/gods98/files.c), with the C
boundary header [`files.h`](../../src/gods98/files.h). The port's file layer
moves, unannotated, to the port tail
[`src/gods98/files_port.cpp`](../../src/gods98/files_port.cpp), which the
native port keeps running. The unit is text `0047f130`–`004805b0`: gods98's
Files.c.

- **File_Open** resolves a name under the data directory
  (File_VerifyFilename, `_fullpath`). It reads the name from a mounted WAD
  when one holds it. Otherwise, and for every write, it uses a C library
  stream, with the CD's `Data` directory as the read fallback.
- **File_Seek/Read/Tell/GetC/GetS** dispatch on the `File` record's type:
  `0` is a WAD entry (`WADFILE`: Wad.c handle and read position), `1` a
  `FILE*`.
- **File_Initialise** mounts `<program>0.wad` to `<program>9.wad`, sets the
  data and base search paths, finds the CD by its `cd.key` and checks the
  install, with the registry's `CDMissing`/`DataMissing` messages.
- **File_CheckRedundantFiles** reads a list of names and walks the data
  directory against it.

"Before" is the *match baseline* at `81a2877b`, the head of #595 (#448's
`.data` padding retirement, finding 580, on top of #597); "after" is this
change, stacked on it. The unit's own scores are the same on `main` at
`3791f0dd`.

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (36 functions, weighted by function span, 5,248 bytes) | 34.86% | **99.10%** |
| Exact functions in the unit | 5 | **27 of 36** |
| Mapped functions in the unit | 34 | 36 |
| Exact functions, full scope | 1,194 | 1,220 |
| Exact-function code bytes (`binary-progress.json`) | 108,385 | **113,287** |
| Unmapped original functions | 154 | 152 |

| Address | gods98 name | Function (matching build) | before | after |
| --- | --- | --- | ---: | ---: |
| `0047f130` | File_Initialise | `resource_mount_startup_target` | 17.51 | **100.00** |
| `0047f4f0` | File_FindDataCD | `resource_find_cd_key_drive` | 28.35 | **100.00** |
| `0047f590` | File_SetBaseSearchPath | `file_set_base_search_path` | 34.48 | 97.56 |
| `0047f600` | File_ErrorFile | `resource_debug_output_format` | 41.86 | 92.86 |
| `0047f640` | File_LoadWad | `file_load_wad` | 10.67 | 87.50 |
| `0047f660` | File_GetCDFilePath | `resource_cd_file_path_get` | 37.04 | **100.00** |
| `0047f6a0` | File_MakeDir | `file_make_dir` | 28.57 | **100.00** |
| `0047f6e0` | File_Open | `game_file_open_dispatch` | 41.10 | **100.00** |
| `0047f850` | File_Seek | `wad_file_seek` | 28.57 | 95.69 |
| `0047f980` | File_Read | `wad_file_read` | 29.00 | 95.18 |
| `0047fa50` | File_Write | `game_file_write` | 49.48 | **100.00** |
| `0047fac0` | File_Close | `game_file_close` | 29.27 | **100.00** |
| `0047fb10` | File_Tell | `wad_file_cursor_get` | 42.42 | **100.00** |
| `0047fb60` | File_Exists | `sound_sample_file_open_by_name` | 44.19 | 98.48 |
| `0047fc20` | File_GetC | `game_file_read_byte` | 43.10 | 96.15 |
| `0047fca0` | File_Length | `file_length` | 85.19 | **100.00** |
| `0047fce0` | File_InternalFGetS | `game_file_read_line` | 30.95 | **100.00** |
| `0047fd40` | File_GetS | `game_file_read_line_dispatch` | 29.47 | **100.00** |
| `0047fdb0` | File_PrintF | `game_file_print_format` | 47.19 | **100.00** |
| `0047fe20` | _File_GetSystem | `file_get_system` | 100.00 | **100.00** (C, was asm) |
| `0047fe30` | _File_CheckSystem | `game_file_open_route_select` | 18.46 | 97.96 |
| `0047fea0` | _File_OpenWad | `file_open_wad` | 24.39 | 94.44 |
| `0047fed0` | _File_Alloc | `game_file_handle_allocate` | 75.95 | **100.00** |
| `0047ff30` | _File_Malloc | `file_malloc` | 100.00 | **100.00** |
| `0047ff40` | _File_Free | `engine_heap_free_wrapper` | 100.00 | **100.00** |
| `0047ff50` | _File_Dealloc | `game_file_handle_release` | 44.44 | **100.00** |
| `0047ffc0` | _File_GetWadName | `game_file_mounted_path_prefix_rewrite` | 40.28 | **100.00** |
| `00480050` | File_GetLine | `game_file_read_crlf_line` | 57.14 | **100.00** |
| `004800a0` | File_LoadBinary | `file_load_binary` | unmapped | **100.00** |
| `004800c0` | File_Load | `file_load` | unmapped | **100.00** |
| `00480170` | File_LoadBinaryHandle | `map_open2` | 100.00 | **100.00** |
| `00480220` | File_VerifyFilename | `resource_path_resolve_against_base` | 16.99 | **100.00** |
| `004802b0` | File_SetLoadCallback | `loader_progress_callback_set` | 100.00 | **100.00** |
| `004802d0` | File_CheckRedundantFiles | `resource_file_token_list_load_and_tree_scan_target` | 13.33 | **100.00** |
| `00480390` | File_CheckDirectory | `resource_directory_tree_scan_target` | 40.76 | **100.00** |
| `00480570` | File_CheckFile | `file_check_file` | 36.36 | **100.00** |

The gods98 names are descriptive; the binary attests only the `__FILE__`
path and the `File_*` names in the error messages. The weights are the
functions' spans up to the next function, including padding.

## The unit

### State

- **`.data`**: `currWadHandle` (`004aaf00`, `-1`), the WAD number every
  Wad.c call passes: all archives, newest first.
- **`.bss`**: one `static File_Globs fileGlobs` at `005339a0`, `0xa1c`
  bytes: the base search path, `_File_GetWadName`'s result, File_VerifyFilename's
  result, an unused word (`005342a4`), the data directory, the load callback
  and its data, the CD letter and the base-path flag.
- **Commons**: File_CheckRedundantFiles's `fileRedundantList[2000][1024]`
  (`005769e0`) and its count `fileRedundantCount` (`0076a9e0`), both
  annotated (below, `gods98/keys.c`).
- **Strings**: each owned literal has a `// STRING` marker. The literals
  pooled with other units are marked here too, because no other unit marks
  them (finding 569): `"%s\\%s"` (`004a1cbc`, GAME-006), `"r"` (`004a1eac`,
  GAME-014), `"w"` (`004a52b8`, GAME-025), `"%s"` (`004a1d60`, GAME-009) and
  `"rb"` (`004a2770`, GAME-017). `"%s"` and `"rb"` were `// GLOBAL`s on named
  arrays in `screen_effect_quads.cpp` and `frontend_unit.cpp`. Only port code
  read them, and files.c cannot name a C++ array, so those two annotations are
  removed. The arrays stay for the port. When those units are converted, the
  markers move to them. With these markers and the newly named callees,
  25 functions outside the unit improve (reward, interface, terrain, sound,
  config). Four of them become exact: `0x00460f00`, `0x0047a870`,
  `0x0047aad0` and `0x0047b580`.

`make unit-data-offsets`, G98-014 row:

| Unit | .data | .rdata | .bss | Commons |
| --- | --- | --- | --- | --- |
| G98-014 | partial (33, 1 unknown) gap 17 first 004aafa4: a4/a8 | n/a | n/a (1) | `005769e0` `_fileRedundantList` (2,048,000), `0076a9e0` `_fileRedundantCount` |

### Names: the unit owns 26 port names in the matching build

Native code calls the port's file layer by the port's names: 59 callers in
21 files, and the native port's file loading depends on it. The port
resolves host paths (POSIX separators, mounted roots, no `_fullpath`),
reads the WADs through `resource.cpp`'s `Resource` records and takes its
startup prompts from host hooks. `files.c` cannot replace it natively, so
the native port keeps `files_port.cpp`, and `files.c` is compiled there
under `file_*` names that nothing calls.

In the matching build (MSVC 5.0), `files.h` maps 26 `file_*` names to the
port's names, and `files_port.cpp` leaves its own definitions of them out
(`FILES_PORT_DEFINES_UNIT_NAMES`). Every port caller of those names then
calls the original function, as the original's callers did. The renaming
is in the unit's own header, so no caller file changes and no declaration
is added (rule B, finding 538). Each name qualifies because all of its
port callers pass the original's arguments: the `GameFileHandle` record is
the original's `File` on a 32-bit host (type word, then `FILE*` or the
`{handle, position, eof}` WAD record). The C++ declarations gain
`extern "C"` in place: `resource.hpp` ×19 (17 of these names and the two
direct Wad.c entries below), `heap.hpp` and `media.hpp`.

These names keep the port's version in every build, because the port
contract differs, the port declaration is in a frozen header, or the port
version lives outside the unit's files:

| Address | Port name | Why |
| --- | --- | --- |
| `0047f590` | `wad_mount_path_cache_store` | frozen `resource_from_files.hpp` (C++ linkage) |
| `0047f640` | `mount_wad_from_path` | no return value; unmounts first |
| `0047f660` | `ares_cd_fallback_path` | `(name, out, size)`; `resource_cd_file_path_get` is the original's order and is owned |
| `0047f6a0` | `save_directory_ensure_exists` | takes the resolved save directory; File_MakeDir prefixes `Data\` |
| `0047fca0` | `save_stream_length` | takes a `FILE*` |
| `0047fe20` | `game_file_system_type_get` | no outside caller |
| `0047fea0` | `wad_entry_read_into_stream_slot` | stores a `Resource*` |
| `0047ff30` | `engine_heap_allocate` | no return value |
| `00480570` | `resource_directory_token_list_contains` | takes the list |
| `004800a0`, `004800c0` | `read_whole_file`, `read_whole_file_with_progress` | defined in `resource.cpp` |

For `read_whole_file`, `resource.h`, which only C units include
(`config.c`, `terrain_map.c`, `sound3d.c`), maps it to `file_load_binary`
in the matching build, as finding 572's aliases do. `sound3d.c` now
includes `files_paths.h`, which has the two declarations it took from the old
`files.h`, unchanged.

### Wad.c (G98-030, not converted): coupling

Files.c calls six Wad.c functions. The new boundary header
[`wad.h`](../../src/gods98/wad.h) declares them with the original's
contracts:

| Original | Address | How `files.c` reaches it | Cost |
| --- | --- | --- | --- |
| Wad_Load | `0048b4e0` | adapter `wad_load`: `wwad_archive_index_load` has the contract but is declared only in the frozen `resource_from_wad.hpp`, with C++ linkage | call operand in `0047f640` |
| Wad_IsFileInWad | `0048bd50` | `wad_entry_find_across_mounted_archives` (`resource.cpp`), direct | its annotation is a `/* */` comment, which Reccmp does not read: call operand in `0047fb60`, `0047fe30` |
| Wad_FileOpen | `0048bf70` | adapter `wad_file_open`: `ares_open_wad_path` returns a `Resource*` | call operand in `0047fea0` |
| Wad_FileClose | `0048bfc0` | `fixed_record_slot_resource_release`, direct | none |
| Wad_FileLength | `0048c010` | adapter `wad_file_length`: `wad_stream_length_get` returns nothing | call operands in `0047f850` (5), `0047f980`, `0047fc20` |
| Wad_FileData | `0048c030` | adapter `wad_file_data`: `wad_memory_stream_buffer_get` returns nothing | call operands in `0047f980`, `0047fc20` |

The adapters (`files_port.cpp`) copy the entry's data into a fixed-record
slot allocated from the C library heap, so `fixed_record_slot_resource_release`
frees it as the original does. Converting Wad.c would leave seven unit
functions with only these call operands to fix (`0047f640`, `0047f850`,
`0047f980`, `0047fb60`, `0047fc20`, `0047fe30`, `0047fea0`). The port's own
reads (`game_file_open_dispatch` and the others in `files_port.cpp`) keep
using `Resource` records through `fixed_record_slot_install`.

### Native

`files.c` compiles natively as C++ and passes `c_boundaries`. On hosts
without `_WIN32`, `files_native.h` maps the VC5 LIBC directory routines
(`_findfirst`, `_getcwd`, `_fullpath`, `_mkdir`, `_chmod`) and `MessageBoxA`,
`GetDriveTypeA`, `OutputDebugStringA` to POSIX stand-ins in
`files_port.cpp` (`// PORT-ONLY`). When compiled as C++, the debug-message calls
go to `files_debug_noop`, because C++ cannot call the original's unprototyped
empty function (`00484c90`) with arguments. The native port's file loading
is unchanged: it never calls `files.c`.

## Shape notes

- **`0047f130`**: `wadFile` and the `_findfirst` record are block locals
  and share a stack slot; the two `title[1024] = "Error"` buffers are block
  locals too.
- **`0047f4f0`, `0047f660`**: the CD letter is `unsigned char`
  (`and reg, 0xff`, the loop byte kept in a stack word).
- **`0047f590`** returns the flag (`mov eax, 1` before the store). The
  `FALSE` path stores the flag after the `rep stosd`, and the original stores
  it before. Five shapes and `msvc_permuter.py` (no candidates) were tried.
  It is an effective match (97.56%).
- **`0047fc20`**: the length is a statement of its own, so `f->wad` is
  reloaded after the call. The original reads a `dword` at the position,
  not a byte.
- **`0047f980`**: two locals (`read`, `amount`) give the original's
  if/else. `lea edi, [edx+ecx]` against `[ecx+edx]` does not follow the
  source order (rule B; permuter: no candidates). `""` is `004b8a14`,
  `config.c`'s static.
- **`0047ffc0`**: early return for a short name inside the
  `basePathSet` block.
- **`004802d0`**: the original passes the `File*` to `fscanf` and
  `fclose` as a `FILE*`. This is kept.

## Near misses

| Address | Score | What is left |
| --- | ---: | --- |
| `0047f600` | 92.86 | `vsprintf` (`0048e1e0`) is annotated as the port's four-argument `formatted_text_buffer_build` |
| `0047f640`, `0047f850`, `0047fea0`, `0047fc20`, `0047fb60`, `0047fe30` | 87–98 | the Wad.c call operands above |
| `0047f980` | 95.18 | Wad.c operands, `""`, the `lea` operand order |

**`005769e0` and `keys.c`**: Key_Find (`0x00485e30`, G98-018) ends its walk
at `005769e0`. #595 (finding 580) sized `key_names` back to 256 slots, and
Key_Find is 96.77% at its head: the rebuild's end bound, `key_names + 0x400`,
is an unnamed address. With `fileRedundantList` annotated, Key_Find stays at
96.77%: the original's bound is now named `fileRedundantList`, and the
rebuild's is `fileRedundantCount`, which the link places between the two
commons (`key_names` `008c9ae0`, `fileRedundantList` `008c9f00`). No
storage is added. `0x004802d0` and `0x00480570` become exact. **For
follow-up:** when the common order puts the list directly after the table,
Key_Find matches too.

## Entropy

`make reccmp-check` against `81a2877b`: no exact match lost; link-order
map OK; `winapi-link-check` OK. 54 functions improved, 2 newly mapped.

One drop, outside the unit:

| Function | File | Score | Label |
| --- | --- | --- | --- |
| `0x00459470` osf_read | `game/object_state_file.cpp` | 12.62 → 9.71 | contract (finding 579 class (c)); source unchanged. It calls the port's `save_stream_length(FILE*)`, which was annotated at `0047fca0`, File_Length, and is now an unannotated port-tail function. The original calls File_Length with a `File*`. No instruction-count change |

The one drop has no instruction-count change. Header edits are
`extern "C"` in place, comments, or macros in `files.h`/`resource.h`, which
add no declaration. `frontend_unit.cpp` and `screen_effect_quads.cpp` only
lose a `// GLOBAL` comment. No file owned by #587, #588, #448, #591 or #592
is edited.

## `/OPT:REF` (finding 579)

| | before | after |
| --- | ---: | ---: |
| `opt_ref.annotated.full`: removed / exact removed | 773 / 339 | **755** / 335 |
| `opt_ref.annotated.game_only`: removed / exact removed | 738 / 312 | 736 / 322 |
| The unit's object: removed (exact) | `files.cpp.obj` | `files.c.obj` 13 (11) |

Exact removed functions rise in game-only scope because 26 more functions are
exact. The 13 still removed are reached only from removed roots (Lego.c,
container, tutorial and startup-config code), or from live port callers
with other contracts. These are class (c): `sav_write_slot_file`,
`osf_write` and `save_write_thumbnail` write with `fopen`/`fwrite`;
`osf_read` passes a `FILE*`; `movie_key_path` and `model_parse` make no call
at the site.

## Reccmp lint ratchet

1,390 → 1,378 entries (−12, no growth): the port tail's static helpers,
which were unannotated definitions in a unit file, are now in a tail file.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox): OK after the baseline
  refresh; link-order map OK; `winapi-link-check` OK.
- `make match-progress` (as `reccmp-report`, `winapi-link-check`,
  `reccmp-baseline-update`, `binary-progress-compare`): the three generated
  files.
- `make unit-data-offsets`: the G98-014 row above.
- `python3 tools/translation_units.py build`: 0 conflicts;
  `tools/binary_progress_inputs.txt` regenerated.
- `make tools-test`: lint (ledger shrinks), link order, winapi list, Meson
  parity, tool tests OK.
- `files.c` passes `gcc -std=gnu89 -Werror=declaration-after-statement
  -fsyntax-only`.
- Native: `cmake -S . -B build && cmake --build build && ctest --test-dir
  build`: 37/37; with `TEST_WADS_DIR` set to the shipped WADs, 39/39.

## Reproducing

    python3 tools/convert_unit.py G98-014 --stem files   # in a copy without the .cpp entry
    make match-function SOURCE=src/gods98/files.c
    make reccmp-check
    make match-progress
    make unit-data-offsets
    make opt-ref-link opt-ref
    make opt-ref-callers OPT_REF_ARGS="--object files.c.obj"
