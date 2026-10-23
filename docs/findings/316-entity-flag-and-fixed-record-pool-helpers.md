# Finding 316 — Entity-Flag and Fixed-Record-Pool Helpers

Two routines now have operation-level names based on their exact storage behavior and callers.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x438580 | EntityFlag80000000SetOrClear | Sets or clears entity bit `0x80000000` at offset `+1000`, then refreshes related state. Dispatch cases `0x22` and `0x23` map to the set/clear operation. |
| 0x47a5c0 | FixedSizeRecordPoolGrow | Allocates a chunk of `0xbf4`-byte records, links each record through offset `+0xbf0`, and adds the chunk to the free list when allocation is needed. |

Bodies and callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). The pool grow caller evidence for `0x47a580` calling `0x47a5c0` is retained in [legorr-engine-helper-audit-low-expansion-5.bundle](../evidence/legorr-engine-helper-audit-low-expansion-5.bundle), record `ev_ab57e81844c1be440f3fa768e026b5980557ba2aaaad39df0e3a0b4275ef7faa`. No direct-function record was found for `0x438580`. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
