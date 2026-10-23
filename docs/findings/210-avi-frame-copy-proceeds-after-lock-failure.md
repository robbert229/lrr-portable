# Finding 210 — AVI Frame Copy Proceeds After Surface Lock Failure

`FUN_0047e9e0` conditionally asks the movie object's surface state to acquire a
lock through `FUN_0047e760`. The helper records a local success flag only when
the surface-state flag at `+8` is clear and the lock helper returns nonzero.
That flag controls only the later `FUN_0047e7b0` call. The function proceeds
to calculate row addresses and run its 15-bit or 16-bit copy loop regardless
of whether the lock helper succeeded or was called at all.

Thus, for a supported bit depth, a failed lock does not skip the frame copy;
the copy still writes through the destination pointer loaded from the surface
state record at `+0x40`. The static evidence establishes this control flow but
does not establish whether that pointer is valid or otherwise safe to use after
a failed lock, or whether the surface was already locked by another path.
Finding 123 documents the row traversal and the conditional lock/unlock calls.

**Evidence.** Static disassembly of shipped `reference/LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
`FUN_0047e9e0` at `0x47e9e0` initializes a local flag to zero (`0x47e9e9`),
sets it only after the `FUN_0047e760` result tests nonzero (`0x47e9f8–0x47ea0a`),
then loads the destination pointer and enters the pixel-copy branches without
testing that flag (`0x47ea0f–0x47eb02`). The flag is tested only before the
`FUN_0047e7b0` call (`0x47eb04–0x47eb15`). The decompilation is in
`docs/decompiles/full/LEGORR.full.c`; finding 123 cites the frame-row-copy
evidence bundle.
