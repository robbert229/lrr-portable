# Finding 659 — Lego.c's three largest near misses: stack slots, a function static, and a scheduling order

Issue #772, part of #388 and ADR 0005. A dedicated pass on Lego.c's three
largest non-exact functions:

| Address | Function | Size | `main` `bf6335e3` | After |
| --- | --- | ---: | ---: | ---: |
| `00429890` | lego_level_load | 6,752 | 95.08 | **95.46** |
| `00426520` | lego_main_sim_interaction_and_cell_update | 6,368 | 71.10 | **71.21** |
| `0042f6f0` | lego_cell_init_pass | 3,008 | 80.99 | **83.85** |

Scores are the full `make reccmp-check` report. None of the three became
exact.

| Measure | `main` `bf6335e3` | This change |
| --- | ---: | ---: |
| Functions that rose / fell | | 3 / **0** |
| Exact functions | 2,408 / 2,833 mapped | 2,408 / 2,833 mapped |
| Exact-function code bytes | 335,096 (52.15%) | 335,096 (52.15%) |
| Accuracy, full / game-only | 97.02% / 96.68% | 97.02% / 96.68% (the means rise below the second decimal) |
| Image end (`make pe-audit`) | `0x7f5000` | `0x7f5000` |

No function fell below the baseline, so no drop needs a label. `make
match-progress`, `make unit-data-offsets` and `make pe-audit` were run on the
final tree, and the generated files are committed (`unit-data-offsets`
changed no tracked file). In `binary-progress.json`, one 32-byte function
moves from `opt_ref`'s `original_annotated` category to `port_unit` (2,534
to 2,533). Finding 654 saw the same kind of move with no annotation change.
The cause was not found here either. `make tools-test` (no
`--allow-growth`), `make winapi-link-check` and native ctest (37 of 37)
pass. The native build starts and runs with the shipped WADs.

Every edit is in `src/game/lego_unit.c`, inside the three functions and
Lego.c's pointer-state globals. No other thread's file is touched.

### Rebased onto the #777–#780 stack

Measured again on #780 (`79f83f77`: #777, #778's one `objectGlobs` struct,
#779, #780) with a full `make reccmp-check`. The three functions rise by the
same amounts, and nothing else moves. The single-function loop over all 223
Lego.c functions differs from #780 only in the three targets, so the
counter balance holds on top of #778.

| Measure | #780 `79f83f77` | This change on it |
| --- | ---: | ---: |
| Functions that rose / fell | | 3 / 0 |
| Exact / mapped, full | 2,415 / 2,833 | 2,415 / 2,833 |
| Exact-function code bytes | 336,831 (52.42%) | 336,831 (52.42%) |
| Accuracy, full / game-only | 97.07% / 96.73% | 97.07% / **96.74%** |
| Image end | `0x7f5000` | `0x7f5000` |

On the stack, `binary-progress.json` shows no `opt_ref` category move.

## Method and tools

The single-function loop (`make match-function`) scores these functions
0.8–3.9 points lower than the full report. Calls through link names, for
example `media_pointer_cursor_select` for `0045cc20`, resolve only in the
link. The loop was used for direction only. Every figure above comes from
the full report.

Two scratch tools did most of the work. They are not committed.

- **A frame-slot metric.** The original's code is disassembled, and every
  `[esp+n]` operand is normalised to its frame offset by tracking pushes,
  pops and `add esp`. The rebuilt object gets the same treatment (Capstone
  over the COFF section). The two sequences are aligned by mnemonic, and the
  metric counts the operands whose offsets agree. One compile of
  `lego_unit.c` takes about 0.9 s, and scoring it is much less noisy than a
  difflib score when only slots differ.
- **A variable-partition search.** `lego_level_load`'s locals are
  generated from a template. Each choice is which source variables share
  one C variable, which address-taken locals get their own inner block, and
  the declaration and initialisation orders. The search scores every
  candidate with the metric above.

### What decides an MSVC 5.0 stack slot

Minimal compiles with the matching flags (scratch experiments, in the
style of `tools/experiments/486/`) give these rules for locals that are
spilled, not kept in registers:

1. **Slots are ordered by the variable's reference count, highest
   nearest `esp`.** A variable with more loads and stores gets the lower
   frame offset. Uses inside an `if` count fully.
2. **Ties go to the variable whose last use comes first.** Declaration
   order and names make no difference: renaming or moving a declaration
   leaves every slot where it was.
3. **Variables whose live ranges do not overlap share a slot.** A
   variable whose address is taken is never shared at function scope. In an
   inner block it is live only within that block, and it shares a slot with
   variables outside the block. Two arrays in sibling blocks, for example,
   share one slot.

So a variable's stack slot depends on which source uses are one C
variable, and on which address-taken locals sit in inner blocks. Both are
invisible in the code itself.

## `00429890` lego_level_load: 95.08 → 95.46

The code was already the original's instruction for instruction. What was
left was cause D, stack slots (findings 644, 648, 652, 656), and one
statement order.

| Region | Cause | Fix | State |
| --- | --- | --- | --- |
| Map parameter splits | the original splits CryOreMap before PathMap | statements swapped | fixed |
| `emergeCreature`'s slot | the original's lookup writes an address-taken int into the same slot as the string, which is dead by then (rule 3: an inner-block variable) | `{ int emergeType; … }` around the EmergeCreature lookup | fixed |
| Slug lookup | the original's out-parameter shares a slot with a map parameter and the `(float)atoi` temporary of the drag box | `int slugType` in the Slug block | fixed |
| Drag-box copy | the original's copy has its own slot, shared with `terrainParam` | `char* dragCopy` and `argv` in one inner block | fixed |
| FullName copy | the original uses its own variable, which shares the emerge slot | `fullName` | fixed |
| Float temporaries | the original's three float variables, each reused (rule 1 orders them) | upgrade times in `value` and `upgradeTime`; the slug, initial slug and fog rates in `slugValue`; the erosion times reuse all three (trigger in `upgradeTime`, erode in `slugValue`, lock in `value`). The partition search over the 295 valid partitions picked this one | fixed |
| Parameter initialisers | rule 2's tie order | declared and zeroed as `pathParam, blockPointersParam, cryOreParam, terrainParam, predugParam` | fixed |
| Five map pointers (`surface`, `predug`, `terrain`, `blockPointers`, `cryOre`) | equal reference counts, and the original's order (cryOre, predug, surface, blockPointers, terrain) is not their last-use order | **open**: no declaration order (720 tried), statement order or variable split reproduces it | 39 of 145 frame operands still differ |

The frame metric went from 22 of 145 operands, after the first scoping
edits, to 106 of 145. The full score went from 95.08 to 95.46. What is left
is the five-map permutation above, plus the swapped slots it causes for
`slugValue`/`upgradeTime` and two map parameters. Rule 2 does not explain
that order, so something else breaks the tie.

**Symbol counter.** The scoping added three block-scope declarations and
`slugValue` and `fullName`, and removed `triggerTime`, `erodeTime`,
`lockTime` and `objType`. That left one symbol over. In the first full run
it shifted `00431450` (100 → 83.78) and `004311d0` (70.80 → 67.15), both
rule-B phases that finding 571 records. The seed now reuses `number`, which
is dead at that point. With that, the single-function loop over all 223
Lego.c functions differs from `main` only in the changed functions.

## `00426520` lego_main_sim_interaction_and_cell_update: 71.10 → 71.21

| Region | Cause | Fix | State |
| --- | --- | --- | --- |
| `s_lego_hold_time += elapsedGame` | rule A (finding 538): the original loads the global first, so it was declared after the parameter. It is a function static (only this function uses the seven pointer-state globals) | the static moves into the function, with its `GLOBAL` annotation, as in `bubbles.c` and `interface_unit.c` | fixed |
| Counter | the static costs one symbol after the function. Keeping a file-scope `extern` shifted `00431450` and `004311d0` again | the file-scope definition is removed, and the overlay buttons' `done` flag reuses `handled` (the two are live in disjoint branches) | neutral (single-function loop) |
| Callees | all resolve in the link (link names) | none needed | — |
| Prologue registers | the original keeps `level` in `ebx` and the zero constant in `ebp`, and ours swaps them. Hundreds of `cmp x, ebp`/`ebx` lines follow from this. The original reads `level` 7 times and reloads `legoGlobs.currLevel` 10 times, against our 5 and 13, and it reuses `ebx` for a block pointer later | **open** | — |
| Terrain-click section (`004276a9`–`004277e3`) | the original uses two flags (the decompile's `bVar12`, set and tested per sub-block, and `bVar4`, which jumps to the tracking code, plus `bVar3` at the end), where ours has one `handled` | **open**: the decompile's flag and goto structure needs a careful reconstruction; the largest remaining block of diffs (`-41/+35`, `-34/+28`, `-23/+1`, `-1/+27`) | — |
| Frame | 92 of 199 frame operands agree | follows from the two rows above | open |

The other six pointer-state globals stay at file scope. Moving them changes
no instruction, and it would cost the counter seven.

## `0042f6f0` lego_cell_init_pass: 80.99 → 83.85

| Region | Cause | Fix | State |
| --- | --- | --- | --- |
| Prologue | the original schedules the `dirs` stores among the block lookup; ours put them first, as an initializer | `dirs` is declared without an initializer and assigned after `block = …`. Of all 120 orders of the five prologue statements, `map`, `wallCount`, `block`, `dirs`, `fade` is the best (80.22 → 83.08 in the loop). The prologue now matches exactly | fixed |
| Third work-flag test (4 walls) | the original tests `flags1 & 0x400` and `flags2 & 4` through a block pointer, then recomputes the whole block address for `flags1 & 0x8000` (`test byte ptr [..+9], 0x80`). We fold it into `test dh, 0x84`. The original is 0x27 bytes longer here, which moves every later jump and all four jump tables, so most of the remaining diff lines come from this one spot | **open**. Tried: Lego.h's `lego_C_block_is_reinforced` for the pair, with `(bx, by)` or with a `Lego_Block*` parameter; the third test through `lego_C_block_at`, a cast, the index in other orders and signedness, or another inline; inline bodies with `if`/`?:`/a result local; a byte-typed third test (this separates the test, but the address is still reused) | — |
| 2-wall prologue | the original copies `flags` into `edx` before `\|= 0x14`, so the loop index is in `eax`; ours swaps them | **open**: `flags \|= 0x14` as an assignment expression and a block-local loop index made no change | — |
| 1/3-wall `flags = (flags & ~0x20) \| 0x14` | operation order (`or ecx, 0x14` against `or al, 0x14`) | open | — |
| `fabs(h1 - h2) < fabs(diag)` | the original emits `fcomp st(1)` … `fstp st(0)`, ours `fcompp` | **open**: the mirrored comparison, `!(… >= …)`, and `(float)` or `(double)` casts change nothing | — |

`make match-search` found nothing: stores (5 candidates) and operands
(1,147 candidates). The operands search stopped at its 800 s cap, before
the pairs phase finished. It can be resumed with `python3
tools/permuter_search.py 0x0042f6f0 --mode operands --seconds 1200
--output build-match/search/0x0042f6f0-operands --resume` on an unchanged
`lego_unit.c`.

## Follow-ups

- **`00429890`**: find what breaks rule 2's ties among the five
  equal-weight maps. With that, the function should reach its last 39 frame
  operands, close to 100%. The scratch metric and the partition search
  could become a `make match-search` mode (`frame`).
- **`00426520`**: reconstruct the terrain-click section from the
  decompile's flags (`bVar12`/`bVar4`/`bVar3`), then look again at the
  `level`/zero register choice.
- **`0042f6f0`**: the CSE barrier before the `0x8000` test (finding 648
  first noted this). Closing it moves the jump tables back, and most of the
  function's remaining lines with them.
