# Finding 296 — Entity Effect Counters Applied from a Record

`0x42b2f0` is now named `EntityEffectCountersApplyFromRecord`. It checks entity flag `0x08`, then applies the two record values through separate entity counter helpers. The exact counter meanings remain unknown, so the name describes the visible record-to-entity operation.

The body is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c) and the function appears in [legorr-engine-helper-audit-low.bundle](../evidence/legorr-engine-helper-audit-low.bundle). No static caller was resolved in the checked-in corpus. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
