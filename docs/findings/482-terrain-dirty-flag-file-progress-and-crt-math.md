# Finding 482 — Terrain Dirty Flag, File Progress, and CRT Math

`0x00552190` is the terrain surface dirty flag. Terrain surface allocation and cell-quad updates set it; `TerrainDirtyCellQuadNormalEmitPass` clears it and rebuilds normals for cell entries whose dirty bit is set.

`0x00555E00` is the image handle for the file-loading progress overlay. `KeyValueEqualsFileLoad` loads the image and `FileLoadingProgressUpdate` draws it while updating progress.

`0x00555E08` begins a 50-entry table of three-dword file-loading key/value records. The loader reads names and progress values into these entries; release code walks the records to serialize configured values and frees their names. `StringTableEntrySetCurrent` stores a pointer to the matching entry at `0x00556060` and resets its progress field.

`0x005490C8` gates CRT floating-point exception processing. The arccos core and floating-point result-dispatch routines call or continue their exception handlers when this value is zero, and bypass those handlers when it is nonzero. The semantic name and writer for this mode flag are not recovered.

Complete pseudocode: [LEGORR.full.c](../decompiles/full/LEGORR.full.c).
