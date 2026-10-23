# Finding 291 — Tracked Entity Pointer Array Removal

`0x43a930` is now named `TrackedEntityPointerArrayRemove`. It scans the populated entries of a global pointer array and zeroes every slot equal to the supplied pointer. Entity teardown `0x437540` calls it before releasing other entity-owned resources.

The direct body and teardown caller are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). No matching direct REA procedure-analysis record was found for this helper. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
