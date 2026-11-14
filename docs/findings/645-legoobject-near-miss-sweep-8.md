# Finding 645 — Near-miss sweep 8 of LegoObject.c

Issue #741, part of #388 and ADR 0005. It follows the earlier LegoObject.c
sweeps (findings 626 and 629) and the caller switches of #735–#738
(findings 638–641). LegoObject.c is GAME-023, in `src/game/legoobject_unit.c`.
This sweep covers its 15 functions at 95–99% (8,352 bytes) and its 19 at
80–94% (8,976 bytes).

Measured on `main` `c46f8e97`, with one full `make reccmp-check` before the
change and one after.

| Measure | `main` | This change |
| --- | ---: | ---: |
| Exact functions, full | 2,259 | **2,263** (+4) |
| Exact-function code bytes | 303,156 (47.18%) | **305,176 (47.49%)** |
| Reccmp mean, full / game-only | 96.48 / 96.20 | 96.48 / 96.20 |
| Functions that rose / fell | | 6 (+1 newly mapped) / **0** |
| `opt_ref` removed / removed exact | 27 / 23 | 28 / 24 (one `/OPT:ICF` twin swap, below) |
| Image end | `0x875000` | `0x875000` |

No exact match was lost and no function fell below the baseline. `make
match-progress`, `make unit-data-offsets` and `make opt-ref-callers
OPT_REF_ARGS=--summary` were run, and the three generated files are
committed.

## What closed the gaps

### 1. A callback Ghidra merged into Weapons.c's `00471fc0` (`00437540`)

`legoobject_remove` (`00437540`) passes `0x00472060` to
RunThroughLists. Nothing annotated that address: Ghidra merged it into
the preceding `00471fc0`, and the ledger (`translation-units.csv`) has no row for
it. The port's matching build passed a `PORT-ONLY` adapter that does
nothing. The original is 31 bytes:

```
mov eax, [esp+4] ; mov edx, [esp+8] ; mov ecx, [eax+0x3c4]
cmp ecx, edx ; jne +10 ; mov dword [eax+0x3c4], 0 ; xor eax, eax ; ret
```

The callback clears an object's laser-hit reference (`+0x3c4`) when that
reference is the object being removed. `weapons.c` now defines it as
`weapons_callback_remove_lazer_hit_reference`, annotated `0x00472060`,
between `00471fc0` and `00472080`. This follows AITask.c's `0x00402a70`,
another merged callback. It matches at 100%, and no other Weapons.c
function changes (`make match-function --source src/game/weapons.c`).
In the matching build, `legoobject_unit_imports.h` points the adapter's
name at the new function with an object-like alias placed before the
name's only declaration. The counter does not move. Natively the adapter
stays. `00437540` goes from 99.47% to **100%**.

### 2. A temporary for a nested call (`0043af30`)

`legoobject_creature_spawn_from_emergence` called
`sound_submission_route_wrapper(legoobject_type_component_get(obj), 10, 0,
1, NULL)`. MSVC pushed the four constants first and then called the getter.
The original calls the getter first, then pushes the constants and `eax`:
the argument was a temporary in the original source. With the temporary,
an unrelated register choice two statements earlier also matches (the
`flags1 |= 0x100` update in `ecx`, not `eax`). That was finding 626's
cause C for this function.

A new local costs one symbol (rule B, finding 538). In this unit that
symbol cost `legoobject_state_pair_predicate` (`0043c200`) its exact
match. The counter probe (`tools/experiments/699/counter_probe.py`) showed
+1 from `0043b270` to the end of the file. The temporary is therefore
`found`, an `int` that is dead at that point and reset two lines later.
This form compiles only for the 32-bit matching build (`_MSC_VER < 1200`);
the native build keeps the nested call, because a pointer does not fit
in an `int` there. The counter is unchanged. 97.89% to **100%**.

### 3. One return instead of two (`00447c40`)

`legoobject_activity_register_bookkeeping` returned `result` inside the
`if` and `0` after it. The rebuild therefore had a second epilogue
(`pop esi; xor eax, eax; pop ebx; add esp; ret`), and every branch over
the body was 11 bytes longer. The original has one exit, `return result`,
with `result = 0` initialised. The counter is unchanged. 96.85% to
**100%**. Finding 626 had recorded this as cause D, block layout.

### 4. The teleport queue's count reload (`0043f190`, `Deviation (scoring only)`)

`legoobject_spawn_request_queue_process` ends by shifting
`legoGlobs.objTeleportQueue` down one entry. In the original, the queue and
its count are fields of one `legoGlobs`. MSVC must therefore assume that
the stores into the queue can change the count, and it reloads the count
on every pass except the first. On the first pass, it compares against the
decremented count still in a register. The port keeps the count as a
separate global (`legoGlobs_f_556514`), so the rebuild counted the loop
down in a register (`sub ecx, ebp; ...; dec ecx; jne`).

The loop is now written rotated:

```c
if (i < --legoGlobs_f_556514) {
    do { ...; i++; } while (i < *(volatile unsigned int*)&legoGlobs_f_556514);
}
```

This is the same remedy as `0043a960`'s volatile increment (finding 629).
The remaining difference is the volatile load, which is scheduled one
store earlier. A volatile store to the last element does not keep MSVC
5.0 from moving it. The rotated form costs one counter symbol from
`0043f560` on. Full Reccmp and `make match-function` both show no change
in any later function. 94.53% to **99.67%**.

### 5. Smaller gains

- **`00437940`** (82.35% to 84.17%): `rec->heading * 0.15915494f * 8.0f` was
  folded into one multiply. The original multiplies twice (`fmul [1/2π]`,
  then `fmul [8.0]`). `(float)(rec->heading * 0.15915494f) * 8.0f` keeps
  both. Lego.c's `0042e020` (`lego_unit.c`, #740's file) has the same
  expression and may have the same fold.
- **`004469c0`** (81.79% to 82.72%): `best`, `bestCount` and `found` are
  initialised in their declarations, ahead of `valid` and the two direction
  tables. The original stores the three `-1`s before the table
  initialisers. Only stack slot assignment is left in that region.

## Per-function results

Sizes are the ledger's address gaps. `main` and After are full `make
reccmp-check` scores. The letters are the causes tallied below.

### The 95–99% band

| Address | Function | Size | `main` | After | Change, or what is left |
| --- | --- | ---: | ---: | ---: | --- |
| `0043bc40` | legoobject_objective_sidecar_append_gate | 1472 | 99.11 | 99.11 | A: `osf_add` (`00459190`) and `entity_cell_path_effects_apply_current` (`00409d50`) are C++ port entries; `00409ff0`'s marker in `entity.cpp` is inside a `/* */` comment, so Reccmp does not map it |
| `00436c20` | legoobject_initialise | 1072 | 96.67 | 96.67 | D: `rep stosd` scheduled against two immediates (effective match) |
| `0043af30` | legoobject_creature_spawn_from_emergence | 832 | 97.89 | **100.00** | the container taken into a temporary before the call (section 2) |
| `0044c1b0` | legoobject_heading_step_move | 752 | 97.75 | 97.75 | E: x87 scheduling in the inlined normalise |
| `00437540` | legoobject_remove | 624 | 99.47 | **100.00** | Weapons.c's `00472060` added and called (section 1) |
| `004428a0` | legoobject_world_position_set | 624 | 98.94 | 98.94 | E: a float self-copy is `fld`/`fstp` in the original, `mov` here |
| `00447c40` | legoobject_activity_register_bookkeeping | 608 | 96.85 | **100.00** | one `return result` (section 3) |
| `004401b0` | legoobject_terrain_cell_to_world_position | 544 | 98.06 | 98.06 | D: a float argument stored with `push ecx; fstp [esp]` in the original, into the dead argument slot here |
| `00441ed0` | legoobject_face_target_action_start | 512 | 98.75 | 98.75 | E: `faddp`/`fsqrt` scheduled against the pushes |
| `00437150` | legoobject_tool_tip_icons_load | 336 | 98.33 | 98.33 | B: two end-of-array pointers name whatever global follows (`named_keys`, `0x5679f0`) |
| `0043ad50` | legoobject_predug_spawn_coordinate_select_and_create | 336 | 99.02 | 99.02 | D: load/store order of a pair copy (effective match) |
| `004477d0` | legoobject_path_step_lookahead_write | 304 | 97.70 | 97.70 | D: `pop edi` scheduled into the last struct copy in the original |
| `00438310` | legoobject_pool_chunk_allocate | 144 | 97.22 | 97.22 | B: the volatile count reload is scheduled one instruction early (effective match) |
| `00437050` | legoobject_shutdown | 96 | 96.00 | 96.00 | B: end-of-array pointer names the next global |
| `004385b0` | legoobject_shared_eligibility_check | 96 | 96.00 | 96.00 | B: a flag immediate lands inside `fileRedundantList` on both sides |

### The 80–94% band

| Address | Function | Size | `main` | After | Change, or what is left |
| --- | --- | ---: | ---: | ---: | --- |
| `004469c0` | legoobject_nearest_connected_cell_route_select | 1152 | 81.79 | **82.72** | the `-1` initialisers in the declarations (section 5); D: stack slots, and a `result` the original keeps in memory |
| `00442b10` | legoobject_movement_accumulator_apply | 1136 | 86.12 | 86.12 | E: x87 scheduling in the movement step |
| `004403d0` | legoobject_level_debug_cell_retarget_dispatch | 992 | 86.12 | 86.12 | D: block layout of a switch |
| `0043f190` | legoobject_spawn_request_queue_process | 976 | 94.53 | **99.67** | rotated loop and volatile count read (section 4); B: the reload is scheduled one store early |
| `00437940` | legoobject_deferred_olist_create_and_link | 736 | 82.35 | **84.17** | the heading's two multiplies kept (section 5); B: `objectGlobs_recordObjs` layout, and register choice |
| `0043eea0` | legoobject_simulation_queue_flush | 608 | 89.26 | 89.26 | D: the original keeps `(i == 0) == 1` as `sete`/`cmp 1`; MSVC folds every C form tried, then pushes `ebp` for 0 |
| `0043a120` | legoobject_cell_step_command_issue_with_jitter | 480 | 81.00 | 81.00 | E: x87 jitter computation |
| `00447210` | legoobject_adjacent_placement_cell_search | 416 | 85.07 | 85.07 | C: register rotation through a lookup loop |
| `0044afb0` | legoobject_tiny_monster_spawn_and_teardown | 400 | 85.71 | 85.71 | D: an `int`-to-float temporary's slot and a hoisted load |
| `0043fe70` | legoobject_route_to_explicit_target | 384 | 82.39 | 82.39 | D: stack slot assignment |
| `0043fff0` | legoobject_route_marker_setup | 320 | 85.37 | 85.37 | D: stack slots (one local in the dead parameter slot) |
| `0043f6a0` | legoobject_damage_apply | 304 | 94.44 | 94.44 | C: `ebx`/`edi` swapped for the damage value |
| `004492b0` | legoobject_cell_step_command_issue_with_target_mark | 304 | 84.62 | 84.62 | D: the original keeps both route pointers in the dead parameter slots (frame `0xc`); the rebuild reuses one (frame `0x10`) |
| `0044bf00` | legoobject_timed_callback_list_update | 208 | 87.14 | 87.14 | B: `s_entity_auxiliary_release_records` (port record) |
| `0044a200` | legoobject_type_component_get | 160 | 91.49 | 91.49 | D: the original reloads the parameter for `return obj` |
| `0043aea0` | legoobject_emergence_random_spawn_attempt | 144 | 92.00 | 92.00 | D: the original reserves one more frame slot |
| `004372a0` | legoobject_spawn_coordinate_pool_clear | 96 | 91.67 | 91.67 | D: `rep stosd` scheduled after three stores (effective match) |
| `0043c3e0` | legoobject_type26_command_completion_gate | 96 | 84.85 | 84.85 | C: `ecx`/`edx` for `eax` on the two `aiTask->targetObject` loads (effective match) |
| `00439950` | legoobject_tool_name_strings_load | 64 | 92.00 | 92.00 | B: start and end pointers name `named_keys` and the next global |

Also newly mapped: Weapons.c's `00472060`
`weapons_callback_remove_lazer_hit_reference`, at 100%.

## Why the other 31 are short

| Cause | Functions | Ledger bytes | What would close it |
| --- | ---: | ---: | --- |
| **A. Callee outside this unit**: C++ port entries `osf_add` and `00409d50`, and an `00409ff0` marker inside a comment (`0043bc40`) | 1 | 1,472 | GAME-008 (`entity.cpp`, `entity_cell_effects.cpp`) and ObjectStateFile in original shape; then a link-name alias here |
| **B. Data layout**: end-of-array pointers (`00437150`, `00437050`, `00439950`), the `fileRedundantList` immediate (`004385b0`), the count reloads of `objectGlobs`/`legoGlobs` fields kept as separate globals (`00438310`, `0043f190`), `objectGlobs_recordObjs` (`00437940`), a port record (`0044bf00`) | 8 | 2,656 | legoGlobs and objectGlobs as single structs; layout identity |
| **C. Integer register choice**: `00447210`, `0043f6a0`, `0043c3e0` | 3 | 816 | not found |
| **D. Scheduling, stack slots, block layout or compare shape**: `00436c20`, `004401b0`, `0043ad50`, `004477d0`, `004469c0`, `004403d0`, `0043eea0`, `0044afb0`, `0043fe70`, `0043fff0`, `004492b0`, `0044a200`, `0043aea0`, `004372a0` | 14 | 6,816 | not found |
| **E. x87 scheduling and float copies**: `0044c1b0`, `004428a0`, `00441ed0`, `00442b10`, `0043a120` | 5 | 3,504 | not found |

Five of the remaining functions are effective matches: `00436c20`,
`0043ad50`, `00438310`, `004372a0` and `0043c3e0`.

What was tried without success, so a later sweep need not repeat it:

- `tools/msvc_permuter.py`, 90 s each, on `0044a200`, `004477d0`,
  `0043c3e0`, `004492b0`, `0043eea0` and `0043aea0`: no improvement.
- `0043c3e0`: nested `if`s, `== 0` for `!`, the result returned as an
  expression, and the casts dropped. All score the same.
- `0043eea0`: `!i == 1`, `(int)(i == 0) == 1`, `(unsigned)`, `(char)`,
  `?:`, `(i < 1)`, `(i <= 0)`, `& 1`, `+ 0`, `* 1`, `1 == (i == 0)`. MSVC
  folds every form, and `(i <= 0) == 1` gains only 1.7 points (`ja` for
  `jne`).
- `004492b0`: all orders of its five locals, and three control-flow shapes
  of the path branch. The frame never shrinks to `0xc`.
- `004372a0`: four statement orders of the three `memset`s and the stores.
- `0043f190`: a volatile store into the queue, so that the count's
  volatile reload stays after it. MSVC 5.0 still schedules the load
  first.

## `/OPT:REF`

`opt_ref` goes from 27 / 23 to 28 / 24. Every live-caller and unreferenced
entry is unchanged. The `/OPT:ICF` folds go from 13 (12) to 14 (13), because
`rng_thunk_draw15` (Maths.c `004798a0`) now folds into its twin in
`level.cpp.obj`. Weapons.c grew 0x20 bytes, which moves every later object
by 0x20, and LINK chose the other twin. Finding 641 lists this function
among the ICF folds that `/OPT:NOICF` (#562) removes. The new `00472060` is
live: `00437540` pushes it.

## Files

- `src/game/weapons.c`: `00472060` added.
- `src/game/legoobject_unit_imports.h`: the matching-build alias of the
  `00472060` adapter.
- `src/game/legoobject_unit.c`: `0043af30`, `00447c40`, `0043f190`,
  `00437940` and `004469c0`.
- `docs/coverage/`: the match baseline, listing digests and binary
  progress, from `make match-progress`. `make unit-data-offsets` leaves
  the tree unchanged.

No file owned by #562, #447, #740 or #453/#481 is edited. Lego.c's
`0042e020` is only named as a lead.
