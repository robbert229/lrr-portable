# Finding 660 — The four largest Interface.c and FrontEnd.c near misses

Issue #773, part of #388 and ADR 0005. A dedicated effort on four large
non-exact functions:

| Address | Function | Unit | Size | `main` | After |
| --- | --- | --- | ---: | ---: | ---: |
| `0041d040` | interface_is_item_available | Interface.c | 3,168 | 94.92 | 94.92 |
| `0041dca0` | interface_do_action | Interface.c | 2,736 | 86.90 | **87.34** |
| `004140c0` | front_load_menu_set | FrontEnd.c | 3,024 | 83.84 | **88.14** |
| `00412c00` | front_menu_update | FrontEnd.c | 3,440 | 49.10 | **49.24** |

Measured on `main` `bf6335e3` with one full `make reccmp-check` before the
change and one after.

| Measure | `main` | This change |
| --- | ---: | ---: |
| Functions that rose / fell | | 3 / **0** |
| New exact functions | | 0 |
| Exact-function code bytes | 335,096 (52.15%) | 335,096 (52.15%) |
| Exact / mapped, full | 2,408 / 2,833 | 2,408 / 2,833 |
| Reccmp accuracy, full / game-only | 97.02% / 96.68% | 97.02% / 96.68% (the rise is below the second decimal) |
| Image end (`make pe-audit`) | `0x7f5000` | `0x7f5000` |

None of the four became exact, so exact-function bytes do not move.
`004140c0` and `0041dca0` are 80 and 4 bytes shorter, so every later
object starts 0x60 bytes earlier in the gate link; no function changes
score from that. No function fell below the baseline. `make
match-progress`, `make unit-data-offsets` and `make pe-audit` were run and
the generated files are committed. `make tools-test` (no
`--allow-growth`), `make winapi-link-check` and native ctest pass.

No file owned by #772, #774, #775 or #776 is touched, and no declaration
was added or removed, so no unit's symbol counter moves.

## Method notes

- **Callee names.** In `make match-function` all four score 1.9–2.9 points
  lower than in the full report. The cause is naming only: the unit calls
  Message.c, Stats.c, Dependencies.c and Construction.c through their
  `*_link_names.h` port names, which the single-function image labels
  differently from the original. The full report names both sides the same.
  Deltas in the single-function loop are still reliable.
- **Rule B.** A counter sweep of N = 0–72 inert declarations before
  `0041d040` (diagnostic only, not committed) left both Interface.c
  functions' code unchanged at every N.
- **Case order.** `0041d040`'s and `0041dca0`'s jump tables already point
  at the original's offsets in order (`0041d040`: all 71 entries agree; the
  ones after case 12 are 4 bytes off only because of the inline epilogue
  below). Case order is therefore already the original's, and a case search
  can only win by misaligning layout (below).

## `004140c0` front_load_menu_set (83.84 → 88.14)

**Fixed: the `Next` item's menu index.** Finding 649 left this function
0x50 bytes long, with `atoi(...) - 1` folded into `[ecx+eax*4-4]` and the
`numParts == 6` and `*stringParts[4]` branches not cross-jumped. The
original computes the index first (`call _atoi; dec eax; push 1`) and only
then pushes the remaining arguments. As an argument expression MSVC
evaluates the call after the `push`. Written as a statement, `k = atoi(
stringParts[4] + 4) - 1;` (`k` is the Cycle case's loop index, free here),
MSVC emits `dec eax` before the push. The two banner branches then end
identically and are cross-jumped (`push 1; jmp` / `push 0`), as in the
original. The function is now the original's size (0xbd0). The `numParts
== 8` branch takes the same form (another 0.16).

**Left: register roles and stack slots.** The original keeps `args` (the
`va_list`) in `edi` and `menuSet` in `esi`; ours swaps them throughout.
The small locals sit 4 bytes apart from the original's (for example `j` at
`[esp+0x1c]` against our `0x18`) in the same frame (`0xddc`).

- `decls` search: all 729 candidates, no gain.
- `operands` search: 435 candidates including pairs (finished), no gain.
- Case order: finding 649's.

## `0041dca0` interface_do_action (86.90 → 87.34)

Divergent regions, in address order:

1. **The early return.** The original's `if (Interface_IsItemDisabled())`
   jumps to one shared `return 0` block at the very end, after the final
   `return 1`. That block is the real epilogue (`pop edi; pop esi; xor
   eax, eax; pop ebx; ret`, scheduled). Ours had an inline `xor eax, eax`
   epilogue at each of the three `return 0` sites.
2. **Case 0's menu-type switch.** The original keeps its three bodies (18,
   then 16/17). Ours cross-jumps the 18 body into case 18's tail and the
   16/17 body into case 17's.
3. **Cases 58–63 and 73**, the other two `return 0` sites (as 1).
4. **Cases 20, 21, 28 and 27.** The original keeps each one's head
   (`mov al, [flags]; mov ebx, mask; test bl, al; je tail; call; ret`),
   merges only 30's and 29's (`mov ebx; jmp`), and shares the tail. Ours
   merges all six heads into case 29's.

**Fixed (part of 1 and 3).** The three failure returns are `goto fail;`
with one `fail: return 0;` after the final return. MSVC then jumps to one
shared block, as the original does, though it places that block at case
73's site rather than at the end. Single-function 83.99 → 85.20; full report
86.90 → 87.34.

**Tried, not better** (single-function scores against 83.99):

| Variant | Score |
| --- | ---: |
| switch wrapped in `if (!disabled) { …; return 1; } return 0;` | 52.87 (cross-jumps every `selection_clear…(); return 1;` tail; finding 636 had 28%) |
| `goto fail` for the first return only | 83.99 |
| unreachable trailing `return 0;` | 83.99 (removed before layout) |
| case 0's inner switch: case 18 first; `default: break;`; `if`/`else` in case 18 | 83.99 |
| case 0's inner switch as the `if` chain Ghidra prints | 84.16 (wrong compare sequence) |
| every in-switch `selection_clear…(); return 1;` as `break;` | 83.99, identical code (below) |
| cases 20–30 as `mask = N; goto toggle;` with one shared block | 83.99, identical code |
| the same with each case's own test before `goto` | 78.22 |
| 1–24 dummy branches or `return 0`s before the switch (diagnostic) | merge pattern unchanged |

**Case search (finding 649's unfinished search).** `make match-search
MODE=cases` on the outer switch ran its 900 s cap: 1,504 of 3,600 single
insertion moves, best 85.33 in the single-function loop (`case 24` moved
up behind `case 0`). That order contradicts the original's jump table:
case 24's body lands where our cross-jumped case-0 bodies left room, so
the gain is Reccmp's alignment, not the original's code. It is not
applied. `--lines 2472` does not isolate the inner mask switch (the outer
switch overlaps every line), so its 719 orders were again not searched
alone. A run on the committed source (with `goto fail`) was not repeated.

**Left:** regions 2 and 4, and 1's block placement (case 73 rather than
the end). All three are MSVC 5.0 cross-jump choices; see the experiments
below.

## `0041d040` interface_is_item_available (94.92, unchanged)

The only difference in the full report is where one `return 0` keeps an
inline epilogue. Case 12's `flag20 || maskC0 || !canEnter` group gets one
in the original, and the shared tail of cases 49–56 is cross-jumped to the
common `xor eax, eax` block. Ours does the reverse. Everything else
differs only by the 4-byte shift between the two sites.

Tried (single-function, against 93.07):

- **Split case 12's last test** (finding 636's move): case 12 then matches,
  but the 49–56 tail keeps its inline epilogue too (+4 bytes, 88.63).
- **Every split of an `||` group and every join of two adjacent single
  tests**, one at a time (60 moves): none better. 15 of them give
  byte-identical code. 22 random combinations of those 15 neutral moves
  also give identical code, so the statement count does not decide the
  merge.
- **Cases 49–56 as `||` groups** (56 only, 53–56, all; with and without
  the split): 85.8–88.4.
- **Case 48** moved before 49, 50, 53, 56, 33 or 65: equal or worse.
- **Cases search:** stopped at 140 of 2,401 moves (about 3 s each), none
  better. The jump table already matches.

## `00412c00` front_menu_update (49.10 → 49.24)

The structure already matches (finding 655). The gap is register roles
throughout:

- the original keeps 0 in `ebx` and -1 in `edi` early on, and the view in
  `esi`, with `oldCamera` in a stack slot;
- ours keeps 0 in `esi` for longer, so it pushes `esi` where the original
  pushes `0` and compares with `cmp reg, esi` where the original uses
  `test reg, reg`.

**Fixed:** the `stores` search found two independent reorders, `selItem`
before `selCenterOff` in the unfocused Select loop and `s_saveSlotToWrite =
-1` before `s_saveMenuOverwriteShowing = 0`. 49.10 → 49.24.

The `operands` search stopped at its 1,200 s cap after 1,596 candidates
(pairs phase), with no gain. The case orders are finding 649's.

## MSVC 5.0 cross-jumping: what the experiments show

The Interface.c gaps are all choices of MSVC's cross-jumping (tail
merging) and epilogue placement. Small C files compiled with the matching
flags show four rules:

1. **A `return 0` after a test keeps its own epilogue only when it is not
   merged.** An unmerged `if (!f()) return 0;` falls through to `pop …;
   ret` and drops the `xor eax, eax`, because `eax` is known zero. A merged
   one becomes `je` to a shared `xor eax, eax` block. That block is the
   last `return 0` in layout (`0041d040`), or, when the function ends in
   `return 0`, the real epilogue at the end.
2. **Shape decides how much merges.** With `if (x) return 0;` at the top
   level and `return 1` at the end, MSVC merged no partially identical
   case tails (`h(i); clr(); return 1;`) in any of 8–75 cases. Wrapped as
   `if (!x) { switch …; clr(); return 1; } return 0;` it merges every one
   of them, down to `push i; jmp`.
3. **The amount of merging depends on the rest of the function.** Six
   cases of the `mask = N; if (flags & mask) {…}` shape all merge alone.
   With N other cases of the form `if (!q(i)) return 0; h(i); return 1;`
   or `if (!q(i)) h(i); return 1;` added, fewer merge: N = 8 leaves 3
   copies and N = 32 leaves all 6. The effect does not depend on where the
   other cases are, and plain-call cases have none. In the real
   `0041dca0`, extra branches before the switch did not change the
   pattern, so this is not a simple per-function count.
4. **MSVC tail-duplicates the code after a switch into each `break`.**
   `case i: h(i); break;` followed by `clr(); return 1;` compiles exactly
   like `case i: h(i); clr(); return 1;`. The two spellings cannot be told
   apart from the binary.

Rule 3 is the likely key to `0041dca0` (the original merges less than
ours) and to `0041d040` (the same number of merges, at different sites).
The rule that picks which candidates merge was not found.

## Unfinished searches

- `0041dca0` cases, outer switch: 1,504 of 3,600 moves (900 s cap), on the
  source before `goto fail`. The inner mask switch (6 blocks) was not
  searched alone.
- `0041d040` cases: 140 of 2,401 moves, stopped (the jump table already
  matches).
- `00412c00` operands: pairs phase, 1,596 candidates (1,200 s cap).

## Files

- `src/game/frontend_unit.c`: `004140c0`'s menu index as a statement;
  `00412c00`'s two store orders.
- `src/game/interface_unit.c`: `0041dca0`'s `goto fail`.
- `docs/coverage/match-baseline.txt`, `match-listing-digests.txt` and
  `binary-progress.json`: regenerated.
