# Finding 479 — Renderer D3D/D3DRM Interface Vtable Slots

This finding resolves the scene/render vtable slots flagged by issue
[#254](https://github.com/robbert229/lrr-portable/issues/254) out of finding
[416](416-scene-and-render-resource-lifecycle.md): building an
address- and call-site-backed slot→method map for every interface-method call
made through the three renderer interface globals `DAT_00505914`,
`DAT_00505918`, and `DAT_0050591c`. All nine slots previously listed as
unresolved in finding 416's render-resource route are now named, and one prior
mis-attribution (`+0x68` = `MultiplyTransform`) is corrected to
`GetTransform`.

**Target and evidence.** Static analysis of shipped `reference/LegoRR.exe`
(SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`)
and the game-shipped `reference/D3DRM.DLL` (Jan 8 1999, DX6-era build), which
the game loads to service every D3DRM call. Canonical function bodies and call
sites are in `docs/decompiles/full/LEGORR.full.c`; all call-site line numbers
below refer to that file. D3DRM.DLL was parsed with the PE format and
disassembled (x86-32); every slot function and its internal implementation is
cited by virtual address. Interface method ordering is checked against DX7-SDK
headers (`d3drmobj.h`, `d3drm.h`, `d3d.h`), flattened with the IUnknown prefix
included. Evidence (what the binary does) is separated from inference (what the
slot is called) throughout.

## Interface identity

### The manager (`DAT_00505914`) is an IDirect3DRM3

`Direct3DRMDeviceInit@0x004781d0` obtains the manager through the exported
D3DRM entry (line 78369) with QueryInterface IID at `0x0049fbd8`, previously
identified as `IID_IDirect3DRM3` (`{4516ec83-8f20-11d0-9b6d-0000c0781bc3}`).
Every slot the game calls on `DAT_00505914` lies inside the 41-slot
IDirect3DRM3 vtable from `d3drm.h` (IUnknown `+0x00/04/08`, then
`CreateObject +0x0c` … `GetOptions +0xa0`).

### The device (`DAT_00505918`) is the D3DRM device from the shipped D3DRM.DLL

`DAT_00505918` is written only by the out-parameter of
`IDirect3DRM3::CreateDeviceFromSurface` (`+0x3c`, line 78373). The shipped
D3DRM.DLL exposes a device **class-factory table** at `0x647822c0` that maps
each device interface GUID to the vtable QI installs for that interface:

| Interface GUID (D3DRM.DLL) | Identity | Vtable installed |
| --- | --- | --- |
| `0x64781660` | `IID_IDirect3DRMDevice` | `0x647df068` (73 slots) |
| `0x64781670` | `IID_IDirect3DRMDevice2` | `0x647df0f0` |
| `0x64781680` | `IID_IDirect3DRMDevice3` | `0x647df190` (83 slots) |
| `0x64781640` | `IID_IDirect3DRMObject` | `0x647df068` |

All three device views share the same slot functions for every called offset
except `+0x54`, `+0x98`, `+0x9c`. `DAT_00505918` carries the **Device3 view
(`0x647df190`)** — the decisive check is `+0x9c`: the view3 slot
(`0x6478619b`) does `ret 0x10` (caller pops `this` + 3 arguments), exactly the
game's call shape `(device, 0x400, 0, &pixelfmt)` (lines 74460, 82610),
while the view1 slot (`0x647924ab`) does `ret 0xc` (`this` + 2) and view2 has
no `+0x9c` entry. The view3 slot's implementation (`0x647a060b`) validates
format-selection bitfields and returns a pixel format — see the table below.

### The D3D handle (`DAT_0050591c`) is an IDirect3DDevice3

`DAT_0050591c` is obtained at line 78376 by QueryInterface-ing the D3DRM
device's backing surface with the IID at `0x0049f6d8` —
`IID_IDirect3DDevice3` (`{b0ab3b60-33d7-11d1-a981-00c04fd7b174}`), matching the
same GUID constant found in D3DRM.DLL at `0x64781680`'s sibling block. Every
slot the game calls lies inside the 42-slot IDirect3DDevice3 vtable from
`d3d.h` (`+0x54/58` Get/SetRenderState … `+0xa0/0xa4` SetTextureStageState /
ValidateDevice); none exceeds it, which retires the earlier "exceeds Device2"
concern — the interface is Device3, not Device2.

## Slot maps

### `DAT_00505914` — IDirect3DRM3 (game PEs only; manager lives in D3DRM.DLL)

| Slot | Method (d3drm.h) | Call sites (LEGORR.full.c) |
| --- | --- | --- |
| `+0x10` | `CreateFrame` | 73221, 73223, 73225, 76106, 76768, 92971, 93814, 94114 |
| `+0x14` | `CreateMesh` | 73799, 74834 |
| `+0x18` | `CreateMeshBuilder` | 76916 |
| `+0x24` | `CreateAnimationSet` | 76770 |
| `+0x30` | `CreateLightRGB` | 73757 |
| `+0x34` | `CreateMaterial` | 93719 |
| `+0x3c` | `CreateDeviceFromSurface` | 78373 |
| `+0x48` | `CreateTextureFromSurface` | 74651 |
| `+0x50` | `CreateViewport` | 77129 |
| `+0x58` | `CreateUserVisual` | 86467, 86539 |
| `+0x5c` | `LoadTexture` | 74612 |

All 11 observed slots are resolved. Naming is direct: the offsets match
IDirect3DRM3 slot order and each call context agrees (for example `+0x3c` is
the only device-creation call, matching its role as the single writer of
`DAT_00505918`).

### `DAT_00505918` — D3DRM device, Device3 view vtable `0x647df190`

| Slot | Method (name per DX7 IDirect3DRMDevice3 order) | Slot fn | Impl | Key evidence | Call sites |
| --- | --- | --- | --- | --- | --- |
| `+0x38` | `Update` | `0x647857cf` | `0x6479ff51` | long per-frame device routine; 1 arg (`ret 4`) | 78229 |
| `+0x44` | `SetBufferCount` | `0x64785808` | `0x6479fd38` | impl validates arg ≥ 1 (`cmp edi,1 / jge` else D3DRMERR `0x88760316`), iterates buffer list at `obj+0x94`; call shape `(dev,2)` = double buffering | 78379 |
| `+0x4c` | `SetDither` | `0x64785ddf` | `0x647940bb` | impl stores arg to `obj+0x60` then calls rebind `0x647b99d3` | 78435 |
| `+0x54` | `SetQuality` | `0x64785e59` | `0x64794194` → `0x6479415d` | view3 wrapper delegates to view1 impl, which dispatches through sub-object vtable `[obj+0x18]+0x34` with the value | 78436 |
| `+0x58` | `SetTextureQuality` | `0x64785e96` | `0x647941b9` | impl writes `obj+0x74` only when changed, then walks texture lists at `obj+0x654`/`obj+0x720`; call arg is 0/1/3+ (texture-quality grade) | 78437 |
| `+0x68` | `GetHeight` | `0x64785a54` | `0x6479435a` | impl returns `[obj+0x40]`; game feeds result through `__ftol` float scaling (77086, 77113) | 77086, 77113 |
| `+0x6c` | `GetWidth` | `0x64785f0c` | `0x64794386` | impl returns `[obj+0x3c]`; same ftol-scaling usage | 77084, 77112 |
| `+0x98` | `GetDirect3DDevice2` | `0x64786133` | `0x6479451f` | impl AddRefs and returns the internal D3D device at `obj+0x5c4` (raises `0x8876030d` if absent); sits exactly between `GetRenderMode` (`+0x94`) and `FindPreferredTextureFormat` (`+0x9c`) | 78375 |
| `+0x9c` | `FindPreferredTextureFormat` | `0x6478619b` | `0x647a060b` | impl validates `arg1 & 0xffffe0ff` and `arg2 & 0xfffffff0` (accepts the game's `0x400, 0`), returns E_INVALIDARG `0x80070057` on bad args, fills a 0x20-byte pixel-format record; call shape `(dev,0x400,0,&fmt)` with retries | 74393, 74460, 82604, 82610 |

The `+0x9c` call sites are the CreateBmpSurface and QueryTextureMem pixel-format
probes: first call returns the format (or a status), then `(dev, 0x400, 0,
&DDPIXELFORMAT-sized buffer)` fills it, and the game reads `buffer[3]` as a
16-bit/32-bit bit depth (e.g. line 74465 `aiStack_50[3] == 0x10`). This is the
behavioral signature of `FindPreferredTextureFormat` and is consistent with the
IDirect3DRMDevice3 slot ordering from `d3drmobj.h`.

### `DAT_0050591c` — IDirect3DDevice3 (d3d.h, 42 slots)

| Slot | Method (d3d.h) | Call sites | Shape notes |
| --- | --- | --- | --- |
| `+0x2c` | `GetDirect3D` | 86338 | gets the owning IDirect3D (`DAT_00540580`), then used to build the shared texture root |
| `+0x30` | `SetCurrentViewport` | 88676, 88718 | viewport comes from `*(param_1+0x10)`; decompile shows extra register args, ignore |
| `+0x54` | `GetRenderState` | 78719, 88843, 88850 | `(dev, 1, &DAT_00540564)` cached-state read pattern |
| `+0x58` | `SetRenderState` | 78721, 78748, 88845, 88871, 88876 | write/clear of cached state `DAT_00540564` (+ tables `DAT_00505940/44`) |
| `+0x5c` | `GetLightState` | 88838 | `(dev, 1, &DAT_00540568)` |
| `+0x60` | `SetLightState` | 88864, 89032 | `(dev, 1, DAT_00540568)` restore |
| `+0x64` | `SetTransform` | 88787 | `(dev, state, &matrix)` second half of MatrixMethodPairApply |
| `+0x68` | `GetTransform` | 88779 | `(dev, state, &matrix)` first half of MatrixMethodPairApply — **corrects** the earlier `MultiplyTransform` attribution |
| `+0x74` | `DrawIndexedPrimitive` | 89041–42 | `(dev, 4, p3…p7)` — primitive type 4 = triangle list, direct draw call in `Direct3DDeviceStateApply` |
| `+0x94` | `GetTexture` | 88841 | `(dev, 0, &DAT_0054056c)` cached texture read |
| `+0x98` | `SetTexture` | 88868, 89004 | `(dev, 0, DAT_0054056c)` restore / clear |
| `+0x9c` | `GetTextureStageState` | 88809 | `(dev, 0)` stage-0 read |
| `+0xa0` | `SetTextureStageState` | 88815 | `(dev, 0, state, &value)` stage-0 write |

All 13 observed slots on this interface are resolved. The render-state /
light-state / texture / texture-stage-state grouping is confirmed by the three
cached D3D state globals (`DAT_00540564/568/56c`), which pair one-to-one with
the Read+Set slot pairs above.

## Evidence vs inference

Direct (binary-observed) evidence: the D3DRM.DLL class-factory table and the
three device-view vtables at `0x647df068/0x647df0f0/0x647df190`; every slot
function and implementation address in the tables above; impl behaviors (field
offsets `obj+0x3c/40/60/74`, texture-list walks, format-bitfield validation,
`ret` sizes); the game's call sites, shapes, and cached-state globals; the two
QI IIDs (`0x0049fbd8` = IDirect3DRM3, `0x0049f6d8` = IDirect3DDevice3).

Inference: the *names* of the shipped-DLL device slots are assigned by aligning
the slot functions with DX7-era interface ordering — sound because the shared
core slots (`Update`, `SetBufferCount`, GetHeight/GetWidth via field reads)
match unambiguously, and the two high slots sit in the exact Device3-header
positions (`GetDirect3DDevice2` then `FindPreferredTextureFormat`) with
matching signatures. The `+0x4c/0x54/0x58` = dither/quality/texture-quality
ordering likewise follows the header where `+0x58`'s texture-field behavior
independently confirms it.

## Explicit gaps

1. **D3DRM.DLL build skew.** The shipped DLL (Jan 8 1999) predates the DX7 SDK
   headers used for naming. Its per-view device vtables are longer than any
   public interface (73–83 slots) and their tails are not self-identifying;
   only the slots the game exercises are named here. Slots `+0xa0 … +0x148`
   (view3) and view1/view3 tail divergence beyond `+0x9c` remain unnamed.
2. **`+0x54` view discrepancy.** The view1 slot `0x64785a17` and view3 slot
   `0x64785e59` differ; the game sees the view3 one (Device3 attribution).
   Both dispatch to the same internal `0x6479415d`, so naming `SetQuality`
   holds for either, but the view1 address is kept on record.
3. **State-id semantics.** The D3D state/layer IDs `1` (render, light) and stage
   `0` (texture) are passed through to the driver; which D3D enum each `1`
   denotes is not resolved here and would require driver-side tracing.
4. **`+0x98` argument count.** The game's init call (line 78375) decompiles as
   `(dev)` (1 argument) while the view3 slot also consumes an out-parameter;
   the extra argument is either dropped by decompilation or tolerated by the
   driver path. Behavior (AddRef'd D3D device returned from `obj+0x5c4`) is
   unchanged either way.

## Corrections to earlier analysis

- Finding 416 marked the `DAT_00505918` slots (+0x38…+0x9c in its render
  route) as unresolved interface offsets. Finding 479 names all of them.
- An earlier sweep read `+0x68` on `DAT_00505918` as `GetHeight`/`GetWidth`
  (correct; field reads `0x40`/`0x3c` confirm) but labeled `DAT_0050591c
  +0x68` as `MultiplyTransform`. The Device3 `d3d.h` order and the
  get→modify→set pairing inside `MatrixMethodPairApply` (88779 then 88787)
  identify it as `GetTransform` with `SetTransform` at `+0x64`.
- `DAT_0050591c` is IDirect3DDevice3 (per its QI IID), not Device2; the
  observed `+0x9c/+0xa0` stops being an outlier once Device3's 42-slot table
  is applied.
- The mystery IID at `0x0049fb48` from finding 416 is `IID_IDirect3DRMUserVisual`
  (see issue [#253](https://github.com/robbert229/lrr-portable/issues/253)),
  unrelated to these device slots.

## Follow-ups

- Issue [#254](https://github.com/robbert229/lrr-portable/issues/254): the
  renderer slot map is now complete for all three interface globals; remaining
  unnamed slots require driver-side (D3DIM) imports, which are not shipped in
  `reference/`.
- The IDirect3D handle produced by `GetDirect3D` (`DAT_00540580`) and its
  `+0x14` usage in `SharedTextureRootInitialize` is a separate IDirect3D-object
  route left for a later finding.
