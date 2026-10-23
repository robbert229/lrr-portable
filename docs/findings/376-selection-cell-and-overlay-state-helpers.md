# Finding 376 — Selection, Cell, and Overlay State Helpers

This pass names 47 operation-level routines across cell records, selection/input flags, UI state, scene transforms, and DirectDraw/audio helpers. Numeric flag names preserve unresolved bit meanings.

| Address | Name | Observed behavior | Evidence and caller context |
| --- | --- | --- | --- |
| `0x00408e10` | `CountMatchingCoordinatesInList` | Counts records whose coordinate pair at +0x2c matches the two supplied integers. | Checked-in pseudocode; no matching direct evidence bundle record reported. |
| `0x00408e50` | `CreateRepeatedResourceInstances` | In one type/variant case sets a flag, increments an object counter, and calls the creation helper the requested number of times. | Resource kind and exact role of the name argument remain unknown. |
| `0x00409110` | `ReleaseSelectionGroupResources` | Resolves a group, notifies selection logic, and calls two cleanup helpers. | Member flag meanings remain unknown. |
| `0x00409150` | `ClearSelectionGroup` | Deactivates each group member, clears flags, and invokes group/selection cleanup. | Flag meanings remain unknown. |
| `0x00409940` | `RefreshEightNeighborCells` | Calls the tile refresh helper for each of eight fixed coordinate offsets. | Checked-in pseudocode; no matching direct evidence bundle record reported. |
| `0x004099d0` | `ReleaseAllGroupRecords` | Walks a global linked list through +0x24 and releases every record. | Exact record type unknown. |
| `0x0040aea0` | `EnsureCellEntriesLoaded` | For a level/row/column cell, obtains associated entries, validates them, and marks the cell loaded. | Resource terminology is inferred from helper behavior. |
| `0x0040af40` | `CellResourceEntryMatches` | Validates an item, compares two fields, and applies an optional index bound. | Checked-in pseudocode; no matching direct evidence bundle record reported. |
| `0x0040b250` | `CellFlag4IsSet` | Returns whether cell flag bit 0x4 is set. | Checked-in pseudocode; no matching direct evidence bundle record reported. |
| `0x0040b2a0` | `CellFlag100Set` | Sets cell flag bit 0x100. | Checked-in pseudocode; no matching direct evidence bundle record reported. |
| `0x0040b2e0` | `CellFlag100IsSet` | Returns whether cell flag bit 0x100 is set. | Checked-in pseudocode; no matching direct evidence bundle record reported. |
| `0x0040bd60` | `BuildPerspectiveProjectionMatrix` | Builds a perspective projection matrix from six scalar parameters. | Coordinate convention remains unknown. |
| `0x0040c570` | `ReadCellTablePair` | Reads and returns two dwords from an indexed cell table. | Meaning of the returned fields is unknown. |
| `0x0040c8a0` | `GetResourceSlotAddressByIndex` | Returns the global slot address selected by an index from 0 through 9. | Slot contents are unresolved. |
| `0x0040d170` | `CreateCellObjectIfAvailable` | Checks two cell predicates, obtains world coordinates, creates an object, and records it at the cell. | Object subtype remains inferred from helper ID. |
| `0x0040d1f0` | `DeleteCellObject` | Fetches a cell object, releases its associated resource, then deletes the object. | Checked-in pseudocode; no matching direct evidence bundle record reported. |
| `0x0040dd90` | `ComputeWorldPositionDelta` | Maps two grid coordinates to world xyz positions and subtracts the first from the second. | Checked-in pseudocode; no matching direct evidence bundle record reported. |
| `0x004114f0` | `HasType6ItemAtIndex` | Bounds-checks a list entry, requires numeric type 6, then runs a lookup helper. | The type’s domain meaning is unknown. |
| `0x004119d0` | `HasActiveResource` | Checks object field +0x38 and then queries global active-resource state. | Exact resource kind unknown. |
| `0x00417fe0` | `CurrentSelectionContextGet` | Returns the current selection-context global. | Checked-in pseudocode; no matching direct evidence bundle record reported. |
| `0x00417ff0` | `CurrentSelectionContextSet` | Stores the current selection-context global. | Checked-in pseudocode; no matching direct evidence bundle record reported. |
| `0x00418120` | `IsAverageValueAbove99` | Averages float-converted entries beginning at index 8 and returns whether the integer average exceeds 99. | Source array meaning unknown. |
| `0x00418f80` | `SelectionModeSetByIndex` | Maps index 0 to helper mode 8 and index 1 to helper mode 9. | Mode meanings unknown. |
| `0x00419450` | `SelectionFlag10SetClear` | Sets or clears bit 0x10 in the selection flags global. | Checked-in pseudocode; no matching direct evidence bundle record reported. |
| `0x004194f0` | `IndexedSelectionFlag20000SetClear` | Sets or clears bit 0x20000 in an indexed selection record. | Checked-in pseudocode; no matching direct evidence bundle record reported. |
| `0x0041c9f0` | `InputEntryFlag8SetClear` | Sets or clears bit 0x8 in the input-entry record. | Checked-in pseudocode; no matching direct evidence bundle record reported. |
| `0x0041ca20` | `PlayerInputFlag8SetClear` | Sets or clears bit 0x8 in the player-input record. | Checked-in pseudocode; no matching direct evidence bundle record reported. |
| `0x0041ca60` | `InputEntryFlag8Get` | Returns whether bit 0x8 is set in the input-entry record. | Checked-in pseudocode; no matching direct evidence bundle record reported. |
| `0x0041ca70` | `PlayerInputFlag8Get` | Returns whether bit 0x8 is set in the player-input record. | Checked-in pseudocode; no matching direct evidence bundle record reported. |
| `0x0041cfb0` | `CancelActionForMatchingCoordinates` | When current action is 1 or 4 and stored global coordinates match, invokes the cancel helper. | Checked-in pseudocode; no matching direct evidence bundle record reported. |
| `0x0041e990` | `CellActionPermissionGet` | Returns the per-cell table byte when a global mode bit is set, otherwise returns 1. | Mode and table value meanings unknown. |
| `0x0041f230` | `QueueActionRequestType2` | Builds a small zeroed request record with the observed type-2 field and enqueues it. | Checked-in pseudocode; no matching direct evidence bundle record reported. |
| `0x0041f270` | `QueueActionRequestValue` | Builds a request record with the observed value field and enqueues it. | Checked-in pseudocode; no matching direct evidence bundle record reported. |
| `0x0041f2b0` | `QueueActionRequestFlag100000` | Builds a request record with bit 0x100000 and enqueues it. | Checked-in pseudocode; no matching direct evidence bundle record reported. |
| `0x0041f2f0` | `QueueActionRequestPair` | Builds a request record with the observed pair fields and enqueues it. | Checked-in pseudocode; no matching direct evidence bundle record reported. |
| `0x0041f340` | `QueueActionRequestIndex` | Builds a request record with the observed index field and enqueues it. | Checked-in pseudocode; no matching direct evidence bundle record reported. |
| `0x0041f3c0` | `QueueCallbackRequest390` | Builds a request record and enqueues it through callback offset +0x390. | Checked-in pseudocode; no matching direct evidence bundle record reported. |
| `0x0041f400` | `QueueCallbackRequest3E0` | Builds a request record and enqueues it through callback offset +0x3e0. | Checked-in pseudocode; no matching direct evidence bundle record reported. |
| `0x0041f970` | `RuntimeFlag40000000Get` | Returns whether bit 0x40000000 is set in the runtime flags value. | Checked-in pseudocode; no matching direct evidence bundle record reported. |
| `0x0045d370` | `RegionBoundsSet` | Stores two supplied coordinate pairs in global region-bound fields. | Caller `0045d110` uses it on a configured region match; context `ev_201d0cb6f607dbca45ff46536bcefc028651d7a6a44daa790adfa1f21343e84e` in `legorr-engine-helpers-expansion-17.bundle`, not direct callee evidence. |
| `0x0044a390` | `Type11CellCoordinateAppend` | Appends an (x,y) pair to parallel arrays while their shared count is below 10. | Caller `0042c480` appends cells with derived type 0x0b; broader queue purpose unknown. |
| `0x0044c710` | `SceneObjectTransformStateInit` | Stores two object handles and a position triple in globals, then applies the position through a helper. | Caller `00433770` configures related scene objects; pseudocode/caller context only. |
| `0x0044c790` | `ConfiguredSceneTransformEnable` | Sets bit 0x100 and applies a configured transform when enabled; clears the bit otherwise. | Caller `004288e0` toggles it on input changes; feature meaning unknown. |
| `0x0046aa90` | `MessageOverlayStateInit` | Initializes overlay geometry/state, sets a slot-like global, clears two globals and a flag, and updates two handles. | Startup/reset caller context; message role is inferred from adjacent slot strings. |
| `0x0047e710` | `DirectDrawSurfaceDescriptorInitialize` | Clears descriptor fields, sets size 0x7c, and zeros surface members. | Direct decompile `ev_d75b809b7beed400983aea831c39d7440674032f2c622d9ed5be41c89bb26690` in `legorr-engine-helper-audit-text-io-expansion-9.bundle`; called by adjacent surface initialization. |
| `0x00485fb0` | `ViewportRectangleGet` | Returns global origin and computes width/height as right/bottom minus origin. | Caller context `ev_9a8a2bad4d7ee0715052c34b672d0def513449748969f6651fc428396a38e` in `legorr-engine-helpers-expansion-9.bundle`; exact rectangle role inferred. |
| `0x00488bb0` | `CdAudioTrackStart` | When audio is available, saves track/callback state and starts playback through the CD-audio helper. | Caller `004297b0`; context `ev_19b3143dc5520915c284c37045e16dfc6e363d5ead2f5d0b131e1cd43b56fc85` in `legorr-engine-helper-audit-text-io-expansion-9.bundle`. |

Complete pseudocode is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c), with named excerpts in [engine-region.c](../decompiles/legacy/engine-region.c). Where no direct record was found, the table says so and the name follows checked-in pseudocode and caller behavior.
