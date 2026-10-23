# Finding 209 — Batched Spatial Sound Queue Drops Requests After Ten

When the packed sound-batching flag (`DAT_00502a2c & 8`) is set,
`FUN_00465090` stores a submission only while `DAT_00502b20 < 10`. Each
accepted request occupies a `0x18`-byte record containing the alias, object or
position data, and packed selector/route flags. If the count is already ten,
the function returns zero without storing the request or reporting an error.
The same zero is returned for a successfully queued request, so a caller cannot
distinguish acceptance from overflow through this return value.

`FUN_004652d0(0, 1)` clears the batching flag before replaying the records.
Entries marked as positional/object submissions are resubmitted through
`FUN_00465090`; the other entries go through `FUN_00464fa0`. It then resets the
packed queue count to zero. Consequently, requests beyond the ten stored
records are absent from this replay. This is the packed queue controlled by
`DAT_00502b20`, separate from the ten-entry alias queue controlled by
`DAT_00502bd0` in finding 149.

The behavior is conditional on batching being enabled and does not establish
that shipped gameplay reaches the full queue. Finding 171 documents one
caller-side consequence of a zero return from the queued path: a shared-object
event stores zero as its sound handle, which can later be passed to the
stop-by-handle helper. Finding 212 shows that the frame sound tick temporarily
clears the batching flag without draining or clearing this packed queue.

**Evidence.** Static disassembly and decompilation of shipped
`reference/LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
`FUN_00465090` at `0x465090` compares the count at `0x502b20` with ten and
branches to its zero-return path when the count is ten or greater; accepted
records use a `0x18`-byte stride and increment that count. `FUN_004652d0` at
`0x4652d0` clears the batch flag, replays each record through the two described
paths, then stores zero to the count. Both functions are present in
`docs/decompiles/full/LEGORR.full.c`; the relevant instructions are at
`0x46509b–0x4650a7` and `0x4652e9–0x465363`. Finding 139 records the packed
flag layout and replay argument mapping.
