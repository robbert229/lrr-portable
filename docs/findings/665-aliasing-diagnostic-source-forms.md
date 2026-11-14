# Finding 665 — Source forms behind single-code pointer-chasing near misses

Issue #784, part of #388 and ADR 0005. Follows finding 663, whose
aliasing diagnostic (`#pragma optimize("a", on)` for one compile) gave
AITask.c's `00404100` the original's registers but the wrong store order.
This change finds the source form behind that function and applies the
same search to the other single-code near misses in AITask.c, Building.c
(`building_model.c`), Config.c, Flic.c and lwo.c.

**Result: six functions rise to exact, and no pragma is needed.** In none of these did the original assume less aliasing. It wrote
fewer statements than the port, or put a value through a variable the
port's expression skipped, and MSVC did the rest.

| Measure | `main` `b928af37` | This change |
| --- | ---: | ---: |
| Exact-function code bytes | 336,831 (52.42%) | **338,091 (52.62%)** |
| Accuracy, full / game-only | 97.07% / 96.74% | **97.09% / 96.76%** |
| Exact / mapped, full | 2,415 / 2,833 | **2,421** / 2,833 |
| Functions that rose / fell | | **6 / 0** |
| Image end (gate) | `0x7f5000` | `0x7f5000` |

`make reccmp-check` passes with no function lower. `make match-progress`,
`make unit-data-offsets` and `make pe-audit` were run and the generated
files committed (`unit-data-offsets` changed no tracked file). `make
tools-test` (no `--allow-growth`), `make winapi-link-check` and native
ctest (37 of 37) pass, and the native build starts.

## Method

All per-function scores come from `tools/match_function.py` (matching-build
flags, MSVC 5.0 RTM). Single-function scores sit below the full report
where a callee's name only resolves at link time, so only deltas within one
compile are compared. Variants were written over the real source path,
compiled, scored and restored (finding 654's in-place rule). No matching
build ran at the same time.

1. **Diagnostic.** Each function from 75% to 99.99% in the five units
   (49 single-function compiles) was wrapped in `#pragma optimize("a",
   on)` … `#pragma optimize("", on)` for one compile.
2. **Real near misses.** The full baseline filters out the 33 whose only
   difference is a call name that the link resolves (they are 100% there).
3. **Source forms.** For the remaining 16: hand rewrites of statement and
   expression shape, then `make match-search` (`decls`, `operands`,
   `stores`) where the hand rewrites stalled. Budget: about 10 variants or
   one 5-minute search per mode per function.

### 1. The diagnostic

`/Oa` lifts only `00404100` (58.82 to 100* single-function, effective
match: registers right, the two global-list pushes store in the other
order). Every other function stays the same (21) or falls (27, down to
31.30). So the diagnostic is specific to `00404100`, and even there it
pointed at a different cause, below.

## Per-function results

| Address | Function | Unit | `/Oa` | Before (full) | After | Source form |
| --- | --- | --- | --- | ---: | ---: | --- |
| `00404100` | aitask_command_queue_sweep_relink | aitask.c | 100* | 58.82 | **100** | `task->next = NULL;` written once after `if (kept) kept->next = task; else kept = task;` |
| `004039a0` | aitask_task_record_create_and_scan_grid | aitask.c | no change | 94.12 | **100** | the created task in a local before `aitask_cmd_set_kind(task, 9)` |
| `004041b0` | aitask_entity_task_chain_contains_kind | aitask.c | no change | 93.33 | **100** | `type == task->type` |
| `004848d0` | Flic_BrunDepack | flic_unit.c | no change | 96.39 | **100** | `src++` in the `for` increment after `line++` |
| `0048d7c0` | lwo_app_obj_load | lwo.c | lower | 96.88 | **100** | `obj = *ao; free(obj);` |
| `00483c80` | Flic_Setup | flic_unit.c | lower | 89.24 | **100** | header width and height read into two locals, then stored to `ddsd` and the struct |
| `004085f0` | building_get_camera_null | building_model.c | no change | 90.00 | 90.00 | not found (10 forms) |
| `00478e60` | block_open | config.c | no change | 92.16 | 92.16 | not found (11 forms; `operands`, `stores` searches) |
| `0048d2c0` | lwo_load | lwo.c | lower | 99.29 | 99.29 | not found (5 forms) |
| `00403d10`, `00404e00` | | aitask.c | | 99.39, 98.65 | | call targets only (port adapters); linkage, not register choice |
| `00405950` | aitask_pending_command_timer_fire_scan | aitask.c | lower | 98.62 | | a port adapter's float return changes the x87 code |
| `00404280`, `0048c820`, `00403810` | | aitask.c, lwo.c | | 83.07, 80.66, 76.83 | | x87 scheduling and block layout; not pointer chasing, not attempted |
| `004792e0` | block_find | config.c | no change | 98.12 | | `conf = conf->next` loads through `eax`, not `ebx`; not attempted |
| `00404fc0` | aitask_place_command_eligibility_check | aitask.c | lower | 99.43 | 99.43 | see "A second counter" |

`*` an effective match (the original's instructions up to register names).

### `00404100`: one store, duplicated by the compiler

The port wrote the two "kept" branches with a `task->next = NULL;` each.
The original's code has those two stores too, but the source had one:

```c
} else {
    if (kept != NULL) {
        kept->next = task;
    } else {
        kept = task;
    }
    task->next = NULL;
}
```

MSVC 5.0 duplicates the shared tail into both arms (each arm ends in its
own jump back to the loop test), so the instructions are the same. The
register allocation is not: with one store fewer in the intermediate code,
`next` takes `esi`, `kept` `edi` and the zero constant `ebx`, exactly as in
the original. `/Oa` reached the same allocation by another route, because it
also weakens what a pointer store kills. That is why it gave the registers
and the wrong store order together. Rule 4 of finding 663 should read
"fewer stores in the source", not "fewer aliasing hazards".

The function's `flags` copy is dropped (`task->flags` is read in each
test; MSVC keeps it in `ecx` because nothing stores between the tests). Both
forms give `00404100` exactly. The copy is dropped because `00404100` sits
before `00404fc0`, and see the next section.

### A second counter: locals before `00404fc0`

Adding `004039a0`'s local alone lowers `00404fc0` (single-function 98.19
to 97.96: two loads of `test eax, edx` swap). Measured with in-place edits:

| Change | `00404fc0` |
| --- | ---: |
| 1, 2, …, 8 or 16 unused locals in `004039a0` | 97.96 |
| one unused local in any of 7 functions before `00404fc0` | 97.96 |
| one unused local in `00405950` (after it) | 98.19 |
| 1–255 `extern` pads before `00404fc0`, or after the includes | 98.19 |
| `004039a0`'s local and `00404100` without `flags` | 98.19 |

So `00404fc0` depends on how many locals precede it in the unit, not on
finding 538's declaration counter (which externs move), and not with a
period visible up to 16. With the local count before it unchanged, both
rewrites keep it. `match-search` `decls` and `operands` (5 minutes, 819
candidates) found nothing that recovers it at the shifted count.

`tools/experiments/776/pads_at.py` showed no movement for any pad, even
where the in-place pads above move scores: it compiles a scratch copy
(`--candidate`), which loses the effect. Pad probes should be made in
place.

### `Flic_Setup`: values in non-address-taken temporaries

The original reads `*fsp` once, loads the header width and height, stores
both into `ddsd`, then into `fsXsize` and (after one reload of `*fsp`)
`fsYsize`, with no reload of the values. The port's chained assignments
(`(*fsp)->fsXsize = ddsd.dwWidth = …`) store one pair before loading the
second. Writing the four assignments as separate statements reloads
`ddsd.dwHeight` from the stack, because `ddsd`'s address is taken. Two
`unsigned int` locals (`width`, `height`) give the original's code; what is
left in the single-function compile differs only in call names
(`file_open` and friends), and the full report scores it 100%.

### `lwo_app_obj_load` and `Flic_BrunDepack`

`lwo_app_obj_load` frees `*ao` through `esi`, the register of the dead
`ao`. A local copy (`obj = *ao; free(obj);`) gives that, `free(ao[0])`,
casts and branch inversions do not. `Flic_BrunDepack` increments `line`
before `src`. Only the `for` increment `line++, src++` (or a `while` with
`line++; src++;` at the end) gives it. `src++, line++` does not.

### Not found

- **`004085f0` building_get_camera_null** saves `edi` before the null test
  in the original and after it here (finding 592; same as Creature.c's
  `00406f80`). Ten forms: early return, `return x = …`, a slot pointer, an
  initialised result, `!= NULL` tests, the sibling functions' shapes. None
  moves the save. The join point of the original's null path is `pop edi`.
  This compile's is past it.
- **`00478e60` block_open**: `comment` and the first loop's `loop` swap
  `ebx`/`edi`. Types, initialisation placement, declaration order, loop
  form, the comment test as two `if`s, `root`/`conf` assignment shapes and
  both searches give 92.16.
- **`0048d2c0` lwo_load**: one `lea` register at the `formSize` read and
  `i++` after the compiler's strength-reduced `newSurf` pointer. A pointer
  local, signedness of `i`/`surfNameCount` and other forms do not move it.

## The rule for future sweeps

Finding 663's rule 4 becomes:

4. **Try the aliasing diagnostic** on short pointer-chasing functions. If
   `/Oa` gives the original's registers, look first for **statements the
   port duplicated that the original wrote once** (shared tails of
   `if`/`else` arms), then for values the original passed through a local
   where the port re-reads memory. MSVC 5.0 duplicates small shared tails,
   so the instructions alone do not show it. Do not commit the pragma.
5. **When a rewrite adds or removes a local, re-score the later functions
   in the unit.** Some functions depend on the count of locals before them,
   which extern pads do not move.

## Files

- `src/game/aitask.c`: `004039a0`, `00404100`, `004041b0`.
- `src/gods98/flic_unit.c`: `00483c80`, `004848d0`.
- `src/gods98/lwo.c`: `0048d7c0`.
- `docs/coverage/`: regenerated.

No file owned by #783, #785 or #786 is touched. No `#pragma optimize` is
committed.
