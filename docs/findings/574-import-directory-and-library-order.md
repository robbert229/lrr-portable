# 574. The import directory: DirectX import libraries, library order and Main.c's clock

Issue #563, a follow-up of finding 563. Measured on `main` at `9b30b563`. The issue's three gaps were the DirectX import libraries the toolchain lacks, the order of the import descriptors and libraries, and `reward_loop`'s `timeGetTime` call.

## Summary

1. **Import libraries.** `dinput.lib`, an ordinal `dsound.lib` and the DirectInput part of `dxguid.lib` are rebuilt from source in `cross/msvc5-importlibs/`, with MSVC 5.0's `LIB /DEF:` and `CL`. No SDK binary is committed.
   - The data members `dilib1.obj` (`c_dfDIMouse`) and `dilib2.obj` (`c_dfDIKeyboard`) compile to `.text` bytes and relocations identical to the DirectX 5 SDK's objects.
   - In the link they land in `.text` between the d3drm thunks and the `DirectInputCreateA` thunk, as in the original (0x49b700–0x49c7a7).
2. **Library order.** The order that fits both the descriptor order and the thunk order is
   `kernel32, libc, oldnames, ole32, d3drm, ddraw, winmm, vfw32, dinput, dsound, user32, gdi32, advapi32`.
   `libc.lib` sits **after** `kernel32.lib` and before the first DirectX library. This replaces ADR 0006 decision 1's thunk-derived order (addendum of 2026-11-12).
3. **`timeGetTime`.** The original's `reward_loop` does not call `[__imp__timeGetTime@0]`, as the issue assumed. It calls `0x00478000`, a Main.c function that compiles to `jmp [__imp__timeGetTime]`. That function is now `main_time_get` in `gods98/frameloop.cpp`, at 100%, and `reward_loop` calls it, rising from 97.25% to 98.28%. Adding `dllimport` would have turned the three 5-byte calls into 6-byte `call [mem]`.
4. **Result.** `make pe-audit` shows the eight DLLs the build imports in the original's descriptor order. The three missing DLLs come back as port code that loads them with `GetProcAddress` is converted.

## Before and after (`make pe-audit`, gate build)

| | Before (`9b30b563`) | After |
| --- | --- | --- |
| DLLs (original 11) | 7 | 8 (`dinput` added) |
| Descriptor order | `d3drm, DSOUND, KERNEL32, WINMM, USER32, GDI32, ADVAPI32` | `KERNEL32, d3drm, WINMM, DINPUT, DSOUND, USER32, GDI32, ADVAPI32` |
| Order of the DLLs both import | different | equal (`common order equal True`) |
| Imported names (original 145) | 151 | 152 |
| `DSOUND` import | `DirectSoundCreate` by name | ordinal 1, as the original |
| `.idata` bytes differing | 3,583 of 4,112 (rebuilt 4,104) | 3,590 of 4,112 (rebuilt 4,154; 564 equal, was 529) |
| `.text` | 1,056,346 | 1,060,644 (+4,264 DirectInput data, +6 thunk, alignment) |
| Import thunks (gate, `/OPT:NOREF`) | 151 | 152 |

`pe_audit.py` now also reports `common_dll_order_equal`, the descriptor order restricted to the DLLs both binaries import, so that it can be checked while DLLs are missing.

## The import libraries

| Library | Source | Built by |
| --- | --- | --- |
| `dinput.lib` | `dinput.def` (`DirectInputCreateA`, by name), `dilib1.c`, `dilib2.c` | `LIB /DEF:dinput.def sdk_exports.obj`, then `LIB` adds the two data members |
| `dsound.lib` | `dsound.def` (`DirectSoundCreate @1 NONAME`) | `LIB /DEF:dsound.def sdk_exports.obj` |
| `dxguid.lib` | `dxguid.c`, compiled once per GUID (`/DDXGUID_GUID_XAxis`, …) | `LIB` of the four objects: `GUID_XAxis`, `GUID_YAxis`, `GUID_ZAxis`, `GUID_Key` |

`cross/msvc5-importlibs/importlibs.cmake` holds the rule. The matching build includes it and writes the libraries to `build-reccmp/msvc5-importlibs/`. The link searches that directory first (`/LIBPATH`), so the rebuilt `dsound.lib` deliberately shadows the toolchain's by-name one. `tools/binary_progress_hash.py` hashes the directory's sources as build inputs.

**Decorated names.** LIB 5.0 alone cannot produce a `__stdcall` symbol with an undecorated import name:
- `DirectInputCreateA@16` in the `.def` imports the name `DirectInputCreateA@16`;
- `DirectInputCreateA` yields the symbol `_DirectInputCreateA`, which a `__stdcall` caller does not resolve.

As in a DLL build, `sdk_exports.c` defines the two functions as `__stdcall`. LIB maps each undecorated export to the decorated symbol it defines, so the members are `_DirectInputCreateA@16` (imports `DirectInputCreateA`) and `_DirectSoundCreate@12` (imports ordinal 1). The object is never linked into the executable.

**The data members.** The original's 4,264 bytes at 0x49b700–0x49c7a7 are:
- `c_dfDIMouse`: 7 object formats, then the `DIDATAFORMAT` at +0x70, 0x88 bytes;
- 8 bytes of `0xCC` alignment fill;
- `c_dfDIKeyboard`: 256 object formats, then the `DIDATAFORMAT` at +0x1000, 0x1018 bytes.

**Provenance.** The values are the public DirectInput 5 layout: `DIDFT_AXIS`, `DIDFT_BUTTON`, `DIDFT_ANYINSTANCE`, `DIDFT_OPTIONAL`, `DIDFT_MAKEINSTANCE(n)` and `DIDF_RELAXIS`, with the documented sizes. They equal the original's bytes there. `#pragma data_seg(".text")` makes MSVC 5.0 emit them in a `.text` section with code attributes and 16-byte alignment, as the SDK objects have.

**Verified:**
- `dilib1.obj` and `dilib2.obj` have the same section size, alignment, symbol offsets, relocations and raw bytes as the DirectX 5 SDK's `dinput.lib` members (in the isle-portable submodule's `3rdparty/dx5`, used only for this comparison).
- In an `/OPT:REF` relink, the 4,282 bytes from `dilib1` to the end of the `RtlUnwind` thunk differ from the original's 0x49b700–0x49c7ba only in relocated addresses: 256 + 3 GUID pointers, 2 `rgodf` pointers and 3 thunk operands.
- The GUIDs equal the original's `.rdata` bytes (`GUID_XAxis` at 0x4a0088, `GUID_Key` at 0x4a0108). The original keeps them 8-aligned in its `.rdata` GUID run, one `dxguid.lib` member each.

**Why the link names them.** No object in the matching build references DirectInput yet: `gods98/input.cpp` reaches it through hooks. `src/link_order.cmake` therefore lists `/INCLUDE:_c_dfDIMouse /INCLUDE:_c_dfDIKeyboard /INCLUDE:_DirectInputCreateA@16` (`LRR_LINK_SYMBOLS`) until Input.c is converted. The order matters: library members are pulled in the order their symbols become unresolved.
- Listing the thunk first gives thunk, keyboard, mouse.
- The order above gives the original's mouse, keyboard, thunk.

## The library order

**Two rules from LINK 5.0** (scratch relinks of the gate's objects, `build-reccmp/CMakeFiles/program.dir/objects1.rsp`):
- **The descriptor order is the order of the import libraries on the command line.** The same holds for `.idata$2`.
- **The thunk order is member pull order.** Libraries are searched in command-line order. A symbol that becomes unresolved only after a library was searched, like LIBC's `RtlUnwind`, is pulled in a later pass.
- Under `/OPT:REF`, which the original used (finding 563), only referenced thunks survive. So only the thunks that objects call directly are evidence: the DirectX ones and LIBC's `RtlUnwind`. Every other Win32 call goes through `__imp_`.

**Evidence.** The original's descriptors are `KERNEL32, ole32, d3drm, DDRAW, WINMM, AVIFIL32, DINPUT, DSOUND, USER32, GDI32, ADVAPI32`. Its `.text` has, in order:
1. all of LIBC except the oldnames members;
2. the thunks d3drm ×3, ddraw ×2, avifil32 ×6;
3. the DirectInput data and the `DirectInputCreateA` thunk;
4. the `DSOUND` #1 thunk, then `RtlUnwind`.

**Relinks.** Each relink used `/OPT:REF` and the three `/INCLUDE`s, plus the rebuilt libraries. The descriptor column lists the DLLs this build imports.

| Order | Descriptors | LIBC before the thunks | Thunk order |
| --- | --- | :---: | --- |
| A: `libc, oldnames, d3drm, ddraw, vfw32, dinput, dsound, kernel32, ole32, winmm, …` (ADR 0006) | `d3drm, dinput, dsound, kernel32, winmm, …` ✗ | ✓ | d3drm, DI data, dinput, dsound, `RtlUnwind` ✓ |
| B: `libc, oldnames, kernel32, ole32, d3drm, …` (descriptor order, LIBC first) | ✓ | ✓ | `RtlUnwind` **first** ✗ |
| C: descriptor order, LIBC as default library | ✓ | ✗ (after the thunks) | ✓ |
| D: descriptor order, `libc oldnames` last | ✓ | ✗ | ✓ |
| **E: `kernel32, libc, oldnames, ole32, d3drm, ddraw, winmm, vfw32, dinput, dsound, user32, gdi32, advapi32`** | ✓ | ✓ | ✓ |
| F: E with `oldnames` last | ✓ | ✓ | ✓ (same image layout as E) |

**Why only E fits.**
- `kernel32.lib` must come first for its descriptor, but its `RtlUnwind` member must not be pulled before the DirectX thunks. LIBC therefore follows `kernel32.lib`: LIBC's reference to `RtlUnwind` appears only after `kernel32.lib`'s first search, so the thunk is pulled in the second pass, after `dsound`.
- LIBC must precede `d3drm.lib` so that its code comes before the thunks.
- `kernel32.lib`'s first-pass thunks precede LIBC, but nothing references them, so `/OPT:REF` drops them.

**What the evidence cannot fix.** Nothing in `ole32.lib` is referenced through a thunk, and LIBC references nothing in it. So `libc` directly after `kernel32` and `libc` after `ole32` give the same image. `oldnames.lib`'s position is not fixed either (E and F agree). The list keeps it after `libc`, where it was.

**The committed link.** It is the gate's command line with E's order, the rebuilt libraries and the `/INCLUDE`s. Relinked with `/OPT:REF`, it gives:
- descriptors `kernel32 d3drm winmm dinput dsound user32 gdi32 advapi32`;
- LIBC at 0x495d20–0x4a26d0;
- then `D3DRMVectorRandom`, `D3DRMVectorRotate`, `c_dfDIMouse`, `c_dfDIKeyboard`, `DirectInputCreateA`, `DSOUND` #1 and `RtlUnwind`, with the original's spacing.

The gate itself still links with `/DEBUG` (`/OPT:NOREF`), so there `kernel32.lib`'s 73 first-pass thunks sit before LIBC until #562 settles the link mode. The gate passed with this order: no function dropped.

## `timeGetTime` and `reward_loop`

**The original.**
- `reward_loop` (0x461410) has three `call 0x00478000` (`E8`).
- 0x478000 is a 6-byte `jmp dword ptr [0x76e594]` (`__imp__timeGetTime`), 16-aligned and followed by `0x90` fill.
- It lies inside Main.c (G98-005, 0x4777a0–0x478df0), between 0x477fd0 and 0x478010.
- Library thunks sit after LIBC and are followed by `0xCC` fill. So 0x478000 is Main.c's own function: `return timeGetTime();` compiled as a tail call through the import.
- The ledger's `WINMM.DLL::timeGetTime` (runtime-import) label for it is Ghidra's thunk name, not an import thunk.

**The rebuild before this change.** `gods98/frameloop.h` redeclared `timeGetTime` without `dllimport` for MSVC. `reward_loop` therefore called the `winmm.lib` thunk: the right instruction, the wrong target.

**Now:**
- `gods98/frameloop.cpp` defines `main_time_get` (`// FUNCTION: LEGORR 0x00478000`). It compiles to the original's 6 bytes.
- `frameloop.h` declares it in place of the redeclaration, and `reward.c` calls it.
- The native port takes `timeGetTime` from `winmm.h`'s emulation, so its time hook still applies.
- `frameloop.cpp`'s legacy key moves from 00478040 to 00478010, the new median. Its position in the list does not change.

**Other declarations.** In the gate build, the only direct calls (`E8`) to a Win32 import thunk are LIBC's: `__global_unwind2` and `_UnwindNestedFrames` call `RtlUnwind`, as in the original. No winmm, kernel32, user32, gdi32 or advapi32 declaration has the same problem.

## Imports still missing

They return as their callers are converted.

| DLL | Missing names | Cause |
| --- | --- | --- |
| `ddraw` | `DirectDrawCreate`, `DirectDrawEnumerateA` | `gods98/surface.cpp` loads them with `GetProcAddress` |
| `d3drm` | `Direct3DRMCreate` | `GetProcAddress` in `platform/winapi/frameloop.cpp` and `gods98/directdraw.cpp` |
| `avifil32` | `AVIFileInit`, `AVIStreamOpenFromFileA`, `AVIStreamInfoA`, `AVIStreamGetFrameOpen`, `AVIStreamGetFrame`, `AVIStreamRelease` | `gods98/movie.cpp` loads them with `GetProcAddress` |
| `ole32` | `CoInitialize`, `CoCreateInstance` | not called by port code |
| `kernel32` | `OutputDebugStringA` | not called by port code |
| `user32` | `MoveWindow` | not called by port code |
| `gdi32` | `GetRegionData`, `CreateRectRgn` | not called by port code |

Port code also adds names the original does not import: `winmm`'s nine `waveOut*`, four in `kernel32`, eight in `user32` and `StretchDIBits`.

## Gate

`make reccmp-check` passes. Two baseline entries change:
- `reward_loop` (0x461410) rises from 97.25% to 98.28%;
- `main_time_get` (0x478000) is new at 100%.

No function drops: 2,235 game-only functions mapped (+1), 2,471 in the full report (+1). The library order moves LIBC and the thunks, and no CRT score changes.

## Reproducing

```sh
make reccmp-check && make pe-audit      # descriptors and "common order equal True"
```

**A scratch relink:**
1. Copy `build-reccmp/CMakeFiles/program.dir/link.txt`.
2. Redirect `/out`, `/pdb`, `/implib` and `/MAP` to a scratch directory.
3. Append `/OPT:REF`, and change the library list for each variant.
4. Run it from `build-reccmp/` in the toolchain environment.

Read the descriptor order from the PE. Read the thunk order from the map's `0001:` symbols whose object is a `*.dll` import member, and from the `dinput:dilib*.obj` entries.
