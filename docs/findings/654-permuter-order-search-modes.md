# Finding 654 — Order search modes for the permuter, and their first application pass

Issue #760, part of #388 and ADR 0005. The near-miss sweeps (findings 644,
645, 648, 649 and 650) left most short functions as cause C (integer
register choice) or D (scheduling, stack slots, statement shape), marked
"not found". The wins that did land there came from ad-hoc exhaustive
searches: finding 635's store orders and finding 649's case orders. This
change makes them one reusable tool, `tools/permuter_search.py`
(`make match-search`; `msvc_permuter.py ADDR --mode ...` hands over to it),
and applies it to finding 650's gods98 cause-C/D list and AITask.c's
95–99% band.

Measured on `main` `1e84f9da`, with one full `make reccmp-check` after the
change; rebased onto `main` `f0be860e` (#763–#769) and measured again, below.

| Measure | `main` `1e84f9da` | This change |
| --- | ---: | ---: |
| Exact-function code bytes | 329,171 (51.23%) | 329,171 (51.23%) |
| Accuracy, full / game-only | 96.98% / 96.64% | **96.99%** / 96.64% |
| Mapped / exact, full | 2,833 / 2,397 | 2,833 / 2,397 |
| Functions that rose / fell, full | | **4 / 0** |
| `opt_ref` annotated removed | 0 | 0 |
| Image end (gate) | `0x7f5000` | `0x7f5000` |

**Rebased onto `f0be860e`** (the stacked #758, #759, #761, #762, #766 and
#767 work), with a full `make reccmp-check`: the same four functions rise by
the same amounts, and nothing else moves.

| Measure | `main` `f0be860e` | This change on it |
| --- | ---: | ---: |
| Exact-function code bytes | 335,096 (52.15%) | 335,096 (52.15%) |
| Accuracy, full / game-only | 97.02% / 96.68% | 97.02% / 96.68% (means rise below the second decimal) |
| Functions that rose / fell, full | | 4 / 0 |
| Image end | `0x7f5000` | `0x7f5000` |

No exact match was lost and no function fell below the baseline. `make
match-progress`, `make unit-data-offsets`, `make opt-ref-callers
OPT_REF_ARGS=--summary` and `make pe-audit` were run and the generated files
committed (`unit-data-offsets` changed no tracked file). In
`binary-progress.json` one 32-byte function moves from the `opt_ref`
`port_unit` category to `original_annotated` (2,532 to 2,533 annotated
functions), on both bases. No annotation changed here, and the four reorders
change no function's size, so the cause was not identified. The figure comes
from the `/OPT:NOREF` relink's translation of the report's rows (it is not
in the gate link's categories, which `make winapi-link-check` prints
unchanged).
`make tools-test` (no `--allow-growth`), `make winapi-link-check` and
native ctest pass.

## The tool

Each mode splits the function into groups and scores every order of a
group (all of them up to `--exhaustive` items, default 6; single insertion
moves for larger groups) with the other groups at their best. Rounds repeat
until one finds nothing better; the operand mode then tries every pair of
non-overlapping sites. The full description, with the dependence rules, is
in `tools/README.md`.

| Mode | Group | What stays in order |
| --- | --- | --- |
| `stores` | a run of one-line assignments/increments | statements that read or write what another writes (by name: same variable, same member through any pointer, any dereference; a write of a global or an address-taken local against every member and dereference); calls end the run |
| `cases` | one `switch`'s case blocks | a block that falls through travels with the next; a last block that falls out stays last |
| `decls` | a block's leading declarations; one line's declarators | initializers that name an earlier declaration or both call; in C++ only builtin scalars and pointers |
| `operands` | one `+ * & \| ^ == !=` or mirrored `< > <= >=` (C only) | operands with side effects; a call against anything but literals; casts' and declarations' `*`/`&`; parentheses added for same-precedence and macro operands |

Two properties the ad-hoc searches lacked:

- **Resumable and capped.** `--seconds` ends a run with exit status 3 and
  `state.json` (round, group, candidate index, the scored hashes, the round's
  base and the best source); `--resume` continues exactly there and refuses if
  the source changed. `result.json` is the best-so-far report: before and
  best scores, every improvement, the ten best candidates, the groups visited
  and up to five compile-failure samples.
- **In-process, in-place scoring.** The project context is built once.
  Each candidate is written over the real source, compiled with the matching
  flags, and the original is written back before the object is analysed
  (about 0.3–0.5 s a candidate, against 3.4 s for a `match_function.py`
  call). A lock file and a copy of the original in the output directory let a
  search killed mid-compile restore the file on its next start.

**Validation.** `front_menu_item_create_cycle` (`00410a10`, exact) with its
six stores reversed scores 87.72% in the single-function loop. The store
search climbed back to **100%** in 632 candidates (about 5 minutes).

`tools/tests/test_permuter_search.py` (in `make tools-test`) covers each
mode's grouping and refusals, and compiles every candidate of a store, case
and operand fixture with the host compiler. Each candidate must give the
same output as the original. It also checks the search loop, its
exact-score stop, and a budget stop and resume that ends with the same
attempts and result as one uninterrupted run.

## The application pass

22 functions: finding 650's 18 gods98 cause-C/D near misses and AITask.c's
four 95–99% functions (3,840 ledger bytes). Each was searched in every mode
from the committed source, sequentially, with caps of 300 s per
stores/cases/decls search and 600 s per operand search. The wins were then
combined, and a second round (stores, decls, operands) ran on the four
improved functions from the combined source.

### Hit rate per mode

"Searchable" means the mode found at least one candidate; a hit is a
function whose single-function score rose.

| Mode | Searchable | Candidates scored | Hits | Hit rate (of searchable) | Time |
| --- | ---: | ---: | ---: | ---: | ---: |
| `stores` | 13 / 22 | 935 | 3 (`0047bdb0`, `00473690`, `00481c50`) | 23% | 5.3 min |
| `cases` | 1 / 22 | 719 | 0 | 0% | 3.4 min |
| `decls` | 19 / 22 | 3,632 | 1 (`00481c50`) | 5% | 20.4 min |
| `operands` | 21 / 22 | 4,907 | 2 (`00472cd0`, `0047bdb0`) | 10% | 27.4 min |
| second round (4 functions) | | 2,869 | 0 | | |

Four distinct functions rose (18% of the 22). None became exact. Each gain is
a scheduling or register change from an order the source did not pin: one
store moved, one declaration with an initializer moved, two comparison
operands mirrored.

The operand searches recorded 67 compile failures, all from two parsing
mistakes found during the run and fixed: `if (p) *p = x` read as a product,
and the `*` of a `(T **)` cast. `00404fc0`'s search was restarted with the
fixed tool.

### Per-function results

Scores are full `make reccmp-check` scores (the single-function loop's in
parentheses where it differs). Candidate counts are stores/cases/decls/
operands (operands: singles + pairs scored); `*` marks a search that ran out
of budget.

| Address | Function | Cause (650) | Candidates s/c/d/o | Before | After | Change |
| --- | --- | --- | --- | ---: | ---: | --- |
| `00472cd0` | container_load | D | 0/0/756/92 | 85.36 | **88.85** | `if (0.0f != scale)` (operands) |
| `00481c50` | mesh_uview_mesh | D | 7/0/158/428 | 85.06 | **86.97** (86.21) | `lwVertCount++` before `newVertCount++` (stores); `uvFile` declared after `poly` and `lineCount` (decls) |
| `00473690` | entity_visual_resource_load | D | 149/0/725/190 | 83.22 | **83.91** | `cloneData->used = 1` first of the three stores (stores) |
| `0047bdb0` | sound_stream_ring_buffer_tick | C/D | 2/0/49/92 | 81.98 | **82.69** | `writeProgress` before `writeOffset` (stores); `bufferSize <= writeOffset` (operands); each alone 82.33 |
| `0048d2c0` | lwo_load | C | 8/0/196/105 | 99.29 | 99.29 | |
| `0047ad70` | s_play_seam | C | 0/0/51/77 | 97.55 | 97.55 | |
| `00476410` | Container_Frame_SearchCallback | C | 0/0/23/104 | 99.44 | 99.44 | |
| `0048d7c0` | lwo_app_obj_load | C | 0/0/0/0 | 96.88 | 96.88 | no candidate in any mode |
| `004876c0` | lws_initialise | C | 5/0/0/3 | 98.11 | 98.11 | |
| `0047baa0` | sound_streamed_wave_buffer_setup | C | 29/0/119/20 | 82.51 | 82.51 | |
| `0047bc30` | sound_stream_drain_check | C | 1/0/36/28 | 84.77 | 84.77 | |
| `00473370` | scene_action_name_set | C | 1/0/719/3 | 80.39 | 80.39 | |
| `00482040` | mesh_set_texture_time2 | C | 0/0/0/15 | 85.19 | 85.19 | |
| `00481820` | mesh_get_surf_info | D | 0/0/145/28 | 97.98 | 97.98 | the slot order is not declaration order (as 650 found by hand) |
| `00473c60` | xf_texture_name_format | D | 0/0/23/15 | 98.99 | 98.99 | |
| `004869f0` | lws_parse | D | 3/0/59/1,378 | 94.80 | 94.80 | all 52 sites and 1,326 pairs |
| `0048c820` | lwo_surface_settings_read | D | 2/719/484/325 | 80.66 | 80.66 | the only switch in the list (7 groups; every insertion move ties) |
| `00472710` | container_initialise_original | D | 719/0/1/1 | 80.43 | 80.43 | all 720 orders of its six default-string stores tie or fall |
| `00403d10` | aitask_entity_command_unlink_by_kind | AITask 95–99 | 6/0/49/102 | 99.39 | 99.39 | |
| `00404e00` | aitask_object_path_submit_to_grid_target | AITask 95–99 | 0/0/23/6 | 98.65 | 98.65 | |
| `00404fc0` | aitask_place_command_eligibility_check | AITask 95–99 | 0/0/3/1,684* | 99.43 | 99.43 | unfinished: pairs phase at 1,582 of ~5,150 |
| `00405950` | aitask_pending_command_timer_fire_scan | AITask 95–99 | 3/0/13/211 | 98.62 | 98.62 | |

Second-round candidates (from the combined source): `0047bdb0` 1/49/91,
`00481c50` 5/85/428, `00472cd0` 0/717*/91, `00473690` 148/718*/136
(stores/decls/operands); no gain.

The four reorders are semantics-preserving by inspection: independent
stores to distinct members, a mirrored comparison of a non-volatile float
and of two `unsigned` members, and a declaration whose initializer reads only
a parameter.

### Unfinished searches

- `00404fc0` operands, pairs phase: stopped by its 600 s cap at pair group
  1,582 (of about 5,150). Resume with
  `python3 tools/permuter_search.py 0x00404fc0 --mode operands --seconds 1200
  --output build-match/search/00404fc0-operands --resume` on an unchanged
  `aitask.c` (the state refuses a changed source; start fresh after any edit).
- `00472cd0` and `00473690` decls, second round: stopped by the 240 s cap at
  717 and 718 of 756 and 725 candidates.
- Finding 649's `0041dca0` outer-switch search (Interface.c) is not resumed
  here: `interface_unit.c` belongs to #761 in this wave. The cases mode with
  `--lines 2237` does that search.

## What this says about causes C and D

The orders the modes can express were not what kept these functions short:
of 22 functions only 4 rose, by 0.7–3.5 points, and none became exact. The
remaining C/D differences need shapes outside the four modes, such as
multi-line statement moves, temporaries, loop forms and expression rewrites
across statements. `msvc_permuter.py`'s rewrites tried some of these and
found nothing (finding 650). Later sweeps should still run the four modes
first: they are cheap (a full pass over a function is usually 1–10 minutes),
and the store and case searches of findings 635 and 649 show that a hit can
be worth a whole function.

## Files

- `tools/permuter_search.py` (new), `tools/tests/test_permuter_search.py`
  (new), `tools/msvc_permuter.py` (`--mode` hand-over), `Makefile`
  (`match-search`, the test in `tools-test`), `tools/README.md`.
- `src/gods98/container.c`, `src/gods98/mesh_unit.c`, `src/gods98/sound3d.c`:
  the four reorders.
- `docs/coverage/match-baseline.txt`, `match-listing-digests.txt`,
  `binary-progress.json`: regenerated.

No file owned by #758, #759, #761 or #762 is touched. The validation run
edited `frontend_unit.c` in this worktree only, and restored it.

## Reproducing

```sh
make reccmp-check
make match-progress
make match-search ADDR=0x00472cd0 MODE=operands SECONDS=600   # and the table's other searches
```
