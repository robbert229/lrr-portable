# 689. AITask.c's `.rdata` in the original's order; where `.rdata` stops next; the `.bss` gaps

Issue #838 (finding 687, follow-ups 1 and 2). Measured on `main`
`77022c16` with full matching builds before and after.

## Summary

1. **AITask.c's `.rdata` is the original's, byte for byte**
   (`0049e010`–`0049e034`, section 1). The original's block is an
   unreferenced 1.0, then 0.0, 100, 130, 6, −6, 250, 0.01 and 99999. The
   rebuild had no 1.0, and it pooled a second 0.0 between 0.01 and 99999.
   AITask.c now defines the 1.0 as a named constant, first in the unit.
   The three `timeIn` compares that used a literal `0.0f` now read the
   named 0.0, as the original's code does (`[0049e014]`). Three functions
   rise and none falls. On the original-mode link, the `.rdata` identical
   prefix grows from 18 to **52 bytes**. Eight `.rdata` items are now at
   their original addresses (32 B; before, none).
2. **The next `.rdata` break is Bezier.c's `{1, 2, 4, -1}` table**
   (`0049e034`, section 2). It is `d3drmdef.h`'s, and finding 646 showed
   that the toolchain's DirectX 3 header moves Bezier.c's symbol counter.
   It needs the DirectX 6 headers, which is #837's area. Work forward stops
   there.
3. **The `.bss` gaps still have no source that names them** (section 3).
   Nothing is defined (ADR 0005 rule 4).

## Before and after

| Measure (`binary-progress.json`) | `main` `77022c16` | This change |
| --- | ---: | ---: |
| **Exact-function code bytes** | **351,876 (54.76%)** | **351,876 (54.76%)** |
| Functions that rose / fell | | **3 / 0** |
| Accuracy, full / game-only | 97.78% / 97.55% | 97.78% / 97.55% |
| `.text` identical at original addresses | 6.92% | 6.92% |
| `.data` / `.rdata` identical (section-relative) | 28.48% / 9.56% | 28.48% / **9.60%** |
| Identical prefix, raw / masked | 11 B / 7,187 B | 11 B / 7,187 B |
| Correct-size functions, full | 2,811 | 2,811 |
| Units with size agreement | 2 | 2 |
| Entry point (original `0048f000`) | `0048ea60` | `0048ea60` |
| SizeOfImage (original 3,608,576) | 3,596,288 | 3,596,288 |
| Data-offset agreement | 22 of 57 units | 22 of 57 |
| Tail roots | 0 | 0 |

The functions that rose (gate scores):

| Address | Function | Before | After |
| --- | --- | ---: | ---: |
| `00404fc0` | aitask_place_command_eligibility_check | 99.55 | 99.66 |
| `00405950` | aitask_pending_command_timer_fire_scan | 98.62 | 99.08 |
| `00405c10` | aitask_pending_command_entity_assignment_tick | 62.45 | 62.61 |

Their code is unchanged. The `.rdata` operand of each 0.0 compare now
names the item the original names.

`data_identity` on the original-mode link, per region:

| Region | Prefix before → after | Items at original offset before → after | At original address before → after | First item not at its offset (after) |
| --- | ---: | ---: | ---: | --- |
| `.rdata` (starts in place) | 18 B → **52 B** | 0 of 95 → **8 of 96** (32 B) | 0 → **8** (32 B) | `0049e048` (Bezier.c), −20 |
| `.data` (4,096 early) | 12 B → 12 B | 192 (3,401 B) → same | 0 | `004a1d5c` (`rock_fall_styles.c`), +132 |
| `.bss` (4,224 early) | — | 1 (1,040 B) → same | 0 | `004b31c8` `aiTaskGlobs`, −8 |
| commons (10,528 early) | — | 16 (59,256 B) → same | 0 | `00556fb8`, −3,848 |

The `.rdata` prefix now stops at `0049e034`, the first byte of Bezier.c's
table. Bezier.c's three constants follow 20 bytes early: 16 for the table
and 4 for the unreferenced 1.0 at `0049e044`.

## 1. AITask.c's constants

The original's `.rdata` from `0049e010` and its readers (every dword in
`.text` that holds an address in the block):

| Address | Value | Read by |
| --- | ---: | --- |
| `0049e010` | 1.0 | nothing |
| `0049e014` | 0.0 | 9 sites, among them `00404ff3`, `00405972`, `00405c3d` |
| `0049e018` | 100 | `004022a8` |
| `0049e01c` | 130 | `00404472` |
| `0049e020` | 6 | `00404825`, `0040484d` |
| `0049e024` | −6 | `00404836`, `0040485e` |
| `0049e028` | 250 | `004049a2` |
| `0049e02c` | 0.01 | `00405412` |
| `0049e030` | 99999 | nothing (`00405c2b` stores it as an immediate) |

The rebuild's block on `main` was 0, 100, 130, 6, −6, 250, 0.01, **0**,
99999, 36 bytes as in the original but in another order. Its three 0.0
compares at `00404ff3`, `00405972` and `00405c3d` read the second 0.0.
They are `task->timeIn > 0.0f` and two `task->timeIn <= 0.0f`. MSVC 5.0 emits
named constants first, in definition order, and literals after them in
first-use order (finding 687). A named constant and a literal of the same
value are never pooled together. The 99999 is a literal: `bestDist =
99999.0f` compiles to an immediate store, but the literal is still pooled.

So the original's order is the seven named constants (0.0 to 0.01), with a
named 1.0 defined before them and nothing reading it, then the 99999
literal. A 1.0 in Advisor.c would be pooled with Advisor.c's own 1.0 at
`0049e008`, so this one is AITask.c's (finding 687). The three compares now
read `s_rdata_0049e014`.

**No symbol-counter cost.** `make match-function SOURCE=src/game/aitask.c`
before and after: the three functions above rise (98.30 → 98.41,
95.41 → 95.87, 61.34 → 61.50, without the link) and all 106 others score
the same. The new named constant adds one symbol, and the 0.0 literal it
replaces took one away.

The constant is `s_rdata_0049e010`, after the unit's naming of the others.
Its original name is unknown. It is the original's form, an unreferenced
`static const float`, and not a pad: the original has the item, and the
rebuild did not.

`translation-unit-ranges.csv` gives GAME-002's `.rdata` as
`0049e010-0049e030` (it was `0049e014-0049e02c`), so the new annotation is
attributed to the unit. `make unit-data-offsets` keeps GAME-002's `.rdata`
`gaps-only`, now with 8 items (7 before).

## 2. The next break: Bezier.c's table

After AITask.c's 99999 the original has `{1, 2, 4, -1}` (`0049e034`, 16 B)
and an unreferenced 1.0 (`0049e044`), then Bezier.c's annotated 3.0, 0.0
and 1.0 (`0049e048`–`0049e050`). The table is `d3drmdef.h`'s (finding 524).
Finding 646 tried `d3drm.h` in GAME-003: the DirectX 3 header in the
toolchain moves the unit's counter by an amount that costs
`cubic_bezier_point_evaluate`. The original was built against the DirectX 6
headers, which hold many more GUID declarations. This is the first break
that needs the GUID block (#837), so this change stops here. Nothing after
`0049e034` was changed.

## 3. The `.bss` gaps

This change looked again for a source of the 8 bytes at `004b31c0` and
the 2,480 bytes at `004b8064`–`004b8a14`:

- No dword in the original's `.text` or data holds an address inside
  either range (finding 687).
- `AITask_Globs` (`aitask_globs.h`) ends with `flags` at `004b8060`. Every
  field before it is read by the original's code. Nothing reads an offset
  past `+0x4e9c`, or below `+0`.
- The word at `004b8a14` is GAME-004's all-zero `""` literal (finding 685).
- One layout fits both gaps: a single 8-aligned object at `004b31c0` of
  0x5854 bytes, with 8 unused bytes before the known fields and 2,476
  after. Separate statics of AITask.c fit as well (finding 687's rules).
  The layout does not decide between them.

Nothing names these bytes or gives their types, so nothing is defined.
A guessed array would be filler (ADR 0005 rule 4).

## Checks

- `make reccmp-check` passes: 3 functions rise and none falls. `make
  match-progress` regenerated the baseline, the listing digests and
  `binary-progress.json`.
- `make unit-data-offsets` and `make pe-audit` pass. Neither changes a
  committed file.
- `make tools-test` (no `--allow-growth`) and `make winapi-link-check`
  pass.
- Native ctest passes, and the native build starts.

## Reproduce

```
make match-function SOURCE=src/game/aitask.c
make reccmp-check && make match-progress
python3 tools/data_identity.py
```

## Files

- `src/game/aitask.c`: `s_rdata_0049e010`; the three 0.0 compares read
  `s_rdata_0049e014`; the comment above the constants.
- `docs/coverage/translation-unit-ranges.csv`: GAME-002's `.rdata` range.
- `docs/coverage/`: regenerated.

No file owned by #837, #839, #840 or #841 is touched. No `#pragma
optimize` and no pad.
