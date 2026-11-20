# 691. Lego.c's `.bss`: its statics, then its externals in definition order

Issue #840: Lego.c's (GAME-021) state in its original form, and its size,
inside finding 671's counter-sensitive span. Base: `main` `77022c16`.
Both sides were measured with full matching builds under the gate's link
mode (`make reccmp-check`, `make match-progress`).

## Summary

1. **The original's Lego.c `.bss` is two runs, not one form.** MSVC 5.0
   lays out an object's uninitialised statics first, in an order set by a
   hash of their names, and its zero-initialised externals after them, in
   definition order (finding 687; section 2 adds the hash). Lego.c's
   `.bss` (`004de208`–`004de590`) splits cleanly on that rule:
   - `004de208`–`004de428` are **statics**: the tool tips' block
     (`208`/`20c`), the radar's last mouse position (`410`/`414`), the drag
     distance (`418`/`41c`) and the last left button (`424`). Their first
     uses in code are out of address order (`425db8`, `4265fd`, `428360`,
     `43555a`), as a hash order would be.
   - `004de428`–`004de590` are **externals**: the other 24 words and the
     rename key map. Their first uses follow their addresses through
     the whole unit (`4230e7` to `435303`), which is definition order
     next to their functions. The four bytes at `004de42c` are the
     alignment before the 256-byte key map: every item of 8 bytes or more
     is 8-aligned (finding 687).

   Finding 686 had the forms the other way round for 13 of the 32 items:
   the five file statics and the function static `004de56c` are
   externals in the original, and seven of the externals are statics.
   Neither a single struct nor all statics fits: a struct would not split
   at `004de428`, and statics would not keep definition order.
2. **The rebuild now has those forms.** In `lego_unit.c`:
   - the seven Run-A items are uninitialised `static`s where they were
     defined;
   - the 25 Run-B items are `= 0` (`= { 0 }`) externals, defined together
     after the last function in the original's order. Each keeps an
     `extern` declaration where its definition or `static` was, so no
     file-scope symbol moves.
3. **Lego.c's externals are in the original's order.** The rebuilt object's
   externals are `s_lego_renameCursorOn` through `s_lego_tutorialIconPhase`
   in address order. Gaps between consecutive Lego.c `.bss` items that
   agree with the original: **22 of 31** (14 before). `make
   unit-data-offsets` reports the unit's `.bss` as `partial (32, 0
   unknown)`, first difference `004de20c` (it was `partial (31, 1
   unknown)`, first difference `004de410`).
4. **No item is at its original unit offset, before or after.** The
   original's static run holds 544 bytes, the rebuilt one 28. The
   difference is dead data (section 3), which is not defined (ADR 0005
   rule 4). With it missing, the externals start at offset `0x1c` instead
   of `0x220`. That offset is not 8-aligned relative to the original, so
   the key map gets no pad and every external after it sits 4 bytes
   nearer the run's start than in the original.
5. **The symbol counter holds.** A function static costs one symbol more
   than a block-scope `extern` declaration. Section 4 gives the count and
   the one tagged declaration that restores it. Every Lego.c function from
   `lego_tooltip_info_strings_load` (`00427e00`) on sees `main`'s counter
   exactly (6,041 at `004311d0`). Only `lego_main_sim_interaction_and_cell_update`
   sees −1, and its code is identical. No function's score changes.
6. **The size deficits are unchanged** (section 5). Lego.c is still 176
   bytes short of the original's 90,800, and the masked prefix still stops
   at `0x402c13`.

## Before and after

| Measure (`binary-progress.json`) | `main` `77022c16` | This change |
| --- | ---: | ---: |
| **Exact-function code bytes** | **351,876 (54.76%)** | **351,876 (54.76%)** |
| Functions that rose / fell | | 0 / 0 |
| Accuracy, full / game-only | 97.78% / 97.55% | 97.78% / 97.55% |
| Correct-size functions, full | 2,811 | 2,811 |
| Identical prefix, raw / masked | 11 B / 7,187 B (to `0x402c13`) | same |
| Units with size agreement | 2 | 2 |
| GAME-021 Lego.c against the original's 90,800 bytes | −176 | −176 |
| `.text` identical at original addresses | 6.92% (44,439 B) | 6.92% (44,440 B) |
| `.text` at original addresses (`text_at_original_addresses`) | 44,553 B | 44,554 B |
| `.data` / `.rdata` identical | 28.48% / 9.56% | same |
| Entry point (original `0048f000`) | `0048ea60` | `0048ea60` |
| SizeOfImage (original 3,608,576) | 3,596,288 | 3,596,288 |
| Data-offset agreement (`layout`) | 22 of 57 units | 22 of 57 |
| Tail roots | 0 | 0 |
| GAME-021 `.bss` (`make unit-data-offsets`) | partial (31, 1 unknown), first `004de410` | partial (32, 0 unknown), first `004de20c` |
| GAME-021 `.bss`: agreeing consecutive gaps | 14 of 31 | **22 of 31** |
| GAME-021 `.bss`: items at original unit offset | 0 of 32 | 0 of 32 |
| GAME-021 `.bss` size (rebuilt; original 904) | 384 | 380 |

`data_identity`, per region (items at original offset, first difference):

| Region | `main` `77022c16` | This change |
| --- | --- | --- |
| `.rdata` | starts in place; 0 of 95; `0049e014` | same |
| `.data` | 4,096 early; 192 of 2,454 (3,401 B); `0x4a1d5c` | same |
| `.bss` | 4,224 early; 1 of 149 (1,040 B); `0x4b31c8` (−8, `aitask.c.obj`) | same |
| commons | 10,528 early; 16 of 61 (59,256 B); `0x556fb8` (−3,848) | same |

Lego.c's `.bss` is 4 bytes smaller (the static run lost its alignment gap
before the key map), so the `.bss` items after it move 4 bytes. None of
them was at its original offset, since `.bss` breaks at AITask.c first.

`.text` at original addresses gains one byte: an operand that names a
moved Lego.c item. `opt_ref`'s `original_annotated` count goes from 2,719
to 2,717 functions (64 bytes to `port_unit`). That is the `/OPT:NOREF`
relink flip of AviPlayer's deleting destructors that finding 686 traced;
no source it covers changed.

**Identity.** Nothing moves away from the original: no unit's size moves
away from the original's, and the identical prefix holds.

### Rebased onto #843

Measured again on #843 (`data/837-guid-block` `60d7e869`, on #842) with a
full `make reccmp-check` and `make match-progress`. The base's figures are
its committed `binary-progress.json`. No function rises or falls, and the
counter at `004311d0` is still 6,041 (25 modulo 64).

| Measure | #843 `60d7e869` | This change on it |
| --- | ---: | ---: |
| **Exact-function code bytes** | **351,876 (54.76%)** | **351,876 (54.76%)** |
| Accuracy, full / game-only | 97.78% / 97.55% | same |
| Correct-size functions, full | 2,811 | 2,811 |
| Identical prefix, raw / masked | 11 B / 7,187 B (to `0x402c13`) | same |
| Units with size agreement | 2 | 2 |
| GAME-021 Lego.c | −176 | −176 |
| `.text` identical at original addresses | 44,731 B (6.96%) | 44,732 B |
| `.data` / `.rdata` identical | 28.48% / 23.00% | same |
| Entry point; SizeOfImage | `0048ea60`; 3,600,384 | same |
| GAME-021 `.bss`: agreeing consecutive gaps | 14 of 31 | **22 of 31** |
| `opt_ref` `original_annotated` | 2,718 | 2,717 (the AviPlayer relink flip) |

`data_identity` is the same on both sides. `.rdata` starts in place, with
8 of 96 items at their original offset (first difference `0x49e048`).
`.data` starts in place, with 192 of 2,454 (first break `0x4a1d5c`).
`.bss` is 128 bytes early, with 1 of 149 (first break `0x4b31c8`). The
commons are 6,432 bytes early, with 16 of 61 (first break `0x556fb8`).

## 1. Evidence from the original

Every reference to `004de200`–`004de590` in the original's code, with the
first instruction that uses each address:

| Address | First use | Rebuilt name | Form in the original |
| --- | --- | --- | --- |
| `004de208`, `20c` | `428360` | `s_lego_toolTipBlockX`, `Y` | static |
| `004de210`–`40f` | none | – | dead (static run) |
| `004de410`, `414` | `425db8` | `s_lego_mouseLastY`, `X` | static |
| `004de418`, `41c` | `4265fd` | `s_lego_drag_x`, `y` | static |
| `004de420` | none | – | dead (static run) |
| `004de424` | `43555a` | `s_lego_mslbLast` | static |
| `004de428` | `4230e7` | `s_lego_renameCursorOn` | external |
| `004de42c` | none | – | alignment |
| `004de430` | `423217` | `s_lego_renamePrevKeyMap` | external |
| `004de530`–`548` | `42403f`–`425cd4` | the three text timers, high-fog phase, previous mouse word, umbra, scroll guard | external |
| `004de54c`–`55c` | `425e28` | the radar's drag state | external |
| `004de560`–`570` | `426575`–`42668c` | the pointer state and the hold time | external |
| `004de574`–`57c` | `42805d`–`428b4a` | the tool tip object, the debug toggles | external |
| `004de580` | none | – | dead (external run) |
| `004de584`–`58c` | `4290ca`–`435303` | the retarget object, the neighbour rotation, the tutorial phase | external |

A scan of every aligned and unaligned dword in the image finds no value
inside `004de210`–`004de410`, `004de420`–`004de424`, `004de42c` or
`004de580`.

## 2. MSVC 5.0's static order

Probes compiled with the matching flags (`/ML /Zi /O2 /Ob1`, scratch
only):

- **Statics come first, externals with `= 0` after them** in definition
  order, as finding 687 found. File-scope and function statics share one
  run.
- **The static run's order depends only on the names.** The seven Run-A
  names give the same order in every definition order tried (forward,
  reversed and shuffled).
- **It is a hash order.** One- to four-character names come out sorted;
  longer names do not (`abcdef` + `a`…`z` gives `abcde nopqrstu fghijklm
  vwxyz`). A final mix of the form `x ^ (x >> 6)` fits each 26-name probe
  on its own, but no simple per-character step tried (multiply-add,
  shift-add, shift-xor, rotate-add) fits all of them, so the hash is not
  known.
- **Unused statics keep their space**, as finding 687 found.

The rebuilt static run is `s_lego_mslbLast`, `mouseLastX`, `mouseLastY`,
`toolTipBlockY`, `toolTipBlockX`, `drag_y`, `drag_x`. The original's is
the tool tip pair, the mouse pair (Y first), the drag pair (x first),
then the button. The names are the rebuild's, so this order says nothing
about the original's names. Renaming items to steer the hash would be
choosing names for their hash, so it was not done. The dead bytes
between the pairs mean the run's offsets cannot agree anyway.

## 3. What is not defined

- **`004de210`–`004de410` (512 bytes) and `004de420` (4 bytes)**, in the
  static run, and **`004de580`** (4 bytes), in the external run. Nothing
  refers to them. They are dead data: statics or externals whose only
  readers the linker removed (`/OPT:REF` drops a function, not the `.bss`
  of its object), or that were never used. Nothing names them, so nothing
  is defined (ADR 0005 rule 4), as with finding 687's gaps.
- The 8-byte alignment of `004de208`, `004de410` and `004de418` would also
  fit three 8-byte objects (two-field structs or arrays) in place of six
  words. The code reads each word separately, so it does not tell, and the
  words stay separate.

## 4. The symbol counter

`match_function.py`-style `/FAs` listings of `lego_unit.c` before and after
give each function's lowest `$L` label, the counter's value there:

- Turning a file-scope `static` into an `extern` declaration in the same
  place changes no function's counter. Neither does turning an initialised
  external definition into an `extern` declaration, or a definition at the
  end of the file that only redeclares a name.
- A function static costs **one symbol more** than a block-scope `extern`
  declaration of the same name. With only the block-scope declaration,
  `lego_main_sim_interaction_and_cell_update` and the 126 Lego.c functions
  after it with labels saw −1, `004311d0` among them (6,040).
- A second, file-scope declaration of the same name just before the
  function costs nothing, because the block-scope one then redeclares it.

So the file declares `s_lego_hold_time` again at file scope right after
`lego_main_sim_interaction_and_cell_update`, its one reader. A block-scope
declaration's name goes out of scope with its block, so this is a first
file-scope declaration and costs one symbol. Every later function sees
`main`'s counter. It is tagged `Deviation (scoring only)`. The sim
interaction's own code is identical at −1 (a full instruction comparison
of its listing; only label names differ), and its score is unchanged in
the full report (73.90). Finding 538's rule A still orders
`s_lego_hold_time += elapsedGame` as before.

**Update (lrr-portable#861).** The file-scope redeclaration is gone. The
`lego_terrain_pointer_command_update` (`00425d90`) fix gives that function
a block-scope `Vector3F` for the camera position, one symbol more before
`lego_main_sim_interaction_and_cell_update`. That symbol keeps every later
function at `main`'s counter, as the redeclaration did, so the
redeclaration was removed in the same change; every other function body
in the unit compiles unchanged.

Every function body in `lego_unit.c` was compared between the two
listings, ignoring label numbers: none changed.

**Native build.** `lego_unit.c` is C++ in the native build (ADR 0006), and
the end of the file is inside its `extern "C"` block. A C++-only block
before the first declarations gives the 22 affected names C linkage. The
matching build does not see it.

## 5. The size deficits

Lego.c's deficits are the ones finding 678 lists: `0041f8e0` (−16),
`00426520` (−96), `00428330` (+32), `0042bf40` (+16), `0042de40` (−16),
`0042f6f0` (−48), `00430530` (−16), `004311d0` (−16), `00431780` (+16)
and `004355c0` (−32). None closed.

- **`0041f8e0`** is `Lego_GetCrystalCount(onlyUsable)`: it returns
  `crystals` (`+0x8c`), less `+0x98` when the argument is zero, and
  returns nothing without a level. The port defines an empty
  `noop_callback(void)`. Its real body changes its contract in
  `lego_unit.h` (included by about ten other units), in four other units'
  import headers and in the native CRT test, and it moves the counter of
  every later Lego.c function. It stays with finding 678's follow-up: a
  conversion of its own, together with its callers.
- **`0042de40`.** The original loads `blockPos` in each `smooth` arm; MSVC
  hoists the load. Copying the position inside both arms (not among
  finding 678's attempts) scores 61.86 against 92.31, at the same size.
  Reverted.
- The others are unchanged from finding 678's attempts. The masked prefix
  stops at the call to `00432e00`, which sits 560 bytes early: 144 bytes
  from Lego.c and 416 from the units before it. Closing every Lego.c
  deficit before `00432e00` would not move the prefix on its own.

## Checks

- `make reccmp-check`: no function rises or falls. `make match-progress`
  rewrote the baseline (input hash only), the listing digests and
  `binary-progress.json`.
- `make unit-data-offsets` and `make pe-audit` pass; neither writes a
  tracked file. The image ends at `0x76e000`.
- `make tools-test` (no `--allow-growth`) and `make winapi-link-check`
  pass.
- The native build compiles, its 39 ctest tests pass with the real WADs,
  and `LegoRR` runs with the offscreen video driver until the timeout.
- No `#pragma`, no pad, no filler. One `Deviation (scoring only)`
  declaration (removed since; see the update above). Only `src/game/lego_unit.c` and the generated files change.

## Reproduce

```sh
make reccmp-check && make match-progress
make unit-data-offsets      # GAME-021 .bss: partial (32, 0 unknown)
python3 tools/data_identity.py
```
