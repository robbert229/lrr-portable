# Finding 324 — Global-State and Record Accessors

Five helpers expose simple global stores, selected-pointer reads, and an input-driven shared-record update. Their names preserve the observed global or offset where the object’s domain meaning is not established.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x425cd0 | Global004de548Set | Stores the argument in global `DAT_004de548`. |
| 0x425d50 | GlobalRenderRecordSetFromInput | Computes a value from its argument and passes it with the input to a helper operating on global `DAT_005560e8`. |
| 0x4295a0 | ContextSelectedGlobalPointerGet | Returns `DAT_005560f8` or `DAT_00556104` according to whether `DAT_00556ec8` is zero. |
| 0x42b500 | GlobalStateField84Get | Returns the dword at offset `+0x84` from `DAT_005560d4`. |
| 0x435550 | GlobalStateZeroTransitionDetect | Returns true when the current value is zero and its stored prior value was nonzero, then saves the current value. |

Bodies and available callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). The globals’ domain meanings remain unresolved. No matching direct retained procedure records were found. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
