# 563. The whole-binary gap beyond function code

Issue #554. Measured on `main` at `63da6269`. That build passed `make reccmp-check`: 919 exact functions, exact function code 79,550 bytes (12.38% of `.text`).

The new tool `tools/pe_audit.py` (`make pe-audit`, tests in `tools/tests/test_pe_audit.py`) measures everything here that it can read from the two PEs and the link map. Scratch relinks of the same objects, with outputs under `/tmp`, tested link options. They changed no flags and no source. Follow-ups: #560, #561, #562, #563.

## Summary

1. **Size, not content, is the gap.**
   - The rebuilt image is 2,435,072 bytes, against the original's 735,744.
   - `.text` is 331,717 bytes too long: the port tail (307,616 bytes) and code that nothing in the matching build references (312,258 bytes, which overlaps the tail).
   - Initialised `.data` is 923,549 bytes too long. Of that, 912,673 bytes are the named-section padding that #448 retires.
   - `.rdata` is 445,223 bytes too long. 431,664 of them are two zero-filled port placeholders (#561).

   Until these sizes match, no byte after `.text` can sit at its original address.
2. **The structures are close.** The DOS stub is identical, and neither build has a Rich header or a `.reloc`. The section set, alignments, linker version (5.0), OS and subsystem versions, stack and heap sizes and checksum (0) are identical. The mode-selection dialog resource is byte-identical. Of the 827 exact functions followed by fill, 787 have the original's fill bytes; after the other 40 come jump tables with absolute addresses.
3. **What differs in structure:**
   - two timestamps and one header bit (#560);
   - the icon resources (#560);
   - the import directory: DLL set, descriptor order, and the DirectX import libraries the toolchain lacks (#563);
   - an 84-byte debug directory that `/DEBUG` puts at the start of `.rdata` (#562).
4. **The original's link mode can be inferred.**
   - It was linked **without `/DEBUG`**, so `/OPT:REF` was in effect: the `LINE_NUMS_STRIPPED` bit is set, and only 14 import thunks were kept.
   - Its objects were probably compiled **without `/Gy`**, because 242 unreferenced functions survive.
   - MSVC 5.0's `/O2` implies `/Gy` and `/Gf`, and `/Ox` does not. This bears on #553's string-pooling question.

## The table

**Columns.** *Bytes differing* counts offsets of the longer side that are not equal at the same section offset, so a size difference counts in full. Function code inside `.text` is Reccmp's measure (`binary-progress.json`), not this table's. **Effort:**
- **S** is a day or less;
- **M** is a few days;
- **L** is the restructuring programme (#388): relocation and conversion waves.

| Area | Bytes differing | Cause | Fix | Effort |
| --- | ---: | --- | --- | :---: |
| DOS stub, PE and section headers | 62 of 1,024 | 4 `TimeDateStamp`, 1 `Characteristics` (`LINE_NUMS_STRIPPED`, cleared by `/DEBUG`); the rest are sizes, `AddressOfEntryPoint`, `BaseOfData`, directories and section-table entries that follow section sizes | post-link stamp and flag (#560); the rest follow the sections | S |
| `.text` size | +331,717 (974,266 vs 642,549) | port tail 307,616; legacy files 117,392 against their original share; LIBC 69,344 vs the original CRT's 57,557 (`/include` list, port references); 151 import thunks vs 14 | drain the tail (#388 waves); #447 (`/include` list, CRT); the link mode (#562) | L |
| `.text`, unreferenced code | 312,258 removable by `/OPT:REF` | units 184,608, tail 79,792, legacy 30,176, LIBC 16,800, thunks 882; live callers are still port code | measure it now, convert callers, then adopt the original's link (#562) | M (measure), L (close) |
| `.text`, inter-function fill | 40 of 827 fill runs after exact functions | jump tables with absolute addresses; the fill bytes (0x90) already agree | none of its own | — |
| `.text`, DirectInput data | 4,264 missing | `c_dfDIMouse`/`c_dfDIKeyboard` from `dinput.lib` sit in `.text` (0x49b700–0x49c7a7); the toolchain has no `dinput.lib` | period DirectX import libraries (#563) | M |
| `.rdata` | 455,296 (456,455 vs 11,232) | 431,664 are `frontend.cpp`'s two `static const s_empty` placeholders (port tail); 84 are the `/DEBUG` debug directory at the section start, shifting every `.rdata` item; units 11,768, legacy 4,408, LIBC 1,599 | #561; the link mode (#562); then per-unit `.rdata` order (ADR 0006 decision 5; #451, #481) | S, then M |
| Repeated strings | not bytes: whole-string repeats in `.data` 1 (original) vs 2 (rebuilt), in `.rdata` 26 vs 16 | the original's 2,301 `.data` strings repeat once, so it pooled them or never repeated them; see #553 | #553 | (#553) |
| `.data`, initialised | 983,675 (raw 996,864 vs 73,216; initialised content 996,685 vs 73,136) | `.data$zzzzzz` 741,632 (`frontend.cpp`) and `.data$yyyyyy` 171,041 (`audio.cpp`) padding; real data 84,012 (units 37,396, legacy 17,284, tail 16,816, LIBC 12,468) | #448, then per-unit order (`make unit-data-offsets`: full 0, gaps-only 2, partial 30) | M, then L |
| `.bss` and commons | virtual 3,339,912 vs 2,863,396 | the original has 616,304 bytes of per-object `.bss` and 2,247,092 of C commons; the rebuild has 18,600 bytes of commons (only converted C units make tentative definitions) and 1,510,532 bytes of port-tail `.bss` | conversion (C commons), #449 (common order), tail drain | L |
| `.idata` | 3,583 of 4,112 | 7 DLLs vs 11 (ddraw, avifil32, ole32 and `Direct3DRMCreate` loaded by `GetProcAddress` in port code; no `dinput.lib`); dsound imported by name, not ordinal 1; descriptor order follows command-line library order; in the DLLs both import, port code adds 22 names and lacks 5 | #563 with #447; conversions restore the calls | M |
| `.rsrc` | 2,457 of 2,936 | icon (2,216 bytes) and group icon (20 bytes), both en-GB, missing; an extra `VERSIONINFO` (728 bytes); directory timestamp; dialog 101 identical | #560: icon from `reference/` at build time, no `VERSIONINFO` in the matching build | S |
| `.reloc` | 0 | neither build has one (`RELOCS_STRIPPED` in both) | none | — |
| Debug directory and overlay | 0 after `strip_pe_debug.py`, but the 84 bytes in `.rdata` remain | `/DEBUG` | #562 | M |
| Library and link effects | in the rows above | LIBC.LIB is not the original's build (finding 540, blocker 3); late members through `oldnames.lib` (LATE-003 to 006) are already in order; `/DEBUG` implies `/OPT:NOREF` | #447; #562; #553 for the compiler flags | M |

## The measurements

### Headers

`make pe-audit` lists every differing field. Only three fields differ for reasons of their own:

| Field | Original | Rebuilt |
| --- | --- | --- |
| COFF `TimeDateStamp` | `0x37EEB911` (1999-09-27) | link time |
| COFF `Characteristics` | `0x10F` | `0x10B` |
| Resource directory `TimeDateStamp` | `0x37EEB90E` | link time |

A scratch link without `/DEBUG` produced `0x10F`. The other differing fields are `SizeOfCode`, `SizeOfInitializedData`, `SizeOfImage`, `AddressOfEntryPoint`, `BaseOfData` and the import, IAT and resource directories. They follow section sizes.

### `.text`

The original's `.text` per unit kind, from `translation-unit-ranges.csv`:

| game | gods98 | CRT | import | late | total |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 463,328 | 112,448 | 57,557 | 4,368 | 4,848 | 642,549 |

The rebuilt `.text` by link-order category: each map symbol owns the bytes up to the next one.

| Link | unit | legacy | tail | LIBC | import thunks | total |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| gate (`/DEBUG`, `/OPT:NOREF`) | 479,008 | 117,392 | 307,616 | 69,344 | 906 | 974,266 |
| scratch `/DEBUG /OPT:REF` | 294,400 | 87,216 | 227,824 | 52,544 | 24 | 662,008 |

- The original keeps fill only between functions. Its 88,657 bytes of fill are:
  - 17,445 of 0x90;
  - 175 of 0xCC;
  - 71,037 bytes of other content: in-function jump tables, and the DirectInput data.
- **Import thunks.** The gate build has 151:

  | kernel32 | user32 | winmm | advapi32 | d3drm | gdi32 | dsound |
  | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
  | 73 | 41 | 29 | 3 | 2 | 2 | 1 |

  Only four are referenced by an object:
  - `maths.c`'s two d3drm calls;
  - `sound3d.c`'s `DirectSoundCreate`;
  - `reward.c`'s `timeGetTime`, through a declaration without `dllimport` in `gods98/winmm.h`. This is why `reward_loop` (0x461410) is at 97.25%.

  The other 147 come with the `__imp_` members and stay under `/OPT:NOREF`.
- **`/OPT:REF` is not usable yet.** Scored by Reccmp, the scratch `/OPT:REF` build maps 1,320 functions instead of 2,422: **1,102 are lost, 420 of them exact**, because nothing in the matching build calls them. Adopting it now would break the gate (ADR 0006 decision 2). The 312,258 removable bytes instead measure how much annotated code still has no original caller.

### Data sections

From the map, by category:

| Group | Bytes | Content |
| --- | ---: | --- |
| `.rdata` | 456,455 | tail 436,328, units 11,768, legacy 4,408, unnamed 2,352 (with the debug directory), LIBC 1,599 |
| `.data` | 83,964 | units 37,396, legacy 17,284, tail 16,816, LIBC 12,468 |
| `.data$yyyyyy` | 171,041 | `audio.cpp`'s `MenuCompatData` |
| `.data$zzzzzz` | 741,632 | `frontend.cpp`'s padding and menu tables |
| `.bss` | 3,339,912 | tail 1,510,532, units 1,223,124, legacy 585,500, commons 18,600, LIBC 1,116 |

Read from the COFF objects instead, `frontend.cpp.obj`'s `.rdata` is 433,312 bytes:
- 383,704 of them are `frontend_menu`'s `static const FeMenu s_empty`;
- 47,960 are `frontend_active_submenu`'s `static const FeSubmenu s_empty`.

`.rdata` precedes `.data`, so these move every data address by about `0x6A000`.

**The debug directory.** `/DEBUG` writes three 28-byte debug-directory entries at the start of `.rdata`. `strip_pe_debug.py` clears the directory pointer and zeroes the entries, but the 84 bytes stay. A `/DEBUG /OPT:REF` link and a link without `/DEBUG` therefore differ in 16,287 `.rdata` bytes and 2,117 `.text` bytes (absolute operands). Everything else is equal: `.data` differs in 186 bytes and `.idata` in none.

**Unreferenced functions in the original.** 242 of the original's 2,564 functions have no direct call and no 32-bit reference anywhere in the image: 203 game, 20 gods98, 15 CRT, 3 late, 1 import. Together they are 60,062 bytes.
- `/OPT:REF` with `/Gy` would have removed them.
- The original dropped unreferenced import thunks, so `/OPT:REF` was on.
- So the game objects were most likely compiled without function-level linking.

### Imports

| | Original | Rebuilt |
| --- | --- | --- |
| Descriptor order | KERNEL32, ole32, d3drm, DDRAW, WINMM, AVIFIL32, DINPUT, DSOUND, USER32, GDI32, ADVAPI32 | d3drm, DSOUND, KERNEL32, WINMM, USER32, GDI32, ADVAPI32 |
| Imported names | 145 | 151 |
| IAT | 0x270 bytes, blocks sorted by DLL name | 0x278 bytes, blocks sorted by DLL name |

Two scratch relinks with reordered libraries showed two things:
- The descriptor order is the order of the import libraries on the command line.
- LIBC's code precedes the thunks only when `libc.lib` is listed explicitly first.

The original's descriptors imply the order `kernel32, ole32, d3drm, ddraw, winmm, vfw32, dinput, dsound, user32, gdi32, advapi32`. ADR 0006 decision 1 derived its library order from the thunk order instead. #563 reconciles the two.

### Resources

| Leaf | Original | Rebuilt |
| --- | --- | --- |
| `ICON 1` | 2,216 bytes, en-GB | missing |
| `DIALOG 101` | 458 bytes, en-US | identical |
| `GROUP_ICON 113` | 20 bytes, en-GB | missing |
| `VERSIONINFO 1` | — | 728 bytes (`src/version.rc`) |

## Recommended order of work

1. **#560: resources and header stamps** (S). Independent. It closes `.rsrc` and every header difference that does not follow from section sizes.
2. **#561: the `.rdata` placeholders** (S). Port-only code, with no score at stake. It takes `.rdata` from 456 KB to about 23 KB and moves all data closer to the original's addresses.
3. **#448: the named-section padding** (M). 912,673 of the 923,549 excess initialised `.data` bytes. Its own issue sets the constraints.
4. **#562: measure unreferenced code** (M). Report the bytes `/OPT:REF` would remove, per category, next to the match metrics. It is the whole-binary counterpart of the exact-byte count, and it falls as conversions give original functions their original callers.
5. **#563 with #447: import libraries and library order** (M). Needs the period DirectX import libraries. It fixes the descriptor order, the DirectInput data in `.text`, and part of the LIBC placement.
6. **The restructuring waves (#388, #442–#446, conversions)** (L). They drain the 307,616-byte tail and the 1.5 MB of tail `.bss`, and turn C++ definitions back into C commons. Then come #449 (common order) and the per-unit `.data`/`.rdata` order.
7. **The original's link mode (#562, step 2)**, last. Switch to a link without `/DEBUG` (or relocate the debug directory) and to `/OPT:REF` once no score drops. #553 should first settle whether the original's objects had `/Gy` and `/Gf`.

Nothing found here is purely native-port work, so no follow-up has the `port` label.

## Reproducing

```sh
make reccmp-check                   # builds build-reccmp/LegoRR.exe and its map
make pe-audit                       # summary
make pe-audit PE_AUDIT_JSON=out.json  # every measurement
make unit-data-offsets
```

**The scratch relinks.**
1. Copy `build-reccmp/CMakeFiles/program.dir/link.txt`.
2. Redirect `/out`, `/pdb`, `/implib` and `/MAP` to a scratch directory.
3. Apply one change per relink:
   - append `/OPT:REF`;
   - remove `/DEBUG` and `/pdb`;
   - reorder the libraries.
4. Run the result from `build-reccmp/` in the toolchain environment.
5. Run `tools/strip_pe_debug.py` on a copy of a `/DEBUG` output before comparing it.

**Scoring the `/OPT:REF` link.** Point `reccmp-build.yml` at it and run `tools/reccmp_full_diff.py`.
