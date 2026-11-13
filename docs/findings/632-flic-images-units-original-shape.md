# Finding 632 — Flic.c (G98-016) and Images.c (G98-011) rebuilt in original shape, with port tails

Issue #711, part of #388 under [ADR 0005](../adr/0005-original-shape-c-units.md).
Both units were relocated verbatim (`gods98/flic_unit.cpp`,
`gods98/images_unit.cpp`) but never converted. This change rebuilds them as
the C units [`src/gods98/flic_unit.c`](../../src/gods98/flic_unit.c) and
[`src/gods98/images_unit.c`](../../src/gods98/images_unit.c). The relocated
files become the port tails `flic_port.cpp` and `images_port.cpp` (`git mv`,
unannotated, `tail` entries in `src/link_order.cmake` and Meson).

**Both extents hold** (see [Unit extent](#unit-extent)). Images.c's boundary
was low-confidence; the data, literal and string evidence below confirms it.

"Before" is the *match baseline* at `main` `7600fb4c`. "After" is this
change, from the full matching build (`make reccmp-check`).

## Result

| | before | after |
| --- | ---: | ---: |
| Flic.c similarity (24 functions, weighted by span, 5,232 bytes) | 22.80% | **92.58%** |
| Flic.c exact / mapped | 3 / 23 | **19 / 24** |
| Images.c similarity (27 functions, weighted by span, 4,176 bytes) | 19.02% | **82.99%** |
| Images.c exact / mapped | 3 / 19 | **19 / 27** |
| Exact-function code bytes (`binary-progress.json`) | 244,057 | **256,207** |
| Accuracy, full / game-only | 92.71 / 92.02 | **93.64 / 93.05** |
| `opt_ref` annotated (full): removed / exact removed | 327 / 198 | 299 / 197 |
| `opt_ref` annotated (game-only) | 321 / 192 | 293 / 191 |
| Gate image end (`make pe-audit`) | `0x86d000` | `0x86f000` |
| `/OPT:REF` relink `.bss` (`binary-progress.json`) | 1,111,760 | 1,111,904 |
| Reccmp lint ledger (entries) | 983 | 965 |

The weights are each function's span up to the next function, padding
included. An unmapped function counts as 0%. No function outside the two
units fell; 75 rose, 31 of them to 100%. Neither unit has naked asm or a
`Deviation (scoring only)`. The added `.bss` (144 bytes) is `imageGlobs`,
Images.c's own state; Flic.c adds none.

The names are the units' own, after gods98's `Flic_*` and `Image_*`
convention (OpenLRR's names where known; the decoders' names are inferred
from the chunk types they handle). In the matching build some take the
port's names ([Callers](#callers)); the tables give the name in the report.

### Flic.c

| Address | Function | Port name before | Span | Before | After |
| --- | --- | --- | ---: | ---: | ---: |
| `00483c80` | `flic_setup` | `flic_open` | 640 | 14.50 | 89.24 |
| `00483f00` | `Flic_Close` | `flic_close` | 96 | 41.98 | **100.00** |
| `00483f60` | `Flic_LoadHeader` | `flic_file_header_validate_path` | 272 | 17.09 | **100.00** |
| `00484070` | `flic_animate` | `(none)` | 352 | unmapped | **100.00** |
| `004841d0` | `Flic_Memory` | `flic_tick_loop` | 144 | 18.18 | **100.00** |
| `00484260` | `Flic_Load` | `flic_stream_frame_advance` | 192 | 14.81 | **100.00** |
| `00484320` | `Flic_FindChunk` | `flic_chunk_walk` | 400 | 17.12 | 67.74 |
| `004844b0` | `Flic_FrameChunk` | `flic_frame_record_walk` | 352 | 16.67 | **100.00** |
| `00484610` | `Flic_DoChunk` | `ledger_stub_00484610_FlicSubrecordOpcodeDispatch` | 272 | 34.00 | 44.87 |
| `00484720` | `Flic_LoadPointers` | `flic_chunk_buffer_and_continue_lk` | 176 | 32.35 | **100.00** |
| `004847d0` | `Flic_LoadPalette64` | `flic_chunk_buffer_and_continue_type0b` | 176 | 33.08 | **100.00** |
| `00484880` | `Flic_Copy` | `flic_opcode16_mode_dispatch` | 80 | 27.78 | 61.11 |
| `004848d0` | `Flic_BrunDepack` | `flic_decode8_bit_opcode25_mode4` | 256 | 22.77 | 96.39 |
| `004849d0` | `Flic_BrunDepack16` | `flic_decode16_bit_keyframe` | 336 | 11.16 | **100.00** |
| `00484b20` | `Flic_Brun` | `flic_opcode25_decode_dispatch` | 112 | 71.88 | **100.00** |
| `00484b90` | `flic_opcode25_success_stub` | `flic_opcode25_success_stub` | 16 | **100.00** | **100.00** |
| `00484ba0` | `Flic_Palette256To64k` | `flic_packed_triplet_table_to_word_build_target` | 96 | 82.76 | **100.00** |
| `00484c00` | `Flic_Colour256` | `flic_decode16_bit_row_records` | 144 | 22.76 | **100.00** |
| `00484c90` | `flic_opcode27_fallback_noop` | `flic_opcode27_fallback_noop` | 16 | **100.00** | **100.00** |
| `00484ca0` | `Flic_DeltaWord` | `flic_decode8_bit_alternate_delta` | 432 | 15.95 | **100.00** |
| `00484e50` | `Flic_DeltaWord16` | `flic_decode16_bit_delta` | 480 | 19.50 | **100.00** |
| `00485030` | `Flic_Delta` | `flic_opcode27_decode_dispatch` | 144 | 64.79 | **100.00** |
| `004850c0` | `Flic_GetPaletteColour` | `indexed_word_table_lookup` | 32 | 29.63 | **100.00** |
| `004850e0` | `object_field10_get` | `object_field10_get` | 16 | **100.00** | **100.00** |

### Images.c

| Address | Function | Port name before | Span | Before | After |
| --- | --- | --- | ---: | ---: | ---: |
| `0047d410` | `renderer_record_table_initialize` | `renderer_record_table_initialize` | 32 | 87.50 | **100.00** |
| `0047d430` | `scene_object_registry_teardown` | `scene_object_registry_teardown` | 64 | 8.33 | **100.00** |
| `0047d470` | `Image_Remove` | `image_surface_release` | 32 | 37.21 | **100.00** |
| `0047d490` | `Image_CopyToDataToSurface` | `surface_pixel_format_query_convert_surface` | 144 | 19.26 | 74.23 |
| `0047d520` | `Image_8BitSourceCopy` | `surface_palette8_to_16bit_surface_expand` | 480 | 33.82 | 42.17 |
| `0047d700` | `Image_CountMaskBits` | `pop_count32` | 32 | **100.00** | **100.00** |
| `0047d720` | `Image_CountMaskBitShift` | `find_first_set_bit32` | 32 | **100.00** | **100.00** |
| `0047d740` | `Image_FlipSurface` | `surface_pixel_rows_flip_with_scratch` | 192 | 22.92 | **100.00** |
| `0047d800` | `Image_24BitSourceCopy` | `surface_bitmap24_bit_surface_convert` | 464 | 29.17 | 75.95 |
| `0047d9d0` | `Image_LoadBMPScaled` | `(none)` | 448 | unmapped | 78.55 |
| `0047db90` | `Image_RGB2CR` | `rgb_channels_pack` | 48 | **100.00** | **100.00** |
| `0047dbc0` | `Image_SetPenZeroTrans` | `resource_record_add_ref` | 48 | 22.22 | **100.00** |
| `0047dbf0` | `Image_SetupTrans` | `image_surface_blit_transformed` | 192 | 18.87 | **100.00** |
| `0047dcb0` | `Image_DisplayScaled` | `(none)` | 432 | unmapped | 68.16 |
| `0047de60` | `Image_LockSurface` | `(none)` | 112 | unmapped | **100.00** |
| `0047ded0` | `Image_UnlockSurface` | `surface_directdraw_unlock` | 32 | 10.81 | **100.00** |
| `0047def0` | `Image_GetPen255` | `(none)` | 96 | unmapped | **100.00** |
| `0047df50` | `Image_GetPixelMask` | `(none)` | 80 | unmapped | **100.00** |
| `0047dfa0` | `Image_GetPixel` | `surface_directdraw_pixel_read` | 176 | 15.25 | **100.00** |
| `0047e050` | `Image_Create` | `(none)` | 112 | unmapped | 66.67 |
| `0047e0c0` | `Image_AddList` | `(none)` | 112 | unmapped | **100.00** |
| `0047e130` | `Image_RemoveAll` | `scene_object_registry_release_all` | 96 | 30.77 | **100.00** |
| `0047e190` | `Image_DDColorMatch` | `font_device_color_convert` | 320 | 18.33 | 98.06 |
| `0047e2d0` | `Image_CR2RGB` | `rgb_channels_unpack` | 48 | 11.11 | **100.00** |
| `0047e300` | `Image_GetScreenshot` | `ledger_stub_0047e300_InterfaceMethod18StructApply` | 224 | 10.53 | 96.88 |
| `0047e3e0` | `Image_InitFromSurface` | `ledger_stub_0047e3e0_InterfaceMethodResultRecordInitialize` | 96 | 76.47 | **100.00** |
| `0047e440` | `Image_SaveBMP` | `(none)` | 32 | unmapped | **100.00** |

`00484070` Flic_Animate had no source; it is written and exact. Images.c's
eight functions with no source (`0047d9d0`, `0047dcb0`, `0047de60`,
`0047def0`, `0047df50`, `0047e050`, `0047e0c0`, `0047e440`) are written and
mapped; five are exact.

Callers that rose with no source change: 75 functions in other units, 31 of them now exact, among them
Bubbles.c `00407240`, FrontEnd.c `00410520`, HelpWindow.c `004185f0` and
`00419100`, Info.c `00419430`, `00419650` and `00419f30`, Interface.c
`0041b270`, Lego.c `00434a00`, Panel.c `0045a2e0`-`0045c5f0` (ten),
Pointer.c `0045ca70`, Reward.c `0045f290`, `0045f3e0`, `0045faf0` and
`00462460`, ScrollInfo.c `004636b0`-`00463810`, Text.c `0046abb0` and
`0046ad00`, and Font.c `0047a470`.

## What the units do

- **Flic.c.** A FLICSTRUCT (0x6e8 bytes) holds the 0x88-byte file header at
  `+0x640`, the 256-entry palette and its 16-bit form, the frame counters and
  a system-memory DirectDraw surface. Flic_Setup (`00483c80`) validates the
  header (Flic_LoadHeader, `00483f60`: the size must equal the file length,
  and the magic must be one of `1234 9119 af11 af12 af43`), creates the
  surface and either reads the whole file (`FLICMEMORY`) or keeps it open.
  Flic_Animate (`00484070`) locks the surface, steps one frame (Flic_Memory
  `004841d0` or Flic_Load `00484260`), unlocks and blits to the back buffer
  (`DDBLT_KEYSRC` when transparent). Flic_FindChunk walks to the next frame
  chunk (`0xf1fa`), Flic_FrameChunk its subchunks, and Flic_DoChunk
  dispatches them: 4 (256-colour palette), 7/27 (delta), 15/25 (byte run),
  16 (copy), `0x5555` ("UU", a no-op) and three unsupported types.
- **Images.c.** An Image (0x20 bytes) is a surface, its size, its two pens
  in the surface's format, pen 0 as RGB, a flags word and the free-list
  link. Images come from a pool of 32 blocks of 1, 2, 4, ... records
  (Image_AddList `0047e0c0`, Image_Create `0047e050`, Image_Remove
  `0047d470`). Image_LoadBMPScaled (`0047d9d0`) parses a bitmap with BMP.c,
  creates a system-memory surface, converts 8-bit or 24-bit pixels to the
  display's 16-bit format and flips the rows (`0047d520`-`0047d800`).
  Image_DisplayScaled (`0047dcb0`) blits to the back buffer with `floor`ed
  rectangles. `images.h`'s `Image` now has the record's full layout (a
  struct's field count does not move the symbol counter, finding 538).

The native port has no DirectDraw, so both units compile to nothing there
(`#if !defined(LEGORRR_BACKEND_MINIWIN)`, as directdraw.c) and the port's
player and software surfaces serve every native caller (#552).

## Unit extent

Findings 599 and 603 apply: MSVC 5.0 pools literals per object.

**Flic.c, `00483c80`-`004850f0`, 24 functions.**

- The strings `004ab10c`-`004ab17e` ("Not Enough Memory > %s", "Flic File
  Invalid %s", "Flic File Invalid", "Flic File Not Found %s", "Flic Not
  Enough Memory") are referenced only from `00483ea7`-`00484829`, inside the
  range. Keys.c's block starts at the next 8-byte boundary (finding 513).
- No `.data`, `.rdata` or `.bss` of its own: it reads DirectDraw.c's common
  `directDrawGlobs` (`0076ac84`, `0076ac8c`).
- Mesh.c ends at `00483c80` (finding 625); TextWindow.c starts at
  `004850f0` (finding 600).
- `00484b90` (return TRUE) and `00484c90` (return) are do-nothing functions
  the linker folded identical functions from other units into: Lego.c,
  LegoObject.c, NERPs.c, Water.c and Files.c call them. Flic_GetWidth was
  folded the same way into `004897d0`, which this tree annotates as
  AnimClone.c's `anim_clone_is_lws`; the callers keep calling that.
- External callees: Files.c, the CRT (`malloc`, `free`, `sprintf`,
  `_splitpath`, `_ftol`) and `00456210` (another folded no-op).

**Images.c, `0047d410`-`0047e460`, 27 functions.**

- `__FILE__` "C:\Dev\SourceSafe\gods98_dx6\gods98\src\Images.c" at
  `004aae84` is used by Image_GetScreenshot (`0047e300`, line 1140): the
  name is attested.
- `.bss` `00533908`-`00533994` (`imageGlobs`: 32 block pointers, the free
  list, the block count, the flags) is referenced only from
  `0047d418`-`0047e3e0`. A scan of the whole `.text` finds no other
  reference to `00533908`-`00533993`.
- `.rdata` `0049e998` (255.0f) is referenced only by Image_SetupTrans
  (`0047dbf0`, six times). "Data\%s" (`004aae7c`) is referenced only by
  `0047d9d0`.
- DirectDraw.c ends at `0047d410` (finding 596). `0047e460` is the AVI
  player's scalar deleting destructor (G98-012, finding 604).
- External callees: DirectDraw.c (`0047c8f0` DirectDraw_SaveBMP), Files.c
  (`004800a0` File_LoadBinary), BMP.c (`00489c30`, `00489d70`), DXError.c
  (`00489dd0`) and the CRT.

`translation-units.spec.csv` records this evidence for both rows.
`python3 tools/translation_units.py build`: 0 conflicts.

## How the conversion is done

### State

Images.c's `Image_Globs imageGlobs` is a `static` definition annotated at
`00533908` (0x8c bytes, `.bss`). The port's `s_renderer_record_table` and
`s_small_record_free` stay in `images_port.cpp`, unannotated. Flic.c owns
no state. `make unit-data-offsets`: G98-011 `.bss` is gaps-only (2, 0 unknown); G98-016 has
no data of its own (gaps-only `.data`, 5, 0 unknown, from the shared
string block). Neither unit has commons.

### Callers

- **Flic.c.** `flic_link_names.h` gives five functions the port's names in
  the matching build: `flic_setup` and `flic_animate` (the original-ABI
  adapters in `flic.cpp`, called only by C units), the two folded no-ops,
  and `object_field10_get` (Flic_GetHeight). The port's definitions of those
  names are compiled out there. Flic_Close keeps the unit's name, because
  the port's `flic_close` is its own player's close (`mission.cpp`); a
  matching-build macro in `flic_api.h` points the C callers (Reward.c,
  FrontEnd.c) at the unit.
- **Images.c.** `images_link_names.h` gives Image_Initialise and
  Image_Shutdown the port's names (Lego.c is their only caller). The other
  port names at Images.c addresses take the port's `TextureSurface` and C++
  port code calls them with one, so they stay the port's. Matching-build,
  C-only macros placed before each declaration point the C callers at the
  unit's names instead: `images.h`, `image_adapters.h`, `font_imports.h`,
  `view_api.h` and the imports headers of Text.c, FrontEnd.c, Info.c,
  Lego.c, Bubbles.c, Pointer.c, LegoObject.c and HelpWindow.c. A macro
  renames a declaration one for one, so no unit's symbol count changes.
- **One edit in another unit's file:** `lego_unit.c`'s own declaration of
  `resource_record_add_ref` (line 9556) takes `struct Image*` instead of
  `void*`, so it agrees with `image_adapters.h` once both are renamed to
  Image_SetPenZeroTrans (MSVC's C4028 otherwise fails the build). Same
  symbol count.
- On the stack, Objective.c (`objective_unit.c`, #710) is a C unit too;
  `objective_unit_imports.h` gets the same block, which also points its
  Image_GetScreenshot adapter at the unit.

Natively nothing changes: the port serves every name.

### Source shapes worth noting

- Flic_Animate: `ok` is assigned just before the lock, not initialised;
  `if (fsp->currentframe == 0) advance = TRUE;`; the blit's fill colour is
  `DDBLTFX.dwFillColor` (`+0x50`).
- Flic_LoadHeader: the five magics as five separate `case`s with their own
  `File_Close; return TRUE;` give the original's switch tree; one shared
  body gives a range check.
- Flic_Delta: an explicit `return TRUE` after the default no-op keeps the
  branches in the original's order.
- The 16-bit decoders convert a 555 word to 565 when it is read, then pick
  the 555 form inside the store (`is15bit ? ... : colour`); counted loops are
  `while (n) { ...; n--; }`; the delta decoders use a byte pointer.
- Image_GetPen255 falls off the end on failure: `eax` returns
  GetPixelFormat's error, as in the original. Image_CR2RGB reads the bytes
  through a byte pointer (`cl`, `ch`, `[esp+6]`).
- Image_DDColorMatch's locals in a found declaration order (98%).

## What is left

- **`00484610` Flic_DoChunk (44.87%) and `00484880` Flic_Copy (61.11%).**
  Both call the linker-folded do-nothing functions from separate branches
  (`00484b90` from three `case`s, `00484c90` from three `if`s). The
  original called different functions there that `/OPT:ICF` later folded,
  so its compiler kept the branches apart; this build calls one name, and
  MSVC merges the identical tails, which shifts the jump table and the
  registers. Tried: casts of the callee, `return` against `retval`, eight
  branch shapes. Calling two stand-in functions raised `00484610` to 72%,
  but adds code the original does not have, so it is not used.
- **`00483c80` Flic_Setup (89%).** The original keeps the file name on the
  stack and `0` in `ebp`; this build keeps the name in `ebp`. The chained
  `fsXsize = ddsd.dwWidth = width` raised it from 61%; the permuter and four
  orders of the size stores found nothing more.
- **`00484320` Flic_FindChunk (68%).** The loops and the per-case seek and
  read now match; register choice (`ecx` against `eax` for the flags and the
  start position) remains.
- **`0047d520` Image_8BitSourceCopy (42%), `0047d800` Image_24BitSourceCopy
  (76%), `0047dcb0` Image_DisplayScaled (68%), `0047d9d0`
  Image_LoadBMPScaled (79%), `0047e050` Image_Create (67%), `0047d490`
  Image_CopyToDataToSurface (74%).** Loop hoisting and register allocation.
  The permuter (200 s each) and 80 random declaration orders found no gain
  in original shape. Image_Create's free-list pop is the shape Mesh.c's
  `004807a0` and Font.c's `0047a580` also miss (finding 625).
- **Port names.** `image_surface_release`, `resource_record_add_ref` and
  `image_surface_blit_transformed` keep the port's models for C++ callers that
  pass a `TextureSurface` (`frontend_port.cpp`, `info_port.cpp`,
  `frontend.cpp`, `pointer_port.cpp`); those callers are port tails.

## Checks

- `make reccmp-check`: no function fell below the baseline (75 rose
  outside the units); winapi link check and link-order `check-map`: OK.
- `make match-progress`: baseline, listing digests and
  `binary-progress.json` regenerated. `make unit-data-offsets`: the rows
  above.
- `make tools-test` (no `--allow-growth`): pass. The lint ledger shrinks
  from 983 to 965 (`make reccmp-lint-update`).
  `tools/binary_progress_inputs.txt` rewritten with
  `binary_progress_hash.py --write-manifest`.
- Native build and `ctest`: 37 of 37 pass.
- `make opt-ref-callers`: `flic_unit.c.obj` loses 2 functions to `/OPT:REF`
  (Flic_Close, Flic_GetHeight: their callers in FrontEnd.c and Reward.c are
  not reached from a live root, finding 579; Panel.c and Pointer.c still call
  the port's `flic_width`/`flic_height`), `images_unit.c.obj` 3
  (Image_GetScreenshot, whose caller `00458670` calls the port's
  `mission_reward_success_snapshot_prepare`; Image_InitFromSurface; and
  Image_SaveBMP, whose FrontEnd.c caller is removed).

## Stacked on #716

Rebased as the top layer over #716 (`26c2af74`, over #715, #714 and #713).
Without the `objective_unit_imports.h` block, `00457d40`
(objective_load_original) fell from 60.88% to 60.11% (its Images.c calls
went to the port's names); with it, nothing falls and no exact function of
#713-#716 is lost. 116 functions rise, 65 of them to 100%. Lego.c's
`0041fb50` (lego_game_config_init, five calls to `0047d9d0`) rises from
98.96% to 99.29%.

| | #716 | this change |
| --- | ---: | ---: |
| Exact-function code bytes | 262,810 | **279,887** |
| Accuracy, full / game-only | 94.46 / 93.96 | 95.38 / 94.98 |
| `opt_ref` annotated (full): removed / exact removed | 191 / 127 | 164 / 127 |
| `opt_ref` annotated (game-only) | 185 / 121 | 158 / 121 |
| Gate image end (`make pe-audit`) | `0x871000` | `0x873000` |
| Relink `.bss` | 1,115,552 | 1,115,696 |

On the stack no Flic.c or Images.c function is removed by `/OPT:REF`.
