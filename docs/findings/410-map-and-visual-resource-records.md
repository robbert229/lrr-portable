# Finding 410 — Map and Visual Resource Records

This pass connects the raw map-cell accessor and terrain ray test to visual
model records, linked resource lists, and the global command-name table. The
map-file layout and terrain mesh construction are documented in Findings 7 and
114; this finding adds the adjacent lookup, picking, and resource-record
behavior.

| Address | Name | Observed behavior and relationship |
| --- | --- | --- |
| `0x004515c0` | `MapFileGetCell` | Computes `y * width + x` from the supplied dimensions and reads the 16-bit cell at map image offset `+0x10`. It performs no bounds check; callers must constrain coordinates. `TerrainCreate` uses it while populating the per-cell terrain block table. See Findings 7 and 114 for the `.map` layout and height use. |
| `0x004515e0` | `TerrainCellRayHitTest` | Gets a cell's four surface vertices through `TerrainCellSurfaceVerticesGet` (`0x004500d0`), tests the supplied ray against the two triangles selected by the cell's diagonal, and writes the nearer valid intersection to the output point. It returns zero when neither triangle passes the plane-intersection and point-in-triangle tests. The intersection helpers do not reject negative ray parameters, so this routine can behave as a line test unless its caller constrains the direction. See Finding 388 for those helper limits. |
| `0x004519b0` | `VisualModelRecordCreate` | Formats the configured model key, tries the primary visual resource path and then a fallback, and returns null if neither resolves. On success it allocates a `0x18`-byte record holding the resource handle, a copied entry name, the supplied detail value, kind value `2`, and a null next pointer; when passed a prior tail, it links the new record through that tail's `+0x14` field. |
| `0x00451ab0` | `PrimitiveModelRecordCreate` | Creates the same `0x18`-byte linked record shape without loading a visual resource: the resource field is zero, the name is copied, the supplied detail value is stored, and kind is `2`. `PolyVariantSectionModelBuild` uses it for placeholder-valued config entries and uses `VisualModelRecordCreate` for resource-backed entries. See Finding 249. |
| `0x00451b30` | `LinkedRecordListFlatten` | Copies a six-dword linked chain into a new contiguous `count * 0x18` allocation, rewrites the `+0x14` links to the copied records, clears each copy's `+4` word, sets flag bit 0 on each record and bit 1 on the first, then terminates the final link. Used by `RendererRecordClone` (`0x00406cb0`) and `SceneRecordTemplateInstantiate` (`0x0046cff0`) to give cloned records their own list storage. |
| `0x00451bc0` | `ResourceListUpdateMatchingKey` | Walks records through `+0x14`, matching the supplied key against `+0x0c`. For a match with a null cached handle at `+4`, it passes the resource name at `+8` through `FUN_00473c60` and `ResourceNameRegisterOrQuery` (`0x00473bc0`), then caches the result. If the handle is available, it forwards the record's `+0` value and the requested state through `InterfaceArrayForwardSync` (`0x00474760`). Renderer and paired-scene update helpers call it for their resource lists; see Findings 254 and 258 for the registration and forwarding paths. |
| `0x00451c30` | `LinkedRecordField4Clear` | Walks the same `+0x14` chain and clears every node's `+4` cached-handle field. `ActivityRunnerUpdate` calls it before updating several resource lists. |
| `0x00451c50` | `OwnedObjectReferenceChainRelease` | Walks the chain, releasing the `+0` resource handle and freeing the copied name at `+8` when flag bit 0 at `+0x10` is clear. It unlinks nodes whose bit 1 is clear when a preceding retained node exists, then frees the remaining linked nodes if the walk found a node with bit 1 set. `EntityPayloadReleaseSubslots` calls it for three embedded chains. The meanings of the two flag bits remain unresolved. See Finding 278. |
| `0x00451cd0` | `MessageStringTableInitialize` | Fills the global pointer table from `0x004ff190` through `0x004ff290` with the built-in `Message_*` command-name strings. `CaseInsensitiveNameTableLookupAtOffset8` (`0x00451fd0`) scans that table, compares the names after the `Message_` prefix without case sensitivity, and returns the matching table index. It is called during engine initialization. See Finding 240. |

## Data flow

The model constructors form linked detail lists as configuration entries are
read. Resource-backed entries resolve to a visual handle, while placeholder
entries create handle-free records. Clone paths flatten those chains into
independent allocations and adjust their link/flag fields. Runtime update paths
can lazily resolve and cache a resource interface for a matching key, then
forward state to the interface array. Cleanup releases resource and name
payloads according to literal record flag checks before freeing the chain.

The map functions sit beside, but do not own, that resource-list lifecycle:
`MapFileGetCell` reads the raw 16-bit grid, while `TerrainCellRayHitTest` tests
the already-built terrain surface geometry and returns the selected
intersection point.

## Evidence and related findings

The complete checked-in Ghidra pseudocode for the nine assigned functions and
their callers is in [`LEGORR.full.c`](../decompiles/full/LEGORR.full.c).
Related traces are [Finding 7](07-level-loading.md) and
[Finding 114](114-terrain-cell-height-and-diagonal-selection.md) for map-file
and terrain construction details; [Finding 240](240-cursor-interaction-overlay-menu-and-scene-light-helpers.md)
for command-name table setup; [Finding 249](249-config-band-loaders-poly-variant-build-panel-and-cell-registration-layout-and-activity-helpers.md),
[Finding 250](250-config-band-loaders-cell-corner-scan-action-command-record-instantiation-and-render-state-helpers.md),
[Finding 251](251-command-band-kind-dispatch-item-name-lookup-route-and-record-helpers.md),
and [Finding 253](253-terrain-scatter-erosion-propagation-cell-queries-menu-records-and-buffer-helpers.md)
for model construction and list copying; [Finding 258](258-interface-method-wrappers-overlay-refresh-and-deferred-set-relays.md)
for interface forwarding; [Finding 278](278-simulation-and-terrain-helper-names.md)
for the release helper's unresolved flag semantics; [Finding 354](354-record-vector-and-resource-state-helpers.md)
and [Finding 377](377-callback-registry-and-runtime-helpers.md) for related
record and update behavior; and [Finding 388](388-geometry-and-matrix-primitives.md)
for ray intersection and point-in-polygon helper constraints.
