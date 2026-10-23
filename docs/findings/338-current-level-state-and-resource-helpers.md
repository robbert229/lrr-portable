# Finding 338 — Current-Level State and Resource Helpers

Three helpers test a raw current-level flag mask, add to a field at a fixed offset, or adjust one of two resource-balance fields and dispatch a sound cue.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x41f720 | CurrentLevelStateFlagsContainMask | Tests whether the current state record’s `+0x3f0` word intersects the supplied mask. |
| 0x41f920 | LevelStateField9CAdd | Adds the supplied delta to `DAT_005560d4 + 0x9c`; the field’s domain meaning is not established. |
| 0x41f9a0 | LevelResourceBalanceAddAndSound | Adds to `DAT_005560d4 + 0xa0` or `+0xac` according to the selector, then dispatches the corresponding sound cue. |

Bodies and callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). Direct analysis of `0x41f720` is retained in `legorr-engine-helper-audit-low-expansion-5.bundle`, record `ev_e95685659bdf199dcba80b7def4e5869c93436edb22e1e0f1519f7fd73deaa4e`. Callers of the resource adjustment are in the reward/resource payout paths; no direct retained record was found for `0x41f9a0` or `0x41f920`. The masks and field meanings remain literal or offset-level. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
