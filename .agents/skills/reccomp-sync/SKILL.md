```markdown
---
name: reccmp-sync
description: Synchronize a specified function's compiled machine code with the reference binary using reccmp.
---

# Reccmp Function Synchronization

Make the user-specified function's compiled machine code byte-identical to the reference binary.

## Workflow

1. Locate the target function and inspect its implementation, existing reccmp annotations, and entry in `docs/coverage/ledger.csv`.
2. Run `make reccmp-report` to rebuild and generate `./build-reccmp/reccmp-report/results.json`.
3. Inspect the report to identify instruction-level differences.
4. Make minimal, targeted source changes to improve the match.
5. Repeat the build-and-compare cycle until the function matches exactly or progress is blocked.
6. Verify the final result and check for regressions in previously matching functions.
7. Update `docs/coverage/ledger.csv` to reflect the verified matching status.

## Rules

- Preserve existing reccmp annotations and add or correct annotations when justified.
- Never remove annotations merely to improve reported matching results.
- Prefer source-level fixes over binary patching or changes to comparison thresholds.
- Avoid unrelated refactoring or modifications outside the target function.
- Do not confuse behavioral equivalence or partial similarity with an exact binary match.
- Report the final match status, changes made, and any remaining differences.

