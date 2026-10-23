# Finding 299 — Task, Interface, and Grid Helpers

Four low-address routines now have operation-level names supported by their bodies and callers.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x4041b0 | EntityTaskChainContainsKind | Walks an entity's linked chain at `+0x2f0` and tests whether a node's first field matches a requested kind. |
| 0x40cf80 | GridCellObjectLinkInsert | Links an object into a grid-cell entry, sets an object flag, and updates the cell. |
| 0x412450 | RenderUiFrameCell | Draws an entry directly or resolves a frame descriptor and submits it for rendering. |
| 0x41a660 | InteractionActionCodeMapInitialize | Initializes indexed action-code lists through the action-code registration helper. |

The Ghidra bodies and caller context are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). Callers use these in task interaction checks, grid object registration, interface frame drawing, and interface initialization. No matching direct procedure-analysis records were found in retained bundles. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
