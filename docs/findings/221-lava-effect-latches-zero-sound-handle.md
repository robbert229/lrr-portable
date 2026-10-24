# Finding 221 — Lava Effect Latches a Zero Sound Handle

The transient-effect instance updater makes one alias-submission attempt when its sound
handle field is zero and its attempt flag is clear. It stores the return value
in the handle field; if that value is zero, it sets the attempt flag and does
not try again on later updates. Under packed batching, an accepted positional
request also returns zero because it has only been queued (finding 209). The
later drain may start playback, but this instance retains no handle for it, and
instance cleanup calls the stop helper only when its stored handle is nonzero.

The same zero result also latches the attempt flag when sound is disabled or
the packed queue is full, so this object does not retry after a later sound
enable or after queue capacity becomes available. These effects are
conditional on the updater being called in those states; static analysis does
not establish that a particular lava emitter encounters them in shipped play.

**Evidence.** In shipped `reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
`FUN_00465b00` (`0x465b00`) calls `FUN_00465050` for a nonzero alias when
handle field `+0x2a4` is zero and flag `0x20` is clear. The call's EAX result
is stored at `+0x2a4`; if it is zero, the updater ORs `0x20` into the instance
flags (`0x465b53–0x465b73`). `FUN_00465050` forwards to `FUN_00465090`, whose
batch branch stores an accepted request and returns zero (finding 209), while
its immediate branch returns zero when sound is disabled or the alias cannot
be resolved. `FUN_004659b0` (`0x4659b0`) invokes `FUN_00465190` to stop playback
only when `+0x2a4` is nonzero. The known constructor caller for this path
creates the `SFX_Lava` instance (finding 137); finding 171 documents a separate
shared-object caller with the same queue-return convention.
