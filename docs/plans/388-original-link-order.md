# Plan: original translation units in original link order (#388)

[ADR 0006](../adr/0006-original-link-order.md) records the decisions this
plan carries out. [Finding 516](../findings/516-msvc5-link-order-and-layout-experiments.md)
has the measurements quoted here.

Baseline (`binary-progress.json`, `main` at `be89ef0b`):

| Measure | Value |
| --- | ---: |
| Function-order inversions, full | 908 adjacent, 862,027 pairs |
| Functions identical at their original address | 0 |
| `.text` identical at original addresses | 18,005 B, at chance |
| Exact-function code bytes | 40,972 B (6.38%) |
| Rebuilt `.text` / original | 933 KB / 643 KB |
| Rebuilt `.data` / original | 987 KB / 73 KB |
| Units in their own file | 9 of 79 |

Those 9 units are all converted: G98-004, G98-006, G98-007, G98-018,
G98-021, G98-024 (#432), G98-027, GAME-003 (#431) and GAME-034.

## Terms

- **Relocation** moves a unit's functions, and the globals they own,
  verbatim into the unit's own file. **Conversion** is the ADR 0005
  rewrite into original shape. A conversion PR also relocates its unit.
- The **link-order list** gives one entry per unit in original order. It is
  the matching build's object order.
- The **port tail** is the port-only objects, linked after the last gods98
  object.
- The **legacy files** are today's mixed source files, while they still
  hold original functions.

## Stages at a glance

| Stage | What | Needs | Runs alongside conversions |
| --- | --- | --- | --- |
| S1 | Link foundation: non-incremental gate, ordered explicit objects, checks, metrics | — | Yes |
| S2 | Relocation tooling and the drift classifier | S1b (wiring) | Yes |
| S3 | Relocation waves R1–R5 | S1, S2 | Yes, with the claim rule; short file freezes for Lego.c and LegoObject.c |
| S4 | Conversions (ADR 0005), ongoing | — | This is them |
| S5 | CRT, import and late-member order | S3's drain of `crt.cpp` | Yes |
| S6 | Data layout: scoring-only storage, pads, commons, per-unit data checks | S1c, S3 for each region | Yes |
| S7 | Address identity: size-exact prefixes, `.text` page fit | S4 and S6, per prefix | Yes |

**Critical path to "one file per unit, inversions near zero":**
S1a → S1b → S2 → S3. Conversions do not gate it.

**Critical path to byte identity at original addresses:** conversion of
every unit up to an address, plus S5 and S6 for the data those units
reference. That is the long tail. S1–S3 make each conversion count towards
it.

## S1 — Link foundation (link-only, no source moves)

### S1a: non-incremental gate build and link map

**Scope.**
- The gate build (`make reccmp-report`, `reccmp-check`) links with
  `/INCREMENTAL:NO` and writes `/MAP`.
- `make binary-progress` reuses that build instead of a second one.
- Measure `/OPT:REF` in the same PR. Adopt it only if no score changes.
- Refresh the baseline once.

**Prerequisites.** None.

**Effect.** Measured: 600 exact functions against 589, 55 up and 5 down
(finding 516). The inversions do not change. One matching build per PR
instead of two.

**Risk.** Low.
- The 5 drops are link effects: thunks and address naming. They are listed
  in finding 516 and classified in the PR.
- `match_function.py` emulates the incremental-link thunks of the `program`
  target (finding 507). Its parity must be rechecked with
  `make match-function-parity`, and the emulation switched off where the
  link no longer makes thunks.

**Alongside conversions.** Yes. In-flight conversion PRs rebase and rerun
`make match-progress`, as for any baseline refresh.

### S1b: the link-order list, linked as explicit objects

**Scope.**
- Add the link-order list. Initially it holds the 9 converted units in
  their original positions, and each legacy file at the position of the
  median original address of its functions.
- In the matching build, link the game and gods98 sources as explicit
  objects in list order, then the port tail (`main.cpp`, `platform/`, the
  legacy files' leftovers), then the libraries. Use LIBC, `oldnames.lib`,
  and the import libraries in the original's thunk order.
- The native and reference builds build `game` and `gods98` from the same
  list (ADR 0003).
- `convert_unit.py` wires new units into the list.
- Amend ADR 0005 step 2 ("never add it to the executable's own sources").
  Update `docs/repository-layout.md`.

**Prerequisites.** S1a, because explicit objects need the non-incremental
link.

**Effect.** Pair inversions fall from about 871,000 to about 650,000
(simulated). Adjacent inversions change little. The order is now
deterministic, and every later relocation lands in place.

**Risk.** Medium.
- Objects that were never pulled are now linked. These are
  `gods98/ptrstack.cpp`, `game/video_graph.cpp` and `engine_calloc`
  (finding 507). Some may fail to link or add duplicate symbols with LIBC;
  move them into the port tail or out of the matching build.
- One-time baseline refresh, under ADR 0006 rule 1.

**Alongside conversions.** Yes. After S1b lands, an open conversion PR
rebases and moves its CMake line into the list. This is a one-line conflict.

### S1c: layout metrics and the order checks

**Scope.**
- Add a static check: every game and gods98 unit appears once in the list,
  in spec order, and every listed unit file holds only its own unit's
  markers. It needs no toolchain, so it can run in `make tools-test` and in
  CI.
- Add a map check: object contribution order against the list.
- Extend `binary_progress.py` (schema 3) with:
  - object-order inversions from the map;
  - unit-relative identity: each unit's bytes against the original unit at
    the same unit offset, with absolute operands masked;
  - the size-exact prefix: the longest original prefix whose rebuilt
    functions have the original sizes;
  - per-unit data-offset agreement (feeds S6).

**Prerequisites.** S1a, for the map.

**Effect.** None on the binary. It makes S3–S7 measurable.

**Risk.** Low. Changes to tools run `make tools-test`. The PR freshness
check needs its input hash list updated.

**Alongside conversions.** Yes.

## S2 — Relocation tooling

### S2a: `relocate_unit.py`

**Scope.** A tool next to `convert_unit.py` that reuses
`conversion_footprint.py`. For one unit it:
- creates `src/<lib>/<stem>.cpp`;
- moves the unit's marked functions verbatim, in original address order;
- moves the `GLOBAL`-annotated state those functions own;
- computes the static closure of the moved code: file-static helpers and
  variables used only by this unit move with it. Ones shared with code left
  behind get external linkage through a private header (`<file>_shared.hpp`)
  next to the source file. It may emit such a header but must never edit a
  shared one;
- keeps the include list of the source file, in its order;
- updates the link-order list, CMake and Meson lists;
- records, for each moved function, a hash of its body before and after;
- prints the conversion footprint and a PR checklist, as `convert_unit.py`
  does.

**Prerequisites.** S1b, for the list.

**Effect.** None by itself.

**Risk.** Medium.
- The big mixed files have 150–400 `static` lines each (finding 516).
- C++ namespaces and functions defined in headers need handling.
- Unmarked port helpers that sit between marked functions stay behind.

**Alongside conversions.** Yes. It is a tool.

### S2b: drift classifier

**Scope.** `match_baseline.py check` classifies each drop:
- **drift**: after normalising register names, stack offsets and branch
  targets, the original and rebuilt listings are identical;
- **regression**: anything else.

It reads the existing Reccmp diff in the report. It makes ADR 0005 rule 2,
and ADR 0006 rule 2, mechanical for relocations and conversions alike.

**Prerequisites.** None.

**Effect.** None on the binary. It shortens every refresh review.

**Risk.** Low to medium. Normalisation must not hide real changes, such as
a different call target or a different constant. Unit-test it on the
documented drift cases (findings 508–513).

**Alongside conversions.** Yes, and conversions benefit at once.

## S3 — Relocation waves

**Rules for every wave:**
- **One commit per unit.** A commit contains only the move and the linkage
  needed to compile it.
- **Batch several units per PR** so one full matching build covers them.
- **Do not relocate a unit with an open conversion PR** (claim rule). The
  conversion relocates it. At the time of writing, claimed units are
  G98-019 (#412), G98-026 (#414) and G98-029 (#415).
- **Do not delete declaration-only deviations** in shared headers during
  relocation (ADR 0006 decision 5). Ticket 18 retires them after R5.
- Keep the native build and `ctest` green after each PR (ADR 0004).
- Each relocation PR does:
  - run `make reccmp-check` and classify with S2b;
  - refresh the baseline under ADR 0006 rule 2;
  - run `make match-progress`;
  - report the inversion delta and the net score effect.

**Wave order.** Start small to prove the tool, then go from the most
misplaced code to the largest units.

| Wave | Units | Why this order | Notes |
| --- | --- | --- | --- |
| R1 | GAME-001 Advisor, GAME-004–GAME-009, GAME-011–GAME-016 (except claimed) | Small and medium game units. Many live in `gods98/view.cpp` and `record.cpp`, so moving them also fixes their library | Proves `relocate_unit.py` on 5–30-function units |
| R2 | gods98 units not converted or claimed: G98-001–003, 005, 008–017, 020, 025, 028, 030, 031 | Moves the gods98 units out of game files (`startup_config.cpp`) and splits `view.cpp`, `surface.cpp`, `resource.cpp` and `record.cpp` | G98-001 and G98-012 are the two C++ objects (finding 505 rule 8). They stay C++, and later conversions keep `/GX` |
| R3 | GAME-017 FrontEnd, GAME-018 HelpWindow, GAME-019 Info, GAME-020 Interface | Drains `frontend.cpp` (20 units), `message.cpp` and `select.cpp` | Short freeze on `frontend.cpp` while the PR lands |
| R4 | GAME-021 Lego, GAME-022, GAME-023 LegoObject | The largest units: 216, 27 and 258 functions, from `level.cpp` and `entity.cpp` | Short freeze on `entity.cpp` and `level.cpp`. First decide whether Lego.c is two objects near `0042f350` (finding 505) |
| R5 | GAME-002 AITask, GAME-010, GAME-024–GAME-050 (except GAME-034) | The rest of the game segment, which drains `crt.cpp`'s 159-unit mix | After R5, every game and gods98 function is in its unit file |

**Effect.** The pair inversions fall with each wave and reach about zero
after R5. They stay a little above zero for these causes, which S1c lists
with explanations:
- functions defined in headers, whose COMDAT goes to the first object in
  link order;
- statics emitted after their first caller (finding 516);
- the 23 report entries that are not function starts.

Scores drift both ways under rule 2. Finding 506's step (a) moved 13
functions with no change, so the net effect is expected to be small. The
at-address metrics barely move, because sizes still differ.

**Risk.** Medium.
- The most work is mechanical: file-static closures in `entity.cpp`,
  `level.cpp`, `frontend.cpp` and `view.cpp`.
- Every PR conflicts on `match-baseline.txt`, `match-listing-digests.txt` and
  `binary-progress.json`,
  which conversions also refresh. Land one restructuring PR at a time;
  conversions keep their own queue.

**Alongside conversions.** Yes.
- Conversion and relocation touch different units, so textual conflicts are
  limited to neighbouring hunks in shared legacy files.
- The only freezes are the file freezes in R3 and R4. Schedule them, and
  announce them to running conversion threads.

## S4 — Conversions (ongoing)

ADR 0005 is unchanged, except the wiring (S1b) and port-only accessors being
compiled out of the matching build (ADR 0006 decision 4).
- Conversions continue bottom-up through the 9 leaf units, the 6 gods98
  levels and then the game cycle (finding 516).
- After relocation, a conversion's footprint is its own file, its callees'
  headers and its callers. Footprints shrink, so more conversions can run
  in parallel.
- The permuter (#382) should target relocated or converted units. Their
  translation units are stable. Forms tuned inside a legacy file can drift
  when the unit later moves.

## S5 — CRT, import and late-member order

**Scope.**
- `src/crt/` is linked into the matching build no longer; the LIBC
  functions come from LIBC.LIB. R5 has drained the game code from
  `crt.cpp`.
- Retire the 115 `/include` directives where the original's references now
  pull the members. Keep, in original order, the few that are still needed.
- Move `trnsctrl.cpp`'s routines out of `game/seh_runtime.cpp`, so they come
  from LIBC.
- Order the import libraries as d3drm, ddraw, avifil32, dinput, dsound,
  kernel32.
- Provide the late members `_strnicmp`, `_itoa`, `_strlwr` and `_strupr`
  through `oldnames.lib`. The WAD decompressor and the display-mode dialog
  come after them.
- Check the LIBC member order in the map against the CRT units of
  `translation-unit-ranges.csv`.

**Prerequisites.** R5 has drained `crt.cpp`. S1c provides the map check.

**Effect.** The CRT region (`0048d920`–`0049b6b0`) gets the original
object order. Its functions are already 85–100% exact, so unit-relative
identity rises sharply there. Absolute addresses still shift by the port
tail.

**Risk.** Medium. Pull order depends on the references. Where the
reconstruction does not yet reference a CRT member as the original did, the
order differs until the referencing unit is converted.

**Alongside conversions.** Yes.

## S6 — Data layout

### S6a: retire storage made up for scoring

**Scope.**
- Remove the named-section pads: `.data$zzzzzz` in `frontend.cpp`,
  `.data$yyyyyy` in `audio.cpp` and `.data$cmdpool` in `cmd.cpp`.
- Find every `Deviation (scoring only)` that changes storage, and replace
  each with a storage-neutral device:
  - `key_names`: drop the spare slot. Annotate Files.c's table at
    `005769e0`, which follows the table once commons are in order. Or
    prototype a Reccmp `data_sources` size row. Or accept the lower score.
- Make `Deviation (scoring only)` the required comment phrase, and the ADR
  0005 PR checklist item.

**Prerequisites.** S1c, so the effect is measured. The pads are easier to
remove after R3, when `frontend.cpp`'s units are in their own files.

**Effect.**
- Rebuilt `.data` shrinks from 987 KB towards 73 KB.
- `.data` byte identity at aligned offsets jumps once the pads go.
- Some scores that depended on the pads' naming drop. Classify them as
  scoring-only losses, and record them in the PR rather than restoring the
  pads.

**Risk.** Medium. This is a deliberate score trade, so the coordinator
should approve the expected drops.

**Alongside conversions.** Yes.

### S6b: characterise common-symbol allocation

**Scope.** Find LINK 5.0's rule for ordering common symbols. Finding 516's
three links rule out object order and its reverse.
- Run a few more small links: vary names, sizes, how many objects define a
  symbol, which object references it first, and the library position.
- Test it against the original's common region from `00549520`.
- Write a finding and decide how the tool should check common order.

**Prerequisites.** None.

**Effect.** None by itself. It decides how the common region can reach its
original layout.

**Risk.** Low. It may show that the order is a hash of the names, which
cannot be reproduced without the original names. Commons would then stay
deferred, with a documented cause.

**Alongside conversions.** Yes.

### S6c: per-unit data placement check

**Scope.**
- For each unit, compare each `GLOBAL` and `STRING` annotation's offset
  from the start of the unit's contribution in its section with the
  original offset from the unit's range in `translation-unit-ranges.csv`.
- Report the units that agree fully.
- Add the check to the ADR 0005 recipe, step 8.

**Prerequisites.** S1a (map) and S1c.

**Effect.** Data layout becomes a per-unit, independent pass/fail. It does
not depend on the sizes before it.

**Risk.** Low.

**Alongside conversions.** Yes. Conversions use it.

### S6d: the `{1,2,4,-1}` `.rdata` tables

**Scope.** Identify the header that gives 19 gods98 objects and one game
object a private 16-byte table (finding 505 rule 4). Include it in those
units.

**Prerequisites.** Several gods98 conversions.

**Effect.** `.rdata` order is right for those objects.

**Risk.** Low.

**Alongside conversions.** Yes.

## S7 — Address identity

**Scope.** No separate restructuring work. Track the size-exact prefix and
unit-relative identity (S1c). Absolute identity becomes possible:
- in `.text`, once the prefix before a function is size-exact;
- for data references, once the sections before them end in the same pages
  as the original's.

The port tail must be empty by then. #389 collapses the dual-path code, and
ADR 0006 compiles port-only code out of the matching build.

**Prerequisites.** S4 for every unit in the prefix; S5 and S6 for the
referenced data.

**Effect.** #388's third criterion: byte-identical `.text` at original
addresses rises measurably. It rises first for prefixes of the game
segment.

**Risk.** Each unit is ordinary conversion work. The risk is the total
time.

## Expected metric movement

| After | Pair inversions (full) | Exact functions | Unit-relative identity | `.text` at original addresses |
| --- | ---: | --- | --- | --- |
| today | 862,027 | 589 gate / 600 non-incremental (#425 tree) | not measured | at chance |
| S1a | unchanged | +11 vs. incremental gate | — | at chance |
| S1b | about 650,000 | unchanged | measured from S1c | at chance |
| R1–R2 | falling | drift both ways, net small | rises for converted units only | at chance |
| R5 | near 0, exceptions listed | drift both ways, net small | as above | at chance |
| S5 | near 0 | unchanged | sharp rise in the CRT range | at chance (port tail) |
| S6a | unchanged | small deliberate drops | `.data` rises | at chance |
| S7 | near 0 | per conversion | per conversion | rises along size-exact prefixes |

## Coordination with running work

- **#416 (Keys.c).** Merged as #426. Its spare slot is the first S6a item.
- **#417 (GAME-003, Bezier).** Merged as #431. The unit is in `bezier.c`
  and goes into the list in S1b. The superseded declarations it keeps in
  `view.hpp` and `record.hpp` are declaration-only scoring deviations, which
  ticket 18 retires.
- **#413 (G98-024, handle table).** Merged as #432. The unit is in
  `g98_024.c` and goes into the list in S1b.
- **#412, #414, #415.** Open conversions. They are claimed units for S3.
  Each wires into the libraries now, or into the list if S1b lands first.
- **#425.** Merged. It restored the gate S1a starts from.
- **#382 (permuter).** Independent of the link. Point it at relocated or
  converted units (S4).
- **#389 (collapse dual-path code).** It feeds the port tail's emptying. It
  is a prerequisite of S7, not of S1–S6.

## Proposed tickets

The coordinator files these. "Parallel" means safe alongside conversions.

1. **Link the matching build non-incrementally, with a link map, as one
   build for the gate and binary progress** (S1a). Add `/INCREMENTAL:NO`
   and `/MAP` to the gate build, and make `make binary-progress` reuse it.
   Recheck `match_function.py` parity and drop the incremental-thunk
   emulation where the link no longer makes thunks. Measure `/OPT:REF` and
   adopt it only if no score changes. Refresh the baseline once under ADR
   0006 rule 1: the PR lists every changed function with its link cause (55
   up and 5 down expected, finding 516). Link-only PR, no source changes.
   Parallel.

2. **Add the link-order list and link game and gods98 objects explicitly in
   original order** (S1b). Add the ordered unit list (unit id and file): the
   9 converted units in place, legacy files at their median address, the
   port tail after gods98. Link the matching executable from it, followed
   by LIBC, `oldnames.lib` and the import libraries in original order. Build
   the native and reference libraries from the same list (ADR 0003 as
   amended). Handle objects that were never pulled before (`ptrstack.cpp`,
   `video_graph.cpp`, `engine_calloc`). Make `convert_unit.py` wire units
   into the list. Amend ADR 0005 step 2 and update
   `docs/repository-layout.md`. One baseline refresh. Needs ticket 1.
   Parallel; open conversions rebase their wiring line.

3. **Check link order statically and from the map** (S1c, part 1). Add a
   toolchain-free check that the list names each game and gods98 unit once,
   in spec order, and that each listed file holds only that unit's markers.
   Run it in `make tools-test` and in CI. Add a map-based check of object
   contribution order against the list for matching builds. Needs ticket 2.
   Parallel.

4. **Extend whole-binary progress with layout measures** (S1c, part 2).
   Bump `binary_progress.py` to schema 3. Add:
   - object-order inversions from the map;
   - unit-relative identity with absolute operands masked;
   - the size-exact prefix;
   - per-unit data-offset agreement.

   Update the PR progress report and its freshness inputs. Run `make
   tools-test`. Needs ticket 1. Parallel.

5. **Build `relocate_unit.py`: verbatim unit relocation** (S2a). One unit
   into `src/<lib>/<stem>.cpp`:
   - move its marked functions and owned globals verbatim, in original
     order;
   - move the file-static closure it alone uses;
   - give shared statics external linkage through a private
     `<file>_shared.hpp`, without editing shared headers;
   - keep the source file's include order;
   - update the list, CMake and Meson;
   - record body hashes before and after for the verbatim check;
   - print the footprint and a PR checklist.

   Prove it on one small unit (GAME-001 Advisor), with the gate and native
   tests green. Needs ticket 2. Parallel.

6. **Classify baseline drops as drift or regression** (S2b). Make
   `match_baseline.py check` normalise registers, stack offsets and branch
   targets in each dropped function's Reccmp diff. Report the drop as drift
   (rule 2) when the normalised listings are identical, and as a regression
   otherwise. Test on the drift cases in findings 508–513. Make the entropy
   review mechanical for relocations and conversions. No prerequisites.
   Parallel.

7. **Relocate wave R1: small and medium game units** (S3). Relocate
   GAME-001, GAME-004–009 and GAME-011–016, except claimed units. Many of
   their functions now sit in `gods98/view.cpp` and `record.cpp`. One commit
   per unit, one or two PRs. Gate under ADR 0006 rule 2, report the
   inversion delta. Needs tickets 2, 5 and 6. Parallel.

8. **Relocate wave R2: gods98 units** (S3). Relocate every gods98 unit not
   converted or claimed:
   - G98-001–003, 005, 008–017, 020, 025, 028, 030, 031;
   - this splits `view.cpp`, `surface.cpp`, `resource.cpp` and `record.cpp`;
   - it takes Files, registry and WAD code out of `game/startup_config.cpp`.

   G98-001 and G98-012 stay C++. Needs tickets 2, 5 and 6. Parallel.

9. **Relocate wave R3: FrontEnd, HelpWindow, Info and Interface** (S3).
   Relocate GAME-017–020, draining `frontend.cpp`, `message.cpp` and
   `select.cpp`. Announce a short freeze on `frontend.cpp`. Needs tickets 5
   and 6. Parallel, except for the freeze.

10. **Relocate wave R4: Lego.c, GAME-022 and LegoObject.c** (S3). First
    decide from the data whether Lego.c is two objects near `0042f350`.
    Update the spec and finding 505 if it is. Then relocate GAME-021, 022
    and 023 out of `level.cpp` and `entity.cpp` (about 500 functions).
    Announce a short freeze on `level.cpp` and `entity.cpp`. Needs tickets 5
    and 6. Parallel, except for the freeze.

11. **Relocate wave R5: the rest of the game segment** (S3). Relocate
    GAME-002, GAME-010 and GAME-024–050 (except GAME-034 and claimed units).
    This drains `crt.cpp`'s game functions. Afterwards every game and gods98
    function is in its unit file. Report the remaining inversions with
    causes (#388's second criterion). Needs tickets 5 and 6. Parallel.

12. **Take the CRT from LIBC.LIB and restore library order** (S5):
    - stop linking `src/crt/` into the matching build;
    - retire the `/include` directives that original references now make
      redundant;
    - move `trnsctrl.cpp`'s routines out of `seh_runtime.cpp`;
    - order the import libraries as d3drm, ddraw, avifil32, dinput, dsound,
      kernel32;
    - provide the late members through `oldnames.lib`;
    - check the LIBC member order in the map against the CRT units.

    Needs ticket 11 for `crt.cpp`'s game functions, and ticket 3. Parallel.

13. **Retire scoring-only storage and the named-section padding** (S6a):
    - remove the `.data$zzzzzz`, `.data$yyyyyy` and `.data$cmdpool`
      devices;
    - replace `key_names`' spare slot with a storage-neutral device
      (annotate the next table, or a Reccmp `data_sources` row, or accept
      96.77%);
    - make `Deviation (scoring only)` the required comment phrase and an
      ADR 0005 checklist item, and add it to the declaration-only cases
      that already exist (`view.hpp`, `record.hpp`; ticket 18 retires
      them).

    Expected score drops are recorded and approved, not restored. Needs
    ticket 4. Better after ticket 9. Parallel.

14. **Characterise LINK 5.0's common-symbol allocation order** (S6b). Use a
    few small links with the matching toolchain: vary names, sizes, defining
    objects, reference order and library position. Test the rule against
    the original's common region from `00549520`. Write a finding. Decide
    whether common order can be reproduced and how it is checked. No
    prerequisites. Parallel.

15. **Check each unit's data at unit-relative offsets** (S6c). For each
    unit, compare each `GLOBAL` and `STRING` annotation's offset within the
    unit's section contribution with its original offset. Report the units
    that agree fully. Add the check to ADR 0005 step 8. Needs tickets 1 and
    4. Parallel.

16. **Identify the `{1,2,4,-1}` `.rdata` header table** (S6d). Find the
    header that gives 19 gods98 objects and one game object a private
    16-byte table, and include it in those units so their `.rdata` order
    matches. Best after several gods98 conversions. Parallel.

17. **Decide Meson's future**. Either retire `meson.build` and the
    per-directory Meson files, or keep their source lists in step with the
    link-order list through the relocation tool. Until this ticket is
    decided, relocation PRs update both. No prerequisites. Parallel.

18. **Retire declaration-only scoring deviations** (after S3). After R5,
    delete in one PR every superseded declaration that a shared header
    keeps only to hold its includers' declaration count, starting with
    `view.hpp`'s six Bezier declarations and `record.hpp`'s in-place
    `scene_object_mode_is_one` (#431). Refresh the baseline under entropy
    rule 2, with ticket 6's classifier. A conversion that shrinks such a
    header to its own unit earlier may retire the ones in it. Tag every
    remaining case with `Deviation (scoring only)` meanwhile (ticket 13
    sets the phrase). Needs ticket 11 and ticket 6. Parallel.
