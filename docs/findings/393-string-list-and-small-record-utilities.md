# Finding 393 — String, List, and Small Record Utilities

This finding maps the 46 functions assigned to `shared-primitives/utility-strings-lists`. Evidence is the canonical full decompile corpus (`docs/decompiles/full/LEGORR.full.c`); names and addresses below correspond to its headings. Caller relationships are direct calls visible in that corpus. The roles inferred from raw structure fields are described as such.

## Functional groups

### Allocation, release, and small records

- `HandleArrayReleaseAndReset @ 0x004020c0` walks the 12 dwords from `DAT_004b31c8` up to (excluding) `DAT_004b31f8`, releases each nonzero value with `FUN_0048db80`, then clears `DAT_004b31f8` and `DAT_004b8060`. The range suggests a fixed handle array; its purpose is not established here.
- `EngineMemoryAllocate @ 0x004021e0` and `EngineMemoryFree @ 0x00402210` are one-call wrappers around `FUN_0048dbd0` and `FUN_0048db80`. Direct callers include `SceneObjectChainRecordCreate @ 0x00435b20` and `CameraViewContextClose @ 0x00435dc8`; `GameConfigInit @ 0x0041fb50` also calls the free wrapper.
- `ObjectAttachedRecordAllocate @ 0x004033c0` allocates `0x1c` bytes with `FUN_00406440`, asks `FUN_00404190` for an associated object, and stores the new pointer at that object's `+0x60` when present or at the input object's `+0x2f0` otherwise. `TerrainCellToWorldPosition @ 0x004401b0` calls it.
- `ThreeWordRecordAllocateAndInitialize @ 0x00410b60` allocates 12 bytes and writes three input words, returning without a record on allocation failure.
- `BoundedFloatRecordCreate @ 0x00410b90` rejects `max <= min`, otherwise allocates `0x1c` bytes and stores seven supplied fields (including the lower and upper floats). Its caller is `MenuBuild @ 0x004140c0`.
- `ThreeValueRecordCopyAndMarkReady @ 0x0040fdb0` copies three dwords through the pointer in record field 0 and sets record field 4 to 1. `EntityMotionToSceneNodeSync @ 0x0044bc30` calls it.
- `RecordCountDigitsFill @ 0x0040a5e0` lays out four digit slots, decomposes an unsigned count in base 10, writes digit glyphs, terminates the visible sequence, hides remaining slots, and stores the visible count plus terminator at `record+8`. `QuadMeshRenderRecordCreate @ 0x0040a4b0` calls it.
- `FloatToLong @ 0x00417430` forwards to the CRT `__ftol` conversion helper; `SlotSummary @ 0x004152c0` calls it.

### String tables, substitutions, and string-keyed caches

- `BubbleActionStringsInit @ 0x004070b0` fills the consecutive globals beginning at `DAT_00557900` with pointers to the `Bubble_*` action labels. `GameConfigInit @ 0x0041fb50` calls this initializer.
- `StringTableIndexLookup @ 0x00408c80` scans the pointer table beginning at `DAT_004b8a3c`, comparing strings byte-by-byte in two-byte steps, and returns the matching zero-based index. A miss (or empty table) returns `DAT_004b8a50`, the table count. `EntityStatsConfigLoad @ 0x004667e0` calls it.
- `StrReplace @ 0x00413f00` replaces every occurrence of one byte in a NUL-terminated string in place and returns the original pointer. It is used by `MenuBuild @ 0x004140c0` (six direct calls), `LevelChainBuild @ 0x004154b0`, `SaveMenuBuild @ 0x00415a10`, `GameConfigInit @ 0x0041fb50`, and `TaskBoxRect @ 0x0045f530`; the corpus does not establish one shared replacement convention for all callers.
- `FilenameExtensionClassify @ 0x00413f30` scans from the start of a filename to its first period, then case-insensitively compares that suffix with three constant strings and returns a small integer classification. `MenuItemRecordAlloc @ 0x00413f90` calls it. The scan assumes a period exists; its behavior on a suffix-free name is not bounded by the pseudocode.
- `StringKeyedCacheEntryFind @ 0x00410450` walks a linked chain rooted at `DAT_004db8cc`, comparing entry field 0 case-insensitively; next links are at entry `+0x0c`. It returns a matching entry or null.
- `StringRegistryIntern @ 0x00410490` rejects null and empty keys, reuses a case-insensitive match from `StringKeyedCacheEntryFind`, or allocates an entry and a copied key, links the entry at the registry head, and initializes entry fields 1 and 2 to zero. Allocation results for the two allocations are not checked before dereference/copy in the pseudocode.
- `StringCacheLookupOrLoad @ 0x00410520` interns a nonempty key and lazily fills entry field 1 using `FUN_0047d9d0`; it returns that cached value. Callers include `SaveMenuListEntryAppend @ 0x00410740`, `MenuItemImageSet @ 0x00410fb0`, and `LoadItemStrings @ 0x00414010`.
- `ResourceNameCacheGetOrLoad @ 0x00410560` uses the same interned entry and lazily fills field 2 using `FUN_00479ee0`. `MenuBuild @ 0x004140c0` calls it.
- `CaseInsensitiveStringTableIndexFind @ 0x0041a9c0` scans the fixed pointer range `[DAT_004dced0, DAT_004dcff8)` case-insensitively; on a match it writes the index and returns 1, otherwise returns 0. `InterfaceImagesConfigLoad @ 0x0041ada0` calls it.

### Lists and resource-handle ownership

- `ResourceRecordAndChildrenRelease @ 0x00410f30` releases the three owned pointers in fields 0, 1, and 6, then walks the child count in field 7 and disposes each child through `ResourceVariantRecordRelease`. It releases field 4 through `FUN_0047d470` before freeing the parent record. `MenuSectionRecordCreate @ 0x00411100` calls it. The child count and array at fields 7 and 6 imply a resource-child collection, but the wider record schema needs caller confirmation.
- `ListAppend @ 0x00411260` rejects null list or item. Its list record uses fields `+0x1c` (item count), `+0x20` (capacity), and `+0x18` (item-array pointer). When full, it allocates or resizes the array to `capacity*4 + 0x3c` bytes and increments capacity by 15, then stores the item and increments count. Direct callers include `MenuBuild`, `SaveMenuBuild`, and `SavMenuList`.
- `ListAlloc @ 0x00414070` allocates a two-word list header and a zeroed pointer array for the requested capacity, then stores capacity in header word 1. The decompile does not check either allocation before using the result.
- `ResourceHandleListAppend @ 0x00410aa0` accepts only a nonnull list with a nonnull array and spare capacity; it obtains a handle through `FUN_004105a0`, stores it in the next slot, and increments count only for a nonzero handle. `MenuBuild` calls it.
- `ResourceHandleListDispose @ 0x00410ae0` releases each nonzero handle in the counted array, then releases the array and list record. `ResourceVariantRecordRelease @ 0x00410c70` uses this for variant kind 0; `MenuItemChoiceStringsAllocate @ 0x00410a10` is another caller.
- `InterfaceResourceHandlesRelease @ 0x00413b50` releases each nonzero pointer in the fixed dword range `DAT_004db890` to `DAT_004db8a4`. `MenuRunLoop @ 0x00413b80` calls it.
- `ResourceHandleSetRelease @ 0x0041a850` releases every other dword from `DAT_004ddc04` to before `DAT_004ddcc4`, then releases twelve additional named globals. The exact resource types are not established by this function alone.
- `RectangularResourceTableDestroy @ 0x0042b480` reads row and column counts at record offsets `+0x74` and `+0x78`, releases each table element through `FUN_0040b7d0`, releases the table allocation at `+0x80`, and clears that field. `PendingLevelName @ 0x0042f0c0` calls it.

### Callback and operation state

- `CallbackStateRecordCreate @ 0x0040f7d0` allocates `0x28` bytes, creates an inner state using `FUN_0040f6c0`, initializes fields 1 and 2 to zero, sets field 3 according to whether input 1 is nonzero, and sets field 4 according to whether input 2 is nonzero. `EntitySceneObjectTreeAttach @ 0x0044b680` calls it.
- `CallbackStateListDispose @ 0x0040f850` repeatedly gets a linked state via `FUN_0040fe60`, releases each via `FUN_0040f7b0`, then releases the head state and list allocation. `SceneObjectResourceAndCallbackStateDispose @ 0x0044bae0` calls it.
- `SetVectorAndNotifyListeners @ 0x0040fde0` forwards a vector pointer and a stack-constructed listener/update context to `FUN_0040fb90`. `SceneObjectListUpdateTransforms @ 0x0040fe00` calls it.
- `CallbackOperationRunAndReturnStatus @ 0x00410280` initializes a local status to 1, calls `FUN_0040fe90` with a callback and status pointer, then returns the resulting status. `GlobalTimedCallbackDispatch @ 0x0044bf00` calls it.
- `NoOpCallback @ 0x0041f8e0` returns immediately. The corpus shows it installed or passed by several unrelated paths, including `CatalogSlotFlagUpdate @ 0x0041c800`, `EntityAvailableResourceSpend @ 0x00438720`, `EntityActionKindSupported @ 0x0043a7c0`, and `VelocitySlotSteerDispatchAndSound @ 0x00471d20`; its purpose is a callable no-op in those contexts.

### Name lists, linked records, and operation helpers

- `ActiveObjectList @ 0x00416e00` returns one of two global list records, selected by the boolean result of `FUN_00416d60`. `SaveMenuOpenFromReward @ 0x00462030` and `CheckpointStep @ 0x00462270` call it.
- `ListFindName @ 0x00416e20` scans the first `count` names at the array in list field 1, comparing case-insensitively, and returns the matching index or `-1`. `ListAddName @ 0x00416ed0` scans the same name list and writes the supplied value into the parallel array at field 4 when a name matches. `ListHasItem @ 0x00416f40` returns the corresponding value from field 4 for a match, else zero. `ListSetNode @ 0x00416fb0` writes to the parallel array at field 3; `ListGetNode @ 0x00417020` reads it. All five are used by `LevelNodeGet @ 0x00417090`.
- `IndexedLinkedListEntryGet @ 0x00419780` selects a registry head and count from five-word records at `DAT_004dca18`, returns null for an out-of-range index, and follows `+0x10` links for a valid positive index. `IncidentMessageQueueRemoveAt @ 0x004197b0` calls it.
- `EntityLinkedListEntriesRemoveByKey @ 0x00419830` scans the indexed registries; for each matching entry found through `FUN_004196a0`, it removes at index zero with `FUN_004197b0`. Empty registry records are removed via `RecordListEntryRemoveAndRelease`. `EntityTeardown @ 0x00437540` calls it.
- `RecordListEntryMoveToFront @ 0x00419de0` shifts five-dword registry records toward the tail and moves the selected record to slot zero, then ORs 3 into `DAT_004dcd48`. UI hit-testing and simulation cursor interaction call it (`UIEntryHitTestAndDispatch @ 0x0041a1a0`, `SimCursorInteractionTick @ 0x0045adb0`).
- `RecordListEntryRemoveAndRelease @ 0x00419e80` releases the first linked entry's payload at `+0x0c` when present. If the selected registry's count is zero, it compacts later five-dword records and decrements the registry count; removing index zero clears bit 0 of `DAT_004dcd48`. It is called by the entity-key removal path and by pending-record cleanup functions.
- `OperationGroupAppendObject @ 0x00408ce0` finds a group using `FUN_004090c0`; if none is found, it dispatches operation `0x16` or `0x1c` through `FUN_00452060` based on the object's kind. With a group and nonnull object it sets object flag `0x100`, appends the pointer at group `+0x2c + count*4`, increments two counts, and when the second count reaches the target at `+0x1c`, writes `1.3125f` at `+0x28` and sets bit 0 at `+0xd0`. Callers include `EntityCommandUnlinkByKind @ 0x00403d10` and `BuildingPlacementPreviewUpdate @ 0x0044a910`.
- `CountMatchingCoordinatesInList @ 0x00408e10` walks `count` pointers beginning at list `+0x2c` and counts entries whose first two dwords equal the requested coordinate pair. It is used by `TerrainDescriptorCoordinateCountLimitCheck @ 0x00408d70` and `TerrainCommandFeedbackDispatch @ 0x00428330`.
- `StringQueueRecordOrFlush @ 0x00410320` buffers up to 15 string records, copying text into fixed `0x400`-byte slots and keeping three parallel metadata arrays. A zero fourth argument flushes each queued record via `FUN_0047a180` and resets the count. `MenuTick @ 0x00412c00` calls it.

## Cross-area flows

- **Menu construction:** `MenuBuild @ 0x004140c0` is the densest observed consumer of this area: it uses resource-name/cache helpers, `ListAppend`, `ListAlloc`, `ResourceHandleListAppend`, `ThreeWordRecordAllocateAndInitialize`, `BoundedFloatRecordCreate`, `StrReplace`, and string loading. `SaveMenuBuild @ 0x00415a10` also appends records and rewrites strings. This supports treating the cluster as shared construction/ownership primitives; it does not imply every utility is menu-specific.
- **Entity/scene lifetime:** scene-object attachment creates callback state (`EntitySceneObjectTreeAttach`), transform synchronization copies three values or dispatches listeners, and scene teardown releases callback state and resources. Entity teardown separately removes keyed entries from the indexed registries. These are direct call chains across neighboring systems, but the complete runtime ordering is outside the scope of the utility functions.
- **Save/checkpoint UI:** the active name-list selection helpers feed `LevelNodeGet`; `SaveMenuOpenFromReward` and `CheckpointStep` select an active list and query names. The list helpers expose two parallel value arrays (fields 3 and 4); which semantic type each represents is not identified by the local pseudocode alone.
- **String cache ownership:** both lazy caches share case-insensitive string interning and one linked registry. The cache functions populate separate entry fields using different loaders. No invalidation or registry-wide teardown appears in these assigned functions, so cache lifetime and resource release need a broader caller/global-lifecycle trace.

## Open questions and focused follow-ups

1. Identify allocation and teardown for the `DAT_004db8cc` string registry, including whether cached resource values in entry fields 1/2 are released elsewhere.
2. Trace `FUN_004105a0` and `FUN_00410690` to establish resource-handle acquisition/release and variant kind 6 semantics.
3. Identify the full structure layout and count/capacity invariants for the `ListAlloc` record versus the separately grown `ListAppend` record; the functions appear to serve different list formats.
4. Resolve the extension strings and classification meanings returned by `FilenameExtensionClassify`, and verify the no-period caller precondition.
5. Trace allocation-failure behavior in `StringRegistryIntern` and `ListAlloc`; these decompiles show dereferences after allocation without visible null checks.
6. Determine the lifecycle and semantic types of the fixed handle sets in `HandleArrayReleaseAndReset`, `InterfaceResourceHandlesRelease`, and `ResourceHandleSetRelease`.

## Coverage index impact

All 46 assigned addresses already have `utility-strings-lists` rows in `docs/coverage/function-areas.csv`; this investigation did not change their names or area assignments. No CSV or generated `function-areas.md` update is required. The new finding is indexed in the repository README.
