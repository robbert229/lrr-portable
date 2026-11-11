# Finding 539 — `#if` and macro relocation blockers: six patterns, and zero-drift reshapes

Issue #490, design input for the relocation waves (#443–#446) of the
[#388 plan](../plans/388-original-link-order.md). The proposal it supports is
[`docs/plans/490-relocation-conditional-code.md`](../plans/490-relocation-conditional-code.md),
which has the full case table. Measured on `main` at `970a1822` (after
#489). No tool or source file changed; every edit below was a scratch copy,
scored and discarded.

## Results

1. **The survey's 16 + 8 units are 17 distinct units.**
   `relocate_unit.py --survey` reports `#if` blocks in 16 units and
   redefined or `#undef`'d macros in 8. Seven units are in both lists:
   GAME-020, G98-003, 010, 011, 014, 025 and 030. GAME-037 is the only unit
   blocked by macros alone.
2. **Six source patterns cover all of them** (plan, Patterns):

   | Pattern | Cases | Units |
   | --- | ---: | --- |
   | P1 split naked head: `#if` / `__declspec(naked)` / `#endif`, one head, a body with `#if` `__asm` / `#else` C | 15 functions | GAME-043 (13), GAME-027, GAME-050 |
   | P2 alternative macro definitions, one per branch (calling conventions, `mkdir`, pointer width) | 8 groups | G98-003, 010, 011, 012, 014, 025, 030; GAME-043 (hidden) |
   | P3a alternatives plus a branch-local helper | 3 | GAME-002, 017, 021 |
   | P3b a conditional declaration, datum or macro | 4 | GAME-017, 027, 032; G98-030, 031 |
   | P4 unconditional macros shadowing same-named statics for one function, then `#undef` | 1 | GAME-037 |
   | P5 original code inside a nested backend branch | 1 region | G98-010 |
   | P6 P2 plus scoring-only padding storage | 1 | GAME-020 |

   The 15 P1 heads are every split naked head in `src/`. Inline `__asm`
   inside a body (`#if` wholly within the braces) already relocates.
3. **The macro refusal is mostly a false positive.** `_redefined` in
   `relocate_unit.py` is true if a file has more than one `#define X` or any
   `#undef X`. The survey's macro refusals in 8 units come from seven
   constructs:
   - six P2 groups, each defining the macro once per branch of one
     `#if`/`#elif`/`#else`, which is not a redefinition;
   - GAME-037's P4 region.

   The P2 groups' `#undef`s, where present, follow the last use. Two of them
   are at the end of the file (`surface.cpp:6214`,
   `scene_resources.cpp:1041`).
4. **Token-identical reshapes are zero drift, measured.** Each reshape was
   checked three ways:
   - every annotated function of the file scored with
     `match_function.py --source`;
   - the single-TU object compared outside `.debug$*`: bytes, relocations
     by symbol name, and the symbol table;
   - the file's own token stream (includes stripped) compared under the
     native, matching and MinGW reference configurations.

   | Reshape | File | Functions | Score changes | Object | Tokens (3 configs) |
   | --- | --- | ---: | ---: | --- | --- |
   | 13 split heads → whole alternatives; hoist GAME-021's string | `game/entity.cpp` | 286 | 0 | identical | identical |
   | 1 split head | `gods98/record.cpp` | 91 | 0 | identical | identical |
   | 1 split head | `game/select.cpp` | 32 | 0 | identical | identical |
   | hoist GAME-002's branch-local macros | `game/cmd.cpp` | 110 | 0 | identical | identical |
   | hoist `_ftol` out of `0x00417430`'s group; move `0x0047e7e0`'s annotation onto its definition | `crt/crt.cpp` | 327 | 0 | identical | identical |
   | rename GAME-037's four region macros, drop their `#undef`s | `game/mission.cpp` | 25 | 0 | identical | identical |
   | **control:** one unused prototype | `game/select.cpp` | 32 | 0 | **differs** (relocations and symbol table renumbered) | **differs** |

   In the control, scores and code bytes stay the same, while MSVC's
   internal `$SG…` symbols renumber (finding 506). The object check
   therefore sees declaration-count drift before any score does.
5. **The survey hides blockers behind the conditionals.** It reports only
   the first failing stage.
   - With P1 reshaped, GAME-043 stops at a further P2 group:
     `ENTITY_STATS_FLOAT_READ`/`WORD_READ`, `entity.cpp:5607`–`5615`. Its
     native branch calls statics `stats_rec_float` and `stats_rec_word`,
     which the private header would need.
   - A probe tree resolved every evaluable `#if` to the matching branch and
     dropped `#undef`s. On it, 9 of the 17 units plan cleanly: GAME-027,
     037, 043; G98-010, 011, 014, 025, 030, 031.
   - The other 8 stop at:
     - an anonymous namespace (#485): GAME-020, 021, 050, G98-003;
     - several declarators (#485): GAME-032;
     - an overload (#485, PR #493): GAME-002;
     - "the private header needs a static that is not shared": GAME-017;
     - out-of-line `AviPlaybackState::` members (`movie.cpp`): G98-012.

## Consequences

- P1, P3a and P4 can be fixed in source with no baseline refresh. Such
  edits are hand edits before a relocation. Whether to allow them is the
  owner's decision; the plan proposes an ADR 0006 addendum ("reshape
  commits").
- P2 and P3b need the tool to move a top-level conditional group whole and
  verbatim. That is a generalisation of the existing merge of
  whole-alternative function groups. It also needs guarded private-header
  declarations, and export of a static that a header item needs. The
  export is not specific to `#if`: G98-030 and GAME-017 hit it too.
- #389's collapse is not a prerequisite. It is blocked by #388, and 13 of
  the 15 P1 functions already score 100% with `__asm`, so collapsing them
  is not zero drift.

## Reproducing

    python3 tools/relocate_unit.py --survey
    python3 tools/relocate_unit.py GAME-043 --stem <scratch> --dry-run --no-footprint
    python3 tools/match_function.py --source src/game/entity.cpp --json --no-diff --keep

The reshape scripts and the object and token comparisons were scratch
scripts. Ticket T0 of the plan turns them into `tools/reshape_check.py`.
