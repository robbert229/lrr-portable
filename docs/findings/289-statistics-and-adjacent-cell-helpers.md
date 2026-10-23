# Finding 289 — Statistics and Adjacent-Cell Helpers

Two anonymous helpers now have names grounded in their direct behavior and caller use.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x42ef40 | StatsLevelRecordCountForStatType | Returns a count or default size for a statistics type. Its caller uses the result to allocate and clear a per-type table while processing `Stats/Levels` data. |
| 0x4093b0 | PassableAdjacentCellSearchExpand | Checks four adjacent coordinate offsets, tests terrain/cell conditions, and recursively expands from candidates that remain invalid. It is reached from entity movement checks. |

Bodies and callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c): `0x4667e0` uses `0x42ef40` for per-type table allocation, and `0x43c570` invokes `0x4093b0` after movement checks. Direct evidence for `0x42ef40` is retained in [legorr-engine-helper-audit-low.bundle](../evidence/legorr-engine-helper-audit-low.bundle), record `ev_96a977baa11dd29928be4a2bf16f89bdc83e04195aefa4cfda355ef37e33fd0e`. No matching direct-function bundle record was found for `0x4093b0`. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
