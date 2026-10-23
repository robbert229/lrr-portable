# Finding 283 — Low-Address Resource and Geometry Helpers

Five helpers in the low-address range now have behavior-based names. The names describe visible record operations and geometry calculations; the surrounding resource and curve schemas remain partially unresolved.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x4073a0 | EntityPointerRegistriesRemove | Clears matching pointers from four global tables, advancing by two words per record. Called during entity teardown. |
| 0x4067b0 | PolylineSegmentLengthsAndTotalCalculate | Computes each adjacent point distance, stores the segment lengths, and returns their sum. |
| 0x406820 | BezierCurveSamplesBuild | Caps the requested sample count at 50, stores the start point, and generates curve samples through `CubicBezierPointEvaluate`. |
| 0x410c70 | ResourceVariantRecordRelease | Releases the record string and dispatches payload cleanup by its tag at offset `+0x14`. |
| 0x410f30 | ResourceRecordAndChildrenRelease | Releases strings, child array and children, an additional resource, and the parent record. |

The Ghidra pseudocode and callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). `0x4067b0` and `0x406820` are used by `0x00444680` and `0x0046fc70`; `0x410c70` is called for children by `0x410f30`; `0x410f30` is used on allocation failure by `0x411100`. No retained direct procedure-analysis records were found for these five addresses. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
