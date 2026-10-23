# Finding 388 — Geometry, Projection, and Random Primitives

Issue #221 groups 15 shared helpers that are used by camera projection,
terrain picking, rendering, movement, and random selection. Their bodies are
available in the checked-in [full decompile corpus](../decompiles/full/LEGORR.full.c);
the rows below describe the observed operations without assigning gameplay
meaning where the callers do not establish it.

| Address | Function | Observed behavior and important callers |
| --- | --- | --- |
| `0x00477290` | `CameraScreenCoordinateProject` | Forwards its input and output pointers to the camera/render interface method at vtable offset `+0x54`. `CameraPointerRayRoute` (`0x00429470`) and `ScreenRayTerrainCellPick` (`0x004502e0`) use it to construct screen-position-dependent rays. |
| `0x004772d0` | `CameraProjectPointToScreen` | Calls the camera/render interface method at vtable offset `+0x50` into a temporary, then writes two output scalars. The decompiler's local types make the quotient operands ambiguous, so the exact divide is unresolved here. Callers such as `TickEntityScreenEffects` (`0x004075a0`) and `SelectedEntityGroundMarkerRender` (`0x004248b0`) consume the result as screen coordinates. |
| `0x00479500` | `D3DRMVectorRandom` | Thin wrapper around the imported Direct3DRM `D3DRMVectorRandom`, forwarding its output pointer. `SteeringPhysicsStep` (`0x0040fb90`) and `ParticleSlotInitializeForEmitter` (`0x00465c50`) use it to obtain randomized vectors. The distribution and output constraints are defined by the imported routine, not this wrapper. |
| `0x00479510` | `D3DRMVectorRotate` | Thin wrapper around imported `D3DRMVectorRotate`, forwarding vector, quaternion, output, and scalar arguments. `EntityVelocityVectorDispatch` (`0x00442300`) and `StepMovementVectorResolve` (`0x00444680`) call it for vector rotation. |
| `0x00479530` | `SurfaceTriangleNormalCalculate` | Forms a cross product from the two triangle edges and normalizes it into the output vector. Terrain surface construction and `TerrainCellSurfaceNormalGet` (`0x00463590`) call it. There is no zero-length guard before the reciprocal square root, so degenerate input is not handled explicitly. |
| `0x00479630` | `SurfaceTriangleAreaCalculate` | Computes three edge lengths, selects a longest edge, derives an angle using the math runtime, and returns half the product of the selected adjacent edge lengths and the sine of that angle. With the fourth argument zero, as in `ProjectedQuadScreenAreaCalculate` (`0x0040b470`), it takes the local edge/angle path. A nonzero flag calls the normal helper and affects the orientation branch, but the decompiler's local stack typing overlaps that output with a scalar; avoid claiming more about the orientation convention without instruction-level confirmation. |
| `0x004798b0` | `RandomFloatRange` | Calls `RandomNext` and scales its 15-bit result by approximately `1/32768` into `min + r * (max - min)`. Numerous callers use it for jitter and placement ranges. The implementation has no explicit bound validation; with ordinary ordered bounds the maximum endpoint is excluded. |
| `0x004798e0` | `ViewRayIntersectionTest` | Normalizes the supplied direction and plane-normal vectors, calls `RayPlaneIntersectionParameter`, and writes the point through `PointAlongRay` on success. `CameraPointerRayRoute` (`0x00429470`) and `TerrainCellRayHitTest` (`0x004515e0`) use it in screen/world picking. It does not reject a negative intersection parameter, so the body tests the infinite line unless its caller constrains the inputs. |
| `0x00479a30` | `RayPlaneIntersectionParameter` | Computes `dot(planePoint - origin, normal) / dot(direction, normal)`, writes the scalar, and returns false only when the denominator is exactly zero. It does not itself require a positive parameter. |
| `0x00479ab0` | `PointAlongRay` | Writes the three-component point `origin + t * direction`; called by `ViewRayIntersectionTest`. |
| `0x00479af0` | `LineIntersection2DCompute` | Computes an intersection point from a 2D origin and direction pairs, returning null when its determinant-like denominator is exactly zero. It does not test segment bounds. `TerrainSurfacePointProjectOffset` (`0x0044fb90`) and `EntitySurfaceQuadSegmentIntersect` (`0x004702b0`) use it in geometry calculations. |
| `0x00479b80` | `PointInPolygon2D` | Applies the even/odd horizontal-ray crossing rule over `count` vertex pairs and returns crossing parity. `ScreenRayTerrainCellPick` (`0x004502e0`) and `TerrainCellRayHitTest` (`0x004515e0`) use it on projected terrain geometry. Boundary inclusion is not separately defined; horizontal edges and exact vertex hits deserve caution because the calculation has no explicit boundary/zero-span handling. |
| `0x00479c10` | `SphereRayIntersectionDiscriminantPositive` | Computes a discriminant-like expression from two vector triples and two scalar inputs, returning whether it is strictly positive. The exact parameter meanings and whether callers use it as a complete ray/sphere test remain unresolved; retain the operation-level name rather than infer hit semantics. |
| `0x0048e160` | `RandomSeedSet` | Stores its input in global RNG state `DAT_004afcc8`. `LevelLoad` (`0x00429890`) calls it after obtaining a seed value from a loaded record. This is a shared global stream, so consumers affect one another's subsequent draws. |
| `0x0048e170` | `RandomNext` | Advances `DAT_004afcc8` with `state = state * 0x343fd + 0x269ec3`, then returns `(state >> 16) & 0x7fff`. `RandomFloatRange` and many independent engine systems consume this same stream; for example, `TerrainCellNeighborFloodWalk` (`0x0040d850`) uses draws to gate neighbor propagation. |

## Data flow and boundaries

The camera wrappers bridge a stored interface pointer (`camera/context +
0x10`) to renderer methods. `CameraPointerRayRoute` projects screen inputs,
forms a direction from projected points, and sends that direction through the
view-ray/plane helpers. Terrain picking then combines ray-plane results with
polygon inclusion tests. These caller chains establish their use in picking;
they do not establish a universal coordinate convention for all interface
methods.

The random helpers are a separate shared service: seeding writes one global
word, draws mutate it, and float-range draws consume one integer draw each.
Per-level seed initialization does not make later consumers independent.
`D3DRMVectorRandom` is a different API path and should not be conflated with
the game's `RandomNext` stream.

The surface area and normal helpers operate on caller-provided coordinates
without ownership of terrain or render state. Existing caller context shows
terrain normals and projected-quad area use; the geometry wrappers themselves
do not establish broader gameplay policy.

## Evidence and unresolved questions

Direct bodies are in `docs/decompiles/full/LEGORR.full.c` at the addresses in
the table. Additional retained decompile records include
`ev_77d33b453abb5e0818a3c20d7632b32618254e6b6e550b92fed8257ce6b6b323` and
`ev_d65bb223e6ded6a8dd1841c84019ff5cb0af216dd0ac711956dc5d09e1b67ced` in
`legorr-engine-helper-audit-render-expansion-9.bundle` (camera projection,
surface normal, and triangle area), plus
`ev_50ac433a55f0fe9d174a1fccee1298b2eddb40d022f7beef590478017d4dd352` in
`legorr-engine-helpers-expansion-17.bundle` (triangle area). The camera/render
caller context is also summarized in finding 235; ray, polygon, and quadratic
helpers are cross-referenced in findings 301, 309, and 323.

Unresolved points suitable for focused follow-up:

- Identify the camera interface method contracts, quotient operands, and
  matrix/coordinate conventions behind vtable offsets `+0x50` and `+0x54`.
- Establish the orientation convention and degenerate-triangle behavior of
  `SurfaceTriangleAreaCalculate` from instruction-level evidence.
- Confirm whether callers constrain ray parameters to the forward half-ray and
  define polygon boundary behavior.
- Recover semantic parameter meanings and caller use for
  `SphereRayIntersectionDiscriminantPositive`.
- Determine whether any code reseeds the global RNG during runtime and map
  which subsystems consume draws on the same simulation path.
