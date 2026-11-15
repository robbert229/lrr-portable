# 690. How LINK orders the commons, one gods98 library, and the next `.data` breaks

Issue #839: `.data` and the commons from their first breaks (finding 680
follow-up 5, ADR 0006 decision 5.4). Measured with full matching builds under
the gate's link mode (`/DEBUG /OPT:REF /OPT:NOICF`), first on `main`
`77022c16`, then rebased onto #844 (`identity/840-lego-state-size`
`6da62c64`, stacked on #842 and #843). The tables give the figures against
#844. `data_identity` reads the original-mode relink (finding 687).
The commons mechanism comes from scratch links of small C objects with the
MSVC 5.0 toolchain.

## Summary

1. **LINK 5.0 allocates the commons input by input.** Scratch links show
   three rules:
   - a common belongs to the first object that defines it, in the order
     LINK reads objects; a later definition or an earlier undefined
     reference does not move it;
   - the explicit objects form one group, then each library forms one, in
     command-line order, whichever pass pulled the library's members;
   - within a group the objects come in reverse order of reading (pulling),
     and within an object the commons come in reverse order of its COFF
     symbol table, which MSVC 5.0 orders by name, not by definition.
2. **The original's four groups follow.** The game units are explicit
   objects, so they come in reverse. gods98 came from one library, so it
   comes in reverse too. RNC and Init.c were pulled in the second pass
   (their code follows the import thunks), but from a library searched
   before gods98's, or from gods98's own library: their commons come between
   the two. LIBC comes last.
3. **The matching build now links the same way** (ADR 0006 addendum of
   2026-11-14, proposed): the late units' libraries, then one library for
   gods98's later units, `unit_gods98.lib`, before the other libraries.
   `.text`, `.data` and every non-common symbol keep their addresses. The
   commons now come in the original's group order: game in reverse, RNC,
   Init.c, gods98 in reverse (Wad.c first, Container.c last), LIBC.
   **Commons at their original offset: 16 → 17 items, 59,256 → 179,132
   bytes** (`initGlobs`, 119,876 bytes).
4. **Within one library, LINK pulls members in the order their symbols first
   become unresolved**, which follows symbol-table order and so the names.
   The rebuilt names are not the original's, and the rebuilt references
   alone pulled Font.c before Maths.c and Sound3D.c and AnimClone.c late.
   `LRR_LINK_MEMBER_ORDER` names one live symbol per gods98 unit, in list
   order (`/INCLUDE`), which restores list order. It is marked `Deviation
   (scoring only)`; it changes no code or storage.
5. **Two `.data` breaks are taken in the original's form:**
   - FrontEnd.c's four menu block names are initialised arrays, not
     literals (`004a1f00`–`004a1f3f`). The original has them first in the
     unit's `.data`, before the `-1` words, where MSVC puts plain data.
     `00411360` rises from 91.24% to 92.45%.
   - Objective.c reads `"CrystalObjective"` before `"OreObjective"`, into a
     local, as the original's code does. The two literals change places.
     `00457d40` rises from 61.49% to 71.59%.

   `.data` items at their original offset: 192 → 196 (3,401 → 3,464 bytes).
   FrontEnd.c's `.data` from its `-1` words onward now sits at one offset,
   −8, up to Input.c.
6. **The first `.data` break, `"LWO"` at `004a1d5c`, is not fixed.** Its
   first user is Rock Fall Styles (`0040c954`), but the original has it
   before GAME-009's literals, where no unit references it. Section 4 lists
   what was ruled out.

## Before and after

| Measure (`binary-progress.json`) | #844 `6da62c64` | This change |
| --- | ---: | ---: |
| **Exact-function code bytes** | **351,876 (54.76%)** | **351,876 (54.76%)** |
| Functions that rose / fell | | **2 / 0** |
| Accuracy, full / game-only | 97.78% / 97.55% | 97.79% / 97.55% |
| `.text` identical at original addresses | 6.98% (44,846 B) | 6.94% (44,616 B) |
| `.data` / `.rdata` identical | 28.48% / 23.00% | 28.70% / 23.00% |
| Identical prefix, raw / masked | 11 B / 7,187 B | same |
| Correct-size functions, full | 2,811 | 2,811 |
| Units with size agreement | 2 | 2 |
| Entry point (original `0048f000`) | `0048ea60` | `0048ea50` (16 bytes further) |
| SizeOfImage (original 3,608,576) | 3,600,384 | 3,600,384 |
| Function-order inversions, full / game-only | 222 / 1 | 222 / 1 |
| Data-offset agreement (`layout`) | 22 of 57 units | 22 of 57 |

Scores (Reccmp, full report):

| Function | Before | After |
| --- | ---: | ---: |
| `00411360` front_menu_item_select_test_string_collision | 91.24 | 92.45 |
| `00457d40` objective_load_original | 61.49 | 71.59 |

`data_identity`, region by region (original-mode link):

| Region | Rebuilt start (delta) | At original offset, before → after | First difference, after |
| --- | --- | --- | --- |
| `.rdata` | `0049e000` (0) | 8 (32 B) of 96 → same | `0049e048`, −20 (unchanged) |
| `.data` | `004a1000` (0) | 192 (3,401 B) → **196 (3,464 B)** of 2,454 | `004a1d5c` `"LWO"`, +132 (unchanged) |
| `.bss` | `004b2d30` (−128) | 1 (1,040 B) → same | `004b31c8` AITask.c, −8 (unchanged) |
| commons | `00547c00` (−6,432) | 16 (59,256 B) → **17 (179,132 B)** of 61 | `00556fb8`, −3,848 (unchanged) |

No item is at its original address in any region yet. On `main` the
figures were the same item counts, with `.data`, `.bss` and the commons
starting 4,096, 4,224 and 10,528 bytes early.

Neither rising function reaches 100%, so exact-function code bytes do not
move.

**Identity.** The link change moves no function, no `.data` item and no
`.bss` item outside the commons. The commons region grows by 80 bytes of
alignment (`DATA` group `0x2b8ff4` → `0x2b9044`), inside the last page, so
SizeOfImage does not move. Two measures move away from the original:
- **The entry point moves 16 bytes further** (`0048ea60` to `0048ea50`).
  Objective.c's `00457d40` shrinks from 2,136 to 2,123 bytes, toward its
  original 2,112, but the unit segment as a whole is still short, so every
  function after it moves 16 bytes further from its original address.
- **`.text` identical at original addresses falls by 230 bytes** (6.98% to
  6.94%; 241 bytes on `main`). The commons moved, so code operands that named them now hold
  other values, and the 16-byte shift moves the code after Objective.c.
  These are coincidence-driven bytes (finding 686 measured the same
  effect).

The identical prefix (11 B raw, 7,187 B masked), the correct-size functions
and the unit sizes do not change.

## 1. The commons rule

Each scratch link used C objects with one common and one function each
(`/O2`, which implies `/Gy`, so every common is referenced from the entry
point to survive `/OPT:REF`):

| Link | Commons order |
| --- | --- |
| explicit `a1 l1 l2 l3` | `l3 l2 l1 a1` |
| explicit `a3 a2 a1 l3 l2 l1` | `l1 l2 l3 a1` |
| `a1`, then `l1.lib l2.lib l3.lib` | `a1 l1 l2 l3` |
| `a1`, then one library of `l1 l2 l3` (pulled `l1 l2 l3`) | `a1 l3 l2 l1` |
| as above, `a1` referencing `l3c` first (pulled `l3 l1 l2`) | `a1 l2 l1 l3` |
| `a1` also defining `a3c` (its symbol table: `a3c a1c`) | `a2 a1 a3` |
| `a1`, `G.lib`, `H.lib`, LIBC; `H` needs `r1` from `G.lib` (pulled after LIBC) | `a1 r1 g2 g1 h1`, LIBC |
| `a1`, `G.lib` (`g1 g2 g3`), LIBC, `R.lib` | `a1 g3 g2 g1`, LIBC, `r1` |
| `a1`, `G1.lib G2.lib G3.lib`, LIBC, `R.lib` (the old matching build's shape) | `a1 g1 g2 g3`, LIBC, `r1` |

- The seventh row is the original's late units: `r1`'s code follows LIBC's,
  but its common comes first in its library's group.
- MSVC 5.0 orders an object's symbol table by name: `int aaa; ... int fff;`
  and the reverse definition order both give `fff ... aaa`. No simple hash
  (sums, multiply-add, shift-xor, modulo up to 8,191) fits a 60-name sample,
  so names cannot yet be chosen to reproduce an order.
- LINK pulls a library's members in the order their symbols became
  unresolved, not in archive order: a library holding `zz mm aa` or
  `aa mm zz` gave the same pull order, the referencing object's symbol
  order.

## 2. The link change

A scratch relink of `main`'s objects compared three layouts:

| Layout | Commons groups | gods98 `.text` |
| --- | --- | --- |
| a library per gods98 unit, late libraries last (`main`) | game rev., gods98 fwd., LIBC, late | list order |
| late libraries, one gods98 library | game rev., RNC, Init.c, gods98 rev., LIBC | Font before Maths; Sound3D, AnimClone moved |
| the same, with `LRR_LINK_MEMBER_ORDER` (this change) | as the original | list order; `.text` identical to `main` |

The full build confirms the scratch relink: `tools/link_order.py
check-map` passes, the function-order inversions stay at 222 / 1, and no
score moves from the link. `tools/link_order.py`'s `member_library()` now
returns `unit_gods98` for every member, so the map check and the tools that
read `link-members.txt` follow.

What remains in the commons, in order:
- `legoGlobs_f_556fb8` comes before `legoGlobs` (−3,848): their order
  within Lego.c follows their names (section 1). `lego_unit.c` is #840's
  file, and no name is known to give the original's order.
- RNC's nine tables are in another order than the original's, for the same
  reason (`initGlobs` after them is in place).
- Wad.c's `wadFiles` comes before `wads`, and the original has 56
  unreferenced bytes after `wads` (`00575668`).
- G98-028's 2,072-byte block (`00575ce0`) is a common in the original and
  `.bss` here: `file_list_streams.cpp` is C++, and MSVC gives a C++ global
  no common.
- Input.c's arrays and the renderer words are still defined in tail
  objects (finding 680).

## 3. The two `.data` edits

**FrontEnd.c.** The original's FrontEnd.c `.data` starts at `004a1f00`
with `"Menu::SaveImage"`, `"Menu::SaveText"`, `"Menu::LevelText"` and
`"Menu::Overwrite"`, then `"Samples"` (`004a1f40`), then the six `-1` words
(`004a1f48`). MSVC emits an object's plain `.data` first and its literals
after (checked with a scratch object, whatever the definition order), so
these five strings are plain data. They were literals here, among the
unit's other literals. The four names are now `static char` arrays defined
before the `-1` words, and their twelve users name them.

`"Samples"` is not added. No code reads it (Lego.c reads its own literal,
`004a4804`), and `make match-function` shows the counter cost of a fifth
array: `004177e0` falls from 100% to 69.33%, `00411530` and `00411870` by up
to 1.5 points. Without it, nothing falls. So FrontEnd.c's items from the
`-1` words onward sit 8 bytes early, as `main`'s did from there on, and
the four arrays reach their original offset.

**Objective.c.** `00457d40` reads `"CrystalObjective"` (`00458330`), stores
the crystal quota in a local, then reads `"OreObjective"` (`0045837e`).
The rebuild passed both as arguments, and MSVC evaluated the ore quota
first, so its literal came first. `crystals = atoi(...)` before the call
restores both the code order and the literal order, and the function rises
by 10.1 points. This is a function-body edit in a unit file; it is listed
in the PR.

## 4. What was tried for `"LWO"`

The original's `"LWO"` (`004a1d5c`) follows Building.c's last literal
(`004a1d4c`) and precedes GAME-009's first (`"%s"`, `004a1d60`). Every
reference to it (`0040c954`, Lego.c's five, `004519dc`, `0046c048`,
`0046cbbc` and Container.c's `00472785`) is in a later unit, and no `.text`
operand in `004078c0`–`0040a0c0` points into `004a1d00`–`004a1d70` apart
from Building.c's own strings. Scratch objects show that MSVC 5.0 emits no
literal for an uncalled `static` function, an uncalled `__inline` function,
an `if (0)` branch, a dead store or an unused local. It does emit one for an
unreferenced `static` data pointer, but that pointer would sit in `.data`,
and the original has none. The owner of `004a1d5c` stays unknown.

Two later breaks are dead data with no attributable owner, and are left as
finding 687 left its gaps:
- 24 bytes at `004aaeb8` (eight RGB triples: black, white, red, green,
  blue, yellow, magenta, cyan), between Images.c's literals and Input.c's;
  either the AVI player's or Input.c's plain data. Every item from Input.c
  onward is 24 bytes earlier because of it.
- the word `0x10` at `004af9e4` before Wad.c's literals (finding 591).

## Files outside this issue's map

- `src/game/objective_unit.c`: the crystal-quota local (a function body).
- `tools/link_order.py`, `tools/tests/test_link_order.py`, and comments in
  `tools/pe_audit.py` and `tools/port_tail_inventory.py` (#841's
  directory): the member library's name.
- `CMakeLists.txt`: the member library and the link order.

## Checks

- `make reccmp-check` passes: two functions rise and none falls. `make
  match-progress` rewrote the baseline, the listing digests and
  `binary-progress.json`.
- `tools/link_order.py check-map` passes on the gate's map.
- `make unit-data-offsets` and `make pe-audit` pass.
- `make tools-test` (without `--allow-growth`) and `make winapi-link-check`
  pass.
- The native build compiles and its 39 ctest tests pass with the real WADs.
  `LegoRR` starts with the offscreen driver and runs until the timeout.

## Reproduce

```
make reccmp-check && make match-progress
python3 tools/data_identity.py
make match-function SOURCE=src/game/frontend_unit.c
make unit-data-offsets && make pe-audit
```
