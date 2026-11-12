# Rebuild code as original translation units in a C-compatible subset, compiled as C in the matching build

**Status:** accepted (issue #384). **Amends** [ADR 0004](0004-miniwin-native-port.md),
decision Q-E ("C++17 for the reconstruction"). **Amended by**
[ADR 0006](0006-original-link-order.md): units are relocated verbatim before
conversion, the matching build links them as explicit objects in original
order (recipe step 2), port-only code in a unit is compiled out of the
matching build (step 4), and restructuring PRs follow its entropy rules.

## Decision

**Adopt.** Game and engine code is restructured into **original translation
units** (finding 505), one source file per unit, each rebuilt in the
original's shape. Each unit is:

- **C-compatible**: the C89 subset that MSVC 5.0's C compiler accepts.
- **C in the *matching build*** (`cl /TC`). The native port and the MinGW
  *reference build* keep compiling it as C++ (CMake `LANGUAGE CXX`), because
  miniwin's headers are C++-only. That part of ADR 0004 still stands.

The C++17 baseline of ADR 0004 now applies to platform adapters and to code
not yet converted. Code that is genuinely C++ in the original (the
DirectShow and AVI players, finding 505 rule 8) stays C++ with `/GX`.

## Evidence

[Finding 506](../findings/506-original-shape-pilot-priorities.md) rebuilt
`Priorities.c` (GAME-034) and measured each part of the shape separately:

- Gathering the existing code into one unit file changed nothing.
- Rewriting it in original shape raised the unit from **15.6% to 67.8%**
  byte-weighted similarity (0 to 4 exact functions), and its one outside
  caller from 52.9% to 100%. Recompiled length fell from +34.5% to +2.2%
  instructions. The gain comes from the original signatures, direct global
  state and original call targets, not from typed fields: an offset-accessor
  control scored the same.
- Compiling as C and dropping `/GX` changed no instruction in this unit.
  They are adopted anyway because they cost nothing once the source is
  C-shaped and because:
  - only C places tentative definitions in the common region the original
    uses (from `00549520`);
  - only a C compile reproduces the original's symbol numbering, which
    drives MSVC's register choices (the entropy effect below);
  - without `/GX` a unit cannot depend on C++ unwinding by accident.
- Converting callee contracts to the original (Config item layout, float
  panel positions) adds about 4 more points. That arrives as the callees'
  own units are converted.

**Rejected alternatives.** Keeping C++-shaped files and tuning functions one
by one: before the pilot, 848 of 1,670 non-matching game functions compile
more than 5% longer, which per-function tuning does not fix. Compiling C++
"in C style" in the matching build: it gives the same codegen today, but not
the common symbols or the original's symbol numbering.

## Entropy policy

MSVC 5.0's register and x87-stack allocation depends on how many symbols a
translation unit has declared (finding 506). Every edit to a shared header
therefore perturbs unrelated functions in every unit that includes it, up
and down. The restructuring cannot avoid this. While it lasts:

1. Prefer zero-drift techniques where they are free:
   - give an existing declaration C linkage with `extern "C"` in place;
   - keep C boundary headers out of shared C++ headers.
2. A restructuring PR may refresh the *match baseline* for drops outside the
   units it converts when all of these hold:
   - the dropped functions' source is unchanged;
   - their listings differ only in registers, stack slots or branch
     layout;
   - the PR reports the net effect.
   Any other drop is a regression.
3. The symbol-count pads that the pilot left in `crt.hpp` and `frontend.hpp`
   are a pilot-only device. The first PR that refreshes under rule 2 removes
   them. Do not add new ones.
4. Every change made only to help the score carries the comment phrase
   `Deviation (scoring only)`, so that `git grep "Deviation (scoring only)"`
   lists them all. This covers declaration-only deviations (kept superseded
   declarations, like those in `view.hpp` and `record.hpp`) and anything
   else that is not the original's. A scoring-only deviation must not change
   storage: no variable's size, alignment, order or section may differ from
   the original's (ADR 0006 decision 5). Named-section storage (`#pragma
   data_seg`, `__declspec(allocate(...))`) is rejected by `make
   reccmp-lint` (finding 580). Where storage would help a score, use an
   annotation instead, or accept the lower score. The rule covers the
   comparison tooling too: `tools/reccmp_plain_immediates.py` carries the
   phrase. It keeps a rebuilt operand plain where the original's same
   function has that value as a constant past the original image (finding
   584). It changes Reccmp's inputs at run time, not its package or its
   scoring.

As units become C with original-shaped headers, the symbol numbering
converges on the original's, and the drift becomes convergence.

## Conversion recipe (#385–#389)

For each original translation unit, in link order where possible:

0. **Scaffold.** Run `python3 tools/convert_unit.py <unit>` (`make
   convert-unit UNIT=…`; see `tools/README.md`). It writes the mechanical part
   of steps 2 to 4 and the wiring, and reports the *conversion footprint*
   (its files, callees, callers and headers), the unit's before scores and
   the checklist for the PR description. The stubs repeat the existing
   functions' annotations, so **`make reccmp-check` fails on a scaffolded tree
   until the old definitions are deleted** (the old functions drop out of the
   report). Delete them in the same conversion PR; only the final PR has to
   pass the gate.
1. **Pick the unit** from `docs/coverage/translation-unit-ranges.csv`. Its
   functions, with their current files, are in `translation-units.csv`.
   Prefer high-confidence boundaries.
2. **Create the file.** Name it `src/game/<stem>.c` or `src/gods98/<stem>.c`,
   lowercase, with a short C-compatible `<stem>.h`. Add its `unit` entry to
   the link-order list, `src/link_order.cmake`, in its original position and
   with its library (`game` or `gods98`). The matching build links every list
   entry as an explicit object in list order; it links without incremental
   mode, so explicit objects are not called through thunks (ADR 0006
   decisions 1 and 2). The list's `.c` files are compiled as C in the
   matching build and as C++ elsewhere. Remove the unit's functions from the
   legacy files they leave; the legacy entries stay where they are.
3. **State.** Turn the unit's `.bss` range into one typed globals struct
   laid out as the original. Use `static` for per-object `.bss` and plain
   file-scope (common) definitions for the common region. Put initialised
   `.data` at file scope in address order. Annotate each with
   `// GLOBAL: LEGORR 0x…`.
4. **Functions.** Write them in original address order from the
   disassembly and the [full pseudocode](../decompiles/full/LEGORR.full.c):
   - use original signatures and direct reads and writes of the globals;
   - add no port hook tables, state parameters or defensive guards; record
     a deviation only when the port needs one;
   - place unreferenced inline helpers at the end (finding 505 rule 6);
   - put port-only test accessors last, after the original functions.
5. **C subset.** Follow MSVC 5.0 C rules:
   - declarations at block start;
   - no `bool`, references, overloads, default arguments, `inline` or C++
     casts;
   - `//` comments are allowed (the Reccmp annotations use them).

   Check with `gcc -std=gnu89 -Werror=declaration-after-statement
   -fsyntax-only`.
6. **Callees.** Every function or global the unit uses must have C linkage:
   - Add `extern "C"` to its existing declaration in the C++ header, in
     place.
   - Declare it in that module's C boundary header (`<module>.h`), which
     shared C++ headers do not include. Add the pair to
     `src/game/test_c_boundaries.cpp`.
   - Where a callee has no original-ABI entry, add a thin unmarked adapter
     with the original signature in its module.
   - Where a callee's contract differs from the original, use the existing
     contract and note it. The callee's own conversion recovers the rest.
   - Use VC5 LIBC names (`_itoa`); `src/crt/vc5names.h` maps them for
     other compilers.
7. **Callers and tests.** Switch callers to the original signatures. Delete
   the superseded wrappers and their declarations (see the entropy policy).
   Move their tests to the new API.
8. **Measure and gate.**
   - Run `make unit-data-offsets` on the matching build; review the unit’s
     section gaps, anchored offsets where available, unknowns and commons.
   - Run `make reccmp-check` and compare the unit's functions before and
     after.
   - Classify any drop outside the unit under the entropy policy.
   - Mark every scoring-only deviation `Deviation (scoring only)` (entropy
     rule 4), and check that none of them changes storage.
   - Run `make match-progress` and commit the refreshed match baseline,
     listing digests and whole-binary progress. It reads the same
     non-incremental build as `make reccmp-check` (ADR 0006 decision 2), so
     it only reruns Reccmp. The PR freshness check verifies the input hash;
     it also checks the baseline digest and does not replace the local gate or
     this entropy review. On conflict, take either side and regenerate all three;
     never hand-merge the generated files.
   - Regenerate the unit map with `python3 tools/translation_units.py build`.
9. **Native.** The unit and every touched file must compile natively with
   no new errors, and `c_boundaries` must pass. Run the native tests once
   `main` builds natively again.

## Consequences

- Each conversion touches its callees' headers, so conversions are cheapest
  bottom-up (engine utilities and Config before their game callers). Each
  converted callee raises its callers' scores by fixing their contracts.
- C boundary headers duplicate declarations until the owning module is
  itself converted. At that point its `.h` becomes the only header and its
  C++ `.hpp` shrinks to port-only additions. `test_c_boundaries.cpp` guards
  the duplication meanwhile.
- The matching build now enables C. Native, MinGW and Meson builds are
  unaffected apart from compiling the `.c` units as C++.
