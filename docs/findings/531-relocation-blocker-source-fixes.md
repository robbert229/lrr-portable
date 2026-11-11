# Finding 531 — One-off source shapes that blocked unit relocation

Issue #478, preparing the relocation waves (#443–#446) of the
[#388 plan](../plans/388-original-link-order.md). It follows
[finding 523](523-relocate-unit-tool-and-advisor-relocation.md), whose survey
found eight units blocked by small source shapes, not by missing features of
`tools/relocate_unit.py`. This change edits only those shapes. It relocates no
unit. Measured on `main` at `9ec01959`, after wave R1 (#477) and the Reccmp
diagnostics (#479).

## Summary

- **Two more units relocate as they are.** They are GAME-024
  (LightEffects.c) and GAME-033 (Pointer.c). `--survey` now reports 17 of 59
  unrelocated units, up from 15.
- **Every in-scope blocker is gone.** The four units still refused now stop at
  a different construct, named below. Two are out of scope (a named namespace
  and `#if` blocks), one is a tool bug and one is an overload.
- **#479 had already fixed two of the annotations.** It attached `0x0044a2a0`
  and `0x00459050` to their definitions, so GAME-029 needed nothing more. For
  `0x0044a2a0` this change moves the stranded comment.
- **G98-013 needed no change.** It already relocates with `--stem input_unit`,
  and the survey retries that stem itself.
- **No generated code changed.** In `make reccmp-check` no score or instruction
  count changed. One function is newly mapped (`0x0047e460`), because its
  annotation now sits on its definition.

## Fixes

One commit each. Each was checked with `match_function.py` on every annotated
function of the touched files, then with the full gate.

| Unit | Blocker (survey) | Fix | Next blocker |
| --- | --- | --- | --- |
| GAME-023 | annotation on a prototype (`0x00447b30`) | annotation moved below the forward declaration, onto `ledger_stub_00447b30_EntityActivityTick` | `entity_record_flag20_query` has two overloads (`entity.cpp:869`, `:8037`), both declared in the shared `entity.hpp` |
| GAME-023 | annotation not attached (`0x0044a2a0`) | fixed on `main` by #479. Here the comment that describes the function moves with it | (as above) |
| GAME-024 | name clash: `s_motion_jitter_flags` in `util.cpp` and `simtick.cpp` | unified. `simtick.cpp`'s file-scope `extern` moves to the end of its private `simtick_shared.hpp`, which `simtick.cpp` includes directly above | none (relocates) |
| GAME-026 | two items on one line, `simtick.cpp:503` | line split | verbatim check (tool bug, below) |
| GAME-029 | annotation on a type (`0x00459050`) | fixed on `main` by #479 | named namespace, `frontend.cpp:10564` (#471) |
| GAME-032 | "several declarators" | the naked/C pair for `0x0045b930` (`crt.cpp`) held an `extern "C" _ftol` declaration in its `#if` branch. The declaration now has its own `#if` block | `#if` block: the `_ftol` declarations the moved `__asm` needs are conditional (`crt.cpp:615`, `:997`) |
| GAME-033 | name clash: `s_cursor_slot` in `crt.cpp` and `media.cpp` | unified. `media.cpp`'s definition gets external linkage, and `crt.cpp` drops its copy and declares the real one in `crt_shared.hpp` | none (relocates) |
| G98-012 | annotation on a prototype, `surface.cpp:6213` (`0x0047e460`) | annotation moved below the forward declaration | `#if` block, `crt.cpp:931` (`0x0047e7e0`) |
| G98-013 | stem clash with `input.h`/`input.hpp` | none: `--stem input_unit` is already enough | none |

### The two name clashes: unify or rename

The issue asked whether each clash is one original global or two. The
original's addresses and `LEGORR.full.c` show that both are one global, so
both were unified rather than renamed.

- **`s_motion_jitter_flags` is `DAT_004eaec8`.** All 31 references in the
  decompile are in LightEffects.c functions, all within GAME-024's text range
  `0044c710`–`0044db50`: `0044c790`, `0044c970`, `0044cc10`, `0044cef0`,
  `0044cf70`, `0044cfa0`, `0044d0d0`, `0044d280`, `0044d760`, `0044da20` and
  `0044da50`. The port already had a single definition (`util.cpp`,
  `GLOBAL 0x004EAEC8`), and `simtick.cpp` only redeclared it. The tool would
  have exported that file-scope `extern` into the target, which also receives
  the definition.

  Declaring it in the shared `simtick.hpp` was measured first and rejected.
  It drifted two `level.cpp` functions through the declaration-count effect of
  finding 514: `0x0044fb90` went from 23.19% to 23.65%, and `0x004512d0` from
  11.34% to 13.27%. In the private header, `simtick.cpp`'s preprocessed tokens
  are unchanged. The header's other includer, R1's
  `screen_effect_quads.cpp`, scores as before in `match_function`.
- **`s_cursor_slot` is `DAT_00500a98`.** PointerCursorSelect (`0045cc20`) and
  PointerCursorOverrideSet (`0045cc40`) write it, and
  GlobalValueGet_00500a98 (`0045cc60`) reads it. All three are Pointer.c
  functions inside GAME-033's range `0045c830`–`0045cdc0`. The port held two
  copies, so `global_cursor_word_get` never saw the cursor type that `media.cpp`
  selected. That is now fixed. In the map, `media.cpp`'s `s_cursor_slot`
  keeps its address (now public). `crt.cpp`'s removed copy moves the libc data
  after it down by 4 bytes. Nothing else in `.data`/`.bss` moves, and the
  `.text` symbols are where they were.

### GAME-032 had no multi-declarator declaration

The survey's label was misleading. Its classifier files "also defined by an
unannotated item (overload or several declarators)" under *several
declarators*. Here the cause was a naked/C `#if` pair that the tool could not
read as a group of whole alternative definitions, because the `#if` branch
also declared `_ftol`. Splitting the declaration out leaves `.data`/`.bss`
order unchanged, since it declares a function. The same shape remains at
`0x00417430` (`crt.cpp:615`), outside this issue's units.

### GAME-026's next blocker is the tool

After the line split, the tool refuses
`level_coord_pair_within_bounds_target` (`0x004502c0`, a naked/C `#if` pair)
with "the moved text differs from the original". The pair is shared with the
remaining code, so `static` is removed from both heads. `_verify` compares a
group's body from its first `#` line, so the `#else` head is part of the
"body", and that comparison does not strip `static`. With `_body` stripping
`static` from group bodies, the unit plans cleanly: 79 items. This is a
tooling fix, so it is left out of this change. PR #483 (#471) reports fixing an `#if` group that loses `static` in the body check, which may be this defect.

## Matching build

`make reccmp-check` against the baseline at `9ec01959`: no function fell, and
no score or instruction count changed. One function is newly mapped:

| Address | Function | before | after | Cause |
| --- | --- | ---: | ---: | --- |
| `0x0047e460` | `ledger_stub_0047e460_ComObjectDestroyAndOptionallyFree` | unmapped | 50.00 | annotation now on its definition |

The `0x00447b30` move changes nothing, because Reccmp already paired that
annotation with the definition. `make match-progress` agrees: no existing
listing digest changed, and one was added.

## Survey, before and after

`python3 tools/relocate_unit.py --survey` on `9ec01959` and on this branch, by
construct (a unit lists only the blockers of the first stage that fails):

| Construct | before | after |
| --- | ---: | ---: |
| named namespace | 31 | 31 |
| `#if` block | 12 | 13 (+GAME-032) |
| macro `#undef`'d or redefined | 4 | 4 |
| annotation on a non-definition | 2 | 0 |
| name clash between source files | 2 | 0 |
| two items on one line | 1 | 0 |
| several declarators (here an overload) | 1 | 1 (GAME-023) |
| verbatim check failed | 0 | 1 (GAME-026) |
| backend-only source | 1 | 1 |
| **relocate as they are** | **15 of 59** | **17 of 59** |

## Not done here

- No unit is relocated. The waves do that.
- GAME-023's overload, GAME-026's verbatim-check fix (tooling), and the `#if`
  blockers of GAME-032 and G98-012 remain.
