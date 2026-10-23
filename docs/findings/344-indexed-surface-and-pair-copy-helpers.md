# Finding 344 — Indexed-Surface and Pair-Copy Helpers

Two routines write a value at an indexed 2D position and copy a pair of dwords to output pointers. Their format and record meanings are not established.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x450700 | GridSurfaceWriteAtXY | Computes `y × width + x` and forwards the index and value to a surface-write helper. |
| 0x439930 | CopyTwoDwordPairToOutputs | Copies two adjacent dwords from a source record to two output pointers. |

Bodies and callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). Circle-fill and recorded-point callers establish indexed 2D use for the surface writer; separate callers use the pair copy for two-field records. No matching direct retained procedure records were found. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
