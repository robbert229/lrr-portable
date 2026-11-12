# 576. Link mode: unreferenced code under `/OPT:REF`, and how Reccmp keeps its PDB

Issue #562, a follow-up of finding 563. The analysis below was measured on `main` at `1d31a556`. [The stacked figures](#refreshed-on-the-stack-581--583) were measured again on #583's head `25529648` (GAME-023 part 1, then GAME-021 part 2). The method and the decision did not change. ADR 0006 decision 2 asked for `/OPT:REF` to be measured and adopted only if it changes no score. Finding 563 showed that the original was linked without `/DEBUG`, so LINK 5.0 applied `/OPT:REF` by default. The gate links with `/DEBUG` (so `/OPT:NOREF`), because Reccmp needs the linker's PDB.

## Summary

1. **The measure.** The new `tools/opt_ref_link.py` relinks the gate's objects without `/DEBUG`, the original's mode, in about 2 s. It then compares the two maps.
   - `make match-progress` runs it (`make opt-ref-link`), and `binary-progress.json` records the result as `opt_ref` (schema 4).
   - `/OPT:REF` removes **375,438 bytes of `.text` in 3,097 symbols**.
   - That includes **1,262 annotated functions, 528 of them exact**. A Reccmp run on a `/DEBUG /OPT:REF` relink loses exactly the same 1,262 functions.
2. **The gate cannot switch yet.** Linking the gate with `/OPT:REF` would unmap 1,262 functions and drop the exact count from 1,027 to 500. Their live callers are still port code. ADR 0006 decision 2 forbids that.
3. **The decision: two links (option a).**
   - The gate keeps `/DEBUG` for Reccmp's PDB and scores.
   - A second link of the same objects without `/DEBUG` gives the original's mode for whole-binary identity.
   - At equal `/OPT` the two links have the same layout, apart from where the debug directory sits in `.rdata`.
   - The second link is implemented here, so the gate and its scores do not change.
   - Option (b), a single link without `/DEBUG` with Reccmp fed another way, is not supported by Reccmp 0.1.7.
   - Option (c), stripping the debug directory after the link, needs 2,900 fixups and still needs the second link's map.
4. **Side effects of the original's mode on the current objects:**
   - **Thunks:** 5 thunks instead of 152, in the original's order. The original has 14 for 145 names; the 9 missing thunks belong to DLLs that port code loads with `GetProcAddress` (finding 574).
   - **Import directory:** the descriptor order holds, and LIBC precedes every thunk.
   - **Entry point:** RVA 0x95d20, against the gate's 0xed350 and the original's 0x8f000.
   - **`.data`:** 21,792 bytes less (616 unreferenced string literals). The 912,673 bytes of named-section padding (#448) stay.

## Before and after

"Gate" is the `/DEBUG` link that this change leaves as it was. "Original mode" is the new second link of the same objects without `/DEBUG`.

| | Original | Gate (`/DEBUG`, `/OPT:NOREF`) | Original mode (no `/DEBUG`, `/OPT:REF`) |
| --- | ---: | ---: | ---: |
| `.text` virtual size | 642,549 | 1,041,892 | **666,410** |
| `.rdata` | 11,232 | 24,984 | 24,792 |
| `.data` initialised (map groups) | 73,136 | 999,725 | 977,933 |
| `.data` raw | 73,216 | 999,936 | 978,432 |
| `.bss` (map) | 2,863,396 (finding 563) | 3,782,112 | 3,781,088 |
| `.idata` | 4,112 | 4,154 | 3,854 |
| Import thunks in `.text` | 14 (145 names) | 152 | **5** |
| Imported names | 145 | 152 | 141 |
| Entry-point RVA | 0x8f000 | 0xed350 | 0x95d20 |
| `Characteristics` | 0x10F | 0x10B | **0x10F** |
| Debug directory | none | 84 bytes at the start of `.rdata` (entry cleared by `strip_pe_debug.py`) | none |
| Reccmp mapped, full report | | 2,471 | 1,209 (scored on `/DEBUG /OPT:REF`) |
| Reccmp exact, full report | | 1,027 | 500 |

The gate's Reccmp numbers do not change: `make reccmp-check` passes, and the baseline differs only in its input hash.

## Refreshed on the stack (#581 → #583)

Measured on `convert/574-game-021-part-2` at `25529648`, with this change rebased onto it. The gate's scores equal #583's baseline.

| | `main` `1d31a556` | Stack `25529648` |
| --- | ---: | ---: |
| Gate: Reccmp mapped / exact (full) | 2,471 / 1,027 | 2,482 / 1,095 |
| Gate: `.text` | 1,041,892 | 1,085,780 |
| Gate: `.data` initialised / `.bss` | 999,725 / 3,782,112 | 1,002,717 / 3,790,928 |
| Gate: entry-point RVA | 0xed350 | 0xf7ec0 |
| `/OPT:REF` removes: `.text` bytes (symbols) | 375,438 (3,097 of 6,720) | **401,854 (3,163 of 6,892)** |
| ⤷ annotated, full report: removed / exact removed | 1,262 / 528 | **1,272 / 546** |
| ⤷ annotated, game-only: removed / exact removed | 1,225 / 499 | 1,235 / 517 |
| ⤷ annotated in `unit` objects, bytes (functions) | 238,224 (1,144) | 241,632 (1,113) |
| ⤷ annotated in `legacy` objects, bytes (functions) | 9,024 (81) | 23,984 (122) |
| ⤷ port code: unit / legacy / tail, bytes | 8,000 / 31,776 / 70,608 | 6,640 / 40,832 / 70,960 |
| ⤷ LIBC / import thunks, bytes | 16,800 / 1,006 | 16,800 / 1,006 |
| Exact functions that survive `/OPT:REF` | 499 | **549** |
| Original mode: `.text` / `.rdata` | 666,410 / 24,792 | 683,882 / 24,840 |
| Original mode: `.data` initialised / `.bss` | 977,933 / 3,781,088 | 979,309 / 3,789,984 |
| Original mode: `.data` proper removed by `/OPT:REF` | 21,792 | 23,408 |
| Original mode: entry-point RVA | 0x95d20 | 0x9a160 |
| Original mode: thunks / imported names / descriptor order | 5 / 141 / equal | 5 / 141 / equal |

**The removed-annotated counts rose, not fell.** The two conversions added 11 mapped functions and 68 exact ones, and much of the new original-shape code is still reached only from port code that calls the port's own implementation.
- `lego_unit.c`'s removed functions went from 69 to 127 (61 exact).
- The new `legoobject_unit.c` adds 63 (47 exact).
- Callers rewired by the conversions took others back into the live graph: `legoobject.cpp` went from 82 to 64, `lego.cpp` from 60 to 37 and `terrain_map.c` from 55 to 42.

The share of annotated functions removed is unchanged at 51%. The exact functions that survive `/OPT:REF`, the ones the original mode keeps, rose by 50. Converting a unit only lowers the count once the unit's callers are converted too. The gate's switch condition therefore still waits.

## What `/OPT:REF` removes (`1d31a556`)

These are `.text` symbols in the gate's map with no symbol of the same name from the same object in the relink's map. Each symbol owns the bytes up to the next one. The categories are `winapi_link_audit.py`'s (finding 575).

| Category | Gate bytes | Removed bytes | Gate functions | Removed functions |
| --- | ---: | ---: | ---: | ---: |
| Annotated original code | 469,888 | 247,248 | 2,234 | 1,225 |
| ⤷ in original-shape and relocated units (`unit`) | 426,848 | 238,224 | 2,050 | 1,144 |
| ⤷ in `legacy` files | 43,040 | 9,024 | 184 | 81 |
| Port code in `unit` objects | 46,128 | 8,000 | 359 | 112 |
| Port code in `legacy` objects | 125,120 | 31,776 | 1,010 | 458 |
| Port tail | 326,224 | 70,608 | 2,522 | 1,026 |
| LIBC | 69,344 | 16,800 | 441 | 129 |
| Import thunks | 1,036 | 1,006 | 152 | 147 |
| DirectInput data (`dinput.lib`) | 4,152 | 0 | 2 | 0 |
| **Total** | **1,041,892** | **375,438** | **6,720** | **3,097** |

**Annotated functions removed.** 1,262 of 2,471 mapped functions in the full report are removed, 528 of them exact. In the game-only report it is 1,225 of 2,235, 499 of them exact. LIBC accounts for the difference: 37 functions, 29 exact.

The removed annotated code sits in 82 objects. The largest are:

| Object | Removed | Exact | Bytes |
| --- | ---: | ---: | ---: |
| `interface_unit.c` | 88 | 37 | 20,608 |
| `legoobject.cpp` | 82 | 2 | 19,648 |
| `lego_unit.c` | 69 | 37 | 47,728 |
| `lego.cpp` | 60 | 37 | 6,688 |
| `container_unit.cpp` | 60 | 18 | 6,976 |
| `frontend_unit.cpp` | 55 | 23 | 8,496 |
| `terrain_map.c` | 55 | 28 | 14,528 |
| `reward.c` | 46 | 19 | 15,456 |

Converted units feature prominently. Their functions are exact, but the code that calls them is still a port adapter that calls the port's own implementation. Conversions that give these functions their original callers lower the count. GAME-002, GAME-021 and GAME-023 (#570, #574, #575) are in flight.

**Validation.** A Reccmp run on a `/DEBUG /OPT:REF` relink maps 1,209 functions, 500 of them exact. The 1,262 it loses are exactly the map measure's 1,262, function for function.
- Of the functions that survive, 9 score lower and 3 higher. Their data moved: for example, `level_current_terrain_get` (0x431570) goes from 100% to 66.67%.
- Two map symbols needed care:
  - LIBC's `__chkstk` and `__alloca_probe` share an address, and the two maps list them in different orders.
  - `flic_opcode27_fallback_noop` survives as a second name at another function's address.

  The measure therefore checks removal against every symbol name in the relink's map.

**The floor.** The original itself keeps 242 functions that nothing references (finding 563). They survived its `/OPT:REF` only because its objects were probably compiled without `/Gy`. The matching build compiles with `/O2`, which implies `/Gy`, so this measure cannot reach zero until #553 settles the compiler flags. Until then, its floor is the annotated subset of those 242 functions.

## The link modes compared

Every relink used the gate's `link.txt` with its outputs redirected (`tools/opt_ref_link.py relink`). The `/DEBUG` executables were compared after `strip_pe_debug.py`, as Reccmp sees them.

| Comparison | `.text` | `.rdata` | `.data` | `.idata` | Map symbols that move |
| --- | --- | --- | --- | --- | --- |
| `/DEBUG /OPT:REF` vs no `/DEBUG` | same size; 2,674 bytes differ | 24,872 vs 24,792; 16,518 bytes differ | same size; 184 bytes differ | identical | 102 of 6,950, all in `.rdata` (85 by 88 bytes, 17 by 80) |
| Gate (`/DEBUG`) vs no `/DEBUG /OPT:NOREF` | same size; 3,635 bytes differ | 24,984 vs 24,904; 16,580 bytes differ | same size; 184 bytes differ | identical | 103 of 10,802, all in `.rdata` |
| Gate vs `/DEBUG /FIXED:NO` | identical | identical | identical | identical | none (`.reloc` added at the end) |
| Gate vs no `/DEBUG` (`/OPT:REF`) | 1,041,892 vs 666,410 | 192 bytes removed | 21,792 bytes removed | 300 bytes removed | 3,625 in `.text`, 3,032 in `.data`, 149 in `.bss`, 102 in `.rdata` |

At equal `/OPT` setting, `/DEBUG` therefore changes only:
- the `.rdata` layout: 84 bytes of debug directory at the start, so each later item moves by 80 or 88 bytes after alignment;
- the absolute operands and pointers that refer to those `.rdata` items;
- the `LINE_NUMS_STRIPPED` bit (0x10B vs 0x10F) and the `.rsrc` timestamp.

Function addresses, `.data`, `.bss`, `.idata`, the entry point and section RVAs are equal.

### (a) Two links: chosen

- The gate stays `/DEBUG` and keeps Reccmp's PDB and the baseline.
- `make match-progress` relinks the same objects without `/DEBUG` (`make opt-ref-link`, about 2 s; no recompile). `binary-progress.json`'s `opt_ref` records:
  - what `/OPT:REF` removed, per category, and the annotated and exact functions among it;
  - the relinked image: sections, `.data` and `.bss`, thunks and their order, the entry point, the import order and `Characteristics`;
  - its section bytes against the original (`opt_ref.image.section_bytes`).
- Where the two links disagree today, the cause is the removed code: 375,438 bytes of `.text` move everything after them. Once the removed annotated count reaches its floor, the gate can take `/OPT:REF` with `/DEBUG` kept. Its scores then stay the same, and the two links differ only in the debug directory, as the first row of the table shows.

### (b) One link without `/DEBUG`: not possible with Reccmp 0.1.7

- Reccmp reads only a PDB, through `cvdump`. It takes line numbers, which match annotations to rebuilt functions, plus symbols, section contributions and types. It has no map-file input.
- LINK 5.0 writes a PDB only with `/DEBUG`.
- Reccmp does not check the PDB's signature against the executable. So a PDB from another link can be paired with the no-`/DEBUG` executable, but its section offsets then point at the wrong `.rdata` items.
- Tested: the no-`/DEBUG /OPT:NOREF` executable with the gate's PDB keeps all 2,471 mappings, but **102 functions drop and 45 exact matches are lost** (1,027 to 982), for example `bubble_action_strings_initialize` (0x4070b0) from 100% to 2.5%.

### (c) Relocate or strip the debug directory: rejected

The debug directory cannot simply be cut out. The image has no `.reloc`, and `.rdata` items move by 80 or 88 bytes, not by 84.
- A `/DEBUG /FIXED:NO` relink gives an identical image plus `.reloc`, so it can list the absolute references to fix. There are 2,900 into `.rdata` after the directory: 2,754 in `.text`, 140 in `.data` and 6 in `.rdata`.
- Each item's new offset then has to come from a link without `/DEBUG`, so the second link is needed anyway.
- The PDB would no longer describe the patched executable, so Reccmp would still need the unpatched one.

(c) can at best reproduce what (a)'s second link already produces.

## Side effects of the original's mode

**Import thunks.** 5 thunks instead of 152, with the DirectInput data:

`d3drm: D3DRMVectorRandom, D3DRMVectorRotate` · `c_dfDIMouse` · `c_dfDIKeyboard` · `dinput: DirectInputCreateA` · `dsound: #1` · `kernel32: RtlUnwind`

- This is the original's order (finding 574) with its 9 other thunks missing: `Direct3DRMCreate`, `ddraw` ×2 and `avifil32` ×6, which port code loads with `GetProcAddress`.
- The 147 removed thunks include `kernel32.lib`'s first-pass thunks. These precede LIBC in the gate (ADR 0006, library-order addendum).

**Import directory.** The descriptors are `KERNEL32, d3drm, WINMM, DINPUT, DSOUND, USER32, GDI32, ADVAPI32`, in the same order as the original's (`common_dll_order_equal` is true). LIBC runs from 0x495d20 to 0x4a26d0, and every thunk follows it (0x4a2a60–0x4a3b24). The library order from #579 therefore holds under `/OPT:REF`. Imported names go from 152 to 141, because `/OPT:REF` drops names that only removed code used.

**Entry point.** `WinMainCRTStartup`, the first LIBC function, is at RVA 0x95d20. That is 0x6d20 (27,936) bytes after the original's 0x8f000, because the code before LIBC is that much too long. The port tail kept by `/OPT:REF` is 255,616 bytes. The gate's entry point is 0xed350.

**`.data` and `.bss`.** The original's raw `.data` is 73,216 bytes; the gate's is 999,936.

| Part | Gate | Original mode |
| --- | ---: | ---: |
| `.data$zzzzzz` (`frontend.cpp`) | 741,632 | 741,632 |
| `.data$yyyyyy` (`audio.cpp`) | 171,041 | 171,041 |
| `.data` proper | 87,052 | 65,260 |
| `.bss` | 3,782,112 | 3,781,088 |

- `/OPT:REF` removes **21,792 bytes**: 616 string-literal COMDATs (`??_C@`) that only removed functions use, 21,620 of them from unit objects. It does not remove the named-section padding. Those **912,673 bytes** (98.5% of the excess) are #448's.
- After both, `.data` proper would be 65,260 bytes, less than the original's 73,136. The data that the original's still-unconverted code references is missing.
- `.bss` loses 1,024 bytes of commons.
- Section padding in the strict sense (raw minus virtual) is 211 bytes in the gate and 499 in the original-mode link.

## Decision

- **Link mode (amends ADR 0006 decision 2).**
  - The matching build keeps two links of one set of objects:
    - the gate, with `/DEBUG`, gives Reccmp its PDB and scores;
    - the original-mode link, without `/DEBUG` (`/OPT:REF`), gives whole-binary identity and the unreferenced-code measure.
  - `make match-progress` produces both.
- **When the gate takes `/OPT:REF`.** When `opt_ref.annotated.full.removed` reaches its floor, the gate's link gets `/OPT:REF` with `/DEBUG` kept, in a link-only PR (ADR 0006 entropy rule 1). The floor is zero, or the annotated subset of the original's own unreferenced functions while the objects keep `/Gy` (#553).
  - From then on the two links differ only by the debug directory (the first row of the comparison).
  - The whole-binary measures of `binary-progress.json` (`sections`, `text_at_original_addresses`) can then move to the original-mode link.
- **Until then** they stay on the gate. `fully_identical_functions_at_original_address` and the layout measures use Reccmp's rebuilt addresses, which only the gate's PDB provides.

## Reproducing

```sh
make reccmp-check          # the gate build
make opt-ref-link          # build-reccmp/opt-ref/: the same objects, no /DEBUG
make opt-ref               # what /OPT:REF removed (also in binary-progress.json)
make match-progress        # both, and the generated files
```

**Scratch variants.** `python3 tools/opt_ref_link.py relink --out <dir> [--keep-debug] [--flag /OPT:REF|/OPT:NOREF|/FIXED:NO]`, then `measure --out <dir>`.

**Scoring a `/DEBUG` variant.**
1. Run `strip_pe_debug.py` on a copy of its executable.
2. Point `reccmp-build.yml` at that copy and at the variant's PDB.
3. Run `tools/reccmp_full_diff.py`.
4. Restore `reccmp-build.yml` afterwards.
