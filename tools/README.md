# `tools/` — game-data tooling

## `lint_reccmp_annotations.py` — annotation ratchet

`make reccmp-lint` checks recognizable definitions in production C/C++ source
translation units under `src/` (excluding test files and `platform/native`),
and named entries in `docs/coverage/ledger.csv` without an address annotation.
It is a source heuristic, not a C++ parser; it scans conditional alternatives
without preprocessing and accepts nearby FUNCTION, STUB or LIBRARY markers.
It looks for the marker above the definition itself, skipping preprocessor
directives and their backslash continuation lines (finding 540).
It needs only Python's standard library, and runs in `make tools-test`.

The sorted `docs/coverage/reccmp-lint-baseline.txt` records existing debt.
Definition keys use scope, name and a normalized signature hash, never location;
identical keys are counted separately. Named ledger keys use address and name.
New entries fail; fixed entries are printed as `can be removed`. Run
`make reccmp-lint-update` to remove fixed entries. It refuses additions unless
explicitly approved with `make reccmp-lint-update
RECCMP_LINT_UPDATE_ARGS=--allow-growth`. A changed signature can appear as a
new and a fixed entry; review it before accepting growth. The heuristic cannot
distinguish identical file-local signatures in the same scope, but counts them.

## `link_order.py` — read and edit the link-order list (ADR 0006)

`src/link_order.cmake` is CMake code with one `lrr_link(...)` entry per line,
so `link_order.py` reads it with the standard library only (`parse`, `load`,
`sources`, `libraries`). `insert_unit` places a unit's entry in its original
position; `convert_unit.py` uses it. `python3 tools/link_order.py
[--platform winapi] [--sources]` prints the entries. Its tests run in `make
tools-test`. The field layout is described at the top of the list and in
`docs/building.md` ("Link order").

## `relocate_unit.py` — move one unit verbatim into its own file (ADR 0006, plan S2a)

*Relocation* (GLOSSARY.md) moves a unit's `// FUNCTION`-annotated functions
and the `// GLOBAL` state it owns, verbatim, into `src/<game or
gods98>/<stem>.cpp`. The functions go in original address order. The file
stays C++. It is not a conversion, and the relocated unit is still converted
later with `convert_unit.py`.

```sh
python3 tools/relocate_unit.py GAME-001 --dry-run     # plan, report, write nothing
python3 tools/relocate_unit.py GAME-001               # relocate Advisor.c into src/game/advisor.cpp
python3 tools/relocate_unit.py GAME-007 --stem placement   # unnamed units need a stem
python3 tools/relocate_unit.py --survey               # what blocks each unrelocated unit
make relocate-unit UNIT=GAME-001 RELOCATE_ARGS=--dry-run
```

**What it moves.**
- The unit's functions are the annotations in the unit's `.text` range. They
  may sit in several legacy files.
- The owned globals are the `GLOBAL` annotations in the unit's data ranges
  (`translation-unit-ranges.csv`). A range ends at its last item's start.
  The few bytes of slack after it count only when no other unit's range
  holds the address, so a unit never claims its neighbour's first item
  (finding 527).
- The file-static closure moves too: static helpers, static variables,
  forward declarations, file-local types and macros that only the moved code
  uses.
- Unmarked code with external linkage stays where it is. So does an item
  annotated for another unit, even if only moved code uses it.
- Each item keeps its leading comments.
- A `typedef` inside a function body is local: it shadows a file-scope name
  of the same spelling, so it is not a use of that name (#525). The scanner
  does not track block scopes, so a use of the file-scope name elsewhere in
  the same function is missed; the relocated file's compile catches it.

**Named namespaces** (#471). A member of `namespace X { }` moves verbatim
into a reopened `namespace X { }` in the unit file. Consecutive members share
one block, and order is unchanged. An out-of-line `T X::f() {}` definition
moves as written. Names are scoped (`n`, `X::n`) and resolve as C++ looks
them up:
- a qualified `X::n` resolves exactly;
- a bare `n` inside `X` resolves to `X::n` before the global `n`;
- a bare `n` at global scope resolves through the file's `using` items.

Declarations the private header needs for namespace members go inside
`namespace X { }` there too. Forward declarations in the unit file are
written the same way. A file-scope `using namespace X;` or `using X::n;`
that moved global-scope code depends on is copied after the includes, and
stays in the source as well. A namespace block that the relocation empties
is dropped from the source when `X` stays declared: by another block in the
file, or by an included header.

**Stems.** The stem defaults to the unit name in lower case. Unnamed units
need `--stem`. When the default names an existing file (`input.h`, or the
legacy `frontend.cpp`), the tool uses `<stem>_unit` and says so;
`--stem` overrides it.

**The new file's layout.**
1. The includes of the source with the most moved functions, in order; then
   the other sources' unconditional includes that are not already there.
   Spellings are re-spelled only when they would resolve to a different
   file. A top-level `#if` group that holds only `#include` lines (a
   platform-header group) is copied verbatim, in its place, when each of
   its includes resolves the same from the unit file (#525). Other
   conditional includes are listed in the plan's notes and not copied.
2. Types and macros.
3. The owned globals in address order, then the closure variables. A
   variable moves up to just before its first user.
4. The forward declarations the new order needs.
5. The unit's functions in address order.
6. Static helpers last, because they are port-only code (ADR 0006 decision
   4).

**Sharing without editing shared headers: per-relocation private headers
(#496).** Sometimes moved code and the code left behind both need something.
- A static gets external linkage: `static` is removed and the static is
  declared in a private header. An array defined as `T a[] = { … }` is
  declared with the bound its one-dimensional brace list implies, so a
  `sizeof a` in the other file still compiles (#525).
- A file-local type or macro moves into a private header.

Each relocation of unit U from legacy file L writes its own pair, next to L:

| Header | Holds | Included by |
| --- | --- | --- |
| `<l>_for_<u>.hpp` | types and macros moved out of L, and declarations of L's items that U uses | U and L |
| `<l>_from_<u>.hpp` | declarations of U's items (moved out of L) that L still uses | L only, and U only when it defines a const object, which needs the extern first |

Both are written once and never edited again: a later relocation from L
writes its own pair. A header that already exists is refused. So:
- a later relocation never changes what an earlier unit file sees. Its
  declaration sequence, and the symbol counters MSVC derives from it (finding
  538, rule B), stay fixed;
- a unit file never sees declarations of its own definitions (finding 538,
  rule A, the cause of `0x0040e9c0`'s drop). A function the unit uses before
  defining it gets a forward declaration in the unit file instead.

Each header includes the earlier private headers of L whose names it uses,
so it is self-contained. A new unit file copies L's include list but keeps
an earlier private header only when it uses a name the header declares.

Headers that other files include are never edited (finding 506).

**Placement.** The unit file includes its `for` headers after its own
includes. L includes each new header after the last `#include` that comes
before L's first use of one of the header's names, so it is never later
than that use; a late include further down the file does not count (finding
527). The tool re-scans L and refuses if code before an include uses that
header's names.

**Migration** (`--migrate-private-headers`, #496). Before #496, each legacy
file had one `<l>_shared.hpp` that every relocation appended to, and every
unit from L included it. The migration gives each such unit U a verbatim
snapshot of that header, `<l>_for_<u>.hpp` (only the guard and the comment
differ), and points U's include at it. The unit's declarations do not change,
so the migration itself changes no listing. `<l>_shared.hpp` stays as it
is, is included only by L, and is never written again. Recorded in
`docs/coverage/relocations/private-header-migration.csv`.

**Wiring.** The tool also updates:
- the link-order list (a `unit` entry at the unit's original position,
  `link_order.insert_unit`);
- the Meson `game_sources` or `gods98_sources` array;
- the `current_source` column of `translation-units.csv`.

Each source it edits keeps the key `link_order.py check` requires, the lower
median of its remaining `FUNCTION`/`LIBRARY` markers: `link_order.place_legacy`
rewrites the key and moves the entry into address order. A legacy file left
with no original function becomes a `tail` entry (`link_order.retire_legacy`). Globals that a conversion annotated in its own
`.c` file (pooled literals) stay there, with a note in the report.

**Verbatim check.** Every moved item is hashed before and after: SHA-256 of
the item's text, with whitespace and line endings normalised. The hashes go
to `docs/coverage/relocations/<unit>.csv`. The tool re-scans the text it
generated and refuses if:
- any body differs;
- a head differs other than by a recorded `static` removal;
- an annotation is lost or duplicated.

A second run finds the unit relocated and does nothing. A half-done state
(the unit listed, but annotations still elsewhere) is refused.

**Overloads, splits and anonymous namespaces** (#485).
- **Overloads.** A name with several function definitions in one file is
  keyed by its parameter types. An annotated overload moves, an
  unannotated one stays, and a use of the name counts as a use of every
  overload.
- **Several declarators, or a type and a variable, in one declaration.**
  When all the names move together, the declaration moves verbatim. When
  they would go different ways, or one has to be shared, the declaration
  must be split. That is a source change, so the relocation refuses and
  names the step:

  ```sh
  python3 tools/relocate_unit.py GAME-005 --split-declarations --check-layout   # rewrite, check, write
  git commit -m "Split ... for the GAME-005 relocation"                       # its own commit
  python3 tools/relocate_unit.py GAME-005                                       # then the relocation
  ```

  The split step only rewrites the declarations the relocation needs split:
  - `static int a = 0, b = 0;` becomes one declaration per name;
  - `static struct T {…} v[3];` becomes `struct T {…};` and
    `static struct T v[3];`; an untagged type is refused. Annotations in
    the leading comments move below the type, onto the first variable,
    because they name the variable (#525).

  Each split is recorded in `docs/coverage/relocations/<unit>-splits.csv`,
  with hashes before and after, and the result is re-scanned for one name
  per declaration and unchanged annotations. `--check-layout` compiles each
  split source before and after with MSVC 5.0, using `match_function.py`'s
  compile commands, and compares every data symbol's section, offset and
  order and every data section's size. Symbol order is compared because
  common allocation follows it (finding 525). A changed layout is refused
  unless you pass `--allow-layout-change`.
- **Anonymous-namespace items shared across files.** This mirrors the static
  rule. Such an item leaves its anonymous namespace, gaining external
  linkage, and is declared in the private header, in its outer namespace if
  it has one.
  - A staying item gets the anonymous block closed before it and reopened
    after it, so source order is kept, and blocks left empty are dropped.
  - A moving item lands in the unit file outside any anonymous block.
  - A file-local type or macro the header needs moves to the header in the
    outer namespace.

  Each change is recorded ("left the anonymous namespace") and covered by
  the hash and container checks.

- **Whole conditional groups (#501, ADR 0006 addendum item 4).** A
  top-level, non-nested `#if` group whose items form one cluster moves
  whole and verbatim, from its `#if` line to its `#endif` line. A group's
  macros, types and prototypes always count as one cluster; data must
  connect to them by name. Examples
  are macro alternatives such as a calling-convention macro, a single
  conditional declaration or datum, or a group with `#include` lines. The
  group goes to the unit file when only moved code uses it, or to the
  `for` header when both files use it and it holds only macros, types,
  prototypes and `#include`s. The hash check covers the group's whole
  text.
  - **An annotated datum in a group (#525).** A group may hold an
    annotated item only when it is a single-branch group around one
    `GLOBAL`-annotated variable, with the annotation directly on the
    declaration, inside the `#if`. Reccmp reads the line after a `GLOBAL`
    annotation as its declaration, so above the `#if` it would name the
    variable `defined`. Such a group moves as that owned global.
  - **Macros in such a group** are alternatives, not redefinitions: one
    definition per branch is accepted. An `#undef` after the macro's last
    use stays where it is and is reported in the plan's notes. An `#undef`
    before a later use, or a second definition, still refuses.
  - **Guarded declarations.** A `static` in a single-branch group, or in a
    group's first branch, that both files use is exported as usual. Its
    private-header declaration is wrapped in the group's `#if` directive,
    copied verbatim, and recorded in the relocation CSV as a
    `guarded-declaration` row. The `static` removal gets its own row. If
    the file that gets only the declaration also uses one of the group's
    macros or types, the relocation refuses; hoist the data into its own
    group first.
- **Statics a private-header item needs (T2, #501).** When a macro, type or
  declaration that goes to the private header uses a file `static`, that
  static is exported by the static rule instead of refused. `inline` and
  anonymous-namespace items still refuse.

- **#514 additions.**
  - *Nested named namespaces* (`namespace A { namespace B { } }`) are
    scope `A::B`. The unit file and the private headers write them as
    nested blocks, because MSVC 5.0 has no `namespace A::B {`.
  - *Shared unannotated `inline` functions* go verbatim to the `for`
    header, after its declarations, as a type or macro does. An annotated
    inline function that both files use still refuses.
  - *Out-of-line class members* (`T C::f() {}`) belong to `C::f`. The class
    definition follows them like any type: into the unit file, or into the
    `for` header when both files use it. They get no extra declarations,
    since the class declares them.
  - *Anonymous-namespace functions used before their definition* get a
    forward declaration in an anonymous namespace, which is the same one in
    any one translation unit.
  - *Unannotated helper functions* may sit in a whole `#if` group, for
    example G98-009's COM-call helpers. Such a group lands in the unit file
    before the unit's functions.
  - *A name clash* refusal reads both definitions' annotations. It says
    whether they are one original global (unify them), two (rename the
    static), or a port-only definition (rename it). Either way it is a
    source fix, not a relocation.

**Refusals.** The tool refuses, with the file and line, rather than guess. It
refuses:
- definitions inside `#if` blocks, except a group whose branches each hold
  one whole definition of the same function (the naked-`__asm`/C pairs)
  and the whole groups above. The refusal names the group and why it
  cannot move whole: it holds an annotated function definition among other
  items, a nested group, an `#undef` or other directive, or unrelated items;
- a guarded declaration for a definition outside a group's first branch
  (its condition would have to be synthesised);
- directives that split a declaration;
- functions defined in headers;
- qualifications with several levels, and macro-built definitions;
- a linkage block inside a namespace, or a namespace inside an
  `extern "C"` block;
- an untagged type defined together with a variable, when it has to be
  split;
- shared annotated `inline` functions;
- a declaration that needs splitting, unless you run `--split-declarations`
  first (below);
- macros that are redefined, or `#undef`'d before a later use;
- two items on one line;
- name clashes between the moved files or with another file;
- backend-only sources;
- a `--stem` that clashes with an existing file;
- a needed `using` whose namespace no included header declares.

The relocator adds each new C++ source to the literal `game_sources` or
`gods98_sources` Meson array, which the native parity check reads.

`--survey` tries every unrelocated unit and tabulates the blocking constructs.
A unit that needs `--split-declarations` is reported "ok after" only when its
relocation also plans on the split text. The survey applies the split in a
scratch copy of `src/` and `docs/coverage/`; otherwise it reports "refused
after `--split-declarations`" with the reasons (#514). See findings 523,
530, 537, 541, 545 and 548.

The scanner is `cxx_items.py`: a comment- and literal-aware top-level item
splitter (stdlib only), not a C++ parser. Tests: `tools/tests/test_relocate_unit.py`
on synthetic sources (`make tools-test`).

## `convert_unit.py` — scaffold an original-shape conversion (ADR 0005, step 0)

Writes the mechanical starting state of a conversion for one *original
translation unit*, so the converting agent spends its time on matching
functions. It does not write bodies, choose signatures where the pseudocode is
ambiguous, migrate callers or edit existing implementations.

```sh
python3 tools/convert_unit.py G98-029 --stem registry   # scaffold into src/, wire the link-order list and the boundary test
python3 tools/convert_unit.py Keys.c --dry-run           # print the report, write nothing
python3 tools/convert_unit.py GAME-003 --out /tmp/scratch  # write into a scratch copy of the tree
python3 tools/convert_unit.py G98-027 --footprint-only [--json]
python3 tools/convert_unit.py GAME-034 --compare --stem priorities   # diff against a hand conversion
make convert-unit UNIT=G98-029 CONVERT_ARGS="--stem registry"
```

`UNIT` is a unit id from `docs/coverage/translation-unit-ranges.csv` or a unit
name (`Keys.c`). The file stem is the name without `.c`; unnamed units default
to the lower-case id (`g98_029`), so pass `--stem` to choose a descriptive one.
The tool refuses to overwrite existing files without `--force`, and only
handles `game` and `gods98` units.

**What it writes**

| Output | Contents |
| --- | --- |
| `src/<game or gods98>/<stem>.c` | SPDX header; the unit's `.bss` range as one typed `<Stem>Globs` struct with fields named by offset (`f_0000`, each commented with its address); the unit's `.rdata`/`.data` items with their initial values, in address order, each with `// GLOBAL: LEGORR 0x…`; data that no other unit touches outside the attributed ranges (the common region) as plain file-scope globals; the unit's string literals as a comment table of `STRING` markers; then one stub per function in original order, each with `// FUNCTION: LEGORR 0x…` and the Ghidra pseudocode above it |
| `src/…/<stem>.h` | C boundary header (`extern "C"`, include guard) declaring the stubs |
| `src/link_order.cmake` | a `unit` entry for the new `.c` file, in its library (`gods98` or `game`), placed after the last unit or legacy entry whose original address is not above the unit's start, so the matching build links it in its original position (ADR 0006). The `.c` suffix makes it C in the matching build |
| `src/game/test_c_boundaries.cpp` | includes for the new header and for the C++ headers that declare the unit's functions today |
| report (stdout, `--report FILE`) | the conversion footprint, a "before" score table and the recipe checklist for the PR description |

Stubs are `void scaffold_<name>(void)` with the existing port name after the
prefix, and every function the port already defines keeps that definition. The
prefix lets the stubs link beside the old code. When you write one, give it its
final name, delete the old definition and its declaration, and switch the
callers.

**Annotations and the gate.** The stubs carry live annotations, so each
address is annotated twice until you delete the old definition. Reccmp keeps
one of them: the old function then drops out of the report (as "newly
unmapped") and `make reccmp-check` **fails until the old definitions are
deleted**, as does any caller scored against it. Only the final conversion PR
has to pass the gate.
In a trial scaffold of the nine leaf units, the full matching build compiled
and linked, and the gate reported 32 old functions unmapped and 3 callers
below baseline for that reason. The report lists the existing `// GLOBAL`
annotations that the scaffold now repeats. Delete the old definitions in the
same change, and run the gate after that.

**Data layout.** Addresses come from disassembling the unit's functions in
`reference/LegoRR.exe`. Each referenced address starts a field that runs to the
next one, typed by the widest access (an x87 operand is `float` or `double`).
Indexed accesses make arrays, and a stride larger than the access (`[ecx*8 +
base]`) makes an array of structs. Adjacent scalars of one type (four or more)
are merged into an array, except in `.rdata`/`.data`. Constants that look like
addresses but are not (an `imul` immediate, an odd value in the common region)
are ignored. The tool cannot see nested types, so a hand-made `Area2F` shows as
four floats and an array indexed with a constant element splits at it. Items
over 512 bytes in `.data` are declared uninitialised with a TODO.

**Before scores.** The table comes from `match_function.py` for every function
of the unit (about 6 s; Wine through `distrobox`, see below). When it cannot
run, the table falls back to the committed *match baseline*; `--scores
baseline|none` choose it.

### Validation against the hand conversions

`--compare` regenerates the mechanical output for an already converted unit and
compares it with the committed files: function annotations and order, `GLOBAL`
and `STRING` addresses, the struct's field starts and extent, the CMake wiring
(strip the unit's entry, rewire, compare membership) and the boundary-test
includes. The checked-in test `tools/test_convert_unit.py` runs it for
Priorities.c (GAME-034), Config.c (G98-006) and utils.c (G98-004), and pins the
differences:

| Unit | Result |
| --- | --- |
| GAME-034 | 14 of 14 functions and addresses in the same order. Struct starts at `00500f00`, ends at `005013c0`, with every hand-made field start present. The tool splits arrays and `Area2F` at five more addresses (`00501118`, `00501264`, `005013ac`, `005013b0`, `005013b4`) because it sees constant-index accesses and cannot see nested types. One extra `GLOBAL` (`0049e654`, an x87 constant that the hand version writes as a literal). The tool also lists three `STRING` addresses that the hand version does not annotate |
| G98-006 | 16 of 16 functions in order. The struct is field-for-field identical (`00506098`–`00506524`). The hand version puts the `.bss` struct before the `.rdata` constants and the tool orders by address. The hand version's shared literals (`004a1cd4`, `004aab18`) and empty string (`004b8a14`, owned by GAME-004's `.bss`) are reported by the tool as pooled with other units rather than defined |
| G98-004 | 7 of 7 functions in order, no state, no differences. The four own `STRING` addresses and the two pooled ones (`004a4120`, `004a4124`) are listed. The hash multiplier `0x600937` (`mov edi, imm`) is not mistaken for an address |

CMake wiring and `c_boundaries` entries match for all three (the tool appends
to a source list where a hand edit placed it in the middle; the order of a
source list is irrelevant).

## `conversion_footprint.py` — conversion footprint library

The *conversion footprint* of a unit is the set of source files and headers an
ADR 0005 conversion touches: its new files, the files that hold its functions
today, its callees' headers and its callers. Two conversions whose footprints
share no header can run in parallel. The module is stdlib-only and is what
`convert_unit.py` reports with; it also works as a library and from the
command line:

```python
import sys; sys.path.insert(0, "tools")
from conversion_footprint import compute_footprint, header_overlap

a, b = compute_footprint("G98-029"), compute_footprint("GAME-003")
a.headers                 # existing headers the conversion is expected to edit
header_overlap(a, b)      # frozenset(): no shared header, so a wave can hold both
a.to_json()               # the same document as `--json`
```

```sh
python3 tools/conversion_footprint.py G98-029 --json
python3 tools/conversion_footprint.py G98-019 G98-024 GAME-003 --table   # summary and overlaps
```

`compute_footprint(unit, root, stem=None, with_data=None)` reads
`translation-unit-ranges.csv`, `translation-units.csv`, `call-graph/edges.csv`
and `src/`, and returns a `Footprint`:

| Field | Meaning |
| --- | --- |
| `unit_id`, `unit_name`, `unit_kind`, `stem`, `confidence`, `text_start/end`, `ranges` | the unit, its address ranges (`rdata`, `data`, `string`, `bss`; the end is the start of the last item) |
| `new_files` | the `.c` and `.h` to create |
| `functions` | per function in address order: address, existing port name, corpus name, current source and line, size, headers declaring it, counts of callers and callees outside |
| `current_sources` | files holding the code to replace |
| `callees` | calls into other units: unit, current source, declarations (`file`, `line`, `in_c`), `status` (`converted`, `c-declared`, `needs-c-linkage`, `needs-boundary-decl`, `undeclared`, `library`, `unmapped`), `needs_extern_c`, `needs_boundary_decl`, `edit_headers`, `new_headers` |
| `callers`, `caller_sources` | functions in other units that call the unit, and their files |
| `external_globals` | globals of other units that the unit's code uses (owner, other users, declaring header, `needs_extern_c`, `needs_declaration` when no header declares a shared global); pooled literals are flagged `literal_candidate` |
| `unattributed_own_data` | data outside the attributed ranges that no other unit touches (scaffolded as the unit's own) |
| `test_files` | `test_*` files that mention the unit's functions |
| `own_declaration_headers` | headers that declare the unit's functions today |
| `headers` | **the set of existing headers the conversion is expected to edit**: own declaration headers, callees' headers that need `extern "C"` or a boundary header, and headers of shared globals needing C linkage. New files are not in it. `header_set` is the same as a `frozenset` |
| `sources` | existing non-header files to edit: current sources, caller files and tests |
| `shared_files` | `CMakeLists.txt` and `test_c_boundaries.cpp`: every conversion touches them, so they are not in `headers` |
| `limitations` | the caveats below, so that every report carries them |

`leaf` is true when no callee is in another game or gods98 unit. `with_data`
adds the global and common-data analysis (needs `pefile`, `capstone` and
`reference/LegoRR.exe`, about 3 s once); `None` enables it when they are
present. Everything else needs only the repository.

**Limits.** `edges.csv` holds only the direct calls that Ghidra resolved. It
has no function-pointer callbacks (window procedures, enumeration callbacks,
handler tables), so a unit's callers and callees reached only that way are
missing from the footprint; check them in the pseudocode. Declarations are
found by name with a small scanner and not a C++ parser, so a callee that
shows as `undeclared` needs a look. Two units can also interact without
sharing a header, through the symbol numbering (ADR 0005 entropy policy).

Tests: `make tools-test`.

## `match_function.py` — single-function match loop

Scores one function of the *matching build* against the shipped
`LegoRR.exe` in about a second, without a link or a full Reccmp run. It finds
the `// FUNCTION|LIBRARY: LEGORR 0x...` annotation for the address, compiles
only that translation unit with the exact matching-build command line (from a
cached CMake configure), and runs Reccmp's own entity database, matchers and
`FunctionComparator` over a synthetic image built from the object file. The
score is Reccmp's `matching` value.

```sh
make match-function ADDR=0x00478e60                 # score + aligned diff
python3 tools/match_function.py 0x478e60 --json     # machine-readable
python3 tools/match_function.py 0x478e60 0x4798b0   # several, compiled in parallel
python3 tools/match_function.py --source src/gods98/block.cpp   # every annotated function in a file
python3 tools/match_function.py 0x478e60 --side-by-side         # orig | recomp columns
python3 tools/match_function.py --warm              # pre-start Wine and cache the flags
```

`make match-function` accepts `ADDR=` (one or more), `SOURCE=`, `JSON=1` and
`MATCH_ARGS=` (any other option). Other options: `--no-diff` (scores only),
`--keep` (keep the object and compiler log), `--timings`, `--orig FILE`.

**Output.** Text mode prints the similarity, the bytes compared, the committed
score from the *match baseline* with the delta, and a Reccmp-style unified
diff. Original addresses are real; recomp addresses are offsets from the
function start (`+0x001f`), with `(file.cpp:line)` source references. `--json`
prints one document:

```json
{"timings": {"compile": 0.36, "total": 1.2, ...},
 "functions": [{"address": "0x478e60", "name": "block_open",
   "source": "src/gods98/block.cpp", "line": 218, "origin": "compiled",
   "matching": 0.2162, "percent": "21.62%", "effective": false,
   "baseline": "21.62", "below_baseline": false,
   "orig_size": 89, "recomp_size": 89, "diff": [...], "error": "..."}]}
```

`diff` has the same structure as the `diff` entries in Reccmp's
`results.json`. `origin` is `compiled`, or `LIBC.LIB(<member>)` for a
`LIBRARY` function that the link takes from the VC5 runtime rather than from
the reconstruction. `error` appears instead of a score when the function was
not found in the object (inlined, excluded by `#if`, renamed) or the file did
not compile (the compiler output is included). Exit status: 0 when every
function was scored, 1 when any was not, 2 for setup errors.

**Environment.** Wine runs directly when `wine` is on `PATH`, otherwise
through `distrobox enter developer` (`MATCH_FN_DISTROBOX=<name>`; empty
disables). `REPROBIT_MSVC_5_0_RTM_ROOT` selects the toolchain. The flags cache
lives in `build-match/cmake` (`--build-dir` or `MATCH_FN_BUILD_DIR`). Its
configure options are read from `tools/reccmp.sh`, and it is re-configured
when those options, `CMakeLists.txt` or the toolchain file change. Each
invocation compiles in its own `build-match/runs/run-*` directory, removed on
exit, and configuring takes a file lock, so concurrent runs in one worktree
or across worktrees share no build state. They do share Wine: the tool keeps
one wineserver for the `WINEPREFIX` alive for 15 minutes after its last use
(`MATCH_FN_WINESERVER_PERSIST=<seconds>`, `0` disables) and starts Wine's
services with detached stdio. The MSVC wrapper pipes Wine through `sed`, and
services started during a compile would otherwise hold that pipe open.

**Speed.** About 1.2–1.4 s per call with a warm wineserver, 1.8 s for the
largest file, 3 s fully cold; several addresses in different files compile
in parallel. Measurements are in
[finding 507](../docs/findings/507-single-function-match-loop.md).

**Parity.** On three source trees the score equals full Reccmp's for all but
one function (`__isindst`); finding 507 has the per-area counts and a
26-function spot-check table. On the tree of #436, 2,333 of 2,347 functions
score identically. The other 14 are `__isindst` and 13 LIBC functions whose
LIBC data operands (`__mb_cur_max`, `_pctype` and others) this tool names by
public symbol while the full run names them by their `GLOBAL` annotation; the
same 14 differed before #436. `make match-function-parity`
(`match_function_parity.py`) repeats the comparison against a full
`results.json` from the same tree; run it after changing these tools.
`synthetic_image.py` builds the stand-in for the linked program:

* Relocations are applied in a synthetic layout at `0x0E401000`; undefined
  symbols get slots named the way the linker PDB would name them (CodeView
  names for project code, public symbols for LIBC), and Win32 calls without
  `dllimport` go through `jmp [IAT]` thunks named by their public symbol.
* Like the matching build, which links with `/INCREMENTAL:NO`, the image has
  no incremental-link thunks and no base relocations. Reccmp finds float
  constants, strings and import thunks only through relocations, so neither
  the full run nor this tool names them from the code: both render a float
  operand as `<OFFSETn>`, as on the original side.
* Calls into other translation units are matched to their `FUNCTION`
  annotation by name; other files' line-referenced annotations are named on
  the original side, as the full run would have matched them.
* A constant inside the linked program's address span is treated as an
  address, because the full run finds some entity there (e.g. the flag
  `0x800000` lands in a large `.bss` array). The span ends where an existing
  `build-reccmp/LegoRR.exe` (or `$RECCMP_BUILD_DIR`) ends, else at
  `0xA00000`; `MATCH_FN_LINK_END` overrides.

Known differences: variables have no CodeView type sizes, so offsets render
as `var+4` rather than `var[1]` (text only), and a `GLOBAL` annotation that the
full link matches but this translation unit never references has no size,
which changes one CRT score (`__isindst`, 98.64% here, 96.36% in full).
Functions in objects the link never pulls in (`ptrstack.cpp`,
`video_graph.cpp`) are scored here but absent from the full report. Before
handing work back, `make reccmp-check` remains the gate.

## `rrlwo.py` — `.lwo` (LightWave 4) → glTF 2.0 converter

Reads the game's 1,132 embedded models (`\Models\…` inside `LegoRR0.wad`)
straight from the archives and exports `.glb` — geometry, materials with
colours, and textures pulled from the same WADs. All format semantics cite
[`docs/findings/11-model-and-scene-formats.md`](../docs/findings/11-model-and-scene-formats.md);
the conversion decisions worth knowing:

* **UVs (planar)** — LWOB stores none; the `CTEX` block says the mapping is
  *planar*, so we project each face along its dominant normal axis and divide
  by the `TSIZ` scale, which is the engine's own scheme, not an invented one.
* **`.uv` sidecars** — the 60 models that ship one get the engine's real
  behaviour instead of the planar guess (see
  [`docs/findings/19-uv-grammar.md`](../docs/findings/19-uv-grammar.md)):
  each surface's texture path is overridden by the sidecar's path line, and
  per-corner UVs (stored `v`, negated as the engine stores it) replace the
  projection in POLS ring order. Shared vertices needing different uv
  duplicate automatically through the keyed-vertex table — the same seam
  break the engine performs. Name lines and the trailing keyframe blocks are
  ignored, exactly like the engine's parser; `--no-uv` opts out.
* **Normals** — flat, so shared positions do not share vertices across
  faces (glTF has no face normals).
* **Round-trip** (`rrlwo.py uv`) — writes a `.uv` sidecar for a model. With
  the sidecar present it is kept verbatim where it matters (name lines,
  tail, corner values — a rebuild is byte-exact for all 60 shipped files);
  geometry edits get planar-synthesized uv blocks for changed rings, and the
  keyframe tail is preserved verbatim. `--no-uv` rebuilds purely from the
  model instead, which drops sidecar-only material names and regenerates
  the tail (documented lossy trade); a LWO TIMG edit is only reported when
  a stale sidecar path shadows it, so adopting it stays explicit.

Lookups resolve by **full path first** (`list` prints full paths, and `-n`
takes them back); the engine's own path layer works the same way. Sidecars
only ever resolve exactly — `.uv` belongs to the model beside it.

```
python3 tools/rrlwo.py list                          # full paths of models in the wads
python3 tools/rrlwo.py dump "Creatures\LavaMonster\Arml.lwo"   # incl. uv summary
python3 tools/rrlwo.py gltf "Buildings\Barracks\Base.lwo" -o base.glb
python3 tools/rrlwo.py gltf PATH.lwo -o out.glb --wad-dir mod/L1   # modded trees
python3 tools/rrlwo.py gltf -n NAME.lwo -o out.glb --no-uv        # ignore sidecars
python3 tools/rrlwo.py uv -n PATH.lwo -o out.uv                    # write/rebuild sidecar
```

Conversion notes found the hard way (documented in finding 11 §3 terms):
polygon indices in `POLS` are 1-based per spec, but four exporters used
0-based — the tool auto-detects per file (a zero index is impossible
1-based). Negative indices mean the polygon is wound backwards and get the
winding flipped. `SURF` blocks use tag + u16-BE size records anchored by the
SRFS surface names; BMP payloads convert to PNG with stdlib only.

Verified: 1,129/1,132 exported, with 3 `*/Steam.lwo` skipped as point-only
particle sprites (single-vertex `POLS` polygons, not fan geometry — a data
fact of those files, flagged explicitly since 2026-10-20) and all 60 `.uv`
sidecar models exported with their real per-corner UVs and overridden
texture paths applied. Structural checks (GLB framing, buffer lengths, index
ranges, PNG inflation) on every model, and a visual check on
`Buildings\Barracks\Base.glb` in a glTF viewer — flat-shaded meshes with
textures, as the design predicts (user, 2026-10-20).

> Correction note: the original 1,132/1,132 batch (2026-10-20, finding 11
> tooling) looked entries up **by basename**, which silently fed duplicate-
> basename models (`arml.lwo` exists under five creature folders, `Steam.lwo`
> under three) the *first* same-named file instead of their own. Exact-path
> resolution landed with the `.uv` pass; per-entry payloads are verified
> correct from then on.

## `rrwad.py` — the `.wad` (WWAD) container tool

Full unpack / pack / verify for `LegoRR0.wad` and `LegoRR1.wad`, per the
format documented in [`docs/findings/01-binary-and-loader-map.md`](../docs/findings/01-binary-and-loader-map.md)
(§ "WAD container format").

```
"WWAD" | u32 count | count*NUL names | count*NUL dev-paths | count*16B TOC | data
TOC entry: flags u32 | orig_len u32 | stored_len u32 | file_offset u32
```

Commands:

| Command | Purpose |
| --- | --- |
| `list WAD` | index listing (`# index size @offset name`) |
| `unpack WAD DEST` | extract a proper directory tree + hidden `.rrwad.json` manifest (names, dev-paths, flags, sizes, offsets) |
| `pack MANIFEST -o OUT` | rebuild the archive from the manifest + edited files |
| `verify WAD` | in-memory round-trip: every archive here packs back **byte-identical** |
| `extract WAD NAME...` | stream one entry to stdout (case-insensitive name, like the game) |

The manifest is what makes mods lossless: the dev-path strings (`\\ROBW\C\
Dev\SourceSafe\Lego\Data\…`) are kept verbatim even though the game ignores
them, so repacked archives are indistinguishable from the originals — except
for the data you changed, and payload offsets recomputed for new sizes.

Round-trip proof (run 2026-10-20, both shipped archives):

```
verify: OK — reference/LegoRR0.wad round-trips byte-identical (5631 entries, 78,968,800 B)
verify: OK — reference/LegoRR1.wad round-trips byte-identical (214 entries, 3,754,454 B)
```

Mod workflow:

```sh
python3 tools/rrwad.py unpack reference/LegoRR1.wad mod/L1
# edit mod/L1/Lego.cfg (or any entry)
python3 tools/rrwad.py pack mod/L1/.rrwad.json -o mod/LegoRR1.wad
```

Notes:

* Nothing is compressed in the shipped files (`stored == original`, flags all
  `1`); the engine's dormant compression codecs stay untested (finding 01).
* Cross-wad shadowing (`LegoRR1` beats `LegoRR0`, scan order `9→0` per
  finding 01) is decided by mount order at runtime, not by entry order —
  repacking either archive never changes which file the game opens.
* Name lookups are case-insensitive `__strcmpi` with no collisions observed
  in either archive, so extracted trees are safe on case-insensitive
  filesystems.
* `unwad.py` is the earlier extract-only script from the finding-01 work;
  `rrwad.py` supersedes it (it also converts `foo\bar.bmp` names into real
  directories instead of literal backslash files).

## Unit-relative data offsets

Run `make unit-data-offsets` after a matching build, or `python3
tools/unit_data_offsets.py --build build-reccmp --json` for machine-readable
results. The standalone stdlib tool reads the link-order list, range CSV, map,
source annotations, the Reccmp entity report, the current CMake link response and matching COFF objects.
The response file excludes stale object directories left by a rebase. It parses section alignment,
section pieces and symbols, and compares each attributable annotation's gap
from the first annotation in its original section. Finding 524's supported
anchors additionally check absolute offsets. `full` requires matching gaps and
anchor; `gaps-only` has no anchor; `partial` reports the first discrepancy;
`n/a` means too few items or incomplete attribution. Unresolved addresses and
common symbols are reported separately. Unit status is partial if any section
mismatches, otherwise n/a when an active section is unknown, then gaps-only or
full. Empty sections do not establish a match.

Ownership ranges are evidence ranges, not exact boundaries; ambiguous and
outside annotations are listed separately. Only unambiguous direct source
declarators in a dedicated link-order object are associated with COFF names;
Reccmp mappings and map symbols also resolve literals when they identify a
unique symbol in the unit object. Subobjects, declarations in other files,
folded symbols and section changes can remain unknown. A common is identified by section zero
and nonzero COFF value and is excluded from offset scoring (finding 525).
Multiple COFF sections with the same base name are laid out in object order,
aligned by their characteristics; COMDATs are individual pieces. This is an
object-layout model, not proof of final linker placement or content identity.
An anchor mismatch can coexist with equal gaps; it signals missing or extra
prefix storage. The tool does not change `binary-progress.json`.

## Whole-binary progress

`make match-progress` regenerates `docs/coverage/match-baseline.txt`,
`docs/coverage/match-listing-digests.txt`, and `docs/coverage/binary-progress.json`
from one fresh matching build. Run `make reccmp-check` first and review any
drops under ADR 0005 before accepting the baseline. The build and Reccmp always
run: a matching input hash alone does not prove that the baseline is current.
The PR freshness check uses `pr_match_progress.py check`, and its `render`
command compares the committed reports without the toolchain. The baseline
and listing headers record the input fingerprint, and the progress file records
digests of both. On conflict, take either side and regenerate all three with
`make match-progress`; never hand-merge them.

For each dropped score, `make reccmp-compare` compares the rebuilt listing
with the accepted listing fingerprints and checks the annotated source against
`origin/main`. `--base-ref` selects another Git base when calling
`tools/match_baseline.py check` directly; `--base-report DIR` compares two
complete reports. A source change or unknown source needs review. Unchanged
source is classified as drift, reordered drift, or drift-review when the
instruction multiset changes. Every nonzero instruction-count delta is flagged
for the owner. Normalisation masks registers, stack offsets, branch targets and
absolute addresses; these fingerprints do not prove behavioral equivalence.
The gate still fails on every score drop.

`make binary-progress` runs the matching build (`make reccmp-report`, in
`build-reccmp/` or `$RECCMP_BUILD_DIR`), then `binary_progress.py`, which
prints the comparison as JSON and writes `docs/coverage/binary-progress.json`.
`make binary-progress-compare` runs only the comparison, on an existing build.
It needs the matching-build requirements plus Python packages `pefile` and
`capstone` (used by `translation_units.py`). Set `BINARY_PROGRESS_PYTHON` to a
Python with those packages. The matching build links with `/INCREMENTAL:NO`
and writes `LegoRR.map`; the linker emits a separate PDB for Reccmp, and
`tools/reccmp.sh` (`strip_pe_debug.py`) removes the PE debug directory and
file-only debug overlay before Reccmp and this comparison read the executable.
`make reccmp-check` is the match baseline gate on the same build.

Schema 3 layout measures attribute mapped functions to original unit ranges.
Unit order uses each unit's median rebuilt function address; contiguity counts
one rebuilt run in original function order, omitting unmapped functions.
Unit-relative identity counts original function bytes only when the entire
function matches after masking image-address operands and external relative
branches. Missing functions count against the denominator. Size-exact units
require every original function to map with equal code size; the prefix uses
original unit spans including padding. Reccmp's PDB sizes and the disassembled
original code extents are approximations. Data agreement is anchored on the
lowest mapped annotation in each original section and uses the evidence ranges
in the unit map, which are not exact contribution boundaries; unmapped and
ambiguous annotations are counted. A unit with no comparable pair is n/a.
The map's first public symbol per object is supplementary and can miss objects
without publics; the link-order list comparison uses unit starts and legacy
median keys and excludes the port tail. None of these is a claim of exact
object or data boundaries.

Section comparisons align each named section at its start and use the
original's virtual size as denominator (missing candidate bytes count as
unequal); raw-file alignment padding is excluded. Imports and resources use
the PE data-directory spans: imports are the descriptor table, not all names
and thunks. The original-address `.text` comparison aligns by virtual address.

For every region the report also counts identical bytes in contiguous runs of
at least 16 bytes. Sixteen suppresses isolated common x86 bytes while still
capturing short sequences; it is a threshold, not proof of semantic identity.
Chance is the median of five comparisons with the original circularly shifted
by 1/11 through 5/11 of its length (integer byte offsets). Raw and run excess
are percentage-point differences from their respective chance rates; negative
values are possible. Long zero runs can still inflate data-section scores, so
the chance estimate is also reported for those regions. The report counts
functions with a raw Reccmp score of exactly 100% that also start at the same
address and whose recovered original instruction span is byte-identical there.

For both full and game-only scopes, adjacent inversions count consecutive
mapped functions in original order whose rebuilt addresses go backwards;
pair inversions count all reversed pairs, with ties excluded. The issue's 874
is an adjacent-inversion baseline from an earlier incremental build, so use
`function_order_inversions.full.adjacent` for that measure (the matching
build has linked without incremental mode since #436, which can change it). Exact-function coverage sums reachable
instruction spans recovered by `translation_units.py` for raw Reccmp 100%
matches, divided by original `.text` virtual size. It is not a byte-for-byte
comparison of those functions. Accuracy uses raw `matching` scores, rounded
per function to two decimal percent before averaging, matching
`match_baseline.py`; effective matches are not promoted to 100%. Header
changes are numeric values (entry point is an RVA).

`python3 tools/binary_progress_hash.py` prints a commit-independent SHA-256
using only the standard library. It selects Git-tracked matching-build sources
from CMake's source declarations and the link-order list
(`src/link_order.cmake`, read by `link_order.py`), excludes native-only and
test-only sources, and conservatively includes non-native project headers. It also hashes the
build configuration, metric code and its coverage inputs. Untracked and ignored
files are excluded. When Git is unavailable, the explicit
`tools/binary_progress_inputs.txt` list is used; update it when the selected
tracked inputs change (the tooling test checks it). The report's schema version
is defined in the hashed metric script. The input hash excludes generated
outputs, reference binary, build artifacts and commit metadata. Run tests with
`make tools-test` (the PE fixture test needs `pefile`).

## `msvc_permuter.py` — bounded source search

Run inside the matching-build environment, with `reference/` available:

```sh
python3 tools/msvc_permuter.py 0x0045d470 --seconds 60 --workers 2 --output build-match/search-45d470
```

The time budget includes the initial score; two workers are the default. The
search tries individual conservative rewrites, scores each through
`match_function.py`, retains improvements, and repeats from the best candidate.
It stops on an exact score, exhaustion, or the deadline. The output directory
contains `result.json`, the complete `best.c` or `best.cpp`, and `best.patch`.
Errors in individual compilations are recorded in the result. A failed initial
score exits nonzero. The search does not edit `src/`.

The passes swap uninitialized primitive declarations, declarators, and
independent adjacent scalar assignments and increments (including
comma-separated `for` steps); reverse simple C comparisons; invert restricted
braced C `if/else` branches; reorder adjacent, explicitly terminating switch
cases;
introduce and remove certain scalar temporaries; commute and reassociate
unsigned scalar expressions; and convert restricted `while`, `for`, and
`do` loops. These are intentionally narrow patterns, not a C/C++ parser:
for example, arithmetic rewrites require known unsigned locals and avoid
nested scopes. Switch reordering rejects fallthrough and nested blocks, and
increment swaps require distinct known primitive scalars or pointers whose
addresses do not escape. Compiler-generated increments have no source statement to swap.
Many functions have no candidates. Review
all changes for semantics (including macro expansion and volatile access) and
MSVC 5.0 compatibility. Scores use the single-function loop and can differ
from the full linked build. Candidate files must preserve line numbers so the
COFF line records map to the original source. `--candidate FILE` on
`match_function.py` is the underlying replacement-source interface.

To list the register/constant/order cohort of an existing game-only report:

```sh
python3 tools/permuter_cohort.py build-reccmp/reccmp-report/game-only.json > build-match/cohort.json
```

The classifier compares multisets of unmatched instructions after replacing
registers and numeric constants with placeholders. It preserves opcodes and
other operand text; order can differ. It is a mechanical filter, not proof
that the functions are semantically equivalent or that this search can reach
them. The [demonstration](../docs/coverage/msvc-permuter-demonstration.md)
records the exact cohort and results.

To apply a reviewed result in a later change, check the patch against the
current tree (`git apply --check build-match/search-45d470/best.patch`), apply
it with `git apply`, then run `make reccmp-check`. A local improvement can
cause other functions to drop through translation-unit symbol numbering
(finding 506); review every drop under ADR 0005. For a source change that is
accepted, run `make match-progress` and include both refreshed coverage files.

## Native CMake/Meson source parity

`python3 tools/meson_source_parity.py` checks the native game, gods98,
platform glue, executable, miniwin, and loop-created test source lists without configuring either
build or requiring a compiler. CMake groups the platform-filtered entries in
`src/link_order.cmake` by target; the check compares those entries and the
executable's explicitly compiled hook sources with Meson. It also compares the
miniwin lists. Keep Meson's literal source arrays and their target wiring in
sync with this check. `make tools-test` runs the check and its fixtures.

## `reshape_check.py` — check a reshape for zero drift

```sh
python3 tools/reshape_check.py src/game/select.cpp
python3 tools/reshape_check.py src/game/select.cpp src/game/entity.cpp --base main --head HEAD --json
```

The defaults compare `HEAD~1` with the working tree; `--base` and `--head`
accept Git revisions, and `worktree` selects the working tree. Each file must
be a translation unit in the configured builds. The tool checks that include
directives are unchanged, strips them from both versions, then preprocesses
with the native, matching and MinGW reference configurations. This follows
plan #490: it compares the source's own tokens without expanding header macros,
and avoids needing the headers for the token checks. It does not prove token
equivalence with headers included; the matching object check still compiles
with the real includes. Changes to headers are outside this check's scope.

Native and matching flags come from CMake's compile commands. When available,
MinGW uses its cross-compiler and winapi CMake configuration. Otherwise the
reference token check uses host `g++ -E` with the reference platform defines;
its result is labelled **emulated**. It does not reproduce every predefined
MinGW macro, so an emulated pass is evidence for those specified defines, not
for arbitrary compiler-specific predefines. The object check uses the
single-TU matching command from `match_function.py` and compares non-debug
COFF sections (including `.drectve`), relocations by symbol name, and the
non-debug symbol table. The tool reports each check and its first difference;
`--json` emits the same results as JSON. Differences and tool failures exit
nonzero; an emulated pass counts as a pass.

Run in the developer distrobox or let the tool enter it when Wine is absent.
The matching toolchain and native build's configure dependencies must be
installed. It configures `build/` and `build-match/` if needed, and uses
`build-match/reshape-winapi/` for the MinGW CMake configuration when the cross
compiler is installed. Temporary source and object files stay in the worktree
and are removed on exit.
