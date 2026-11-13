# Finding 596 — DirectDraw unit (G98-010) rebuilt in original shape

Issue #629, under [ADR 0005](../adr/0005-original-shape-c-units.md). #540
relocated G98-010 verbatim into `src/gods98/directdraw.cpp`. Those functions
were port models of the display layer: hook tables, the port's renderer state
and a save-slot thumbnail writer. Two of the seventeen mapped functions had no
source, and three more functions were missing from the map. This change
rebuilds the unit as the C unit
[`src/gods98/directdraw.c`](../../src/gods98/directdraw.c), with the C
boundary headers [`directdraw.h`](../../src/gods98/directdraw.h) and
[`directdraw_imports.h`](../../src/gods98/directdraw_imports.h). The port's
display layer moves, unannotated, to the port tail
[`src/gods98/directdraw_port.cpp`](../../src/gods98/directdraw_port.cpp), and
the native port keeps running it. The unit is text `0047c170`–`0047d410`:
gods98's DirectDraw.c.

- **Enumeration.** DirectDraw_EnumDrivers runs DirectDrawEnumerateA. The
  three callbacks fill 0x114-byte driver and device records and 0x110-byte
  mode records. EnumDevices and EnumModes create a DirectDraw object, query
  IDirectDraw4 (and IDirect3D3 for devices), and enumerate.
- **Display.** DirectDraw_Setup creates the IDirectDraw4 object, sets the
  cooperative level and, in full screen, the display mode. It then creates
  the front buffer and the back buffer, the clippers (DirectDraw_CreateClipper)
  and Direct3D (Main.c's `0x004781d0`).
- **Frame.** DirectDraw_Flip flips the buffers in full screen and otherwise
  blits them (BlitBuffers). ReturnFrontBuffer copies the front buffer back,
  Clear fills the back buffer, and Shutdown releases the objects.
- **Surfaces.** SaveBMP writes a surface as a 24-bit BMP. Blt8To16 expands a
  palettised surface into a 16-bit one. GetColour packs an RGB value for a
  surface, and CountMaskBits counts a mask's bits.

"Before" is the *match baseline* at #634's head (`95a7c989`, stack #636:
#630 Viewports.c, #631 LightEffects.c, #633 HelpWindow.c, #634 ToolTip.c on
`main` `12a0e950`). "After" is this change, rebased onto it as the stack's
top layer.

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (20 functions, weighted by function span, 4,768 bytes) | 13.99% | **90.11%** |
| Exact functions in the unit | 3 | **18 of 20** |
| Mapped functions in the unit | 15 | 20 |
| Exact-function code bytes (`binary-progress.json`) | 176,586 (27.48%) | **178,866** (27.84%) |
| Accuracy, full / game-only | 82.90% / 81.24% | 83.30% / 81.67% |

| Address | gods98 name | Function, before | before | Function, after | after |
| --- | --- | --- | ---: | --- | ---: |
| `0047c170` | DirectDraw_Initialise | `window_state_initialize_retail` | 100.00 | `directdraw_initialise` | **100.00** |
| `0047c1c0` | DirectDraw_EnumDrivers | `directdraw_enumerate_and_store_result` | 9.30 | `directdraw_enum_drivers` | **100.00** |
| `0047c1f0` | DirectDraw_EnumDriverCallback | — | unmapped | `directdraw_enum_driver_callback` | **100.00** |
| `0047c2e0` | DirectDraw_EnumDevices | `directdraw_device_create` | 8.33 | `directdraw_enum_devices` | **100.00** |
| `0047c380` | DirectDraw_EnumDeviceCallback | — | unmapped | `directdraw_enum_device_callback` | **100.00** |
| `0047c4b0` | DirectDraw_EnumModes | `directdraw_enumerated_device_create` | 29.79 | `directdraw_enum_modes` | **100.00** |
| `0047c550` | DirectDraw_EnumModeCallback | — | unmapped | `directdraw_enum_mode_callback` | **100.00** |
| `0047c610` | DirectDraw_Setup | `directdraw_renderer_init` | 10.27 | `directdraw_setup` | **100.00** |
| `0047c8d0` | DirectDraw_Flip | `render_surface_present` | 100.00 | `directdraw_flip` | **100.00** |
| `0047c8f0` | DirectDraw_SaveBMP | `save_write_thumbnail` | 11.70 | `directdraw_save_bmp` | 71.49 |
| `0047cc20` | DirectDraw_ReturnFrontBuffer | — (no source) | unmapped | `directdraw_return_front_buffer` | **100.00** |
| `0047cc50` | DirectDraw_BlitBuffers | `windowed_render_surface_blit_retail` | 88.64 | `directdraw_blit_buffers` | **100.00** |
| `0047ccf0` | DirectDraw_Shutdown | `directdraw_objects_release` | 8.62 | `directdraw_shutdown` | **100.00** |
| `0047cd50` | DirectDraw_AdjustTextureUsage | `query_texture_memory` | 10.91 | `directdraw_adjust_texture_usage` | **100.00** |
| `0047cdd0` | DirectDraw_GetAvailTextureMem | `surface_description_get` | 27.27 | `directdraw_get_avail_texture_mem` | **100.00** |
| `0047ce20` | DirectDraw_Clear | `projection_overlay_draw_submit` | 21.43 | `directdraw_clear` | **100.00** |
| `0047cee0` | DirectDraw_CreateClipper | `directdraw_clipper_set_region` | 12.56 | `directdraw_create_clipper` | **100.00** |
| `0047d000` | DirectDraw_Blt8To16 | — (no source) | unmapped | `directdraw_blt_8_to_16` | 66.82 |
| `0047d2d0` | DirectDraw_GetColour | `surface_pixel_value_encode` | 9.23 | `directdraw_get_colour` | **100.00** |
| `0047d3f0` | DirectDraw_CountMaskBits | `bit_count32` | 100.00 | `directdraw_count_mask_bits` | **100.00** |

The binary does not attest the gods98 names. The weights are the functions'
spans up to the next function, padding included. The "before" figure counts
the five unmapped functions as 0.

Outside the unit, three callers rise and two import thunks are newly mapped:

| Address | Function | before | after | Why |
| --- | --- | ---: | ---: | --- |
| `0041fb50` | `lego_game_config_init` | 93.77 | 93.80 | calls GetAvailTextureMem and AdjustTextureUsage, not adapters |
| `00424d00` | `lego_engine_shutdown` | 90.07 | 90.44 | calls DirectDraw_Clear, not an adapter |
| `0042efc0` | `lego_play_movie` | 95.00 | 96.25 | calls ReturnFrontBuffer, not a no-op adapter |
| `0049b6c2` | `_DirectDrawEnumerateA@8` (thunk) | unmapped | 100.00 | `LIBRARY ... SYMBOL` marker in `crt.cpp` |
| `0049b6c8` | `_DirectDrawCreate@12` (thunk) | unmapped | 100.00 | as above |

The thunk markers also let Reccmp name the unit's direct calls into DDRAW.DLL.
Without them, EnumDrivers, EnumDevices and Setup stay below 100%, because the
original's call operand stays a placeholder (as `audio_system_init`'s
DirectSoundCreate call still does, at 99.25%).

## Unit extent

The map gave all 17 functions low confidence, and two had no current source.
The extent is confirmed. No function moves to another unit, and three are
added:

- **Callbacks.** `0047c1f0`, `0047c380` and `0047c550` are the
  DirectDrawEnumerateA, IDirect3D3::EnumDevices and
  IDirectDraw4::EnumDisplayModes callbacks. Each address occurs exactly once
  in the image, as the `push` immediate in `0047c1c0`, `0047c2e0` and
  `0047c4b0`. Each has its own prologue after the previous function's `ret`
  and padding, and each returns with `ret 0x10`, `ret 0x18` and `ret 8`, the
  stdcall callback shapes. Ghidra left them as labels, so the corpus manifest
  (`docs/decompiles/full/manifest.csv`) and the unit map generated from it do
  not list them. Reccmp maps them through their annotations.
- **Call graph.** The 20 functions call each other, Main.c (`00477f80`,
  `004780b0`, `004781d0`), Files.c (`0047f6e0`, `0047fa50`, `0047fac0`),
  LIBC (`_sprintf`, `_malloc`, `_free`, `__ftol`), USER32/GDI32 and DDRAW.DLL.
  They are the only callers of the DDRAW thunks `0049b6c2` and `0049b6c8`.
  Their outside callers are Lego.c (`0041fb50`, `00424d00`, `0042efc0`),
  `0044e0a0`, `0045dbc0`, Container.c (`00474050`), Main.c (`004777a0`,
  `00477f30`), Images.c (`0047e440`) and the start-up dialog (`0049d030`,
  `0049d630`, `0049d780`).
- **Commons.** A scan of `.text` for operands in `0076ac80`–`0076acc8` finds
  writes only from the unit. The 17 outside references are all reads of
  `0076ac84` (the IDirectDraw4 object) and `0076ac8c` (the back buffer), from
  AITask.c's two inline accessors, Container.c, Main.c, Images.c, Flic.c and
  Draw.c. The next referenced common, `0076ace0`, belongs to code at
  `00476d58`. Words `+0x20` and `+0x28` of the block have no reference.
- **Strings.** `"%ix%i"` (`004aae64`) and `"%ix%i (%i bit)"` (`004aae6c`) are
  referenced only from `0047c550`. They follow Sound3D's `"%s.wav"`
  (`004aae5c`, referenced from `0047a8b0`) and come before Images.c's
  `"Data\%s"` (`004aae7c`). `"%s (%s)"` (`004a3544`) is pooled with GAME-021,
  and `"wb"` (`004a2794`) with GAME-017; both are marked there.
- **Neighbours.** `0047c160` (`return -10000`, DSBVOLUME_MIN) stays Sound3D's
  last function. `0047d410` clears Images.c's `.bss` at `00533908`.

`translation-units.spec.csv` records this evidence in G98-010's note and names
the unit `DirectDraw.c` (inferred: it is the unit that calls DDRAW.DLL).
`tools/translation_units.py` computes the boundary confidence, and it stays
`low` (finding 505).

## The unit

### State

- **`directDrawGlobs`** (`0076ac80`, 0x48 bytes): one file-scope definition of
  `DirectDraw_Globs`, so it is a common, as in the original. Its fields are the
  window, the IDirectDraw4 object, the front, back and z buffers, the two
  clippers, the three enumeration lists and counts, the full-screen flag and
  the size. The fields at `+0x20` and `+0x28` are never referenced and are
  kept for the layout.
- **IIDs**: `IID_IDirectDraw4` (`0049f328`) and `IID_IDirect3D3` (`0049f648`)
  are `static const` annotated copies, as `sound3d.c` and `lws.c` define
  theirs. The original links them from dxguid.lib. MSVC 5.0's SDK has
  neither, and `cross/msvc5-importlibs/dxguid.c` could provide them in a
  follow-up.
- **COM layouts**: MSVC 5.0's `ddraw.h` is DirectX 3's. The unit declares the
  DirectX 6 layouts it uses (IDirectDraw, IDirectDraw4, IDirectDrawSurface4,
  IDirectDrawClipper, IDirectDrawPalette, IDirect3D3, the IDirect3DRMDevice3
  slot it calls, DDSURFACEDESC2, DDPIXELFORMAT, DDSCAPS2, DDBLTFX,
  D3DDEVICEDESC). Calls go through `p->lpVtbl->Method(p, …)`, the shape the
  original's C macros expand to.
- **Main.c's device** (`00505918`, `lpDevice()`): the stand-in
  `s_main_device` that Viewports.c also reads. #630 defines and annotates it
  in `viewports_port.cpp`; this unit declares it in `directdraw_imports.h`.

The old port storage loses its annotations. These are 11 `crt.cpp` words
(`s_renderer_global_dword`, `s_renderer_primary_surface`,
`s_render_surface_secondary`, `s_renderer_window_state_*`) and four
`surface.cpp` words (`s_renderer_window_handle`, `s_renderer_fullscreen`,
`s_renderer_surface_width`, `s_renderer_surface_height`). Removing only the
comment lines leaves both files' code unchanged.

`make unit-data-offsets`, G98-010 row:

| Unit | .data | .rdata | .bss | Commons |
| --- | --- | --- | --- | --- |
| G98-010 | n/a (0, 0 unknown) fewer than two attributable items | n/a (0, 0 unknown) fewer than two attributable items | n/a (0, 0 unknown) fewer than two attributable items | `0076ac80` `_directDrawGlobs` (72) |

### Imports (finding 574)

The original calls DirectDrawCreate and DirectDrawEnumerateA directly
(`call 0049b6c8`, `call 0049b6c2`), through the import thunks. The unit
declares both without `dllimport`, so MSVC 5.0 emits the same `E8` calls, and
`ddraw.lib` is already in the library list. `make pe-audit` on the gate build
now shows:

- `ddraw.dll` imported, by name, with two thunks. The DLLs go from 8 to 9, in
  the original's descriptor order (`common order equal True`). The DLLs
  missing are now `ole32` and `avifil32`.
- In the `/OPT:REF` relink, the DDRAW thunks are dropped again. The only
  functions that call them (EnumDrivers, EnumDevices, EnumModes, Setup) have
  no live caller, because the start-up dialog that calls them in the original
  is still the port's (`0049d030`). The relink's DLL list is unchanged.

The CMake winapi link (MinGW reference build) gains `ddraw`, because the unit
now references DirectDrawCreate there too. No MinGW toolchain was available to
build it; see [Validation](#validation).

### Port names and callers

The port's display layer keeps its names and contracts and moves unannotated
to `directdraw_port.cpp` (`git mv`, a `tail` link-order entry and a Meson
source). The 15 port functions that carried the unit's annotations lose them.
Among them are `window_state_initialize_retail`, `directdraw_renderer_init`,
`save_write_thumbnail` (the save-slot thumbnail, which native savegames and
`test_savegame` use), `query_texture_memory`, `surface_description_get`,
`projection_overlay_draw_submit` and `surface_pixel_value_encode`, plus the
enumeration and clipper hook models. The two that lose their callers are
deleted (below).

Callers whose contract matches the original's now call the unit:

| Caller | Before | After |
| --- | --- | --- |
| `lego_unit.c` `0041fb50` | `surface_description_get`, `query_texture_memory_original` (adapter) | `directdraw_get_avail_texture_mem`, `directdraw_adjust_texture_usage` |
| `lego_unit.c` `00424d00` | `projection_overlay_draw_submit_adapter(0, 0)` | `directdraw_clear(NULL, 0)` |
| `lego_unit.c` `0042efc0` | `lego_render_surface_restore_adapter()` (no-op, `lego_port.cpp`) | `directdraw_return_front_buffer()` |
| `frameloop.cpp` `00477f30` (Windows builds) | `render_surface_present` | `directdraw_flip` |
| `surface.cpp` `surface_winapi_present_rgba` (port) | `render_surface_present` | `directdraw_flip` |

`lego_unit_imports.h` and `surface.hpp` swap one declaration for each one they
drop, with the same number of named parameters, so the symbol numbering does
not move (finding 538, rule B). Deleted, because nothing calls them any more:

- the adapters `projection_overlay_draw_submit_adapter`,
  `query_texture_memory_original` and `lego_render_surface_restore_adapter`;
- the port's `render_surface_present` and
  `windowed_render_surface_blit_retail`.

`frameloop.cpp`'s `00477f80` (Main_GetWindowsBitDepth, which EnumModeCallback
calls) now returns `GetDeviceCaps(dc, BITSPIXEL)` instead of discarding it.
`surface.hpp` gives Main.c's `004780b0` and `004781d0` `extern "C"` in place.

**The readers of the commons.** Draw.c's five back-buffer functions and
AITask.c's two accessors were exact through the old `crt.cpp` annotations.
They now read `directDrawGlobs` in the Windows builds:

- `crt.h` declares it as `extern uint32_t directDrawGlobs[];`, with
  `s_render_surface_secondary` as a macro for word 3.
- `aitask_imports.h` declares a four-word struct view of it, with the two
  port names as macros for its fields.

Each replaces the declarations it supersedes one for one. The view types are
not the unit's, so both carry `Deviation (scoring only)` (declaration only;
storage is unchanged). Before and after, under `make match-function`:

- `draw.c`: 5 changed (the five back-buffer functions, back to 100%);
- `aitask.c`: 2 changed (`0x004065d0`, `0x004065e0`, back to 100%);
- the other `crt.h` includers (`lws.c`, `advisor.c`, `interface_unit.c`,
  `reward.c`): 0 changed.

Natively, both headers keep the port's words.

**Callers left as they are** (class (c), finding 579):

- the start-up dialog `0049d030`/`0049d630`/`0049d780`, the port's device
  selection;
- WinMain `004777a0`, which runs `winmain_application_run`;
- Container.c `00474050` (`render_object_state_apply_to_entries`);
- Images.c `0047e440`, a ledger stub;
- `0044e0a0` and `0045dbc0`, both unmapped.

`main_loop_frame_pump` (`00477f30`) also calls DirectDraw_Clear in the
original. The rebuild calls an uninstalled hook there, so switching it would
start clearing the back buffer each frame in the Windows build. It is left
as it is.

**Main_SetupDirect3D.** The port's model at `004781d0` takes an adapter
record, its flags, a surface and a single-mode flag; the original takes the
device, the IDirectDraw object, the back buffer and the full-screen flag.
`directdraw_setup` passes the original's arguments (`directdraw_imports.h`),
as ADR 0005 step 6 allows. Nothing runs `directdraw_setup` in any build.

### Runtime

- **Native**: unchanged. The unit compiles to nothing natively (it has no
  DirectDraw), and the port's display layer runs as before.
  `directdraw_port.cpp` gives the four entry points `lego_unit.c` calls
  native bodies with the replaced adapters' behaviour. ReturnFrontBuffer
  does nothing, as its adapter did.
- **Windows builds**: the port renderer still sets up the display
  (`directdraw_renderer_init_winapi`). After it commits, it publishes the
  window, the IDirectDraw4 object, both buffers, the mode and its
  IDirect3DRMDevice3 into `directDrawGlobs` and `s_main_device`
  (`directdraw_port_publish`), and its `window_state_initialize_retail`
  also runs `directdraw_initialise`. The unit's Flip, BlitBuffers,
  ReturnFrontBuffer and Clear, and Draw.c, therefore act on the display the
  port renderer made. The port's release clears its own words but not
  `directDrawGlobs`, as DirectDraw_Shutdown does not clear it either.

## Shape notes

- **`0047c1f0`** indexes `driverList[driverCount]` in every statement; the
  device and mode callbacks take a local pointer once. Both shapes are the
  original's.
- **`0047c4b0`**: `guid` is assigned by an `if`/`else` before the call. A `?:`
  inside the call argument list (as `0047c610` has it) schedules
  `lea &lpDD` before the guid and costs 27 points (73.04%). That was not a
  counter-phase effect: a sweep of 0–4,100 inert declarations left it
  unchanged.
- **`0047c550`** reads `*(BOOL*)lpContext` at each use; a local copy moves it
  into a register (89.76%).
- **`0047cc50`**: the source rectangle is assigned left, top, right, bottom.
  This order was found by trying all 24 orders; the relocated port's order
  scored 88.64%.
- **`0047cdd0`**: `*total = 0; *avail = 0;` as two statements, not a chained
  assignment.
- **`0047cee0`**: the full-screen test is the `else` of `if (!fullScreen)`,
  which puts the early return after the window-mode path. The original frees
  the region data twice when a clipper call fails after SetClipList, and
  that is kept.
- **`0047c8f0`** (71.49%): the row size is computed in two statements, which
  matches the original's `lea`/`sbb` sequence. The remaining differences are
  register allocation. The original keeps `success` and the row's byte index
  in `esi`, and uses `esi` as the zero register. 40 declaration orders, three
  index shapes and five pointer associations did not reproduce it, and
  `tools/msvc_permuter.py` (300 s) found nothing.
- **`0047d000`** (66.82%): the source descriptor is locked first, then the
  target. The bit counts are `int`s, and the palette index and channels are
  bytes, as the listing's stack slots show. The original's frame is 8 bytes
  smaller, and its register assignment differs. A 0–64 inert-declaration
  sweep shows two phases, 66.82% and 68.72%, neither exact. Adding `guid` to
  `0047c4b0` moved this function from the second phase to the first. The
  permuter (300 s) found nothing.

No naked asm and no filler declarations are used. The two
`Deviation (scoring only)` declarations are in `crt.h` and `aitask_imports.h`
(above).

## Entropy

`make reccmp-check` against the baseline at #634's head (`95a7c989`) passes:

- 15 functions improved and 7 newly mapped;
- no function fell, and no exact match was lost;
- `winapi-link-check`, the link-order map and the plain-immediate check are
  OK.

There are no drops to label and no instruction-count changes outside the
unit. During the work, `main_loop_frame_pump` dropped from 60.47% to 55.81%
(drift, +0 instructions) while it still called the unannotated
`render_surface_present`. Switching it to `directdraw_flip` restored it.

Header and wiring edits:

- `extern "C"` in place: `surface.hpp` (two declarations) and
  `frameloop.hpp` (`00477f80`, now returning `int`);
- one-for-one declaration swaps: `lego_unit_imports.h`, `surface.hpp`,
  `crt.h`, `aitask_imports.h`;
- comment-only: the 15 storage annotations removed from `crt.cpp` and
  `surface.cpp`, and the two thunk markers added to `crt.cpp`;
- link order: `crt.cpp`'s legacy key moves from `00494c30` to `00494c70`,
  its new median with the markers. Its position is unchanged.

No file owned by #623 (`building_model.cpp`), #624 (`viewports.cpp`), #625
(`lighteffects.cpp`) or #628 (`helpwindow.cpp`) is edited.

**G98-003 (Viewports.c, #630), the neighbour below in the stack.** Neither unit
calls the other in the original's call graph, and Viewports.c does not
reference `0076ac80`–`0076acc8` (its commons start at `0076ace0`). There is no
call to switch in either direction. The two units share one piece of state,
Main.c's device `00505918`:

- #630 defines the stand-in `s_main_device` in `viewports_port.cpp`. Before the
  rebase this change defined it too, which failed the link (`LNK2005`). Only
  #630's definition remains.
- #630's comment says the stand-in stays NULL. In Windows builds the port
  renderer now publishes its IDirect3DRMDevice3 there
  (`directdraw_port_publish`), the original's `lpDevice()` contract.
  `viewport_create` and `viewport_create_pixel` dereference `lpDevice()`
  without a test, so this can only make a call that crashed on NULL work. The
  native port is unchanged. `viewports_port.cpp`'s comment is left for #630's
  owner.

**The image end.** The base's image ends at `0xa48000`, 731 KB lower than
before #633. It still covers the original's plain constant `0x800000`, so
`make pe-audit` still warns, and the plain-immediate check is still OK
(finding 584). This change does not move it.

## `/OPT:REF` (finding 579)

| | before (`main`) | after |
| --- | ---: | ---: |
| `opt_ref.annotated.full`: removed / exact removed | 490 / 228 | 498 / 239 |
| `opt_ref.annotated.game_only`: removed / exact removed | 473 / 217 | 479 / 226 |
| Exact annotated functions | 1,539 | 1,556 |
| The unit's object: removed (exact) | `directdraw.cpp.obj`: 6 (1) | `directdraw.c.obj`: 11 (9) |

The unit adds 17 exact functions, so more exact functions can be removed. The
11 removed are those whose original callers are still port code:

- EnumDrivers, EnumDevices, EnumModes and their callbacks, Setup and
  CreateClipper: the start-up dialog `0049d030`;
- Shutdown: WinMain;
- Blt8To16: Container.c `00474050`;
- SaveBMP: Images.c `0047e440`, not mapped.

Kept, because a live caller reaches them: Initialise, Flip, BlitBuffers,
ReturnFrontBuffer, AdjustTextureUsage, GetAvailTextureMem, Clear, GetColour
and CountMaskBits.

**Measurement noise.** Before the rebase, relinks of two trees that differ
only in the `lego_play_movie` switch measured 504 / 236 and 506 / 238. The per-object
lists of removed functions by symbol (`opt-ref-callers --min 1`) differ only
in `directdraw_return_front_buffer`, which the switch keeps (492 to 491
removed). The by-address total moves the other way because of the
duplicate-name copies that finding 591 describes.

## Reccmp lint ratchet

1,272 → 1,267 entries (−5, no growth; on #634's head). Five statics in the old unit file are
now in a tail file: `directdraw_device_create_internal`,
`directdraw_renderer_init_winapi`, `winapi_renderer_active`,
`winapi_renderer_clipper_attach` and `winapi_rm_device_create`.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox): OK. The link-order
  map, `winapi-link-check` and the plain-immediate check are OK too.
- `make match-progress`: the three generated files.
- `make unit-data-offsets`: the G98-010 row above.
- `python3 tools/translation_units.py build`: 0 conflicts.
  `tools/binary_progress_inputs.txt` is regenerated.
- `make tools-test`: OK (lint, with the ledger shrinking, link order, winapi
  list, Meson parity and the tool tests).
- The matching build compiles `directdraw.c` as C (`cl /TC`), which rejects
  declarations after statements. The host has no `windows.h`, so the
  `gcc -std=gnu89` check cannot run on this file.
- Native: `cmake -S . -B build -DTEST_WADS_DIR=<shipped WADs> && cmake
  --build build && ctest --test-dir build`: 39/39, real-data tests included.
- MinGW reference build: not built. No i686 MinGW toolchain is installed in
  the developer distrobox. The unit, `directdraw_port.cpp`'s publish step
  and the `crt.h`/`aitask_imports.h` views are compiled there for the first
  time.

## Reproducing

    python3 tools/convert_unit.py G98-010 --stem directdraw   # after git mv directdraw.cpp directdraw_port.cpp
    make match-function SOURCE=src/gods98/directdraw.c
    make reccmp-check
    make match-progress
    make unit-data-offsets
    make opt-ref-link opt-ref
    make opt-ref-callers OPT_REF_ARGS="--object directdraw.c.obj"
    make pe-audit
