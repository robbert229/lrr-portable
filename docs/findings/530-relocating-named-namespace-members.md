# Finding 530 — Relocating named-namespace members; GAME-007 relocated; three R1 tool defects fixed

Issue #471, part of #388 stage S2 (tooling for the relocation waves
#442–#446). It extends `tools/relocate_unit.py` from
[finding 523](523-relocate-unit-tool-and-advisor-relocation.md) and fixes
the three tool defects that [finding 527](527-relocation-wave-r1-unblocked-units.md)'s
wave R1 worked around by hand. Measured on `main` at `d4af3221` (after #477,
#479, #482 and #480).

## Summary

1. **Named namespaces relocate.**
   - Members of `namespace X { }`, including `namespace X { namespace { } }`,
     move verbatim into the same blocks, reopened in the unit file.
   - Out-of-line `T X::f()` definitions move as written.
   - Shared namespace members are declared inside `namespace X { }` in the
     private header.
   - A file-scope `using` that the moved code depends on is copied.
   - The verbatim check also requires every moved item to keep its scope.
2. **Proof on GAME-007** (the 2D point/segment and placement-search helpers;
   an R1 unit):
   - 30 functions, 3 owned globals and 6 closure items move from 7 legacy
     files into `src/game/placement.cpp`.
   - 7 of the moved items are members of `unported_ledger_stubs`.
   - Every body hash agrees.
   - The native build, all 35 ctest tests and both link-order checks pass.
3. **Layout** (`binary-progress.json`):

   | Measure | Before | After |
   | --- | ---: | ---: |
   | Pair inversions, full | 513,793 | 493,667 (−20,126) |
   | Unit-order pair inversions | 3,585 | 3,533 (−52) |
   | Unit contiguity | 70.05% | 70.51% |
4. **Scores** (ADR 0006 rule 2):
   - 1 function falls below baseline and 6 rise. 8 listings change in
     total.
   - 5 of the 8 change instruction count and are flagged for the owner.
     The one drop is among them: `terrain_cell_normal_sample`
     (`0x004512d0`), at −1 instruction.
   - All 8 are compiler scheduling or operand-order changes in unchanged
     source. Exact-function bytes do not change.
5. **R1's three defects are fixed**, each with a fixture test:
   - a `#define` that ended early at a `/* */` comment;
   - a private include placed too late in the file;
   - `.bss` slack that crossed into the next unit.

   Replaying R1 on `main` before #477 needs none of its five hand fix-up
   commits. The replay builds natively and passes ctest.
6. **Survey.**
   - Named namespaces blocked 31 units and now block none.
   - 33 of 59 unrelocated units relocate as they are, against 18 before.
   - 14 of the 31 namespace-blocked units are fully unblocked. The other 17
     stop at other constructs, listed below.

## The tool changes

### Scoped names

`cxx_items.py` now records more:
- **qualified uses**: `X::n` is one token, and `::n` reads as `n`;
- the **qualifier** of an out-of-line definition (`T X::f()`);
- **`using` items**: `using namespace X;` and `using X::n;`. An alias
  `using T = ...;` counts as a type.

`relocate_unit.py` keys every symbol by its qualified name: `n` in the
global scope, `X::n` in namespace X. A use resolves as C++ looks names up:
- `X::n` resolves exactly;
- a bare `n` inside X resolves to `X::n` when the file defines it, else to
  the global `n`;
- a bare `n` in the global scope resolves through the file's `using` items.

Header visibility is computed with the same qualified names, from
`cxx_items` scans of the included headers. The earlier tool used a scanner
that ignored namespaces.

An out-of-line definition belongs to X only when X is a namespace somewhere
in `src/`. `AviPlaybackState::` member definitions are still refused.

### Moving and sharing members

- **The unit file.** Moved members are wrapped in `namespace X { … } //
  namespace X`. Consecutive members with the same wrap share one block.
  Address order and leading comments are kept.
- **Out-of-line definitions** move unwrapped, exactly as written.
- **The private header.** Shared statics and declarations of namespace
  members are written inside `namespace X { }`, both in the private header
  and as forward declarations in the unit file. An out-of-line definition's
  qualifier is dropped inside the block.
- **Emptied blocks.** A `namespace X { }` block that the relocation empties
  is removed from the source if X stays declared: by another block in the
  file, or by an included header. Otherwise it is kept, since a later
  `X::` use may need it.
- **`using` items.** `using namespace X;` is copied after the includes when
  moved global-scope code uses a bare name that resolves only to `X::name`.
  `using X::n;` is copied when that code uses a bare `n`. Both stay in the
  source as well.
- **Anonymous namespace inside a named one.** `namespace X { namespace { } }`
  is scope X with internal linkage, and moves wrapped the same way. It
  occurs in `lego` (`scene_pool.cpp`, `scene_resources.cpp`,
  `weapon.cpp`, `spawn.cpp`, `upgrades.cpp`).
- **Other nesting** (two named namespaces, or a namespace inside
  `extern "C"`) is refused. It occurs twice in `src/`, both times as
  `unported_ledger_stubs` reopened inside itself before its block closes:
  `entity.cpp:15193` (around `0x00444460`) and `view.cpp:7359`. No unit in
  the survey reaches it yet. When one does, the fix is a source one: close
  the outer block first.

### Other fixes found on the way

- **Annotated items stay with their unit.** An item annotated for another
  unit is never moved as closure. Before, a static `FUNCTION`-annotated
  helper of GAME-002 could ride along with GAME-007, and #465's static check
  caught it. Such an item now stays and gets external linkage if needed.
- **Type and variable in one declaration.** A declaration that defines a
  type and a variable (`static struct T {…} v[3];`) records T as well. It is
  refused if it has to be shared, because declaring it would define T
  twice. It still moves whole if only moved code uses it.
- **Clashes.** The clash check counts definitions only. An `extern`
  declaration or prototype from one source may meet the definition from
  another. This unblocked GAME-019, GAME-028 and GAME-042.
- **`#if` groups that lose `static`.** For such a group, the body comparison
  now compares against the expected text. A latent bug compared it with the
  original, and GAME-010 tripped it.
- **Stems.** When the default stem names an existing file, the tool uses
  `<stem>_unit` and says so: G98-013 gives `input_unit`, G98-016
  `flic_unit`, GAME-029 `objective_unit`.

## The three R1 defects (finding 527)

| Defect | Fix | Test |
| --- | --- | --- |
| A `#define` ended at a line inside a `/* */` comment. The rest of `entity_config.cpp`'s ObjectNames load macro scanned as items, and `extern i < 9u;` landed in `entity_config_shared.hpp`, twice | A directive continues while a block comment that began in it is open. A backslash after the comment's end continues it as before | `test_block_comment_inside_a_define_belongs_to_it`, `test_macro_with_comment_does_not_leak_into_the_private_header` |
| The private include went after the file's *last* `#include`, too late in `frameloop.cpp` and `entity.cpp` | It goes after the last unconditional `#include` before the first use of one of the header's names. With no use, it goes after the top include block. An existing private include moves up when a new relocation's names are used earlier. The verification pass refuses if code before the include uses the header's names | `test_private_header_goes_after_the_top_include_block`, `test_includes_before_the_first_use_stay_in_front`, `test_late_private_include_is_caught` |
| `.bss` slack (`hi + 8`) crossed units: GAME-014 claimed `004c7eb0`, GAME-015's first item | `owns_data`: an address past a range's last item belongs to the unit only if no other unit's range holds it | `test_bss_slack_never_reaches_the_next_unit` |

**The include rule.** The ticket proposed "after the top include block".
That puts `frameloop_shared.hpp` at line 41 and `cmd_shared.hpp` at line 26.
Both files have a block of namespace declarations there, followed by more
includes that the private header's types can need. "After the last include
before the first use" is the top block whenever all the includes come first.
It reproduces #477's hand-placed positions in `frameloop.cpp`, `entity.cpp`
and `cmd.cpp`.

**Replay.** I relocated GAME-004, 006, 009, 014, 015 and 016 on `4fe92113`
(main before #477) with the fixed tool and no hand edits. GAME-014 went
before GAME-015, the order that failed before:
- no bogus declaration in `entity_config_shared.hpp`;
- `004c7eb0` goes to GAME-015's `lava_erosion.cpp`;
- the private includes sit where #477 put them by hand;
- `link_order.py check` passes, and the tree builds natively and passes all
  35 ctest tests.

The remaining differences from #477's tree come from the reversed
GAME-014/015 order: declaration order in two private headers, Meson order,
and source line numbers in the hash records. The one exception is
`entity_config.cpp`, which now includes its private header after the top
block instead of #477's line 1733. That tree compiled too.

## Proof: GAME-007 into `src/game/placement.cpp`

GAME-007 has no name. Its stem comes from its unit note, "2D point/segment
distance and placement-search helpers". Sources: `level.cpp`, `cmd.cpp`,
`entity.cpp`, `util.cpp`, `gods98/record.cpp`, `gods98/simtick.cpp` and
`gods98/view.cpp`.

- **Namespace members.** The 7 members of `unported_ledger_stubs` move into
  3 reopened blocks. That includes a forward prototype; the definitions go
  in address order.
- **Private headers.** `level_shared.hpp` declares two of them inside
  `namespace unported_ledger_stubs { }`: one used by the rest of `level.cpp`,
  one exported from it. New private headers: `util_shared.hpp`. Extended:
  `cmd_shared.hpp`, `level_shared.hpp`, `record_shared.hpp`,
  `simtick_shared.hpp`.
- **Hash record.** `docs/coverage/relocations/GAME-007.csv` has 30
  functions, 3 globals, 6 closure items and 4 shared types or macros. 18
  heads lose `static`, all recorded; every body agrees.
- **Owned globals.** GAME-007 owns 3 globals, not 4: `004b8a54` sits in its
  slack but is GAME-008's only `.bss` item, and the clamp now leaves it
  there.
- **Link-order keys.** Seven legacy keys change to the lower median of their
  remaining markers. `simtick.cpp` moves furthest, from `004291a0` to
  `00449c00`.
- **Gates:**
  - static check and Meson parity: OK;
  - native build and ctest: 35/35;
  - `make reccmp-check` (#479's warning gate included): passes after the
    baseline refresh below;
  - map check: OK;
  - `make match-progress` and the freshness check: fresh.

### Layout metrics

`binary-progress.json`, `main` (`d4af3221`) against this change:

| Measure | Before | After |
| --- | ---: | ---: |
| Pair inversions, full | 513,793 | 493,667 |
| Pair inversions, game-only | 499,970 | 479,964 |
| Adjacent inversions, full | 843 | 832 |
| Unit-order inversions, pairs | 3,585 | 3,533 |
| Unit-order inversions, adjacent | 88 | 88 |
| Unit contiguity | 70.05% (152 units) | 70.51% (153 units) |
| Exact-function code bytes | 44,551 | 44,551 |
| Mean score, full / game-only | 55.15% / 50.99% | 55.16% / 51.00% |
| `.text` identical at original addresses | 18,285 B | 18,337 B |
| `.rdata` identical at original addresses | 1,423 B | 1,482 B |
| `.data` identical at original addresses | 12,774 B | 12,864 B |

### Every changed function, with its classifier label

The changed listings were compared instruction by instruction with a report
of `main`'s tree whose digests equal `main`'s (2,378 of 2,378). None of
these functions' source changes.

| Address | Function | File | Score | Label | Change |
| --- | --- | --- | --- | --- | --- |
| `0x004512d0` | `terrain_cell_normal_sample` | `level.cpp` | 13.27 → 11.34 (**drop**) | `drift-review`, **−1, owner review** | commutative `fadd` operands swap; a `lea` folds into a displacement |
| `0x0040ff50` | `scene_object_list_apply_float_callback` | `view.cpp` | 75.00 → 87.50 | — (listing unchanged) | score only |
| `0x004388b0` | `entity_action_kind_prerequisite_check` | `entity.cpp` | 14.49 → 18.12 | `drift-reordered`, +0 | instruction order |
| `0x00444680` | `entity_step_movement_vector_resolve` | `entity.cpp` | 4.88 → 4.89 | `drift-review`, **−1, owner review** | one `fxch` fewer |
| `0x00446610` | `ledger_stub_00446610_…` | `entity.cpp` | 12.07 → 12.27 | `drift-review`, **−4, owner review** | x87 schedule; a spill and reload removed |
| `0x0044e0c0` | `terrain_create_retail` | `level.cpp` | 11.44 → 11.73 | `drift-review`, **−2, owner review** | a spill and reload removed |
| `0x00471370` | `cmd_place_ray_candidate_check` | `cmd.cpp` | 15.76 → 15.85 | `drift-review`, **−1, owner review** | one `fxch` fewer |
| `0x004078c0` | `ledger_stub_004078c0_…` | `entity.cpp` | unchanged | `drift-review`, +0 | a pushed literal is now named `(string)` |
| `0x0042c5b0` | `terrain_cell_byte_state_classify_current` | `level.cpp` | unchanged | `drift-review`, +0 | commutative `imul` operands swap |

All of these are in the files the relocation edited. Functions that move
out of a file renumber the rest, and the compiler schedules them a little
differently: the renumbering drift ADR 0006 rule 2 expects. The baseline is
refreshed with the one drop and the six improvements. The five
instruction-count changes are flagged for the owner.

## Survey: before and after

`relocate_unit.py --survey` on `main` (`d4af3221`, before this PR's
relocation). "Before" is `main`'s tool, "after" is this PR's. A unit lists
only the blockers of the first stage that fails. The "after" column counts
GAME-007, which this PR then relocates.

| Construct | Units blocked, before | After |
| --- | ---: | ---: |
| Named namespace | 31 | **0** |
| `#if` block | 13 | 16 |
| Macro `#undef`'d or redefined | 4 | 8 |
| Anonymous-namespace member that both sides need | 0 | 6 |
| Several declarators shared | 1 | 3 |
| Verbatim check failed (the `#if`-group bug above) | 1 | 0 |
| Private header needs an unshared static | 0 | 1 |
| Type and variable in one declaration | 0 | 1 |
| Backend-only source | 1 | 1 |
| **Units that relocate as they are** | **18 of 59** | **33 of 59** |

- **Newly relocatable (15):** GAME-007, 008, 010, 012, 013, 019, 025, 026,
  028, 029, 035, 039, 042; G98-016, 028.
- **Lost:** none.
- **Stems:** three need a `_unit` variant, which the tool picks: G98-013,
  G98-016, GAME-029.
- **Namespace-blocked units still blocked (17)**, by what now stops them:
  - **`#if` blocks and COM-call macros defined per compiler**: GAME-020, 021,
    027, 037; G98-003, 011, 025, 030, 031;
  - **anonymous-namespace members both sides need**: GAME-018, GAME-050,
    G98-002, G98-015, G98-020. Giving one external linkage means moving it
    out of its `namespace { }`, a structural edit the tool does not make;
  - **several declarators**: GAME-011 (an unannotated overload of
    `scene_rotation_flags_apply`), G98-009 (`static int s_sound_on,
    s_sound_active;`);
  - **a type and a variable in one declaration, both sides need it**:
    GAME-005 (`effects.cpp`'s `ScreenEffectTickRecord`).

For the wave tickets:
- **R1:**
  - GAME-007 is relocated here;
  - GAME-008, 012 and 013 relocate as they are now;
  - GAME-005 and GAME-011 need one-line source fixes in their own commits:
    split the declaration; annotate or rename the overload.
- **The next tooling lever** is sharing an anonymous-namespace member
  (6 units). After that come conditionally defined COM-call macros
  (`VIEW_COM_CALL` and the like), which block most of the `#if` cases.
