# Finding 327 — Entity Threshold and Flag-Mask Predicates

Two entity predicates preserve the exact threshold field and bit mapping rather than guessing the domain meaning of either.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x437cc0 | EntityFlag40000ThresholdCompare | Tests entity mask `0x40000`, then returns true if the helper-derived value is below the float at entity offset `+0x340`. |
| 0x438ed0 | EntityFlagsContainRequestedSingleBit | Maps requested values 1, 2, 4, 8, `0x10`, and `0x20` to entity masks 4, 8, `0x10`, `0x20`, `0x40`, and `0x80`. |

Bodies and callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). No matching direct retained evidence records were found. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
