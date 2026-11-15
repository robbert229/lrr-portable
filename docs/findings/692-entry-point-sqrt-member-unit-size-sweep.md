# 692. The entry point: LIBC's sqrt.obj, from the camera unit's inline copies; the unit-size sweep; an empty port tail

Issue #841. Follows findings 658, 667 and 683. Measured with full matching
builds under the gate's link mode on `main` `77022c16` and on this change.

## Summary

1. **The LIBC members before the entry point were the original's, except
   one: `sqrt.obj`** (`__CIsqrt` and `_sqrt`, 192 bytes,
   `0048e5c0`-`0048e680` in the original). The tail did not pull any member
   before the entry. The original links `sqrt.obj` between `crt0dat.obj`
   and `fabs.obj`, yet nothing in its image calls it or holds its address.
2. **The reference came from the camera unit's copies of Maths.h's inline
   functions.** GAME-022 (`camera_control.c`) was compiled without
   optimisation, and its code calls the vector functions out of line
   (finding 587). Without optimisation MSVC 5.0 expands no inline function:
   it emits a copy in the object, as a pick-any COMDAT. LINK keeps the first
   copy, Advisor.c's (`004013a0`-`00401750`), and discards the rest. The
   camera's copies call `sqrt` as a function, so the object references
   `_sqrt` ahead of `camera_update`'s `_fabs`, after Lego.c's `_exit`. That
   is `sqrt.obj`'s place.
3. **The matching build now does the same.** `camera_maths.h` gives the
   camera unit the seven functions as `extern __inline`. Advisor.c defines
   them as `extern __inline` too, so both are pick-any COMDATs. The camera
   unit takes `/Ob0` after `/Ob1`, as plain `/Od` does. Advisor.c's
   functions keep their code and addresses, the camera's code is unchanged,
   and `sqrt.obj` is linked where the original has it.
4. **The entry point moves 192 bytes toward the original**: `0048ea60` to
   `0048eb20` (original `0048f000`). The 1,248 bytes left are:
   - the unit segment, 1,296 bytes short (unit functions shorter than the
     original's);
   - less 48 bytes of LIBC that the original folds and the gate keeps:
     `__fpclear` (16) and `_CallMemberFunction1`/`2` (32). These are
     identical-COMDAT folding, which the gate's `/OPT:NOICF` keeps apart
     (findings 642 and 650).
5. **The unit-size sweep raised one function**:
   `interface_callback_hotkey` (`0041c470`) goes from 91.14% to 100%, with
   the correct size. Sixteen other near-misses did not move; section 3
   lists what was tried.
6. **`tools/port_tail_inventory.py` reports an empty tail.** Before, it
   stopped with `max() iterable argument is empty`.

## Before and after

| Measure (`binary-progress.json`) | `main` `77022c16` | This change |
| --- | ---: | ---: |
| **Exact-function code bytes** | **351,876 (54.76%)** | **351,876 (54.76%)** |
| Exact functions, full / game-only | 2,651 / 2,362 | 2,652 / 2,363 |
| Accuracy, full / game-only | 97.78% / 97.55% | 97.78% / 97.55% |
| `.text` identical at original addresses | 6.93% (44,553 B) | 6.96% (44,733 B) |
| `.data` / `.rdata` identical | 28.48% / 9.56% | 28.56% / 9.62% |
| Identical prefix, raw / masked (original-mode link) | 11 B / 7,187 B | same |
| Correct-size functions, full / game-only | 2,811 / 2,519 | 2,812 / 2,520 |
| Units with size agreement (sizes, start, end) | 2 of 227 (171, 5, 4) | same |
| Entry point (original `0048f000`) | `0048ea60` | **`0048eb20`** |
| SizeOfImage (original 3,608,576) | 3,596,288 | same |
| Image end (original `0x771000`) | `0x76e000` | same |
| Data-offset agreement | 22 of 57 units | same |
| LIBC `.text` (original-mode relink) | 65,536 B, 417 functions | 65,728 B, 419 functions |
| Tail roots | 0 | 0 |

`interface_callback_hotkey` is exact now, but exact-function code bytes do
not change. That measure counts the functions in the listed original
function table (`translation_units.load_functions`), and this callback is
not one of them. The exact-function count and the correct-size count do
include it.

No function falls. No identity measure moves away from the original.

### data_identity

| Region | Original start | Rebuilt start (before → after) | Items at original offset | Paired items | First item not at its offset |
| --- | --- | --- | ---: | ---: | --- |
| `.rdata` | `0049e000` | `0049e000` (in place) | 0 | 95 | `0049e014`, −4 (unchanged) |
| `.data` | `004a1000` | `004a0000` (4,096 early) | 192 (3,401 B) | 2,454 | `0x4a1d5c` `rock_fall_styles.c`, +132 (unchanged) |
| `.bss` | `004b2db0` | `004b1d30` → `004b1d40` (4,224 → 4,208 early) | 1 (1,040 B) | 149 | `004b31c8` `aiTaskGlobs`, −8 (unchanged) |
| commons | `00549520` | `00546c00` → `00546c20` (10,528 → 10,496 early) | 16 (59,256 B) | 61 | `0x556fb8`, −3,848 (unchanged) |

`sqrt.obj` brings 16 bytes of `.data` with it (`.data` 73,008 → 73,024
bytes, original 73,136), so `.bss` and the commons start 16 and 32 bytes
later, both toward the original.

### On the stack (#845, `data/839-data-commons` `1b68e19c`)

Rebased onto #842 to #845 and measured again with full builds. These are
the stack's final figures. No function falls; `0041c470` rises.

| Measure | #845 `1b68e19c` | This change |
| --- | ---: | ---: |
| **Exact-function code bytes** | **351,876 (54.76%)** | **351,876 (54.76%)** |
| Exact functions, full / game-only | 2,651 / 2,362 | 2,652 / 2,363 |
| Accuracy, full / game-only | 97.79% / 97.55% | same |
| `.text` identical at original addresses | 6.94% (44,616 B) | 6.96% (44,727 B) |
| `.data` / `.rdata` identical | 28.70% / 23.00% | 28.78% / 23.03% |
| Identical prefix, raw / masked (original-mode link and gate) | 11 B / 7,187 B (to `0x402c13`) | same |
| Correct-size functions, full / game-only | 2,811 / 2,519 | 2,812 / 2,520 |
| Units with size agreement (sizes, start, end) | 2 (171, 5, 4) | same |
| Entry point (original `0048f000`) | `0048ea50` | **`0048eb10`** |
| SizeOfImage (original 3,608,576) | 3,600,384 | same |
| Data-offset agreement | 22 of 57 | same |

| Region | Rebuilt start (#845 → this) | At original offset | At original address | Prefix | First item not at its offset |
| --- | --- | ---: | ---: | ---: | --- |
| `.rdata` | `0049e000` (in place) | 8 (32 B) | 8 (32 B) | 52 B | `0049e048`, −20 |
| `.data` | `004a1000` (in place) | 196 (3,464 B) | 196 (3,464 B) | 12 B | `0x4a1d5c` `rock_fall_styles.c`, +132 |
| `.bss` | `004b2d30` → `004b2d40` (128 → 112 early) | 1 (1,040 B) | 0 | — | `004b31c8` `aiTaskGlobs`, −8 |
| commons | `00547c00` → `00547c20` (6,432 → 6,400 early) | 17 (179,132 B) | 0 | — | `0x556fb8`, −3,848 |

## 1. The LIBC members before the entry point

The members were compared with finding 658's method. Each LIBC member's
span in the map was set beside the original's CRT unit ranges, in order:

| Member | Original | Rebuilt (before) | Difference |
| --- | ---: | ---: | --- |
| `fpinit.obj` | 96 | 112 | `__fpclear`, folded in the original |
| `crt0dat.obj` + `sqrt.obj` | 320 + 192 | 320 + 0 | **`sqrt.obj` missing** |
| `trnsctrl.obj` | 864 | 896 | `_CallMemberFunction1`/`2`, folded in the original |
| All other members up to `exsup.obj` | | | same sizes |
| **LIBC before the entry** | **5,856** | **5,712** | **−144** |

The same members are linked in both, apart from `sqrt.obj`. Some come in a
different order: `acos`, `87fmod`, `rand` and `stricmp` are pulled by
different units' references. That changes no size before the entry. The
original's CRT-011 is `acos.obj` (208 bytes), and its CRT-013 (54 bytes,
after `rand`) is probably `87fmod.obj`. These are ADR 0006 member-order
follow-ups.

`fflush.obj` and `__allmul` were explained the same way in finding 667:
references from functions that `/OPT:REF` later removed. Here the removed
code is not a function `/OPT:REF` drops. It is a duplicate COMDAT that LINK
discards when it resolves symbols. Finding 667's scratch compiles showed
that MSVC 5.0 drops the extern of a `static` or unused `__inline` function.
`extern __inline` is different: it emits the copy whether or not anything
calls it.

| Probe (MSVC 5.0) | Externals |
| --- | --- |
| `(float)sqrt(x)`, `/O2 /Ob1` | none (`fsqrt` inline) |
| the same, `/Od` | `_sqrt` |
| the same, `/O2 /Ob1 /Op` or `/Oi-` | `_sqrt` |
| `__inline` caller, `/O2 /Ob1 /Od` (the gate's camera flags) | expanded, `_sqrt` |
| `extern __inline` copy, `/O2 /Ob1` | pick-any COMDAT, emitted although unused |

No `/O2` unit can produce the reference. `camera_control.c` is the only
unoptimised unit, and it is the first unit that references `_fabs`. So the
reference comes from the camera unit.

### Why `/Ob0`

The gate compiled the camera unit with `/O2 /Ob1 /Od`. `/Ob1` survives the
`/Od`, so the first build of this change expanded the copies inside
`camera_update`. The camera then called its own code, and Advisor.c's copies
lost their last callers. The original's camera calls `004013a0`, so it was
compiled with plain `/Od`, which implies `/Ob0`. The camera unit now takes
`/Od /Ob0`. Its code is unchanged, because nothing was inlined before. The
new `D4025 : overriding '/Ob1' with '/Ob0'` warning is allowed by its exact
text in `tools/reccmp_diagnostics.py`, with a test.

### Why Advisor.c changes

With `/Ob0`, the camera's copies are pick-any COMDATs. Advisor.c's plain
definitions under `/Gy` are no-duplicates COMDATs, and LINK rejected the
pair (`LNK2005`, `LNK1169`). In the original, every object that included
Maths.h emitted pick-any copies, and LINK kept the first object's, which
is Advisor.c's. Advisor.c's seven definitions are now `MATHS_INLINE`, which
is `extern __inline` under MSVC 5.0 and empty elsewhere. The code, the
addresses and the scores do not change.

## 2. `port_tail_inventory.py` with no tail

`second_pass_libc()` returns the LIBC symbols after the last tail run, or
nothing when there is no tail run. The result has `tail_linked`, and the
text output says so:

```
entry point 0x48eb20 vs 0x48f000 (-1248): unit segment end -1296, LIBC before the entry +48
...
no port tail in the link: no tail object, tail code or second-pass LIBC member
tail code kept (COFF /OPT:REF model): 0 of 675,216 compiled
LIBC after the tail: 0 functions, 0 bytes
```

Three tests were added (`tools/tests/test_port_tail_inventory.py`): the
second pass with and without a tail run, and the reachability model with no
tail objects.

## 3. The unit-size sweep

Outside Lego.c, AITask.c and the CRT, 175 mapped functions have the wrong
size. The unit segment is 1,296 bytes short overall. The biggest deficits
are Interface.c (−179), ObjInfo.c (−165), FrontEnd.c (−148), GAME-026 (−103)
and GAME-037 (−83).

**Fixed.** `interface_callback_hotkey` tested a newly pressed key with
`cur != prev`. The original's form, which FrontEnd.c, Reward.c and
TutorialScript already use, is `cur && (prev ^ cur)`. It compiles to the
original's `xor al, cl` and gives 100%, 2 bytes longer. No other unit uses
the `!=` form.

**Tried, no change.**
- `tools/msvc_permuter.py`, 55 s each (90 s for the first), found nothing for the 17
  highest-scoring wrong-size functions: `0047a1e0`, `004157c0`, `00462ad0`,
  `00419530`, `0043b8d0`, `004638a0`, `0041baa0`, `00465430`, `00417460`,
  `0045c0d0`, `00477150`, `004089d0`, `00445770`, `0041c470`, `0044a200`,
  `00411360` and `0043c2f0` (`0041c470` was then fixed by hand).
- **The loop-head reload.** `font_v_print_f2` (`0047a1e0`, −2) and
  `info_compile_text` (`00419530`, −2) have the same difference. The
  original reloads the loop's character at the top of a rotated `while
  (*p != '\0')` loop, although the loop's bottom has just tested it.
  Ten source forms of `info_compile_text` did not reproduce it: `while
  (*text)`, two `for` forms, `text[0]` in the test or the body, casts and
  `!= 0`.
- **`legoobject_type_component_get`** (`0044a200`, −8): the original
  reloads the parameter for the final `return obj`. Three forms of the last
  `if` (an `else`, a ternary, the test inverted) did not change it.
- `front_play_intro_splash`, `reward_scroll_update`,
  `legoobject_behavior_callback_a` and `ddiscene_draw_end` differ only in
  x87 register order (`fxch`, `fstp`/`fmulp`). These are finding 655's
  hardest category.

`legoobject_nearest_cell_route_commit` (`00446e40`, +157) is exact. The
size difference is the extent measure's: in the original, about 160 bytes
of code that no function starts claim follow the function.

## Files outside the issue's own

- `src/game/advisor.c` (#838): `MATHS_INLINE` on seven definitions and the
  macro with its comment. These are function definitions, not constants.
- `src/game/camera_control.c` and the new `src/game/camera_maths.h`: the
  inline copies, for the matching build only.
- `src/game/interface_unit.c`: one condition, a function body.
- `CMakeLists.txt`: `/Ob0` for `LRR_UNOPTIMISED_UNITS`.

## Checks

- `make reccmp-check`: no function below the baseline, one up. Winapi link
  check and link-order map check OK.
- `make match-progress`: the baseline, the listing digests and
  `binary-progress.json` are regenerated.
- `make unit-data-offsets` and `make pe-audit` (stdout only): the thunk
  order and the DLL order are unchanged, and the image end is `0x76e000`.
- `make tools-test` (no `--allow-growth`) passes, `reccmp-lint` included.
  `make winapi-link-check` passes.
- Native: the build compiles, its 37 ctest tests pass, and `LegoRR` runs
  for 20 s with the offscreen driver.

## Reproduce

```sh
make reccmp-check && make match-progress && make unit-data-offsets && make pe-audit
python3 tools/port_tail_inventory.py --json /tmp/tail.json
grep -E 'sqrt|_WinMainCRTStartup' build-reccmp/LegoRR.map
```
