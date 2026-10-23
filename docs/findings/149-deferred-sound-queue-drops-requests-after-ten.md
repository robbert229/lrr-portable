# Finding 149 — Deferred Sound Queue Drops Requests After Ten

`FUN_00464f60(alias, selector)` appends a deferred sound request only while `DAT_00502bd0 < 10`. At ten pending entries, it returns without storing the new alias or selector and without reporting a failure. `FUN_004651a0` drains the prior pending list, moves the current list into the next-frame list, and resets the current count to zero. Thus this queue can carry at most ten requests into a frame's pending list; further requests accumulated before the tick are silently discarded.

This cap applies to callers that use `FUN_00464f60`, including the menu-overlay and music-stream submissions documented in findings 141 and 144. It does not establish the capacity or behavior of the separate packed request queue used when batching is enabled in `FUN_00465090`.

| Queue stage | Behavior |
| --- | --- |
| Append via `FUN_00464f60` | Store alias and selector when count is below 10 |
| Eleventh and later append before rotation | Return without storing the request |
| `FUN_004651a0` tick | Submit the previous pending list, rotate current entries into it, then clear the current count |

**Evidence.** Shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. The queue writer is `FUN_00464f60` at `0x464f60`; the drain and list rotation are in `FUN_004651a0` at `0x4651a0`, both in `docs/decompiles/full/LEGORR.full.c`. The writer is recorded in `docs/evidence/legorr-menu-background-audio.bundle` and `docs/evidence/legorr-samples-config.bundle` (`ev_fbc624c0dcf6fb79a7fa07cd5d03d2473a3afc6434b1d81e19571d7ba5930d7c`). The frame-tick flow is also traced in finding 141.
