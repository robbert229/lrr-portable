# Finding 519 — 2D drawing unit (G98-019) rebuilt in original shape

Issue #412. This conversion follows the
[ADR 0005](../adr/0005-original-shape-c-units.md) recipe, starting from the
`tools/convert_unit.py G98-019 --stem draw` scaffold. It rebuilds the gods98
2D drawing unit: the clip window, clipped Bresenham lines, rectangle fills,
the dotted circle, the colour packer, the surface lock and the pixel writers.
The unit is original translation unit G98-019, text `00485e80`–`004869f0`
([finding 505](505-original-translation-units-and-link-order.md)). It now
lives in [`src/gods98/draw.c`](../../src/gods98/draw.c) with the C boundary
header [`draw.h`](../../src/gods98/draw.h).

"Before" is the *match baseline* on `main` at `1afc504d` (after #455, the
non-incremental link). "After" is this change. `make reccmp-check` and
`make match-function` give the same per-function scores.

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (byte-weighted, all 18 functions, 2,853 bytes) | 11.19% | **96.22%** |
| The 10 functions mapped before (2,194 bytes) | 14.55% | 99.88% |
| Exact functions | 0 | **16 of 18** |
| Mapped functions | 10 | 18 |
| Accuracy, full (`binary-progress.json`) | 54.25% | 54.71% |
| Accuracy, game-only | 49.93% | 50.46% |
| Exact-function code bytes | 41,053 | 43,412 |
| `.text` identical at original addresses | 18,180 | 18,460 |
| Adjacent order inversions, full | 906 | 903 |

Byte weights are the original's code bytes, without the `0x90` fill. An
unmapped function counts as 0%.

| Address | Function (descriptive name) | Bytes | before | after |
| --- | --- | ---: | ---: | ---: |
| `00485e80` | `surface_render_setup` (Draw_Initialise) | 29 | 87.50 | **100.00** |
| `00485ea0` | `viewport_subrectangle_set` (Draw_SetClipWindow) | 271 | unmapped | **100.00** |
| `00485fb0` | `viewport_rectangle_get` (Draw_GetClipWindow) | 52 | unmapped | **100.00** |
| `00485ff0` | `surface_draw_polyline` (Draw_LineListEx) | 158 | 14.93 | 98.36 |
| `00486090` | `surface_fill_pixel_spans` (Draw_RectListEx) | 383 | 10.81 | **100.00** |
| `00486210` | `surface_fill_pixel_span_records` (Draw_RectList2Ex) | 384 | 10.00 | **100.00** |
| `00486390` | `surface_draw_circle_pixels` (Draw_DotCircle) | 315 | 9.96 | **100.00** |
| `004864d0` | `surface_color_pack_to_pixel` (Draw_GetColour) | 126 | 18.75 | **100.00** |
| `00486550` | `surface_active_pixel_writer_configure` (Draw_LockSurface) | 245 | 25.26 | **100.00** |
| `00486650` | `surface_unlock_reset_pixel_writer` (Draw_UnlockSurface) | 61 | 18.18 | **100.00** |
| `00486690` | `surface_pixel_writer_select_by_format` (Draw_SetDrawPixelFunc) | 143 | 8.77 | **100.00** |
| `00486720` | `surface_bresenham_line_walk_clipped` (Draw_LineActual) | 350 | 14.29 | **100.00** |
| `00486880` | `Draw_Pixel8` | 31 | unmapped | **100.00** |
| `004868a0` | `Draw_Pixel16` | 33 | unmapped | **100.00** |
| `004868d0` | `Draw_Pixel16XOR` | 35 | unmapped | **100.00** |
| `00486900` | `Draw_Pixel16HalfTrans` | 158 | unmapped | 33.33 |
| `004869a0` | `Draw_Pixel24` | 48 | unmapped | **100.00** |
| `004869d0` | `Draw_Pixel32` | 31 | unmapped | **100.00** |

The exported functions keep the port's names. The binary names neither the
file nor the functions. The `Draw_*` names are descriptive only.

### Near misses (permuter targets, #382)

One function ends between 75% and 99%:

- **`surface_draw_polyline` (`00485ff0`), 98.36%.** The listing has the
  original's 158 bytes and instructions. The only difference is the order of
  the two strength-reduced pointer increments at the loop's foot: the
  original has `add esi, 8; add edi, 8` (from-list, then to-list), and the
  rebuild has them the other way round. Reccmp calls it an effective match.
  Pointer-walking loops scored 61–63%, and swapping the local declarations
  changed nothing. *Update (#382):* now at 100%. The loop takes per-iteration
  element pointers, `from = &fromList[loop]; to = &toList[loop];`. The
  attempts are in
  [`msvc-permuter-demonstration.md`](../coverage/msvc-permuter-demonstration.md#polyline-acceptance-target).

One function is below that band:

- **`Draw_Pixel16HalfTrans` (`00486900`), 33.33%.** Same length (138 bytes
  as Reccmp counts them). The semantics match: each channel is the sum of
  the two halved values, masked with the channel mask. The original does
  the halving in 16-bit registers and computes `colour >> 1` and
  `pixel >> 1` twice, once for green and red and once for blue. MSVC
  computes each only once in every form tried: eight variants, with
  16-bit temporaries, per-channel statements, casts of `value` and fused
  expressions, all scoring 26–33%. The odd constants `0x7fe0` (green) and
  `0x7800` (red, in place of the red mask) are the original's.

`tools/msvc_permuter.py` (#428), run for 90 s on each, found no applicable
rewrite (0 attempts). Neither function has the declaration, comparison or
loop shapes its passes rewrite.

## The boundary

The spec boundary is **confirmed**. Its confidence stays **low** by the
tool's rules, because one slack function (`00485e30`) is placed by
behaviour only.

- **Six functions missing from the manifest.** The pixel writers
  `00486880`–`004869d0` close the unit. Nothing calls them directly: the
  writer selection (`00486690`) stores their addresses in the unit's
  pixel-writer pointer. The Ghidra manifest therefore lacks them, and so do
  `translation-units.csv` and the before table of the scaffold. They are now
  annotated, and Reccmp maps all six.
- **Start (`00485e80`).** `00485e80` writes only this unit's `.bss` (the
  flags word `00540830`). Keys.c's last function, `00485e30`, uses only the
  common key-name table (finding 513). Neither touches the other's data, and
  `00485e80` calls only `00485ea0`.
- **`.rdata`.** The block opens with the `{1, 2, 4, -1}` header table at
  `0049ea10`. G98-017's own floats (`0049ea04`, `0049ea0c`) have no table,
  and Keys.c has no `.rdata`. Then come the unit's four constants: 0.0
  (`0049ea20`, clip window), 1.0 (`0049ea24`, both rectangle fills), 2π as a
  float (`0049ea28`, dotted circle) and the double 255.0 (`0049ea30`, colour
  packer). The dword at `0049ea2c` is the double's alignment.
- **`.bss`.** `005407e8`–`00540833` is one 0x4c-byte globals struct: the
  pixel-writer pointer, the clip start and end, 16 unreferenced bytes, the
  locked surface pointer, pitch, depth, the three channel masks, their bit
  counts and the flags word. G98-020's `.bss` starts at `00540838`.
- **End (`004869f0`).** G98-020 (Lws.c) opens its own `.rdata` header at
  `0049ea38` and its `.bss` at `00540838`. That is a data step.
- **Calls.** The unit calls nothing outside itself apart from LIBC's
  `_ftol`, so no neighbour needs converting first.

The spec note records this evidence, and `translation_units.py build`
reports zero data conflicts.

## The unit

- **State.** One `static` struct annotated at `005407e8`, laid out as the
  original's `.bss` on Win32. The four constants are annotated statics in
  address order. The header table is not reproduced, as in the other
  converted units.
- **The back buffer.** The original reads the DirectDraw back buffer from
  the common word `0076ac8c`. The port holds that word as
  `s_render_surface_secondary` (a `uint32_t` in `crt.cpp`). Its declaration
  in `crt.hpp` gained `extern "C"` in place. The new C boundary header
  [`src/crt/crt.h`](../../src/crt/crt.h) declares it for C units, and
  `test_c_boundaries.cpp` checks the pair.
- **The surface interface.** The matching build has no DirectDraw headers.
  `draw.h` declares the original's `DDSURFACEDESC2` (0x7c bytes on Win32)
  and the three `IDirectDrawSurface4` slots the unit calls:
  `GetSurfaceDesc` (+0x58), `Lock` (+0x64) and `Unlock` (+0x80). The
  functions take the surface as `void*`, which has the same ABI as the
  original's interface pointer.
- **Original contracts restored.**
  - Colours are 0..1 channels scaled by 255.0. The port's packer took 0..255
    channels.
  - The writers address the locked surface by pitch.
  - The 24-bit writer stores a whole dword, with the colour in bytes 1–3,
    and keeps byte 0. That is the original's behaviour.
  - A rectangle that reaches the clip edge stops one pixel short of it.
  - The port's raster contexts, writer tables, NULL guards and its callback
    lock adapter are gone.

### Shape notes

- **Draw_SetClipWindow** converts the surface size into two float locals
  and compares the window against those, not the globals: this is the
  original's 0x88-byte frame. Comparing against the globals scored 71.83%.
- **Draw_DotCircle** casts `sin(angle) * radius` to `float` before adding the
  centre. Without the cast MSVC loads the radius before the angle (90.48%),
  and `radius * sin(...)` or `centre + ...` changed nothing.
- **The clip test** for single pixels is a macro (`Draw_PixelClip`). The
  line walk and the circle share its exact code.

### Native port

The native port has no DirectDraw back buffer, and a 64-bit pointer does not
fit the port's word at `0076ac8c`. Under `LEGORRR_BACKEND_MINIWIN`, `draw.c`
therefore reads `draw_port_back_surface` instead. That variable is defined in
the native-only
[`draw_port.cpp`](../../src/gods98/draw_port.cpp). Its default surface fails
`GetSurfaceDesc` and `Lock`, so the clip window keeps its value and nothing
is drawn. Before this change nothing was drawn either: the only live caller
went through a hook that nothing installed. The matching and Win32 builds
read the original word.

## Callers

`edges.csv` lists eight outside callers. The port reimplemented them all
around hooks, and none called the unit:

| Caller | Original calls | Now |
| --- | --- | --- |
| `0041cd30` `position_tracker_box_draw` (Interface.c, `frontend.cpp`) | Draw_LineListEx ×2 | **calls the unit**; 14.46% → 19.35% |
| `004777a0` WinMain (`entry.cpp`) | Draw_Initialise | unchanged; the port's WinMain has no matching call site |
| `004248b0`, `00459b00`, `0045db00`, `0046b8b0` | line lists, rectangle fills, dotted circle, clip window | unchanged; each draws through its own hook table, which belongs to its unit's conversion |
| `004293b0`, `0045dbc0` | line lists, rectangle records, clip window | not in the port |

`position_tracker_box_draw`'s hook already had Draw_LineListEx's signature
(from-list, to-list shifted by one point, 4 lines, green, no effect). It now
calls `surface_draw_polyline` directly, as the original does. That call is
also what makes the link pull `draw.obj`, as the effects callers did for
`bezier.obj` (finding 514). The unused ledger stub
`ledger_stub_0041cd30_PositionTrackerBoxDraw` was deleted.

**Tests.** The new [`test_draw.cpp`](../../src/gods98/test_draw.cpp)
(`draw`) installs a fake surface. It checks the clip window (NULL, inside,
past the edge, negative origin), the lock and colour packer, both ends of a
diagonal, line clipping, the exclusive far edges of the fills, the
one-short clip edge, the per-rectangle colours, the circle's dot positions,
the 16-bit half-transparent and XOR effects, the 32-, 24- and 8-bit writers,
and an unsupported depth. It also checks that the native default surface
draws nothing.

## Entropy

The superseded port declarations were deleted only where that caused no
drift (rule 1). Three groups had to stay, each with a comment:

- **`view.hpp`** keeps `viewport_rectangle_set`,
  `ViewportDimensionsQueryFn` and `viewport_dimensions_query_hook_set`.
  Nothing defines them now. Removing them moved
  `lava_erosion_delay_from_cell` (`0040e9c0`) from 100% to 84.21% and
  `cmd_place_ray_candidate_check` (`00471370`) from 15.30% to 15.22%.
- **`surface.hpp`** keeps the old drawing types and two undefined peeks.
  Deleting `surface_lock_configure_pixel_writer`'s declaration exactly
  offsets the new signatures' extra parameters. Without that deletion,
  `parse_srfs` (`0048c690`) fell from 10.57% to 7.11% and
  `text_buffer_draw_wrapped` (`00485390`) from 5.99% to 5.96%.
- **`frontend.hpp`** keeps the now-unused `outline_draw` member of
  `PositionTrackerBoxDrawOps`. Removing it moved `0040e9c0` to 84.21% and
  `osf_read` (`00459470`) from 21.20% to 12.62%.

The other headers changed only by `extern "C"` and signature edits in
place.

**What remains.** `make reccmp-check` on this change, against `1afc504d`:

| Address | Function | File | before | after | Classifier | Notes |
| --- | --- | --- | ---: | ---: | --- | --- |
| `0047d520` | `surface_palette8_to_16bit_surface_expand` | `surface.cpp` | 33.82 | 32.65 | `drift-reordered; instructions +0` | rule 2 drift |
| `004089d0` | `segment_2d_point_distance` | `view.cpp` | 40.89 | 43.56 | (rose) | file lost the unit's definitions |
| `004882e0` | `scene_node_transform_matrix_apply` | `view.cpp` | 13.97 | 14.00 | (rose) | same |
| `00444680` | `entity_step_movement_vector_resolve` | `entity.cpp` | 5.23 | 5.24 | (rose) | `view.hpp` signature change |
| `0041cd30` | `position_tracker_box_draw` | `frontend.cpp` | 14.46 | 19.35 | (rose) | now calls the unit |

`0047d520`'s source did not change, but its file did: `surface.cpp` lost the
unit's old definitions, which the conversion must delete. Keeping the
deleted statics without annotations did not restore it. The function
bodies are what count, and keeping 14 dead functions was not worth 1.17
points. Single-function listings from before and after:

- both have 166 instructions, 509 bytes and the same opcode multiset;
- seven hunks differ, all in stack-slot assignment (`[esp + 0x30]` against
  `[esp + 0x38]` and so on) and in which byte register receives which
  component;
- the classifier labels it `drift-reordered; instructions +0`.

It is entropy rule 2 drift. **No exception for the owner:** the instruction
count is unchanged. The classifier's two known gaps do not apply here.
There is no `drift-review` caused by symbolic operand names, and no
`LIBRARY` function moved.

**Net effect, both scopes:** 14 functions rose and 1 fell, for a net of
+786.34 percentage points across functions mapped in both builds; 8 were
newly mapped and none was lost. Game-only and full are identical: no CRT
function moved.

## Validation

- `make reccmp-check` passes apart from `0047d520` above. `make
  match-progress` regenerated the baseline, the listing digests and
  `binary-progress.json` inside the developer distrobox, with `pefile` and
  `capstone` from a worktree-local venv under `build-match/`.
  `pr_match_progress.py check` reports them fresh.
- `draw.c` passes `gcc -std=gnu89 -Werror=declaration-after-statement
  -fsyntax-only -Wall -Wextra`.
- `cmake -S . -B build && cmake --build build && ctest --test-dir build
  --output-on-failure` builds with no errors, and all 35 tests pass,
  including `c_boundaries` and the new `draw`.
- `translation_units.py build` reports zero data conflicts.

## Reproducing

    python3 tools/translation_units.py dossier 485e80 4869f0
    python3 tools/convert_unit.py G98-019 --stem draw --dry-run
    make match-function SOURCE=src/gods98/draw.c
    make reccmp-check
    python3 tools/translation_units.py build
