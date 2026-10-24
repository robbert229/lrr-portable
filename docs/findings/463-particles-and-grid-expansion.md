# Particles and grid expansion

The assigned functions split into a transient-effect particle path and a terrain-cell record / expansion path. The decompilation supports the data flow and bit operations below; it does not establish semantic names for every flag or grid field.

## Function map

| Address | Function | Behavior and evidence | Callers / limits |
| --- | --- | --- | --- |
| `0x00465c50` | `ParticleSlotInitializeForEmitter` | Initializes a particle slot at `emitter + slot * 0x3c`: marks it active, creates its render object, obtains a random vector through `D3DRMVectorRandom` (`0x00479500`), derives initial velocity and position from emitter fields, stores an associated transform, assigns lifetime `rand % 0x1e + 0x28`, and updates a scale field. If emitter flag `+0x294 & 8` is set, increments `+0x284` and sets bit `0x10` when the old count is 3; later writes `4` to `+0x294`. | Called for empty slots by `TransientEffectEmitterCreate` (`0x00465430`) and `TransientEffectInstanceUpdate` (`0x00465b00`). Existing Finding 388 documents the random-vector wrapper. The meaning of emitter bits and returned variation value remain unknown. |
| `0x00465e00` | `AdvanceTimedVisualSegmentAndFade` | Advances the slot's three position components by velocity times `param_3`, accumulates absolute vertical displacement in slot `+0x38`, and deactivates/releases the render slot when that accumulator reaches lifetime at `+0x34`. Otherwise advances elapsed time at `+0x30` and sets alpha to `0.7 - progress * 0.7`. | Called for active slots from `TransientEffectEmitterCreate` (`0x00465430`) and `TransientEffectInstanceUpdate` (`0x00465b00`). The expiry threshold uses accumulated absolute vertical travel, not elapsed time. |
| `0x00465f40` | `InstanceQuadVerticesBuild` | Reads an orientation/transform from the scene object, derives two side vectors, scales them by slot width and height factors, and assembles four corners around the slot position before submitting four vertices with `FUN_00482500`. | No direct static call site found in the decompile. Vertex winding, render-state meaning, and the two scalar inputs are not fully recoverable from this helper alone. |
| `0x004661c0` | `GridCellRecordBufferAllocate` | Allocates `width * height * 0x0c` bytes using dimensions at grid descriptor `+0x14/+0x18`, zeroes the records, and stores the descriptor in `DAT_005520ec`; the buffer pointer is `DAT_005520e8`. | Called by `GridCellRecordBufferReplace` (`0x00466230`). Allocation failure is not propagated; descriptor is still stored. |
| `0x00466210` | `EntityFieldGridBufferRelease` | Frees `DAT_005520e8` if non-null, then clears that pointer. | Called by `GridCellRecordBufferReplace` (`0x00466230`). It does not clear the descriptor global. |
| `0x00466230` | `GridCellRecordBufferReplace` | Releases the current cell-record buffer and allocates/zeros a replacement for the supplied grid descriptor. | Called during `LevelLoad` (`0x00429890`) and `MapLoadPipeline` (`0x0042b510`). The record is 12 bytes; its exact field meanings are only partly established by consumers. |
| `0x00466250` | `ExpandCellEntitySpawn` | For a grid cell whose record flags at `+8` do not include `0x100`, asks `CanSpawnExpansionEntityAtGridCell` for eligibility and orientation, maps the cell to world coordinates, creates an entity with type argument `0x0c`, stores it in the record, sets entity bit `0x100`, and starts the `Expand` animation. | Called from `LevelDebugInputCellMutatorDispatch` (`0x004288e0`) when a debug cell state changes. It returns 1 only after spawning. Existing Finding 256 documents the entity-cell helpers at `0x004649b0` and `0x00464a30`; this routine does not itself establish what expansion represents in gameplay. |
| `0x00466380` | `SetExpansionEntityAtGridCell` | Stores an entity pointer in the cell record's first word, ORs `0x100` into record flags at `+8`, and writes `100.0f` at `+4`. | Called by `ExpandCellEntitySpawn` (`0x00466250`) and `OListEntityRecordParse` (`0x0042e020`) while restoring a parsed entity. The 100.0 value's timer/scale semantics are not proven. |
| `0x004663f0` | `CanSpawnExpansionEntityAtGridCell` | Requires bit `0x8` in the terrain-cell flags at `+8`. If both inspected side-neighbor flags include `0x10`, returns eligible with angle `0x3fc90fdb` (approximately π/2); otherwise requires both vertical-neighbor flags to include `0x10`, returns eligible with angle 0. | Direct caller is `ExpandCellEntitySpawn` (`0x00466250`). It reads a distinct terrain buffer from grid descriptor `+0xb8`, with `0x48`-byte terrain-cell stride; neighbor indices have no local bounds checks. The meanings of terrain bits `0x8` and `0x10` are not named here. |
| `0x00466490` | `CellSightRayCheckAndFlag` | Maps an entity to its cell in the 12-byte entity-cell record buffer, requires record bit `0x100`, obtains the cell world point and performs collision query `FUN_00470260(entity, cellPoint, worldPoint, 0)`. On hit, ORs `0x02000000` into `+0x3e0` for both the checking entity and the cell's entity. | Called by callback `LAB_004665e0`, which is passed to entity walker `FUN_004377b0` from relay `FUN_004665c0`. `EngineTick` invokes the relay (`0x00423620`; see Finding 03 and the call-site trace in Finding 256). The callback-to-helper step is indirect in this pseudocode. Existing Finding 256 describes the collision helper and its sole caller. The flag's semantic name is not established. |
| `0x004665c0` | callback relay | Passes `LAB_004665e0` and its stack context to `FUN_004377b0`, the callback-walker routine. | Called by `EngineTick` (`0x00423620`; see Finding 03). It passes callback `LAB_004665e0` to entity walker `FUN_004377b0`; the walker enumerates entities and dispatches the callback, which invokes `CellSightRayCheckAndFlag` (`0x00466490`). |
| `0x00466750` | `GridCellInvalidateAndClearFlags` | For the entity stored in the cell record, ORs `0x800000` into entity field `+1000`, clears entity flag `+0x3e0 & 0x02000000`, and clears cell-record bit `0x100` at `+8`. | No direct static caller found. Existing Finding 377 records the same pseudocode. The invalidation flag and dirty field semantics remain unresolved. |

## Grid record layout

| Offset | Observed use |
| --- | --- |
| `+0x00` | Entity pointer (set/read by expansion, sight check, and invalidation helpers). |
| `+0x04` | `100.0f` on entity assignment; broader meaning unknown. |
| `+0x08` | Entity-cell record bit field: `0x100` is set/cleared for an assigned entity and required by the sight check. |

`CanSpawnExpansionEntityAtGridCell` reads a separate terrain-cell buffer through descriptor `+0xb8`; those records have `0x48`-byte stride and use bits `0x8` and `0x10` in the eligibility check. These flags are not part of the 12-byte entity-cell record above.

## Follow-up boundaries

- Trace terrain-cell bits `0x8` and `0x10`, plus the separate entity-cell bit `0x100`, before assigning domain names.
- Identify any indirect/renderer caller of `InstanceQuadVerticesBuild` and recover its parameter contract.
- Inspect the callback target and adjacent engine-tick dispatch if the sight-ray scan needs a fuller entity iteration description.
