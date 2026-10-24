# Finding 480 — Terrain Activity and Render Audio Contexts

`0x004EABCC` is the base of a 100-slot entity-pointer array used for entities whose current terrain cell has tile type 9. `EntityLavaCrustActivityRegister` appends eligible entities while the extent at `0x004EAD5C` is below 100. Removal and tile-update paths clear matching slots but leave the extent unchanged, so it is a high-water bound for scans rather than a live population count. `SpawnCoordinatePoolClear` clears the entire array and resets the extent.

`0x00549840` is the base of the staged terrain cell-quad run table. Staging and emit helpers advance between records by `0x10A` dwords (`0x428` bytes); each record stores source grid-run data and derived quad geometry state. `0x0054BEB0` is the number of staged records.

`0x00506688` stores the current view/listener context used by DirectSound spatial updates. `DirectSoundListenerViewSet` assigns it while the sound system is enabled, and `DirectSoundListenerUpdateTick` passes it to `DirectSoundListenerSpatialUpdate`.

Complete pseudocode: [LEGORR.full.c](../decompiles/full/LEGORR.full.c).
