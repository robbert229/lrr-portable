# Finding 230 — Engine Input, Selection, and Resource Helpers

The low, middle, and high address audits identified seventeen omitted engine
helpers. Direct decompiles were added to the named pseudocode corpus; caller
contexts were used to keep names tied to observed behavior.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x435dc8 | CameraViewContextClose | Releases three owned view resources when the context kind permits, then frees the context. Engine shutdown calls it for the three global view contexts. |
| 0x4102b0 | UiTextUnderscoresToSpaces | Copies strings up to 255 bytes to a shared buffer and converts underscores to spaces. The returned pointer refers to that reusable buffer. |
| 0x41f420 | SelectedEntitiesAnyMatch | Calls a supplied predicate for selected entities and returns true on the first match. |
| 0x4248b0 | SelectedEntityGroundMarkerRender | Builds and submits an eight-point marker around an entity's projected position, scaling it from entity dimensions. Callers use it for selected entities or when an entity flag is set. “Ground marker” describes the geometry and caller context. |
| 0x428780 | TerrainCellCommandPredicate | Applies state-bit gates and checks a terrain-cell byte at offset +4. Its exact command meaning is not established. |
| 0x42b480 | RectangularResourceTableDestroy | Releases entries in a width-by-height pointer table, frees the table, and clears its pointer during teardown. |
| 0x42cd50 | UpgradeNameEntryFind | Case-insensitively searches the upgrade-name table and returns its matching 16-byte record or zero. |
| 0x441940 | EntityActionSequenceReset | Releases an entity's current action-sequence buffer, clears sequence counters and state flags, and invokes the shared entity reset. |
| 0x44a310 | ObjectMovementInputAccumulate | Resolves a type-2 object proxy, resets its active sequence when flagged, gates one movement delta on an entity capability bit, and accumulates three deltas. |
| 0x44f1a0 | InputKeyMappingAddBidirectional | Adds a key pairing to both mapping lists when the pair is absent. The input-context initializer uses it to build keyboard mappings. |
| 0x46a510 | SceneObjectStateDispatch | Maps a selector bit to an ordinal, optionally allocates and links a 32-byte state record, then calls a callback over registered scene objects. The callback's per-object effect remains unresolved. |
| 0x479020 | SortedEntryNextEqualKey | Follows a sorted linked chain and returns the next node whose key matches the starting node's key, or null when the key changes. |
| 0x4798b0 | RandomFloatRange | Scales the engine's signed-short random value and interpolates between the supplied floating-point bounds. |
| 0x47d700 | PopCount32 | Counts set bits in a 32-bit value with a shift-and-count loop. |
| 0x47d720 | FindFirstSetBit32 | Returns the least-significant set-bit index, or -1 when the input is zero. |
| 0x48bce0 | WadArchiveRecordAtIndex | Returns the 36-byte mounted-WAD registry record at `0x575500 + index * 0x24`. Its callers are the WAD index loader, archive-entry lookup/open functions, and adjacent WAD registry accessors; it is not an LWO chunk-table accessor. |
| 0x48fb90 | GameFileReadBuffered | Reads from the engine stream buffer, refilling or reading directly as needed, updates EOF/error flags, and returns the number of bytes or elements read. |

**Evidence.** Direct pseudocode for these targets is in the batch decompile
records ev_259bd804d24a294fbdf1af85667dcaefd555930f5d7ab8e1fb81af25c1867985
and ev_8c1e0225e71a2a8fa82e79f1de8c038d5edcb7eb3c349a5f7ac490d2ec88f2cd,
preserved in
[legorr-engine-helpers-expansion-5.bundle](../evidence/legorr-engine-helpers-expansion-5.bundle).
The low-range caller audit is preserved in
[legorr-engine-helper-audit-low-expansion-5.bundle](../evidence/legorr-engine-helper-audit-low-expansion-5.bundle),
the simulation/input audit in
[legorr-engine-helper-audit-mid-expansion-5.bundle](../evidence/legorr-engine-helper-audit-mid-expansion-5.bundle),
and the resource/rendering audit in
[legorr-engine-helper-audit-high-expansion-5.bundle](../evidence/legorr-engine-helper-audit-high-expansion-5.bundle).
All decompiles use shipped reference/LegoRR.exe, SHA-256
c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f.
