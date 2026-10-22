# 45 — Timer expiry returns before the other objective predicates

`FUN_00459050` evaluates an armed timer before crystal, ore, and the delegated
objective checks. When the timer reaches zero, that branch returns immediately;
the `0x20` mode bit chooses which outcome marker the caller announces. Thus the
timer branch can decide the evaluation result without reaching the other checks.

**Observations from** REA/Ghidra 12.1.4 over `source/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence: `docs/evidence/legorr-campaign-expansion.bundle`.
Focused IDs: `ev_1d5be4a8ee35d532f5533b2e28b4a59d9a30a156f8558af4ef63322faf41f169`
(`FUN_00459050`) and `ev_509a5c13e8c5b080ff314d3a2c76ad8486a6a3afa70c97f8d136f649ca8bae23`
(`FUN_00458be0`).

## Evaluation order

The evaluator first refuses to run when objective-suppression bit `0x10` is
set, then initializes its secondary result output to `1`. With timer bit
`0x800` set, it subtracts the supplied delta from the timer at `level+0x104`.
If the new value is nonpositive, it returns `true` immediately. When mode bit
`0x20` is also set, it changes the secondary result to `0` before returning;
otherwise the initialized `1` remains.

The caller `FUN_00458be0` maps that output to `FUN_00458670(2)` when it is zero
and `FUN_00458670(1)` when nonzero. Only when the timer has not expired does
the evaluator proceed to the crystal and ore quota comparisons and the
delegated predicate at `LAB_00459100`.

Finding 22 records the meaning of the relevant flag bits. This finding adds
the short-circuit ordering and its consequence: at expiry, the current crystal,
ore, and delegated predicate results are not consulted by this evaluator.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| Timer subtraction precedes the crystal, ore, and delegated checks | observed in the complete `FUN_00459050` body |
| Expired timer returns immediately | observed |
| Bit `0x20` selects the zero/nonzero result passed to the outcome announcer | observed across `FUN_00459050` and its caller |
| Which level configurations use this mode in normal play | not enumerated in this pass |
| User-visible timing and frequency of objective evaluation | unknown; static call flow only |
