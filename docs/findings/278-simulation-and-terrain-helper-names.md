# Finding 278 — Simulation and Terrain Helper Names

These seven routines have behavior-level names supported by direct Ghidra
pseudocode and caller context. Field meanings and higher-level feature roles
remain open where noted.

| Address | Name | Observed behavior and remaining uncertainty |
| --- | --- | --- |
| 0x439bd0 | EntityAcceptsTargetForTaskCheck | Filters entity kind/state bits, action categories, optional state predicates, and owner/current-task category before target pathing. The flag and category meanings remain unknown. |
| 0x451230 | TerrainCellSurfaceVectorWrite | For an in-bounds cell/orientation, normalizes orientation when a cell flag is set and replaces the stored three-component surface vector. The vector's role is unknown. |
| 0x451450 | TerrainCellSurfaceVectorOffsetAdd | Adds a supplied three-component vector to a per-cell/per-orientation vector. Coordinate frame and vector meaning remain unknown. |
| 0x451c50 | OwnedObjectReferenceChainRelease | Walks a linked chain, frees payloads selected by flag bits, unlinks retained entries, and frees remaining chain nodes. Payload types and ownership bits remain unknown. |
| 0x453220 | AdvisorDisplayStateResourcesRelease | Releases active resources and queued items, frees arrays, clears related globals/tables, and runs a global cleanup helper. The exact UI subsystem remains uncertain. |
| 0x45a500 | PanelImageSlotEntryAllocate | Allocates a 0x10-byte text-output wrapper and a text buffer using the supplied font handle, rectangle dimensions, and capacity; stores converted Y/X coordinates and the indexed category pointer. Coordinate units and font-resource ownership remain caller-dependent. |
| 0x45d7e0 | LevelIdentifierRemapTableLoad | Reads paired identifiers from a child list, converts each pair, and stores them in a global remap table. Identifier domain and table capacity remain unresolved. |

**Direct pseudocode and callers.** The direct bodies and caller paths are in
[LEGORR.full.c](../decompiles/full/LEGORR.full.c): `0x439bd0` is called by
`0x439a20`, `0x43a300`, and `0x404fc0` before target pathing;
`0x451230` is called during terrain update by `0x450f30`; `0x451450` is called
from `0x44f380`; `0x451c50` is called during entity/object cleanup by
`0x406ec0`; `0x453220` is called by level teardown `0x42f0c0`; `0x45a500` is
called by panel loaders `0x45b740` and `0x45b850`; and `0x45d7e0` is called by
level config setup `0x429890`, with companion lookup `0x45d870` rewriting
matching identifiers. Additional cleanup evidence for one `0x453220` item
action is in [legorr-engine-helpers-expansion-9.bundle](../evidence/legorr-engine-helpers-expansion-9.bundle),
record `ev_f6de601194598c8371878bbc355055a40b3ade5c5d74535b963ca9a654b86795`.
The pseudocode is static Ghidra output for `reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`).
