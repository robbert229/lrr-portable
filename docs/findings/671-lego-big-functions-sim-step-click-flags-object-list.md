# Finding 671 — Lego.c's remaining big near misses: the sim step's container local, the click section's two flags, and a counter that must not move

Issue #796, part of #388 and ADR 0005. This is a second pass over Lego.c's
five largest non-exact functions, after finding 659. All scores are from the
full `make reccmp-check` report.

| Address | Function | Size (B) | `main` `1f5d8a1e` | After | Size agrees |
| --- | --- | ---: | ---: | ---: | --- |
| `004250c0` | lego_main_sim_step | 2,684 | 91.51 | **99.61** | yes |
| `00426520` | lego_main_sim_interaction_and_cell_update | 6,266 | 71.21 | **73.90** | no (6,259) |
| `0042e020` | lego_object_list_load_file | 2,171 | 74.24 | **74.85** | no (2,177; `main` 2,176) |
| `00429890` | lego_level_load | 6,740 | 95.46 | 95.46 | yes |
| `0042f6f0` | lego_cell_init_pass | 2,960 | 83.85 | 83.85 | yes |

| Measure | `main` `1f5d8a1e` | This change |
| --- | ---: | ---: |
| Exact-function code bytes | 338,194 (52.63%) | 338,194 (52.63%) |
| Accuracy, full / game-only | 97.10% / 96.76% | 97.10% / **96.77%** |
| `.text` identical at original addresses | 5.34% | **5.38%** |
| Image end (`make pe-audit`) | `0x7f5000` | `0x7f5000` |
| Function-order inversions, full / game-only | 222 / 1 pairs | 222 / 1 pairs |
| Functions that rose / fell | | 3 / **0** |

No function fell below the baseline, so no drop needs a label. `make
match-progress`, `make unit-data-offsets` and `make pe-audit` were run on
the final tree, and the generated files are committed. `unit-data-offsets`
changed no tracked file. In `binary-progress.json`, one function moves from
`opt_ref`'s `original_annotated` category to `port_unit` (2,533 to 2,532).
That is the `/OPT:NOREF` relink flip that findings 654, 659 and 663 also saw.
`make tools-test` (no `--allow-growth`), `make winapi-link-check` and native
ctest (37 of 37) pass, and the native build starts with the shipped WADs.

Every edit is in `src/game/lego_unit.c`. No other thread's file is touched.

## The constraint: Lego.c's symbol counter

Nine functions after the object list are counter-phase sensitive:
`004311d0`, `00431450`, `004314c0`, `00432170`, `004322d0`, `004323f0`,
`00432c50`, `00433030` and `00434530` (finding 538 rule B; all but `004311d0`
are exact). They hold their scores only when MSVC's label/symbol counter reaches
them at the value it has on `main`, modulo 64. The ways to measure this:

- `match_function.py --cflags /FAs --keep` writes a listing.
- The lowest `$L` label number in each `PROC` is the counter's value at that
  function.
- `phase_probe.py pads-at` gives the tolerance:
  - Pads after the object list restore all nine only at a net shift of 0
    or 64.
  - `0042c120` and `0042c330`, before the object list, tolerate +1 to +4.

The counter moves with anything the compiler numbers, not only
declarations:

- Each local is one.
- Each `if`/`else` arm, tail-merged call and `&&` chain adds labels.
- On its own, each of the three improvements below shifted `004311d0` and
  `00431450`.

So every source-shape gain has to be paid for in the same span. The final
tree's counter at `004311d0` equals `main`'s (6,041):

| Edit | Counter |
| --- | ---: |
| sim step: the container local | +1 |
| sim interaction: the click section's flags (below) | +2 |
| object list: `j` for the direction and the free loop, `olName` for the shape | −3 |

The object list's reuses follow the original:

- Its inner `j` loop, its free loop and its direction all live in `esi`.
- Its shape pointer shares `olName`'s stack slot (Ghidra's
  `local_99c` holds both).

A further object-list rewrite was found but not taken. It replaces the
nested `?:` of the drive-type argument with the original's `if`/`else`
calls, which the compiler tail-merges into `push 1`/`push 2`/`push 4`. It
scores 76.88 in the single-function loop with the original's size, but it
costs +4 to +6 on the counter, and no compensating reuse was found.

## `004250c0` lego_main_sim_step: 91.51 → 99.61

- The original calls `LegoObject_GetActivityContainer` before it pushes
  `Container_GetPosition`'s other arguments. That means the container is a
  local, as in `cont = ...; Container_GetPosition(cont, NULL, &wPos)`. As a
  nested argument, the `lea` and `push 0` came first and every later
  register rotated by one.
- The rename fields are written in the original's order: input, then
  position x, then y.

What is left is three call operands, all of them link names:

- The original calls Container.c's `Container_Light_SetSpotPenumbra` and
  `_SetSpotUmbra` (`00473460`/`00473480`, link names
  `light_interface_call_slot_40`/`_3c`) directly.
- `lego_unit.c` reaches them through `lego_port.cpp`'s float adapters.

Pointing the calls at the link names needs one float-typed declaration
before the sim step. `lego_unit.c`'s own later declarations of the same
names (line 8620, in the port's bit-pattern contract) would then redeclare
an existing symbol. That moves the counter by −2 for every later function,
and `00434530` dropped to 78.26. The native port also keeps the bit-pattern
contract. **Follow-up:** give the two light setters their float contract in
both builds, with the declaration count kept. The function should then be
exact (+2,684 exact bytes).

## `00426520` lego_main_sim_interaction_and_cell_update: 71.21 → 73.90

This is finding 659's open "terrain-click flags" item. The decompile's
`bVar12`/`bVar4`/`bVar3` turn out to be two variables:

- **`handled`** (`bVar4`) is the original's in-memory flag: a command was
  found, and it gates `goto tracking`.
- **A per-test flag in a register** (`bVar12`, `ebx`) is reset and set
  around each test. It is the one tested by `if (!x && (block->flags1 & 8))`,
  `if (!x)` and the `0x8000` test. The original sets only `handled` for the
  `0x8000` and `0x100000` tests, then checks `if (handled) goto tracking`.
  The `0x40000000` test sets only the register flag.

`bVar3`, the "no command, play sound 1" flag of the last tests, is
`handled` again (it is false on that path).

Two variables that are dead in this section stand in for the register flag,
so the declaration count does not change:

- `picked` is the register flag.
- `handled` doubles as the sound flag.
- Finding 659 already uses `handled` for the overlay buttons' flag.

`picked = 0;` before the `0x40000000` test, rather than an `else` arm, is
one label fewer and scores higher.

Effect: `handled`'s reference count falls below `block`'s. MSVC then gives
`block` the dead first parameter's slot (`[esp+0x54]`) and `handled` the
second (`[esp+0x58]`), as the original does. On `main`, `handled` took the
first and `block` shared the `__int64` conversion temporary's slot. Frame
operands that agree went from 85 to 120 of 181 (the CFG-propagated frame
metric of finding 659).

Open:

- **`level`/zero registers.** These are unchanged from finding 659. Moving
  or splitting `level`, and reordering the prologue statements, change
  nothing.
- **Reinforced test.** The original materializes `Level_Block_IsReinforced`'s
  result as 0/1 (`xor ecx, ecx` / `mov ecx, 1` / `test ecx, ecx`). Four inline
  bodies (a local block pointer, `?:`, `if`/`return`) and the use-site forms
  `== 0`/`!= 1` all fold. `? 0 : 1` materializes, but adds 20 bytes.
- **Overlay buttons.** The original stores `(float)mouseX` and
  `(float)mouseY` in the two dead parameter slots and loads the image's
  width and height first. Block locals `w`, `h`, `mx` and `my` reproduce the
  loads (+0.15). They cost 8 on the counter, so they are not applied.

## `0042e020` lego_object_list_load_file: 74.24 → 74.85

The fixes:

- `0.0f == (health = ...)`: the original loads the constant before the
  `fst`.
- The declaration and initialisation order `result`, `first`, `count`.
- `bx` is converted before `by`.
- The counter-paying reuses above.

Left:

- Most of the remaining diff is stack slots. The original has one fewer
  slot (`0x9c8` against `0x9cc`): it keeps `by` in `ebp` for the whole loop
  body, where the port spills it.
- The drive-type `if`/`else` described above.

Tried without effect:

- the shape/direction statement orders;
- a `do`/`while` item loop (grows the function);
- five forms of the position conversion;
- returning early on `!result`.

## `00429890` lego_level_load, `0042f6f0` lego_cell_init_pass: unchanged

- **Level load.** The instructions already match the original's. What is
  left is finding 659's five equal-weight map pointers. The two
  `if`/`else` upgrade-time stores, rewritten as an assignment then an `if`,
  each save one label but lower the function (94.52 → 94.09 in the
  single-function loop), so they cannot pay for other edits.
- **Cell init.** The `0x8000` test's CSE barrier and the `fcomp st(1)` …
  `fstp st(0)` pair (a kept difference value) are still open. Three forms of
  the second height difference lower the score, and the inline-body forms
  are counter-neutral but change nothing.

## Method

- **Scoring.** The single-function loop is used for direction. A
  whole-unit `--source` run after each candidate checks the other 220
  functions. Every figure in the tables comes from the full report.
- **Counter.** Each candidate's counter effect is read from the `/FAs`
  listing's `$L` numbers.

The scratch tools (a frame-slot metric with CFG-propagated `esp`, a
listing-counter reader, and a variant runner that reports score and counter
together) are not committed.
