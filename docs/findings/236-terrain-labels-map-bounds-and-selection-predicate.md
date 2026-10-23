# Finding 236 — Terrain Labels, Map Bounds, and Selection Predicate

Three small helpers exposed useful state that had remained hidden behind
`FUN_` names.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x435170 | TerrainSurfaceTypeNameGet | Returns the indexed surface-type name, or the literal `Need config for surface type` when that entry is null. A terrain feedback routine uses it to describe cell types. |
| 0x435f28 | TerrainGridBorderInsetGet | Returns a shared integer used by map-coordinate clamp paths as the minimum inset from the grid edge. The global's broader purpose remains inferred from those callers. |
| 0x4525b0 | SelectedEntitySetHasAny | Returns whether the selected-entity count is nonzero. Nearby accessors expose the matching count and member-array storage; callers use this as a selected-set presence test. |

**Evidence.** Direct decompiles and caller context are preserved in
[legorr-engine-helper-audit-terrain-expansion-9.bundle](../evidence/legorr-engine-helper-audit-terrain-expansion-9.bundle).
The direct observations are `ev_0c28e20550f8b810decffceb4a414e9e41ad3fdca583ed146c31125c8c70c164`,
`ev_3aa7842ac81f8a1d247ae757b8fb72065e3f3679a72069fd19f6ac8ebfbf1cce`, and
`ev_70f748b2995a85538c0dcfe3fdf0d62ecd087af38d10f7d0ca1263779e381feb`.
The selected-set interpretation is corroborated by `ev_6593fc3058957a4d04dcea9d3dd86bc9f8a6310347ad02413a685658528848fe`,
`ev_74d512d65d56ed6ebd0a29c845cefabfec3ae18227355e88e0d1183003671c48`,
and `ev_9d00a15036893334beb9f7312470bc08847be92a032e9f58de6f61765314f1e3`.
Analyses used shipped `reference/LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
