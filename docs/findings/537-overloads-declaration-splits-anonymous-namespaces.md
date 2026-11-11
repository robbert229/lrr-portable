# Finding 537 — Relocating overloads, split declarations and shared anonymous-namespace items; Bubbles.c relocated

Issue #485, part of #388 (tooling for the relocation waves #442–#446). It
extends `tools/relocate_unit.py` after [finding 523](523-relocate-unit-tool-and-advisor-relocation.md)
and [finding 530](530-relocating-named-namespace-members.md). Measured on
`main` at `970a1822` (after #483, #484, #488, #495 and #489).

## Summary

1. **Three refusals are gone**, and none blocks a unit any more:
   - **Overloads.** The issue counted GAME-011 and GAME-023 as "several
     declarators". They are two definitions of one name, an annotated one
     and an unannotated overload.
   - **Several declarators, or a type and a variable, in one declaration**
     (G98-009, GAME-005). These are split by a separate, opt-in, recorded
     step whose data layout is checked with MSVC 5.0.
   - **Anonymous-namespace items shared across files** (GAME-018, GAME-050,
     G98-002, G98-015, G98-020, G98-003). Such an item leaves its anonymous
     namespace and is declared in the private header, as statics already
     are.
2. **Proof on GAME-005 (Bubbles.c)**, in three commits: the tool, the split
   (a source change), and the relocation.
   - The split passes the MSVC layout check: 152 data symbols and every data
     section's size are identical.
   - 15 functions, 39 owned globals and 6 closure items move from 6 legacy
     files into `src/game/bubbles.cpp`. Every body hash agrees.
   - The native build, all 35 ctest tests and both link-order checks pass.
3. **Layout** (`binary-progress.json`, against `970a1822`):

   | Measure | Before | After |
   | --- | ---: | ---: |
   | Pair inversions, full | 408,393 | 395,256 (−13,137) |
   | Unit-order pair inversions | 3,323 | 3,303 (−20) |
   | Unit contiguity | 74.65% | 75.12% |
4. **Scores** (ADR 0006 rule 2):
   - 2 functions fall below baseline and 1 rises. 6 listings change, all in
     unchanged source.
   - 2 change instruction count and are flagged for the owner.
   - One drop, `segment_2d_point_distance` (43.56% → 40.89%, +0), sits in
     GAME-007's unit file, which this PR does not edit. It drifted through
     the private headers that file includes and this relocation extends
     (below).
   - Measured on `504f5ef6` (before #489), a GAME-015 function also dropped
     this way: `lava_erosion_delay_from_cell`, 94.74% → 78.95%. On
     `970a1822` its listing equals `main`'s; the drop is gone.
5. **Survey** (on `970a1822`, 48 unrelocated units):

   | | Before | After |
   | --- | ---: | ---: |
   | Relocate as they are | 21 of 48 | 23 of 48 |
   | Relocate after `--split-declarations` | — | 1 more (GAME-005) |

   The three constructs blocked 10 units before and block none after. The
   rest now stop at the per-compiler `*_COM_CALL` macros, `#if` blocks, one
   shared `inline` function and the self-nested `unported_ledger_stubs`
   block.

## The tool changes

### Overloads

When one file defines a name more than once (C++ overloads), each
definition and prototype is keyed by its parameter types, for example
`entity_record_flag20_query(EntitySlot*,uint32_t)`. Parameter names,
defaults and array bounds are dropped (`_param_key`). An annotated overload
moves and an unannotated one stays. A use of the bare name counts as a use
of every overload, which is conservative: it can share more than needed but
never less. A name defined once is keyed as before, so earlier relocations
regenerate identically.

### Several declarators, or a type and a variable

A declaration with several declarators (`static int a = 0, b = 0;`), or one
that defines a type and a variable (`static struct T {…} v[3];`), moves
verbatim when all its names move together. When the names would go
different ways, or one of them has to be shared, the declaration has to be
split first. That is a source change, so it is never done silently:

- **Refusal.** Without the flag, the relocation refuses and names the step:
  `split it first with relocate_unit.py <unit> --split-declarations`.
- **The split step.** `--split-declarations` plans the relocation, rewrites
  only the declarations it needs split, writes them and
  `docs/coverage/relocations/<unit>-splits.csv` (hashes before and after),
  and stops. You commit the split on its own (ADR 0006 rule 3), then run the
  relocation.
- **The rewrite** is one declaration per name, in the original order:
  - shared specifiers are repeated (`static int a = 0;` and
    `static int b = 0;`);
  - a type-and-variable declaration becomes the type, then
    `static struct T v[3];`;
  - leading comments stay above the first line, a trailing comment follows
    the last.
- **Refused splits:** an untagged type, which a separate declaration could
  not name, and function-pointer declarators.
- **Verification.** The result is re-scanned: one name per declaration, the
  same names, the same annotations.
- **Layout check (finding 525).** `--check-layout` compiles each split source
  before and after with the matching flags (MSVC 5.0, `match_function.py`'s
  compile commands). It compares `data_layout`: every data symbol's section,
  offset and order, from COFF externals, commons and CodeView data records
  (statics are named only there), plus every data section's size. Order is
  part of the comparison because common allocation follows the COFF symbol
  stream (finding 525). A changed layout is refused unless
  `--allow-layout-change`.

  The check is sensitive: swapping two adjacent statics in `effects.cpp` left
  their `.bss` offsets unchanged but reordered their records, and the check
  reported the change.

### Anonymous-namespace items shared across files

An item in `namespace { }` (or `namespace X { namespace { } }`) has internal
linkage. When moved code and remaining code both need it, it now does what a
shared `static` does:
- it leaves its anonymous namespace and gains external linkage;
- it is declared in the private header, inside X if it has one;
- the change is recorded ("left the anonymous namespace") and covered by the
  hash, annotation and scope checks.

In the source, the anonymous block is closed before the shared item and
reopened after it, so source order is unchanged; blocks the edit leaves
empty are dropped. A moving item lands in the unit file outside any
anonymous block. A file-local type the header needs moves to the header in
the outer namespace. A shared `inline` function in an anonymous namespace is
still refused.

GAME-018 shows this on real code. Four `lego::` hooks in `upgrades.cpp` leave
their anonymous block, and `upgrades_shared.hpp` declares them in
`namespace lego`. A scratch relocation of GAME-018 built natively and passed
all 35 ctest tests. It is not part of this PR: GAME-018 is free to relocate,
but the proof is GAME-005.

### Also fixed

- **Duplicate private includes.** A unit file could include a private header
  twice: once from a source's include list, once from the private-include
  block. `lava_erosion.cpp` (from #477) has two such duplicates. The tool now
  includes each header once.
- **The survey** tries `--split-declarations` for units that need only a
  split, and reports "ok after --split-declarations".

## Proof: GAME-005 (Bubbles.c) into `src/game/bubbles.cpp`

1. **The split**, its own commit (`--split-declarations --check-layout`).
   - It splits `effects.cpp`'s `static struct ScreenEffectTickRecord {…}
     s_screen_effect_tick_records[3][50];` into the type and the array.
   - The array is shared between GAME-005's code and the rest of
     `effects.cpp`.
   - Layout: unchanged, 152 data symbols and every data section's size.
   - Recorded in `docs/coverage/relocations/GAME-005-splits.csv`.
2. **The relocation.**
   - It moves 15 functions, 39 owned globals (the action-name strings and
     tables) and 6 closure items from `crt.cpp`, `ai.cpp`, `cmd.cpp`,
     `effects.cpp`, `entity.cpp` and `util.cpp`.
   - `ScreenEffectTickRecord` goes to `effects_shared.hpp` as a shared type,
     next to `extern struct ScreenEffectTickRecord
     s_screen_effect_tick_records[3][50];`.
   - Three legacy keys move to their new lower medians: `crt.cpp`, `ai.cpp`
     and `entity.cpp`.
   - Hash record: `docs/coverage/relocations/GAME-005.csv`. Every body agrees.
3. **Gates.**
   - Native build and ctest: 35/35.
   - `make tools-test`, with #488's annotation-lint ratchet: passes. The
     ratchet keys definitions by name and signature hash, so a verbatim move
     keeps its identity.
   - `link_order.py check` and `check-map`, and Meson parity: OK.
   - `make reccmp-check`: the drops below.
   - `make match-progress`, and the local freshness check: fresh.

### Layout metrics

`binary-progress.json`, `main` (`970a1822`) against this change:

| Measure | Before | After |
| --- | ---: | ---: |
| Pair inversions, full | 408,393 | 395,256 |
| Pair inversions, game-only | 394,790 | 381,653 |
| Adjacent inversions, full | 764 | 756 |
| Unit-order inversions, pairs | 3,323 | 3,303 |
| Unit-order inversions, adjacent | 83 | 82 |
| Unit contiguity | 74.65% (162 units) | 75.12% (163 units) |
| Exact-function code bytes | 44,636 | 44,636 |
| Mean score, full / game-only | 55.19% / 51.04% | 55.19% / 51.04% |
| Unit-relative identity (masked) | 36,674 B | 36,674 B |
| `.text` identical at original addresses | 18,245 B | 18,298 B |
| `.rdata` identical at original addresses | 1,519 B | 1,276 B |
| `.data` identical at original addresses | 13,918 B | 13,679 B |

The `.rdata` and `.data` at-address measures fall a little because
Bubbles.c's 39 globals, most of them strings, now link with GAME-005, near
the front. Everything after them shifts. These measures stay at chance
until whole prefixes are size-exact (ADR 0006 decision 6).

### Every changed function, with its classifier label

Each changed listing was compared instruction by instruction with a report
of `main`'s tree whose digests equal `main`'s (2,378 of 2,378). None of
these functions' source changes.

| Address | Function | File | Score | Label | Change |
| --- | --- | --- | --- | --- | --- |
| `0x004089d0` | `segment_2d_point_distance` | `placement.cpp` (GAME-007, not edited) | 43.56 → 40.89 (**drop**) | `drift-review`, +0, **owner review** | x87 stack slots |
| `0x00444680` | `entity_step_movement_vector_resolve` | `entity.cpp` | 4.89 → 4.88 (**drop**) | `drift-review`, **+1, owner review** | one `fxch` more |
| `0x0044a070` | `entity_follow_target_pose_get` | `entity.cpp` | 22.49 → 28.69 | `drift-review`, **−2, owner review** | a reload and `fxch` removed; x87 schedule |
| `0x004078c0` | `ledger_stub_004078c0_…` (moved) | `bubbles.cpp` | unchanged | `drift-review`, +0 | a pushed literal is now named `(string)` |
| `0x00446610` | `ledger_stub_00446610_…` | `entity.cpp` | unchanged | `drift-review`, +0 | commutative `fmul` operands swap |
| `0x00463d20` | `view_blit_rect_positions_apply` | `crt.cpp` | unchanged | `drift-review`, +0 | commutative `fadd` operands swap |

The baseline is refreshed with the two drops and the improvement. The two
instruction-count changes (+1, −2) and the +0 drop in an unedited unit file
are flagged for the owner.

### Private headers spread drift to unit files

This relocation extends `crt_shared.hpp`, `cmd_shared.hpp` and
`util_shared.hpp` (and creates or extends `ai_shared.hpp` and
`effects_shared.hpp`). A relocated unit includes the private headers of the
files it came from. On `970a1822`, 12 earlier unit files include at least
one of the three extended headers: GAME-001, 006–010, 012–015, 019 and
G98-008. Every later relocation from those files changes their declaration
counts: finding 506's drift, which shared headers are kept unchanged to
avoid.

Whether a given function's code changes depends on the numbering.
- On `504f5ef6` the effect reached `lava_erosion_delay_from_cell` in
  GAME-015's `lava_erosion.cpp` (−15.79 points) and
  `segment_2d_point_distance` in GAME-007's `placement.cpp`.
- On `970a1822`, after #489's relocations, only `segment_2d_point_distance`
  changes, back to its pre-#483 score.

This is a cost of the one-private-header-per-source design (ADR 0006
decision 4). Two ways to remove it:
- give each relocation its own private header, `<file>_<unit>_shared.hpp`,
  so earlier units never see later declarations;
- or have a unit file include only the declarations it uses.

The owner will file the first as a follow-up ticket. Until then, each
relocation PR should expect drift in units that include the same private
headers.

The duplicate-include fix above also matters here. #489's G98-008
`bitmap_font.cpp`, made with `main`'s tool, includes `frontend_shared.hpp`
under two spellings. This PR's tool compares resolved paths and includes
each header once.

## Survey: before and after

`relocate_unit.py --survey` on `main` (`970a1822`, 48 unrelocated units).
"Before" is `main`'s tool, "after" is this PR's (before this PR's
relocation). A unit lists only the blockers of the first stage that fails.

| Construct | Units blocked, before | After |
| --- | ---: | ---: |
| Several declarators (really overloads, and G98-009) | 3 | **0** |
| Type and variable in one declaration | 1 | **0** |
| Anonymous-namespace item shared | 6 | **0** |
| Needs `--split-declarations` (relocates after the split: GAME-005) | 0 | 2 |
| `#if` block | 16 | 19 |
| Macro `#undef`'d or redefined | 8 | 11 |
| Shared `inline` function | 0 | 1 |
| Nested namespace (`unported_ledger_stubs` inside itself) | 0 | 1 |
| Name clash with another file | 1 | 1 |
| Private header needs an unshared static | 1 | 1 |
| Backend-only source | 1 | 1 |
| **Relocate as they are** | **21 of 48** | **23 of 48**, plus GAME-005 after the split |

None regressed.

The issue's ten units, after this PR:

| Unit | Now |
| --- | --- |
| GAME-005 | relocated here (after the split) |
| GAME-011 | relocates as it is |
| GAME-018 | relocates as it is (scratch run built and passed ctest) |
| GAME-023 | overload solved; next, a shared `inline` (`entity_movement_scale_impl`) and the self-nested `unported_ledger_stubs` block at `entity.cpp:15193` |
| G98-009 | its split works; next, `#if` blocks in `sound.cpp` |
| GAME-050, G98-002, G98-015, G98-020, G98-003 | anonymous items handled; next, the per-compiler `*_COM_CALL` macros and `#if` blocks |

For the next tooling ticket, by units unblocked: conditionally defined
calling-convention macros (`VIEW_COM_CALL`, `RECORD_COM_CALL`,
`SCENE_RESOURCE_COM_CALL` and the like) and the `#if` blocks around them,
then per-relocation private headers.
