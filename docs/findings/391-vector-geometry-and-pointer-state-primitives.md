# Finding 391 — Vector, Geometry, and Pointer-State Primitives

This finding maps the 29 functions assigned to `math-geometry`. Most are literal scalar/vector, curve, projection, or matrix operations. The issue's assigned area also includes three pointer-stack helpers and one pointer-action state dispatcher; these are recorded separately because their behavior is structural rather than geometric.

| Address | Name | Observed behavior and context |
| --- | --- | --- |
| `0x00401240` | `Vector2Distance` | Returns Euclidean distance between two 2D vectors. |
| `0x004013a0` | `Vector3Length` | Returns the Euclidean length of a 3D vector. |
| `0x00401420` | `Vector3Distance` | Returns Euclidean distance between two 3D vectors. |
| `0x00401470` | `Vector3CrossProduct` | Writes the cross product of its two input vectors to the output vector. A caller in `docs/decompiles/full/LEGORR.full.c` uses it to construct a perpendicular direction before normalization. |
| `0x00401500` | `Vector3Add` | Adds corresponding components of two 3D vectors into an output vector. |
| `0x004016c0` | `Vector3Subtract` | Writes the second input minus the third input component by component. |
| `0x004016f0` | `Vector3DotProduct` | Returns the sum of pairwise component products. |
| `0x00401720` | Provisional alias `Vector3Scale` (`FUN_00401720`) | Multiplies each component of a 3D vector by one scalar; used alongside add, subtract, dot, cross, and normalize in spatial calculations. |
| `0x00401750` | `Vector3Normalize` | Scales a vector by the reciprocal of its Euclidean length. The pseudocode has no zero-length guard. |
| `0x004065f0` | `CubicBezierPointEvaluate` | Evaluates the two-coordinate cubic Bézier polynomial for four control-point vectors and parameter `t`. `BezierCurveSamplesBuild` calls it. |
| `0x00406730` | `Vector2DistanceVariant` | Computes the same 2D Euclidean distance as `Vector2Distance`; used by the polyline length builder. |
| `0x00406760` | `Vector2SetLength` | Rescales a 2D vector to the requested length by dividing by its current length and multiplying to target magnitude; zero-length/zero-target cases are not guarded. |
| `0x004067b0` | `PolylineSegmentLengthsAndTotalCalculate` | For the stored point sequence, writes each adjacent 2D segment length into the companion length array and returns their sum. Used after sample generation. |
| `0x00406820` | `BezierCurveSamplesBuild` | Stores the start point and samples a two-dimensional cubic Bézier at evenly spaced `t` values; caps the requested sample count at 50. |
| `0x004068c0` | `PathArcLengthInterpolate` | Accumulates stored segment lengths to find the segment containing the requested distance, interpolates between its adjacent points, and returns the segment index; copies the final point when the distance passes the stored path. Callers include `PathStepAdvanceAndArrivalCheck` (Finding 253). |
| `0x004089d0` | `Segment2DPointDistance` | Computes distance from a point to a 2D segment by using normalized segment direction and endpoint tests to choose a perpendicular distance or an endpoint distance. Used by the polyline minimum-distance helper. |
| `0x00408b00` | `PointToPolylineMinimumSegmentDistance` | Calls `Segment2DPointDistance` for the first segment and remaining adjacent pairs, returning the minimum result. Reached from entity movement/world-position routines (Finding 256). |
| `0x00408b60` | `PointProjectOntoLine2D` | Projects a point onto the infinite line through two points after normalizing their direction; it does not clamp to the endpoints (Finding 373). |
| `0x00408bf0` | `Point2DProjectOntoDirection` | Projects a point onto the infinite line from a start point along a normalized direction; no segment clamp is applied (Finding 374). |
| `0x0040b470` | `ProjectedQuadScreenAreaCalculate` | Projects four quad corners through a matrix, divides x/y by w, computes two triangle areas, and optionally reduces or zeros the result based on corner bounds/depth. Sole caller: `TerrainProjectedCellStateRefresh` (Finding 247). |
| `0x0040bd60` | `BuildPerspectiveProjectionMatrix` | Clears a 4x4 matrix and writes a sparse projection form from six scalar inputs. Coordinate and parameter conventions remain unresolved (Finding 376). |
| `0x0040dd90` | `ComputeWorldPositionDelta` | Converts two grid-coordinate pairs through the current terrain context into world positions, then writes the three-component delta between the resulting positions (Finding 376). |
| `0x0040fb00` | `MatrixRotationComposeXYZ` | Builds three rotation matrices from values at record offsets `+0x24`, `+0x28`, and `+0x2c`, multiplies them in sequence, and writes the result at `+0x58`; exact axis labels/order semantics are not independently established (Finding 374). |
| `0x0040fcc0` | `VectorScaleComponentsInPlace` | When its vector pointer is nonnull, multiplies the three components by three independently supplied scale values (Finding 366). |
| `0x0040fe40` | `PointerStackPush` | When the container and item are nonnull, links the item to the current head through item offset `+0x9c`, makes it the head, and increments the count (Finding 366). |
| `0x0040fe60` | `PointerStackPop` | If the count is nonzero, returns the head, advances through item offset `+0x9c`, and decrements the count (Finding 366). |
| `0x0040fe90` | `PointerStackForEach` | Traverses the item chain through offset `+0x9c` and calls a supplied callback with the container, item, and context (Finding 366). |
| `0x0040ff70` | `ObjectVector3Set` | Writes three supplied values to object offsets `+0x3c`, `+0x40`, and `+0x44`; caller `FUN_0044bb30` (Finding 381). |
| `0x0041c6e0` | `PointerActionStateDispatch` | Tracks a kind and coordinate pair in globals, resets to sentinel `0x4a` when disabled, and dispatches kind `0x49` through `0x0041ca20`; caller `SimulationPointerActionDispatch` (Finding 254). |

## How the helpers connect

The vector operations are small building blocks rather than an owner subsystem. In the checked-in pseudocode they combine in camera/scene calculations, collision or path queries, world-coordinate conversion, and motion code. The curve path is more explicit: `BezierCurveSamplesBuild` uses `CubicBezierPointEvaluate`; `PolylineSegmentLengthsAndTotalCalculate` derives segment lengths from those samples; `PathArcLengthInterpolate` consumes the point/length layout to map distance back to a point. Separately, `PointToPolylineMinimumSegmentDistance` repeatedly uses the segment-distance helper, and the pointer-stack routines are used by callback traversal sites.

Names and arithmetic above follow the canonical pseudocode in [`LEGORR.full.c`](../decompiles/full/LEGORR.full.c). Related caller and prior analysis notes are in Findings [247](247-config-graphics-camera-tween-and-pool-spawn-helpers.md), [253](253-terrain-scatter-erosion-propagation-cell-queries-menu-records-and-buffer-helpers.md), [254](254-command-sweeps-cell-issuing-config-priorities-statistics-and-directdraw-create-helpers.md), [256](256-command-band-step-issuers-flag-behavior-timers-and-steering-helpers.md), [280](280-vector-math-helper-names.md), [283](283-low-address-resource-and-geometry-helpers.md), [333](333-advisor-name-and-vector-math-helpers.md), [366](366-task-records-callback-dispatch-and-pointer-stacks.md), [373](373-cursor-simulation-renderer-and-file-helpers.md), [374](374-terrain-interface-and-media-helpers.md), [376](376-selection-cell-and-overlay-state-helpers.md), [377](377-callback-registry-and-runtime-helpers.md), and [381](381-selection-resource-runtime-and-grid-helpers.md). Exact coordinate conventions, degenerate-input behavior for several geometric helpers, and domain meanings for the two pointer-state routines remain open where the pseudocode/callers do not settle them.
