# Finding 315 — Pooled-Record and Display-Mode Helpers

Three routines manage record-pool allocation and display-mode dialog refresh.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x4382c0 | PooledRecordAllocateAndReset | Pops a free-list record, clears its fields, and initializes its link and sentinel values. |
| 0x438310 | PooledRecordChunkAllocateAndLink | Allocates a chunk of fixed-size records and chains its entries into the free list. |
| 0x49d5e0 | DisplayModeDialogSetModeAndRefreshChoices | Stores a mode value, refreshes display-mode choices, and sets associated dialog text. |

Bodies and callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). Caller analysis for record allocation is retained in [legorr-engine-helper-audit-mid.bundle](../evidence/legorr-engine-helper-audit-mid.bundle). Display-mode caller/refresh evidence is retained in [legorr-engine-helper-audit-text-io-expansion-9.bundle](../evidence/legorr-engine-helper-audit-text-io-expansion-9.bundle), records `ev_063bdde652592c453554a48c8d747df0428e2f540e7d892383383b6b5fd92ff1` and `ev_eaeb0c1f924a8658bb02fb7e050bc8806b07144c8582395fd978b40a79b99dbe`. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
