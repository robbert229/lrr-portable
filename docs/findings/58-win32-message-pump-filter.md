# 58 — Win32 message pump filters selected messages before dispatch

**Observations from** REA/Ghidra 12.1.4 over `source/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence bundle:
[`legorr-win32-input-path.bundle`](../evidence/legorr-win32-input-path.bundle).

`FUN_00478040` drains the thread's queued Win32 messages with `PeekMessageA`
using a null window filter and removal flag `1`. For each message it calls
`FUN_00478010`; accepted messages go through `TranslateMessage` and then
`DispatchMessageA`, while rejected messages skip both calls. The pump loops
until the queue has no message ready, so this path does not wait for a message
before the rest of the frame proceeds. [Evidence:
`ev_61d8e3aef50acdf6156f7668d4090a99959b00a9c6230946d0e2b95dbd3ad1a7`]

The filter rejects only three message IDs, and only while bit `0x2` of
`DAT_00505f84` is set: `0x1c` (`WM_ACTIVATEAPP`), `0x104`
(`WM_SYSKEYDOWN`), and `0x105` (`WM_SYSKEYUP`). This is a dispatch gate for
that mode bit; the binary evidence does not name the mode or explain why those
messages are suppressed. Other IDs pass this filter, including ordinary
`WM_KEYDOWN` (`0x100`) and `WM_KEYUP` (`0x101`). [Evidence:
`ev_535f060a3ba41fe5ee2bfe65f8d171a4f1c4794ad8f40b1df3f2bbb746ce2ec9`]

In the main loop at `FUN_004777a0`, the message pump runs before
`FUN_0047eef0` polls the 256-byte DirectInput state buffer and before the
mouse-position update. The same ordering appears in the secondary tick helper
`FUN_00477f30`. Thus queued Win32 messages are dispatched before that frame's
sample of the key-state table; this is an observed call order, not a timing
guarantee about when the OS enqueues input. [Evidence:
`ev_d33f142852d8d7442ed67a0c9c5d4c3193ccfc911051c0fe9e5921134b4cb618`,
`ev_bc7ffdfa7b5f5de4f720417e10e5c06df8e6cf4145f318cd7a375561eb7db2b9`]

The registered window class uses a callback pointer represented by Ghidra as
`LAB_00478880`; that address is outside a defined function body in this
analysis. Two nearby message-handler functions are present, but the current
static evidence does not safely resolve which one is reached by that callback.
This finding therefore describes the message queue's translation/dispatch
gate and leaves full window-procedure routing open. [Evidence:
`ev_92f3d5724b17f1604c18c0105f90be305b0c44cc54f303deea6659ac1e60896a`,
`ev_f0cc15883e4efc0177059198f304b0f12cbaa4e8b5e27e81bef588dabf879045`]

## Open questions

- What runtime state sets `DAT_00505f84 & 0x2`, and what user-visible mode it
  represents.
- Which callback implementation begins at `LAB_00478880` and how it routes
  messages to the nearby handlers.
- Whether the dispatched character messages are consumed elsewhere; the
  keyboard text path documented in #59 reads the DirectInput state table.
