# Finding 326 — Render-Config and Grid-Record Tables

Six helpers copy a six-value render configuration and maintain/read a pair of indexed grid tables. The names describe storage operations and do not assign a domain meaning to the table values.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x4370b0 | RenderConfigSixFieldCopyOut | Copies six dwords from a global configuration record to the caller’s buffer. |
| 0x4370d0 | RenderConfigSixFieldCopyInOrClear | Copies six dwords into the global configuration record, or clears the six values when passed a null pointer. |
| 0x437100 | GridRecordCountDifferenceSum | Sums indexed table differences across the 15-by-16 grid, optionally skipping rows selected by mask `8`. |
| 0x437440 | GridRecordCountTablesClear | Clears two arrays of `0x12c0` dwords each, the first at `DAT_004e3944` and the second at `DAT_004dee44`. |
| 0x437460 | GridRecordCountDifferenceGet | Maps `(param_1, param_2, param_3)` with `((param_2 + param_1 * 15) * 16 + param_3)`, returns the second-table value, and optionally subtracts the first-table value. |
| 0x4374a0 | GridRecordCountGet | Reads one value from the first table at the same computed index. |

Bodies and callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). No matching direct retained evidence records were found; caller context identifies grid indexing and configuration use, but the stored quantities’ exact domain meaning is unresolved. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
