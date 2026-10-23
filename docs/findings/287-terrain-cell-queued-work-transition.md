# Finding 287 — Terrain Cell Queued-Work Transition

`0x431140` is now named `TerrainCellQueuedWorkTransition`. It checks that the cell has flag `0x10` and none of flags `0xe0`, sets bit `0x20`, updates terrain state, queues the cell coordinate, and emits event `0x1c`. The meanings of those bits and the event are not established, so the name describes the visible state transition.

Its body and caller `0x449980` are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). The caller reaches it after a terrain/activity eligibility transition and then clears route markers. Direct function evidence is retained in [legorr-sim-terrain-third-pass-2026-10-23.bundle](../evidence/legorr-sim-terrain-third-pass-2026-10-23.bundle), record `ev_04756413c5a9f453f41bb9303afa160709b3668aae9f32ec7b2f6321250b227c`. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
