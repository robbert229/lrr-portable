
# Autonomous LEGO Rock Raiders Porting Loop

## Objective

Continuously port unimplemented functions from the LEGO Rock Raiders decompilation corpus into the native C++ reconstruction.

Work autonomously, completing one function or tightly coupled cluster per iteration. Each successful iteration must produce a tested, documented commit pushed to `integration/port`.

Repeat until no actionable functions remain.

## 1. Project Context

Read and follow:
- `AGENTS.md` — repository conventions and safety rules.
- `README.md` — current architecture and implemented modules.
- `docs/coverage/README.md` and `ledger.csv` — porting progress.
- `docs/decompiles/full/LEGORR.full.c` — canonical Ghidra pseudocode.
- `docs/findings/` — behavioral analysis and dependencies.
- `docs/adr/` — architectural decisions.

Use existing implementations and tests as patterns. Do not redesign established systems.

Repository constraints:
- Plain C-shaped C++, `extern "C"`, `calloc`/`free`.
- No STL or platform-specific OS headers in portable modules.
- Reuse `GameContext`, `LevelState`, `EntitySlot`, and existing accessors.
- Use `level_store_ptr`/`level_load_ptr` and pointer registries for native pointers; never truncate pointers into 32-bit corpus words.
- Preserve literal unknown masks and corpus-specific behavior.
- Document intentional safety deviations at their implementation sites.

## 2. Select the Next Function

1. Read the coverage ledger to identify unported functions.
2. Check dependencies and relevant findings.
3. Prefer functions that:
   - Have satisfied dependencies.
   - Have documented behavior and clear observable effects.
   - Unlock additional functions.
   - Fit naturally into existing modules.
4. Choose one function or a small, inseparable cluster.
5. Skip previously blocked functions unless their dependencies or circumstances have changed.

Do not select work solely because it is easy. Prioritize meaningful dependency progress.

Use GitHub `[port]` issues for additional specifications when available, but treat the coverage ledger as the authoritative work queue.

## 3. Investigate

Before writing code:

1. Locate the exact function and address in `LEGORR.full.c`.
2. Read its entire decompiled body.
3. Read its findings, callers, callees, and relevant data structures.
4. Identify all inputs, outputs, state mutations, allocations, and failure paths.
5. Examine corresponding existing modules and tests.
6. Identify dependencies that must already be implemented.

If behavior is ambiguous, inspect the relevant corpus context or use reverse-engineering tools.

Never invent behavior to fill gaps. If correctness cannot be established, record the blocker and choose another function.

## 4. Implement and Test

Implement the function within the existing subsystem.

- Extend existing modules instead of creating unnecessary new ones.
- Add a new `.hpp`/`.cpp` pair only when appropriate.
- Match corpus semantics, including edge cases and side effects.
- Preserve existing allocation and ownership conventions.
- Avoid unrelated refactoring or speculative improvements.
- Clearly document any intentional divergence from the original.

Write or extend unit tests that verify:
- Normal behavior and observable state changes.
- Boundary conditions and failure paths.
- No-op and not-found behavior.
- Corpus-specific quirks where relevant.

Follow existing `test_*.cpp` patterns, synthetic fixtures, and Meson test registration.

Prefer test-driven implementation when practical.

## 5. Validate

Before committing:

1. Run `meson compile -C build`.
2. Run all relevant new and existing test suites.
3. Run `python3 tools/coverage_ledger.py`.
4. Audit the resulting coverage diff.
5. Update `README.md` under "Reconstruction build".
6. Review the entire diff for correctness and unintended changes.

Coverage rules:
- Only actually implemented functions may become `ported`.
- Address references in `src/` can trigger false positives.
- Never cite unimplemented caller/callee addresses in source comments.
- Explicitly verify that only intended coverage entries changed.
- Do not manipulate the ledger to inflate progress.
- Preserve consistency between source, headers, README, and ledger.

Known baseline failures: `flic` and `frontend` tests fail on clean `main`. Report these separately; do not modify unrelated code to fix them.

Never commit a failing new test or unverified implementation.

## 6. Adversarial Review

Before committing, review the change as a skeptical maintainer.

Check:
- Does behavior actually match the corpus?
- Are any branches or side effects omitted?
- Are ownership, pointer width, and initialization correct?
- Are tests meaningful rather than merely confirming implementation assumptions?
- Did unrelated coverage entries change?
- Is the change narrowly scoped and maintainable?

Use the `code-review` skill when available.

Address significant findings and rerun affected tests.

## 7. Commit and Push

Work exclusively on `integration/port`.

At the start of each iteration, ensure the working tree is clean and synchronize with the remote branch using SSH.

Git remote:
`git@github.com:robbert229/lrr-portable.git`

After successful validation:

1. Stage only the relevant implementation, tests, documentation, and ledger changes.
2. Create one descriptive commit identifying the function and corpus address.
3. Push the commit to `integration/port`.
4. Verify that the push succeeded.

Never force-push, overwrite unrelated work, create PRs, or merge other branches.

Never commit `reference/`, `Saves/`, or generated build artifacts. Never modify `Saves/`.

If synchronization or pushing fails, resolve safely if possible; otherwise stop rather than risk losing work.

## 8. Handle Blockers

If a function cannot be ported safely:

1. Record its address, name, blocker, and required next action in `docs/coverage/blocked.md`.
2. Do not mark it as ported.
3. Skip it for the remainder of the session unless the blocker is resolved.
4. Continue with another eligible function.

Do not repeatedly investigate the same unresolved blocker.

If an implementation fails validation and cannot be corrected, leave no partial commit and restore only changes belonging to that attempt. Preserve any pre-existing user changes.

## 9. Repeat

After each successful push:
- Confirm the working tree is clean.
- Re-read the updated coverage ledger.
- Select the next eligible function.
- Repeat the entire process immediately.

Do not stop after one function.
Do not request human approval between iterations.
Do not create handoff documents after every iteration.

Stop only when:
- All remaining functions are blocked or already ported.
- A repository-wide problem prevents safe progress.
- Git synchronization cannot be completed safely.

When stopping, report completed ports, commits, coverage progress, and outstanding blockers.

The objective is continuous, correct, independently verifiable progress — not merely increasing the coverage counter.
