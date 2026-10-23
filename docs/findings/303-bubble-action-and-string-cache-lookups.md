# Finding 303 — Bubble-Action and String-Cache Lookups

Two low-address routines now have names grounded in direct lookup behavior and their immediate callers.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x407300 | BubbleActionNameToIndex | Compares an input case-insensitively against a null-terminated bubble action-name table and returns the matching index. Bubble config loading uses that index to store the resolved resource. |
| 0x410450 | StringKeyedCacheEntryFind | Walks a singly linked cache and compares each entry key case-insensitively, returning the matching node or null. Its caller reuses a match or allocates and links a new entry. |

Bodies and callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). No matching retained direct-analysis bundle record was found for either routine. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
