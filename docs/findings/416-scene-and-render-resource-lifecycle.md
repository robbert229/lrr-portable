# Finding 416 — Scene and Render Resource Lifecycle

This finding reviews the 68 addresses listed by Issue #191 and maps the
resource, scene, animation, and texture routes that connect them. Sixty-one
remain in the `scene-resource-lifecycle` area. Seven entries had been assigned
to that area incorrectly: two are DirectSound bank helpers, four manage the
map-file slot table, and one returns config-list records to the BlockTextToken
pool. Their canonical names or area assignments and the generated indexes are
corrected here.

**Target and evidence.** Static analysis of shipped `reference/LegoRR.exe`,
SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
The canonical function bodies and direct call sites are in
[`LEGORR.full.c`](../decompiles/full/LEGORR.full.c); prior focused findings
provide retained call-context evidence. Bodies and direct call edges below are
pseudocode observations. Subsystem interpretations follow those bodies,
callers, shared tables, and interface operations. Vtable slot meanings and
several record-field roles remain explicitly unresolved.

## Route overview

The resource-name route packages a name and mode into a callback record, then
walks an interface-backed object tree. `ResourceTreeCallbackWalk` first tests
the current object with `ResourceNameMatchCallback`; if it does not match, it
enumerates children and recurses. The callback reads the object's name through
an interface method and can return a matching handle, count, or ordinal result.
The route is used by visual and scene registration callers, including
`SceneChannelSlotsRebuild` and `ResourceSlotRegisterName` (finding 255).
`XfTextureNameFormat` formats the associated registration name.

Texture creation and reuse are separate but connected paths. `CreateBmpSurface`
reads a bitmap and asks the DirectDraw-backed device wrapper to create and fill a
surface. `TextureSurfaceCreate` checks the configured surface database before
using that create path. `TextureGetOrCreate` performs case-insensitive lookup
in the primary cache, attempts creation on a miss, retries through the shared
texture cache, and stores successful results. The animated-texture helpers
resolve and set individual frames; `AnimatedTextureFrameSurfaceSet` applies a
surface to a cached render-resource instance. Findings [13](13-texture-system.md)
and [50](50-tile-texture-refresh-order.md) contain the detailed loader, cache,
and refresh evidence.

Scene loading crosses a generic resource-tree interface boundary. The
`LwscSceneLoad` wrapper builds callback contexts and routes to `LoadLWSCScene`,
which parses the LWS scene records. Context builders count or select matching
resource-tree entries; scene frame records are cloned or created from those
results. `ResourceTreeCallbackReferenceChainRelease` and
`OwnedResourceTreeRelease` unwind separate reference/ownership structures.
The format and DDiScene runtime are detailed in findings
[11](11-model-and-scene-formats.md) and [25](25-ddscene-object-runtime.md).

Render-resource tracking uses a global list plus a fixed slot table.
`RenderResourceTrackAndRetain` links a wrapper and calls its wrapped object's
reference method; `RenderResourceTrackedReferencesRelease` unwinds those
references before `ResourceSlotTableReleaseAll` releases the remaining slots.
The `DAT_0076ad60` head links 0x20-byte pooled render-resource records through
their `+0x1c` fields: pool growth chains new records into it,
`ScreenBoundedSurfaceResourceAllocate` removes the head and clears its link,
and `RenderResourceTrackAndRetain` links a wrapper at the head.
Several interface helpers forward through fixed vtable offsets; their names
intentionally preserve the slot number because the semantic contract is not
known. Cached render-resource instances use a separate 0x34-byte pool (finding
387).

The shared-object route stores a directory root and callback triplet, resolves
or clones cached objects, then updates animation tracks and timed sound events
as the frame time advances. Keyframe interpolation and sound-event timing are
separate branches. Existing findings [169](169-shared-object-sound-events-use-time-windows.md),
[171](171-queued-shared-sound-records-zero-handle.md), and
[246](246-entity-move-route-command-and-object-clone-helpers.md) detail the
sound callback and clone state.

## Assigned function map

### Texture names, surfaces, and animated frames

| Address | Function | Observed behavior and route |
| --- | --- | --- |
| `0x00473bc0` | `ResourceNameRegisterOrQuery` | Builds a name/length/mode record and dispatches `ResourceTreeCallbackWalk` with `ResourceNameMatchCallback`; callers register or resolve named scene resources. The matching modes return a resource handle, copy a resolved value, or select an ordinal (finding 254). |
| `0x00473c60` | `XfTextureNameFormat` | Formats an `Xf`-prefixed registration name for a scene/resource slot; `SceneChannelSlotsRebuild` passes the result to `ResourceNameRegisterOrQuery` (finding 241). |
| `0x00474050` | `CreateBmpSurface` | Reads bitmap bytes through the game-file layer, parses the indexed-surface suffix, creates a DirectDraw surface/texture, and uploads the image (finding 13). |
| `0x00474410` | `ParseIndexedSurfaceName` | Parses the indexed suffix used by the bitmap surface path; called by `CreateBmpSurface` (finding 13). |
| `0x004744f0` | `TextureSurfaceCreate` | Checks the configured surface database through interface slots `+0x5c/+0x60` before taking the bitmap create path; used by the tile texture manager and refresh route (findings 13, 50). |
| `0x00474710` | `TextureRelease` | Releases the two surface references held by a texture record, at offsets `0` and `+4`, until each release reports zero (finding 50). |
| `0x00474a20` | `AnimatedTextureFrameCount` | Returns the number of frame slots in an animated texture record; the world-tile refresh loop uses it to bound frame replacement (finding 50). |
| `0x00474a60` | `SceneSurfaceTextureBindByMode` | Dispatches a scene-surface binding operation by mode; `SceneQuadGridEmit` calls it while posting generated quad faces. The target interface slot's meaning is unresolved. |
| `0x00474c00` | `AnimatedTextureFrameResolve` | Resolves the surface for a requested animated-texture slot; `AnimatedTextureFrameSet` uses the result on its non-nested update path (finding 50). |
| `0x00474da0` | `AnimatedTextureFrameSet` | Updates one frame slot: with a nested cached resource it calls `AnimatedTextureFrameSurfaceSet`; otherwise it resolves the surface and invokes a wrapped-object method at `+0x54` (findings 13, 50). |

### Resource-tree lookup and render-resource ownership

| Address | Function | Observed behavior and route |
| --- | --- | --- |
| `0x00476330` | `ResourceTreeCallbackWalk` | Calls the supplied predicate on the current node, then obtains the child enumerator through interface slot `+0x4c` and recursively visits child interfaces until a match or exhaustion (finding 255). |
| `0x00476410` | `ResourceNameMatchCallback` | Reads an object's name through interface slot `+0x24` and applies the requested exact/case-insensitive or ordinal match; this is the callback passed by `ResourceNameRegisterOrQuery` (finding 255). |
| `0x004765c0` | `LwscSceneLoad` | Builds a scene-load request and callback contexts for enumerator/resource modes, then routes scene records to the LWS parser; called by the DDiScene object-kind loader (findings 25, 380). |
| `0x00476d10` | `RenderResourcePairReleaseAndClear` | Queries a value from a wrapped interface at `+0x1c`, releases that value and the paired stored resource, then clears both fields (finding 349). |
| `0x00476d50` | `ResourceSlotTableInitialize` | Clears the 32-dword scene-resource slot table and initializes its count/state fields. |
| `0x00476d80` | `ResourceSlotTableReleaseAll` | Releases tracked references, releases each non-null slot value, and resets the table state. |
| `0x00476dc0` | `RenderTargetViewportScaleSet` | Queries device dimensions, converts the supplied viewport values, and delegates to the bounded-surface allocation path; its precise parameter interpretation remains tentative (finding 375). |
| `0x00476e50` | `ScreenBoundedSurfaceResourceAllocate` | Normalizes negative coordinates against device dimensions, rejects rectangles outside the surface bounds, obtains a pooled record, and asks the resource interface at `+0x50` to create the bounded surface. |
| `0x00476f10` | `RenderSurfaceGetValuePair80And84` | Forwards wrapped-interface slots `+0x80` and `+0x84` into optional output pointers; returned values are not semantically identified (finding 349). |
| `0x00476f50` | `ResourceInterfaceCallSlot48` | Forwards a value through wrapped-interface slot `+0x48` (finding 361). |
| `0x00476fd0` | `ResourceInterfaceCallSlot8C` | Calls wrapped-interface slot `+0x8c`; slot purpose is unresolved (finding 361). |
| `0x00476ff0` | `ResourceInterfaceCallSlot90` | Calls wrapped-interface slot `+0x90`; slot purpose is unresolved (finding 361). |
| `0x00477220` | `RenderResourceTrackAndRetain` | Links a wrapper into the global render-resource tracking list and calls the wrapped object's slot `+8` (finding 349). |
| `0x00477250` | `ResourceInterfaceCallSlot40AndClearField14` | Calls wrapped-interface slot `+0x40` with the supplied value, then clears wrapper field `+0x14` (finding 361). |
| `0x00477270` | `ResourceInterfaceCallSlot88` | Calls wrapped-interface slot `+0x88`; slot purpose is unresolved (finding 361). |
| `0x004773e0` | `RenderResourceTrackedReferencesRelease` | Walks allocated 0x20-byte tracking records and invokes slot `+8` for each wrapped object; table release later frees the backing blocks (finding 349). |

### Adjacent list, sound, and map-slot corrections

| Address | Function | Observed behavior and corrected area |
| --- | --- | --- |
| `0x00479240` | `ReleaseLinkedObjectList` | Releases the root payload and walks linked records through their `+0x14`/`+0x1c` links, returning each node through `GlobalRecordListPushHead` to the `BlockTextToken` pool. Its callers are config and object-list parsers, so it is classified under config-text parsing rather than scene resources. |
| `0x0047ab00` | `SoundBankEntryRelease` | Under the DirectSound initialization guard, clears a sound-bank slot's active bit, frees its streamed-sample memory, releases three buffer interfaces, and clears them. `AdvisorDisplayStateResourcesRelease` calls it for stored sound-slot indices; reclassified to audio playback/mixing. |
| `0x0047b050` | `SoundBankEntryVolumeSet` | Uses `SoundVolumeValueValid` to accept DirectSound volume values (invalid input becomes zero), stores the value, and forwards it to one or three sound buffers depending on the slot flags; reclassified to audio playback/mixing. |
| `0x0047b2a0` | `KeyedResourceEntryRemove` | Uses the recursive matcher to unlink and free the matching head entry from a global keyed list. |
| `0x0047b2e0` | `NestedRecordListRemoveByKeyRecursive` | Recurses through a child link at `+0x0c`; unlinks/frees the child on a match and reports whether the current node's key at `+8` matches (finding 368). |
| `0x0047b330` | `NestedRecordListRemoveMatchingKey` | Recurses through a nested link at `+0x10`, then removes the matching keyed child list and frees its node (finding 358). |
| `0x0047dbc0` | `ResourceRecordAddRef` | Calls a wrapped object's slot `+0x74` with operation value `8`, then sets bit `2` in the record flags. |
| `0x0047e480` | `TwoInterfaceFieldsReleaseAndReset` | Switches the object's vtable, releases non-null interface fields at `+0x18` and `+0x10`, and clears both (findings 368, 384). |
| `0x004893f0` | `MapFileResourceSlotsReset` | Clears the 2,000-entry pointer/active-bit table used by the map-file slot helpers; renamed and reclassified to level-map loading (findings 02, 07). |
| `0x00489410` | `MapFileResourceSlotAllocate` | Allocates the requested payload, stores it in the first free map-file slot, and marks the slot active; reclassified to level-map loading (finding 07). |
| `0x00489460` | `MapFileResourceSlotRelease` | For an in-range active map-file slot, releases its payload and clears its pointer/state; reclassified to level-map loading (finding 07). |
| `0x004894a0` | `MapFileResourceSlotGet` | Returns the payload for an in-range active map-file slot or zero; reclassified to level-map loading (finding 07). |

### Shared textures, cached instances, and animation

| Address | Function | Observed behavior and route |
| --- | --- | --- |
| `0x004805b0` | `SharedTextureRootRegister` | Replaces the shared texture root, normalizes its trailing separator, clears the render-resource instance pool state, and invokes the root initializer (finding 13). |
| `0x00480650` | `SharedTextureRootInitialize` | Initializes the shared texture root through interface calls; successful setup invokes `FUN_00480780` and then releases the prior interface. The interface-slot meanings remain unresolved (finding 13). |
| `0x00480900` | `CachedRenderResourceInstanceCreate` | Acquires a 0x34-byte pool record, copies the template resource entries, creates per-entry render resources, retains the wrapped interface, and increments the source instance count (findings 13, 246, 387). |
| `0x00482040` | `AnimationTrackApplyWrappedFrameOffset` | When the render-resource record's animation flag is set, computes each flagged frame index modulo its frame count and applies the corresponding surface through `AnimatedTextureFrameSurfaceSet`. |
| `0x004820d0` | `FUN_004820d0` | Releases clone/template references and cached surfaces, decrements the source instance count, and returns the record to the render-resource pool (finding 387). The routine remains unnamed because its full contract is not established. |
| `0x00482e70` | `TextureGetOrCreate` | Builds a texture name, checks the primary cache, loads on a miss, retries the shared-texture cache, then caches a successful result (finding 13). |
| `0x00483030` | `TextureCacheLookup` | Performs a case-insensitive lookup in a 12-byte-entry texture table and AddRefs the returned record on a hit (finding 13). |
| `0x00483080` | `TextureCacheInsert` | Stores an owned copy of the texture name, resource pointer, and size in the selected cache table (finding 13). |
| `0x004830c0` | `AnimatedTextureFrameSurfaceSet` | Updates the selected 0x70-byte render-resource entry with the supplied frame surface; called by the animation frame update paths. |

### Scene files, shared objects, and resource ownership

| Address | Function | Observed behavior and route |
| --- | --- | --- |
| `0x004869f0` | `LoadLWSCScene` | Parses an LWS scene stream into scene/resource records and expands each scene entry through `SceneEntrySlotTableExpand` (finding 11). |
| `0x004876c0` | `SharedObjectDirectoryScan` | Stores the shared-object directory root plus name-resolution, sound-submission, and sound-enabled callbacks; `GameConfigInit` supplies the `SharedObjects` path (findings 13, 169). |
| `0x00487760` | `ResourceStringTablesFree` | Releases the counted string arrays used by shared-object/resource entries. |
| `0x004877d0` | `ResourceEntryIdGet` | Reads the 16-bit entry ID at offset `+2`; shared-object time-window logic uses it as the record's period (findings 169, 381). |
| `0x004877e0` | `SceneEntrySlotTableExpand` | Builds expanded per-entry data for flagged scene records, splits names into tokens, calls the registered entry callback, and stores token/index values in the expanded table. |
| `0x00487a00` | `SharedObjectClone` | Copies a shared-object record, duplicates child/interface state, creates cached render-resource instances for eligible entries, links the clone to its source, and updates it (finding 246). |
| `0x00487ba0` | `SharedObjectAnimationAndTimedEventUpdate` | Advances/clamps or wraps shared-object time, samples track keyframes, applies animation tracks, then processes per-entry callbacks and timed sound events (findings 169, 246). |
| `0x00487cb0` | `AnimationTrackFindKeyframeAndFraction` | Binary-searches a time-key track and returns the interpolation fraction while outputting the key index. |
| `0x00487de0` | `SharedObjectTimedSoundEventUpdate` | Checks configured sound-event times against the object's current interval, submits new aliases, stores returned handles, and stops prior handles on changed events (findings 169–171). |
| `0x00487ed0` | `SharedObjectSampleTimeWindowMatch` | Tests whether an event time lies in the current interval, including the wrap-around branch when the interval spans at least half the record period (finding 169). |
| `0x00487fc0` | `KeyframeTimeSegmentAndFractionGet` | Finds the first time-key at or above the sample and returns the preceding segment index and normalized fraction; out-of-range input selects the last key with zero fraction (finding 374). |
| `0x00488170` | `AnimationTrackKeysInterpolate` | Interpolates the selected transform/key values and applies them to the scene object through the registered interface methods (findings 248, 252). |
| `0x004885c0` | `ModelResourceResolveOrLoad` | Builds the model cache key, reuses/AddRefs a cached model entry on hit, otherwise calls `LwoModelEntryLoad` and inserts a successful result; retries through the shared model root when present (findings 11, 13). |
| `0x004889d0` | `ResourceTreeCallbackReferenceChainRelease` | Walks the callback-reference chain, decrements references, releases entry interfaces and backing arrays at zero, then frees the chain (finding 380). |
| `0x00489910` | `ResourceTreeCallbackCount` | Applies a callback to an object tree and, when the enumerator interface has no container, recursively counts child entries (finding 255). |
| `0x0048c180` | `OwnedResourceTreeRelease` | Releases the two owned payload pointers, recursively releases child records, and optionally frees the current record (finding 374). |
| `0x0048d7c0` | `ModelObjectCreate` | Parses LWO point, curve, polygon, and surface chunks into a model object and resolves its materials; the format evidence is in findings 11 and 19. |
| `0x0048d870` | `ModelObjectFree` | Releases the model object's owned resource tree and parser allocations after model-entry creation (findings 11, 19). |

### Resource-tree contexts and clone records

| Address | Function | Observed behavior and route |
| --- | --- | --- |
| `0x00489520` | `ResourceTreeCallbackContextCreateForEnumerator` | Allocates a context, AddRefs its enumerator, counts callback matches, and allocates an index array for the matching entries (finding 380). |
| `0x004895c0` | `ResourceTreeCallbackContextCreateForResource` | Creates the alternate resource-mode context and index array from the same enumerator; mode/index semantics remain unresolved (finding 380). |
| `0x00489660` | `SceneActFrameRecordCloneOrCreate` | Creates a new scene-frame record and resource-tree clone when no prior record exists, or clones a shared-object record when one is available (findings 246, 275). |

The port has no production Direct3DRM object-array/enumerator interface for
these context constructors. Their former ledger wrappers treated the second
argument as a `PortableResourceTreeNode`, which does not match the observed
COM AddRef and array traversal contract; those mappings were removed until a
real scene-resource owner can provide that interface.

## Index corrections and limits

The Issue #191 addresses were all present in the area CSV, but the area assignment
was not reliable for every helper. The README findings index now includes this
finding. The generated coverage ledger and area report retain the texture,
scene, and render-resource members, move the sound-bank pair to
`audio-playback-mixing`, move the map-file table and its reset to
`level-map-loading`, and move `ReleaseLinkedObjectList` to config-text parsing.
The map-file reset is now named `MapFileResourceSlotsReset` in the canonical
pseudocode and Ghidra mapping. These changes keep the area reports aligned with
the actual callers and the table each helper operates on.

Directly observed vtable offsets do not identify their implementing interface
method. The GUID/interface at `DAT_0049fb48`, the semantic meaning of the
render-resource slots, and some scene-entry fields remain unknown. Focused
follow-ups are [issue 253](https://github.com/robbert229/lrr-portable/issues/253)
to identify that interface through its query/release callers and
[issue 254](https://github.com/robbert229/lrr-portable/issues/254) to trace the renderer
slots through implementations before assigning higher-level meanings. Map-file
slot ownership and its 2,000-entry bound are covered separately in finding 07;
no new map-format behavior is inferred here.
