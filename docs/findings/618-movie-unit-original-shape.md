# Finding 618 — Movie.cpp (G98-001) rebuilt in original shape

Issue #682, part of #388 under [ADR 0005](../adr/0005-original-shape-c-units.md).
G98-001 (`004721e0`-`00472710`) is gods98's movie player. It plays a file
through DirectShow's multimedia streaming (amstream). The unit map gave all
11 functions low confidence (boundary evidence only), and the code lived in
`src/game/video_graph.cpp` as the port's model: a `DirectShowGraphState`
record driven by a hook table that nothing set.

This change rebuilds the unit in the original's shape as the C++ unit
[`src/gods98/movie_unit.cpp`](../../src/gods98/movie_unit.cpp), with the
boundary header [`movie_unit.h`](../../src/gods98/movie_unit.h), in the
`gods98` library, at the same link-order position. As in finding 604, the
unit stays **C++** because the original object has thiscall methods and an
EH frame (finding 505 rule 8). The old file, `video_graph.hpp`, the hook
table and Lego.c's two port adapters are deleted. Nothing calls them once
the callers are switched (see [Callers](#callers)).

- **G98CMovie** (0x38 bytes, no vtable) opens the file into an
  IAMMultiMediaStream with a DirectDraw video stream and the default audio
  renderer. It creates one stream sample, and on each Update it blits the
  sample's surface to the back buffer (or a rectangle of it).
- **Entry points** (`extern "C"`): `movie_load`, `movie_get_size`,
  `movie_get_duration`, `movie_update` and `movie_free`.

The binary has no RTTI and attests none of the names. The unit name
`Movie.cpp` and the class name `G98CMovie` are `inferred`, from gods98's
movie module as OpenLRR reconstructs it. The method names are descriptive.

"Before" is `main` `9043af89`. "After" is this change, from the full
matching build (`make reccmp-check`).

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (11 functions, weighted by span, 1,328 bytes) | 12.62% | **99.68%** |
| Exact functions in the unit | 0 | **10 of 11** |
| Exact-function code bytes (`binary-progress.json`) | 205,613 | **206,989** |
| Exact functions, full / game-only | 1,731 / 1,522 | **1,742 / 1,533** |
| `opt_ref` annotated (full, by address): removed / exact removed | 412 / 223 | **403 / 218** |
| `opt_ref` annotated (game-only): removed / exact removed | 405 / 216 | **396 / 211** |
| Import DLLs (`make pe-audit`) | 10 (`ole32` missing) | **11, all in the original's order** |
| Reccmp lint ledger (entries) | 1,052 | **1,049** |

| Address | Function, before | before | Function, after | Bytes | after |
| --- | --- | ---: | --- | ---: | ---: |
| `004721e0` | `lego::directshow_sample_setup` | 8.26 | `G98CMovie::InitSample` | 208 | **100.00** |
| `004722b0` | `lego::configured_video_graph_setup` | 8.33 | `G98CMovie::OpenStream` | 224 | **100.00** |
| `00472390` | `lego::configured_video_com_route_init` | 10.53 | `G98CMovie::G98CMovie` | 160 | **100.00** |
| `00472430` | `lego::directshow_interface_release` | 13.51 | `G98CMovie::~G98CMovie` | 112 | **100.00** |
| `004724a0` | `lego::configured_video_sample_update` | 6.45 | `G98CMovie::Update` | 144 | 97.01 |
| `00472530` | `lego::video_duration_frame_bound_update` | 10.26 | `G98CMovie::GetDuration` | 48 | **100.00** |
| `00472560` | `directshow_render_surface_create` | 13.33 | `movie_load` | 272 | **100.00** |
| `00472670` | `lego::directdraw_surface_description_get` | 12.50 | `movie_get_size` | 80 | **100.00** |
| `004726c0` | `lego::video_frame_bound_refresh` | 0.00 | `movie_get_duration` | 16 | **100.00** |
| `004726d0` | `lego::directshow_sample_update_and_blit` | 58.82 | `movie_update` | 32 | **100.00** |
| `004726f0` | `directshow_wrapper_destroy` | 64.00 | `movie_free` | 32 | **100.00** |

Bytes are each function's span up to the next function, padding included.

Outside the unit, `0x0042efc0` `lego_play_movie` (Lego.c's
Lego_PlayMovie_old) goes from 96.25% to **100%**, because it now calls the
unit's Movie_GetSize and Movie_Update. No function fell below the baseline.

## Unit extent

The map's boundary is right: the unit is the 11 functions
`004721e0`-`00472710`, and none moves.

- **Call graph.** An `E8`/`E9` scan of `.text` finds the unit entered from
  outside only by:
  - FrontEnd.c: `00415700` (Front_PlayMovie: Movie_Update, Movie_GetDuration)
    and `00415910`/`00415990` (Movie_Load, Movie_Free);
  - Lego.c `0042efc0` (Movie_Load, Movie_GetSize, Movie_Update, Movie_Free);
  - Reward.c `00461740` (Movie_Load, Movie_Free).

  The five methods are reached only from the unit.
- **Callees.** The 11 functions call only each other and:
  - AITask.c's out-of-line DirectDraw accessors `004065d0`/`004065e0`;
  - Files.c's File_GetCDFilePath `0047f660` and File_VerifyFilename `00480220`;
  - LIBC: `malloc` `0048dbd0`, `free` `0048db80`, `__alldiv` `0048e9e0`,
    operator new `0048ead0`, `fclose` `0048eae0`, `fopen` `0048eb90`;
  - operator delete `0045daf0`;
  - through the import table, `CoInitialize`, `CoCreateInstance` (ole32) and
    `MultiByteToWideChar` (KERNEL32).
- **Data.** The unit has no `.data` or `.bss`. Its only `.rdata` references
  are seven GUIDs in the GUID block `0049f158`-`0049f3a0`. That block is
  library data (DirectShow's and DirectX's GUID libraries): IID_IAMMultiMediaStream
  `0049f168`, CLSID_AMMultiMediaStream `0049f1e8`, IID_IDirectDrawMediaStream
  `0049f228`, MSPID_PrimaryVideo `0049f288`, MSPID_PrimaryAudio `0049f298`,
  IID_IDirectDraw2 `0049f318` and IID_IDirectDrawSurface3 `0049f358`. Nothing
  outside the unit references any of the seven. The `"r"` string `004a1eac`
  is pooled with GAME-014 and 20 other references; it stays unmarked here.
- **Imports.** `CoInitialize` (`0076e5c8`) and `CoCreateInstance`
  (`0076e5c4`) are referenced only by the unit.
- **EH.** The handler `0049ddac` and its unwind funclet `0049dd9e` (operator
  delete on the new-expression's temporary) belong to `00472560`.
- **Neighbours.** `00472130` is Weapons.c's last function (GAME-050), and
  `00472710` starts Container.c (G98-002).

`translation-units.spec.csv` records this evidence in G98-001's note, with
the name `Movie.cpp` (`inferred`). The computed boundary confidence stays
`low`, because padding cannot separate objects (finding 505).

## The unit

### Layout

G98CMovie's field offsets are the ones the code uses:

| Offset | Field |
| ---: | --- |
| `+0x00` | IAMMultiMediaStream |
| `+0x04` | the last HRESULT |
| `+0x08` | IMediaStream (the primary video stream) |
| `+0x0c` | IDirectDrawMediaStream |
| `+0x10` | IDirectDrawSurface (the sample's) |
| `+0x14` | IDirectDrawSurface3 (the same surface) |
| `+0x18` | IDirectDrawStreamSample |
| `+0x1c` | IDirectDrawSurface3 (the back buffer) |
| `+0x20` | the sample's RECT |
| `+0x30` | the file name (`malloc`) |
| `+0x34` | IDirectDraw2 |

The constructor leaves `+0x00` and `+0x04` alone until OpenStream and
InitSample set them, as the original does.

### Interfaces, GUIDs and imports

MSVC 5.0 has no `amstream.h`, and its `ddraw.h` is DirectX 3's (finding
596). The unit declares each interface it calls, up to the last slot it
uses, as an abstract struct with `__stdcall` virtuals, as `avi_player.cpp`
does. The seven GUIDs are the unit's own `static const` copies with
`// GLOBAL` annotations, as in `directdraw.c`. The matching build already
lists `ole32.lib` (finding 574), so the unit's `CoInitialize` and
`CoCreateInstance` calls bring in the ole32 import descriptor. `make
pe-audit` now finds 11 DLLs, none missing, in the original's order (`order
equal True`). Finding 604 had left `ole32` as the last missing DLL. The
MinGW reference build gains `ole32` in its library list.

### Source shapes

- **InitSample** is a chain of `if (FAILED(m_err)) goto fail;` with
  `return true;` before the `fail:` label. The original's layout, with the
  success return after the four releases and no reload of `m_err` after
  SetState, comes only from this shape. Two nested-`if` forms scored 85% and
  89%.
- **`cond ? true : false` returns** in InitSample, OpenStream and Update.
  With `return cond;`, MSVC 5.0 zeroes `eax` before the `setcc` (or uses
  `neg`/`sbb`/`inc`). The original has a bare `setcc`. movie_update's
  `? 1 : 0` gives the original's `neg`/`sbb`/`neg` from the bool.
- **Update** stays at 97.01%. The original compares both HRESULTs with
  `cmp eax, edi` (the register holding 0); the rebuild uses `test eax,
  eax`. These were tried, with no change:
  - the constants `0`, `0L`, `0UL`, `0u`, `NULL`, `(HRESULT)NULL` and
    `(DWORD)0`;
  - `DWORD`, `ULONG`, `int` and `void*` return types on the two methods;
  - either operand order;
  - an `HRESULT` local (91.43%), a zero local, a `bool` result local
    (29.46%) and an if/else layout (66.67%);
  - 1 to 32 inert declarations before the class (rule B: no change in any
    unit function).

  `msvc_permuter.py` made no attempts on the C++ file. No asm is used.

## Callers

The unit's contract is the original's, so every caller with the original's
contract now calls it:

- **`lego_unit.c`** (`0042efc0` Lego_PlayMovie_old) calls `movie_load`,
  `movie_get_size`, `movie_update` and `movie_free`. The two `PORT-ONLY`
  adapters in `lego_port.cpp` are deleted: `lego_movie_get_size_adapter`
  filled a port surface description, and `lego_movie_update_adapter`
  returned 0.
- **`reward.c`** (`00461740`) calls `movie_load` and `movie_free`. Its
  boundary header `video_graph.h` becomes
  [`reward_movie.h`](../../src/game/reward_movie.h) under `git mv`, with the
  same four declarations. FrontEnd.c's `movie_play_wait` adapter
  (Front_PlayMovie, `00415700`, not yet converted) now takes a `G98CMovie*`.

FrontEnd.c's `00415910`/`00415990` callers are port models that do not play
movies, and Front_PlayMovie is still an unannotated no-op adapter. So
`/OPT:REF` keeps 9 of the 11 unit functions. It removes GetDuration and
movie_get_duration, whose only caller is Front_PlayMovie.

### Lego.c's declarations: `Deviation (scoring only)`

`lego_unit_imports.h` swaps the old block for the unit's four declarations,
with `struct tagRECT` for Movie_Update's RECT (`lego_unit.c` casts its
`LegoRect2E`, which has RECT's layout). With every parameter named, the
symbol counter moved and two Lego.c functions dropped (finding 538 rule B):

- `0x00431450` from 100% to 83.78% (register assignment);
- `0x004311d0` from 70.80% to 67.15%.

Inert declarations at the same place (−1 to +3) did not restore them.
Leaving `movie_load`'s parameter unnamed does, so it is tagged `Deviation
(scoring only)`. The `G98CMovie` tag is declared by `movie_load`'s return
type, with no separate forward declaration.

## Native

The native port has no DirectShow. `movie_unit.cpp` is empty there
(`LEGORRR_BACKEND_MINIWIN`), and the native-only port tail
[`video_graph_port.cpp`](../../src/gods98/video_graph_port.cpp), unannotated,
defines the five entry points. `movie_load` returns NULL, so Lego.c and
Reward.c skip the playback, as they did before:

- the old model's update returned 0;
- `movie_play_wait` is a no-op.

Native movie playback through `movie.cpp`'s FFmpeg reader is a port gap
(native gameplay is deferred, #552), filed as #692. Native ctest passes
(37 of 37).

## Measurements

- `make reccmp-check`: 12 improved, none fell below the baseline. No
  listing digest outside the unit changed (`match-listing-digests.txt`), so
  no instruction count did either; the only score outside the unit that
  moved is `lego_play_movie`'s.
- `make match-progress`: the three generated files are refreshed.
- `make unit-data-offsets`, G98-001 row: `n/a` in `.data`, `.rdata` and
  `.bss` (fewer than two attributable items), as expected for a unit that
  has no data of its own.
- `make opt-ref-callers`: `movie_unit.cpp` is not listed among objects with
  removed annotated functions that have live callers.
- `make tools-test` (no `--allow-growth`): the lint ledger loses the three
  old port definitions (`lego::copy_string`,
  `lego::directshow_graph_hooks_set`, `lego::directshow_partial_release`).
