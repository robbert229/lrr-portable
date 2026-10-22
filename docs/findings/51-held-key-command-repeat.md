# 51 — Held debug keys replay registered commands

`EnginePreFrame` installs a compact key-to-command table, and
`CommandQueueTick` checks the configured key state before its ordinary queue
dispatch. While the time-freeze bit is clear, a key that remains down enqueues
its registered opcode and arguments on each tick. The shipped image therefore
supports held-key repetition; it does not identify the user-facing names of
the five registered keys.

**Observations from** REA/Ghidra 12.1.4 over `source/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence: `docs/evidence/legorr-command-hotkey-expansion.bundle`.
Focused IDs: `ev_9edd71f55757f615737c8493ccc3623790a37b34b4bff033d04600717640a764`
(`FUN_00452010`, registration writes),
`ev_52e2bebf0234c2c9c691bca227e1b36bce733891bec2d0d74e87cb8175095fc0`
(`FUN_0041fb50` outgoing call sites),
`ev_b16ce5599f6591cf8a19e7569c83679509da373f8c94262a0500b75ce75cf21a`
(call arguments), and
`ev_3c5098e26c267d5c03bf6916710d965400a770fa80e55c9e38d678e6afe6dc9f`
(`FUN_004520d0`, pre-dispatch key polling).

## Registration and repeat path

`FUN_00452010(key, opcode, arg1, arg2, extra)` stores the key byte in
`DAT_004ff0bc[index]`, with the opcode and arguments in a parallel five-dword
record beginning at `DAT_004ff0c8`. Its only resolved caller is
`FUN_0041fb50`, which registers four rows: key bytes `2` and `3` map to opcode
`9`; key byte `5` maps to opcode `10`; and key byte `4` maps to opcode `11`.
The key bytes are raw table indices into the key-state array; the binary
evidence does not supply their human-facing labels.

On each `FUN_004520d0` tick, when bit `0x100000` in `DAT_00556ecc` is clear, the
routine reads each registered key byte and checks the corresponding
`DAT_0076ab80` key-state entry. A nonzero state enqueues that row's stored
opcode and arguments through `FUN_00452060`. The key-state test is level-based,
not edge-based, so a key held across ticks can enqueue repeatedly. The queue
helper silently drops an enqueue when the selected 2048-entry buffer is full.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| Registration stores a key byte separately from opcode and arguments | observed in `FUN_00452010` |
| Registration call sites are in `FUN_0041fb50` | observed; complete outgoing reference inventory |
| Polling is gated by `DAT_00556ecc & 0x100000` and checks current key state | observed in `FUN_004520d0` |
| A held key can enqueue on successive ticks | observed from the level-based state test |
| Human-facing key labels and intended meanings | unknown; values are table indices, not recovered labels |
| Four registered key/opcode pairs | observed in the call-site argument pushes |
