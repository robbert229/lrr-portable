# 63 — Sound submissions select a buffer instance and keep streamed playback separate

The direct sound path is gated by the sound subsystem, the loaded bank slot,
and bank flags. An in-memory entry whose duplicate-buffer flag is set rotates
through three buffer pointers; an entry without that flag uses one. Finding 134
traces this flag to the leading `*` on a `Samples` path. The helper returns an
integer formed from the bank slot and selected buffer index. Stream-mode entries
take a separate file-backed path. These are submission and buffer-management
facts; they do not establish that a request becomes audible.

The stream path replaces any active stream in its selected stream-state record
before loading the next one; finding 136 traces the stop, release, and restart
sequence. The selector's user-facing meaning remains unknown.

**Observations from** REA/Ghidra 12.1.4 over `reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence: `docs/evidence/legorr-sound-instance-spatial-expansion.bundle`.
Focused IDs:

- `ev_98a95a387f00ede6b484669f9341f5e0b38636f97b03e08fe8bccb6df4bc1b1f`
  (`FUN_0047a870`, bank-entry setup and flag selection),
- `ev_02979193ddb48091c08741d27b02a3ab17faf60623138fd8969a49fc55ae655e`
  (`FUN_0047ad70`, direct buffer selection, dispatch and return value),
- `ev_7da0a488bdbf46abfc69ed8002c9fc64f46e9742a0fc060034aedd5bca77cae1`
  (`FUN_00465090`, direct versus deferred positional submission),
- `ev_8ed5142f30f90fce1c662e14e72a4da2682def15105ccb314193b237a8323fa0`
  (`FUN_0047b020`, sound-buffer association record),
- `ev_e39098c0e346a8bd7e528969940123c46de845facd33b2c3da7b15990538d6af`
  (`FUN_0047b1a0`, per-buffer tracking registration),
- `ev_711061f64d4ff328bc93f4c5a0dc5a2c3869c9223308872f6c5cc62174fdc70c`
  (`FUN_0047b220`, stop and tracking cleanup), and
- `ev_be8e1e65cbfba0701c22725feb605a1782556c1323859d2ca1f54a393cf230fa`
  (`FUN_00464fa0`, stop submission through the bank lookup).

## Buffer selection and lifetime bookkeeping

Finding 168 traces the in-memory branch's unchecked `SetCurrentPosition` and
`Play` results; the returned integer does not report either call's success.

`FUN_0047ad70` returns `-1` unless sound is enabled, the requested bank slot
is not `-1`, and the slot's active bit is set. When the slot is in memory mode,
flag `0x4` advances a per-slot index modulo three and selects one of three
buffer pointers. Without that flag, it selects the single buffer pointer. The
selected buffer is queried and configured before a DirectSound buffer call at
vtable slot `+0x30`; the return is `slot * 3 + selected_index`. The slot setup
in `FUN_0047a870` sets flag `0x4` when the in-memory entry's duplicate-buffer
argument is nonzero; finding 134 traces that argument from the config marker.

The same dispatch function treats flag `0x8` as the streamed-entry path: it
opens the file-backed entry and delegates to the streaming helper, returning
`slot * 3` on that path. Thus the integer return is a bank/instance-derived
identifier, not evidence that playback completed. The positional helper
`FUN_00465090` either passes a resolved sound slot to `FUN_0047ad70` directly
when its mode flags allow it, or appends a request to the ten-entry deferred
queue. When positional bookkeeping is created, `FUN_0047b1a0` adds a tracking
record; the stop path in `FUN_0047b220` releases the buffer and removes
associated records.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| In-memory entries with the duplicate-buffer flag rotate over three buffer pointers | observed (`FUN_0047a870`, `FUN_0047ad70`; config source traced in finding 134) |
| The direct helper returns `slot * 3 + buffer_index`; invalid or disabled requests return `-1` | observed (`FUN_0047ad70`) |
| Stream-mode entries use a separate file-backed dispatch | observed (`FUN_0047a870`, `FUN_0047ad70`) |
| Positional submission can be immediate or queued up to ten entries | observed (`FUN_00465090`) |
| Stop cleanup releases a buffer and clears associated tracking slots | observed (`FUN_0047b220`) |
| Whether a submitted/queued request is eventually audible or how long it plays | unknown; no runtime sound observation was performed |
