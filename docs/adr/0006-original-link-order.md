# The matching build links one object per original unit, explicitly and in original order; units move to their files before they are converted

**Status:** proposed (issue #388). **Amends**
[ADR 0003](0003-per-subsystem-static-libs.md) (what the libraries are for)
and [ADR 0005](0005-original-shape-c-units.md) (recipe steps 2 and 4,
the entropy policy). [ADR 0004](0004-miniwin-native-port.md) is unchanged:
the native port stays the primary target.

The staged plan and proposed tickets are in
[`docs/plans/388-original-link-order.md`](../plans/388-original-link-order.md).
The evidence is [finding 505](../findings/505-original-translation-units-and-link-order.md)
(the unit map) and [finding 516](../findings/516-msvc5-link-order-and-layout-experiments.md)
(the linker experiments and measurements).

## Context

`binary-progress.json` on `main` shows 0 functions identical at their
original address. Byte identity in `.text` at original addresses is at
chance, and function order has 871,052 reversed pairs. Conversions raise
per-function scores, but they cannot move these numbers. Function and data
addresses come from unit boundaries and link order:

- Today's matching build lays out each source file as one contiguous run, in
  static-library pull order (finding 516).
- 42 of 61 marked source files mix units.

## Decisions

### 1. Link order comes from one ordered list of unit objects

The matching build links every game and gods98 unit as an **explicit
object, in original order**:

- the 50 game units in spec order (the original's alphabetical project
  order);
- then the 29 gods98 units in spec order;
- then the *port tail* (decision 4);
- then the libraries: LIBC (default library), `oldnames.lib`, and the import
  libraries in the original's thunk order (`d3drm`, `ddraw`, `avifil32`,
  `dinput`, `dsound`, `kernel32`, then the rest). *Superseded by the
  addendum of 2026-11-12: `kernel32`, LIBC, `oldnames`, then the import
  libraries in descriptor order.*

One **link-order list** holds the order: one entry per unit, giving its
unit id and source file. It is checked against
`docs/coverage/translation-unit-ranges.csv`. It is the only authority on
link order. CMake builds the matching executable from it. The native and
reference builds use the same list to build their libraries, where order
does not matter. The ticket that adds the list decides its format; the
CMake-native list with a tool check is preferred.

**Why explicit objects.** LINK 5.0 keeps command-line order for objects in
every section (finding 516).

- **Library pull order** is not used because it follows references. It would
  reproduce the original only when every object references the same symbols
  in the same order. The gods98 order in finding 505 "fits library-member
  pull order"; with explicit objects the same order needs no such fit. Every
  original object was pulled, so linking all of them explicitly loses
  nothing.
- **`/ORDER`** is not used because it cannot name `static` functions and
  does not order data. Only object order places `.data`, `.bss`, `.rdata`
  and the first-user literal COMDATs.

**Cheap verification**, in order of cost:

1. A static check that the list names every game and gods98 unit once, in
   spec order, and that each listed file holds only that unit's markers.
   It needs no toolchain, so it can run in `make tools-test` and in CI.
2. A map check. The matching link writes `/MAP`, and a tool compares the
   order of object contributions with the list.
3. The whole-binary inversion counts.

### 2. The matching build links without incremental mode

Calls to explicitly linked objects go through incremental-link thunks
(finding 506, step a). This is why ADR 0005 step 2 forbade adding units to
the executable's sources. The gate build therefore switches to
`/INCREMENTAL:NO`, as `make binary-progress` already does. The gate build and
the binary-progress build then become one build, which halves the matching
builds a PR runs on the shared machine.

Switching scores 11 more exact functions; 55 functions go up and 5 go down
(finding 516). The switch refreshes the baseline once, in a PR that changes
nothing else.

That PR also measures `/OPT:REF`. With `/DEBUG`, LINK defaults to
`/OPT:NOREF` and keeps unreferenced COMDATs; the original was probably
linked without `/DEBUG`. Adopt `/OPT:REF` only if it changes no score.

*Amended by the link-mode addendum of 2026-11-12 (#562, finding 576): the
matching build keeps two links of the same objects, and the gate takes
`/OPT:REF` only when no annotated function is unreferenced.*

### 3. Units are relocated before they are converted

Restructuring separates two operations:

- **Relocation**: move a unit's existing functions and the globals they own,
  **verbatim**, into the unit's own file, in original address order. C++
  stays C++ (`<stem>.cpp`). Contracts do not change, and no shared header is
  edited. Finding 506 step (a) showed that this alone does not change scores.
  It is what fixes link order.
- **Conversion**: the ADR 0005 rewrite into original shape (`<stem>.c`).

A move changes no contract, so relocation does not depend on the call graph.
It can go in any order, including inside the 43-unit game cycle. Conversion
keeps ADR 0005's bottom-up order.

- A conversion PR is also its unit's relocation, so units with a conversion
  in flight are not relocated separately.
- A relocated unit's conversion footprint shrinks to its own file, its
  callees' headers and its callers.

The critical path to the issue's first two acceptance criteria (one file
per unit, inversions near zero) is **relocation**. Byte identity at original
addresses is a later goal (decision 6), and its critical path is conversion
plus data layout.

### 4. File layout

- **One unit, one file.**
  - Game unit GAME-*n*: `src/game/<stem>.c` (converted) or `<stem>.cpp`
    (relocated, still C++).
  - gods98 unit G98-*n*: `src/gods98/<stem>.c` or `<stem>.cpp`.
- **Stems are lowercase.**
  - A named unit uses its name: `advisor`, `aitask`, `lego`, `legoobject`.
  - A numbered unit gets a descriptive stem when it is relocated. The
    link-order list records it.
  - Alphabetical order of stems is not required: the list fixes the order,
    not the name.
  - If a stem clashes with a header that already exists, use a variant, as
    `soundsys` did.
- **A unit file holds only that unit's original functions and data, in
  original order.**
- **Port-only code** goes into the *port tail*: hook tables, adapters, test
  accessors, the `platform/` entry points and the drained remainder of
  today's mixed files.
  - It is linked after the last gods98 object. Explicit objects always
    precede library members, so the tail shifts LIBC until it is empty. That
    is accepted.
  - Port-only code that must sit in a unit file (a test accessor of `static`
    state) comes after the original functions and is **compiled out of the
    matching build** by a matching-build-only macro. This amends ADR 0005
    step 4, which put accessors last but kept them in the matching build.
- **Headers.**
  - A relocated unit keeps today's C++ headers. Relocation gives moved
    helpers and state external linkage through a private header next to the
    unit. It does not split shared headers, because that would cause drift
    in every includer.
  - Conversion creates the unit's C `<stem>.h` (ADR 0005 step 6).
- **`src/crt/`** holds no game or gods98 code once it is drained. In the
  matching build the CRT comes from LIBC.LIB. `crt/` keeps only the
  native-build stand-ins and is not linked into the matching build.

### 5. Data must land where the original put it

For data to land at original addresses, these must hold, in this order:

1. **Object order** (decision 1). It places each object's `.rdata`,
   `.data`, literals and per-object `.bss`.
2. **Each object's own contents in original order.**
   - Its plain `.data` comes first, in address order.
   - Then its literals: they are COMDATs that the first object in link order
     keeps, and each function emits them in reverse order of use. They
     follow from converting the functions; literal placement then needs
     nothing extra.
   - Its private `.rdata` constants, in first-use order.
   - Its `.bss` is one typed globals struct (ADR 0005 step 3), because MSVC
     lays out separate statics by name hash (finding 512).
3. **The same section sizes before it.** A section's address depends on the
   sizes of the sections before it. So `.text` must end in the same page as
   the original before any absolute operand can match.
4. **Common symbols** (from `00549520`): their allocation order is not yet
   understood (finding 516). It is characterised in its own stage. The
   region comes last in `.bss`, so deferring it moves nothing else.
5. **No storage made up for scoring.**
   - The named-section padding in `frontend.cpp`, `audio.cpp` and `cmd.cpp`
     goes.
   - So does any storage whose size, alignment, order or section differs
     from the original's.

**What waits:**
- common-symbol order;
- LIBC's data;
- the `{1,2,4,-1}` `.rdata` header tables (finding 505 rule 4), until their
  header is identified;
- whole-section byte identity.

None of these blocks relocation or conversion. Data identity is first
measured per unit (unit-relative offsets), not at absolute addresses.

**Deviations made only for scoring.** Some conversions change the program
only to help the score. There are three kinds:

- **Storage deviations** change data. `keys.c` declared `key_names` with
  257 slots so the past-the-end operand got a name (finding 513). #448
  removed the spare slot (finding 580), so none remain.
- **Comparison deviations** change neither code nor storage. A larger
  rebuilt image can cover a value that the original uses as a plain
  constant past its own end (`0x800000`, finding 584). Reccmp then names
  the rebuilt operand. `tools/reccmp_plain_immediates.py` kept that operand
  plain until the gate's image ended at `0x7f5000` (finding 642); it was
  retired then (finding 647), and `make reccmp-compare` and `make pe-audit`
  now fail if the image covers `0x800000` again. None remain.
- **Declaration-only deviations** change no storage. They keep a symbol
  count. `view.hpp` keeps six superseded C++ declarations that nothing
  defines or calls, and `record.hpp` keeps `scene_object_mode_is_one`
  declared in place. Both hold every includer's declaration count, so
  unchanged functions do not drift (finding 514, #431).

From now on:

- **Storage stays original.** In the matching build, a scoring-only
  deviation must not change any variable's size, alignment, order or
  section. A deviation that does (like the spare slot) is allowed only
  while its region's data layout is not yet a goal. It must be retired before
  the data stage reaches that region.
- **Prefer annotations to storage.** Options:
  - an annotation of the variable that really follows. Once commons are in
    original order, Files.c's table at `005769e0` follows `key_names` in the
    rebuild too, so the spare slot can go;
  - a Reccmp `data_sources` CSV row that gives the original entity its
    size. It changes no code; prototype it before relying on it. For
    `key_names` it does not help: the row sizes only the original side, and
    the rebuilt side still names the next common (finding 580);
  - otherwise, accept the lower score. The bytes are identical, and the
    whole-binary report counts them.
- **Declaration-only deviations** move no data, so the data stage does not
  need them gone.
  - They are allowed under ADR 0005 entropy rule 1 because they keep
    existing declarations. Rule 3 still forbids adding inert pads.
  - They hold each includer's symbol numbering away from the original's,
    so they block the convergence ADR 0005 expects.
  - They are retired once the units that include the header are relocated
    or converted, because those PRs accept drift anyway. Relocation never
    edits shared headers (decision 4), so retiring them takes one cleanup
    PR after the last relocation wave, or the conversion that shrinks the
    header to its own unit.
- **Ledger.** Every scoring-only deviation of either kind carries the
  comment phrase `Deviation (scoring only)` (ADR 0005 entropy rule 4), so
  one grep lists them. The data stage closes the storage entries. The
  header cleanup closes the declaration-only entries.

### 6. Metrics and acceptance

- **Inversions.** The function-order inversion counts follow relocation.
  They should fall to near zero once every unit is relocated. Simulated on
  #425's tree: 873,206 reversed pairs there, about 650,000 from reordering today's
  files alone, about 0 when every function is in its unit file (finding
  516).
- **At-address identity.** Byte identity of `.text` at *original addresses*
  needs every byte before it, and the data it references, to match. It
  stays near chance until whole prefixes of the binary are size-exact.
- **New measures.** `binary_progress.py` should add:
  - **object-order inversions** read from the map;
  - **unit-relative identity**: each rebuilt unit compared with the
    original unit at the same offset from the unit's start, with absolute
    operands masked;
  - the **size-exact prefix**.

  These show restructuring and conversion progress where address identity
  cannot.
- **Acceptance.** The coordinator should consider restating #388's criterion
  "byte-identical `.text` at original addresses rises measurably" in terms of
  unit-relative identity, keeping at-address identity as the end goal.

### 7. Entropy policy for restructuring PRs

These additions to ADR 0005's entropy policy are about how the baseline gate
treats a restructuring PR:

1. **Link-only PRs** change how objects are linked but no source:
   - non-incremental linking;
   - the ordered list;
   - library order.

   Every score change in such a PR comes from the link. The PR refreshes the
   baseline once and lists the changed functions with their cause. No other
   change rides along.
2. **Relocation PRs** are entropy rule 2 by construction: the moved
   functions' source is unchanged.
   - A tool checks that each moved function's body is byte-identical to its
     old one.
   - A drop is accepted only when the normalised listings differ only in
     registers, stack slots or branch layout. A drift classifier in
     `match_baseline.py` checks this, so the classification is mechanical.
   - Any other drop is a regression. The PR reports the net effect.
3. **Keep commits separable.** Never mix relocation with conversion, renames
   or fixes in one commit. One commit per unit. Several units may share a
   PR, so one full matching build covers them.
4. **No pads.** Rule 3 stands: entropy is never offset with dummy
   declarations.

Moving a function out of a file renumbers the functions after it in that
file. Moving it into a new file gives it different neighbours. Both cause
drift, both ways. This is the cost of the restructuring and is accepted.

### 8. ADR 0003 survives, narrowed

The per-subsystem static libraries (`gods98`, `game`, `platform_glue`)
remain how the **native port and the reference build** group and link the
sources, and how tests link them.

- **Library membership follows the original segment.** A game unit is in
  `game`, a gods98 unit in `gods98`. Today's misplaced files fix this as
  they drain: game functions in `gods98/view.cpp` and `record.cpp`, and the
  gods98 Files/registry/WAD units in `game/startup_config.cpp`.
- In the **matching build** the libraries are not the link inputs for unit
  objects. Decision 1's list is.
- The original's 6 direct gods98→game call pairs keep the libraries mutually
  dependent, so the native link keeps repeating them.

**Meson** builds natively only and does not decide link order. Until a
separate ticket retires it or keeps it, relocation PRs update its source
lists along with CMake's.

## Addendum (2026-11-11): reshape commits

**Status: accepted by the owner.** It was proposed by #490 ([plan](../plans/490-relocation-conditional-code.md), [finding 539](../findings/539-conditional-relocation-blockers.md)), and it amends decision 3 and the entropy policy in decision 7.

**Reshape commits.** A relocation may be preceded by a *reshape*: a hand edit to source that changes no build's input.

1. A reshape counts as zero drift (ADR 0005 entropy rule 1) only when `tools/reshape_check.py` (#500) shows both of the following for every file it touches:
   - an identical token stream under the native, matching and MinGW reference configurations;
   - an identical matching-build object outside debug sections: bytes, relocations and symbol table.

   It refreshes no baseline. Its PR includes the check's output.
2. A reshape may only:
   - split a conditional head into whole alternative definitions, duplicating the head;
   - hoist branch-local helpers out of an alternatives group;
   - move annotations or comments;
   - rename a file-local macro;
   - close and reopen `#if` groups with equivalent conditions.

   It doesn't change a contract, collapse a dual path (#389), or edit a shared header.
3. Reshapes follow rule 3: one commit per file, never in the same commit as a relocation, conversion or fix. They may share a PR with the relocation they prepare.
4. The relocation tool moves a top-level conditional group whole and verbatim (#501). When it writes a private-header declaration for a conditional item, it wraps the declaration in that item's group condition, copied verbatim, and records it in the relocation CSV like a `static` removal.

## Addendum (2026-11-12): library order

**Status: proposed by #563** ([finding 574](../findings/574-import-directory-and-library-order.md)). It amends the library part of decision 1.

The matching build links its libraries in this order:

`kernel32, libc, oldnames, ole32, d3drm, ddraw, winmm, vfw32, dinput, dsound, user32, gdi32, advapi32`

The DirectX libraries the toolchain lacks are rebuilt from source in `cross/msvc5-importlibs/`, and `dxguid` follows the import libraries.

**Why.** Decision 1 derived its order from the import thunks alone. The import descriptors also follow command-line library order, and the original's start with `KERNEL32`. Scratch relinks under `/OPT:REF`, the original's mode, show:
- the descriptor order is the order of the import libraries;
- the thunk order is member pull order, and only referenced thunks survive.

Of six orders tested, only the one above reproduces all three properties of the original:
- the descriptor order;
- LIBC's code before the thunks;
- `RtlUnwind`'s thunk last, after `dsound`'s.

`libc.lib` must be listed explicitly, after `kernel32.lib` and before `d3drm.lib`. Its position relative to `ole32.lib`, and that of `oldnames.lib`, are not fixed by the evidence. In the `/DEBUG` gate (`/OPT:NOREF`), `kernel32.lib`'s unreferenced first-pass thunks precede LIBC until the link mode changes (#562).

## Addendum (2026-11-12): link mode

**Status: proposed by #562** ([finding 576](../findings/576-link-mode-and-unreferenced-code.md)). It amends decision 2's `/OPT:REF` paragraph.

The original was linked without `/DEBUG`, so `/OPT:REF` was in effect (finding 563). Reccmp needs the linker's PDB, which LINK 5.0 writes only with `/DEBUG`, and it reads no map file. The matching build therefore keeps **two links of one set of objects**:

1. **The gate** links with `/DEBUG` (so `/OPT:NOREF`). It gives Reccmp its PDB, the scores and the baseline.
2. **The original-mode link** relinks the gate's objects without `/DEBUG` (`make opt-ref-link`, `tools/opt_ref_link.py`; no recompile). `make match-progress` runs it, and `binary-progress.json` records it as `opt_ref`:
   - what `/OPT:REF` removes, per link-order category;
   - the annotated and exact functions among the removed code;
   - the relinked image compared with the original.

At equal `/OPT` setting the two links differ only in `.rdata`: the 84-byte debug directory moves 102 items by 80 or 88 bytes, and the operands that refer to them change. Function addresses, `.data`, `.bss`, `.idata` and the entry point are equal.

**When the gate takes `/OPT:REF`.** On `1d31a556`, `/OPT:REF` removes 1,262 annotated functions, 528 of them exact, because their live callers are still port code. The gate's link takes `/OPT:REF`, with `/DEBUG` kept, once `opt_ref.annotated.full.removed` reaches its floor. The floor is zero, or the annotated subset of the original's own unreferenced functions while the objects keep `/Gy` (#553).
- The switch is a link-only PR (decision 7 rule 1).
- The whole-binary measures of `binary-progress.json` can then move to the original-mode link.

**Taken (2026-11-13, [finding 642](../findings/642-gate-takes-the-original-link-mode.md)).** The gate links with `/DEBUG /OPT:REF /OPT:NOICF`. `/OPT:NOICF` because the original folded no identical functions (finding 637). The floor is one function, LegoObject.c's unreferenced `00438390`, kept with `/INCLUDE` as the original's objects (no `/Gy`) kept it; a second `/INCLUDE` keeps crt.cpp's stand-in for LIBC's static `__shift` (`004907c0`), which Reccmp cannot name (`Deviation (scoring only)`, #447). No score fell. The caller switches that reached the floor landed in the same PR, so it is not link-only. The no-`/DEBUG` relink stays as the whole-binary link, and a `/DEBUG /OPT:NOREF` relink now measures what the gate's `/OPT:REF` removes.

**Rejected:**
- Stripping or relocating the debug directory after a `/DEBUG` link: the image has no `.reloc`, the shift is not uniform, and the PDB would no longer describe the patched executable.
- Pairing a PDB from one link with another link's executable: it loses 45 exact matches through the `.rdata` shift.

## Addendum (2026-11-13): library members and the port tail

**Status: proposed by #767** ([finding 658](../findings/658-libc-member-order-gods98-members-and-tail-library.md)). It amends decision 1 (every unit an explicit object) and decision 4 (the tail before LIBC). The owner decides whether it stands.

The matching build links, in this order:
1. the game units and gods98's first unit, Movie.cpp, as explicit objects (`LRR_LINK_EXPLICIT_LAST`);
2. each later gods98 unit from its own library, `unit_<stem>.lib`, in list order;
3. the libraries of the 2026-11-12 addendum (LIBC, `oldnames`, the import libraries);
4. the late units (finding 657);
5. the port tail and the CRT-range legacy files from one library, `lrr_tail.lib`.

The link names no `/ENTRY`.

**Why.**
- LINK 5.0 pulls a LIBC member when the first unresolved reference to it comes up. The original's LIBC order is therefore its referencers' order. When the entry point is not named, LINK infers it after the explicit objects. The original's `wincrt0.obj` follows Movie.cpp's LIBC references and precedes Container.c's, so the original's explicit objects ended with Movie.cpp, and the other gods98 units came from a library.
- With one library per unit, each searched before LIBC, the units keep list order in every section, as explicit objects do. The map check fails if one is pulled out of order.
- The port tail's references to LIBC came before LIBC's own references, and that pulled members early. From a library after LIBC, the tail is pulled in the first pass. The members only it needs are pulled in the second pass, after the import thunks, where the original has nothing of its own.
- `/ENTRY` made `wincrt0.obj` LIBC's first member.

Together these take the full function-order inversions from 5,568 to 801 pairs, and game-only from 17 to 6. LIBC's `__output` and `__NMSG_WRITE` reach 100%, and no function falls.

**What decision 1's reasons still say.** Explicit objects remain the rule for the game segment and Movie.cpp. Member libraries reproduce list order only while every member is referenced before its library is searched. Today they all are. A conversion that breaks this fails `make reccmp-check` through the map check. The link-order list is still the only authority on order.

**What decision 4 loses.** Tail objects that nothing references are not linked. Under `/OPT:REF` they contributed nothing but one `.bss` block (`sfx_port.cpp`'s, which nothing reads). The tail's code and data now follow LIBC's and the late units', so they no longer shift LIBC.

## Consequences

- The issue's acceptance criteria become reachable in stages:
  - one file per unit, and inversions near zero, through relocation;
  - byte identity, through conversion and the data stages.
- `convert_unit.py` wires new units into the link-order list instead of
  appending to library source lists. A relocation tool joins it.
- Each relocation and conversion PR still refreshes `match-baseline.txt`
  and `binary-progress.json`. These two files remain the serialisation
  point: the PR that lands second rebases and reruns `make
  match-progress`.
- The port tail sits between gods98 and LIBC until it is empty, so LIBC's
  addresses stay shifted until then. The tail empties as dual-path code
  collapses (#389) and port-only code is compiled out of the matching build.
- `docs/repository-layout.md`, ADR 0003's text and ADR 0005's recipe are
  updated by the PRs that implement decisions 1, 2 and 4, not by this ADR.
