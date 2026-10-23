# Finding 212 — Frame Sound Tick Does Not Drain the Packed Batch Queue

`FUN_004651a0` checks whether packed batching is enabled, then calls
`FUN_004652d0(0, 0)`. This clears the batch flag but does not request the
optional replay pass. The function submits and rotates the separate deferred
alias lists (`DAT_00502bd0` / `DAT_00502bd4`), and, if batching was enabled on
entry, calls `FUN_004652c0(1)` to enable it again. That enable path only sets
the batch flag. `FUN_004651a0` resets the alias-list count, but does not reset
the packed queue count `DAT_00502b20`.

As a result, the frame sound tick does not itself replay or clear requests
queued by `FUN_00465090`. If requests are present when the tick runs, they
remain pending while the separate alias queues advance; after the tick, packed
batching can be enabled again. Finding 209 documents the ten-entry capacity
and the `FUN_004652d0(0, 1)` replay path. This establishes persistence across
this tick routine, not how often a particular caller enters or exits batching
or whether another explicit flush follows.

**Evidence.** Static decompilation of shipped `reference/LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
`FUN_004651a0` at `0x4651a0` reads the packed-mode bit through
`FUN_004652b0`, calls `FUN_004652d0(0, 0)`, later restores the mode through
`FUN_004652c0(1)`, and resets `DAT_00502bd0`; there is no write to
`DAT_00502b20` in that function. At `0x4652d0`, the queue loop is entered only
when the second argument is nonzero, whereas `FUN_004652c0(1)` calls the
non-draining enable path. The relevant functions are in
`docs/decompiles/full/LEGORR.full.c`; findings 139 and 209 document the queue
record layout and explicit drain behavior.
