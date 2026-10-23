# Finding 286 — Task Registry and Category-Mask Helpers

Three low-address routines now have names grounded in their linked-list operations and callers. Task code meanings and category-mask semantics remain unresolved.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x402890 | TaskType23CreateForCategoryMask | Checks category eligibility, allocates a task record of type `0x17`, stores the object and mask, maps mask values to action codes, optionally refreshes the record, and queues it. |
| 0x403430 | TaskRegistryRemoveType0EntryByPair | Finds and unlinks the first type-0 list node whose two key fields match the supplied pair, then frees it. No caller was resolved. |
| 0x403560 | TaskRegistryRemoveType10EntriesForEntity | Removes and frees every type-10 list node whose owner field matches the supplied entity. Called during entity state cleanup. |

All bodies are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). `0x402890` is called by task/action paths `0x405c10` and `0x41dca0`; `0x403560` is called from entity cleanup `0x43bc40`. Direct caller evidence for `0x402890` is retained in [legorr-engine-helper-audit-low-expansion-6.bundle](../evidence/legorr-engine-helper-audit-low-expansion-6.bundle), record `ev_da0c97e785b0140104395800a5516ac43924be856fb7f69258d2a2fbcc36f118`. No retained caller record was found for `0x403430`. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
