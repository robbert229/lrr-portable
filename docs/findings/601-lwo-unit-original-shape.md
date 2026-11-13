# Finding 601 — LightWave object unit (G98-031) rebuilt in original shape

Issue #640, under [ADR 0005](../adr/0005-original-shape-c-units.md). #496's
relocation put G98-031 verbatim into `src/gods98/lwo.cpp`. Those functions
were the port's whole-image LWO reader (`model_parse`, `model_open`,
`model_free` and their static chunk parsers). They carried the unit's
annotations, but they did not have the original's contract. This change
rebuilds the unit as the C unit [`src/gods98/lwo.c`](../../src/gods98/lwo.c),
with the C boundary header [`lwo.h`](../../src/gods98/lwo.h). The port's
reader moves, unannotated, to the port tail
[`src/gods98/lwo_port.cpp`](../../src/gods98/lwo_port.cpp) (`git mv`, a
`tail` link-order entry and a Meson source). Native model loading keeps
using it. The unit is text `0048c040`–`0048d920`: gods98's LightWave object
loader. The binary does not name the file.

- **LoadAppObj** (`0048d7c0`) allocates a 0x20-byte `APPOBJ`. It keeps the
  object's directory (`_splitpath`) and calls LoadLWO.
- **LoadLWO** (`0048d2c0`) opens `"<name>.lwo"` through Files.c and checks
  the `FORM`/`LWOB` header. It then dispatches each chunk: `PNTS`
  (vertices), `SRFS` (surface names), `SURF` (one surface's settings),
  `CRVS` (walked and dropped) and `POLS` (polygons). It seeks past any
  other chunk. Optionally it opens `"<name>.uv"` and keeps that file open.
  At the end it puts the surfaces in `SRFS` order, and gives a name with
  no `SURF` chunk a white `"Default"` surface.
- **FreeLWOB** (`0048d870`) frees all of it. `0048c180` frees one surface
  and its chain of `TTEX` texture layers.

"Before" is the *match baseline* on `main` at `8601d8ab`. "After" is this
change.

## Result

| | before | after |
| --- | ---: | ---: |
| Exact functions in the unit | 2 | **8 of 13** |
| Mapped functions in the unit | 13 | 13 |
| Exact-function code bytes (`binary-progress.json`) | 178,866 | **179,821** |
| Accuracy, full / game-only | 83.30% / 81.67% | 83.56% / 81.95% |

| Address | gods98 role | Function (matching build) | Bytes | before | after |
| --- | --- | --- | ---: | ---: | ---: |
| `0048c040` | stristr (private copy) | `string_case_substring_find` | 128 | 76.71 | **100.00** |
| `0048c0c0` | even-size string copy | `lwo_string_duplicate` | 96 | 89.80 | **100.00** |
| `0048c120` | CTEX/TTEX projection | `lwo_texture_projection_mode_parse` | 96 | 100.00 | **100.00** |
| `0048c180` | free a surface | `owned_resource_tree_release` | 80 | 90.62 | **100.00** |
| `0048c1d0` | big-endian float | `lwo_read_big_endian_float` | 64 | 100.00 | **100.00** |
| `0048c210` | LwoPoints | `lwo_points_read` | 336 | 13.61 | 72.97 |
| `0048c360` | LwoCurves | `lwo_curves_read` | 128 | 23.88 | **100.00** |
| `0048c3e0` | LwoPolygons | `lwo_polygons_read` | 688 | 10.47 | 58.35 |
| `0048c690` | LwoSurfaceNames | `lwo_surface_names_read` | 400 | 7.11 | **100.00** |
| `0048c820` | LwoSurfaceSettings | `lwo_surface_settings_read` | 2720 | 6.34 | 80.66 |
| `0048d2c0` | LoadLWO | `lwo_load` | 1280 | 10.03 | 99.29 |
| `0048d7c0` | LoadAppObj | `lwo_app_obj_load` | 176 | 9.13 | 96.88 |
| `0048d870` | FreeLWOB | `lwo_app_obj_free` | 176 | 14.69 | **100.00** |

Bytes are each function's span up to the next function, padding included.
The gods98 names are descriptive; the binary attests only the chunk tags.
`make match-function` scores `0x0048d2c0`, `0x0048d870` and `0x0048c360` a
little lower than the full build does. The difference is the Files.c call
operands: `files.h` gives Files.c's functions the port's link names in the
matching build, and only the full build maps them.

## Unit extent

The map gave the first two functions low confidence. Both belong to the
unit:

- `0048c040` (a stristr duplicating Utils.c's `00477610`) is called only by
  `0048c820`.
- `0048c0c0` is called only by `0048c820`, `0048d2c0` and `0048d7c0`.
- The unit's only callers outside it are Mesh.c's `004809e0` (Mesh_Load),
  through `0048d7c0` and `0048d870`
  (`docs/coverage/call-graph/edges.csv`).
- A scan of `.text` for operands in `004af9f4`–`004afca0` (the literals)
  and for `0049eaf0` (`0.00390625f`) finds 61 references. All of them are
  in `0048c12f`–`0048d6d7`.
- `0048d920` is LIBC's `__fpmath`.

`translation-units.spec.csv` records this evidence in G98-031's note. The
computed confidence stays `low`, because padding cannot separate C objects
(finding 505).

## The unit

### State

The unit has no `.data` or `.bss` of its own. Its state is the `APPOBJ`,
`LWSIZE`, `LWPOLY`, `LWSURFLIST` and 0x54-byte `LWSURFACE` records, laid
out in `lwo.h` from the field offsets the code uses.

- **Strings**: each owned literal has a `// STRING` marker on the line
  above the line that uses it. The original lays them out in reverse order
  of use within each function, and MSVC 5.0 reproduces that.
- **Pooled strings**: `","` (`004a13e4`, GAME-001's) and `"rb"`
  (`004a2770`, marked in Files.c) carry no marker here.
- **`0049eaf0`**: a plain `/ 256.0f` literal.

`make unit-data-offsets`, G98-031 row:

| Unit | .data | .rdata | .bss | Commons |
| --- | --- | --- | --- | --- |
| G98-031 | gaps-only (58, 0 unknown) | n/a (0, 0 unknown) fewer than two attributable items | n/a (0, 0 unknown) fewer than two attributable items | - |

Unit summary: full 0, gaps-only 7, partial 27, n/a 191.

### Callees: Files.c and Wad.c

`lwo.c` includes [`files.h`](../../src/gods98/files.h) and calls
`file_open`, `file_read`, `file_seek`, `file_close` and `file_exists`. In
the matching build these are Files.c's own functions, under their port
link names (finding 583): `game_file_open_dispatch`, `wad_file_read`,
`wad_file_seek`, `game_file_close` and `sound_sample_file_open_by_name`.
They read Wad.c's tables (finding 591). No caller edit is needed for this.

The other callees are LIBC:

- `sprintf`, `strncpy`, `_strnicmp`, `_strcmpi`, `malloc` and `free`
  reach the port's CRT through `crt/vc5names.h` on non-MSVC hosts.
- `_splitpath` is new in that header's non-MSVC branch. It maps to
  `crt_path_split`, a `PORT-ONLY` model at the end of `lwo_port.cpp`. The
  MSVC builds link VC5's `splitpat.obj`.

### Contracts: five helpers keep port names

Port code calls five of the unit's functions whose contracts match the
original's. They keep the port's names. `lwo.h` declares them with the
port's types, and `lwo.c` defines them with the original's types under its
own (`extern "C"`) prototypes (the `lws.c` pattern, finding 561).

| Function | Declared in (`extern "C"`) | Port callers |
| --- | --- | --- |
| `string_case_substring_find` | `string.hpp` (already C) | `test_string.cpp` |
| `lwo_string_duplicate` | `model.hpp` (added in place) | `test_model.cpp` |
| `lwo_texture_projection_mode_parse` | `model.hpp` (added in place) | **`lwo_port.cpp`'s `build_model`** (native model loading), `test_model.cpp` |
| `owned_resource_tree_release` | `crt.hpp` (added in place, guarded) | `test_crt.cpp` |
| `lwo_read_big_endian_float` | `model.hpp` (added in place) | `test_model.cpp` |

The port's NULL guards are gone, as the original has none. The three tests'
NULL calls are removed (`test_string.cpp`, `test_model.cpp`,
`test_crt.cpp`).

The length output of `string_case_substring_find` is a `size_t*`. That is
`unsigned int` under MSVC 5.0, so the code is the same, and on 64-bit
hosts it is the port's type. `owned_resource_tree_release`'s three pointer
fields sit at the same pointer-sized slots that the port's `void**` node
uses.

### Port tail and native model loading

`model_parse`, `model_open` and `model_free` do not have the original's
contract. They parse a whole image into the port's `Model`, while the
original fills an `APPOBJ` through File_Read. So they keep their names,
unannotated, in `lwo_port.cpp`, with their static parsers.

- Natively, `mesh_unit.cpp`'s `lwo_model_entry_load` (Mesh_Load, still the
  port's) loads meshes through them. Their only change is that
  `build_model` now calls the unit's projection parser.
- The ledger stub for `0048c360` (`ledger_stub_0048c360_LwoCurves`) had no
  callers and is deleted.
- The three projection-name statics `s_lwo_projection_*`, whose
  `// GLOBAL` annotations became the unit's `// STRING` markers, are also
  deleted.

**Not switched:** Mesh_Load's calls to LoadAppObj/FreeLWOB. The port's
Mesh_Load consumes a `Model`, not an `APPOBJ`, so it needs Mesh.c's own
conversion (G98-015).

## Shape notes

- **`char id[10] = {0}`** appears in LoadLWO and SurfaceSettings, where the
  chunk tag is read into it. It also appears, unused, in Points and
  SurfaceNames: the original zeroes nine bytes of an unused 10-byte local
  there (the first byte's store is dead). Declaring it takes SurfaceNames
  from 79.20% to 99.24%. In Points it changes the frame, but the slot
  order still differs (below). This is attested storage, not a filler: the
  stores are in the original's code.
- **Two byte-swap spellings.**
  - `((x & 0xff) << 8) + (x >> 8)` (`LWO_SWAPWORD`) gives the original's
    `mov dl, ah`/`shl` form in Curves and in SurfaceSettings' sub-chunk
    size, `TFLG` and `FLAG`.
  - `(unsigned short)((x << 8) | (x >> 8))` (`LWO_SWAPSHORT`) scores best
    in Polygons and in the `TRAN`…`GLOS` value.
  - Eight spellings were compared per function.
- **`0048c360`**: the vertex skip is `for (i = 0; i != count; i++)` with
  an `unsigned short` `i`. With `<` it gives `jbe`; the original has `je`.
- **`0048c690`**: the `LWSURFLIST` is cleared with `memset`, which gives
  the original's separate `xor ecx, ecx`.
- **`0048c040`**: written as `for (i = 0; i < len; i++, str++)`, so `inc
  edi` comes before `inc esi`.
- **`0048d2c0`**: the `FORM` and `LWOB` checks are early error returns.
  MSVC tail-merges the two identical error blocks into the one the
  original places between the header checks and the chunk loop (84.53% to
  96.71%).
- **`0048d870`**: the counts are copied to a local before each loop, so
  MSVC counts down a register (41% to 100%).
- **`0048c820`**:
  - The `COLR` bytes are passed as `(unsigned char)`.
  - `srfTexFlags |= 0x80` comes before the `"(sequence)"` truncation.
  - The loop indices and `printed` are `unsigned int`/`unsigned short`.
  - `TFLG`'s name table has 7 entries but its loop runs 8 times, as in the
    original: the eighth read is the next array's first slot.

No naked asm and no `Deviation (scoring only)` are in the unit.

## Near misses

| Address | Score | What is left |
| --- | ---: | --- |
| `0x0048d7c0` | 96.88 | `free(*ao)` loads into `ecx` where the original reuses `esi`. Three source shapes tried; inert declarations before it (1–16) change nothing |
| `0x0048c820` | 80.66 | (1) both flag loops: the original keeps a separate count-down (`mov [esp+0x18], 8` … `dec`/`jne`), the rebuild compares `i` with the bound (`cmp`/`jb`). `!=`, `<=`, `do`/`while`, a variable bound and five index types all compile the same. (2) Register choice: the original keeps `0` in `esi` and reads `surf` from its argument slot; the rebuild gives `surf` a register. (3) A one-dword shift in the frame from slot sharing. `msvc_permuter.py`, 300 s: no candidates |
| `0x0048c210` | 72.97 | Frame order only: the original has the point (and the unused `id`) at `esp+0x10`, then `ptr`, `buff` and the count-down; the rebuild puts `ptr` first. Declaration order (60 random permutations), separate floats, block scopes, statement order and 1–8 inert declarations all leave the code unchanged. Permuter: no candidates |
| `0x0048c3e0` | 58.35 | Register allocation: the original keeps `ptr` in `ebx` and leaves `csize` and the polygon count in memory; the rebuild enregisters `csize`. With `LWO_SWAPWORD` the swaps match instruction for instruction but the allocation moves further (53.55%). Loop forms and index types tried. Permuter, 240 s: no candidates |

## Entropy

`make reccmp-check` against the baseline at `8601d8ab` passes:

- 11 functions improved in the unit;
- no function fell, and no exact match was lost;
- the link-order map is OK, `winapi-link-check` is OK, and the
  plain-immediate check is OK.

No score changed outside the unit. The only listing digests that changed
are the unit's 13, so there are no instruction-count changes elsewhere.

Header edits:

- in place: `extern "C"` on three declarations in `model.hpp`, and
  `crt.hpp`'s `owned_resource_tree_release` (guarded with `#ifdef
  __cplusplus`);
- comments only: `string.hpp`;
- `crt/vc5names.h`: its non-MSVC branch only (`crt_path_split`,
  `_splitpath`).

No declaration was added to or removed from a matching-build header. No
frozen `_for_`/`_from_` header is edited. No file owned by #637, #638,
#639 or #641 is edited.

## `/OPT:REF` (finding 579)

| | before (`main`) | after |
| --- | ---: | ---: |
| `opt_ref.annotated.full`: removed / exact removed | 498 / 239 | **507 / 247** |
| `opt_ref.annotated.game_only`: removed / exact removed | 479 / 226 | 488 / 234 |
| Exact annotated functions (full) | 1,556 | 1,562 |

`make opt-ref-callers OPT_REF_ARGS="--object lwo.c.obj"` reports 12
removed, 7 of them exact. All twelve are under one root:

- LoadAppObj and FreeLWOB's only original caller, Mesh_Load (`004809e0`),
  is live but is the port's, and calls `model_open` and its own texture
  binder instead.
- Every other unit function is reached only from LoadAppObj and FreeLWOB.
- The one kept function is `lwo_texture_projection_mode_parse`, which the
  port reader calls.

The removed count rises because six more exact functions sit under that
root. It falls when Mesh.c is converted and Mesh_Load calls LoadAppObj.

## Reccmp lint ratchet

1,267 → 1,250 entries (−17, no growth). The port reader's 17 static
helpers (`be16`, `build_model`, `surfrec_addblock`, …) are now in a tail
file.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox): OK; link-order map
  OK.
- `make match-progress`: the three generated files.
- `make unit-data-offsets`: the G98-031 row above.
- `python3 tools/translation_units.py build`: 0 conflicts.
  `tools/binary_progress_inputs.txt` is regenerated.
- `make tools-test`: OK. Lint (the ledger shrinks), link order, the winapi
  list, Meson parity and the tool tests all pass.
- `make winapi-link-check`: OK.
- `lwo.c` passes `gcc -std=gnu89 -Werror=declaration-after-statement
  -fsyntax-only`. With `-Wall` it warns only that the two `id` locals are
  unused, which they are in the original too.
- Native: `cmake -S . -B build -DTEST_WADS_DIR=<shipped WADs> && cmake
  --build build && ctest --test-dir build`: 39/39.
  - The tests include `c_boundaries`, which now pairs `lwo.h` with
    `model.hpp`, `string.hpp` and `crt.hpp`.
  - They also include the real-data `model` test: all 1,132 shipped `.lwo`
    files parse through the port reader (and so through the unit's
    projection parser), 36,506 vertices and 20,119 polygons, digest
    `3ef7c74c`.

## Reproducing

    git mv src/gods98/lwo.cpp src/gods98/lwo_port.cpp   # tail entry, then:
    python3 tools/convert_unit.py G98-031 --stem lwo
    make match-function SOURCE=src/gods98/lwo.c
    make reccmp-check
    make match-progress
    make unit-data-offsets
    make opt-ref-link opt-ref
    make opt-ref-callers OPT_REF_ARGS="--object lwo.c.obj"
