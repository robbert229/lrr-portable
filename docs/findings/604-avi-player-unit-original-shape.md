# Finding 604 — AVI player unit (G98-012) rebuilt in original shape

Issue #649, under [ADR 0005](../adr/0005-original-shape-c-units.md). #496
relocated G98-012 verbatim into `src/gods98/avi_player.cpp`. Those functions
were the port's models of the AVI player: `AviPlaybackState` methods over the
port's movie and surface layers, hook tables, and ledger stubs. They carried
the unit's annotations, but they did not have the original's contract.

This change rebuilds the unit in the original's shape, in the same file
[`src/gods98/avi_player.cpp`](../../src/gods98/avi_player.cpp), with the
boundary header [`avi_player.h`](../../src/gods98/avi_player.h). The port's
models move, unannotated, to the port tail
[`src/gods98/avi_player_port.cpp`](../../src/gods98/avi_player_port.cpp), as
a `tail` link-order entry and a Meson source.

This is the first unit that stays **C++** in original shape. The original
object has `thiscall` methods, two vtables and EH frames (finding 505
rule 8), so it is compiled as C++ with `/GX`, like the other `.cpp` list
entries. The unit is text `0047e460`–`0047ed90`.

- **AviSurface** (0x98 bytes, vtable `0049e9a0`) wraps an
  IDirectDrawSurface4. The DirectDraw object comes from `avi_initialise`.
  `Init` builds a DDSURFACEDESC2 for the requested size and bit depth. It
  creates the surface, gives an 8-bit surface a zeroed palette, and marks a
  16-bit surface whose masks sum to 15 bits as 555.
- **AviMovie** (0xc4 bytes, vtable `0049e9b0`) opens a file's video stream
  with `AVIStreamOpenFromFileA` (the name goes through Files.c's
  `File_VerifyFilename`). It opens a frame decoder and reads the stream
  information. It then allocates an AviSurface the size of the frame.
  `Update` decodes the current frame, copies it bottom-up into the surface
  (15 or 16 bits per pixel only), and derives the next frame from the
  elapsed time.
- **Entry points** (`extern "C"`): `avi_initialise`, `avi_load`,
  `avi_update`, `avi_blit_to_back_buffer` (Blt to the back buffer),
  `avi_set_time`, `avi_get_time`, `avi_get_length`, `avi_remove` and
  `avi_is_ok`.

The binary has no RTTI and attests none of the names; the class and
function names are descriptive. "Before" is the *match baseline* at #658's
head (`ef9487e0`, stack #656: #655, #654, #657, #658 on `main` `321fdbf1`).
"After" is this change, rebased onto it as the stack's top layer. The
unit's per-function scores are the same as on `main` `18cc9483`.

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (27 functions, weighted by span, 2,352 bytes) | 22.19% | **94.10%** |
| Exact functions in the unit | 4 | **24 of 27** |
| Mapped functions in the unit | 25 | 27 |
| Exact-function code bytes (`binary-progress.json`) | 188,731 | **190,333** |
| Accuracy, full / game-only | 85.60% / 84.21% | 86.18% / 84.81% |
| Annotated exact functions (full) | 1,626 | 1,654 |

| Address | Function, before | before | Function, after | Bytes | after |
| --- | --- | ---: | --- | ---: | ---: |
| `0047e460` | `ledger_stub_0047e460_ComObjectDestroyAndOptionallyFree` | 50.00 | ``AviSurface::`scalar deleting destructor'`` | 32 | **100.00** |
| `0047e480` | `ledger_stub_0047e480_TwoInterfaceFieldsReleaseAndReset` | 38.10 | `AviSurface::~AviSurface` | 64 | 94.74 |
| `0047e4c0` | `ledger_stub_0047e4c0_AviPlaybackStateInitialize` | 14.81 | `AviSurface::AviSurface` | 80 | **100.00** |
| `0047e510` | `AviPlaybackState::display_mode_setup` | 7.69 | `AviSurface::Init` | 512 | **100.00** |
| `0047e710` | `surface_descriptor_initialize` | 31.82 | `AviSurface::InitDesc` | 48 | **100.00** |
| `0047e740` | — | unmapped | `AviSurface::Width` | 16 | **100.00** |
| `0047e750` | — | unmapped | `AviSurface::Height` | 16 | **100.00** |
| `0047e760` | `AviPlaybackState::surface_lock` | 23.26 | `AviSurface::Lock` | 80 | **100.00** |
| `0047e7b0` | `AviPlaybackState::surface_unlock` | 25.40 | `AviSurface::Unlock` | 48 | **100.00** |
| `0047e7e0` | `bit_count32_by_position` | 100.00 | `AviSurface::CountMaskBits` | 32 | **100.00** |
| `0047e800` | `AviPlaybackState::frame_retriever_open` | 15.19 | `AviMovie::AviMovie` | 384 | **100.00** |
| `0047e980` | `AviPlaybackState::player_destroy` | 51.61 | ``AviMovie::`scalar deleting destructor'`` | 32 | **100.00** |
| `0047e9a0` | `AviPlaybackState::frame_retriever_destroy` | 11.76 | `AviMovie::~AviMovie` | 64 | **100.00** |
| `0047e9e0` | `AviPlaybackState::frame_decode_and_copy` | 17.24 | `AviMovie::FrameToSurface` | 320 | 59.81 |
| `0047eb20` | `AviPlaybackState::frame_update_and_blit` | 22.22 | `AviMovie::Update` | 192 | 96.49 |
| `0047ebe0` | `AviPlaybackState::stream_frame_count_get` | 70.00 | `AviMovie::GetLength` | 16 | **100.00** |
| `0047ebf0` | `AviPlaybackState::frame_seek_and_clock_rebase` | 40.62 | `AviMovie::SetTime` | 112 | **100.00** |
| `0047ec60` | `AviPlaybackState::playback_frame_index_get` | 23.53 | `AviMovie::GetTime` | 32 | **100.00** |
| `0047ec80` | `avi_file_initialize` | 36.36 | `avi_initialise` | 16 | **100.00** |
| `0047ec90` | `avi_player_create` | 8.51 | `avi_load` | 96 | **100.00** |
| `0047ecf0` | `ledger_stub_0047ecf0_AviFrameUpdate` | 50.00 | `avi_update` | 16 | **100.00** |
| `0047ed00` | `avi_playback_frame_loop` | 20.34 | `avi_blit_to_back_buffer` | 64 | **100.00** |
| `0047ed40` | `ledger_stub_0047ed40_AviFrameIndexAndClockReset` | 80.00 | `avi_set_time` | 16 | **100.00** |
| `0047ed50` | `ledger_stub_0047ed50_AviCurrentFrameIndexGet` | 100.00 | `avi_get_time` | 16 | **100.00** |
| `0047ed60` | `ledger_stub_0047ed60_AviStreamFrameCountGet` | 100.00 | `avi_get_length` | 16 | **100.00** |
| `0047ed70` | `lego::interface_invoke_one_if_nonnull` | 75.00 | `avi_remove` | 16 | **100.00** |
| `0047ed80` | `avi_player_ready_get` | 100.00 | `avi_is_ok` | 16 | **100.00** |

Bytes are each function's span up to the next function, padding included.
The "before" figure counts the two unmapped functions as 0.

Also newly mapped at 100%:
- the two vtables, through `// VTABLE` annotations on the classes;
- the six AVIFIL32 import thunks (see [Imports](#imports-finding-574)).

## Unit extent

The map gave eight functions (`0047ec90`–`0047ed80`) low confidence and
one (`0047e460`) medium. All 25 belong to the unit, and two more are
added. No function moves to another unit.

- **Missing getters.** The vtable at `0049e9a0` has three slots: `0047e460`
  (the scalar deleting destructor), `0047e740` and `0047e750`. These are
  `mov eax, [ecx+0x28]; ret` and `mov eax, [ecx+0x24]; ret`, the surface
  description's width and height. Each address occurs only once in the
  image, in that vtable. Ghidra left them unlabelled, so the corpus
  manifest and the unit map generated from it do not list them, as with
  finding 596's callbacks. Reccmp maps them through their annotations.
- **Call graph.** An `E8` scan of `.text` finds the unit entered from
  outside only by:
  - the credits loop at `0040a0c0`, which calls the eight wrappers
    `0047ec90`, `0047ecf0`, `0047ed00`, `0047ed40`, `0047ed50`, `0047ed60`,
    `0047ed70` and `0047ed80`;
  - WinMain at `00477a22`, which calls `0047ec80`.

  The methods are reached only from each other and through the two
  vtables.
- **Callees.** The 27 functions call only each other and:
  - the back-buffer getter `004065e0`;
  - the global operator delete `0045daf0` and operator new `0048ead0`;
  - Files.c's `00480220`;
  - `__ftol` (`0048e060`);
  - the six AVIFIL32 thunks `0049b6ce`–`0049b6ec`;
  - `timeGetTime` through its import slot.
- **Data.**
  - `.bss` `00533998` (a byte) and `0053399c` (the IDirectDraw4 pointer)
    are referenced only from the unit. `005339a0` starts Files.c's
    `fileGlobs`.
  - `.rdata` `0049e9a0`–`0049e9b4` holds the two vtables and `0.001f`
    (`0049e9ac`), referenced only from `0047e800`, `0047eb20` and
    `0047ebf0`. The words from `0049e9b4` have no code reference.
- **EH.** The unwind funclets `0049ddc0`–`0049ddf4` at the end of `.text`
  belong to `0047e800` and `0047ec90`.
- **Neighbours.** `0047e440` (it calls DirectDraw_SaveBMP) stays Images.c's
  last function. `0047ed90` starts Input.c (its `__FILE__`).

`translation-units.spec.csv` records this evidence in G98-012's note. The
computed boundary confidence stays `medium`, because padding cannot separate
objects (finding 505).

## The unit

### State and layout

- **`.bss`**: `s_aviDisabled` (`00533998`, `bool`) and `s_aviDirectDraw`
  (`0053399c`), both `static` and annotated.
  - Nothing in the image writes `00533998`, so it is always false.
  - The port's `s_avi_playback_disabled`, which `movie.cpp` writes, stays
    in the tail, unannotated.
- **Classes**: the field offsets are the ones the code uses. Words the code
  only clears or never touches are kept for the layout:
  - AviSurface: `+0x04`, and the bytes `+0x14` and `+0x15`;
  - AviMovie: the byte `+0x98` and the word `+0xbc`.
- **`0.001f`** is a plain literal.

`make unit-data-offsets`, G98-012 row:

| Unit | .data | .rdata | .bss | Commons |
| --- | --- | --- | --- | --- |
| G98-012 | n/a (0, 0 unknown) fewer than two attributable items | n/a (0, 0 unknown) fewer than two attributable items | gaps-only (2, 0 unknown) | - |

Unit summary: full 0, gaps-only 9, partial 23, n/a 195.

### No `vfw.h`

MSVC 5.0's `vfw.h` defines `static DWORD dwICValue;`, which adds a word to
the unit's `.bss`. With it the unit's `.bss` was 12 bytes, against the
original's 8 (`00533998`–`005339a0`). That moved `fileGlobs` by 8, so
Sound3D's `sound_bank_free_slot_find` (`0047aad0`) fell from 100% to
92.86%: its loop bound is a constant that the original names `fileGlobs+8`.

The unit therefore declares the part of the AVIFile API it uses:
- `AVISTREAMINFOA`, 0x8c bytes;
- the six functions, `__stdcall` and `extern "C"`, without `dllimport`.

The rebuilt object's `.bss` is now 8 bytes, and `0047aad0` stays exact.

### DirectX 6 interfaces

MSVC 5.0's `ddraw.h` is DirectX 3's, as finding 596 notes. The unit declares:
- IDirectDraw4 up to CreateSurface;
- IDirectDrawPalette up to Release;
- the full IDirectDrawSurface4;
- DDSURFACEDESC2 and DDPIXELFORMAT.

The interfaces are C++ abstract structs with `__stdcall` virtuals, so the
calls compile to the original's `mov ecx, [eax]` … `call [ecx+slot]` shape.

### Imports (finding 574)

The original calls the six AVIFIL32 exports directly (`call 0049b6ce` …
`0049b6ec`). The toolchain's `vfw32.lib` already holds AVIFIL32's import
members (it is in the library list since finding 574), so no `.def` is
needed.

`crt.cpp` gains six `// LIBRARY: … SYMBOL` markers for the thunks, as
finding 596 added DDRAW's. This also moves `crt.cpp`'s legacy key from
`00494c70` to the new median `00494d10`; its list position is unchanged.

`make pe-audit` on the gate build:
- **AVIFIL32.dll is imported.** The DLLs go from 9 to 10, in the original's
  descriptor order (`KERNEL32, d3drm, DDRAW, WINMM, AVIFIL32, DINPUT,
  DSOUND, USER32, GDI32, ADVAPI32`; `common order equal True`). The only
  DLL still missing is `ole32`.
- **The six thunks are in the original's order**: `AVIStreamInfoA`,
  `AVIStreamGetFrameOpen`, `AVIStreamOpenFromFileA`, `AVIStreamRelease`,
  `AVIStreamGetFrame`, `AVIFileInit`, after the DDRAW thunks.
- **Import thunks** in the gate build go from 157 to 163.
- **The `/OPT:REF` relink drops them again.** The unit has no live caller,
  so its thunks go with it, and the relink's DLL list is unchanged.

The CMake winapi link (MinGW reference build) gains `vfw32`. No MinGW
toolchain was available here to build it.

### Operator new and delete

In the original:
- the new-expressions call `0048ead0`, LIBC's `operator new`;
- the scalar deleting destructors and the EH funclets call `0045daf0`, the
  project's own `operator delete` (a `free` wrapper in GAME-036's object).

In the matching build:
- `0048ead0` is annotated on `engine_calloc` (`crt_heap.cpp`);
- `0045daf0` is annotated on `ui_render_record_free` (`map_view.cpp`);
- the global operators are LIBC's.

Each class therefore has inline class-scope `operator new` and `operator
delete` that forward to those two functions. They compile to the original's
direct calls and are marked `Deviation (scoring only)` and `PORT-ONLY` (the
lint exemption). `heap.hpp` declares both functions, and `aitask.h` declares
the back-buffer getter `render_surface_secondary_get` (`004065e0`).

### Callers and the port tail

The original's callers are the credits loop (`0040a0c0`, in
`screen_effect_quads.cpp`) and WinMain (`004777a0`). Both are still the
port's:
- the credits loop plays its backdrop through `movie.cpp`'s
  `movie_open_path`/`movie_next`;
- WinMain does not call the AVI player.

No port caller has the original's contract, so none is switched.

`avi_player_port.cpp` keeps the port's code under its own names, unannotated:
- the `AviPlaybackState` methods;
- `avi_file_initialize`, `avi_player_create` and `avi_playback_frame_loop`;
- `surface_descriptor_initialize`;
- `bit_count32_by_position` (`test_crt`);
- `lego::interface_invoke_one_if_nonnull`;
- `avi_player_ready_get` (`test_record`).

The seven ledger stubs that only held annotations
(`ledger_stub_0047e460` … `0047ed60`) are deleted, with their forward
declaration. `movie.cpp`'s unannotated stubs are untouched. No frozen
`_for_`/`_from_` header is edited.

**Native movie playback is unchanged.**
- On native the unit compiles to nothing (`#if
  !defined(LEGORRR_BACKEND_MINIWIN)`, as `directdraw.c` does).
- `movie.cpp` keeps decoding through FFmpeg (`MOVIES=auto|enabled|disabled`).
- The real-data `movie` test decodes `intro.avi` (320×200, 15 fps, 3,821
  frames).

## Shape notes

- **`AviMovie::AviMovie`**:
  - `m_init`, `m_ok` and `m_surf` are in the mem-initializer list, which
    puts the vptr store after them, as in the original.
  - The rectangle is cleared top, bottom, left, right.
  - It is copied field by field after `m_startTime`, `m_ok` and
    `m_currFrame`. A search over the 72 orders took it from 83.96% to
    99.06% in the single-function loop and 100% in the full build.
- **`AviSurface::Init`**:
  - The caps are written `dwCaps = OFFSCREENPLAIN;` then `if (vram) dwCaps
    |= VIDEOMEMORY; else dwCaps |= SYSTEMMEMORY;`. The ternary forms
    eliminate the first store and stop folding `0x840`.
  - The last 1.24% is the order of the width/height flag computation. It is
    a period-16 phase of MSVC 5.0's symbol counter (finding 538 rule B),
    and no expression order changes it.
- **`AviMovie::Update`**:
  - `timeGetTime()` goes into a local first.
  - `float rate = (float)dwRate, scale = (float)dwScale;` gives the
    original's two `fild qword` and `fdivp`. A direct `(float)a /
    (float)b` gives `fidiv dword`.
- **`AviMovie::GetTime`**: `unsigned long frame = m_currFrame; if
  (s_aviDisabled) return 0; return frame;` compiles to the original's
  `neg/sbb/not/and`. `?:` and the plain `if` give a branch. Mask forms give
  the sbb sequence in other registers or with a `dec`.
- **`AviSurface::CountMaskBits`** is a `thiscall` member (`ret 4`, `this`
  unused). `InitDesc` takes the constructor's unused fifth argument, as
  the original passes it.
- The scalar deleting destructors are compiler-generated. They carry
  name-reference annotations (`// FUNCTION:` followed by
  ``// AviSurface::`scalar deleting destructor'``).

### `Deviation (scoring only)`

There are two, and neither changes storage:
- the class-scope operators above;
- the full IDirectDrawSurface4 table. The unit calls nothing after
  `Unlock` (slot 32), and the other two interfaces stop at their last
  called slot. The 12 declarations after it put `AviSurface::Init`
  (512 bytes) in the right counter phase: 98.76% without them, 100% with
  them. Completing IDirectDraw4 as well moves it to 94.15%. These are the
  interface's real methods, not inert declarations.

## Near misses

| Address | Score | What is left |
| --- | ---: | --- |
| `0x0047e9e0` FrameToSurface | 59.81 | Register allocation in the copy loops. The original keeps the row count in `eax` and spills two separate row strides (`[esp+0x10]` and `[esp+0x14]`, both `pitch*2`). The rebuild keeps one shared stride in `eax` and spills the count. It also spills `m_is555` between the two depth checks, where the original reloads `[esi+0x16]`. Tried: pointer types (`WORD*`/`char*`, mixed), explicit `*2` and `sizeof` strides, indexed `memcpy` forms, `else if`/`\|\|`/nested forms, `surf` versus `m_surf` in the checks, inline accessors, 40 declaration orders, counter offsets 0–2,048 (59.81 or 60.75). `msvc_permuter.py` (400 s) found no candidates; its passes do not apply to this C++ |
| `0x0047eb20` Update | 96.49 | x87 scheduling: the original stores `m_currTime` before `fmul st(1)`, and the rebuild multiplies first. Five statement forms (locals, a separate `elapsed`, operand order) give the same code |
| `0x0047e480` ~AviSurface | 94.74 | The rebuild hoists the `m_surf` load above the vptr store. `!= NULL`, a local copy, comma forms and unconditional clears do not move it. The single-function loop scores it as an effective match |

No naked asm is in the unit.

## Entropy

`make reccmp-check` against the baseline at #658's head (`ef9487e0`) passes. In the full
report:
- 21 functions improved and 10 entities are newly mapped: the two getters,
  the two vtables and the six thunks;
- no function fell, and no exact match was lost;
- the link-order map, `winapi-link-check` and the plain-immediate check are
  OK.

The only listing digests that changed are the unit's 27 functions, the six
thunks and the two vtables, so there are no instruction-count changes
elsewhere.

The unit's GetTime briefly fell to 15.38% (from the port's 23.53%) when
`vfw.h` was dropped. The form above restores it to 100%.

Other file edits:
- `crt.cpp`: comments only (the six thunk markers);
- `CMakeLists.txt`: `vfw32` in the winapi link.

No declaration was added to or removed from a shared header. No file owned
by #648, #650, #651 or #652 is edited.

## `/OPT:REF` (finding 579)

| | before (#658 `ef9487e0`) | after |
| --- | ---: | ---: |
| `opt_ref.annotated.full`: removed / exact removed | 471 / 221 | 477 / 246 |
| `opt_ref.annotated.game_only`: removed / exact removed | 452 / 208 | 452 / 227 |
| Exact annotated functions (full) | 1,626 | 1,654 |

`make opt-ref-callers OPT_REF_ARGS="--object avi_player.cpp.obj"` reports
27 removed, 24 of them exact. They sit under two roots, both port code:
- the credits loop (`0040a0c0`), which calls `movie.cpp` instead;
- WinMain (`004777a0`), whose port calls `winmain_application_run`.

The methods and getters are reached only from the wrappers and the vtables.
The six thunks go with them. The count falls when the credits loop's unit is
converted and calls `avi_load` and the other wrappers.

## Reccmp lint ratchet

1,227 → 1,222 entries (−5, no growth). The port's five static helpers
(`avi_clock_milliseconds`, `movie_read_u16le`, `movie_read_u32le`,
`movie_surface_raw_dib_compatible`, `movie_surface_indexed_dib_copy`) are now
in a tail file.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox): OK; link-order map
  OK.
- `make match-progress`: the three generated files.
- `make unit-data-offsets`: the G98-012 row above.
- `python3 tools/translation_units.py build`: 0 conflicts. The thunks are
  now attributed to `crt.cpp`. `tools/binary_progress_inputs.txt` is
  regenerated (`avi_player.h`, `avi_player_port.cpp`).
- `make tools-test`: OK. That covers lint (the ledger shrinks), link order,
  the winapi list, Meson parity and the tool tests.
- `make winapi-link-check`: OK.
- `make pe-audit`: as in [Imports](#imports-finding-574).
- Native: `cmake -S . -B build -DTEST_WADS_DIR=<shipped WADs>
  -DMOVIES=auto && cmake --build build && ctest --test-dir build`: 39/39,
  including `movie` (FFmpeg present; `intro.avi` decodes), `record`, `crt`
  and `c_boundaries`.
- The MinGW winapi reference build was not built (no toolchain here).

## Reproducing

    git mv src/gods98/avi_player.cpp src/gods98/avi_player_port.cpp  # tail entry
    python3 tools/convert_unit.py G98-012 --stem avi_player --out <scratch>  # report only; the unit is C++
    make match-function SOURCE=src/gods98/avi_player.cpp
    make reccmp-check
    make match-progress
    make unit-data-offsets
    make opt-ref-link opt-ref
    make opt-ref-callers OPT_REF_ARGS="--object avi_player.cpp.obj"
    make pe-audit
