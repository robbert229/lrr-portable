# Finding 319 — Entity-Stats and Mask Predicates

Four helpers expose exact masks used by command and route/action eligibility code. Their numeric names preserve the decompile’s known values without assigning undocumented meanings to those bits.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x439cf0 | EntityStatsWordFlag10Test | Returns whether the stats word contains mask `0x10`. |
| 0x439d10 | EntityStatsWordFlag20Test | Returns whether the stats word contains mask `0x20`. |
| 0x439d30 | EntityStatsWordFlag40Test | Returns whether the stats word contains mask `0x40`. |
| 0x43baf0 | EntityFlagsMask107C33DEOr1C00203AnyTest | Returns true when either entity word at `+0x3e0` / `+0x3e4` intersects its corresponding literal mask. |

Bodies and callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). No matching direct retained analysis records were found; caller relationships are visible in the checked-in pseudocode. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
