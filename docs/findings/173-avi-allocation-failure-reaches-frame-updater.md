# Finding 173 — AVI Allocation Failure Reaches a Null Frame Updater

`FUN_0047ec90` requests a `0xc4`-byte movie object from `FUN_0048ead0`. If the allocator returns null, the wrapper returns zero without calling the constructor. In the subtitle/video command path, `FUN_0040a0c0` stores that result in `iVar2` but does not check it before entering its playback loop. The first loop iteration calls `FUN_0047ecf0(iVar2)`, which forwards to `FUN_0047eb20`; that method reads the initialized byte at `this + 0x99` before any null check. A null allocation therefore reaches a null-based state read.

This is a conditional allocation-failure path. The evidence does not show whether `FUN_0048ead0` can return null under a particular runtime memory condition or whether any shipped AVI playback reaches it.

**Evidence.** Shipped `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. Allocation and zero return are in `FUN_0047ec90` at `0x47ec90`; the unguarded result and first updater call are in the movie command loop `FUN_0040a0c0` at `0x40a0c0`; forwarding and the `+0x99` read are in `FUN_0047ecf0`/`FUN_0047eb20` at `0x47ecf0`/`0x47eb20`. The decompilation is in `docs/decompiles/full/LEGORR.full.c`; staged AVI allocation/setup behavior is also covered by finding 69.
