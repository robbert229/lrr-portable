# Finding 308 — Effect Mode and Slot Helpers

Three routines resolve a render-surface query type and locate free records for effect/segment visuals.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x46fef0 | EffectModeSurfaceQueryTypeResolve | Maps effect modes 1–4 to query types `0x13`, `0x10`, `0x11`, and `5`, and writes a zeroed secondary result. |
| 0x46ff70 | ActiveEffectRecordSlotFind | Scans fixed-stride effect records and returns the first empty slot or `-1`. |
| 0x46ff90 | ActiveSegmentVisualRecordSlotFind | Scans a fixed-size segment-visual slot table and returns an empty index or `-1`. |

Their callers resolve surface queries and find free records before allocation. Direct bodies and caller paths are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c); no matching retained direct-analysis bundles were found. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
