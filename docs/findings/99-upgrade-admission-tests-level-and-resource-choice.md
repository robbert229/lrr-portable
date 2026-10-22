# 99 — Upgrade admission checks the next index and one configured resource path

The shipped executable has a dedicated building-upgrade gate at `FUN_00438770`. It reads the type/index table through `FUN_00469f40`; when that value is less than or equal to the entity's current `+0x330` index plus one, it rejects the attempt. Otherwise it compares resource slot `DAT_005560d4+0xa0` against `DAT_00556f98`. If that balance is short, it tries `DAT_005560d4+0xac` against `DAT_00556f9c`; if both are short it rejects, and if the second balance is sufficient it returns that required amount. When the first balance suffices, the output amount is zero, which selects the first payment path in the commit helper.

`FUN_0041fb50` loads those two thresholds from `BuildingUpgradeCostOre` and `BuildingUpgradeCostStuds`, with defaults 5 and 1 respectively. `FUN_0041f900` reads the two resource slots and `FUN_0041f9e0` debits one selected slot. The evidence establishes the two configured alternatives and the branch behavior; it does not identify the numeric resource slots by their in-game resource names.

## Evidence

- `ev_7cfcbdaac0e0f2b0c38b72ccbe9107bd53267918d51a42fb93849c723f67a586` — `FUN_00438770` level-index and resource gate.
- `ev_287c98f81d5b802895f9e5a2baab6d87aea6ded8423d0c7a1b740e52bfe1b66f` — `FUN_00469f40` table lookup used by the gate.
- `ev_6661c4ac8226af5cc4c49bf530108d7be57a7377f961510af502d3a15ad03561` — config keys and default thresholds.
- `ev_a54a607c7f0803241f6ae1b7765a8f3f1cf65aeb5625c3bc63c73c0f115f4535` and `ev_9f1da3e1fb201183d886614881b2a30801769d3b298a9024efb7357638d870bd` — resource-balance reads and debit paths.

Observations use REA/Ghidra 12.1.4 over shipped `source/LegoRR.exe` (SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`), 2026-10-22. Bundle: `upgrade-transition-focused.bundle`.
