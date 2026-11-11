# Plan: relocating units blocked by `#if` blocks and redefined macros (#490)

Design only. Part of [#388](388-original-link-order.md) (relocation waves
#443–#446). Inputs:

- [ADR 0005](../adr/0005-original-shape-c-units.md): entropy rule 1, zero
  drift where it is free.
- [ADR 0006](../adr/0006-original-link-order.md): decision 3 (relocation is
  verbatim), decision 4 (no shared header edits) and entropy rules 2–3.
- Findings [523](../findings/523-relocate-unit-tool-and-advisor-relocation.md),
  [527](../findings/527-relocation-wave-r1-unblocked-units.md),
  [530](../findings/530-relocating-named-namespace-members.md) and
  [531](../findings/531-relocation-blocker-source-fixes.md) (the tool, its
  survey and its refusals).
- Findings [506](../findings/506-original-shape-pilot-priorities.md) and
  [514](../findings/514-bezier-unit-original-shape.md) (declaration-count
  drift).

The measurements are in [finding 539](../findings/539-conditional-relocation-blockers.md).
Everything was measured on `main` at `970a1822` (after #489). Scratch edits
were scored and then discarded; this change touches no file in `tools/` or
`src/`.

## Summary

- **17 distinct units, not 24.** `--survey` counts 16 units blocked by `#if`
  blocks and 8 by macros, but 7 units are in both lists: GAME-020, G98-003,
  010, 011, 014, 025 and 030. Only GAME-037 is blocked by macros alone.
- **They reduce to six patterns.** Two of them cover most cases:
  - **P1, split naked heads (15 functions in 3 units).** `#if` /
    `__declspec(naked)` / `#endif`, then one head and a body whose own `#if`
    picks `__asm` or C.
  - **P2, alternative macro definitions (8 groups).** One macro defined once
    per `#if`/`#elif`/`#else` branch, usually a calling convention. The
    tool's "redefined" refusal is a **false positive** here: it counts
    definitions in mutually exclusive branches as redefinitions.
- **Source reshapes can be zero drift by construction, and this was
  measured.** Each scratch reshape below left all of these unchanged:
  - the file's preprocessed token stream under the native, matching and
    MinGW reference configurations;
  - the matching build's object file: code, data, relocations and the symbol
    table.

  The reshapes were the 15 split heads, 3 hoists of branch-local helpers,
  one annotation move and GAME-037's macro rename. A negative control (one
  unused prototype added) is caught by both checks.
- **Recommendation:**
  - **B (reshape the source)** where a token-identical reshape exists: P1,
    the mixed groups of P3, P4, and P5's region boundaries.
  - **A (teach the tool)** to move a whole top-level conditional group
    verbatim (P2, P3), plus two small additions: guarded private-header
    declarations, and exporting a static that a header item needs.
  - **Not #389 first.** #389 is blocked by #388, is not zero drift for
    naked-`__asm` functions that already score 100%, and would edit shared
    headers.
- **Outcome.** With conditionals resolved in a scratch tree, 9 of the 17
  units plan cleanly. 6 more then stop only at #485's constructs: anonymous
  namespaces, several declarators, or an overload. GAME-017 stops at
  a static the private header needs, which ticket T2 adds. G98-012 stays
  blocked by a different construct (out-of-line class members, `movie.cpp`).
- **One decision is the owner's:** whether to accept *reshape commits*, a
  hand edit before a relocation that is proved token-identical. A proposed
  ADR 0006 addendum is at the end.

## How the tool refuses today

`tools/cxx_items.py` splits a file into top-level items and tracks `#if`
groups. `tools/relocate_unit.py` refuses in three places:

1. **An annotation that attaches to no item** and is followed by a `#`
   line. `relocate_unit.py` reports it as "inside an `#if` block that is not
   a group of whole alternative definitions" (`_plan_source`). This catches
   P1, because the scanner records an item that has a directive in its head
   with a problem and does not attach the annotation. It also catches the
   mixed groups of P3, which `_merge_groups` does not merge, and G98-012's
   annotation that sits above a macro group.
2. **Any involved item with `cond > 0`**: "inside an `#if` block". An item
   is involved if it moves, goes into the private header or is exported.
   The one exception is a merged group whose every branch is exactly one
   definition of the same function (`_merge_groups`). Advisor's vector
   helpers are that shape.
3. **A macro for which `_redefined` is true**: "macro X is `#undef`'d or
   redefined in the file". It is true if the file has more than one
   `#define X` or any `#undef X`, regardless of where they are. Alternatives
   in exclusive branches and an `#undef` after the last use both trip it.

`--survey` reports only the first stage that fails. Several units therefore
have blockers behind their conditionals that the survey does not show (see
[What is behind the conditionals](#what-is-behind-the-conditionals)).

## Survey: every blocking case

`python3 tools/relocate_unit.py --survey` on `970a1822`: 20 of 48 units
relocate as they are. `#if` block: 16 units; macro redefined or `#undef`'d:
8 units. Each unit was then run with `--dry-run` (unnamed units with a
scratch `--stem`) to list every refusal site. One row per construct; line
numbers are `main` at `970a1822`.

Guard key:
- **MSVC-x86**: `defined(_MSC_VER) && defined(_M_IX86)`, the matching build.
- **non-VC5**: `!defined(_MSC_VER) || _MSC_VER >= 1200 || !defined(_M_IX86)`.
- **CC**: `#if _MSC_VER` / `#elif __i386__ && (GCC || clang)` / `#else`.
  These select a calling convention.
- **64-bit host**: `_WIN64 || __x86_64__ || __aarch64__`.
- **winapi**: `LEGORRR_BACKEND_WINAPI`.

| # | Unit(s) | File:line | Guard | Shape | Contains | Pattern |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | GAME-043 | `game/entity.cpp:3112`, `3148`, `3258`, `3286`, `3328`, `9640`, `9717`, `9756`, `9781`, `9867`, `9896`, `9925`, `9954` | MSVC-x86 | split head, shared body; the body's `#if` picks `__asm` or C. `3148` (`0x0046a120`) spells the `#else` as `#endif` + `#if !(MSVC-x86)` | 13 functions: `0x00469a90`, `0x0046a120`, `0x00469ac0`, `0x00469af0`, `0x0046a0c0`, `0x00469f40`, `0x00469b20`, `0x00469f60`, `0x0046a0f0`, `0x00469b50`, `0x00469b80`, `0x00469bb0`, `0x00469be0` | P1 |
| 2 | GAME-027 | `game/select.cpp:302` | MSVC-x86 | split head, shared body | `selection_active_entity_get` (`0x004525c0`) | P1 |
| 3 | GAME-050 | `gods98/record.cpp:322` | MSVC-x86 | split head, shared body | `entity_velocity_state_record_get` (`0x004712c0`) | P1 |
| 4 | GAME-002 | `game/cmd.cpp:1703`–`1900` | MSVC-x86 / else | whole alternatives; the `#else` branch also defines two helper macros and `#undef`s them | `cmd_registry_init_target` (`0x00401cf0`), naked `__asm` / C; `CMD_TASK_NAME_SET`, `CMD_PRIORITY_NAME_SET` | P3a |
| 5 | GAME-021 | `game/entity.cpp:7868`–`8027` | MSVC-x86 / else | whole alternatives; the MSVC branch also defines data | `entity_cell_type_speed_multiplier_resolve_current` (`0x00433180`); `static const char s_entity_rock_fall_in_name[]` | P3a |
| 6 | GAME-017 | `crt/crt.cpp:613`–`642` | MSVC-x86 / else | whole alternatives; the MSVC branch also declares a function | `float_to_long_x87` (`0x00417430`); `extern "C" int __cdecl _ftol(void);` at `:615` | P3a |
| 7 | GAME-017, GAME-032 | `crt/crt.cpp:615`, `:996`–`998` | MSVC-x86 | declaration only (the second is #478's split-out `#if` block) | `_ftol`, which the moved `__asm` calls | P3b |
| 8 | GAME-027 | `game/cmd.cpp:4360`–`4362` | non-VC5 | data, single branch | `static uint32_t s_cmd_rand_state` (the LCG of `cmd_rand15`; VC5 uses LIBC `rand`) | P3b |
| 9 | G98-031 | `gods98/model.cpp:20`–`24` | `_MSC_VER` / else | macro / `#include` alternatives | `#define strcasecmp _stricmp` / `#include <strings.h>` | P3b |
| 10 | G98-030 | `gods98/resource.cpp:3861`–`3865` | 64-bit host | data, single branch, shared with remaining port code (`:4137`–`4164`) | `static void* s_fixed_slot_resources[]`, a side table of host pointers | P3b |
| 11 | G98-030 | `gods98/resource.cpp:3867`–`3872`; `#undef` `:4164` | 64-bit host / else | macro alternatives, `#undef` after last use | `fixed_record_slot_pointer_get(slot)` | P2 |
| 12 | G98-010, G98-011 | `gods98/surface.cpp:20`–`26`; `#undef` `:6214` (end of file) | CC | macro alternatives | `SURFACE_COM_CALL` | P2 |
| 13 | G98-003 | `gods98/scene_resources.cpp:12`–`18`; `#undef` `:1041` (last line) | CC | macro alternatives | `SCENE_RESOURCE_COM_CALL` | P2 |
| 14 | G98-025 | `gods98/view.cpp:26`–`32` | CC | macro alternatives, no `#undef` | `VIEW_COM_CALL` | P2 |
| 15 | G98-014 | `game/savegame.cpp:27`–`33` | winapi / else | macro alternatives with an `#include` in each branch, no `#undef` | `LRR_MKDIR`: `<direct.h>` `_mkdir` / `<sys/stat.h>` `mkdir` | P2 |
| 16 | G98-012 | `crt/crt.cpp:931`–`951` | CC | the annotation sits above a macro group; the macro is used in the function's head and `#undef`'d after it | `LEGORRR_CRT_STDCALL`; `bit_count32_by_position` (`0x0047e7e0`) | P2 (+ annotation placement) |
| 17 | GAME-020 | `game/frontend.cpp:70`–`79`; `#undef` `:162` | winapi / else | macro alternatives; the winapi branch also holds a `#pragma data_seg` and a padding array | `MENU_ACTION_TABLE_ALLOC`; `s_menu_action_table_gap[0x3aad0]` (scoring-only storage, ADR 0006 decision 5) | P6 (P2 + storage) |
| 18 | GAME-037 | `game/mission.cpp:747`–`754`; `#undef` `:994`–`997` | none | macro region: four unconditional macros that shadow same-named `static` functions (`:646`–`668`) for one function, then `#undef` | `mission_blob_{float,word}_{get,set}`; used by `0x00460360` | P4 |
| 19 | G98-010 | `gods98/surface.cpp:3996`–`5092`, nested in `3732`–`5443`; forward declarations `:1352`–`1357` and `:1358`–`1360` | `#if MINIWIN` … `#elif` winapi → `#if _WIN32 && !_WIN64` | nested backend region; the winapi branch's original code and its closure | `windowed_render_surface_blit_retail` (`0x0047cc50`), `render_surface_present` (`0x0047c8d0`), globals `0x0076acbc`/`c0`/`c4`; closure: 4 DirectDraw struct types, `s_iid_direct_draw4`, `surface_com_vtable`/`release`, `surface_ddraw_api_get`, `WinapiRendererState` and its static, `winapi_renderer_active`, `_state_release`, `_clipper_attach`, `winapi_rm_device_create`, `directdraw_renderer_init_winapi`, `s_surface_description_interface` | P5 |
| 20 | GAME-043 (hidden behind 1) | `game/entity.cpp:5607`–`5615`; `#undef` `:5840`–`5841` | MSVC-x86 / else | macro alternatives; the else branch calls file statics | `ENTITY_STATS_FLOAT_READ`, `ENTITY_STATS_WORD_READ` → `stats_rec_float`, `stats_rec_word` | P2 (+ T2) |

Row 20 is not in the survey. It appears once rows 1–3 are reshaped: then
GAME-043 stops at these macros, and at "the private header needs
`stats_rec_word`, a static that is not shared".

Inline `__asm` inside a body is **not** a blocker. A function whose
`#if`/`#else` sits wholly inside its braces scans as one item and moves
today, for example `selection_active_count_reset` (`0x00452600`) and
`entity_velocity_slot_value_get` (`0x004712f0`). The 15 P1 heads are every
split naked head in `src/`.

## Patterns

| Pattern | Shape | Cases | Units (as blocker) |
| --- | --- | ---: | --- |
| **P1** split naked head | `#if` / `__declspec(naked)` / `#endif`, one head, body with `#if` `__asm` / `#else` C | 15 functions | GAME-043 (13), GAME-027, GAME-050 |
| **P2** alternative macro definitions | one or two macros defined in each branch of one group; an `#undef` after the last use, if any | 8 groups (rows 11–16, 20, and 17's macro) | G98-003, 010, 011, 012, 014, 025, 030; GAME-043 (hidden); GAME-020 via P6 |
| **P3a** alternatives plus branch-local helpers | whole alternative definitions whose branch also holds a helper (macro, data, declaration) | 3 groups | GAME-002, GAME-017, GAME-021 |
| **P3b** a conditional declaration, datum or macro | single-branch or two-branch group around a non-function item | 4 groups (rows 7–10) | GAME-017, 027, 032; G98-030, 031 |
| **P4** shadowing macro region | unconditional macros that shadow same-named functions for one function, then `#undef` | 1 | GAME-037 |
| **P5** nested backend region | original code that exists only inside a branch of a larger backend group | 1 region, 5 annotated items, about 17 closure items | G98-010 |
| **P6** storage-deviation group | P2 whose branch also allocates scoring-only padding | 1 | GAME-020 |

## Evidence: is a reshape zero drift?

ADR 0005 rule 1 requires B's edits to show zero drift. Each reshape was
applied to a scratch copy and checked three ways:

1. **Scores:** `tools/match_function.py --source <file>` scores every
   annotated function of the file, before and after. The scratch file was
   copied into the worktree for one run and restored with `git checkout`;
   `--candidate` takes only one address.
2. **Objects:** the single-TU object (`--keep`), compared section by section
   outside `.debug$*`: raw bytes, relocations (by symbol name) and the
   symbol table. Debug sections differ, since they hold line numbers.
3. **Tokens:** the file's own preprocessing compared under three
   configurations: native (`LEGORRR_BACKEND_MINIWIN`, GCC x86-64), matching
   (`_MSC_VER=1100`, `_M_IX86`, `_WIN32`, `LEGORRR_BACKEND_WINAPI`) and MinGW
   reference (GCC `__i386__`, `_WIN32`, `LEGORRR_BACKEND_WINAPI`). `#include`
   lines are stripped from both sides, since the reshapes change only the
   file's own directives. That also sidesteps this host's missing SDL3
   headers. On the first run (`504f5ef6`), `crt.cpp` was also compared fully preprocessed with the native
   build's flags: identical.

| Reshape (scratch, not committed) | File | Functions scored | Score changes | Object (non-debug) | Tokens native / matching / MinGW |
| --- | --- | ---: | ---: | --- | --- |
| P1: 13 split heads → whole alternatives, and P3a: hoist GAME-021's string into its own `#if` before the annotation | `game/entity.cpp` | 286 | 0 | identical, 849 sections | same / same / same |
| P1: 1 split head | `gods98/record.cpp` | 91 | 0 | identical | same / same / same |
| P1: 1 split head | `game/select.cpp` | 32 | 0 | identical | same / same / same |
| P3a: hoist GAME-002's helper macros out of the `#else` (defined before the annotation, `#undef` after `#endif`) | `game/cmd.cpp` | 110 | 0 | identical | same / same / same |
| P3a: hoist GAME-017's `_ftol` declaration (as #478 did for `0x0045b930`), and move G98-012's annotation onto its definition | `crt/crt.cpp` | 327 | 0 | identical | same / same / same |
| P4: rename the four region macros to `…_at` (67 occurrences) and drop their `#undef`s | `game/mission.cpp` | 25 | 0 | identical | same / same / same |
| **Control:** one unused prototype after the includes | `game/select.cpp` | 32 | 0 | **differs**: same code bytes, but relocations and symbol table renumbered (`$SG…`) | **differs** |

The control matters. Finding 506's declaration-count effect shows up first
as renumbered internal symbols, before it changes any score. The object
check catches it, so "identical" above means that MSVC's symbol numbering is
unchanged too, not only that the scores are. Identical tokens are
sufficient: the compiler cannot tell the files apart.

What the reshape looks like (P1):

```c
// before                                   // after
// FUNCTION: LEGORR 0x00469ac0              // FUNCTION: LEGORR 0x00469ac0
#if defined(_MSC_VER) && defined(_M_IX86)   #if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked)                           __declspec(naked)
#endif                                      uint32_t entity_indexed_record_word_at_b0_get(int group, int index, int slot)
uint32_t entity_indexed_record_word_at_b0_get(int group, int index, int slot)
{                                           {
#if defined(_MSC_VER) && defined(_M_IX86)       __asm { … }
    __asm { … }                             }
#else                                       #else
    return …;                               uint32_t entity_indexed_record_word_at_b0_get(int group, int index, int slot)
#endif                                      {
}                                               return …;
                                            }
                                            #endif
```

The result is the shape `_merge_groups` already accepts. With rows 1–6 and
16's annotation reshaped in a scratch tree, the dry-runs move on: GAME-002
→ overload `cmd_create_move` (#485); GAME-017 and GAME-032 → `_ftol` (P3b);
GAME-021 → `s_entity_rock_fall_in_name` (P3b, now a one-item group);
GAME-027 → `s_cmd_rand_state` (P3b); GAME-043 → row 20; GAME-050 → anonymous
namespace (#485); G98-012 → `LEGORRR_CRT_STDCALL` (P2).

## What is behind the conditionals

To size what the conditional work unblocks, the dry-runs were repeated on a
scratch tree that takes the reshape tree and then:
- resolves every evaluable `#if` to the matching build's branch, a tiny
  `unifdef`;
- drops `#undef` lines.

This is a probe, not a proposal: it removes the native code.

| Plans cleanly (9) | Next blocker, not a conditional (8) |
| --- | --- |
| GAME-027, GAME-037, GAME-043, G98-010, G98-011, G98-014, G98-025, G98-030, G98-031 | Anonymous namespace (#485): GAME-020, GAME-021, GAME-050, G98-003. Several declarators (#485): GAME-032 (`simtick.cpp:351`). Overload (#485, PR #493): GAME-002 (`cmd_create_move`). The private header needs a static that is not shared: GAME-017 (`s_fe_flow_*`, `frontend.cpp`). Out-of-line class members: G98-012 (`AviPlaybackState::`, `movie.cpp:619`…) |

The probe's real equivalent, the tool support in T1, may still find
verify-stage problems, so these are upper bounds.

## Options per pattern

The three options:
- **A**: teach the tool.
- **B**: a separate, non-verbatim reshape commit before relocation.
- **C**: do #389's collapse first.

These hold for all patterns:

- **Verbatim and hashing.** B leaves relocation verbatim. The relocation
  hashes the reshaped text, and the reshape commit is audited by the token
  and object checks above. A moves text byte for byte, but its generated
  lines (guards around header declarations) are new text. Like today's
  generated declarations, they are checked by the compiler, not by the
  hash.
- **#389.** It is `blocked-by: #388`. Collapsing a dual path is a
  behaviour or codegen decision per function, not a mechanical move:
  - 13 of the 15 P1 functions already score 100% with `__asm`;
  - the C paths are the port's;
  - "original-shaped code" for a naked thunk means C that MSVC 5.0
    compiles to the same bytes, which has to be found and measured per
    function.

  C cannot be zero drift in general, and it would serialise relocation
  behind the largest open refactor. It removes P1 and P3a eventually, and
  is not a prerequisite for either.
- **Native and winapi.** Everything recommended here keeps every build's
  token stream unchanged (B) or moves platform branches whole (A), so the
  native port and the reference build compile the same programs as before.

### P1 split naked head: **B**

- **A** would let a directive group sit inside a function head when it
  holds only `__declspec(...)` tokens. The item then moves whole. But
  `_head_decl` would also have to write a declaration from such a head when
  the function is shared, and `cxx_items` would need a head grammar for
  directives. This is new scanner surface for one shape, which exists only
  in these 15 functions.
- **B** is measured above: zero drift in every build. It is about 110
  changed lines, mechanical, scriptable, and reviewable by the checks rather
  than by reading.
- **Cost of B:** the head is duplicated in both branches until #389
  collapses them, and a signature change then needs two edits.
- **C:** see above.
- **Choose B.**

### P2 alternative macro definitions: **A**

- **B** has no zero-drift option that keeps the platform split. Making a
  calling-convention macro unconditional breaks native. Moving it to a
  shared header is forbidden by ADR 0006 decision 4: it changes every
  includer's text and invites drift. Renaming per unit does not remove the
  alternatives.
- **A** is small and verbatim. The tool already merges one shape of whole
  group. It generalises to "a top-level group whose items form one cluster
  moves whole, verbatim", and fixes `_redefined`:
  - definitions in exclusive branches of one group are alternatives, not
    redefinitions;
  - an `#undef` after the macro's last use in the file stays in the source
    as a harmless no-op, and is reported.

  Where the group goes:
  - into the unit file when only moved code uses it;
  - into the private header when both sides use it and the group holds
    only header-safe items (macros, types, prototypes, `#include`s).

  Groups containing `#include`s (rows 9, 15) move as text; the private
  header then includes what the branch included.
- **C** (#389) would put calling conventions in a shared compatibility
  header. That is right eventually, but it is a shared-header edit with
  drift, not a relocation prerequisite.
- **Choose A.** P2 covers 8 groups across 9 units.

### P3a alternatives plus branch-local helpers: **B** (hoist)

- **A** would have to decide which helper belongs to which branch, and
  merge a group's closure. `_ftol` (row 6) shows the risk: GAME-032's code
  declares it too, so the clusters would merge and pull GAME-017's whole
  function group along.
- **B** hoists each helper into its own `#if` block before the annotation,
  or after the `#endif` for `#undef`s. #478 set the precedent with
  `0x0045b930`. It is measured zero drift for all three, and is 3–6 lines
  each.
- **Choose B.** The group is then whole alternatives (handled today), and
  the hoisted helper is P3b.

### P3b a conditional declaration, datum or macro: **A**

- This is the same feature as P2: a one-item or one-cluster group moves
  whole.
- Two sub-cases need small additions:
  - **Guarded declarations (rows 7, 10, and P5).** A conditional static
    that both sides need gets external linkage as usual. Its private-header
    declaration is wrapped in the group's own condition, copied verbatim.
    Allow it only for an item in the first branch of a top-level group, or
    in a single-branch group. Otherwise refuse: the condition of an `#elif`
    branch would have to be synthesised.
  - **Exporting a static a header item needs (T2).** This is not specific
    to `#if`. Today "the private header needs X, a static that is not
    shared" refuses, where the existing static rule could export X (strip
    `static`, declare it, record it). It blocks G98-030 (row 10, via the
    row 11 macro), GAME-043 (row 20's native branch) and GAME-017 (the
    `s_fe_flow_*` statics behind its conditionals).
- **B** (making them unconditional) is possible for row 8 and the `_ftol`
  declarations, but it changes native's token stream (a declaration or an
  unused static on the other platform). It is not zero drift by
  construction, so prefer A.
- **Choose A.**

### P4 shadowing macro region: **B** (rename)

- **A** would need macro-scope analysis: which items lie between a
  `#define` and its `#undef`, and a cluster rule that keeps the shadowed
  `static` functions (used after the region, at `:1482` and `:2082`) apart
  from the macros. All that for one unit.
- **B** renames the four macros (`mission_blob_float_get` →
  `mission_blob_float_get_at`, and so on) inside the region and drops the
  `#undef`s. Measured: tokens and object identical.
- It edits one function body (67 occurrences), so the relocation's hash of
  that body records the renamed text. The rename is its own commit.
- **Choose B.**

### P5 nested backend region: **B** (region split) **then A**, last

The two original functions and three globals are in the winapi branch
(`#elif LEGORRR_BACKEND_WINAPI`, `:3991`), inside `#if _WIN32 &&
!_WIN64` (`:3996`–`5092`), next to miniwin and fallback branches full of
other code. The scanner marks the outer group nested and merges nothing.

- **A** (a "guard stack") would compute the effective condition of nested
  branches, including the negations of earlier `#if`/`#elif` arms, and
  write a generated wrapper around moved items and their header
  declarations. That is the most complex tool feature here, for one unit.
- **B** closes and reopens the groups around G98-010's items and closure,
  with conditions that are equivalent in every configuration. For example,
  end the chain before `:4138` and reopen it as
  `#if !defined(LEGORRR_BACKEND_MINIWIN) && defined(LEGORRR_BACKEND_WINAPI)
  && defined(_WIN32) && !defined(_WIN64)` for the moved items. The items
  then sit in top-level single-branch groups that A (P2/P3b) handles.

  Equivalence is checked by the token check in three configurations, plus
  the object check. The diff is larger than the others: tens of lines of
  directives, no code. The closure's shared state (`WinapiRendererState`,
  the COM helpers) then needs guarded header declarations.
- **C** would move the Win32 DirectDraw loader and its COM plumbing to the
  platform layer, and write the two original functions once against the
  seam. That is the right end state (#389, ADR 0004), but not zero drift:
  `0x0047cc50` scores 88.64% today.
- **Choose B then A**, as the last ticket, so the reshape is designed
  against a finished T1. If the owner finds the region split too invasive,
  the fallback is to leave G98-010 like G98-005 (Main.c), a backend-only
  unit, until #389.

### P6 storage-deviation group: **A**, measured

`MENU_ACTION_TABLE_ALLOC`'s winapi branch also allocates
`s_menu_action_table_gap` in `.data$zzzzzz`, a scoring-only storage
deviation that ADR 0006 decision 5 says goes. Under A the whole group moves
with the label table into the unit file, which moves the padding to another
object.

- The relocation PR must measure that with the full gate. Single-TU scores
  cannot see `.data` placement.
- If any score changes beyond rule 2's drift, retire the padding first, in
  its own PR, under decision 5.
- GAME-020 also waits for #485 (anonymous namespace), so this is not on the
  critical path.

## Recommendation and tickets

**Order:**
1. #485 (PR #493) lands first: T1 and T2 edit the same tool.
2. T0, B1 and B2 touch no tool file shared with #485, so they can go in
   parallel with it.

| Ticket | What | Size | Depends on | Unblocks or moves on |
| --- | --- | --- | --- | --- |
| **T0** `reshape_check` | Tool: given a file and its reshaped version, compare the file's own token stream under the native, matching and MinGW configurations, and the matching single-TU object outside debug sections (bytes, relocations, symbol table) via `match_function.py --keep`. Exit non-zero on any difference. Add fixture tests to `make tools-test`. This makes the method of this plan repeatable, and is the evidence a reshape PR attaches | S | — | evidence for B1, B2, B3 |
| **B1** reshape P1 + P3a + G98-012's annotation | One commit per file: `entity.cpp` (13 heads, GAME-021 hoist), `select.cpp`, `record.cpp`, `cmd.cpp` (GAME-002 hoist), `crt.cpp` (GAME-017 `_ftol` hoist; move `0x0047e7e0`'s annotation onto its definition). T0 output in the PR. No baseline refresh: zero drift | S | T0, the addendum | GAME-043 → row 20; GAME-027 → row 8; GAME-021 → P3b; GAME-017 → P3b; GAME-002 and GAME-050 → #485 |
| **B2** reshape P4 | `mission.cpp`: rename the region macros and drop their `#undef`s, in one commit | S | T0, the addendum | GAME-037 (relocates) |
| **T1** whole conditional groups | Tool: (1) a top-level, non-nested group whose items form one cluster (macro alternatives, a single conditional item, a group with `#include`s) is one item that moves whole and verbatim, to the unit file or, when header-safe, to the private header. (2) `_redefined` ignores alternatives in exclusive branches, and accepts an `#undef` after the last use (left in place, reported). (3) Guarded private-header declarations for a shared static in a single-branch group or a first branch. (4) Refuse anything else, naming the group. Fixture tests for each; the hash check covers a group's whole text. Proof relocation: G98-025 (only `VIEW_COM_CALL` blocks it), with the gate | M | #485 | G98-011, G98-014, G98-025, G98-031; with B1: GAME-027; G98-003, GAME-021, GAME-032 → #485 |
| **T2** export statics a header item needs | Tool: when a private-header item (macro, type, declaration) uses a `static` from the source, export it as the static rule already does, instead of refusing. Fixture tests | S | #485 (same file); can be T1's second commit | GAME-043, G98-030; GAME-017 (whatever is behind) |
| **W** relocation wave | Generated commits only, one per unit, under ADR 0006 rule 2: GAME-027, GAME-037, GAME-043, G98-011, G98-014, G98-030, G98-031, plus G98-025 if T1's proof did not take it. Then GAME-017 once T2 is in, and the #485-gated units (GAME-002, GAME-020 per P6, GAME-021, GAME-032, GAME-050, G98-003) as #485 lands | M | B1, B2, T1, T2 | 7 units, then about 7 more |
| **B3** P5 region split, then G98-010 | `surface.cpp`: close and reopen the backend groups around G98-010's items and closure (T0-checked, zero drift), then relocate G98-010 | M | T1, T2 | G98-010 |
| — | Not covered: G98-012's out-of-line `AviPlaybackState::` members (`movie.cpp`) | new issue | — | G98-012 |

Each ticket re-runs `--survey` first: line numbers move as other units
relocate.

## Proposed ADR 0006 addendum (for the owner; not accepted)

> **Status: proposed by #490, for the owner to accept or reject.** It amends
> ADR 0006 decision 3 and entropy rule 3.
>
> **Reshape commits.** A relocation may be preceded by a *reshape*: a hand
> edit to source that changes no build's input.
>
> 1. A reshape is accepted as zero drift (ADR 0005 entropy rule 1) only when
>    `tools/reshape_check.py` shows both of these for every file it touches:
>    - an identical token stream under the native, matching and MinGW
>      reference configurations;
>    - an identical matching-build object outside debug sections: bytes,
>      relocations and symbol table.
>
>    It refreshes no baseline. Its PR includes the check's output.
> 2. A reshape may only:
>    - split a conditional head into whole alternative definitions,
>      duplicating the head;
>    - hoist branch-local helpers out of an alternatives group;
>    - move annotations or comments;
>    - rename a file-local macro;
>    - close and reopen `#if` groups with equivalent conditions.
>
>    It does not change a contract, collapse a dual path (#389), or edit a
>    shared header.
> 3. Reshapes follow rule 3: one commit per file, never in the same commit
>    as a relocation, conversion or fix. They may share a PR with the
>    relocation they prepare.
> 4. The relocation tool moves a top-level conditional group whole and
>    verbatim. When it writes a private-header declaration for a
>    conditional item, it wraps it in that item's group condition, copied
>    verbatim, and records it in the relocation CSV like a `static` removal.

## Not done here

- No tool or source change. All edits were scratch copies, scored and
  discarded. `git status` was clean after each run.
- P5's region split and P6's padding move were not measured. Both need
  T1 to exist first: the split's boundaries depend on what T1 can move, and
  `.data` placement needs a full link.
- The probe's "plans cleanly" means `--dry-run` reaches a plan with the
  conditionals removed. It does not prove the real relocation passes the
  tool's verify stage or the gate.
