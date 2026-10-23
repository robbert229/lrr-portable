# Finding 333 — Advisor-Name and Vector-Math Helpers

Four helpers map advisor labels to indices or perform common vector operations. One 2D distance routine duplicates an already named helper at another address, so it carries a variant suffix to keep the Ghidra symbol unique.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x4013e0 | AdvisorPositionNameToIndex | Case-insensitively scans the configured advisor-position name table and returns the matching ordinal. |
| 0x4014c0 | AdvisorAnimationNameToIndex | Case-insensitively scans the advisor-animation name table and returns the matching ordinal. |
| 0x406730 | Vector2DistanceVariant | Returns the Euclidean distance between two 2D coordinate pairs; behavior matches the named `Vector2Distance` helper at `0x401240`. |
| 0x406760 | Vector2SetLength | Scales a 2D vector to the requested magnitude. |

Bodies and callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). The advisor caller parses `AdvisorPositions_*` and `Advisor_Anim_*` keys; caller context for the vector helpers is also visible in the corpus. No direct retained function records were found for these routines. Finding 248 documents the advisor table caller. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
