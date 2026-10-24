# Finding 449 — Spatial Queries and Candidate Search

This issue maps 13 spatial query and candidate-selection functions. The wrappers build a fixed stack query and call the same entity-pool enumerator; direct evidence establishes centers, criterion masks, and the shared 10,000-unit radius. The callback's detailed spatial eligibility and output semantics are not present in the recovered pseudocode, so result meanings are qualified below.

| Address | Name | Observed behavior |
| --- | --- | --- |
| `0x00438a60` | `SpatialObjectQueryCenterMask01` | Converts the supplied in-bounds cell-coordinate pair to a world-space cell center, then builds a query with radius 10,000 and mask `0x1`; carries the remaining parameters into callback context and returns its captured result. If conversion fails, the initialized center stays `(0,0)` and the query still runs. |
| `0x00438ae0` | `SpatialObjectQueryCenterMask02` | Uses mask `0x2` and radius 10,000; centers on the supplied entity's position when non-null, otherwise converts the supplied in-bounds cell-coordinate pair to a world-space cell center. If that conversion fails, the initialized center stays `(0,0)` and the query still runs. It forwards extra query parameters and returns the callback result. |
| `0x00438b80` | `SpatialObjectQueryCenterMaskFromArgument` | Uses a supplied object position and caller-provided mask with radius 10,000. |
| `0x00438bf0` | `SpatialObjectQueryCenterMask80` | Uses an entity's position, mask `0x80`, and radius 10,000. |
| `0x00438c60` | `EntityCandidateSearch` | Uses an entity's position, mask `0x4`, and radius 10,000; the result is consumed as a candidate entity pointer by the low-resource routing caller. |
| `0x00438cd0` | `SpatialObjectQueryCenterMask20` | Uses an explicit position, mask `0x20`, and radius 10,000. |
| `0x00438d30` | `SpatialObjectQueryCenterMask10` | Uses an explicit position, mask `0x10`, and radius 10,000. |
| `0x00438d90` | `SpatialObjectQueryCenterMask200` | Uses an explicit position, mask `0x200`, and radius 10,000. |
| `0x00438df0` | `EntityLookupAtWorldPosition` | Uses an explicit position, mask `0x8`, and radius 10,000; callers use a nonzero result as an entity handle. |
| `0x00438e50` | `SpatialObjectQueryCenterMask100` | Starts from an optional explicit position, overrides it with an entity position if supplied, and queries with mask `0x100`, radius 10,000, and a caller-provided context value. |
| `0x00438ed0` | `EntityFlagsContainRequestedSingleBit` | Returns true when the requested selector `1, 2, 4, 8, 0x10, or 0x20` corresponds respectively to a set entity flag `4, 8, 0x10, 0x20, 0x40, or 0x80` from `FUN_00469e80`. |
| `0x00438f60` | `DockDefinitionMatchesCell` | Reads the `Docks` definition through `FUN_0042e8b0` and compares its returned cell coordinates to the supplied pair. |
| `0x00439200` | `TaskKindFilteredSearch` | Rejects task kinds 1, 2, and 4; for other inputs it enumerates entities with `FUN_00439240` and returns whether the callback finds one. |

## Query flow and callers

The spatial wrappers construct a callback context containing the center coordinates, radius `0x461c4000` (10,000.0f), criterion mask, and optional extra values, then call `EntityPoolFindByCallbackFiltered` (`0x004377b0`). Wrappers receiving cell coordinates convert them to a world-space cell center through `FUN_00431570` and `FUN_0044f640`; `MapCellToWorldCenter` writes coordinates only for in-bounds cells. In the first two wrappers, failed conversion leaves the initialized `(0,0)` center in place, and they still invoke the callback. Wrappers receiving explicit positions pass those through, and entity-based wrappers can obtain position through `FUN_00442260`. The filtered helper delegates to `EntityPoolFindByCallback` (`0x004377d0`) with filtering enabled. The enumerator walks entity-pool slots, checks the slot's self pointer, excludes candidates with `+0x3e8` flag-word bit `0x80000` set, and stops when the callback returns nonzero. The callback `LAB_00438fb0` is referenced but not recovered as a named body here, so exact distance tests, mask interpretation, and which entity is captured in the wrapper result remain unresolved.

Direct caller examples show distinct use cases without proving the mask names are gameplay categories. `TerrainCellRecordCountUpdate` (`0x00403810`) queries with caller-supplied mask/parameters and may fall back to a mask-7 query; `ObjectCommandTaskCreate` (`0x00403320`) uses `SpatialObjectQueryCenterMask80` for one entity kind and `EntityLookupAtWorldPosition` for another. `FUN_0043f7d0` calls `EntityCandidateSearch`, then validates routes to the candidate and creates a task command (finding 86). `EntityFlagsContainRequestedSingleBit` feeds `TaskType23CreateForCategoryMask` (`0x00402890`), which maps accepted selectors to task categories. `CellResourceEntryMatches` (`0x0040af40`) calls `TaskKindFilteredSearch`; its callback `TaskEntityFlagsAndCellTaskMatch` (`0x00439240`) requires `FUN_00469e60 bit 0x8` and a matching task via `FUN_0043a7c0`.

`DockDefinitionMatchesCell` provides a direct coordinate comparison against the loaded `Docks` record. The behavior when that configuration lookup fails is a false result; the role of the dock coordinates in its caller flow needs further tracing.

## Evidence boundary and follow-up

The center construction, masks, radius, forwarded arguments, and callback calls are direct pseudocode evidence. Calling `EntityCandidateSearch` a candidate lookup and `EntityLookupAtWorldPosition` an entity lookup follows their names and caller use; it does not establish the callback's exact ranking or match policy. Mask meanings and the callback's handling of null/invalid positions are unknown.

Focused follow-ups:

- Recover or trace `LAB_00438fb0` to establish the spatial match predicate and result capture rules.
- Compare call sites for each mask to determine whether masks represent entity classes, query modes, or another filter.
- Trace the `Docks` definition consumer and configuration failure path.
- Document task category selector values and the flag mapping in `EntityFlagsContainRequestedSingleBit` at its task callers.

Related findings: [10](10-entity-command-primitives.md), [86](86-low-resource-targets-queue-type-eleven-command.md), [256](256-command-band-step-issuers-flag-behavior-timers-and-steering-helpers.md), and [386](386-spatial-ui-com-and-crt-runtime.md). Canonical pseudocode: [LEGORR.full.c](../decompiles/full/LEGORR.full.c).
