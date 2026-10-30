# Finding 379 — Terrain, UI, and CRT Support Helpers

This pass names 27 operation-level routines across terrain state, UI color/cursor updates, shutdown cleanup, and CRT support. Names preserve numeric flags and generic field meanings where the surrounding domain is uncertain.

| Address | Name | Observed behavior | Evidence and caller context |
| --- | --- | --- | --- |
| `0x0040d4f0` | `TerrainCellActivationTry` | Validates a terrain cell or mapped entity cell, then calls the cell activation helper and returns success. | Direct analysis `ev_a9b7901e3501e4472b0ae41ca0a165bfa129dd5156f4656d15ebb2f7ab26f306` in `legorr-engine-helper-audit-terrain-expansion-8.bundle`. |
| `0x0040cea0` | `TerrainCellAndNeighborsUpdate` | Applies a terrain helper to one cell and four offset neighbors. | `ev_fb654e9ce69c90085cda97e49f50a18cc194e4faf49dda19c52176f3299c7377` in `legorr-engine-helpers-expansion-20.bundle`; exact state semantics unclear. |
| `0x0040c750` | `TerrainPointObjectCreate` | Creates and records a scene object at supplied coordinates or coordinates obtained from an entity; limited to four entries. | “Terrain point” role inferred from shared globals with nearby terrain routines. |
| `0x0040c830` | `TerrainPointObjectsRemoveMatching` | Releases recorded objects matching the argument and compacts the list. | Pseudocode only; pairs with `0040c750` by shared storage. |
| `0x0040cc40` | `TerrainMarkerSetsClear` | Calls a clear helper on ten global marker sets. | Pseudocode only; exact marker semantics unresolved. |
| `0x0040d030` | `TerrainRecordPoolChunkAllocate` | Allocates a chunk of 0x14-byte records and links it into a free list. | Record count is based on a global shift value; chunk role is inferred from adjacent routines. |
| `0x0040e700` | `TerrainChoiceResourceSelect` | Selects a resource from a table based on a decoded choice type, updates selection globals, and refreshes a UI object. | Choice type meaning unknown. |
| `0x0041a850` | `ResourceHandleSetRelease` | Releases each nonzero handle in an indexed set and then releases additional global handles. | Owning subsystem is unknown. |
| `0x0041b9c0` | `SelectionEntityCodeResolve` | Returns an existing code in one mode; otherwise selects code 0x22 or 0x23 from an entity flag and writes it to output. | Code meanings unknown. |
| `0x0041cab0` | `IndexedEntityValueGet` | Selects a value from one of three indexed tables using two predicates. | Table semantics unknown. |
| `0x0041cce0` | `SelectionCoordinatesSet` | Stores two coordinates and two additional values in selection/UI globals. | Exact coordinate/value roles unclear. |
| `0x0041e9d0` | `SelectionRegionParametersSet` | Stores four integer parameters as floats and stores a fifth value scaled by 0.04. | Caller identification remains uncertain. |
| `0x0041ec30` | `EntityPairCountRefresh` | Counts records matching two values, converts the count, and updates a global UI quantity. | Displayed quantity meaning unknown. |
| `0x0044cff0` | `UiElementRgbColorSetClamped` | Clamps three global float channels to [0,1] and sends them with alpha 1.0 to the UI element when its type is 5. | Pseudocode in `LEGORR.full.c`; no direct function-specific evidence bundle record reported. |
| `0x0044dd40` | `EngineLoaderProfileEntriesRelease` | Walks the fixed loader-profile table, optionally writes formatted records, frees each entry string, and closes associated handles. | Shutdown caller context in `docs/findings/03-engine-tick.md`. |
| `0x0044e0a0` | `GlobalResourceHandleRelease` | If the global handle is nonzero, releases it through the resource helper and invokes cleanup. | Shutdown caller context in `docs/findings/03-engine-tick.md`; resource type unknown. |
| `0x0045d550` | `UiCursorMoveToChoiceEntry` | Maps choice values to internal items, gets selected-item geometry, computes coordinates, and positions the cursor there. | Pseudocode in `LEGORR.full.c`; no direct function-specific evidence bundle record reported. |
| `0x0045d640` | `UiCursorMoveToElementCenter` | Looks up an element, gets its dimensions, halves width/height, and positions the cursor at the center. | Pseudocode in `LEGORR.full.c`; no direct function-specific evidence bundle record reported. |
| `0x00494d10` | `CrtFileDescriptorClose` | Validates the descriptor, closes its OS handle, clears the descriptor entry, and reports close errors. | Caller `0048eae0`; pseudocode/caller context only. |
| `0x00494df0` | `CrtOwnedStreamBufferRelease` | Frees an owned stream buffer and clears its ownership state and fields. | Caller `0048eae0`; pseudocode/caller context only. |
| `0x00498f00` | `CrtCharacterClassFlag8Check` | Tests character-class bit 0x08 with locale-aware fallback. | Callers `00493ef0`, `00494cc0`; exact class meaning unconfirmed. |
| `0x00497f40` | `___dtold` | VC5 `LIBC.LIB` `cfout.obj` implementation of double-to-extended80 conversion; caller `0x00497ec0` (`__fltout`). |
| `0x0048e870` | `DoubleIsFinite` | Returns false when all exponent bits of the double are set. | Callers `00444680`, `0046d380`, `00479630`; pseudocode only. |
| `0x00475830` | `SceneObjectFirstNonKind6ChildGet` | Walks child links and returns the first resolved child whose kind is not 6. | Pseudocode only; kind meaning unknown. |
| `0x00498b40` | `CrtFileDescriptorFlag40Get` | Bounds-checks a descriptor index and returns flag bit 0x40 from its table entry. | Callers `004908a0`, `004974d0`; flag meaning unconfirmed. |
| `0x00495db0` | `CrtCharacterClassFlagTest` | Tests a caller-supplied mask against the CRT character table and locale fallback. | Wrapper `00495d90` supplies mask 4; pseudocode only. |
| `0x00493150` | `__frnd` | Rounds a double to an integral x87 value using the active rounding mode. | Exact instruction sequence match to VC5 `LIBC.LIB` `frnd.obj`; the MSVC wrapper now calls the runtime entry. |

Complete pseudocode is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c), with named excerpts in [engine-region.c](../decompiles/legacy/engine-region.c). Where no direct record was found, the table says so and the name follows checked-in pseudocode and caller behavior.
