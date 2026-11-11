# Finding 538 — What makes MSVC 5.0 codegen drift with translation-unit context

Issue #486. Relocations and header edits keep changing code in functions
whose source did not change. Operands of `fmul`, `imul` and `or` swap, an
`fxch` appears or goes, or a spill comes or goes (findings 506, 514, 527,
530 and 532). This finding isolates two rules that explain every case
tested. It applies one of them to restore `0x0040e9c0`
`lava_erosion_delay_from_cell` from 94.74% to 100%, with no other change.

All experiments compile with the matching build's exact `cl` flags (MSVC
5.0, `/O2 /Ob1`, under Wine) and fingerprint the function's `/FAs` listing.
They are rerunnable from [`tools/experiments/486/`](../../tools/experiments/486/README.md).

## The two rules

MSVC 5.0's front end gives every declaration in a translation unit a
sequence number: the *symbol counter* (finding 506). It shows in listings
as the `$S<n>` suffix of a static's name. The two rules use it in different
ways.

**Rule A: declaration order (high confidence, no counterexample).** In a
product of float memory operands that name variables, MSVC emits the
operand with the **higher** symbol number first, i.e. the variable whose
*first* declaration came later. The operand order in the source does not
matter, and neither does the distance between the two declarations. A later
redeclaration or definition keeps the first number. The order is
deterministic and does not depend on the counter's absolute value.

**Rule B: the counter's phase (high confidence for the phenomenon; the
compiler's internal mechanism is not identified).** Every other drift seen
here depends only on the counter's value at the function, taken modulo a
power of two. This covers integer commutative operand order, register
choice, x87 `fld`/`fxch` order, extra `fxch` and spills. The period depends
on the function; 16, 32, 64 and 512 were observed. A run of inert
declarations before the function cycles its code with that period.
Ordinary SDK headers act exactly like that run. What counts is each
declaration's counter cost, not its content, its name or its line.
Declarations after the function, or inside a later block of it, have no
effect. Locals declared at the top of the function interact with the phase
in a way that is not periodic.

A change in the number of declarations before a function therefore re-rolls
every tie that the function's code depends on. Units that move into new
files (the relocation waves) or gain a large include (`d3drm.h`, #481)
change the counter by hundreds or thousands. Each function with such a tie
then lands in the other phase with roughly even odds.

## Experiments

### 1. Reproduction

| Case | Change | Effect (`make match-function`) |
| --- | --- | --- |
| `0x004869a0` Draw_Pixel24 | `#include <windows.h>` + `<d3drm.h>` in `src/gods98/draw.c` (as #484 tried) | 100.00 → 71.43; `(value << 8)` and `(*addr & 0xff)` evaluated in the other order |
| `0x0040e9c0` lava_erosion_delay_from_cell | `main` since #477 | 94.74; the two `fmul [global]` swap order |
| `0x0040fcc0` vector_scale_components_in_place | `main` since #469 | 91.30; `fld [esi]` and `fld [esp+0x10]` swap |

The `draw.c` flip reproduces on the preprocessed file, so preprocessing
plays no part.

### 2. Bisecting `d3drm.h`

| Variant inserted after `draw.c`'s includes | Draw_Pixel24 |
| --- | --- |
| nothing (`main`) | original |
| `<windows.h>` | original |
| `<windows.h>` + `<d3drm.h>` | **flipped** |
| `d3drmdef.h`'s four `static const` definitions alone | original |
| one `static const`, one `static int` or one `extern int` | original |

`ddmin.py` delta-debugged `d3drm.h`'s preprocessed text after
`<windows.h>`: 294 top-level declarations. After 137 compiles, the smallest
set that still flips the function was 22 unrelated typedefs and structs
(`LPDDSURFACEDESC`, `D3DVALUE`, `D3DCOLOR`, `D3DVECTOR`, `D3DTLVERTEX`,
`D3DMATRIX`, `D3DVIEWPORT`, …). None of them is used by `Draw_Pixel24`,
which points to their number, not their content.

### 3. Counting, not content

N copies of one declaration were inserted after `draw.c`'s includes.
Positions are N (`A` is the original code, `B` the flipped code):

| Inserted | First flip | Code by N |
| --- | ---: | --- |
| `extern int pN;`, `typedef int tN;`, `struct sN { int a; };`, `struct sN { int a, b, c, d; };` | 137 | identical for all four |
| `extern int …;` with 2-, 6-, 46- and 85-character names | 130 (`d3drm.h` already included) | identical for all name lengths |
| N blank lines, N comment lines, `#line N` | none | always `A` |
| all `extern` on one line, or one per line | 137 | identical |

Over N = 0–1,420, Draw_Pixel24 flips at exactly N = 137, 393, 649, 905,
1,161 and 1,417. That is a period of 512 with a duty of exactly a half, so
the code is `B` when bit 8 of (N + 119) is set. In a minimal 10-line
translation unit (`pixel24.c`) the same function flips at 171, 427, 683
and 939.

Where the declarations go (minimal unit, N = 150–199):

| Position | Effect |
| --- | --- |
| at the top of the file | flips at 171 |
| between `drawGlobs` and the function | flips at 171 (the static's own number is irrelevant) |
| block-scope `extern`s or unused locals before `addr` | flips at 167, then at 298, 299, 303 and 327 (not periodic) |
| in an inner block after `addr` | no effect |
| after the function | no effect |

### 4. Symbol-counter costs

Each cost is the change in a following static's `$S` number per
construct, measured over ten copies (`run.py counter`):

| Construct | Cost |
| --- | ---: |
| `extern int v;`, `int v;`, `static const float v = 1.0f;` | 1 |
| `typedef int T;` | 1 |
| `struct S { … };`, whatever the number of fields | 1 |
| `typedef struct S { … } T, *PT;` | 3 |
| `enum` with 3 members | 4 (1 + 1 per member) |
| `void f(void);` | 1 |
| `int f(int a, int b);` | 3 (1 + 1 per named parameter) |
| `int f(int x) { return x; }` (definition) | 3 |
| redeclaring an existing variable | 0 |
| `#define`, comments, blank lines | 0 |
| an SDK header whose content an earlier header already declared (`<stdarg.h>`, `<ctype.h>`, `<memory.h>` after `<windows.h>`) | 0–1 |
| block-scope `extern`, local, parameter | 1 |

This refines finding 506's estimate.

### 5. Rule B in the real units

The same sweep (`sweep.py`, matching-build flags, N inert `extern`s after
the unit's includes):

| Function | Unit | Codes | Period | What alternates |
| --- | --- | ---: | ---: | --- |
| `0x004869a0` Draw_Pixel24 | `draw.c` | 2 | 512 | order of the `|` operands (flips at 137 + 256k) |
| `0x00485ea0` viewport_subrectangle_set | `draw.c` | 2 | 16 | x87 operand order |
| `0x00486900` Draw_Pixel16HalfTrans | `draw.c` | 6 | 64 | register choice (several bits) |
| the other 15 `draw.c` functions | `draw.c` | 1 | — | nothing |
| `0x0040e9c0` lava_erosion_delay_from_cell | `lava_erosion.cpp` | 2 | 32 | `imul` operands; the `fmul` order never changes (rule A) |
| `0x0040fcc0` vector_scale_components_in_place | `steering.cpp` | 2 | 512 | `fld` order; the other phase (N ≡ 220–475) scores **100%** |
| `0x00471370` cmd_place_ray_candidate_check | `cmd.cpp` | 2 | 64 | 22 against 23 `fxch`, 193 against 194 instructions (as in #477 and #483) |

The last row is the instruction-count drift that #477 and #483 reported for
owner review. It is rule B: an `fxch` goes in or out with the counter's
phase.

### 6. Predicting `draw.c`

Over N = 0–1,023, the residues N mod 512 that keep **all 18** `draw.c`
functions' code unchanged are 0–6, 48–54, 64–70, 112–118, 128–134, 432–438,
448–454 and 496–502 (even values only; 32 residues in all). The counter
shift of each include set was measured from `drawGlobs`'s `$S`:

| Added to `draw.c` | Counter shift | mod 512 | All 18 unchanged? |
| --- | ---: | ---: | --- |
| `<windows.h>` | 21,551 | 47 | no |
| `<windows.h>` + `<d3drm.h>` (#484's experiment) | 23,815 | 263 | no (Draw_Pixel24, as in #484) |
| … + `<stdio.h>` | 23,923 | 371 | no |
| … + `<stdio.h>` + `<wchar.h>` | | 436 | yes |
| … + `<malloc.h>` + `<digitalv.h>` | | 432 | yes |
| … + `<stdio.h>` + `<mmreg.h>` | | 64 | yes |

80 such include sets were compiled. In **80 of 80** the measured residue
predicted whether all 18 functions kept their code. That makes rule B
predictive, not only descriptive. Header costs do not quite add up, because
headers share sub-includes, so a candidate's residue must be measured.

### 7. Rule A in isolation

`float f(const int* xy) { int s = 6 - xy[0]; return (float)s * a * b; }`
(`run.py order`, 24 variants):

| First declaration | Second declaration | Gap between them | `fmul` order |
| --- | --- | ---: | --- |
| `extern float a;` | `static const float b = 25.0f;` | 0, 255, 300 | `b`, then `a` |
| `static const float b = 25.0f;` | `extern float a;` | 0, 255, 300 | `a`, then `b` |
| `extern float a;` | `extern float b;` | 0, 255, 300 | `b`, then `a` |
| `extern float b;` | `extern float a;` | 0, 255, 300 | `a`, then `b` |

Writing `s * b * a` instead gives the same four results. A definition after
an earlier `extern` (`extern float a; … float a;`) keeps the extern's place.

## Application

### `0x0040e9c0` restored (committed)

The original multiplies by `s_erosion_delay_scale` (`0x004d78bc`) before
the 25.0f constant `s_erosion_ticks_per_second` (`0x0049e124`). Under rule
A, the original's first declaration of the global came **after** the
constant. In C, that happens when the unit defines its state below its
constants and nothing earlier declares it.

Since R1 (#477), `lava_erosion.cpp` includes the private header
`level_shared.hpp`, whose `extern float s_erosion_delay_scale;` precedes
the constants. That is the swap #477 attributed to "the new file's context".
`level_shared.hpp` now hides that one extern from the unit that defines the
variable (`LRR_LAVA_EROSION_UNIT`), so the definition at line 86 is the
first declaration again. The original unit cannot have shared the extern
either. The counter at every function is unchanged, because the
declaration moves rather than disappears. Rule B's phases are therefore
unaffected.

| Function | Unit | Before | After | Classifier label | Instructions |
| --- | --- | ---: | ---: | --- | --- |
| `0x0040e9c0` lava_erosion_delay_from_cell | `lava_erosion.cpp` | 94.74 | **100.00** | improved | 19 → 19 |

`make reccmp-check`: one function improved, none fell, no other listing
changed (game-only mean 51.04%, full 55.20%, on `main` at `970a1822`). `make match-progress`
regenerated the three progress files. Exact-function code bytes went from
44,636 to 44,702. The other `level_shared.hpp` users (`level.cpp`,
`placement.cpp`, `encyclopedia.cpp`, `screen_effect_quads.cpp`) do not
define the macro, so their preprocessed text and counters are unchanged.

### Not applied

- **`0x0040fcc0`** is a rule B phase case: the original's code needs the
  `steering.cpp` counter shifted by 220–475 (mod 512). Only inert
  declarations reach it today, which ADR 0005 entropy rule 3 forbids.
  It recovers when the unit's real declaration set does.
- **`draw.c` with `d3drm.h` (#481).** The include moves the counter by 263
  (mod 512), outside the 32 safe residues. Section 6 lists header sets that
  restore all 18 functions, but nothing shows the original `Draw.c`
  included them. Choosing one only for its count would be a pad by another
  name, so none is committed. For #481 the test is now cheap: an include
  chain backed by evidence is acceptable for `draw.c` exactly when its
  measured residue is in the safe set. `sweep.py` and the include-cost
  method check a candidate in seconds.

## Implications

- **Relocation waves.** Expect drift of this kind in every wave: moving a
  unit changes the counter before its functions by an arbitrary amount.
  Rule B drift is neither a regression nor a fix; it is a coin toss per
  tied function and converges only when a unit's declarations reach the
  original's. Classify it under ADR 0005 rule 2. Instruction-count changes
  of ±1–4 caused by `fxch` or spills belong to it (`0x00471370`). Rule A
  drift has a cause that can be fixed: a private header that declares a
  unit's own globals before the unit's constants or before other globals
  reverses their first-declaration order. Keep a unit's own state out of
  the headers it includes, or keep their order.
- **Diagnosing a drop.** Run `sweep.py` on the function over one period.
  If the original's code appears at some N, the drop is rule B, and the
  required residue tells you how many counter units the unit is missing. If
  it never appears, look for a rule A order change, or for a real source
  difference.
- **#481.** `d3drm.h` is viable in a unit when the unit's counter residue
  after the include falls in its safe set. The safe set is cheap to compute
  per unit (section 6). Config's residue (#484) was not measured here.
- **The pads in `crt.hpp` and `frontend.hpp`** (finding 506) worked
  are consistent with rule B: a count alone, with no content, restored
  code.

## Limits

- Rule B is characterized, not explained. The period and the transitions
  are exact, but which internal quantity the compiler reads (one bit of a
  symbol's number, or several) varies by function and was not
  reverse-engineered. Some functions have ragged transitions
  (`0x00471370`) or more than two codes (`0x00486900`).
- `#pragma` settings were not varied: neither the units nor `d3drm.h`'s
  minimal flipping set contain one, so they cannot explain these cases.
- Rule A was tested for a float product of an `int` conversion and two
  named memory operands. Longer chains, sums and integer products of named
  globals were not tested separately.
