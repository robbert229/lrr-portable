# Finding 544 — Steered-group unit (GAME-016) rebuilt in original shape

Issue #491, under [ADR 0005](../adr/0005-original-shape-c-units.md). R1
(finding 527) relocated GAME-016 verbatim into `src/game/steering.cpp`. This
change rebuilds it as the C unit [`src/game/steering.c`](../../src/game/steering.c),
with the C boundary header [`steering.h`](../../src/game/steering.h). The unit
is text `0040f6c0`–`004102b0`. Its 0xa0-byte items each turn towards a target
and move at it. A 0x28-byte group record holds a leader and a stack of
followers, linked through item `+0x9c`. The followers wander around the
leader's position.

"Before" is the *match baseline* on `main` at `8e8d4da3` (after #506). "After"
is this change. `make reccmp-check` and `make match-function` give the same
per-function scores.

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (byte-weighted, all 30 functions, 2,842 bytes) | 17.51% | **86.60%** |
| Exact functions | 2 | **24 of 30** |
| Mapped functions | 19 | 30 |
| Accuracy, full (`binary-progress.json`) | 55.86% | 56.34% |
| Accuracy, game-only | 51.10% | 51.65% |
| Exact-function code bytes | 64,843 | 65,713 |
| Adjacent order inversions, full | 632 | 632 |

Byte weights are the original's code bytes, without the `0x90` fill. An
unmapped function counts as 0%. Pair inversions rise from 320,302 to 320,874
only because 11 more functions are mapped. Inside the unit, all 30 functions
link in original order (see "Static callbacks" below).

| Address | Function | Bytes | before | after |
| --- | --- | ---: | ---: | ---: |
| `0040f6c0` | `scene_node_record_create` | 238 | 7.27 | 82.93 |
| `0040f7b0` | `optional_resource_destroy` | 18 | 100.00 | **100.00** |
| `0040f7d0` | `callback_state_record_create` | 121 | 36.78 | **100.00** |
| `0040f850` | `callback_state_list_dispose` | 62 | 46.38 | **100.00** |
| `0040f890` | `direction_driven_heading_angle_step` | 133 | unmapped | **100.00** |
| `0040f920` | `kind_two_steering_torque_apply` | 465 | unmapped | 81.48 |
| `0040fb00` | `matrix_rotation_compose_xyz` | 130 | 100.00 | **100.00** |
| `0040fb90` | `steering_physics_step` | 289 | unmapped | 98.97 |
| `0040fcc0` | `vector_scale_components_in_place` | 58 | 91.30 | **100.00** |
| `0040fd00` | `steering_item_wander` | 169 | unmapped | 89.66 |
| `0040fdb0` | `three_value_record_copy_and_mark_ready` | 36 | 53.85 | **100.00** |
| `0040fde0` | `set_vector_and_notify_listeners` | 21 | 75.00 | **100.00** |
| `0040fe00` | `scene_object_list_update_transforms` | 61 | 32.14 | **100.00** |
| `0040fe40` | `ptrstack_push` | 32 | 6.45 | **100.00** |
| `0040fe60` | `ptrstack_pop` | 38 | 28.00 | **100.00** |
| `0040fe90` | `ptrstack_for_each` | 46 | 54.55 | **100.00** |
| `0040fec0` | `random_unit_float` (was `set_global_scalar_to_one`) | 16 | 90.91 | **100.00** |
| `0040fed0` | `steering_item_spread_tuning` | 121 | unmapped | 36.62 |
| `0040ff50` | `scene_object_list_apply_float_callback` | 24 | 87.50 | **100.00** |
| `0040ff70` | `object_vector3_set` | 32 | 83.33 | **100.00** |
| `0040ff90` | `steering_item_set_tuning` | 77 | unmapped | **100.00** |
| `0040ffe0` | `global_vector_set_and_notify` | 59 | unmapped | **100.00** |
| `00410020` | `scene_node_position_set` | 45 | 17.14 | **100.00** |
| `00410050` | `steering_item_set_position` | 28 | unmapped | **100.00** |
| `00410070` | `scene_object_list_apply_position_callback` | 24 | 75.00 | **100.00** |
| `00410090` | `steering_item_set_target` | 25 | unmapped | **100.00** |
| `004100b0` | `scene_object_list_apply_second_position_callback` | 24 | 75.00 | **100.00** |
| `004100d0` | `steering_item_follow_target` | 341 | unmapped | 53.99 |
| `00410230` | `steering_item_arrived` | 71 | unmapped | **100.00** |
| `00410280` | `callback_operation_run_and_return_status` | 38 | 20.00 | **100.00** |

The exported functions keep the port's names. `set_global_scalar_to_one` is
renamed `random_unit_float`: it returns `Maths_RandRange(0, 1)` in `st(0)`;
it does not set anything.

## The unit

### Functions the manifest misses

The Ghidra manifest has 23 functions. Seven more are callbacks that
`ptrstack_for_each` (`0040fe90`) receives as immediates. Nothing calls them
directly, so they show only as `LAB_` targets in the pseudocode:

| Address | Pushed by | What it does |
| --- | --- | --- |
| `0040fd00` | `0040fe2d` | wander: every `wanderPeriod` time units, a new target within `wanderRadius` of the leader |
| `0040fed0` | `0040ff59` | spread each current tuning value around its base by up to 1/spread |
| `0040ff90` | `00410001` | copy the four tuning values from the common at `00557850` into both copies |
| `00410050` | `00410079` | follower position = argument |
| `00410090` | `004100b9` | follower target = argument |
| `004100d0` | `0044bf1e` (GAME-023) | step, then move straight at the old target; arrive within one step |
| `00410230` | `0041028a` | clear the "all arrived" flag unless position == target; hide an arrived item (`scene_render_lock_toggle(container, 1)`) |

All seven are now annotated and mapped. Three functions the port had
defined in `entity.cpp` with inert `/* FUNCTION */` comments (`0040f890`,
`0040f920`, `0040fb90`) move into the unit. So does `view.cpp`'s
reimplementation of `0040ffe0` (`global_vector_set_and_notify` and its
notify hook).

### State

- **`.data`**: five floats at `004a1ee8`–`004a1ef8` (max pitch 0.8, pitch
  damping 0.95, roll gain 9, turn gain 10, climb threshold 1). They are
  annotated statics in address order.
- **`.rdata`**: the unit's float literals, in first-use order. The unit
  references `0049e148` (0.0) and `0049e168` (0.001), both outside the
  recorded range `0049e14c`–`0049e164`. Nothing else in `.text` references
  them. GAME-017's `.rdata` starts at `0049e170`. The unit's block is
  therefore `0049e148`–`0049e16c`:
  - 0.0, 1.0, 0.5, then four unreferenced bytes;
  - the double 0.01 at `0049e158`;
  - 0.1, 1e-5 and 0.001;
  - an unreferenced 2.0 at `0049e16c`, which MSVC folds into `fadd st, st`.

  The seven referenced items are annotated statics. Whether `0049e140`/`144`
  (1.0, −1.0) belong to GAME-015 or GAME-016 is not settled.
- **Common**: four floats at `00557850`, the arguments of `0040ffe0` for its
  walk. They are one plain file-scope struct, `steeringTuning` (C common,
  16 bytes).
- **No `.bss`.**

`make unit-data-offsets`, GAME-016 row:

| Unit | .data | .rdata | .bss | Commons |
| --- | --- | --- | --- | --- |
| GAME-016 | gaps-only (5, 0 unknown) | gaps-only (5, 0 unknown) | n/a (0, 0 unknown) fewer than two attributable items | `00557850:_steeringTuning(size=16)` |

Before, every column was n/a. The `.rdata` count is 5 of the 7 annotated
items because the range CSV still says `0049e14c`–`0049e164`.

### Types and contracts

- `steering.h` declares `SteeringItem` (0xa0 bytes on Win32) and
  `SteeringGroup` (0x28 bytes), with the float fields at the original
  offsets.
- The native port widens only the trailing `container` and `next` pointers
  and the group's three pointers. Nothing outside the unit addresses them by
  offset.
- Vectors are a three-float struct, `SteeringVector3`. The original's struct
  copies need it; they compile to integer moves.
- Callbacks take `(group, item, void* data)`. The port's scene-object hook
  table, NULL guards and `__builtin_memcpy` offset accessors are gone.
- The unit's two outside callees need C linkage. `scene_render_lock_toggle`
  (`00473b40`, G98-002) gets `extern "C"` in `record.hpp` and a new C
  boundary header [`gods98/record.h`](../../src/gods98/record.h).
  `optional_resource_destroy`, which `record.cpp` calls, gets `extern "C"` in
  `heap.hpp`. Both pairs are in `test_c_boundaries.cpp`.

### Shape notes

- **Maths.h inline helpers.** The original expands Maths.h-style inline
  helpers in this unit: modulus, scale, normalise, dot, cross, add and
  subtract. They are the same shapes `maths.c` uses for G98-007. Written as
  `static __inline` functions they raised the torque step from 27.63% to
  81.48%, the physics step from 68% to 98.97% and the follow callback from
  48% to 53.99%.
  - As macros, the same expressions compile differently: 24.80%, 69.57% and
    48.69%.
  - The helpers live in [`steering_maths.h`](../../src/game/steering_maths.h),
    which only `steering.c` includes, after its `.rdata`. They have no
    address of their own, so they cannot carry a Reccmp annotation.
  - The annotation lint scans `.c`/`.cpp` definitions. In a header they add
    no lint debt; the original's helpers were in a header too.
- **Static callbacks.** With the six callbacks `static`, MSVC emits each one
  after its first caller (`steering_item_wander` after
  `scene_object_list_update_transforms`). The original has them in source
  order, so they are external functions. Dropping `static` changed no score
  and put all 30 functions in original link order.
- **`0040f890`**: an `if`/`else if`/`else` on the direction's height against
  the `.data` threshold.
- **`0040fd00`**: the declaration order `elapsed`, `time`, `before` scores
  89.66%. The other five orders score 73.04–86.21%.
- **`0040fb90`**: the per-field heading copy (three float stores) is exact.
  A struct copy through a pointer is not (65%).

### Near misses

All five ran through `tools/msvc_permuter.py`: 0 applicable rewrites, as in
finding 519. Rule B (finding 538) was swept for each one: 0–520 inert
`extern int` declarations before the functions, step 4–16.

- **`0040fb90` steering_physics_step, 98.97%** (Reccmp: effective match).
  The listing is the same except that the copy of `*data` to the stack is
  one instruction earlier. Twelve source variants (declaration order,
  assignment position, sqrt form) and the sweep never reach 100%. The best
  phase is the current one.
- **`0040fd00` steering_item_wander, 89.66%.** Only the registers around the
  `vector_scale_components_in_place` call differ. The sweep gives 89.66 or
  86.21, never 100%.
- **`0040f6c0` scene_node_record_create, 82.93%.** The original stores each
  tuning parameter to its base and current fields through one register.
  - With `float` parameters, MSVC 5.0 copies the value into a second
    register for the second store. That holds for every form tried: separate
    stores, chained `=` both ways, and re-reading the stored field.
  - With `int`-typed parameters the single-register shape appears, so the
    float typing is the cause. Integer parameters are not plausible for
    these values.
  - The sweep does not change the code.
- **`0040f920` kind_two_steering_torque_apply, 81.48%.** The first normalise
  schedules `heading.x` later than the original, and the original keeps the
  dot product in a stack slot.
  - Rule B moves it between 72.78% and 86.62% (period 16–64).
  - Of seven variants, computing the turn from the dot before the in-place
    cross product scored best.
- **`004100d0` steering_item_follow_target, 53.99%.** The original keeps the
  old target's y and z in `ebx`/`ebp` across the call and `dir.x` in an x87
  register. The sweep does not change the code. Eighteen
  declaration-order and else-branch variants peaked at 53.99%.
- **`0040fed0` steering_item_spread_tuning, 36.62%.** The original computes
  `base + base * (1/spread * (2r − 1))` with `1/spread * factor` evaluated
  once per field, and keeps the `+ base`.
  - In every form tried MSVC 5.0 folds `x*b + b` into `(x − (−1)) * b`:
    twelve expression shapes, an inline helper, and inline `2r − 1`.
  - That loses the `fadd`s; the best variant scored 47.06% without the
    original's slot reuse.
  - The sweep does not change the code. How the original avoided the fold
    is open.

### Rule B note for `0040fcc0`

`vector_scale_components_in_place` is finding 538's example of a rule-B
tie. The unit's own declarations put it in the original's phase (100%), but
only about five declarations from the edge. Adding six or more declarations
before it in `steering.c` flips it to 91.30%. Any later edit above line
~250 of the unit should re-check it.

## Callers and tests

No port code calls the unit:

- the GAME-023 callers are ledger stubs or not yet in the port;
- the hook that `scene_object_list_*` dispatched through was never
  installed.

`record.cpp`'s `optional_resource_destroy` call keeps working through the
`extern "C"` declaration.

- The new [`test_steering.cpp`](../../src/game/test_steering.cpp)
  (`steering`) tests these:
  - the layout;
  - creation and group flags;
  - the stack and walks;
  - the pitch clamp and damping;
  - the cross-product turn;
  - one physics step;
  - follow and arrive;
  - the spread bounds;
  - the leader-centred wander.
- The unit's checks leave `test_view.cpp`.
- They also leave `test_record.cpp` and `test_ptrstack.cpp`, which no build
  compiles. `ptrstack.hpp` now declares only `callback_record_allocate_initialize`.

## Entropy

The first full build deleted every superseded declaration. That dropped
seven functions outside the unit; four of them gained 1–2 instructions:

- `0x004089d0`, `0x0044a070`, `0x0044fb90`, `0x0045e940`;
- `0x00482620`, `0x00483420`, `0x004882e0`.

Under rule 1 the declarations are restored verbatim, uncalled, with a
comment, in `view.hpp`, `record.hpp`, `util.hpp`, `entity.hpp` and
`view_shared.hpp`, as finding 519 did. `util.hpp`'s copy of
`callback_operation_run_and_return_status` sits inside an `extern "C"` block
and would clash with the C unit's. It is renamed
`callback_operation_run_superseded`; finding 538 measured that names cost
nothing.

After that, `make reccmp-check` against `8e8d4da3` reports:

- **no function below baseline**;
- **no score change outside the unit**;
- **no instruction-count change.**

Three listing digests outside the unit change at equal score and equal
instruction count:

| Address | Function | File | Score | Instructions | Change |
| --- | --- | --- | --- | --- | --- |
| `0x00404f50` | `object_filter_matches_action_target` | `entity.cpp` | 25.00 = | 84 = | two argument pushes trade registers (rule B) |
| `0x00438770` | `building_upgrade_resource_admission_check` | `entity.cpp` | 13.95 = | 116 = | register choice (rule B) |
| `0x004078c0` | `ledger_stub_004078c0_…` | `bubbles.cpp` | 10.26 = | 42 = | full-link listing only; the single-unit compile is identical to `main` |

The two `entity.cpp` changes come from deleting the three old steering
definitions that preceded them in that file. The conversion must delete
them. Their source is unchanged. **For the owner:** none is a drop, and none
changes the instruction count.

## Tooling notes

- **`convert_unit.py` refuses a relocated unit.** `link_order.insert_unit`
  raises because the unit is already listed under its `.cpp`. The scaffold
  ran after the `.cpp` entry was removed by hand. The tool then re-inserted
  `game/steering.c` at the same position.
- **Rule-B sweeps need the pads on a code line.** Tokens after an
  `#include` on the same line are ignored, which silently gives a flat
  sweep. `match_function.py --candidate` maps addresses by line number, so
  the pads must not add lines.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox) passes against
  `8e8d4da3`: 17 improved, 11 newly mapped, none below baseline, link-order
  map OK.
- `make match-progress` regenerated the baseline, the listing digests and
  `binary-progress.json`. `pr_match_progress.py check --base origin/main
  --head .` reports them fresh.
- `make unit-data-offsets` gives the GAME-016 row above.
- `python3 tools/translation_units.py build` reports 0 conflicts.
  `tools/binary_progress_inputs.txt` was regenerated.
- `make tools-test` passes, including Meson parity and the reccmp-lint
  ratchet. The ratchet baseline loses the 10 port definitions this
  conversion removes and gains nothing.
- `steering.c` passes `gcc -std=gnu89 -Werror=declaration-after-statement
  -fsyntax-only`. The only warnings are the unused callback parameters, as
  in the original.
- `cmake -S . -B build && cmake --build build && ctest --test-dir build
  --output-on-failure`: 36/36 pass, including `c_boundaries` and the new
  `steering`.

## Reproducing

    python3 tools/convert_unit.py GAME-016 --stem steering --dry-run   # after removing the .cpp entry
    make match-function SOURCE=src/game/steering.c
    make reccmp-check
    make match-progress
    make unit-data-offsets
