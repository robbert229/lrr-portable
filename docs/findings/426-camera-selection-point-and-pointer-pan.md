# Finding 426 — Camera Selection Point and Pointer Pan

This finding maps the four `player-interface/camera-selection-state` functions assigned to Issue #172 and connects them to the existing cursor-routing, selection, and camera-control findings. The canonical pseudocode is static evidence; meanings not established by callers or paired helpers remain qualified.

## Assigned functions

| Address | Function | Observed behavior |
| --- | --- | --- |
| `0x00436b0b` | `SelectionRecordPointSet` | Only when the record's first word is `1`, forwards the two supplied values to `DDiSceneObjectPositionSet @ 0x00475430` using the object pointer stored at record offset `+0x28`. Other record kinds are unchanged. |
| `0x00436b33` | `CameraGetPositionWithTerrainHeight` | For record kind `1`, obtains a three-component position through `DDiSceneMoveOrPose @ 0x004754c0` (an indirect scene-interface dispatch), queries terrain height with the supplied level/context and the position's x/y, and replaces the output z. The function returns `1` even when the kind guard prevents writing the output. |
| `0x00436b79` | `CameraRotationGet` | Returns the float stored at camera offset `+0x84`. Its paired bounded setter and transform application are described in Finding 339; the float's exact axis/rotation convention is not established here. |
| `0x00436b87` | `CameraPointerPanUpdate` | Normalizes the supplied 3D movement vector, advances a per-view accumulator at `+0x9c` by `param_3 * DAT_005560a8`, caps it at `DAT_005560a0`, scales the normalized vector by the capped accumulator and `param_3`, and adds it to the view record's three-float field at `+0x18`. Finding 102 ties those globals to the startup `CameraAcceleration` and `CameraSpeed` settings. |

## Caller and state flow

- `TerrainPointerCommandUpdate @ 0x00425d90` writes cursor-derived coordinates through `SelectionRecordPointSet` after its pointer/terrain checks and only when `DAT_00556ec8 == 1`. A second call site in `OListEntityRecordParse @ 0x0042e020` writes a point when its placement helper returns the sentinel `-1`, alongside camera angle/distance updates and matching `DAT_005563f4/f8/fc` state writes.
- `CursorRayToCameraRoute @ 0x00426420` calls `CameraGetPositionWithTerrainHeight` before building a cursor ray through `0x00429470`, forms a delta, then passes that vector and its incoming scalar to `CameraPointerPanUpdate`. Finding 61 documents the surrounding pointer bounds and ray path; this finding follows the handoff into the view-record field and camera-speed accumulator. The decompiler's stack-local pointer offsets make the exact buffer layout less clear than the call sequence.
- The caller at `0x0046a420` also references `CameraGetPositionWithTerrainHeight`, but the decompiler shows argument counts/types that conflict with the helper's three-argument signature and the `CurrentLevelHeightGet` signature. Treat that apparent data flow as unresolved rather than evidence of a valid call contract; Finding 389 records the same discrepancy.
- `CameraRotationGet` has no direct caller identified in the checked-in full corpus. Its `+0x84` field is read directly here; Finding 339 records the related setter/bounds path. The getter name does not prove the field's user-visible axis or units.

## Evidence and limits

- `docs/decompiles/full/LEGORR.full.c` contains the four assigned bodies and the call sites above. Function names correspond to the current coverage manifest.
- Finding [61](61-cursor-screen-ray-camera-routing.md) and bundle [`legorr-cursor-routing-expansion.bundle`](../evidence/legorr-cursor-routing-expansion.bundle) support the screen-to-ray-to-camera path. Its direct cursor-route observation is `ev_8761a64ae9fcd212d43808b274b938e2ac3b0ef3e4184bc7805f2599ea903559`.
- Finding [102](102-camera-config-pan-and-follow-bounds.md) and bundle [`legorr-camera-control-focused.bundle`](../evidence/legorr-camera-control-focused.bundle) support the `CameraAcceleration` / `CameraSpeed` global mapping and pointer-pan accumulator behavior. Relevant records: `ev_2cb4c64e593318467e712f388ead1aad45c0db28c3ae38ead855ef909c0785f5` and `ev_5176270c3a003e4b9aae88df79ae9427d1fe692285b2200dde6eb2be7127f81b`.
- Findings [339](339-camera-distance-and-position-helpers.md), [378](378-input-selection-and-action-state-helpers.md), and [389](389-linked-records-and-state-helpers.md) preserve prior helper semantics and the unresolved signature conflict.
- No direct retained Ghidra procedure record for the four assigned addresses was found in the checked evidence bundles. The account above relies on the checked-in decompile plus the cited neighboring analyses; it is not a runtime trace.

## Open questions and focused follow-ups

1. Resolve the mismatched call signature at `0x0046a420` and establish whether it contains a decompiler artifact or a real indirect/stack-based call convention.
2. Trace `CameraRotationGet` references outside the current full corpus and document the exact `+0x84` units/axis from read/write sites.
3. Determine the provenance and lifetime of the view-local `+0x9c` pan accumulator and the input scalar passed as `param_3`, including whether it is a frame delta.
4. Confirm the type-1 selection record's `+0x28` object and whether the point setter's values are world coordinates, terrain coordinates, or another scene-space representation.
5. Verify the overlapping stack-local layout used for the `CameraGetPositionWithTerrainHeight` output in `CursorRayToCameraRoute` before assigning a more exact position-buffer contract.

## Coverage index impact

All four assigned addresses already have `camera-selection-state` rows in `docs/coverage/function-areas.csv`, `docs/coverage/function-areas.md`, and `docs/coverage/ledger.csv`. No function-area or ledger update is required; the new finding is added to the README findings index.
