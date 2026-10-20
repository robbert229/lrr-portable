# 15 — Modern-Compatibility Audit (A.1): Imported API Surface

*Target: `LegoRR.exe` (SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`), imports + Ghidra 12.1.4 via REA.*

## Purpose

Decide, before any live testing (A.2), how much of the binary can run under
modern-system compatibility layers (dgVoodoo2 / DDrawCompat / Wine+DXVK)
versus needing a real port. This is the *import-level* survey; everything
beyond `Create*` entry points is COM vtable traffic the wrappers own.

## 1. Import surface (objdump, VA = import-table entry + `0x400000`)

| DLL | Imports | Role (evidence) |
| --- | --- | --- |
| `DDRAW.dll` | `DirectDrawCreate` (hint 7), `DirectDrawEnumerateA` (hint 9) — **only two**; all other DDraw use is vtable calls on the created object | device init module `0x47c2e0` (158 B), callers `FUN_0049d030/0049d780` |
| `d3drm.dll` | `Direct3DRMCreate`, `D3DRMVectorRotate`, `D3DRMVectorRandom` — bundled **in the archive** (`source/D3DRM.DLL` present), so no OS optional-component dependency | 314 B creator `FUN_004781d0`, single caller `FUN_0047c610` (inside the device-init region) |
| `DINPUT.dll` | `DirectInputCreateA` (hint 0) — **pre-`DInput8`** API | thunk `0x49b6xx` region |
| `DSOUND.dll` | ordinal 1 = `DirectSoundCreate` (v1) only | `FUN_0047a640` ← `FUN_00488b50` (finding 14) |
| `WINMM.dll` | `mmio*` (9), `mixer*` (6), `timeGetTime`, `mciSendStringA`, `mciGetErrorStringA` | sound RIFF parsing + mixer volume + **CDDA music** (finding 14) |
| `AVIFIL32.dll` | `AVIFileInit`, `AVIStream{OpenFromFileA,InfoA,GetFrameOpen,GetFrame,Release}` | movies module `FUN_0047e800` (finding 14) |
| `KERNEL32` | routine + `GetVersion`, `SetUnhandledExceptionFilter`, CRT `IsBad*Ptr` | see §2 |
| `USER32/GDI32` | 39 window/GDI calls incl. one `GetDeviceCaps` (mode caps probe at `0x477f8f`) | standard |
| `ole32` | `CoInitialize`, `CoCreateInstance` | standard |
| `ADVAPI32` | `RegCreateKeyA`, `RegQueryValueExA`, `RegCloseKey` — save/settings keys only | standard |

**No `D3D*.dll` import at all** — consistent with findings 11/13: the 3D path
runs through the bundled `d3drm.dll` and DDraw surfaces; raw D3D is never
talked to directly. Dev-string cluster confirms the vintage:
`C:\Dev\SourceSafe\gods98_dx6\gods98\src\{Main,Viewports,Images,Input}.c`
(`0x4aaba4–0x4aaed0`) — **DirectX 6 SDK era**, 1998–99.

## 2. Launch-gate scan

* `GetVersion` (`0x48f026`): result is **decomposed into globals**
  (`DAT_005490dc` = major, `DAT_005490e8/4` = minor/build) with **no
  conditional gate in this function** — no NT refusal, no version branch.
* No `GetVersionEx`, no `VerifyVersionInfo`, no `IsProcessorFeature…`
  imports.
* CD-key checks are **file-based** (`cd.key`, `delme.dat`, mount of
  `%s%i.wad`, finding 01) — an install-directory concern, not an API gate.
* Registry usage is read/write of the game's own keys only.

**Verdict: no OS-version launcher blocker found.**

## 3. Wrapper-coverage matrix (static view)

| Surface | dgVoodoo2 | Wine (ddraw→DXVK) | Notes |
| --- | --- | --- | --- |
| DDraw `Create`+vtables (DX6-era GUIDs) | yes | yes (mature) | vtable-level: `SetDisplayMode`, flips, overlays untested statically |
| `d3drm.dll` (bundled) | yes (D3DRM→D3D9) | needs the bundled DLL under its own DDraw | OS copy is long removed — shipping was lucky |
| DInput (`DirectInputCreateA`) | yes | yes | legacy path, not dinput8 |
| DSOUND ordinal 1 | yes | yes | mixer APIs also covered |
| `AVIFIL32` | partial | partial | old codecs (MSRLE/MSVC) absent on modern hosts |
| MCI `play cdaudio` | **no** (no CD) | no | the one hard functional blocker |

## 4. Expected friction, ordered by evidence strength

1. **CDDA music** (finding 14: `"play cdaudio from %i"` @`0x489210` + 4 s
   watchdog): guaranteed dead on modern systems. Mitigation is small: either
   patch the 80-byte MCI wrapper to `PlaySound`/DSOUND, or accept silent
   levels. Everything else degrades gracefully.
2. **AVI codecs** (finding 14 §4): intro/loop movies may decode black if
   the host lacks RLE/MSVC codecs. Built-in escape hatch: `DontPlayAvis
   TRUE`; or re-encode the four `.avi`s as uncompressed RGB via `rrwad`.
3. **Fixed 640×480 surface** (config positions hardcoded for it,
   `lmi640.avi`): exclusive-mode 4:3 — exactly what wrappers emulate.
4. DInput exclusive mode / mouse grab: standard wrapper territory.
5. Palette textures (`BM8` tags in the archives): DDraw palettes, wrappers
   convert — but this class historically hides fidelity bugs, so a first-run
   visual check of `World\WorldTextures\*Split\*` is worthwhile.

## 5. Open items before/alongside A.2

1. **COM GUID versions**: which `IDirectDrawN` the code queries (2 vs 4)
   changes which `IDirectDrawSurface` it uses; visible only as GUID data
   operands near `0x47c2e0`/device globals. A 15-min Ghidra pass.
2. **D3DRM device flavor**: HAL vs software `CreateDevice` — decides whether
   the 3D view runs accelerated through the wrapper or in software (perf).
3. Exception handler: `SetUnhandledExceptionFilter` + the `Missing config`
   style dialogs mean crashes *report* — useful when we run it.

### Confidence

| Claim | Status |
| --- | --- |
| Import lists / entry points | observed (objdump + xrefs) |
| No version gate in binary | observed (GetVersion xref, single site, no branch) |
| Renderer = DDraw + bundled d3drm, no D3D import | observed (import table + findings 11/13) |
| dx6 SDK era | observed (dev-path strings) |
| GUID/interface versions, D3DRM device flavor | **open** (§5) |
| Wrapper coverage column values | inferred from public wrapper docs — verify in A.2 |
