# Finding 297 — Activity-Record and Message-Table Helpers

Seven routines now have names supported by direct list, record, flag, and string operations.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x4314c0 | ActivityBuildRecordRelease | Unlinks a record from global and neighboring links, clears its owning grid-cell slot, and frees it. |
| 0x431530 | ActivityBuildRecordsReleaseAll | Walks the record list and releases each entry through `0x4314c0`. Teardown calls it. |
| 0x46a120 | GroupedCodeTableFindValueIndex | Scans grouped code entries and returns the matching group index through an output pointer. |
| 0x46a370 | LinkedRecordChainFree | Recursively follows the `+0x1c` link and frees each record. |
| 0x46a390 | GlobalLinkedRecordChainClear | Resets the global record state and frees the head chain through `0x46a370`. |
| 0x46a760 | FirstSetFlagOrdinal | Returns the one-based ordinal of the lowest set bit among bits 0–12, or zero if none are set. |
| 0x46aad0 | MessageLabelIndexFind | Case-insensitively matches a label against a global message-label table and returns its index. |

Bodies and caller paths are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). Direct bit-ordinal evidence is retained in [legorr-engine-helper-audit-mid-expansion-5.bundle](../evidence/legorr-engine-helper-audit-mid-expansion-5.bundle), record `ev_406f854beae3509e70f55a6de0059e1dc6f799193827e08c59a49cf99c18b865`; teardown records are also in the full corpus. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
