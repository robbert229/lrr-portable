# Finding 663 — What drives MSVC 5.0's register choice in the cause-C near misses

Issue #776, part of #388 and ADR 0005. The near-miss sweeps (findings 644,
645, 648, 649 and 650) left integer register choice (cause C) as "not
found", and finding 654's order searches did not move it. This finding tests
three families of explanation on 32 near misses from units no other thread
owns this wave: per-unit compiler options, the symbol counter (finding 538
rule B), and source-order rules beyond finding 654's modes.

**Result: no change can be applied, but cause C splits cleanly in two.**

- **Per-unit options: negative.** None of 15 option sets raises any function
  in any of 33 units without lowering others in the same unit. The original
  was not built with different options per file.
- **About one function in six is a symbol-counter phase case.** In 5 of 32,
  a counter shift alone gives the original's registers: 4 exact, 1 from
  96.73 to 99.18. The shift needed differs from function to function within
  one unit. Missing declarations therefore lie between functions, not only in
  the headers. Pads are the only way to reach those shifts today, and entropy
  rule 3 forbids them. These functions recover when their unit's real
  declarations do.
- **The rest have their register choice fixed by the source.** 24 of 32 keep
  one code over 256 counter shifts. For two of them, a total of 30 source
  variants change nothing: declaration order, `register`, block-scope loop
  variables, the function's own local count, loop shapes and an inline
  helper. Only the expression and statement shape can move them.

One new lead: `#pragma optimize("a", on)` (assume no aliasing) around AITask.c's
`0x00404100` alone gives the original's registers exactly (58.82 to 96.08 in
the single-function loop). It also moves two stores out of the original's
order. So the original was not built that way, but its allocation is the
no-alias one. Not applied (see "Leads").

| Measure | `main` `bf6335e3` | This change |
| --- | ---: | ---: |
| Exact-function code bytes | 335,096 (52.15%) | 335,096 (52.15%) |
| Accuracy, full / game-only | 97.02% / 96.68% | 97.02% / 96.68% |
| Exact / mapped, full | 2,408 / 2,833 | 2,408 / 2,833 |
| Functions that rose / fell | | 0 / 0 |
| Image end (gate) | `0x7f5000` | `0x7f5000` |

No source or build input changes. `make reccmp-check` passes with no function
lower. `make match-progress`, `make unit-data-offsets` and `make pe-audit`
were run. The baseline and listing digests are unchanged.
`binary-progress.json` changes in one place: one 32-byte function moves from
`original_annotated` back to the `opt_ref` `port_unit` category (2,534 to
2,533). That is the `/OPT:NOREF` relink flip finding 654 recorded in the
other direction, with no source change. `make tools-test`, `make
winapi-link-check` and native ctest pass, and the native build starts.

## Method

All scores come from `tools/match_function.py` with the matching build's
flags (`/O2 /Ob1`, MSVC 5.0 RTM). These single-function scores sit below the
full report where a callee's name only resolves at link time. Only deltas
within one compile are compared. The scripts are in
[`tools/experiments/776/`](../../tools/experiments/776/README.md).
`match_function.py` gained `--cflags` (extra cl options after the configured
ones). `--candidate` now accepts several addresses of one unit.

### 1. Which near misses are cause C

`classify.py` compared the original and recompiled instructions of all 192
functions at 90–99.99% in the full baseline:

| Class | Functions | What it means |
| --- | ---: | --- |
| `NAME` | 21 | only symbol names differ in the unlinked compile; not cause C |
| `REG` | 14 | identical once register names are normalised |
| `REGOP` | 35 | identical once registers, address-term order and instruction order are normalised (register choice or scheduling) |
| `OTHER` | 113 | anything else |
| not compiled / unannotated | 9 | CRT objects |

The sample is every `REG`/`REGOP` function outside the files owned by
#772–#775, plus aitask.c's `0x00404100`. That function is a pure register
rotation that `classify.py` puts in `OTHER`, because its pushes move too.

### 2. Per-unit options

`flags.py` scored all 1,027 functions in the 33 sampled units under each
option set, appended after `/O2 /Ob1`.

| Options | Rose | Fell | Note |
| --- | ---: | ---: | --- |
| `/Ox` | 0 | 0 | identical code (`/O2` adds only `/Gf /Gy`) |
| `/G5` | 0 | 0 | identical (the default) |
| `/GB` | 0 | 0 | identical |
| `/Ob0` | 0 | 31 | |
| `/Oi-` | 6 | 146 | `0044fe70` → 100, but 11 others in `terrain_map.c` fall |
| `/Op` | 4 | 156 | `0047ad70` 96.73 → 99.18, 3 others in `sound3d.c` fall |
| `/Ow` | 4 | 162 | |
| `/Oa` | 4 | 247 | `00404100` 58.82 → 96.08; 32 others in `aitask.c` fall, 20 of them exact |
| `/Ob2` | 4 | 365 | |
| `/G6` | 6 | 732 | |
| `/Oa /G6`, `/Ow /G6` | 4, 6 | 748, 743 | |
| `/Os` | 1 | 921 | |
| `/Oy-` | 1 | 935 | |
| `/Og-` | 0 | 1,023 | |

No set gives a unit a rise without a fall. The few rises are single
functions, each against several exact functions lost in the same unit. The
fingerprints say the same. With an omitted frame pointer, `/Ob1` inlining and
the default `/G5` scheduling, every sampled unit matches the configured
build. Only `camera_control.c` differs (`/Od`, finding 587). The hypothesis
is rejected, and finding 486's global sweep now holds for each unit too.

### 3. The symbol counter (rule B)

`phase_probe.py` adds N inert `extern`s after the unit's first `#include`
(N = 0–255, every value). It fingerprints the function's listing and scores
the first N of each distinct code.

| Address | Function | Unit | Now | Codes | Best phase | Required shift |
| --- | --- | --- | ---: | ---: | --- | --- |
| `00479a30` | ray_plane_intersection_parameter | maths.c | 93.62 | 8 | **100** | +3 (mod 16; also 19, 23) |
| `0044fab0` | terrain_map_world_normal_get | terrain_map.c | 91.78 | 2 | **100** | +52 (mod 128, one value in 128) |
| `0044fe70` | terrain_map_world_z_unmodified_get | terrain_map.c | 97.99 | 2 | **100** | +3 (mod 4) |
| `00476410` | Container_Frame_SearchCallback | container.c | 100* | 2 | **100** (true) | ≥ +165 within 0–255 |
| `0047ad70` | s_play_seam | sound3d.c | 96.73 | 2 | 99.18 | +255 |
| `0046e610` | water_update_flow_blocks | water.c | 100* | 15 | current | — |
| `00471630` | weapons_ray_block_sweep | weapons.c | 100* | (period 8) | current | 4 of every 8 shifts fall to 97.41 |
| `0040fb90` | steering_physics_step | steering.c | 100* | 2 | current | +63 falls to 95.79 |

`*` marks an effective match: the original's instructions up to a register
permutation, so the baseline score stays below 100.

These 24 keep **one code over all 256 shifts**:

`00401270` advisor_config_slots_load_target, `004039a0`, `00404100`,
`004085f0`, `00409a90`, `0040a5e0`, `0040de40`, `004181b0`
help_window_initialise_adapter, `0044f700`, `004526e0`, `0045e460`,
`004629d0`, `00463590`, `0046c6f0`, `0046e390`, `00477bf0`, `004780b0`,
`00478e60` block_open, `0047e300`, `0047f590`, `0047f980`, `004848d0`
Flic_BrunDepack, `0048d2c0` lwo_load, `0048d7c0` lwo_app_obj_load.

Several `REGOP` functions turned out to be scheduling, not registers. For
example, `00401270` differs only in where two `fstp`s go.

**The shift is local.** `pads_at.py` puts the pads at chosen lines of
`terrain_map.c`:

| Pads | Rises | Falls |
| --- | --- | --- |
| 52 after the includes | `0044fab0` → 100 | `0044e0c0`, `0044f5c0`, `00450700` |
| 52 there + 3 before `0044fe70` (line 901) | `0044fab0`, `0044fe70` → 100 | the same three |
| 52 there + 3 before `0044fb90` (line 821) | the same | the same three |
| 52 there + 1 or 2 before line 901 | `0044fab0` only | the same three |
| 52 there + 7 before line 901 | both | the three and `00451450` |

Pads before a function move only that function and later ones. A function's
phase is therefore the count of everything declared before it, including
earlier functions' parameters and locals (finding 538's cost table). No
single top-of-file shift fits both terrain functions: `0044fab0` needs 52
(mod 128), which is 0 (mod 4), and `0044fe70` needs 3 (mod 4). The
original's declarations thus differ from the port's both before
`0044fab0` and between `0044fab0` and `0044fe70`. The second gap holds only
`0044fab0` itself and `0044fb90` (57.35%, far from its original shape), so
`0044fb90`'s locals are the likeliest place. That is a lead, not evidence.

Over a whole unit (maths.c, N = 0–31), the shift that makes `00479a30`
exact (+3) lowers `00479c10` from 49.35 to 29.68. Shifts of 2, 6, 16 and 22
raise `00479a30` to 95.74 and lower nothing.

The `/Oi-` and `/Op` rises in section 2 hit only phase-sensitive functions
(`0044fe70`, `0046e610`, `0047ad70`). They are probably the same effect,
since the option changes what the function's code depends on, not the
original's options.

### 4. Source-rule hypotheses on fixed-code functions

The tests used `004181b0` (98.17). The original loads each loop's first
character through the pointer copy (`mov al, [ecx]`); the port loads it
through the global's value (`[eax]`). They also used `00404100` (58.82),
where kept/zero/next sit in `edi`/`ebx`/`esi` in the original and in
`esi`/`edi`/`ebx` here.

| Hypothesis | Variants | Result |
| --- | ---: | --- |
| First-use / declaration order of locals | 2 + 2 | no change (`004181b0`, `00404100`) |
| `register` on the loop pointer or the character | 2 | no change |
| Loop variable declared in the inner block | 1 + 1 | no change |
| The function's own local count (0–23 unused locals at the top) | 24 | no change (`004181b0`) |
| Loop form (`for`/`while`, `*s` vs a `c` copy, index form) | 6 + 3 | equal or lower (84.04–98.17; 58.82) |
| Inline helper expansion (`static __inline` replace loop) | 3 | equal or lower (98.17, 84.51, 84.51) |
| `#pragma optimize` per function, `00404100` | 6 | `a`, `w`, `ga`, `at`: effective match (96.08); `g`, `p`: no change |
| `make match-search` `decls`/`operands` (`004039a0`, `0045e460`) | 0 / 0, 23 / 101 | no change (`004039a0` has no candidates) |

Argument evaluation order is not separately testable: MSVC 5.0 always pushes
right to left, and finding 654's `operands` mode already covers commutative
operand order.

## The rule for future sweeps

1. **Classify first** (`classify.py`). Treat `NAME` rows as link-name
   artefacts, not cause C. For `REGOP` rows, check whether only scheduling
   differs.
2. **Probe the phase** (`phase_probe.py`; 256 shifts take 1–3 minutes for
   most units, ~10 for weapons.c and aitask.c). If the original's code
   appears at some shift, the function is rule B. Record the shift and
   period, and stop working on its source shape: no source rewrite inside
   the function will fix it. It converges when the declarations before it do,
   through header and local shapes backed by evidence (finding 538
   "Implications"). The shift points to a place. Compare it with the shifts
   of the functions before and after (`pads_at.py`) to find the gap where
   the count goes wrong. A gap of 1–3 between two functions points at a
   local or parameter in the function between them.
3. **If there is one code,** the choice comes from the source. Per-unit
   options, local declaration order, `register`, block placement and the
   function's own local count will not move it. Spend the budget on
   expression and statement shape (finding 654's modes, then rewrites).
4. **Try the aliasing diagnostic** on short pointer-chasing loops: wrap the
   function in `#pragma optimize("a", on)` for one compile. If that gives
   the original's registers, the original's code assumes fewer aliasing
   hazards than the port's statements make. Look for a source form with
   fewer stores through pointers between the uses. Do not commit the pragma.

## Leads (not applied)

- **`00404100` aitask_command_queue_sweep_relink.** `#pragma optimize("a",
  on)` around this one function raises it from 58.82 to 96.08% (single
  function, no other aitask.c function changes) with every register right.
  The two `task->next = head; head = task;` pairs come out in the other
  order, so the original was not compiled with `/Oa`. As a `Deviation
  (scoring only)` the pragma would satisfy entropy rule 4 (no storage
  changes), but it is not the original's code and would hide the real
  source difference. It is left for the owner to decide.
- **`0044fab0`/`0044fe70`** need +52 (mod 128) before `0044fab0` and a
  further 3 (mod 4) before `0044fe70`. `0044fb90`'s local set is the
  likeliest place for the second.
- **`00479a30`** needs +3 (mod 16) at maths.c's counter. Finding 646 shows
  the original's `d3drm.h` include moves it by far more, so the residue can
  only be judged once maths.c's headers are original-shaped.

## Unfinished

None of the searches hit its 20-minute cap. Phase probes covered N = 0–255
only. Functions with a period of 512 (finding 538's `Draw_Pixel24`) could
show a second code above 255. Those classed as single-code above were not
checked beyond 255.

## Files

- `tools/match_function.py`: `--cflags` / `MATCH_FN_EXTRA_CFLAGS`
  (`with_extra_flags`); `--candidate` with several addresses of one unit.
- `tools/tests/test_match_function_flags.py` (new, in `make tools-test`),
  `Makefile`, `tools/README.md`.
- `tools/experiments/776/` (new): `classify.py`, `flags.py`,
  `phase_probe.py`, `pads_at.py`, README.
- `docs/coverage/binary-progress.json`: regenerated.

No file owned by #772, #773, #774 or #775 is touched.

## Reproducing

```sh
python3 tools/experiments/776/flags.py src/game/aitask.c,src/gods98/maths.c
python3 tools/experiments/776/phase_probe.py src/gods98/maths.c 0x479a30:ray_plane_intersection_parameter
python3 tools/experiments/776/pads_at.py src/game/terrain_map.c '16:52' '16:52,901:3'
```
