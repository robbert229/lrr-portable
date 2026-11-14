# Finding 675 — AITask.c at its original size, and the prefix breaks in `0x401000`–`0x40a000`

Issue #806, part of #388 and ADR 0005. This works the size breaks that
finding 669 left in GAME-002 (AITask.c) and around it. It also retargets
GAME-002's four port-tail adapter calls (finding 668, section 6, item 4).

**Result: GAME-002 is its original size again.** It starts at `00401cb0`
and ends at `004065f0`, so it spans the original's 18,752 bytes; on `main`
it was 48 bytes short. Its last function ends at the original's end
address. GAME-003 now starts at its original address. Three functions
rise to exact and none falls. The masked identical prefix grows from 528
to 7,187 bytes.

| Measure (`binary-progress.json`) | `main` `316b70f6` | This change |
| --- | ---: | ---: |
| Exact-function code bytes | 340,131 (52.93%) | **340,372 (52.97%)** |
| Accuracy, full / game-only | 97.20% / 96.88% | **97.23% / 96.92%** |
| Functions that rose / fell | | **6 / 0** (3 to exact) |
| Correct-size functions, full | 2,600 (445,640 B) | **2,603 (448,690 B)** |
| Correct-size functions, game-only | 2,309 (394,813 B) | **2,312 (397,863 B)** |
| Identical prefix, raw | 11 B (`0040100b`) | 11 B |
| Identical prefix, address operands masked | 528 B (to `00401210`) | **7,187 B (to `00402c13`)** |
| Units with size agreement | 0 of 227 | **2 (GAME-001, GAME-008)** |
| Units with all sizes correct / start / end in place | 166 / 2 / 1 | 167 / 5 / 4 |
| `.text` identical at original addresses | 25,975 B (4.04%) | **39,820 B (6.20%)** |
| Fully identical functions at original address | 8 (384 B) | **42 (2,307 B)** |
| Entry point (original `0048f000`) | `0048d0d0` | `0048d100` |
| Function-order inversions, full / game-only | 222 / 1 pairs | 222 / 1 pairs |
| Image end (gate) | `0x7f5000` | `0x7f5000` |

**On the stack.** Rebased onto #812 (`match/807-unit-size-deficits`
`057557e6`, the top of #809 → #810 → #811 → #812), the same six functions
rise and none falls:

| Measure | #812 `057557e6` | This change |
| --- | ---: | ---: |
| Exact-function code bytes | 343,761 (53.50%) | **344,002 (53.54%)** |
| Accuracy, full / game-only | 97.53% / 97.26% | 97.56% / 97.30% |
| Correct-size functions, full | 2,793 (457,127 B) | 2,796 (460,177 B) |
| Correct-size functions, game-only | 2,502 (406,300 B) | 2,505 (409,350 B) |
| Identical prefix, raw / masked | 11 / 528 B | 11 / 7,187 B |
| Units with size agreement | 0 of 227 | 2 (GAME-001, GAME-008) |
| Units with all sizes correct / start / end in place | 167 / 2 / 1 | 168 / 5 / 4 |
| `.text` identical at original addresses | 29,832 B (4.64%) | 43,833 B (6.82%) |
| Fully identical functions at original address | 8 (384 B) | 42 (2,307 B) |
| Entry point (original `0048f000`) | `0048eb40` | `0048eb80` |
| SizeOfImage (original 3,608,576) | 4,030,464 | 4,026,368 |
| Function-order inversions, full / game-only | 222 / 1 | 222 / 1 |

#805's alias for GAME-002's `entity.cpp` call (`0043f5b0`) and this
change's four adapter aliases sit side by side in `aitask_imports.h`.

No measure moved away from the original. GAME-002 itself does not yet
count as agreeing. Two of its functions are still the wrong size, but both
differences stay inside their alignment padding: `00403810` (+4) and
`00405c10` (+8). The masked prefix now stops at `00402c13`. That is the
call from `00402bb0` to Lego.c's `00432e00`, which sits 576 bytes early in
the rebuild (GAME-021, outside this issue).

`make reccmp-check` passes with no function lower. `make match-progress`,
`make unit-data-offsets` and `make pe-audit` were run and the generated
files committed. `unit-data-offsets` and `pe-audit` changed no tracked
file. `make tools-test` (no `--allow-growth`), `make winapi-link-check`,
`make reccmp-lint` and native ctest (39 of 39) pass. The native build starts
and runs with the shipped WADs.

## Method

Per-function scores come from `tools/match_function.py` (MSVC 5.0 RTM,
matching-build flags). Variants were written over the real source path,
scored and restored, and none ran during a matching build. For codegen
questions, a 30-line C file was compiled with the same `cl` and `/FAs`.
`make phase-probe` (N = 0–63) was run on `00404280` and `00403810`; both are
single-code. Sizes are `identity_metrics.code_extent` on both images, the
rule `binary-progress.json` uses.

## Per-function results

| Address | Function | Start | End | Size | Change / what was tried |
| --- | --- | ---: | ---: | --- | --- |
| `00401210` | advisor_scene_visual_state_initialize | 78.26 | **100** | ok | A `volatile` local keeps the dead front-clip store (below) |
| `00402c20` | aitask_callback_00402c20 | 31.37 | **100** | **−16 → ok** | The comparison reads the task's block through a local pointer (below) |
| `00403810` | aitask_terrain_cell_record_count_update | 76.83 | 76.83 | +4 (in padding) | Unchanged (below) |
| `00404280` | aitask_cmd_entity_command_chain_tick | 83.07 | **92.18** | **−64 → ok** | Case 0's flag form, the inline distance (below) |
| `00404e00` | aitask_object_path_submit_to_grid_target | 98.65 | **100** | ok | `00441700` call retargeted |
| `00404fc0` | aitask_place_command_eligibility_check | 99.43 | 99.55 | ok | `0043a5f0` and `00432550` calls retargeted |
| `00405c10` | aitask_pending_command_entity_assignment_tick | 62.29 | 62.45 | +8 (in padding) | `00438e50` call retargeted; size not attempted (below) |
| `004065f0` | cubic_bezier_point_evaluate | 56.88 | 56.88 | +16 | x87 scheduling; finding 669's 0–127 phase probe, not repeated |
| `00406f80` | activity_runner_state_read | 81.82 | 81.82 | +16 | 14 more forms (below) |

### `00402c20`: the comparison through a pointer

The original compares the task's block in registers. After the flags
store it reloads both coordinates for the `Map3D` call, and it saves only
`esi`. MSVC kept the compared values for the call, which needs `edi`, and
that made the function 6 bytes shorter than the original. Finding 669 had
tried 11 forms. Reading the block through `int* taskPos = &task->blockX`
for the comparison, and through `task->blockX`/`task->blockY` for the
call, stops MSVC from treating the two as one subexpression. It gives the
original's code, 0x46 bytes in the 0x50 slot.

### `00401210`: the dead store

The original stores the front clip (`fstp [esp+4]`) to a local it never
reads. Finding 669 tried 12 forms and MSVC removed the store in all of
them, as it did in 13 more tried here: an empty `if`, a self-assignment,
an expression statement, initialised declarations, a copy chain, a pointer
store, a struct local, a `double` local, `if (0)`, a comma expression, a
use through the parameter, a parameter-address store and a product with
zero. Only a `volatile float` local keeps it. The function is then exact
in the full build. The original's declaration is unknown, so the
qualifier is marked `Deviation (scoring only)`. It changes no storage,
because the local is in the original's frame too.

### `00404280`: 64 bytes, from MSVC's block layout and cross-jumping

The rebuild's code was 61 bytes short. In the original, each of the
switch's ~25 `result = f(...); break;` cases keeps its own `mov edi, eax`.
In the rebuild they all cross-jumped into one. A small test file shows the
rule: **MSVC 5.0 cross-jumps a case tail only into the block that falls
through into the join.** It never merges two jumping tails with each
other. The original's two identical `xor edi, edi; jmp` blocks in case 26
stay separate in both builds.

So the question is which block falls into the join. It is case 0's last
block. Case 0 (`AITask_Type_Goto`) is `if (pos != task's block) { route;
result = submit(...); } else { result = 1; remove(object); }`. In every
direct form, MSVC lays the `else` arm first and the route arm last:
`jne A; jne A; B; jmp; A`. The route arm ends in `mov edi, eax`, so every
other case merges into it. Forms tried in the test file:

- with `else`;
- with `break` in either arm;
- the equality form with the arms swapped;
- nested `if`s;
- `goto` in both directions;
- `while`;
- `?:`;
- `!(a && b)`;
- the operands reversed;
- `px - X || py - Y`;
- `y` compared first.

The original's `jne A; je B; A; jmp; B` comes from two sources: two
separate `if` statements that share one body (`goto` into the block), or
a flag set by `if`/`else` and then tested. MSVC threads the flag away.
The rebuild reuses `result` as the flag, which adds no symbol:

```c
if (pos.x != task->blockX || pos.y != bp[1])
    result = 0;
else
    result = 1;
if (!result) {
    /* route */
    result = aitask_object_path_submit_to_grid_target(object, bp, ...);
    break;
}
result = 1;
aitask_entity_command_chain_head_remove(object);
break;
```

Now the remove arm falls into the join, and only case 19's `add esp, 4`
merges with it, as in the original. `bp = &task->blockX` gives the
original's `[edi]`/`[edi+4]` operands. Together these give 82.39 → 85.97
(single-function).

**Case 2's distance is `Maths_Vector2DDistance` inlined.** The original's
x87 sequence keeps the x difference in a stack slot and multiplies it from
memory. That slot is the dead `object` parameter, `[esp+0x80]`. The
out-of-line copy at `00401240` (`vec2_dist`) has the same shape.
`aitask_maths.h` declares it as a `static __inline` function, in the
pattern of `teleporter_maths.h`. It uses the direct product form;
`c = a - b` with a modulus gives 87.17, the product form 91.52. This also
returns `remove` to the original's `[esp+0x10]`. Before, it took the
parameter slot, and each of its three accesses used a 4-byte displacement.

**Left:** `lea edi` comes before the case 0 comparison in the rebuild and
after it in the original. The `taskPos` store order differs in the
wandering loop. And the final `if (task) attach(...) else ...` hoists the
shared `push ebp` above the branch: three forms tried, among them an
inverted `if`, `else { if }` and `goto`. The last one makes the code 1
byte shorter, and the jump table's 4-byte alignment absorbs it. Phase probe
0–63: single-code. Declaration orders (all 23 permutations of the four
locals) and block-scoped `remove`/`prev` give identical code.

### `00403810`: a near miss that would change behaviour

The original returns early when the first query's create call succeeds
(`jne` to the final epilogue) and otherwise jumps to the count block,
which sits last. The rebuild's `goto count_it` form puts an inline epilogue
there, 4 bytes longer. Tried: `else if` with `return` (61.04, 170 bytes),
`if (...) return; goto count_it;` (61.04), a flat `if` without `else`
(75.61, 175), a 3D index into `commandCounts`, and the term order. The
phase probe is single-code. One form, a `count > 0` guard around the loop,
reaches 172 bytes at 85.35, but it changes behaviour: with no capacity it
falls through and counts the request, where the original returns
(`jbe 004038b7`). It was not taken.

### `00406f80`: the early `edi` save

The original saves `edi` before the null test and shares one epilogue,
returning the tested null in `eax`. MSVC delays the push past the test in
every form tried (14). These were an early `return NULL`, with and without
the inverted test, `return object->cameraNullName`, `return 0`, `else
return NULL`, a single result variable, a name local with either return, a
slot pointer, `goto`, `?:`, `register`, a container local and `!= NULL`. The early return comes closest (83.72 at 0x31
bytes), but the slot is 0x30.

### `00405c10`: not attempted

Its +8 stays in its padding. The original's frame is 4 bytes larger
(`0x94`). Its two `switch` tables come in the other order (the
`3/0xc/0x15/0x16` table first). It loads `0x100` into `ebx` inside the
loop and keeps `best` in `ebx` rather than `ebp`. That is a restructure of
a 1.9 KB function.

## The four adapter calls

Finding 668 named four GAME-002 calls that reach `aitask_port.cpp`
adapters. Their callees now have original-shape definitions, so
`aitask_imports.h` renames each one in the matching build only, in the
pattern of `legoobject_port_aliases.h`:

| Adapter | Original function |
| --- | --- |
| `aitask_object_container_get_adapter` | `legoobject_visual_resource_get_or_create` (`0043a5f0`), which leaves the container in `eax` |
| `aitask_route_setup_adapter` | `legoobject_route_setup_and_marker_associate` (`00441700`) |
| `aitask_object_find_trainer_adapter` | `legoobject_query_center_mask100` (`00438e50`) |
| `aitask_place_command_cell_accept_adapter` | `always_accept_place_command_cell` (`00432550`), declared with the block parameter that the original pushes |

Each macro renames one declaration and its calls, so the unit's symbol
numbering does not change. None of the four adapters is in the rebuilt
map. GAME-002 no longer references `aitask_port.cpp`, which is still
pulled by GAME-021, GAME-027 and GAME-043 (`port_tail_inventory.py`).
GAME-002's only remaining tail reference is `entity.cpp` (one call). The
linked tail code falls from 194,432 to 194,224 bytes: the adapters are
dropped and the two accessors below are added.

## The two port-only accessors

`aitask_port_priority_values` and `_states` (finding 570) added 32 bytes
at GAME-002's end. They now live in `aitask_port.cpp`, which reads
`aiTaskGlobs` through a new `aitask_globs.h`. That header holds the
struct type and its `extern` declaration, so the definition in `aitask.c`
gets C linkage in the native C++ compile too. `aitask.c` includes it
where the typedef was. All 109 annotated functions score the same with
it, so the symbol numbering is unchanged (finding 538). Finding 669 had
left this step for the change that fixed `00402c20` and `00404280`.

## Reproduce

```sh
make match-function ADDR=0x00404280
make phase-probe ADDR=0x00404280 PROBE_ARGS="--range 0 64 1"
make reccmp-check && make match-progress
python3 tools/port_tail_inventory.py --json /tmp/tail.json
```

## Files

- `src/game/aitask.c`: the forms for `00402c20` and `00404280`; the struct
  type moved to `aitask_globs.h`; the accessors removed.
- `src/game/aitask_globs.h` (new): `AITask_Globs` and `aiTaskGlobs`.
- `src/game/aitask_maths.h` (new): the inline `Maths_Vector2DDistance`.
- `src/game/aitask_imports.h`: the four adapter aliases.
- `src/game/aitask.h`: the accessors' comment.
- `src/game/aitask_port.cpp`: the two accessors.
- `src/game/advisor.c`: `00401210`'s `volatile` local.
- `docs/coverage/`: regenerated.

No file owned by #804, #805, #807 or #808 is touched. No `#pragma
optimize` is committed, and no pad or filler was added.
