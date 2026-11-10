# Finding 506 — Original-shape pilot: Priorities.c

Issue #384. Rebuilds one original translation unit, `Priorities.c`
(GAME-034, text `0045cdc0`–`0045d7e0`, finding 505), in the original's shape
and measures each part of that shape separately in the *matching build*. The
decision it feeds is [ADR 0005](../adr/0005-original-shape-c-units.md).

## Result

Rebuilding the unit in its original shape raised it from **15.6% to 67.8%**
byte-weighted similarity, from 0 to 4 exact functions, and from 13 to 14
mapped functions. That is a step, not an increment. The step came from
writing the original's code, not from any compiler or layout setting:

| Step | What changed | Unit similarity | Exact | Effect |
| --- | --- | ---: | ---: | ---: |
| before | `main` at `653caf92` | 15.64% | 0 | |
| (a) | gather the 13 existing functions, unchanged, into one file | 15.64% | 0 | **0** |
| (b) | rewrite in original shape, compiled as C++ with `/GX` | 67.75% | 4 | **+52.11** |
| (b1) | (b), but typed fields replaced by raw byte offsets (control) | 68.02% | 4 | +0.27 vs (b) |
| (c) | (b) compiled as C (`/TC`) | 67.75% | 4 | **0** |
| (d) | (b) compiled as C++ without `/GX` | identical code | | **0** |
| (e) | (c) with the callees' original contracts (ceiling, not committed) | 71.80% | 4 | +4.05 vs (c) |

Similarity is the Reccmp score of each function weighted by its size in the
original (2,518 code bytes in the unit, padding excluded), from the
game-only report. "Exact" counts functions at 100%.

The same rewrite moved the one outside caller, the Panel button dispatcher
at `0045b620`, from 52.94% to 100%: it now calls the unit's functions with
the original arguments.

Code length tells the same story. The non-exact functions recompiled to
34.5% more instructions than the original before (10 of 13 more than 5%
longer). After step (c) they are 2.2% longer (3 of 10).

## Per-function scores

Scores in percent. Bracketed pairs are recompiled/original instruction counts
in Reccmp's diff (non-exact functions only).

| Address | Function | Bytes | before | (a) | (b), (c) | (b1) | (e) |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `0045cdc0` | PriorityImagesConfigLoad | 319 | 17.76 (160/99) | 17.76 | 65.69 (105/99) | 65.69 | 91.84 (86/86) |
| `0045cf00` | PriorityLabelLayoutApply | 74 | 21.62 (40/34) | 21.62 | 51.61 (29/33) | 51.61 | 51.61 |
| `0045cf50` | PrioritiesConfigLoad | 270 | 18.10 (127/83) | 18.10 | 66.27 (86/83) | 66.27 | 57.69 (79/77) |
| `0045d060` | PrioritiesImagePositionsConfigLoad | 163 | 32.50 (89/71) | 32.50 | 96.55 (32/32) | 96.55 | 96.55 |
| `0045d110` | RegionContainmentScanUpdate | 491 | 7.62 (62/43) | 7.62 | 53.99 (164/162) | 53.33 | 50.16 (158/149) |
| `0045d300` | PriorityImageSelectionStateUpdate | 110 | 32.00 (38/37) | 32.00 | **100.00** | 100.00 | 100.00 |
| `0045d370` | RegionBoundsSet | 75 | 32.56 (21/22) | 32.56 | **100.00** | 100.00 | 100.00 |
| `0045d3c0` | PriorityPanelRowMoveDispatch | 41 | 38.10 (20/22) | 38.10 | **100.00** | 100.00 | 100.00 |
| `0045d3f0` | PriorityImageOrderMoveToFront | 120 | 14.55 (58/52) | 14.55 | 53.01 (43/40) | 53.01 | 53.01 |
| `0045d470` | UIListEntriesSwap | 120 | 28.57 (54/44) | 28.57 | 75.00 (35/37) | 75.00 | 75.00 |
| `0045d4f0` | ConfigEntryEnabledToggleAtIndex | 92 | 16.28 (48/38) | 16.28 | **100.00** | 100.00 | 100.00 |
| `0045d550` | UiCursorMoveToChoiceEntry | 228 | 11.21 (64/43) | 11.21 | 73.10 (74/71) | 73.10 | 82.35 (66/66) |
| `0045d640` | UiCursorMoveToElementCenter | 144 | 10.00 (50/30) | 10.00 | 52.27 (48/40) | 54.55 | 62.50 (40/40) |
| `0045d6d0` | PositionTrackerUpdateAll | 271 | unmapped | unmapped | 52.87 (85/89) | 55.37 | 61.94 (78/77) |
| | **Byte-weighted** | **2,518** | **15.64** | **15.64** | **67.75** | **68.02** | **71.80** |
| | Matched-equivalent bytes | | 394 | 394 | 1,706 | 1,713 | 1,808 |

Outside the unit, steps (a), (b), (c), (b1) and (e) changed no function's
score. The game-only mean rose from 48.24% to 48.59%.

## What each step was

Commits on the pilot branch: (a) `0636c52a`, (b) `f91f49e3`, (c) `bc7af334`.
Variants (b1), (d) and (e) are measurements only and were not committed.

- **(a) One file per unit.** The 13 marked functions moved verbatim from
  `frontend.cpp`, `ui_list.cpp` and `crt/crt.cpp` into one file in original
  order. No score moved. A first attempt also listed the file in the
  executable's own sources and dropped three call sites: MSVC 5.0 routes
  calls into objects linked directly (and `/INCREMENTAL`) through jump
  thunks, while library members are called directly. Keep unit objects in
  the static libraries.
- **(b) Original shape.** `src/game/priorities.c` has the 14 functions in
  original order, with the original signatures (no port hook tables, no
  state parameters, no deviation guards) and one typed globals struct whose
  layout is the original `.bss` (`00500f00`–`005013c0`). The two initialised
  words at `004a8ee8` are file-scope statics, as the `.data` order requires.
  It calls the existing reconstruction where a callee exists. Three callees
  had no original-ABI implementation and got thin adapters:
  `Image_LoadBMPScaled` (`0047d9d0`), `Image_DisplayScaled` (`0047dcb0`) and
  `ToolTip_SetContent` (`0046b520`). The rewrite is where the whole gain is.
- **(b1) Typed fields versus offsets.** Generated from (b) by turning every
  field of the globals struct and `Image` into a cast at a byte offset (the
  pattern #385–#387 remove). Codegen is the same to within a few
  instructions. Typed structs are worth having for correctness and review,
  but they are not what raises scores.
- **(c) C compilation.** The matching build now enables C (`cl /TC`, same
  flags plus `/ML`). For this unit C and C++ produce instruction-identical
  code; only MSVC's internal symbol numbers differ (`$S672` in C against
  `$S1382` in C++). That numbering is the input to the entropy effect below,
  so only a C compile can reproduce the original's.
- **(d) Exception handling.** `/GX` and `/GX-` give byte-identical listings
  for this unit in both languages. `/GX` only changes functions that own
  objects with destructors, and C-shaped code has none. Dropping `/GX`
  matters for unit boundaries, not for codegen.
- **(e) Callee contracts.** The rewrite still calls reconstruction APIs whose
  contracts differ from the original. The ceiling variant used the original
  contracts in the unit only: Config items read directly at `+4`/`+8`,
  `Config_FindArray` returning the first item, and the panel position as
  floats. That gains 4.05 points. It cannot be committed until the callees
  change:
  - the config layer (`gods98/block.cpp`) is a tree whose records keep the
    name at `+0`. `block_subgroup` (`00478ff0`) returns the container, where
    the original returns the first child, so the unit calls
    `block_first_child`, `block_name` and `block_value`;
  - the panel position (`indexed_pair_get`, `0045a700`) is stored as
    integers, where the original stores floats, so the unit converts it;
  - `block_get_strptr` (`004790b0`) does match: like the original it returns
    the record's own value, which the unit tokenises in place.

The remaining 28% is ordinary per-function work: loop form
(`0045d3f0`, `0045cf00`), evaluation order and stack-slot layout (`0045d110`,
`0045d6d0`), and statement order (`0045cf50`).

## The facts from finding 505

1. **Common symbols.** Confirmed on the toolchain: `int x;` at file scope
   compiles to a COMMON symbol in C (section 0, value = size) and to a
   `.bss` definition in C++. The Priorities state is per-object `.bss`, so it
   is `static` and the difference does not arise here. Units with globals in
   the common region (from `00549520`, for example ToolTip's
   `0054bf20`–`0054c000`) need C to place them.
2. **Exception handling.** See (d): no codegen effect without destructors.
3. **Link order.** The pilot unit sits in the `game` library. The exe's
   object order is the order the linker pulls members, which these libraries
   do not yet control. Reccmp scores are independent of placement, so this
   pilot does not test link order. Data layout and literal pooling will need
   it (#388).
4. **Inline helpers.** The unit uses none.

## Entropy: header edits change unrelated code

The first version of (b) moved declarations from the C++ headers into new C
headers that the C++ headers included. Five functions in untouched files
then dropped (one from 100% to 84.21%) and nineteen rose. Single-TU MSVC
compiles with `/FAs` listings pinned the cause:

- MSVC 5.0 numbers every symbol a translation unit declares. Including any
  file costs 2 numbers, a function declaration about 2 (one more for each
  extra parameter), a redeclaration 1, a variable 1, and an `extern "C"`
  block nothing.
- Register and x87-stack choices in a function depend on that numbering.
  Restoring the count alone, with 17 inert declarations, restored
  `lava_erosion_delay_from_cell` byte for byte.

Any edit to a shared header therefore perturbs every unit that includes it.
The committed pilot keeps other units identical by:

- adding `extern "C"` to the existing C++ declarations in place (free);
- keeping the C declarations in separate boundary headers that shared C++
  headers do not include, checked against the C++ ones by
  `src/game/test_c_boundaries.cpp`;
- filling the slots of declarations it removed from `crt.hpp` (7) and
  `frontend.hpp` (24) with one documented, never-defined pad declaration
  each.

The final build changes no function outside the unit. ADR 0005 sets the
policy for the restructuring that follows, which cannot avoid this.

## Native port

The native port compiles `priorities.c` as C++ (CMake `LANGUAGE CXX`), and
GCC accepts it as C89 with `-Werror=declaration-after-statement`. `main` does
not build natively at `653caf92`: 34 errors in `gods98/resource.cpp`,
`surface.cpp`, `frameloop.cpp`, `game/entity.cpp`, `level.cpp`,
`tutorial_hud.cpp` and `test_level.cpp`, from #372 onwards. Every object
still compiles to the same error set after this change and no new error
appears. The new `c_boundaries` test builds and passes. The other unit tests
cannot link until `main` is fixed.

## Reproducing

    make reccmp-check                      # matching build + baseline gate
    git checkout 0636c52a && make reccmp-report   # step (a)
    git checkout f91f49e3 && make reccmp-report   # step (b)

Variants: (b1) and (e) replace `src/game/priorities.c` for one build, and
(d) compiles it with `/GX-`. The per-function table uses the game-only
`results` JSON and the unit's rows in
[`translation-units.csv`](../coverage/translation-units.csv).

Complete pseudocode: [LEGORR.full.c](../decompiles/full/LEGORR.full.c).
