# Finding 343 — Advisor Action and Animation Record Helpers

Three helpers store and retrieve advisor action coordinates and perform advisor-animation cleanup. Unknown record flags remain literal.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x401840 | AdvisorActionPositionAndModeSet | Stores a mode and two coordinates in an advisor-action record; sets or clears raw flag `0x10000` when the mode equals `0x0c`. |
| 0x401890 | AdvisorActionPositionGet | Copies two dwords at record offsets `+0x1c` and `+0x20` to output pointers. |
| 0x4011c0 | AdvisorAnimationEndCleanup | Sets a scene flag on the animation and base objects, then clears bit 0 of `DAT_004b31bc`. |

Bodies and callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). Caller context pairs the coordinate setter/getter while adjusting advisor action positions. Direct analysis for cleanup is retained in `legorr-advisor-animation.bundle`, record `ev_7be305cc76fef15854d1121b11361ffa068ac5b98a814d18fcf4943dcc3430a1`; caller evidence is `ev_8142acdd211db2c7d85717d37cbc2605c78fc8f45daaafb57cde4d4baa297397`. No matching direct retained records were found for the record accessors. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
