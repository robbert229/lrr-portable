# Finding 213 — Disabling Sound Does Not Clear Packed Requests

`FUN_00465260(0, 1)` disables sound by clearing bit 0 of
`DAT_00502a2c`, then calls `FUN_0047b160` to stop tracked sound buffers and
streams. It does not clear the packed batching bit (bit 3) or the pending
request count `DAT_00502b20`. `FUN_00429810(0)` reaches this path through
`FUN_004652a0(0)`.

The packed requests therefore remain pending after the sound-effects toggle
disables playback. When `FUN_004652d0(0, 1)` later drains them, its replay calls
`FUN_00465090` with batching disabled; that function submits immediately only
when bit 0 is set. If sound is still disabled, replayed requests are skipped
and the queue count is then reset. If sound is re-enabled before the drain,
those queued requests can instead be submitted. This is conditional behavior:
the static call flow does not establish that a nonempty packed queue coincides
with a user toggle in shipped play.

**Evidence.** Static decompilation of shipped `reference/LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
`FUN_00465260` at `0x465260` clears only bit 0 and optionally calls
`FUN_0047b160`; `FUN_0047b160` stops tracked buffers and both stream states.
`FUN_00465090` at `0x465090` checks the packed-batch bit before checking the
sound-enabled bit on its immediate path. `FUN_004652d0` at `0x4652d0` clears
batching, replays the queue, then resets `DAT_00502b20`. `FUN_00429810` at
`0x429810` calls `FUN_004652a0`, which wraps `FUN_00465260` with the stop flag
set. The functions are in `docs/decompiles/full/LEGORR.full.c`; findings 209
and 212 document packed-queue capacity and persistence across frame ticks.
