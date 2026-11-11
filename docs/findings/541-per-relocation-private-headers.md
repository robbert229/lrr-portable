# Finding 541 — Per-relocation private headers: later relocations stop drifting earlier units

Issue #496, the follow-up proposed in finding 537. Part of #388. It
changes how `tools/relocate_unit.py` shares declarations between a relocated
unit and its legacy source, and migrates the existing private headers.
The migration is measured on `main` at `8bdb5e28` (after #494's wave R5,
#498 and #505), and the replay check on `a0096524` (after #494's wave R5).

## Summary

1. **The problem.** Each legacy file *L* had one private `<l>_shared.hpp`.
   Every relocation out of *L* appended to it, and every unit relocated out of
   *L* included it. So each later relocation changed what the earlier units
   saw. MSVC 5.0 re-rolls tied codegen when the declaration count before a
   function changes (finding 538, rule B), so unedited unit files drifted:
   `0x0040e9c0` in #493 (finding 537) and `0x004089d0` repeatedly. Units also
   saw `extern`s of their own definitions, which reverses operand order
   (finding 538, rule A; the cause of `0x0040e9c0`).
2. **The design.** Each relocation of unit *U* from *L* now writes its own
   pair of private headers:
   - `<l>_for_<u>.hpp`, which *U* and *L* include;
   - `<l>_from_<u>.hpp`, which only *L* includes.

   Both are written once and never edited. A later relocation from *L*
   writes its own pair, so an earlier unit's view never changes. A unit never
   sees declarations of its own definitions.
3. **The migration.** Each of the 142 (unit, old header) pairs gets a verbatim
   snapshot, `<l>_for_<u>.hpp`, and the unit's includes name it with their
   spelling otherwise unchanged. **The migration changes no listing:** 0 of
   2,386 digests change, and the gate passes with no score change.
   Two units then get back the view they had before wave R5 (#494)
   appended to their legacy files' headers. That restores `0x0040e9c0` to
   100% (#498's fix, which R5 undid) and `0x00484e50` to its pre-R5 score
   (below).
4. **A new measurement.** MSVC 5.0's codegen depends on the **spelled path**
   of an `#include`, not only on the declarations it brings in. Respelling
   three includes of the same file (`"record_shared.hpp"` →
   `"gods98/record_…"`) re-rolled `0x004089d0` with an identical
   preprocessed token stream. Guard names, file-name length and file
   identity made no difference (below).
5. **Replay check.** Relocating GAME-011 from five shared legacy files on top
   of the migration left every other unit file, and every header they
   include, byte-identical. The 715 functions in the other unit files kept
   identical instructions. One listing digest changed, `0x004078c0`, only
   because Reccmp labels a pushed string address differently after the data
   moved (below).

## Design

Two shapes were possible.

| | Per-relocation private headers (chosen) | Minimal, stable per-unit declaration sets |
| --- | --- | --- |
| What a unit sees | the headers its relocation wrote, frozen | exactly the declarations it uses, in a canonical order |
| Later relocations | write new headers; never touch earlier ones | must not change any earlier set |
| Types and macros moved out of the legacy file | live once, in the relocation's `for` header; later units include that header | would need their own frozen home anyway |
| Legacy file | includes every pair | the same |
| Migration of existing units | a verbatim snapshot: **no** declaration change | recompute each unit's set: **every** unit's declaration sequence changes |

The second shape needs headers too, because a type that both the unit and
the legacy file use must be defined once where both see it. Its "minimal"
sets also change every existing unit's declaration sequence when they are
introduced. Rule B makes that a coin toss per tied function in every
relocated unit, which is the very drift this ticket is meant to stop. A
first trial of a minimal split confirmed this: it re-rolled `0x004089d0` and
failed to compile `bitmap_font.cpp`, which relied on the old header for a
function it defines later. The chosen shape freezes what exists and makes
every new view minimal from the start.

### The pair a relocation writes

| Header | Holds | Included by |
| --- | --- | --- |
| `<l>_for_<u>.hpp` | types and macros moved out of *L*, and declarations of *L*'s items that *U* uses | *U* and *L* |
| `<l>_from_<u>.hpp` | declarations of *U*'s items, moved out of *L*, that *L* still uses | *L* only. *U* includes it only when it defines a const object, which needs its `extern` first for external linkage |

Further rules:
- **Self-contained.** Each header includes the earlier private headers of
  *L* whose names it uses.
- **Copying includes.** A new unit file copies *L*'s include list, but keeps
  an earlier private header only when it uses a name the header declares.
- **Frozen.** If a header already exists, the relocation refuses; it is
  never rewritten.
- **Forward declarations.** A function the unit uses before defining it,
  declared only in the `from` header, now gets a forward declaration in the
  unit file.
- **Placement.** Unchanged from finding 530 (first-use placement in *L*),
  and checked per header.

### The migration (`--migrate-private-headers`)

For each `<l>_shared.hpp` and each unit file that includes it:
- `<l>_for_<u>.hpp` is the old header's body, verbatim; only the include
  guard and the comment differ.
- Every `#include` of the old header in that unit names the snapshot instead.
  Only the file name changes; any directory prefix is kept exactly. Repeated
  includes stay repeated. 41 of the 142 pairs had two includes of the same
  header, a leftover of the pre-#493 tool, sometimes spelled two ways.
- The old `<l>_shared.hpp` stays as it is, included only by *L*. The tool
  never writes it again.
- A header included by anything other than its legacy file and unit files
  is refused. None was.
- The migration checks that no annotated item changes, and that each
  snapshot's declarations equal the old header's, in order.

Records: `docs/coverage/relocations/private-header-migration.csv`, one row
per snapshot, with the body's SHA-256 and the number of includes replaced.

`tools/convert_unit.py` uses the same test (`relocate_unit.is_private_header`)
to leave all three kinds of private header out of the boundary test's
includes, as it already did for `<l>_shared.hpp`.

## Why the migration keeps include spellings: an experiment

A first migration pointed each unit's include at its snapshot with the
tool's canonical spelling. Same-directory headers were bare, others got a
path from `src/`. The gate then showed one change: `0x004089d0`
`segment_2d_point_distance` (GAME-007, `placement.cpp`) moved from 40.89% to
43.56%, with an x87 register choice changed. MSVC's `/E` output for the old
and new `placement.cpp` was identical token for token. Single-function
compiles (`match_function.py`) isolated the cause:

| `placement.cpp` variant (headers with identical content) | `0x004089d0` |
| --- | ---: |
| original (`*_shared.hpp`, as spelled) | 40.89 |
| snapshots, canonical spelling | 43.56 |
| snapshots, the old include guard names restored | 43.56 |
| snapshots, every include respelled without a directory | 43.56 |
| old spelling, each name +1, +2, +4 … +8 characters | 40.89 |
| old spelling, names `*_for_placement.hpp` | 40.89 |
| old spelling, except three `"gods98/…"` prefixes (the canonical form) | 43.56 |

The original file includes the gods98 headers twice, first by search path
(`"record_shared.hpp"`), then as `"gods98/record_shared.hpp"`.
Respelling only how the same three files were named flips the function. Its
code depends on the spelled include path, not on the file's content, name,
name length or guard. This extends finding 538. The compiler probably keeps
a table of spelled include names, with a cost or a hash that interacts with
rule B. The mechanism is not identified.

The rule for any tool that rewrites include lines in a unit that is not
meant to change: change the file name only, and keep the spelling.

## Results

### Migration (`main` `8bdb5e28`)

- 32 old headers, 40 unit files, 142 snapshots; 183 include lines renamed.
  This covers the headers of every relocation so far, including #494's
  wave R5. Runs on `5031c134` (before R5, 70 snapshots) and `a0096524`
  (before #505) gave the same result; the snapshots and renamed includes
  were byte-identical between `a0096524` and `8bdb5e28`.
- Native build and ctest: 35/35. `link_order.py check` and `check-map`,
  and Meson parity: OK.
- `make reccmp-check` on the verbatim migration: OK, no function falls or
  rises. The listing digests
  equal `main`'s for all 2,386 functions (game-only mean 51.09%, full
  55.85%, as on `main`). `make match-progress` leaves the three generated
  files' measurements unchanged; only their input hashes change. The
  input manifest `tools/binary_progress_inputs.txt` gains the 142 snapshots.
- **The verbatim migration recovers no earlier drift, by design.** It
  keeps every unit's declaration sequence, so it keeps every unit's codegen.

### Restoring pre-R5 views (earlier drift recovered)

#498 restored `0x0040e9c0` to 100% on `5031c134`. It added a rule A guard to
`level_shared.hpp` that hides `extern float s_erosion_delay_scale;` from
`lava_erosion.cpp`. Wave R5 (#494, `a0096524`) then appended to 22 shared
headers, including three that `lava_erosion.cpp` included. That shifted the
unit's declaration count, and `0x0040e9c0` fell to 84.21% (finding 536: the
integer `imul` operands swap, a rule B re-roll; the guarded `fmul` order
stays right). This is the drift that per-unit headers are designed to stop.

Per-unit snapshots make it cheap to undo, because editing one unit's
snapshot changes no other translation unit. A survey used single-unit
compiles (`match_function.py --source`). For each of the 18 units relocated
before R5, it removed R5's appended blocks from all of that unit's
snapshots, returning the unit to its exact pre-R5 view. Every R5 change was
a pure append. Only three functions changed, each back to its exact pre-R5
score:

| Function | Unit | `main` (after R5) | pre-R5 view | Applied |
| --- | --- | ---: | ---: | --- |
| `0x0040e9c0` lava_erosion_delay_from_cell | `lava_erosion.cpp` | 84.21 | **100.00** | yes |
| `0x00484e50` flic_decode16_bit_delta | `flic_unit.cpp` | 19.44 | 19.50 | yes |
| `0x004089d0` segment_2d_point_distance | `placement.cpp` | 43.56 | 40.89 | no: R5's re-roll happened to gain; restoring would fall below the baseline |

The pre-R5 view is applied to `lava_erosion.cpp` (`crt_`, `cmd_` and
`level_for_lava_erosion.hpp`) and `flic_unit.cpp` (`record_for_flic_unit.hpp`).
The removed text is R5's appended blocks, so each snapshot equals its old
header's body as it stood before R5. The #498 guard is kept, verbatim.
Records: `docs/coverage/relocations/private-header-view-restores.csv`. The
full gate shows exactly these two digests changed (`0x0040e9c0` 84.21 →
100.00, `0x00484e50` 19.44 → 19.50), with nothing falling. The means become
51.10% game-only and 55.86% full.

Counter effects were measured directly, not modelled. The verbatim
migration leaves every unit's preprocessed token stream, and so finding
538's counter at every function, unchanged, and the full gate confirms it.
For the restore, single-unit compiles score every function of the unit
in about a second, so the kit in `tools/experiments/486/` (sweeps and
delta debugging, for finding causes) was not needed.

Per-unit headers make such fixes safe from now on: editing one unit's
snapshot changes no other unit. Finding 538's counter rule suggests the
  next step. Dropping a unit's `extern`s of its own variables from its
  snapshot does not move the counter at any function after those variables'
  definitions, which is every function in a relocated unit file. That would
  sweep rule A cases without re-rolling rule B, a candidate follow-up.

### Replay check (`a0096524`)

On top of the migration, GAME-011 (tile variant textures) was relocated with
the new tool. It moves 11 functions and 2 closure items from `level.cpp`,
`terrain_grid.cpp`, `util.cpp`, `gods98/record.cpp` and `gods98/view.cpp`.
Before the migration, `level_shared.hpp` was included by 12 unit files,
`record_shared.hpp` by 14, `view_shared.hpp` by 13, `util_shared.hpp` by 5
and `terrain_grid_shared.hpp` by 4. It was then rolled back; it is not part of this PR.

| Check | Result |
| --- | --- |
| Files changed | the five legacy files, the new unit file, two new `for` headers (`util_for_tile_variants.hpp`, `view_for_tile_variants.hpp`), the list, Meson, `translation-units.csv` |
| Existing unit files or private headers changed | **none** (byte-identical) |
| `make reccmp-report`: listings of the 715 functions in other unit files | instructions identical for all 715, with operand labels and addresses masked |
| Listing digests changed in other unit files | 1: `0x004078c0` (`bubbles.cpp`). The same `push` of a string literal's address is labelled `<OFFSET4>` before and `"Interface_MenuItem_UpgardeCarry" (STRING)` after. GAME-011's move shifted the data, so Reccmp names the address differently. The code is identical. (The earlier run on `5031c134` saw the same function's `"LoadSel"` push relabelled the other way.) |
| Listing digests changed in legacy files | 1 (expected: they lost code) |

So a relocation no longer changes any other unit file's code. Reccmp's label
for an address can still change when the data layout moves, as it can for
any change to the link.

## Limits

- **Legacy files still drift** with every relocation out of them: they lose
  code and gain includes. That is accepted (ADR 0006 rule 2).
- **Operand labels follow data layout.** A label like the `(STRING)` one can
  still change in any unit when data moves. The drift classifier sees that
  as `drift-review` with +0 instructions.
- **No sweep of rule A yet.** Existing units still see whatever their
  snapshot holds, including `extern`s of their own variables. The sweep
  sketched above is a separate change, with its own gate.
- **The spelled-path effect** is shown for one function and one set of
  respellings. Which spellings are safe is not characterized; the tools keep
  spellings as they are.
