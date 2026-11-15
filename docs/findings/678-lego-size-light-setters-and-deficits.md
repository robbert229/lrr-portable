# Finding 678 — Lego.c's light setters reach Container.c, and three more functions reach their original size

Issue #815, part of #388, ADR 0005 and ADR 0006. Base: `main` `160f5efc`.
Both sides were measured with full matching builds: `make reccmp-check`,
`make match-progress` and `tools/port_tail_inventory.py`. Unit offsets come
from `results.json` (rebuilt address minus original address).

## Summary

| Measure | `main` `160f5efc` | This change |
| --- | ---: | ---: |
| **Exact-function code bytes** | 344,002 (53.54%) | **347,430 (54.07%)** |
| Functions that rose / fell | | **4 / 0** |
| Accuracy, full / game-only | 97.56% / 97.30% | **97.57% / 97.31%** |
| GAME-021 Lego.c against the original's 90,800 bytes | −224 | **−176** |
| Lego.c's offset at `00432e00` (the masked-prefix blocker) | −576 | **−560** |
| Correct-size functions, full: count / original bytes | 2,796 / 460,177 | **2,799 / 461,980** |
| Correct-size functions, game-only | 2,505 / 409,350 | **2,508 / 411,153** |
| Identical prefix, raw / masked | 11 B / 7,187 B (to `0x402c13`) | 11 B / 7,187 B (to `0x402c13`) |
| Units with size agreement (whole / all sizes / start / end) | 2 / 168 / 5 / 4 | 2 / 168 / 5 / 4 |
| `.text` identical at original addresses | 43,833 B (6.82%) | 42,783 B (6.66%) |
| Fully identical functions at original address | 42 (2,307 B) | 42 (2,307 B) |
| `.data` identical | 24.31% | 24.36% |
| Entry point (original `0048f000`) | `0048eb80` | **`0048ebb0`** |
| SizeOfImage (original 3,608,576) | 4,026,368 | 4,026,368 |
| Image end (gate) | `0x7d8000` | `0x7d7000` |
| Function-order inversions, full / game-only (pairs) | 222 / 1 | 222 / 1 |
| `lego_port.cpp` referenced from units | GAME-021 (3 references) | **none** |

- **The light setters.** `lego_unit.c` now calls Container.c's
  `Container_Light_SetSpotPenumbra` (`00473460`) and
  `Container_Light_SetSpotUmbra` (`00473480`) with a float angle, as the
  original does. `lego_main_sim_step` (`004250c0`) becomes exact (99.61 to
  100, 2,684 bytes). `lego_port.cpp` is no longer a tail root: no unit
  references it. The image ends one page earlier.
- **Three more functions reach the original's padded size**:
  `0042f0c0` (−16), `00433c10` (−16) and `00434710` (−16). Lego.c moves
  48 bytes closer to its original size, and 16 of those bytes are before
  `00432e00`.
- **The symbol counter.** The original's counter phase holds for all nine
  phase-sensitive functions of finding 671, and for `00433380` and
  `00434530` (section 2).

The masked identical prefix does not move. It still stops at `0x402c13`,
the call to `00432e00`, which now sits 560 bytes early instead of 576.
Lego.c accounts for 144 of those bytes and the units before it for 416
(Lego.c itself starts 416 bytes early).

`.text` identical at original addresses falls by 1,050 bytes. Everything
after Lego.c's first fixed function now sits 16 to 48 bytes later than on
`main`. That is nearer the original addresses but not at them, so some runs
that matched by chance are lost. No function's code got worse, and no
unit's size moved away from the original's.

`opt_ref.by_category` moves one 32-byte function from `port_unit` to
`original_annotated` (2,718 to 2,719). This is the `/OPT:NOREF` relink flip
that findings 654, 659, 663, 668, 671 and 676 also saw.

### Rebased onto the #820–#822 stack

Measured again on #822 (`match/819-near-miss-size-first`, `ac51ce6b`). The
base was built on its own for its unit spans and tail roots, and the
branch was measured with the full gate. The same four functions rise and
nothing falls. The counter at `004311d0` is still 6,041.

| Measure | #822 `ac51ce6b` | This change on it |
| --- | ---: | ---: |
| **Exact-function code bytes** | 344,002 (53.54%) | **347,430 (54.07%)** |
| Accuracy, full / game-only | 97.58% / 97.32% | **97.59% / 97.33%** |
| GAME-021 Lego.c | −224 | **−176** |
| Offset of `00432e00` | −576 | −560 |
| Correct-size functions, full | 2,798 (463,008 B) | **2,801 (464,811 B)** |
| Correct-size functions, game-only | 2,507 (412,181 B) | **2,510 (413,984 B)** |
| Identical prefix, raw / masked | 11 / 7,187 B | 11 / 7,187 B |
| Units with size agreement | 2 | 2 |
| `.text` identical at original addresses | 43,072 B | **43,437 B** |
| `.data` identical | 24.31% | 24.41% |
| Entry point (original `0048f000`) | `0048eb80` | **`0048ebb0`** |
| SizeOfImage | 4,030,464 | 4,030,464 |
| Tail objects referenced from units | 9 (`lego_port.cpp` among them) | **8** |

On this base, `.text` identical at original addresses rises by 365
bytes. The later units are close enough to their original addresses that
the 48-byte move gains runs.

## 1. Per-function results

Sizes are `code_extent` (finding 668), original and rebuilt. Scores come
from the full report.

| Address | Function | Original | `main` | After | `main` | After |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| `004250c0` | lego_main_sim_step | 2,684 | 2,684 | 2,684 | 99.61 | **100** |
| `0042f0c0` | lego_level_free | 530 | 527 | **530** | 90.03 | **99.32** |
| `00433c10` | lego_entity_neighbor_blocked_cell_step_probe | 531 | 520 | **531** | 72.12 | **98.83** |
| `00434710` | lego_panel_button_layout_config_load | 742 | 733 | **742** | 95.55 | **100** |

- **`0042f0c0`.** The flag clears are in the original's store order. The
  entity flag's `&= ~0x40` comes second, so it gets `edx` and its own
  `and edx, 0xffffffbf`. The panel flags' two clears come after the five
  zero stores. In the second group, `flags1 &= ~0x10000000` comes after the
  panel's `&= ~4`. What is left is one store's position: the entity flag
  is stored after the zero stores in the original.
- **`00433c10`.** Three changes:
  - The neighbour test is Lego.c's inline wall test,
    `lego_C_block_is_wall` (00432bd0's body, in `lego_maths.h`). It
    materialises 0/1 (`mov eax, 1` / `xor eax, eax` / `test eax, eax`) as
    the original does. It replaces the port's `LEGO_3B_BLOCK_OPEN` macro,
    which is gone.
  - The second branch reads the stats rate before the `elapsed != 0.0f`
    test. The original schedules that load between `fcomp` and `fnstsw`.
  - The rate is read as `*(float*)(LEGO_FIELD(object, char*, 0x334) +
    0x140)`. Through a `void*` base, MSVC copies the float with
    `fld`/`fstp`. Through `char*` it copies it with integer moves, and the
    original's `rate = maxRate` reuses the same register (`mov ecx, edx`).
    The value read is the same.
- **`00434710`.** `copyCount` is declared first, with its initializer
  (`unsigned int copyCount = 0;`). The zero store then comes after the
  `sprintf` arguments as an immediate, as in the original. The free loop
  keeps the count in `eax` before it moves to `edi`. The function is exact
  in the full build.

## 2. The light setters and the symbol counter

Finding 674 section 3 left the setters to Lego.c's owner. In the
matching build `lego_unit_imports.h` aliases the adapter names to
Container.c's link names (`light_interface_call_slot_40`/`_3c`) and
declares them with the original's float angle. Each macro renames one
declaration and its calls (finding 634). Natively, the names stay
`lego_port.cpp`'s adapters, which pass the float's bits to the port's light
interface. They now return the interface's result.
`lego_scene_lights_build_from_config` (`00433770`) passes `0.85f`, `0.4f`
and `0.0f` instead of bit patterns, and its code is unchanged.

The old declarations at line 8620 carried the port's `uint32_t` contract
and conflicted with the new ones, so they had to go. Removing them moved
the counter of every later function by −6 and dropped `00434530` (95.65
to 78.26 in the loop). Two measurements explain the count:

- A redeclaration allocates no symbol for the function's name, but it
  still allocates one for each named parameter.
- A first declaration allocates both.

So each two-parameter declaration that went cost 3, and moving a
redeclared one-parameter prototype moves 1.

`phase_probe.py` and `pads-at` give the windows. On `main`'s counter (net
0), `00434530` keeps its code at net shifts ≡ −1 to +6 (mod 16), and
`00433380` at net shifts ≡ 0 to 3 (mod 8). At −3 and −2, `00433380`
falls to 90.91, which is finding 674's drop. The final tree takes the
counter back to net 0 at line 8620 with two declarations:

- The setters are declared again in the group 3B import block, with the
  float angle, for the lights set-up that uses them. This is +4: two
  redeclarations, two parameters each.
- Group 3C's two message loaders, `lego_text_messages_config_load` and
  `lego_message_load_infomessages`, are declared there rather than with
  group 3C's other entries. This is +2. It is a declaration-only
  deviation and is tagged `Deviation (scoring only)`. The original's
  Lego.h declares all of them before the unit's first function.

Every function from line 8620 to `00433c10` now sees `main`'s counter
exactly. `00433c10`'s new inline test adds 1, so `00434530` sees +1, which
is inside its window. The counter at `004311d0` is still 6,041. No change
before it adds or removes a label.

## 3. Tried and left

Every figure here is from the single-function loop. Each variant was
reverted.

- **`0041f8e0`** (−16). It is `Lego_GetCrystalCount(onlyUsable)`, but
  the port defines an empty `noop_callback(void)` that four other units
  cast and call. The real body needs a parameter, which moves the counter
  of every later function, the nine sensitive ones included. It needs a
  conversion of its own, together with those callers.
- **`00426520`** (−96), **`00430530`** (−16), **`0042f6f0`** (−48). The
  bytes are lost where the original computes `width * y + x` again for
  each block test, and the rebuild shares one copy:
  - In `00426520`, a run of `lego_C_block_*` inline tests loses about 80
    bytes between `00426f3d` and `00427676`.
  - In `00430530`, the original keeps three separate tests in the inlined
    `00430320` (flags1 & 0x400, flags2 & 4, then a recomputed
    flags1 & 0x8000). MSVC merges the two flags1 tests into
    `test ch, 0x84`.

  `00430320` itself is still naked assembly in the port, because no C
  form gives it. Nested `if`s, `?:`, `== 0`, `lego_C_block_is_reinforced`
  and `lego_C_block_at` all compile to identical code.
- **`00428330`** (+32). Three of the original's choices are not
  reproduced:
  - It keeps the block pointer in `ebp` and `show` in `edx`. The f_02 9/6
    tail then merges with the last `else` branch, which is about 21 bytes.
  - It keeps the early "block moved" return inline, with its own epilogue.
  - It passes `playSound ? sfx : 0`.

  A `goto` for the moved test gives the inline epilogue (69.39 to 70.98
  and 8 bytes less). The register choice did not follow from it, and
  neither did a lower label count. Declaration order changes nothing, and
  `lego_C_block_flag20000000` compiles to the same code as `valid`.
- **`0042bf40`** (+1 byte, +16 padded). The original tests `value % 2` as
  `mov edx, eax; and edx, 1; test dl, dl`. These forms did not give it:
  `== 1`, `!= 0`, `& 1`, truthiness, `(unsigned char)` and `(char)` casts,
  and `unsigned short`/`unsigned char` values. Each one either keeps
  `cmp dl, 1` or tests `al` and reallocates every register.
- **`0042de40`** (−4 bytes, −16 padded). The original loads `blockPos` in
  each `smooth` arm, and MSVC hoists the load above the test. Swapping the
  arms, using `level->map` and copying the struct did not help. A
  `blockPos` null test gives the original's size but changes behaviour.
- **`004311d0`** (−16). The original passes the direction with
  `fld`/`fstp` and uses `blockPos`'s home slot for the direction. These
  forms all compile to identical code: casts, `+ 0.0f`, a `float*` view,
  a `void*` view and a signed direction. The function is phase-sensitive.
- **`00431780`** (+16). The original writes `count >= 2` (`cmp eax, 2;
  sbb; inc`). That form saves 4 bytes but lowers the score to 60.37,
  because every register is reallocated. The original also indexes `dirs`
  through an element pointer.
- **`004355c0`** (−32). Finding 676's float reads, with the original's
  strict-inside test, give 0x363 bytes at 56.31. The original keeps three
  rect zero stores that MSVC drops from every form tried.

## Checks

- `make reccmp-check`: 4 rise, none falls, and the map check is OK.
- `make match-progress`, `make unit-data-offsets` and `make pe-audit` were
  run on the final tree. The three generated files are committed.
  `unit-data-offsets` and `pe-audit` changed no tracked file.
- `make tools-test` (no `--allow-growth`, `reccmp-lint` included) and
  `make winapi-link-check` pass.
- The native build compiles. ctest passes 39 of 39 with the shipped WADs.
  `LegoRR` runs for 25 s with the offscreen video driver without an error.
- No `#pragma`, no pad and no filler. The one deviation is the placement of
  two real declarations, tagged.

## Files

- `src/game/lego_unit.c`: the four functions, the group 3B declarations
  and the lights set-up's float arguments.
- `src/game/lego_unit_imports.h`: the setters' matching-build aliases and
  their float declarations.
- `src/game/lego_port.cpp`: the native adapters return the result.
- `docs/coverage/`: regenerated.

## Reproduce

```sh
make reccmp-check && make match-progress
python3 tools/port_tail_inventory.py --json /tmp/tail.json   # unit_spans: GAME-021 −176
make match-function ADDR=0x433c10
make phase-probe ADDR=0x434530 PROBE_ARGS='0x433380 --range 0 16 1 --at 8631'
```
