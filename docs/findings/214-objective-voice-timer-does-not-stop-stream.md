# Finding 214 — Objective Voice Timer Does Not Stop the Stream

`FUN_00458be0` stores a result from the objective-voice submission in
`DAT_004ffe3c` and computes a separate countdown in `DAT_004ffe40`. On each
update, when the sentinel is not `-1`, it subtracts the frame delta from that
countdown. When the countdown reaches zero, the function calls
`FUN_00401b20` and sets `DAT_004ffe3c` to `-1`; the expiry branch does not call
the stream-stop helper.

`FUN_00401b20` only clears bit `0x2` in `DAT_004b31bc`. The explicit state-zero
stream stop is in `FUN_004588e0`, which calls `FUN_0047ba20(0)` only while
`DAT_004ffe3c != -1`. Since timer expiry sets the sentinel to `-1`, that later
cleanup path skips its explicit stop for the expired request. The timer thus
clears a local/UI state marker; it does not serve as an audio stop timer. The
stream may still end naturally or be replaced by another state-zero request
(finding 136). This is a static control-flow result, not evidence of audible
duration.

**Evidence.** Static disassembly of shipped `reference/LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
In `FUN_00458be0` at `0x458be0`, the expiry branch calls `FUN_00401b20` at
`0x458df2` and writes `-1` to `DAT_004ffe3c` at `0x458df7`. The timer setup
and voice submission are at `0x458e5e–0x458e9f`. `FUN_00401b20` at `0x401b20`
only clears `DAT_004b31bc & 2`. `FUN_004588e0` at `0x4588e0` conditionally
calls `FUN_0047ba20(0)` if the sentinel is not `-1`. These functions are in
`docs/decompiles/full/LEGORR.full.c`; finding 140 documents the voice alias
families and selector zero, and finding 136 documents stream-state replacement
and stop behavior.
