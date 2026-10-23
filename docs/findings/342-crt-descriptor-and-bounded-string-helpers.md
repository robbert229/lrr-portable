# Finding 342 — CRT Descriptor and Bounded-String Helpers

Three helpers allocate CRT file-descriptor slots, associate an OS handle with a slot, and measure a bounded wide string.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x498fe0 | CrtFileDescriptorAllocate | Scans descriptor blocks, allocates and initializes a block when necessary, extends the descriptor limit, and returns a free slot. |
| 0x4990a0 | CrtFileDescriptorSetHandle | Stores an OS handle and initializes descriptor metadata for the selected slot. |
| 0x49add0 | BoundedWideStringLengthIncludingNull | Scans at most the supplied number of wide characters; returns the terminator index plus one when found, otherwise the bound. |

Bodies and callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). The descriptor helpers are called by the file-open path; retained evidence `legorr-crt-fifth-pass-2026-10-23.bundle`, record `ev_8a8026347352df84d7af2c3fa25871ffa3b0ce6ce9eb3dc3db356ae7ca3bac0b`, analyzes that caller rather than either helper directly. No matching direct retained record was found for the bounded string helper. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
