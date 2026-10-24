# Finding 489 — UI Config and Tooltip State Tables

`0x004DB890` begins five loaded SaveImage resource handles. `SaveMenuThumbnailConfigLoad` forms the per-slot names, loads their resources, and `MenuSaveImageConfigRectBlit` uses a non-null entry to draw the corresponding save image. Interface-resource teardown releases the handles.

`0x004DCEC4` and `0x004DCEC8` store the configured width and height of `InterfaceBackButton`. The advisor overlay hit-test combines those dimensions with the configured position, and the animation positioning path uses the same dimensions.

`0x004DCFF8` begins 74 indexed interface-image resource handles. `InterfaceImagesConfigLoad` assigns one handle per case-insensitive interface name; `IndexedEntityValueGet` selects among this table and two parallel tables according to the entity's configured class/flags.

`0x004DEE18` begins a mapping table indexed by action/activity codes and used to select an entry from an entity's action resource. Initialization values and consumers are visible, but the domain meaning of every mapped number is not resolved; this entry remains partial.

`0x004FF978` stores the engine/tutorial mode value copied by `EngineModeSet` and returned by `GlobalFlagGet_004ff978`. Tutorial script loading sets it to 1 before starting the script runtime. The flag also controls event-timer behavior; its full set of modes is unresolved.

`0x0050118C` is the Y coordinate of the first configured priority-image position. The matching X coordinate is at `0x00501188`; `PrioritiesImagePositionsConfigLoad` fills 27 XY pairs at eight-byte strides.

`0x00502C28` begins the 15-by-15 entity-stat level-limit matrix. `EntityStatsConfigLoad` stores each `Stats.<entity>.Levels` value at `[entity category * 15 + stat index]`; `EntityBuildingIndexSet` uses the corresponding value as the upper bound for an entity's selected stat/building index.

`0x0054BF20` stores the tooltip text-rendering context supplied to `TooltipOverlayStateInitialize`; UI text loading and tooltip drawing use it. `0x0054BF3C` stores the timed-tooltip duration threshold in 25-unit ticks, and reset/update helpers compare each tooltip timer against it.

`0x0054C000` begins the per-tooltip runtime record table. The initializer clears `0x1839` dwords; records use a `0x9F`-dword (`0x27C`-byte) stride, and `UiTextEntryLoad` writes per-entry formatted text and render state into the indexed record.

Complete pseudocode: [LEGORR.full.c](../decompiles/full/LEGORR.full.c).
