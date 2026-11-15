# 686. Per-object `.bss` for 35 items, and Stats.c's records in Stats.c

Issue #829 implements finding 680's follow-up 2. In the original, 35 paired
items sit in their own units' `.bss`. In the rebuild they were commons.
This change gives them the original's definition form. It also moves
Stats.c's four ObjectStats records out of LegoObject.c's file and into
Stats.c's own (`entity_stats.c`). Every figure comes from full matching
builds under the gate's link mode (`/DEBUG /OPT:REF /OPT:NOICF`) on `main`
`ebc07595`.

## Summary

1. **The items are zero-initialised externals now.** In MSVC 5.0 a
   zero-initialised external goes to its object's `.bss`, and a tentative
   definition becomes a common (findings 554 and 669). The items are:
   - `objectGlobs`, `objectGlobs_f_eadcc` and
     `s_entity_auxiliary_release_records` (GAME-023);
   - Lego.c's 26 words, `004de208`–`004de58c` (GAME-021);
   - `objectRecallGlobs` (GAME-030), `textGlobs` (GAME-045) and
     `fontGlobs` (G98-008).

   Each now has `= { 0 }` or `= 0`. Natively the same definitions are C++
   (ADR 0006), so nothing changes there. They all stay external. Several
   are read by port files (`lego_port.cpp`, `level.cpp`, `engine.cpp`,
   `text_port.cpp`, `entity.cpp`), and a `static` form would cut those
   readers off. Main.c's two items need no change: #824 (finding 679)
   already defines them as fields of `mainGlobs = { 0 }`.
2. **The symbol counter does not move.** An initialiser changes neither
   the number nor the order of declarations (finding 538). No function's
   score changes: the match baseline is identical apart from its input
   hash.
3. **`objectGlobs_f_eadcc` follows `objectGlobs`, as in the original.**
   MSVC 5.0 lays out a unit's external `.bss` in definition order.
   `objectGlobs_f_eadcc` (`004eadcc`) was defined near the top of the file,
   so it landed before the struct. It is now defined right after
   `objectGlobs`, at the end of the file. An `extern` declaration takes its
   old place, which keeps its symbol number. LegoObject.c's three items now
   follow each other on both sides, in the original's order: `objectGlobs`,
   then the word at `+0xc63c`, then the list at `+0xc640`.
4. **Stats.c defines its four ObjectStats records** (`004a97a0`–
   `004a9ce0`, 1,344 B). They are the first items in its `.data`, and
   their rebuilt offset delta (−1,032) is now the same as for the items
   after them. Finding 680 had a +296 step at GAME-023 and a −1,032 step
   at GAME-043. Both are gone: LegoObject.c's data ends 1,344 bytes
   earlier, and Stats.c's starts with its own records. `legoobject_unit.c`
   declares the four records `extern`, in place of its four definitions,
   so its counter does not move. `entity_stats.c` defines them before its
   first function. That adds four declarations, and no Stats.c function
   changes. `translation-unit-ranges.csv` now gives GAME-043 a `.data`
   range (`004a97a0-004a9b90`). `make unit-data-offsets` attributes
   Stats.c's 133 `.data` items (129 before), and the unattributed
   annotations fall from 73 to 69.
5. **The commons agree up to Smoke.c, as finding 680 predicted.** With
   `textGlobs` out of the commons, 16 commons are at their original offset
   (2 before), 59,244 bytes (35,760 before). The first difference moves
   from `spiderWebGlobs` (`005520e8`, +1,272) to Smoke.c's
   `smokeListHead` (`00552128`, −52). The original's four Smoke.c words
   are one 52-byte block (finding 680, follow-up 3).

## Before and after

| Measure (`binary-progress.json`) | `main` `ebc07595` | This change |
| --- | ---: | ---: |
| **Exact-function code bytes** | **348,304 (54.21%)** | **348,304 (54.21%)** |
| Functions that rose / fell | | 0 / 0 |
| Accuracy, full / game-only | 97.59% / 97.34% | 97.59% / 97.34% |
| `.text` identical at original addresses | 6.76% (43,434 B) | 6.76% (43,445 B) |
| `.data` identical (section-relative) | 24.72% (18,102 B) | **23.80% (17,426 B)** |
| `.rdata` identical | 11.67% | 11.67% |
| Identical prefix, raw / masked | 11 B / 7,187 B (to `00402c13`) | same |
| Correct-size functions, full | 2,801 | 2,801 |
| Units with size agreement | 2 | 2 |
| Entry point (original `0048f000`) | `0048ebb0` | `0048ebb0` |
| SizeOfImage (original 3,608,576) | 4,030,464 | 4,030,464 |
| Data-offset agreement (`layout`) | 20 of 57 units | 20 of 57 |
| `layout` unattributed or unmapped annotations | 129 | 125 |
| Tail roots | 3 | 3 |
| `data_identity` `.bss`: items at original offset | 1 of 149 (1,040 B) | 1 of 149 (1,040 B) |
| `data_identity` `.bss`: rebuilt bytes, objects with paired items | 869,488, 25 | 921,744, 29 |
| `data_identity` commons: items at original offset | **2 of 62 (35,760 B)** | **16 of 62 (59,244 B)** |
| `data_identity` commons: rebuilt bytes, start | 2,297,252 at `005a3600` | 2,244,900 at `005b0220` |
| `data_identity` commons: first difference | `005520e8` (+1,272) | `00552128` (−52) |
| `data_identity` `.data`: prefix / items at original offset | 12 B / 158 of 2,455 | same |
| `data_identity` `.rdata`: items at original offset | 0 of 95 | same |
| `data_identity`: items at original address, every region | 0 | 0 |

Region starts (original / rebuilt): `.rdata` `0049e000` / `004b2000`;
`.data` `004a1000` / `004b8000`; `.bss` `004b2db0` / `004cf190`; commons
`00549520` / `005a3600` before and `005b0220` after. The commons now start
52,256 bytes later because the 35 items' bytes moved into the units'
`.bss`, ahead of them. No item is at its original address in any region
until the tail leaves the link (finding 680, rank 1).

**Identity cost: `.data` identical falls from 24.72% to 23.80%** (676
bytes). The measure compares raw bytes at equal section offsets, and
neither side's `.data` starts at the same address. Moving the records takes
1,344 bytes out of LegoObject.c and puts them back at Stats.c's start, so
every byte between the two units sits 1,344 bytes earlier. The bytes that
matched there by coincidence before do not match now. Item offsets are what
place data once the regions start in place (finding 680, section 5), and
there the change helps: GAME-023 to GAME-043 now share one delta with the
units before them. The raw prefix and the masked prefix do not change.

`opt_ref` (the `/OPT:NOREF` relink) counts fewer annotated functions:
2,718 against 2,719, with one more port-unit function (32 bytes). The
missing ones are AviPlayer's deleting destructors (`??_GAviSurface`,
`??_EAviMovie`). Each shares its address with its `??_E`/`??_G` twin, and
the relink's map can list either name at that address. Two builds of this
same source gave 2,717 and 2,718. `avi_player.cpp` is unchanged, no
function is removed (`removed: 0`), and the gate's own count is still
2,719.

## What remains of follow-up 2

- **Lego.c's words are in their units, but not in the original's order.**
  The rebuilt Lego.c `.bss` holds the 26 externals in definition order
  (`s_lego_drag_x` first, at offset delta `0x1b728`). The original
  interleaves them with Lego.c's five statics: `004de428`–`004de538` are
  `static` in `lego_unit.c`, between externals at `004de410`–`004de424`
  and `004de53c`–`004de58c`. MSVC 5.0 puts statics and externals in
  separate runs (finding 679). So the original's interleaving means its
  Lego.c state used one form throughout, probably one struct or all
  statics. The 512 bytes at `004de210`–`004de410` are not annotated.
  Matching that order means moving definitions, which changes declaration
  order (finding 538 rule A) and finding 671's counter-sensitive span at
  `004311d0`. That is left for its own issue.
- **`.bss` offsets cannot count yet.** The first `.bss` break is still the
  8 bytes before `aiTaskGlobs` (finding 680, follow-up 8), so `.bss` items
  at their original offset stay at 1 whatever the later units do.

## Checks

- `make reccmp-check` passes: no function rises or falls. `make
  match-progress` rewrote the baseline (input hash only), the listing
  digests and `binary-progress.json`.
- `make unit-data-offsets` passes (statuses unchanged; GAME-043 `.data`
  gaps-only with 133 items). `make pe-audit` passes: the image ends at
  `0x7d8000`, 163,840 bytes below `0x800000`.
- `make tools-test` passes without `--allow-growth`, and so does
  `make winapi-link-check`.
- The native build compiles, and its 39 ctest tests pass with the real
  WADs. `LegoRR` starts with the offscreen driver and runs until the
  timeout.

## Reproduce

```
make reccmp-check && make match-progress
python3 tools/data_identity.py
make unit-data-offsets && make pe-audit
```
