# 688. The GUID block: amstrmid.lib and dxguid.lib as single members

Issue #837: finding 680's follow-up 4. Stacked on #842 (`data/838-rdata-start`
`429c6314`, finding 689, on `main` `77022c16`); measured with full matching
builds before and after.

## Summary

1. **The original's GUID block is 6,656 bytes: 416 GUIDs at
   `0049f158`–`004a0b58`**, after LIBC's `.rdata` (section 1). Finding 680
   gave 6,784 bytes (`0049f160`–`004a0be0`, 424 GUIDs). That range starts
   8 bytes late, and its last 136 bytes are an unreferenced `1.0f` with its
   padding (`004a0b58`) and `.xdata$x` (`004a0b60`–`004a0be0`).
2. **The block is two library members, each defining its GUIDs together**
   (section 2). The first 25 GUIDs are the DirectShow multimedia-stream
   GUIDs (`amstrmid.lib`). The other 391 are every GUID of the DirectX 6.1
   headers (`dxguid.lib`), DirectPlay and DirectMusic included. The code
   references 5 of the first group and 24 of the second. A library with one
   member per GUID, or one per header, would leave out what nothing
   references, so each library is one member. The rebuilt
   `cross/msvc5-importlibs/amstrmid.c` and `dxguid.c` define them in the
   original's order. LINK pulls `amstrmid.obj` and then `dxguid.obj` after
   LIBC's last member, as in the original. The block's 6,656 bytes are
   identical to the original's.
3. **The units declare the 26 GUIDs they used to define as `extern`**, under
   the SDK names, in the matching build. The portable builds keep the
   units' own copies (section 3). This removes 416 bytes of unit `.rdata`
   that the original does not have.
4. **`.rdata` grows from 4,664 to 10,824 bytes** (original 11,232). That is
   enough for `.data` to start at the original's `004a1000`. Its first 192
   items (3,401 B) are now at their original addresses (0 before). The
   remaining 408 bytes are not GUIDs (section 4).

## 1. The block

The block sits between LIBC's last `.rdata` item (`"1#SNAN"` at
`0049f14c`, then 4 bytes of alignment) and an unreferenced `1.0f` at
`004a0b58`, which `.xdata$x` follows at `004a0b60`. Each GUID's name comes
from the SDK headers (DirectX 5, Wine's and MinGW's DirectX headers and
IDL). Seven of them are not in any available header:

| Address | GUID | Name used | Basis |
| --- | --- | --- | --- |
| `0049f1d8` | `4E6CDE29-C0C4-11D0-8FF1-00C04FD9189D` | `LIBID_DirectShowStreamLib` | MIDL's position for amstream.idl's library, between the interfaces and the coclass |
| `0049f4a8` | `8D56C120-6A08-11D0-9B06-00A0C903A3B8` | `IID_IDirectDrawKernel` | ddkernel.h |
| `0049f4b8` | `60755DA0-6A40-11D0-9B06-00A0C903A3B8` | `IID_IDirectDrawSurfaceKernel` | ddkernel.h |
| `0049f4c8` | `ACA12120-3356-11D1-8FCF-00C04FC29B4E` | `IID_IDDVideoAcceleratorContainer` | the DirectDraw video accelerator interfaces |
| `0049f4d8` | `C9B2D740-3356-11D1-8FCF-00C04FC29B4E` | `IID_IDirectDrawVideoAccelerator` | as above |
| `0049f538` | `82D4F372-D097-11D0-A9B3-00AA00C0993E` | `GUID_DDraw_82D4F372` | name not confirmed; among the DirectDraw driver GUIDs |
| `004a0248` | `CAE56030-684A-11D0-D6F6-00A0C90F57DA` | `GUID_DInput_CAE56030` | name not confirmed; between DirectInput's GUIDs and the device classes |

Nothing references these seven, so their names change no byte. The two
unconfirmed names are marked in `dxguid.c`.

The groups, in order:

| From | GUIDs | Source |
| --- | ---: | --- |
| `0049f158` | 10 | amstream.h (MIDL's amstream_i.c: interfaces, type library, coclass) |
| `0049f1f8` | 3 | mmstream.h |
| `0049f228` | 2 | ddstream.h |
| `0049f248` | 4 | austream.h |
| `0049f288` | 6 | the DEFINE_GUIDs of mmstream.h (`MSPID_*`) and amstream.h (`CLSID_AM*`) |
| `0049f2e8` | 19 | ddraw.h |
| `0049f418` | 9 | dvp.h |
| `0049f4a8` | 4 | ddkernel.h, video accelerators |
| `0049f4e8` | 20 | DirectDraw driver GUIDs (ddrawi.h, dxmini.h, d3dhal.h) |
| `0049f628` | 23 | d3d.h |
| `0049f798` | 71 | d3drm.h, d3drmobj.h, d3drmwin.h |
| `0049fc08` | 52 | rmxftmpl.h |
| `0049ff48` | 9 | dxfile.h |
| `0049ffd8` | 44 | dinput.h, then the device classes |
| `004a0298` | 12 | dplay.h |
| `004a0358` | 22 | dplobby.h |
| `004a04b8` | 10 | dsound.h |
| `004a0558` | 28 | dmusicc.h (IReferenceClock first) |
| `004a0718` | 65 | dmusici.h |
| `004a0b28` | 3 | dmksctrl.h |

The DirectMusic GUIDs date the library to the DirectX 6.1 SDK. The
DirectX 5 SDK's `dxguid.lib` (in `3rdparty/isle-portable/3rdparty/dx5`)
has 183 members, one GUID each. Finding 574 took that shape for the
rebuild, but it cannot produce this block.

The original's code references these 29 GUIDs (operand scan of `.text`):

- amstrmid: `IID_IAMMultiMediaStream`, `CLSID_AMMultiMediaStream`,
  `IID_IDirectDrawMediaStream`, `MSPID_PrimaryVideo`,
  `MSPID_PrimaryAudio` (Movie.cpp).
- dxguid: `IID_IDirectDraw2`, `IID_IDirectDraw4`,
  `IID_IDirectDrawSurface`, `IID_IDirectDrawSurface3`, `IID_IDirect3D3`,
  `IID_IDirect3DDevice2`, `IID_IDirect3DDevice3`, `IID_IDirect3DTexture2`,
  `IID_IDirect3DViewport3`, `IID_IDirect3DRMViewport2`,
  `IID_IDirect3DRMFrame`, `IID_IDirect3DRMFrame3`, `IID_IDirect3DRMMesh`,
  `IID_IDirect3DRMMeshBuilder2`, `IID_IDirect3DRMTexture`,
  `IID_IDirect3DRMUserVisual`, `IID_IDirect3DRM3`, `GUID_SysKeyboard`,
  `IID_IDirectSound3DListener` and `IID_IDirectSound3DBuffer` from units;
  `GUID_XAxis`, `GUID_YAxis`, `GUID_ZAxis` and `GUID_Key` from
  dinput.lib's data members.

## 2. The libraries

`cross/msvc5-importlibs/sdk_guid.h` gives the SDK's `DEFINE_GUID` form under
INITGUID: an external `const` GUID. MSVC 5.0's own headers are DirectX 3's
and lack most of these GUIDs, so each library source spells its GUIDs out
with that macro. MSVC 5.0 emits them in definition order, 8-aligned, as the
original's block is.

- `amstrmid.lib` has one member, `amstrmid.obj` (25 GUIDs).
- `dxguid.lib` has one member, `dxguid.obj` (391 GUIDs). It replaces
  finding 574's five per-GUID members.

`src/link_order.cmake` lists `amstrmid.lib` just before `dxguid.lib`, after
the import libraries. In the first library pass, the movie unit's
references pull `amstrmid.obj`. Then dinput.lib's data members and the
units pull `dxguid.obj`. Both come after LIBC's last `.rdata` member
(`x10fout.obj`), as in the original. The map shows LIBC, then
`amstrmid.obj`, then `dxguid.obj`, and nothing after them in `.rdata`.

## 3. The units

In the matching build these units now declare their GUIDs `extern`, under
the SDK names, with their `GLOBAL` markers on the declarations (as
`input_unit.c` already did for `GUID_SysKeyboard`):

| Unit | GUIDs | Old name(s) |
| --- | --- | --- |
| G98-001 `movie_unit.cpp` | 5 amstrmid, 2 dxguid | `s_iid_am_multi_media_stream` … `s_iid_direct_draw_surface3` |
| G98-002 `container.c` | 6 | `IID_Cnt*`, `s_resource_tree_child_iid` |
| G98-003 `viewports.c` | 1 | `IID_IDirect3DViewport3` (was defined here) |
| G98-005 `main.c` | 3 | `s_iid_direct_draw_surface`, `s_iid_direct3d_device3`, `s_iid_direct3drm3` |
| G98-009 `sound3d.c` | 2 | `s_iid_directsound3d_listener`, `s_iid_directsound3d_buffer` |
| G98-010 `directdraw.c` | 2 | `s_iid_direct_draw4`, `s_iid_direct3d3` |
| G98-015 `mesh_unit.c` | 3 | static copies of the SDK names |
| G98-020 `lws.c` | 1 | `s_iid_user_visual` |
| G98-025 `animclone.c` | 2 (declarations only) | `s_resource_tree_child_iid`, `s_iid_user_visual` |

`container.c`'s `IID_CntMeshBuilder3` held `IID_IDirect3DRMMeshBuilder2`'s
value (`4516EC77`). It now uses that name.

The portable builds (native and MinGW) keep each unit's own copy, under the
same SDK name, in the `#else` branch. As before, IID_IDirect3DRMFrame3
comes from `resource.cpp`'s `s_resource_tree_child_iid` in those builds.
The uses in function bodies changed only their identifiers.
`direct3drm_device_initialize` (`004781d0`) rises from 98% to 99%. No
function falls.

## 4. What is left in `.rdata`

| Part | Original | Rebuilt | Difference |
| --- | --- | --- | ---: |
| Units' constants, before LIBC | `0049e000`–`0049eb38` | `0049e000`–`0049e9b8` | −384 |
| LIBC | `0049eb38`–`0049f158` | `0049e9b8`–`0049efc8` | −16 |
| GUID block | 6,656 B | 6,656 B, identical | 0 |
| `1.0f` and its padding | `004a0b58`, 8 B | — | −8 |
| `.xdata$x` | 128 B | 128 B | 0 |

The units' 384 bytes are the per-unit constant counts of finding 680,
section 2 (Lws.c 132 short, the rest within ±60 per unit). They belong to
#838 and #841. The 16 LIBC bytes come from LIBC member differences. Nothing
references the `1.0f` after the block. Its object is unknown: it follows
`dxguid.obj`, so it comes from a later library member, or from a late object.

## Before and after

#842 `429c6314` against this change. Both are full matching builds.

| Measure (`binary-progress.json`) | #842 `429c6314` | This change |
| --- | ---: | ---: |
| **Exact-function code bytes** | **351,876 (54.76%)** | **351,876 (54.76%)** |
| Accuracy, full / game-only | 97.78% / 97.55% | 97.78% / 97.55% |
| Functions that rose / fell | | 1 / 0 (`004781d0` 98% → 99%) |
| `.text` identical at original addresses | 6.93% (44,553 B) | 6.98% (44,845 B) |
| `.data` / `.rdata` identical (section-relative) | 28.48% / 9.6% | 28.48% / **23.0%** |
| Identical prefix, raw / masked | 11 B / 7,187 B | 11 B / 7,187 B |
| Correct-size functions, full | 2,811 | 2,811 |
| Units with size agreement | 2 | 2 |
| Fully identical functions at their original address | 42 (2,307 B) | 42 (2,307 B) |
| Entry point (original `0048f000`) | `0048ea60` | `0048ea60` |
| SizeOfImage (original 3,608,576) | 3,596,288 | 3,600,384 |
| `.rdata` size (original 11,232) | 4,664 | **10,824** |
| Data-offset agreement (`layout`) | 22 of 57 units | 22 of 57 |
| Tail roots | 0 | 0 |

The `opt_ref` record shows one 32-byte function moving from
`original_annotated` to `port_unit` in the `/OPT:NOREF` relink. Reccmp's
mapped count is unchanged (3,019 full). The move comes from the measure's
gate-to-relink address translation, not from a lost mapping.

`data_identity`, per region (original-mode link):

| Region | Rebuilt start before → after (original) | Items at original offset | At original address before → after | First item not at its offset (after) |
| --- | --- | ---: | ---: | --- |
| `.rdata` | `0049e000` → `0049e000` (`0049e000`) | 8 of 96 (32 B) → same | 8 → 8 | `0049e048`, −20 |
| `.data` | `004a0000` → **`004a1000`** (`004a1000`) | 192 (3,401 B) → same | **0 → 192 (3,401 B)** | `004a1d5c` (`rock_fall_styles.c`), +132 |
| `.bss` | `004b1d30` → `004b2d30` (`004b2db0`) | 1 (1,040 B) → same | 0 → 0 | `004b31c8` `aiTaskGlobs`, −8 |
| commons | `00546c00` → `00547c00` (`00549520`) | 16 (59,256 B) → same | 0 → 0 | `00556fb8`, −3,848 |

`.rdata`'s rebuilt bytes in `data_identity` (without `.xdata$x`) go from
4,584 to 10,744. `.bss` now starts 128 bytes early (4,224 before). The
commons start 6,432 bytes early (10,528 before).

## Follow-ups

1. The units' `.rdata` constants, 384 bytes short before LIBC (#838, #841).
2. LIBC's 16 bytes, and the unreferenced `1.0f` at `004a0b58`.
3. The MinGW reference build was not tried here (no MinGW toolchain).
   Its units keep their own GUID copies, so its link is unchanged.
