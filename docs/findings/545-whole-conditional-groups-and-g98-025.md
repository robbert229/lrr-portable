# Finding 545 — Whole conditional groups in the relocator, and G98-025

Issue #501 covers tickets T1 and T2 of the #490 plan
([plan](../plans/490-relocation-conditional-code.md), [finding 539](539-conditional-relocation-blockers.md)),
under item 4 of the ADR 0006 addendum. It is part of #388.
`tools/relocate_unit.py` now moves a top-level `#if` group whole, writes
guarded private-header declarations, and exports a `static` that a
private-header item needs. The proof relocation is **G98-025**
(`gods98/com_object_tree.cpp`). Measured on `main` at `d6d7bc4f` (after
#507's reshapes B1 and B2, and #508's GAME-011, 018 and 022).

## Summary

1. **T1, whole groups.** A top-level, non-nested `#if` group whose items
   form one cluster now scans as one item that moves verbatim, from its
   `#if` line to its `#endif` line. Examples are calling-convention macro
   alternatives, a single conditional datum or declaration, and a group
   with `#include` lines. It goes to the unit file, or to the
   `<l>_for_<u>.hpp` header when both files use it. The hash check covers
   the group's whole text.
2. **The redefined-macro check** counts one definition per branch of a
   group as one definition. It accepts an `#undef` after the macro's last
   use, which stays in place and is reported in the plan's notes. An
   `#undef` before a later use, or a second definition, still refuses.
3. **Guarded declarations.** A shared `static` in a single-branch group, or
   in a group's first branch, is exported. Its declaration in the frozen
   #496 header pair is wrapped in the group's `#if` directive, copied
   verbatim. Each one is recorded as a `guarded-declaration` row, next to
   the `static` removal's own row (addendum item 4). A definition in a later
   branch refuses, because its condition would have to be synthesised.
4. **Refusals name the group.** Anything else refuses with the group's line
   and the reason: a function definition among other items, a nested
   group, an `#undef` or other directive, unrelated items, or an annotated
   item inside.
5. **T2.** A `static` that a private-header macro, type or declaration uses
   is exported by the static rule instead of refused. `inline` and
   anonymous-namespace items still refuse.
6. **Two latent defects surfaced** once G98-025 got past its conditionals.
   The survey reported the unit as relocatable, but the native build of the
   result failed. Both are fixed, with fixture tests:
   - the scanner found no name in `typedef R (CONV *Name)(...)` when `CONV`
     is a macro or `__attribute__((...))`. Such a typedef was then invisible
     to clustering. A nameless typedef now refuses;
   - an external item pinned to another unit (it carries another unit's
     annotation) that moved code used got no declaration when no header
     declared it. G98-025 calls `view.cpp`'s
     `unported_ledger_stubs::ledger_stub_00487ba0_…`, annotated
     `0x00487ba0`.
7. **Survey: 1 → 7 of the 23 unrelocated units relocate as they are**,
   plus 2 more after `--split-declarations`. This is on `main` with #507's
   reshapes. GAME-043,
   G98-003, 020, 025, 030 and 031 are unblocked, and GAME-021 and GAME-032
   need only `--split-declarations`.
8. **G98-025 relocated**, with 12 functions and 21 closure items. Its two
   calling-convention groups, `VIEW_COM_CALL` and `RESOURCE_TREE_STDCALL`,
   moved whole into the `for` headers. The gate passes with nothing below
   the baseline. Four listings change: two improve and two are reordered at
   equal scores. Every change is labelled below.

## Design

### What merges (`cxx_items._cluster_group`)

A group merges when all of these hold:

- it is top-level (not inside an item) and has no nested `#if`;
- it is not an include guard (`#ifndef X` / `#define X`);
- its members are only macros, `#include`s, types, prototypes and
  variables, and not all of them are `#include`s. All-`#include` groups
  keep their previous treatment: a unit file does not copy conditional
  includes;
- its non-`#include` members form one cluster. Macros, types and
  prototypes always count as one cluster: they can all go to the private
  header together, and the plan's P2 allows two macros per branch
  (GAME-043's `ENTITY_STATS_FLOAT_READ`/`_WORD_READ`). Data must connect to
  them, or to other data, by sharing or using a name. The `#include` lines
  come along.

The merged item keeps its members (`parts`, each with its branch index),
its `#if` directive (`condition`) and its `#endif` line. Its category is the
"largest" of its members (variable > prototype > type > macro). So the
existing rules decide where it goes:
- macros and types are file-local and go to the header when both files use
  them;
- a `static` datum is internal and gets exported;
- an `extern`-only group is a declaration.

Its uses include the identifiers in its `#if`/`#elif` conditions, so a group
whose condition tests a file-local macro carries that macro along.

Function definitions are deliberately excluded. Whole alternatives of one
function were already merged (`_merge_groups`). Anything else with a
function is P3a in the plan, a reshape (B1, PR #507), not a tool feature.

A group that does not merge records, on each member, the line of its
outermost group and the reason. The `#if block` refusal reports both, for
example:

```text
src/gods98/view.cpp:169 (…): inside an #if block: the group at line 169 cannot move whole
(it holds the function definition ddiscene_payload_owner_detach_call besides other items)
```

### Guarded declarations

The declaration is built from the member, not the group: its own head, with
`static` dropped. It is wrapped like this:

```c
#if defined(FEATURE) /* port only */      <- the group's #if directive, verbatim
extern int s_cond;
#endif /* FEATURE */                      <- the group's #endif line (single-branch groups);
                                             a plain #endif after a first branch
```

The `static` is removed from that member only, never from a macro body in
the same group.

A group split across the files this way gives the other file its data's
declaration, never its macros or types. If that file also uses one of the
group's macros or types, the relocation refuses and names it. The fix is to
hoist the data into its own group first (a B1-style reshape). The relocation CSV gets a `guarded-declaration` row: the
name, the source line, the header, a hash of the condition and of the
wrapped declaration, and the condition's first line. `_verify` checks that
the wrapped text is in the header it names.

### `_redefined`

The old check was true if a file had any `#undef X` or more than one
`#define X`. The new one:

- counts each group's definitions of `X` once if no branch defines it twice;
- refuses on a second definition anywhere else;
- accepts an `#undef X` only after the last use of `X` in the file. The use
  sites are identifier matches outside the `#undef` lines and `X`'s own
  `#define` lines.

An accepted `#undef` stays where it is. If the macro moved to the unit file,
it is a no-op. If it moved to the `for` header, it undefines the header's
macro at the same point as before.

## Survey before and after

`python3 tools/relocate_unit.py --survey`, on `main` `d6d7bc4f`, without and
with this change. Only rows whose result changed are shown:

| Unit | Before | After |
| --- | --- | --- |
| GAME-002 | `#if` ×6, macro ×6 | name clash with another file (behind #507's hoisted helper macros) |
| GAME-017 | `#if` ×2 | `#if` ×27, macro ×14: behind the `_ftol` hoist, T2 exports the `s_fe_flow_*` statics, and the closure then reaches `frontend.cpp`'s conditional code |
| GAME-021 | `#if` ×1 | ok after `--split-declarations` |
| GAME-027 | `#if` ×1 | name clash with another file: `s_queue_count` would get external linkage, but `info.cpp:83` defines one too |
| GAME-032 | `#if` ×2 | ok after `--split-declarations` |
| GAME-043 | `#if` ×4, header needs a static ×6, macro ×4 | **ok** (62 functions): row 20's two-macro group merges, and T2 exports `stats_rec_float`/`stats_rec_word` |
| G98-002 | `#if` ×3, macro ×3 | `#if` ×5, nested namespace ×1 (now behind its macro group) |
| G98-003 | `#if` ×3, macro ×3 | **ok** (22 functions) |
| G98-010 | `#if` ×26, macro ×3 | `#if` ×29 (P5: B3) |
| G98-011 | `#if` ×3, macro ×3 | `#if` ×6: its declarations need types inside `surface.cpp`'s nested backend region (P5: B3) |
| G98-012 | `#if` ×3, macro ×3 | `#if` ×10, out-of-line class member ×11 (`movie.cpp`; not covered by the plan) |
| G98-014 | `#if` ×2, macro ×2 | `#if` ×1 |
| G98-015 | `#if` ×3, macro ×3 | `#if` ×1 |
| G98-020 | `#if` ×3, macro ×3 | **ok** (21 functions) |
| G98-025 | `#if` ×3, macro ×3 | **ok** (12 functions), relocated here |
| G98-030 | `#if` ×2, header needs a static ×2, macro ×2 | **ok** (9 functions). Both features at once: the `fixed_record_slot_pointer_get` group goes whole to the `for` header. T2 exports the static it uses, `s_fixed_slot_resources`, which sits in a single-branch 64-bit-host group, so its declaration is guarded by that group's `#if`. The `#undef` after the last use stays (row 11 of the plan) |
| G98-031 | `#if` ×1 | **ok** (13 functions) |

Totals: 1 of 23 units relocates as it is before (GAME-037, unblocked by
#507's B2 alone), 7 after, plus GAME-021 and GAME-032 after
`--split-declarations`. Blocked by an `#if` block: 18 → 8. Macro redefined
or `#undef`'d: 13 → 2.

Of the three units that #507 and this ticket were to unblock together,
GAME-037 and GAME-043 now plan cleanly. GAME-027 stops at a name clash,
which is not a conditional. GAME-050 is unchanged: two anonymous-namespace
helpers are used before their definition in the unit file, which the tool
cannot forward-declare.

"Ok" means the plan passes the tool's verify stage. It does not compile
anything. Both defects in summary item 6 passed verify and failed only in
the native build. The wave that relocates these units has to build each
one.

## G98-025: the proof relocation

`tools/relocate_unit.py G98-025 --stem com_object_tree`. It moves 12
functions and 21 closure items from `gods98/view.cpp` and
`gods98/resource.cpp`; the body hashes are in
`docs/coverage/relocations/G98-025.csv`. Shared items:

- **`view_for_com_object_tree.hpp`** (both files): the whole `VIEW_COM_CALL`
  group, the pinned ledger stub's declaration in its namespace, and five
  statics plus one static function (static removed, recorded).
- **`resource_for_com_object_tree.hpp`** (both files): the whole
  `RESOURCE_TREE_STDCALL` group with its leading comment, five
  function-pointer typedefs that use it, and
  `s_resource_tree_child_iid`'s declaration.
- No `from` header: neither legacy file uses anything that moved.

### Gate (ADR 0006 rule 2)

| Check | Result |
| --- | --- |
| `make reccmp-check` (distrobox) | OK, nothing below the baseline; improved: `0x004882e0` 12.11 → 15.33, `0x00489910` 45.03 → 56.50 |
| Means | game-only 51.653% → **51.660%** (2,161 functions); full 56.339% → 56.345% (2,397) |
| `link_order.py check` / `check-map` | OK / OK |
| Native build + `ctest` | 36/36 |
| Meson parity | OK |
| `make match-progress` | function-order inversions, full: 610 → 603 adjacent, 292,291 → 289,242 pairs. Unit-order inversions: 66 → 65 adjacent, 3,238 → 3,222 pairs. Identical `.text` bytes at original addresses: 18,528 → 18,579 |

Every function whose listing digest changed, labelled by
`match_listings.classify`. Each was then re-measured with
`match_function.py` on `main` and on this branch: all reproduce in a
single-TU compile, so none comes from the link.

| Function | File | Score | Instructions | Label | Cause |
| --- | --- | ---: | ---: | --- | --- |
| `0x00489910` resource_tree_callback_count | `com_object_tree.cpp` (moved) | 45.03 → **56.50** | −10 | `drift-review` | the same code. The duplicated release-and-epilogue tail after the loop is now tail-merged with one `jmp`; 242 → 218 bytes |
| `0x004882e0` scene_node_transform_matrix_apply | `view.cpp` | 12.11 → **15.33** | +3 | `drift-review` | x87 operand order and stack scheduling re-rolled: `fld`/`fmul` operands swap, `fxch` depths change (finding 538) |
| `0x0049c7c0` wad_expand_payload | `resource.cpp` | 58.27 = | 0 | `drift-reordered` | `shr`/`and` register pair swapped |
| `0x0049cc70` wad_bitstream_output_initialize | `resource.cpp` | 26.09 = | 0 | `drift-reordered` | `shr`/`and` register pair swapped |

No function falls. The two `drift-review` changes are improvements, and
both change the instruction count, so they are flagged for the owner as
rule 2 asks.

## Limits

- **P1, P3a, P4 and P5 are not tool features** (plan: B1, B2, B3). Groups
  holding a function definition besides whole alternatives still refuse,
  now naming the group.
- **Guarded declarations need the first branch.** A shared static defined in
  an `#elif`/`#else` branch refuses, as the plan specifies.
- **"Ok" in the survey is not "builds".** See summary item 6. The
  relocation wave should build each unit before its gate.
- **Merged groups carry their `#include` lines.** A group with an
  `#include` that goes to the `for` header makes both files include that
  system header under the group's condition. That is the same text the
  legacy file had, but a new include for the unit file.

## Rebases

The relocation was regenerated, never rebased as text, each time `main`
moved: on `8e8d4da3`, `cd80158d` (#509), `c155edab` (#507) and `d6d7bc4f`
(#508). Each time the tool's plan was the same and the gate passed. On
`8e8d4da3`, `cd80158d` and `c155edab`, a fifth function also changed:
`0x004750b0` (`view.cpp`), `drift-reordered` at an equal score. After #508,
which moved other units out of `view.cpp`, it no longer changes.
`0x004882e0` started from 12.13% before #508 and from 12.11% after it.
