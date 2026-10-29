# Goal: Incrementally Reconstruct and Annotate LEGO Rock Raiders

Your goal is to autonomously improve the completeness, correctness, and binary fidelity of the LEGO Rock Raiders recompilation.

The repository contains stubs, partially reconstructed functions, approximate implementations, and potentially matching functions.

You have access to REA for reverse engineering the original executable, a coverage ledger, and reccmp for comparing the rebuilt code against the reference binary.

Work incrementally, prioritizing changes that deliver measurable progress toward a complete and faithful recompilation.

## Existing Infrastructure

**Coverage ledger:** `docs/coverage/ledger.csv`

This is the authoritative record of reconstruction progress. Inspect its schema and conventions before updating it. Do not create a competing ledger.

**Comparison command:** `make reccmp-report`

This rebuilds the recompilation and compares it against the reference executable.

**Comparison report:** `./build-reccmp/reccmp-report/index.html`

Use the report and any available supporting machine-readable data to evaluate progress, identify mismatches, and measure improvements.

**Annotation reference:**
https://github.com/isledecomp/reccmp/blob/master/docs/annotations.md

Follow this documentation when annotating source code. Inspect existing repository annotations to determine the correct binary target aliases, source organization, and conventions.

## Operating Loop

Repeatedly execute the following phases.

### Phase 1: Assess

1. Read the coverage ledger.
2. Generate or inspect the current reccmp report.
3. Identify missing, stubbed, partially implemented, mismatched, and unannotated functions.
4. Identify reconstructed functions that may not yet be correctly associated with their original binary addresses.
5. Examine relevant dependencies and call relationships using REA.
6. Identify opportunities to improve fidelity, coverage, annotation completeness, or functional execution.

Establish a reproducible baseline before modifying code.

Do not assume existing implementations or annotations are correct.

### Phase 2: Prioritize

Choose one bounded, high-value reconstruction task.

Prefer, in roughly this order:

1. Nearly matching functions that may be brought to exact agreement with small corrections.
2. Existing implementations missing reliable reccmp annotations.
3. Stubbed or incomplete functions whose original behavior can be recovered.
4. Shared data structures, globals, signatures, and calling conventions causing multiple mismatches.
5. Foundational functions that unblock additional reconstruction work.
6. Important executable paths that cannot yet run correctly.

Consider impact, feasibility, dependencies, and verification cost.

Do not simply implement functions in address order or select arbitrary stubs.

Do not spend excessive time on a difficult function when more productive work is available elsewhere.

### Phase 3: Investigate with REA

For the selected function or subsystem:

1. Locate the original implementation in the reference binary.
2. Recover its original address and determine which binary target it belongs to.
3. Examine assembly, decompiled code, callers, callees, and cross-references.
4. Recover signatures, calling conventions, data types, structure layouts, and global state.
5. Identify constants, branching behavior, loops, side effects, and error handling.
6. Compare the reference implementation to the existing source.
7. Examine current reccmp results to identify specific discrepancies.
8. Develop a concrete hypothesis about the required implementation or correction.

Distinguish confirmed findings from hypotheses.

The reference binary is authoritative. REA decompilation is an interpretation of the binary and should be cross-checked against assembly when necessary.

### Phase 4: Implement

Implement the smallest coherent change that advances the selected task.

- Preserve existing architecture and conventions.
- Prefer faithful reconstruction over plausible approximations.
- Preserve calling conventions, memory layouts, and important compiler-sensitive behavior.
- Repair existing partial implementations rather than duplicating them.
- Avoid unrelated refactoring.
- Do not invent behavior without supporting evidence.
- Do not change compiler settings, reference binaries, or comparison criteria merely to improve scores.

When uncertain, document the uncertainty instead of silently treating guesses as facts.

### Phase 5: Annotate the Source

Source annotations are a required part of reconstruction work.

Follow the official reccmp annotation specification:

https://github.com/isledecomp/reccmp/blob/master/docs/annotations.md

Use addresses recovered from the original executable, not addresses guessed from source order or generated recompilation output.

Use the correct target alias determined from the project's configuration.

#### Function annotations

Use `FUNCTION` for reasonably complete, non-templated, non-synthetic implementations that should participate in assembly comparison.

Example:

`// FUNCTION: <TARGET> 0x00401230`

Place the annotation directly before the function implementation whenever possible.

Use `STUB` for functions with no implementation or very incomplete implementations.

Example:

`// STUB: <TARGET> 0x00405670`

`STUB` functions are excluded from assembly comparison.

Do not classify a function as `STUB` merely because its comparison score is poor. If it has a reasonably complete implementation, retain `FUNCTION` so its mismatch remains visible.

When a stub becomes reasonably complete, replace its `STUB` annotation with `FUNCTION`, then use reccmp to assess fidelity.

Do not declare a function complete merely because its annotation was changed.

#### Other function annotation types

Use the appropriate specialized annotation when applicable:

- `TEMPLATE` for templated functions.
- `SYNTHETIC` for compiler-generated functions, such as scalar deleting destructors.
- `LIBRARY` for functions belonging to third-party libraries.

`SYNTHETIC` takes precedence over `TEMPLATE`.

When direct implementation annotation is inappropriate, use the documented comment-based function-name or symbol annotation syntax.

For overloaded or otherwise ambiguous functions, use the appropriate decorated symbol where necessary.

Use the `SYMBOL` option when explicitly identifying an undecorated symbol name.

Use `FOLDED` only when evidence establishes that multiple functions share an address because of compiler folding. Do not use it to resolve accidental duplicate annotations.

#### Global variables

When REA establishes an original global variable address, add a `GLOBAL` annotation to the corresponding source declaration or definition, following the project's conventions.

Example:

`// GLOBAL: <TARGET> 0x004A1000`

Do not confuse the global pointer's address with the address of the object it references.

#### Virtual tables

When a class's original virtual table address is established, add a `VTABLE` annotation to the appropriate class declaration.

Example:

`// VTABLE: <TARGET> 0x004B2000`

Do not guess virtual table addresses from class names or inheritance alone.

#### Strings

Use `STRING` annotations for string constants when useful for correspondence or bookkeeping.

Example:

`// STRING: <TARGET> 0x004C3000`

Most strings can be detected automatically, so do not annotate every string unnecessarily.

Remember that a pointer-to-string global and the actual string data may occupy different original addresses and require distinct `GLOBAL` and `STRING` annotations.

#### Line annotations

Use `LINE` annotations selectively when a mismatch is difficult to interpret or reccmp incorrectly aligns portions of the generated assembly.

Example:

`// LINE: <TARGET> 0x00401245`

The annotation must immediately precede the corresponding source-code line, without an intervening blank line or comment.

Do not blanket-annotate every statement.

#### Annotation ordering and integrity

- Functions within a compilation unit must be ordered by ascending original address, as required by reccmp.
- Preserve correct existing annotations.
- Avoid duplicate, conflicting, or speculative address mappings.
- Ensure every new annotation refers to the correct original target.
- Prefer updating existing annotations over adding redundant ones.
- Validate annotations through the comparison tooling.

Do not rearrange functions casually if doing so would alter existing compiler behavior or invalidate matching code. Investigate such conflicts explicitly.

#### Annotation discovery

When examining code, look for missing annotations on already implemented functions, globals, and virtual tables.

Add annotations when the original identity and address can be established confidently.

Annotation-only work is valuable when it enables previously untracked reconstructed code to participate in comparison.

However, do not generate arbitrary annotations merely to increase coverage counts.

### Phase 6: Validate and Measure

After implementation and annotation changes, run:

`make reccmp-report`

Inspect:

`./build-reccmp/reccmp-report/index.html`

Compare the result against the baseline.

Determine:

1. Did the targeted function's accuracy improve?
2. Did any function become an exact match?
3. Did previously matching functions regress?
4. Were new functions correctly discovered through annotations?
5. Did any function unexpectedly disappear from comparison?
6. Did the number of stubs change?
7. Did annotation changes alter measured coverage?
8. Did compilation or comparison fail?

Distinguish actual binary fidelity improvements from changes caused solely by adding, removing, or reclassifying annotations.

In particular, converting an inaccurate `FUNCTION` into a `STUB` must never be represented as a fidelity improvement.

For a function promoted from `STUB` to `FUNCTION`, report the newly measured comparison result without assuming that the promotion indicates an exact match.

Treat exact binary agreement as strong evidence of reconstruction fidelity, while recognizing that functional behavior and wider system correctness may require additional validation.

If an attempted improvement regresses existing matches, investigate and repair the regression before proceeding.

If a change does not improve the result, reassess the hypothesis rather than making arbitrary adjustments.

### Phase 7: Update Reconstruction Records

Update `docs/coverage/ledger.csv` after validating the work.

Preserve its existing schema and conventions.

Ensure its status information remains consistent with the implementation, annotation state, and actual comparison evidence.

Where the existing documentation supports it, preserve:

- Original function addresses.
- Reconstructed source identities.
- Verified signatures and calling conventions.
- Recovered structures and global relationships.
- Comparison results and outstanding mismatches.
- Remaining unknowns and important hypotheses.
- Dependencies or functions unlocked by the work.

Do not create a separate coverage ledger or duplicate tracking system.

Preserve valuable reverse-engineering discoveries in the existing source comments or documentation when appropriate.

Avoid cluttering source files with lengthy speculative explanations.

### Phase 8: Repeat

After a validated improvement:

1. Reassess the updated ledger.
2. Review the latest comparison results.
3. Identify the next highest-value task.
4. Repeat the reconstruction cycle.

Do not continue blindly through a predetermined list when new evidence suggests a better priority.

If a task becomes blocked or disproportionately expensive, preserve the findings and move to a more productive target.

## Operating Principles

**Evidence over intuition.**
Use REA, the original assembly, and reccmp results to guide decisions.

**Accurate annotations are essential.**
Code that cannot be mapped to the original binary cannot be meaningfully evaluated by reccmp.

**Implementation and annotation progress are distinct.**
Track both, without confusing new comparison coverage with improved matching accuracy.

**Protect established matches.**
Avoid regressions and unnecessary source changes.

**Incremental improvement.**
Prefer bounded changes with measurable results.

**No score manipulation.**
Never hide mismatches, fabricate annotations, or weaken comparison settings to make results appear better.

**No speculative rewrites.**
Do not rewrite entire subsystems when a narrower improvement is sufficient.

**Preserve knowledge.**
Keep annotations and the existing ledger accurate so progress survives between sessions.

**Make forward progress.**
Turn reverse-engineering discoveries into concrete, verifiable source improvements.

## Definition of Success

Each work cycle should deliver one or more of:

1. A newly reconstructed function with a verified original address.
2. An existing function brought closer to an exact binary match.
3. A function achieving an exact binary match.
4. A correctly promoted `STUB` to `FUNCTION` with a measured comparison result.
5. A previously unannotated implementation correctly associated with the reference binary.
6. A verified global, virtual table, or specialized function annotation.
7. A repaired structural or calling-convention mismatch.
8. A resolved dependency that unlocks further reconstruction.

**The ultimate objective is a complete, behaviorally faithful recompilation with accurate source-to-binary annotations and increasing verified binary agreement.**

Continue autonomously through bounded work cycles until the available execution budget is exhausted or a genuine blocker prevents useful progress.

At the end of the session, summarize:

- Functions investigated and implemented.
- Annotations added, corrected, or promoted.
- Comparison results before and after.
- Actual fidelity and coverage improvements.
- Regressions encountered and resolved.
- Ledger updates.
- Outstanding uncertainties.
- The recommended next reconstruction target.
