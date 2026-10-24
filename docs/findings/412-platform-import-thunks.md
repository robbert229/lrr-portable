# Finding 412 — Platform import thunks

The 15 assigned entries are named imports rather than engine implementations.
Their decompilation can look like a same-name recursive call because Ghidra
renders the import jump as a call; it does not show a local implementation of
the imported API. The engine-side behavior is visible at callers, while the
DLL's internal behavior is outside this executable's code.

| Address | Imported symbol | Engine-side role and observed call path |
| --- | --- | --- |
| `0x00478000` | `WINMM.DLL::timeGetTime` | Supplies the engine's clock reads. Movie playback stores a start tick and derives AVI frame indices from elapsed time (Findings 68–69); `CDAudioWatchdogTick` (`0x00488c00`) checks elapsed time against a 4000 ms interval before polling/restarting the selected track (Finding 14). Other callsites use it for elapsed-time and tick state. |
| `0x0049b6b0` | `D3DRM.DLL::Direct3DRMCreate` | `Direct3DRMDeviceInit` (`0x004781d0`) creates a D3DRM object, queries `IID_IDirect3DRM3` (`0x0049fbd8`) into `DAT_00505914`, then creates and stores the device/viewport-side interfaces. The returned `IDirect3DRM3` vtable is used by scene and resource helpers throughout the engine. The imported D3DRM library is bundled as `source/D3DRM.DLL`; the import therefore does not imply a dependency on a system D3DRM installation (Findings 15 and 22). |
| `0x0049b6b6` | `D3DRM.DLL::D3DRMVectorRandom` | Called through the thin engine wrapper `FUN_00479500` (`0x00479500`). The import performs the vector operation; the wrapper adds no local state or policy. See Finding 22 for the vector-helper context. |
| `0x0049b6bc` | `D3DRM.DLL::D3DRMVectorRotate` | Called through `FUN_00479510` (`0x00479510`), which forwards the four arguments. The import supplies the rotation operation; the wrapper adds no local state or policy (Finding 22). |
| `0x0049b6c2` | `DDRAW.DLL::DirectDrawEnumerateA` | `DirectDrawEnumerateAndStoreCallbackResult` (`0x0047c1c0`) passes callback `0x0047c1f0` and stores the callback-selected result in `DAT_0076acb0`. Enumeration is part of DirectDraw device selection. See Finding 15. |
| `0x0049b6c8` | `DDRAW.DLL::DirectDrawCreate` | Called by the engine's DirectDraw creation paths, including `DirectDrawDeviceCreate` (`0x0047c2e0`), `DirectDrawEnumeratedDeviceCreate` (`0x0047c4b0`), and `DirectDrawRendererInit` (`0x0047c610`). The callers use the returned interface to query/configure DirectDraw and initialize renderer globals; actual DirectDraw behavior is supplied by the imported library (Findings 15 and 48). |
| `0x0049b6ce` | `AVIFIL32.DLL::AVIStreamInfoA` | `FUN_0047e800` calls this after opening an AVI stream and frame retriever; failure leaves the movie object uninitialized (Finding 69). |
| `0x0049b6d4` | `AVIFIL32.DLL::AVIStreamGetFrameOpen` | `FUN_0047e800` opens a frame retriever for the acquired stream and stores the returned handle before querying stream info. A null handle exits initialization (Finding 69). |
| `0x0049b6da` | `AVIFIL32.DLL::AVIStreamOpenFromFileA` | `FUN_0047e800` opens the `vids` stream from the requested AVI path. Its returned stream handle is retained for subsequent frame operations and later release (Findings 14 and 69). |
| `0x0049b6e0` | `AVIFIL32.DLL::AVIStreamRelease` | The AVI object destructor releases a non-null stream handle when its global AVI-enabled gate permits; the frame retriever is not explicitly closed by an imported close function (Findings 69 and 174). |
| `0x0049b6e6` | `AVIFIL32.DLL::AVIStreamGetFrame` | The AVI update method requests the current frame index, copies a non-null decoded frame into the game's surface, then derives the next index from elapsed time and stream rate (Finding 68). |
| `0x0049b6ec` | `AVIFIL32.DLL::AVIFileInit` | The wrapper `AviFileInitialize` (`0x0047ec80`) first stores its supplied value in `DAT_0053399c`, then calls this import before stream operations (Finding 14). The executable's AVIFIL32 import list has no matching `AVIFileExit`; this alone does not establish the library's process-wide shutdown behavior. |
| `0x0049c7a8` | `DINPUT.DLL::DirectInputCreateA` | `DirectInputInitialize` (`0x0047ed90`) creates a DirectInput object using version `0x500`, stores it in `DAT_0076ab00`, and then uses its vtable to create/configure the input device and cooperative state. The COM-style device operations are outside this import thunk (Finding 15). |
| `0x0049c7ae` | `DSOUND.DLL::Ordinal_1` | The checked-in function manifest names this entry by ordinal; Finding 14 identifies DSOUND ordinal 1 as `DirectSoundCreate`. `FUN_0047a640` calls it during sound initialization, stores the returned DirectSound interface, and then configures it through vtable methods. The name mapping is specific to the shipped import surface. |
| `0x0049c7b4` | `KERNEL32.DLL::RtlUnwind` | Called by `SehFrameUnwind` (`0x0048ec00`) and `__global_unwind2` (`0x0048ef10`) as part of the compiler/runtime structured-exception unwind paths. `SehFrameUnwind` then clears an exception flag and restores the FS-linked exception frame; `__global_unwind2` passes a null exception record and return value. This is not an ordinary engine cleanup API. |

## What the entries establish

The import surface divides into timing (`timeGetTime`), rendering setup
(D3DRM and DirectDraw), AVI stream/frame access, input/sound object creation,
and Windows exception unwinding. The engine controls call order, argument
values, local state, and subsequent interface calls; the external DLLs own the
implementation behind each import.

The evidence supports the caller-visible behavior above, but does not identify
all internal DLL side effects, exact behavior of the DirectSound ordinal on
other binaries, or runtime results under a particular Windows compatibility
layer. The AVI teardown path still has an unresolved explicit frame-retriever
close question (Finding 174).

## Evidence and related findings

The complete Ghidra pseudocode for the assigned entries and their callers is in
[`LEGORR.full.c`](../decompiles/full/LEGORR.full.c); names and addresses are
also recorded in [`manifest.csv`](../decompiles/full/manifest.csv). The existing
coverage ledger classifies these entries as runtime imports, and
`docs/coverage/function-areas.csv` already assigns all 15 to
`platform-import-thunks`, so neither coverage index needs a row change.

Related traces: [Finding 14](14-sound-and-video.md),
[Finding 15](15-compat-audit.md), [Finding 22](22-research-crumbs.md),
[Finding 48](48-directdraw-renderer-setup.md), [Finding 68](68-avi-frame-timebase.md),
[Finding 69](69-avi-stream-open-and-teardown.md), and
[Finding 174](174-avi-frame-retriever-is-not-explicitly-closed.md).
