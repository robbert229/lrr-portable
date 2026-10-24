# Finding 487 — Loader Callback Context and Terrain Record Base

`0x005343B0` stores the opaque context paired with the loader-progress callback at `0x005343AC`. `LoaderProgressCallbackSet` assigns both values; whole-file and map readers invoke the callback with the path, file size, and this context.

`0x00549520` is the base of the staged terrain cell-quad record array. `StartMapRowSpanBufferRecord` writes each `0x428`-byte record here and increments the record count at `0x0054BEB0`; the staging path sorts and later processes the records at the same stride. The field at `0x00549840` is offset `0x320` within the first record.

Complete pseudocode: [LEGORR.full.c](../decompiles/full/LEGORR.full.c).
