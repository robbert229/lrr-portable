# Finding 318 — Entity Eligibility and Dock Helpers

Three anonymous helpers now have names that describe their observed predicates. Numeric masks and thresholds stay explicit where the decompile does not establish their domain meaning.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x438f60 | DockDefinitionMatchesCell | Reads the configured `Docks` coordinate and checks whether both supplied coordinates match it. |
| 0x4399c0 | EntityFlag400AndHealthThresholdCheck | Requires entity mask `0x400`; for kind 1 it also checks a count comparison and whether the value at `+0x3d4` is below 75. |
| 0x4436f0 | EntityType3Flag04000000Test | Returns true only for kind 3 with mask `0x04000000` set at `+0x3e8`. |

Bodies and available caller context are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). No matching direct retained analysis records were found for these three routines; the names are based on the checked-in pseudocode. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
