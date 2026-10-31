# Finding 490 — WAD, Advisor, and Terrain-Selection State

`0x004AAF00` is the WAD-slot wildcard sentinel, initialized to `-1`. `WadEntryReadIntoStreamSlot` passes it to `WadPathOpen`; the helper responds to `-1` by searching mounted WAD archives from slot 9 down to slot 0. File-route and sound-file paths use the same sentinel when opening a WAD entry.

`0x005756A0` begins 100 fixed-stride WAD resource slots of `0x10` bytes each. Each slot stores a resource handle, an active flag at `+4`, a WAD/archive handle at `+8`, and an entry index at `+0xC`. Allocation scans the flags; release and lookup use the indexed handle.

`0x004A6768` stores the selected advisor sound-bank slot. Advisor inline-token processing reads the slot from its configured table, passes it to `StreamedWaveBufferStepRate` and `SoundBufferPlayWithPosition`, and saves it for the global sound helper. The latter plays the slot directly when it is not `-1`; it does not pass the value through alias lookup or variant selection.

`0x004A9090` and `0x004A9094` store the selected terrain-view tile coordinates. `ScreenCoordNormalizeInit` resets them to `-1`; `TerrainViewTileGeometryBuild` highlights the geometry whose X/Y indices match the pair.

`0x00556518` is the `MiniFigureRunAway` configuration distance, defaulting to 10. The flagged-vector movement helper scales its direction vector by this value when calculating the runaway destination.

`0x004B28E0` is a CRT math-exception mode value. The math exception handlers use zero to invoke the CRT math-error callback path and nonzero to set errno/return the input result without invoking that path. The global has no recovered writer, so the exact mode name remains unresolved.

`0x00555118` receives a value associated with the named `Pilot` operation during level startup and is copied into checkpoint statistics at record offset `+0x277C`. The called helper is decompiled as `void`, so the value's source and semantic type are not established.

`0x00555D40` is the first word in a 45-dword tutorial/advisor/camera-focus state block. Tutorial-script load and advisor-state release clear the block; camera-focus update tests this word as its active gate and may clear it when lookup fails. No setter for the gate is present in the decompile.

Complete pseudocode: [LEGORR.full.c](../decompiles/full/LEGORR.full.c).
