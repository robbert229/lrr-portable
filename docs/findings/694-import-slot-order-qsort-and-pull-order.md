# 694. Import slot order within each DLL: LINK 5.0's qsort, the runtime it runs on, and the pull order

Part of the identity work in PR #861. Follows findings 574 and 693.
Measured with scratch links and full relinks of the matching build's
objects (`build-reccmp/CMakeFiles/program.dir/link.txt`).

## Summary

1. **The rule.** LINK 5.0 builds each DLL's import address table
   (`.idata$5`, and the lookup table `.idata$4` in step) by sorting the
   `.idata$5` contribution of every import-library member it loaded with the
   C runtime's `qsort`:
   - the input is load order, as `/VERBOSE` prints it (`Found …` then
     `Loaded lib(DLL)`), the members' null thunks included;
   - the key is the member's DLL name alone, compared case-sensitively, so the
     DLLs come out in ASCII order (`ADVAPI32 … WINMM`, then `d3drm`, `ole32`);
   - imports of one DLL compare equal, so their order is whatever `qsort`
     leaves. The null thunk ties with its DLL's imports, and LINK writes the
     terminator last wherever it sorted;
   - under `/OPT:REF` the unreferenced slots are dropped after the sort, so
     members that only removed code references still take part.

   Hint/name strings (`.idata$6`) and descriptors (`.idata$2`) are not
   sorted. `.idata$6` keeps load order, so **the original's `.idata$6` shows
   its pull order**.
2. **The runtime matters.** `link.exe` imports `qsort` from `MSVCRT.dll`.
   Under Wine that is Wine's builtin, whose `qsort` permutes equal keys
   differently from Microsoft's MSVCRT 5.00. A link under Wine therefore
   cannot give the order a link on Windows gives, whatever its inputs. The
   toolchain's own `msvcenv.sh` says Wine's builtin `msvcrt` does not perturb
   5.0 output; that is not true of the IAT.
3. **The matching build now links on the toolchain's own MSVCRT 5.00**
   (`cross/msvc5-native-crt-link.sh`, `cross/msvc5_native_crt.py`). Copies of
   `link.exe` and the DLLs it loads are staged in
   `build-reccmp/msvc5-native-crt/`, with `msvcrt.dll` renamed `MSVCR5.dll`
   and their imports pointed at it. Wine itself keeps its builtin `msvcrt`.
   Nothing else in the image changes: `.text`, `.data` and the map's layout
   are identical. Only the IAT slots and the `[__imp_]` operands that address
   them move. The map lists symbols that share an address in another order,
   since the map is sorted with `qsort` too.
4. **The original's pull order is ours, with two link-order differences**
   (from its `.idata$6`):
   - KERNEL32's first search pulled only `Sleep` and `MultiByteToWideChar`,
     the explicit objects' imports. Main.c's seven imports came with LIBC's,
     so `kernel32.lib` was searched before the gods98 library.
   - That second KERNEL32 run comes before USER32's first run. So
     `kernel32.lib` was searched again between `dsound.lib` and `user32.lib`
     in LINK's first library loop.

   The matching build now links that way. LINK reads a library name once
   (case-insensitively), so the second search names the toolchain's
   `kernel32.lib` by its full path. Its `.idata$6` then equals the
   original's except for two gods98 imports (below). Slot agreement rises
   from 12 (Wine) and 8 (native, old order) to 39 of 145.
5. **What remains is not visible in the image.** With the original's exact
   visible pull order, Microsoft's `qsort` gives 27 to 39 of 145 slots. The
   rest depends on members the original pulled whose slots `/OPT:REF` then
   removed: they take part in the sort but leave no IAT slot and no
   `.idata$6` string. Greedy searches for such "ghost" members reach 102 to
   112 slots but do not converge on a consistent set.
6. **Fourteen ghost members give 145 of 145.** A search over per-run ghost
   counts converges on one family of 14 (section *Ghost import members*).
   The matching build now carries them as unused header inline copies,
   together with gods98's visible pull order, and every DLL's slots equal
   the original's. Only their counts and positions are evidenced; most
   names are placeholders.

## The model, step by step

`tools/iat_order.py predict LOG --crt microsoft|wine --exe EXE` applies the
rule to a `/VERBOSE` link log; `tools/iat_order.py slots ORIGINAL REBUILT`
measures slot agreement per DLL. Both `qsort`s are transcribed there:
Microsoft's (VC 5 `qsort.c`: `CUTOFF` 8, the middle element swapped to the
front, an unstable partition) and Wine 11's (`dlls/msvcrt/misc.c`,
`quick_sort`: median of three, its own partition).

| Probe | Imports | Result |
| --- | --- | --- |
| one object, `/NODEFAULTLIB`, KERNEL32 only | 6 | IAT = load order rotated by one (Microsoft's `shortsort` on equal keys) |
| same, 7 to 22 imports | 7–22 | from 8 imports on, element 0 and element (n+1)/2 swap: `qsort`'s pivot over the imports plus the null thunk |
| KERNEL32 and USER32, both library orders | 3 + 3 | with the null thunks as equal keys, exact in both; with the null thunk sorting last, wrong in one |
| KERNEL32, USER32 and GDI32, shuffled calls | 16 + 16 + 2 | exact |
| full relink, native runtime (157 contributions, 11 null thunks) | 145 | exact, 145 of 145 |
| the gate's Wine link, Wine's `qsort` | 145 | exact, 145 of 145 |

In the 20-import probe the native link swaps elements 0 and 10 (Microsoft's
pivot), and the Wine link leaves the order alone. That is the difference
behind summary item 2. Loading `MSVCRT.dll` as `MSVCR5.dll` makes the probe
fail when the renamed file is removed, so the staged copy is the one used.

## The original's pull order

Read from the original's hint/name strings in address order (the RVAs its
import lookup table holds); `DSOUND` imports by ordinal and has none:

```
Sleep MultiByteToWideChar KERNEL32.dll
CoCreateInstance CoInitialize ole32.dll   Direct3DRMCreate D3DRMVectorRandom D3DRMVectorRotate d3drm.dll
DirectDrawEnumerateA DirectDrawCreate DDRAW.dll   timeGetTime mixer… mmio… mci… WINMM.dll
AVIStreamInfoA … AVIFileInit AVIFIL32.dll   DirectInputCreateA DINPUT.dll   DSOUND.dll
GetCommandLineA GetLastError CreateMutexA GlobalFree GlobalAlloc OutputDebugStringA GetDriveTypeA CloseHandle
  HeapFree … SetEnvironmentVariableA            (LIBC, in LIBC member order)
DestroyWindow … wsprintfA USER32.dll   GetDeviceCaps … GDI32.dll   RegCloseKey … ADVAPI32.dll
DialogBoxParamA EndDialog SendMessageA GetDlgItem MoveWindow EnableWindow   (Init.c, second pass)
```

Each DLL name follows its library's first run, because LINK loads the
descriptor member after the imports that reference it.

Before this change, the matching build's `.idata$6` differed from this in
three places:
- Main.c's seven KERNEL32 imports came in the first run, because
  `unit_gods98.lib` was searched before `kernel32.lib`;
- the USER32, GDI32 and ADVAPI32 runs came before LIBC's KERNEL32 run, which
  waited for LINK's second library loop;
- gods98 references `OutputDebugStringA` after `GetDriveTypeA`, where the
  original has them the other way round. Also, no gods98 unit references
  `CloseHandle` (LIBC pulls it later); in the original a gods98 unit
  references it right after `GetDriveTypeA`.

The first two are link order, fixed here. The third is source order in
gods98. In the scratch model it alone lowers the agreement from 39 to 27;
it is fixed together with the ghost members (section *Ghost import
members*).

## Link order

`src/link_order.cmake`:
- `LRR_LINK_FIRST_LIBRARIES` (`kernel32.lib`) is searched after the late
  libraries and before `unit_gods98.lib`;
- `LRR_LINK_LIBRARIES` names `kernel32.lib` again before `user32.lib`, and
  `CMakeLists.txt` passes that second one by the toolchain's full path.

LINK pulls the same members:
- `kernel32.lib` has no commons (finding 690);
- the entry point is still inferred after the explicit objects (finding 658);
- under `/OPT:REF` the first run's two thunks are dropped as before.

The descriptor order is unchanged, because the descriptors follow each
library's first search.

One thing moves, toward the original. RtlUnwind's thunk is now pulled in
the first pass, right after DSOUND's. That is where finding 574 found it in
the original (`0049c5e4`, the original's `0049c7b4` less the region's
existing 0x1d0 deficit). Before, it came after the late units. LIBC's
second-pass members after the late units (`strnicmp`, `xtoa`, …) move 16
bytes earlier, and `.text` is 16 bytes shorter. No score changes.

## Slot agreement

| Build | KERNEL32 | USER32 | WINMM | All (145) |
| --- | ---: | ---: | ---: | ---: |
| Wine runtime, old order (base) | 1 | 1 | 1 | 12 |
| native runtime, old order | 0 | 0 | 2 | 8 |
| native runtime, this link order (gate) | 24 | 1 | 1 | 39 |
| with the ghost members below | 70 | 34 | 20 | 145 |

The middle row is lower than the base, but both differences are accidents of
two `qsort`s. Only a native-runtime link can reach the original's order.

`make reccmp-check` passes after each change, with no function below the
baseline. The means are unchanged: game-only 98.52%, full 98.66%.

## Ghost import members

With the original's visible pull order, 14 ghost import members give
**145 of 145** slots: members whose only references are code `/OPT:REF`
removed. They take part in LINK's `qsort` and then lose their slots and
`.idata$6` strings, so the image shows only their effect on the order.

**How the set was found.** A ghost member of a library can load only when
LINK searches that library, which gives 24 runs. Only the number of ghosts
in each run changes the permutation, because the `qsort` sees only the key
sequence. Placement within a run was then checked exactly by backtracking
(a scratch search over `iat_order.py`'s model; its programs are not kept).
- An exhaustive search over up to three ghosts reached 111 slots at most.
- Annealing with exhaustive pair moves found 145 in 10 of 96 restarts, all
  with the same per-run counts.
- From that vector, no 13-ghost neighbour reaches 145. A breadth-first
  search over single moves at 14 ghosts finds exactly four vectors.

The search above three ghosts is not exhaustive over every count vector,
but every independent search converged on this family.

| DLL | Ghosts | Allowed positions (undefined-symbol order) |
| --- | ---: | --- |
| KERNEL32, gods98 | 7 | before crt0dat.obj's `GetCurrentProcess`; any spread over gods98's references and LIBC's up to crt0dat |
| KERNEL32, LIBC | 2 | after `GetCurrentProcess`, before testfdiv.obj's `GetProcAddress`; at least one after getcwd.obj's `GetFullPathNameA` |
| d3drm | 1 | after Maths.c's `D3DRMVectorRandom` |
| DDRAW | 1 | after DirectDraw.c's `DirectDrawCreate` |
| AVIFIL32 | 1 | straight after `AVIFileInit` |
| USER32 | 1 | first USER32 run, after Main.c's `CreateWindowExA`, no later than SoundSys.c's `wsprintfA` |
| one of four | 1 | ADVAPI32 after `RegCloseKey` in the registry unit's run; ADVAPI32 or AVIFIL32 in the second loop; or GDI32 |

Ghosts are not allowed in the explicit objects' KERNEL32 run, ole32, WINMM,
DINPUT, DSOUND or Init.c's USER32 run.

**What the matching build uses.** Each reference is an unused
`extern __inline` copy in a small header included after its unit's last
function (finding 693's form); `/OPT:REF` removes the copy:

| Header | Ghost (and DLL) | Evidence for the name |
| --- | --- | --- |
| `g98_029_tail.h` | `RegSetValueExA` (ADVAPI32) | good: the unit queries values and never sets one |
| `avi_player_tail.h` | `AVIFileExit` (AVIFIL32) | good: the unit calls `AVIFileInit` and never `AVIFileExit` |
| `files_tail.h` | `_rmdir`, `rename` (LIBC: `RemoveDirectoryA`, `MoveFileA`) | good: Files.c wraps `remove`, `_chmod`, `_mkdir`, `_findfirst` and `_getcwd`; neither member pulls anything new |
| `maths_tail.h` | `D3DRMVectorModulus` (d3drm) | placeholder; Maths.c wraps `D3DRMVector*` functions |
| `directdraw_tail.h` | `DirectDrawCreateClipper` (DDRAW) | placeholder |
| `input_tail.h` | `GetKeyState` (USER32) | placeholder |
| `main_tail.h`, `sound3d_tail.h`, `files_tail.h` | seven KERNEL32 imports | placeholders: `GetTickCount`, `QueryPerformanceCounter`, `QueryPerformanceFrequency`, `GlobalLock`, `GlobalUnlock`, `GetVolumeInformationA`, `GetFileSize` |

Only the counts and positions are evidenced. The names leave no trace in
the image. LIBC carriers that were rejected:
- `time.obj` pulls tzset.obj early and moves the visible
  `GetTimeZoneInformation` (109 slots);
- `chdir.obj` and `drive.obj` pull `SetEnvironmentVariableA` and
  `_toupper`.

**The visible order.** The same change fixes gods98's pull order:
- `input_tail.h` makes Input.c, the object before Files.c, reference
  `OutputDebugStringA`, since Files.c's own reference (File_Error) follows its
  `GetDriveTypeA` (File_FindDataCD);
- `files_tail.h` makes Files.c reference `CloseHandle`.

Which gods98 object the original's earlier `OutputDebugStringA` came from is
not known.

**Compiler fact used for placement** (scratch compile, `/O2`). An object
lists its undefined externals function by function, in definition order, and
within one function in *reverse* call order. Example: registry_path_create_keys
calls Query, Create and Close, and the object lists `RegCloseKey`,
`RegCreateKeyA`, `RegQueryValueExA`. MSVC 5.0 emits an unused
`extern __inline` copy in C++ as in C (avi_player.cpp).

**Slots, one change at a time** (each a full matching build):

| Change | Slots |
| --- | ---: |
| before | 39 |
| d3drm (`maths_tail.h`) | 41 |
| DDRAW (`directdraw_tail.h`) | 41 |
| AVIFIL32 (`avi_player_tail.h`) | 51 |
| USER32 (`input_tail.h`) | 58 |
| ADVAPI32 (`g98_029_tail.h`) | 99 |
| KERNEL32: visible order and nine ghosts | 145 |

The KERNEL32 part alone, without the others, gives 35. At every step
`.idata$6` keeps its order, and after the last step it equals the original's.
`.rdata`, `.data` and `.idata` sizes are unchanged and the 146 LIBC members
keep their order.

One function moves. With `maths_tail.h`,
surface_triangle_area_calculate (`0x00479630`) rises from 60.37% to 63.47%:
its body is 0x26e bytes, the original's length, where it was 0x276. `.text`
is 16 bytes shorter (one alignment step). The header comes after the
function and adds no code to the image, so the change is in how MSVC 5.0
compiles the function. Why a later declaration moves it is not known. No
other score moves.

## Not done

- **ReproBit** runs `link.exe` through its own toolchain transport, not
  CMake's `CMAKE_LINKER`. It needs the same staged runtime, or a Wine
  override scoped to `link.exe` that keeps Wine's own `msvcrt`.
