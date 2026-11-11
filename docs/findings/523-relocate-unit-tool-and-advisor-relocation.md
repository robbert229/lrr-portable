# Finding 523 — Verbatim unit relocation: `relocate_unit.py`, proved on Advisor.c (GAME-001)

Issue #440, stage S2a of the [#388 plan](../plans/388-original-link-order.md)
([ADR 0006](../adr/0006-original-link-order.md) decisions 3 and 4, entropy
rule 2). Measured on `main` at `93f939a8` (after #463–#467 and #470; the
link-order checks and listing digest format 4 of #465 included).

## Summary

1. **The tool.** `tools/relocate_unit.py <unit>` moves a unit's annotated
   functions and the globals it owns into `src/<lib>/<stem>.cpp`, verbatim,
   in original address order. It also does the following:
   - moves the file-static closure that only the moved code uses;
   - gives statics shared with the remaining code external linkage through a
     private `<file>_shared.hpp`;
   - wires the link-order list, Meson and `translation-units.csv`;
   - hashes every moved item before and after, into
     `docs/coverage/relocations/<unit>.csv`.

   It refuses a construct it cannot move safely and names the file and line.
   It works without a C++ parser: `tools/cxx_items.py` splits files into
   top-level items. `tools/README.md` documents both tools.
2. **GAME-001 relocates completely.**
   - 31 of 32 functions move. `0x00401c00` has no definition in `src/`.
   - 43 owned globals and 3 static helpers move.
   - The sources are five legacy files: `game/objective.cpp`,
     `gods98/view.cpp`, `crt/crt.cpp`, `gods98/engine.cpp` and
     `game/frontend.cpp`.
   - Every body hash agrees. 14 heads lose `static`, all recorded.
   - No shared header changes.
   - The native build, all 35 `ctest` tests and the matching build pass.
3. **Layout improves** (schema 3, `binary-progress.json`):

   | Measure | Before | After |
   | --- | ---: | ---: |
   | Pair inversions, full | 633,221 | 593,010 (−40,211) |
   | Unit-order pair inversions | 3,816 | 3,736 (−80) |
   | Unit contiguity | 66.82% | 67.28% |
4. **Scores drift as rule 2 expects.**
   - 3 functions fall below baseline and 1 rises. 7 rebuilt listing digests
     change, all with the same instruction count.
   - 4 are commutative x87 operand swaps in unchanged code, 2 are reorders
     and 1 is a register-allocation change.
   - The 19 listings that differ from `main` only in `name[i].field+N`
     operand names have equal format-4 digests, so they classify as `drift`.
   - One drop, `vector_scale_components_in_place`, is a real operand swap
     (100% → 91.30%) and needs owner review.
5. **Survey.** 21 of the 65 unrelocated units relocate as they are, and a
   22nd needs only `--stem`.
   - The largest blocker is a **named namespace**, in 31 units. Most are
     `unported_ledger_stubs`, which holds annotated ledger stubs.
   - The next is **`#if` blocks**, in 12 units.

## The tool

### What moves, and where it goes

The unit's functions are the `FUNCTION`/`STUB` annotations in its `.text`
range, in any linked legacy or tail file. The owned globals are the `GLOBAL`
annotations in its data ranges (`translation-unit-ranges.csv`).

The closure is computed per source file. Items that define a common name form
one symbol, for example a prototype and its definition. Starting from the
moved items, the tool takes the greatest set of file-local symbols whose every
user also moves: static functions and variables, forward declarations,
file-local types and macros. Reachability follows only items that move, so a
static used only by a function that stays does not count. An earlier draft
followed every reference. It exported eight `frontend.cpp` font helpers that
only `formatted_text_format_draw` uses, and that function stays.

A symbol that moved code and remaining code both need is handled by kind:

- A **static variable or function** loses `static`. An `extern` or prototype
  declaration goes into the private header.
- A **file-local type or macro** moves into that header.
- A symbol with **external linkage** that no visible header declares gets a
  declaration there.

Visibility comes from the transitive includes, resolved with each library's
include path. Both files include the private header after their own
includes. Heads that lose `static` are recorded in the hash file.

The new file's layout follows ADR 0006 decision 4:

1. Includes, then the private headers.
2. Types and macros.
3. The owned globals in address order, then the closure variables. A variable
   moves up to just before its first user.
4. The forward declarations the new order needs.
5. The unit's functions in address order.
6. The static helpers last, because they are port-only code.

Items that were adjacent in their source stay adjacent. Each item keeps the
comments directly above it.

**Includes.** The new file keeps the include list of the source that holds
the most moved functions, in order. The other sources' unconditional
includes follow. A spelling is changed only if it would resolve to a
different file from the new location. Conditional includes are not copied,
and the report lists them. For GAME-001 they are crt.cpp's platform headers
and one view.cpp include; the build confirms nothing needed them. The union
is large: 68 includes plus 3 private headers. Drift from it is accepted under
rule 2.

**Wiring.**
- The unit gets a `unit` entry at its original position
  (`link_order.insert_unit`).
- Every source the relocation edits keeps the key #465's static check
  (`link_order.py check`) requires: the lower median of its remaining
  `FUNCTION`/`LIBRARY` markers. `link_order.place_legacy` rewrites the key and
  moves the entry into address order. A legacy file left with no original
  function becomes a `tail` entry instead (`link_order.retire_legacy`).
  For GAME-001 four keys change: `frontend.cpp` `0041b9b0` → `0041b9c0`,
  `objective.cpp` `00452e70` → `00458580` and `view.cpp` `00473340` →
  `004734c0`, all in place, and `crt.cpp` `00491470` → `00491cc0`, which
  moves it after `crt/crt_heap.cpp` (`00491710`). The list passes the static
  check, and the map check passes on the rebuilt map.
- The new file is added to the Meson `static_library` list.
- `current_source` in `translation-units.csv` follows the moved functions.
- A pooled literal that a conversion annotated in its own `.c` file stays
  there, with a note. Four units hit this: `004b8a14` and others in
  `gods98/config.c`, `game/priorities.c` and `gods98/soundsys.c`.

### The verbatim check

Each moved item's text is hashed: SHA-256 after collapsing whitespace and line
endings. The hash covers the head and body, without the leading comments.

The tool then re-scans the generated text and finds each item again: by
annotation address, or by name for closure items. It requires the same hash,
or for a recorded `static` removal the expected head and an identical body.

It also requires every `FUNCTION`/`GLOBAL` annotation to appear exactly once
across the edited files. A mismatch refuses the whole relocation.

`docs/coverage/relocations/GAME-001.csv` holds the 88 rows for Advisor. Every
row agrees, and the 14 rows that differ in head are the recorded `static`
removals:

- 5 owned globals that remaining code also uses: `s_adv_actions`,
  `s_advisor_animation_names`, `s_adv_animation_slots`, `s_adv_scene_scale`
  and `s_advisor_state_word`;
- 9 statics that stay and that moved code uses: 8 hook and state words in
  `objective.cpp`, and `s_application_mode` in `engine.cpp`.

The classifier's own source index (`match_listings.source_index`) reads the
same marker-to-closing-brace text. So the moved functions show as unchanged
source (`drift…`), never `source-changed`.

**Idempotence.**
- A second run finds the unit listed, with no annotations left elsewhere,
  and does nothing.
- A partly relocated state is refused: the unit is listed, but annotations
  are still elsewhere.
- Rerunning on `main`'s tree reproduces the committed relocation byte for
  byte.

### Refusals

The scanner marks anything it cannot place, and the tool refuses to move such
an item:

- **`#if` blocks.** A definition inside `#if` is refused, with one exception:
  a group whose branches each hold one whole definition of the same function
  is moved as a unit. These are the naked-`__asm`/C pairs; Advisor's vector
  helpers are this shape. A directive inside a declaration head is refused,
  for example a conditional `__declspec(naked)`.
- **Definitions it cannot name or place:**
  - functions defined in headers;
  - qualified definitions (`ns::f`);
  - a macro or complex declarator before the name;
  - an annotation not attached to a definition;
  - an annotation on a prototype.
- **Scoping:**
  - members of a named namespace;
  - a moved function that uses a name only a namespace in its source
    defines;
  - an anonymous-namespace or `inline` static that both sides need.
- **Declarations it cannot share:**
  - a shared declaration of several declarators;
  - default arguments;
  - a macro that is `#undef`'d or redefined;
  - two items on one line.
- **Clashes:**
  - the same name moving from two files;
  - a static that would clash with an external definition elsewhere;
  - a stem that matches an existing file. `--stem` picks a variant.
- **Backend-only sources.**

`tools/tests/test_relocate_unit.py` covers each refusal on synthetic sources,
plus the closure, the private header, wiring, hashes, idempotence, a second
unit draining a file, and the survey.

## Proof on GAME-001 (Advisor.c)

| Source | Moved | Private header |
| --- | --- | --- |
| `game/objective.cpp` | 19 functions; 38 globals (the 32 name strings, two name tables, the action band, the animation slots, scene words); `obj_strcasecmp`, `advisor_name_tables_initialize`, `advisor_config_tokens_split` | `objective_shared.hpp`: `ADV_ACTION_ROWS`, `AdvisorAnimationSlot`, 12 declarations |
| `gods98/view.cpp` | 9 vector helpers (7 as `#if` naked/C groups); `s_normalize_unit` (`0049e008`) | none |
| `crt/crt.cpp` | `global_flag_bit0_is_set`, `advisor_clear_active_state`, `display_width_get`, `display_height_get`; `s_advisor_state_word` (`004b31bc`) | `crt_shared.hpp`: 1 declaration |
| `gods98/engine.cpp` | `engine_app_mode_get` | `engine_shared.hpp`: `s_application_mode` |
| `game/frontend.cpp` | `formatted_text_printf_width`, `formatted_text_printf` | none |

The tool added three forward declarations, for the static helpers that now
follow the unit's functions. The new `src/game/advisor.cpp` is the first
`game` entry of the list.

Gates:
- `cmake --build build` and `ctest`: 35 of 35 pass.
- Meson (with `PKG_CONFIG_PATH` pointing at SDL3's `.pc`): the `game` and
  `gods98` libraries compile, `advisor.cpp` included. Meson's executable and
  tests already fail to link on `main`. The converted `.c` units (such as
  `bezier.c`) are not in its lists, and the gods98 tests do not link `game`.
  `vec_scale`, which moves from gods98 to game, joins that existing list of
  missing gods98-test symbols.
- `make tools-test`, which now runs `link_order.py check`: passes.
- `make reccmp-check`: the 3 drops below, then passes after the baseline
  refresh. `link_order.py check-map build-reccmp/LegoRR.map`: OK.
- `make match-progress` and the local `pr_match_progress.py check` (the
  `matching-progress-freshness` job): fresh.

### Layout metrics

From `binary-progress.json`, `main` (`93f939a8`) against this change:

| Measure | Before | After |
| --- | ---: | ---: |
| Pair inversions, full | 633,221 | 593,010 |
| Pair inversions, game-only | 619,320 | 579,179 |
| Adjacent inversions, full | 896 | 877 |
| Adjacent inversions, game-only | 833 | 816 |
| Unit-order inversions, pairs | 3,816 | 3,736 |
| Unit-order inversions, adjacent | 93 | 90 |
| Unit contiguity | 66.82% (145 units) | 67.28% (146 units) |
| Unit-relative identity (masked) | 36,732 B (6.63%) | 36,674 B (6.62%) |
| Exact-function code bytes | 44,675 | 44,617 (−58, `vector_scale_components_in_place`) |
| Mean score, full / game-only | 55.14% / 50.97% | 55.13% / 50.96% |
| `.text` identical at original addresses | 18,260 B (2.84%) | 17,801 B (2.77%) |
| `.rdata` identical at original addresses | 1,696 B (15.10%) | 1,277 B (11.37%) |
| `.data` identical at original addresses | 13,823 B (18.88%) | 13,683 B (18.69%) |

The `.rdata` loss is all in the first 4 KB of the section, where the first
objects' constants land. Advisor's object now links first, ahead of
`game/cmd.cpp`. `.text` identity at original addresses stays at chance
(2.8%), so its small fall is noise, not a regression.

### Every changed function, with its classifier label

`make reccmp-check` against `main`'s baseline found the following (the same
in both scopes):

| Address | Function | Score | Label | Cause |
| --- | --- | --- | --- | --- |
| `0x004018c0` | `advisor_request_admit` (moved) | 20.69 → 13.79 | `drift-reordered`, +0 | instruction order |
| `0x0040ff50` | `scene_object_list_apply_float_callback` | 87.50 → 75.00 | `drift`, +0 | listing identical after normalisation |
| `0x0040fcc0` | `vector_scale_components_in_place` | 100.00 → 91.30 | `drift-review`, +0, **OWNER REVIEW** | commutative `fmul` operands swapped (below) |
| `0x00401270` | `advisor_config_slots_load_target` (moved) | 6.51 → 11.16 (up) | `drift-review`, +0 | register allocation |

The baseline is refreshed with these four (ADR 0006 rule 2).

Listing digests against `main`'s (format 4), with each changed listing
compared instruction by instruction against a report of the `main` tree:

| Kind | Functions | Label |
| --- | --- | --- |
| Operand names only | `0x00404280`, `0x00404fc0`, `0x00405c10`, `0x00407a10`, `0x00409600`, `0x00409a40`, `0x0042f6f0`, `0x00430530`, `0x0043bbc0`, `0x0043bc40`, `0x0043f7d0`, `0x00441ed0`, `0x00444460`, `0x00447ea0`, `0x0044bfd0`, `0x0044c110`, `0x00466750`, `0x00477bf0`, `0x00482b50` (19; none falls below baseline; digest unchanged) | `drift` |
| Commutative x87 operand swap | `0x0040fcc0`, `0x00418da0`, `0x0041c1c0`, `0x00463d20` | `drift-review`, +0 |
| Reordered | `0x004018c0`, `0x00411360` | `drift-reordered`, +0 |
| Register allocation | `0x00401270` | `drift-review`, +0 |

Notes on the table:

- **The operand-name changes** are finding 521's naming gap. A flag
  immediate equals an address inside a large rebuilt array, and Reccmp names
  it as an element of that array. Here
  `s_entity_path_motion_runtime[1000].samples+56` becomes
  `s_entity_path_motion_runtime[998].samples+48`, because `.bss` moved.
  Format 4 (#465) masks `name[i].field+N` operands, so these 19 listings
  keep their digests and classify as `drift`. Under format 3 they were
  `drift-review` with +0 instructions.
- **The x87 swaps** are in unchanged source. Each one loads the other operand
  of a commutative `fadd` or `fmul` first. For example `0x0040fcc0`: `fld
  [esp+0x10]; fmul [esi]` becomes `fld [esi]; fmul [esp+0x10]`. The function
  now sits 270 lines earlier in `view.cpp`, because the vector helpers above
  it left. This is ADR 0006's "renumbering drift". It is not register, stack
  or branch drift, so the classifier rightly leaves it for review. Only
  `0x0040fcc0` falls below baseline. It lost its exact match, 58 bytes, and
  needs the owner's acceptance.
- **The register-allocation change** is in `advisor_config_slots_load_target`,
  a moved function. It keeps the target-lookup hook, now an `extern`, in a
  register (`call ebx`) instead of a stack slot. Its score rises.

The net effect is −3 scores, +1 score and −58 exact bytes, against a fall of
40,211 pair inversions and 80 unit-order inversions.

## Survey of the remaining units

`python3 tools/relocate_unit.py --survey` plans every unrelocated game and
gods98 unit, and retries stem clashes with a `<stem>_unit` variant. The tool
checks in stages, so a unit lists only the blockers of the first stage that
fails. Measured on this tree:

| Status | Units |
| --- | --- |
| Relocate as they are | GAME-004, 006, 009, 014, 015, 016, 022, 030, 031, 036, 038, 040, 041, 044, 045, 046, 047, 048, 049; G98-008, 017 (21) |
| Needs `--stem` only | G98-013 Input.c (`input.h`/`input.hpp` exist) |
| Blocked | 43 |

Blocking constructs, by the number of units each blocks:

| Construct | Units | Notes |
| --- | ---: | --- |
| Named namespace | 31 | Mostly `unported_ledger_stubs`. Annotated ledger stubs live in it, and moved code calls stubs defined in it. `lego` (`video_graph.cpp`) and `reference_target` add a few |
| `#if` block | 12 | Definitions inside platform conditionals, with mixed bodies or conditional `__declspec(naked)` heads: `surface.cpp` (G98-010, 26 items), `entity.cpp` (GAME-043, 13), `cmd.cpp`'s `cmd_registry_init_target` (GAME-002) |
| Macro `#undef`'d or redefined | 4 | `surface.cpp`, `startup_config.cpp` |
| Annotation on a non-definition | 3 | An annotation above a prototype (GAME-023 `0x00447b30`, GAME-029, G98-012) |
| Name clash between source files | 2 | `s_motion_jitter_flags` is in both `util.cpp` and `simtick.cpp` (GAME-024); GAME-033 |
| Annotation not attached | 1 | GAME-023 |
| Two items on one line | 1 | `simtick.cpp:503` (GAME-026) |
| Several declarators shared | 1 | GAME-032 |
| Backend-only source | 1 | G98-005 Main.c lives in `platform/winapi/*.cpp` |

Inputs for the wave tickets (#442–#446):

- **Namespace support is the one feature that unblocks most of R1–R5.** It
  means moving members of a named namespace inside the same namespace, and
  resolving `ns::name` uses. Without it, R1 can take GAME-004, 006, 009, 014,
  015 and 016 now, plus GAME-001. Its others (GAME-005, 007, 008, 011, 012,
  013) wait.
- The `#if` cases are mostly real source shapes, for example a naked head
  under `#if` and one shared body with inline `__asm` blocks. Either the tool
  learns to move a conditional head with its definition, or a small source
  edit turns them into whole-definition alternatives. That edit is not
  verbatim, so it would be a separate commit.
- The rest are one-off source fixes, each in its own commit: an annotation
  on a prototype, two items on one line, two statics with the same name in
  two files.

## Not done here

- `0x0040fcc0`'s operand swap is accepted into the baseline under rule 2 by
  this PR, and is flagged for the owner.
