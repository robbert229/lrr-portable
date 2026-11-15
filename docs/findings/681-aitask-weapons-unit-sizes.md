# Finding 681 — AITask.c and Weapons.c toward their original sizes

Issue #818, part of #388 and ADR 0005. It works the two unit-size gaps
that finding 674 §4 recorded: AITask.c (GAME-002) and Weapons.c
(GAME-050).

**Result: Weapons.c goes from 32 to 16 bytes long.** `00471370`
weapons_callback_ray_candidate now has the original's size (0x2be) and
scores 99.55. `0046f3b0` weapons_projectile_hit_check rises from 87.75 to
88.98. Neither reaches exact, so exact-function code bytes do not change.
AITask.c is unchanged. On `main` it was already in place at both ends; the
"64 bytes short" figure in the issue comes from finding 674 §4 and predates
finding 675. Its two in-padding size misses, `00403810` (+4) and
`00405c10` (+8), were worked and stay open (below).

| Measure (`binary-progress.json`) | `main` `160f5efc` | This change |
| --- | ---: | ---: |
| Exact-function code bytes | 344,002 (53.54%) | 344,002 (53.54%) |
| Accuracy, full / game-only | 97.56% / 97.30% | 97.56% / **97.31%** |
| Functions that rose / fell | | **2 / 0** |
| Correct-size functions, full | 2,796 (460,177 B) | **2,797 (460,879 B)** |
| Correct-size functions, game-only | 2,505 (409,350 B) | **2,506 (410,052 B)** |
| Identical prefix, raw / masked | 11 B / 7,187 B (to `00402c13`) | 11 B / 7,187 B |
| Units with size agreement | 2 (GAME-001, GAME-008) | 2 |
| Units with all sizes correct / start / end in place | 168 / 5 / 4 | 168 / 5 / 4 |
| `.text` identical at original addresses | 43,833 B (6.82%) | 43,593 B (6.78%) |
| … of which in runs of 16+ bytes | 16,882 B | **16,899 B** |
| `.data` identical | 24.31% | 24.31% |
| Fully identical functions at original address | 42 (2,307 B) | 42 (2,307 B) |
| Entry point (original `0048f000`) | `0048eb80` | `0048eb70` |
| SizeOfImage (original 3,608,576) | 4,026,368 | 4,026,368 |
| Function-order inversions, full / game-only | 222 / 1 pairs | 222 / 1 pairs |
| Data-offset agreement | 20 of 57 units | 20 of 57 units |

**An identity cost, stated plainly.** Weapons.c moves 16 bytes closer to
its original span. Everything linked after it then sits 16 bytes earlier,
and the rebuild was already short there. So the entry point moves from
1,152 to 1,168 bytes early, and `.text` identical at original addresses
falls by 240 bytes. Those 240 bytes were chance matches: the part of that
measure in runs of 16 bytes or more rises by 17. The identical prefix does
not move.

`make reccmp-check` passes with no function lower. `make match-progress`,
`make unit-data-offsets` and `make pe-audit` were run and the generated
files committed; `unit-data-offsets` and `pe-audit` changed no tracked
file. `make tools-test` (no `--allow-growth`), `make winapi-link-check`,
`make reccmp-lint` and native ctest (37 of 37) pass. The native build
starts and runs with the shipped WADs.

## Method

Per-function sizes are `identity_metrics.code_extent` on both images, the
rule `binary-progress.json` uses, read from the full matching build. The
unit-span sums use whole 16-byte slots, because a function only moves its
neighbours when it crosses one. Variants were scored with
`tools/match_function.py` at the real source path, and none ran during a
matching build. `make phase-probe` (N = 0–63) was run on `0046f3b0`,
`0046fc70`, `0046ffb0` and `00471a60`. All four are single-code, so their
code comes from the source, not the symbol counter's phase.

## Weapons.c: where the 32 bytes were

| Address | Function | Original | Rebuild (`main`) | Slot change |
| --- | --- | ---: | ---: | ---: |
| `0046f3b0` | weapons_projectile_hit_check | 416 | 417 | +16 |
| `0046fc70` | weapons_projectile_create_curved | 640 | 616 | −16 |
| `0046ffb0` | weapons_fire_lazer | 676 | 668 | −16 |
| `00470760` | weapons_mesh_build_segment | 2,911 | 2,946 | +48 |
| `00471370` | weapons_callback_ray_candidate | 702 | 704 | +16 |
| `00471a60` | weapons_model_aim | 521 | 505 | −16 |

Total: +32. `weapons_initialise` (+3), `weapons_projectile_update` (+1)
and `weapons_object_quad_intersect` (+7) stay inside their padding.
`00471370` is a static callback that the translation-unit function list
misses, so the per-unit size check does not cover it, but it shifts every
function after it.

### `00471370`: the ray as a pair, and its modulus

The original's frame keeps the ray from the origin to the projected point
in two adjacent slots (`[esp+0x18]`, `[esp+0x1c]`). It stores the ray
length's divisor `dirLen` in the dead `obj` parameter slot. The rebuild
had `rx` and `ry` as two scalars. MSVC put `ry` in the parameter slot and
`dirLen` at `[esp+0x10]`, so every local moved. With `float r[2]` the frame
matches (82.59 → 95.09, single-function). The other declaration positions
give the same code.

The original also squares each component straight after storing it
(`fst [r0]; fmul [r0]; fxch; fst [r1]; fmul [r1]`). The direct
`sqrt(r[0] * r[0] + r[1] * r[1])` stores both first and adds one `fxch`,
which is the 2 bytes. gods98's `Maths_Vector2DModulus` expanded inline
gives the original's order and size (97.75 single-function; 99.55 in the
full build). It goes into `weapons_maths.h` as `weapons_vector2d_modulus`,
over a `float` pair, because the unit keeps its 2D points as pairs. Every
form tried scores the same: subtract-then-modulus, a dot-product modulus,
and the nested call. All 47 annotated functions in `weapons.c` score as
before except the two in this finding.

### `0046f3b0`: the range test kept on the stack

The original compares the range with `fld st(0); fcomp [0.0f]` and keeps
the range on the x87 stack for the distance test. The rebuild's
`range = f(); if (range != 0.0f && ...)` gives `fcom`, 2 bytes shorter.
Assigning in the test, `if ((range = f()) != 0.0f && ...)`, gives the
original's pair (87.75 → 88.98). The function is now 0x1a3 bytes in the
0x1a0 slot, so its +16 stands.

The 3 bytes left are the final call. The original loads `weaponType` into
`edi` and `&projectile->direction` into `esi` before the end-point
arithmetic. It then reads `knownWeapon` as `[esi+0x1c]`, 3 bytes shorter
than the rebuild's `[esi+0x2ac]`, which MSVC loads first. The end point's
`x` product is also swapped (`fld [v]; fmul [t]` against `fld [t]; fmul
[v]`) while `y` and `z` agree. Forms tried, none closer:

- `knownWeapon`, `weaponType`, the direction pointer or the hit object in
  a block local, alone and together;
- the end point computed into `hitPos` as a statement before the call, also
  with the locals above;
- the end point as an explicit scale and add (77.17, 0x1b3);
- `hitPos` at function scope (74.02);
- `t` in a local;
- `&projectile->direction.x`;
- the projectile parameter copied to a local;
- all six orders of the three function-scope declarations, with and
  without `hitPos` among them;
- `double range`;
- nested `if`s and an early return for the hit object;
- the operands of the range test reversed (80.62, 0x1b1).

### Not changed

- **`00471a60` weapons_model_aim (−16) and `0046f920`
  weapons_projectile_update.** The original passes each float argument to
  Container_SetOrientation as `push ecx; fld; fstp [esp]`, and the rebuild
  copies them through integer registers. This is the pattern that findings
  589, 615, 655 and 677 could not reproduce. model_aim also recomputes
  `&fireDirs[slot].y` at each use and keeps `xPivot` in `ebp`, where the
  rebuild keeps the address in `ebp` and spills `xPivot`. Not retried.
- **`00470760` weapons_mesh_build_segment (+48), `0046fc70` (−16) and
  `0046ffb0` (−16).** Finding 677 worked these and found nothing
  committable. Single-code under the phase probe.

Fixing only the two short functions would bring the span within 16 bytes
of agreement at best, by size-cancelling errors (finding 669). It is not
attempted.

## AITask.c

GAME-002 starts at `00401cb0` and its last function ends at the
original's end on `main`, so the span already agrees. It does not count as
agreeing because two functions are the wrong size inside their padding.

### `00403810` (+4): register roles

The original keeps `blockPos` in `ebp` and `b` in `edi`. The second
query's result overwrites `edi`, so the count block reloads `b`
(`mov edi, [esp+0x1c]`) on that path only. The create-success branch is
`jne` to the final epilogue, then `jmp` to the count. The rebuild has `b`
in `ebp` and `blockPos` in `edi`. It reloads `blockPos` for the third call
instead and gives the create-success branch its own epilogue. Forms tried
here, on top of finding 675's list:

- the count statement repeated in the first branch (MSVC cross-jumps it,
  but puts the merged block after the first branch: 36.36, 0xab);
- the count in an `else` with the label inside it, with and without the
  loop's `return`;
- `goto done` forms that mirror the original's block order (61.04, 0xab,
  1 byte short);
- a second object local for the second query;
- `a` instead of `7` in the second branch;
- `b` tested as a bare condition, or after `a == 7` (73.17).

No form gave the original's register roles. The phase probe is
single-code (finding 675).

### `00405c10` (+8): not restructured

The original loads `0x100` into `ebx` inside the candidate loop, after the
null test (`mov eax, [esi+0x5c]; mov ebx, 0x100; test ebx, eax`). It then
reuses `ebx` for `ok`, which is never spilled. MSVC hoists the constant to
the loop head, which costs a `jmp` and a second load, and spills `ok` to
`[esp+0x78]`. The original's frame is 4 bytes larger (`0x94`), and
`best`'s register differs, as finding 675 recorded. That is register
allocation across a 1.9 KB function. No form was found that moves it
without a restructure, and none was committed.

## Reproduce

```sh
make match-function ADDR=0x00471370
make match-function ADDR=0x0046f3b0
make phase-probe ADDR="0x0046f3b0 0x00471a60 0x0046ffb0 0x0046fc70" PROBE_ARGS="--range 0 64 1"
make reccmp-check && make match-progress
```

## Files

- `src/game/weapons.c`: `00471370`'s ray as `float r[2]` with the inline
  modulus; `0046f3b0`'s range assigned in the test.
- `src/game/weapons_maths.h`: `weapons_vector2d_modulus`
  (Maths_Vector2DModulus).
- `docs/coverage/`: regenerated.

Besides this finding, its README entry and the regenerated coverage files,
no file owned by #815, #816, #817 or #819 is touched. No `#pragma
optimize` is committed, and no pad or filler was added.
