# Finding 285 — Render, Collision, Fog, and Segment Helpers

Four routines now have operation-level names based on their pseudocode and relevant caller context. “Hit” and “collision” describe the tested operations; higher-level gameplay meanings remain open.

| Address | Name | Observed behavior and uncertainty |
| --- | --- | --- |
| 0x4718d0 | ObjectPairRayHitCheck | Builds the ray segment between two objects and reports a hit only when the selected hit object differs from the second object. Caller `0x0043c2b0` uses the result in an object interaction check. |
| 0x470690 | TimedEntitySegmentVisualCreate | Allocates two visual buffers, builds two segment shapes from the same endpoints with different radii/colors, and stores an owner plus a five-second duration in a free slot. No “beam” interpretation is asserted. |
| 0x470260 | EntitySurfaceSegmentCollisionCheck | Selects either a quadrilateral segment-intersection path or a proximity path based on entity flag `0x100`. Caller `0x004663f0` uses it in a surface interaction/collision path. |
| 0x473e10 | RendererFogColorSet | Packs three color channels into a value, stores it, and sends it through a renderer vtable call. Fog-color semantics are supported by callers that load and animate fog color fields. |

Direct bodies and caller paths are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). Direct records for `0x4718d0` and `0x470690` are retained in [legorr-deferred-simulation-pass-2-2026-10-23.bundle](../evidence/legorr-deferred-simulation-pass-2-2026-10-23.bundle) and [legorr-engine-helper-audit-text-io-expansion-10.bundle](../evidence/legorr-engine-helper-audit-text-io-expansion-10.bundle), respectively. The `0x470260` rectangle helper is in [legorr-engine-helpers-expansion-17.bundle](../evidence/legorr-engine-helpers-expansion-17.bundle); its proximity fallback is in [legorr-engine-helpers-expansion-28.bundle](../evidence/legorr-engine-helpers-expansion-28.bundle). Fog caller evidence for `0x473e10` is in [legorr-engine-helper-audit-low.bundle](../evidence/legorr-engine-helper-audit-low.bundle). Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
