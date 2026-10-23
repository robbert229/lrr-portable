# Finding 277 — Low-Address Helper Names

These ten local routines have names grounded in the checked-in Ghidra
pseudocode and observed caller paths. Several table fields and record types
remain unknown, so the names describe the operations visible in the binary.

| Address | Name | Observed behavior and remaining uncertainty |
| --- | --- | --- |
| 0x4031e0 | ActiveEntityListContainsCode | Checks whether a code occurs in an entity's `+0x370` array, searching either a global list or the selected-object list. The code values and list modes remain unknown. |
| 0x403810 | TerrainCellRecordCountUpdate | Updates a flattened per-cell count or decrements matching records for a secondary key case. The table and key meanings remain unresolved. |
| 0x403910 | CellRecordCountsPruneForEntity | Scans the cell-count table, applies per-cell predicates, and subtracts records accepted for removal. The indexed table's exact role remains unknown. |
| 0x408860 | AnimationLoopTimeAdjust | Reads an animation time, subtracts one cycle duration per loop index, and may reset a related animation object. Whether the value is normalized or remaining time is unresolved. |
| 0x409ff0 | EntityDestroyTrackedPointerCleanup | Replaces selected global entity pointers with position-based lookups, then calls teardown and refresh helpers. The pointer globals and mask bits are not identified. |
| 0x40c2f0 | TransientEffectRenderSlotConfigure | Finds a free slot in one of two pools and configures its render object from point/vector inputs. Slot and parameter semantics remain open. |
| 0x40cdc0 | MapCellRecordGridInitialize | Allocates and clears a width-by-height grid of 20-byte records, stores the map pointer, and resets its pool/free-list state. The record schema is unknown. |
| 0x40f890 | DirectionDrivenHeadingAngleStep | Adjusts and clamps an angle field using a step and the sign/magnitude of a direction vector. Angle units and object semantics remain unresolved. |
| 0x418920 | CategoryCellCandidateListBuild | Collects records for a three-part key, filters them by a flag, and packs associated values into parallel arrays. Category and field meanings remain unknown. |
| 0x419830 | EntityLinkedListEntriesRemoveByKey | Removes keyed entries across linked-list buckets and deletes buckets that become empty. Bucket domains and payload meanings remain unknown. |

**Direct pseudocode and callers.** Each function body and its caller paths are
in [LEGORR.full.c](../decompiles/full/LEGORR.full.c): `0x4031e0` is used by
command-eligibility branches in `0x41d040`; `0x403810` is called by object and
command setup helpers `0x402d50`, `0x402e40`, `0x402fb0`, and `0x403750`;
`0x403910` is called after entity removal by `0x4039a0` and during object
teardown by `0x43a850`; `0x408860` is called from the type-4 animation path
`0x447b30`; `0x409ff0` is reached from `0x43bc40` and `0x452cc0`;
`0x40c2f0` is called from terrain/effect paths `0x40dc20` and `0x40e0c0`;
`0x40cdc0` is initialized through `0x40ce80` during map setup; `0x40f890` is
called by direction update `0x40fb90`; `0x418920` is called by
`0x418830`; and `0x419830` is called during entity teardown by `0x437540`.
The direct pseudocode corpus is Ghidra output for `reference/LegoRR.exe`,
SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
