# Finding 294 — Render Segment and Object-State Helpers

Seven routines now have operation-level names based on direct pseudocode. Unresolved renderer bits and interface modes are left numeric or neutral.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x471860 | SceneSegmentTraceNearestHit | Sets up scene-trace callback state and invokes the nearest-hit scan over intersected map cells. |
| 0x472080 | EntityCentersWithinConfiguredRadius | Compares two entity-center positions and returns whether distance is below a configured radius. |
| 0x472130 | EntityCentersWithinObjectRadius | Performs the same center-distance check using a radius obtained from an entity/object helper. |
| 0x4728e0 | RendererFlagBit6Set | Sets or clears bit `0x40` in a renderer global. The flag meaning is unresolved. |
| 0x473ec0 | RenderObjectStateApplyToEntries | Applies a supplied value to per-type render-object entries through callbacks. |
| 0x474e90 | RenderObjectHierarchyFloat3Set | Sends a three-float value to an object and each contained object through an interface method. |
| 0x4750b0 | RenderObjectSetPackedRgbaFromFloatChannels | Converts four float channels to integers, packs them into one 32-bit color, and sends it to type-5 render objects. |

Bodies and callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). `0x471860` is called by scene segment checks; both radius tests are used as proximity gates; `0x4728e0` is toggled during render-object setup; and the object-state/color helpers are used in cloned render-resource and mesh setup paths. Direct evidence is retained in [legorr-deferred-simulation-pass-2-followup-2026-10-23.bundle](../evidence/legorr-deferred-simulation-pass-2-followup-2026-10-23.bundle) (`0x471860`), [legorr-deferred-simulation-pass-2-2026-10-23.bundle](../evidence/legorr-deferred-simulation-pass-2-2026-10-23.bundle) (`0x472080`), [legorr-ddscene-shutdown-reachability.bundle](../evidence/legorr-ddscene-shutdown-reachability.bundle) (`0x473ec0`), and [legorr-engine-helper-audit-render-expansion-8.bundle](../evidence/legorr-engine-helper-audit-render-expansion-8.bundle) (`0x474e90`). Other names rely on the checked-in Ghidra bodies and caller context. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
