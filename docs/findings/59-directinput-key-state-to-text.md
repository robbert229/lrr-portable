# 59 — DirectInput key-state snapshots feed text entry

**Observations from** REA/Ghidra 12.1.4 over `reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence bundle:
[`legorr-win32-input-path.bundle`](../evidence/legorr-win32-input-path.bundle).
This traces the native input source behind the key tables and text pump
described in findings [04](04-sim-input-and-events.md) and
[05](05-exit-selection-and-text-input.md); the separate held-key command table
is in [51](51-held-key-command-repeat.md).

## Device setup and polling

`FUN_0047ed90` creates a DirectInput interface and device, applies the data
format at `DAT_0049c790`, sets cooperative-level flags to raw value `6`, and
acquires the device. The decompiler does not recover a symbolic device or data
format name, so the device identity is described here by how the program uses
it: `FUN_0047eef0` asks it for exactly `0x100` bytes into
`DAT_0076ab80`, the 256-entry table consumed as key state elsewhere.
[Evidence: `ev_0458f5888b2c75e12d9fb03ecf49b49abeaaa95644fe5a3dfa1b0df540bbd006`,
`ev_1b8441c3c389660b8ace9e868d8e3c3833f6397aee044b8885bc7e615cfe7901`]

Each active poll copies the current 256 bytes from `DAT_0076ab80` to the
poller previous-state table `DAT_0076aa00`, clears the current table, then
invokes the device's `GetDeviceState` vtable slot with size `0x100`. If the
read fails, it clears the ready flag and tries the device's `Acquire` slot; if
the application is inactive, it clears both poller tables and calls the
device's `Unacquire` slot. The separate text-entry mirror `DAT_004de430` is
updated on the text-entry path by `EnginePreTick`, as described below and in
Finding 04. [Evidence:
`ev_1b8441c3c389660b8ace9e868d8e3c3833f6397aee044b8885bc7e615cfe7901`]

The table's observed addressing range is raw byte index `0x00`–`0xff`; this
analysis does not establish that those indices are Win32 `VK_*` values or
recover a human-readable key label for every entry. The prior/current
comparison used by text entry is an edge check on these bytes.
[Evidence: `ev_93c4cfdb4802a452ba37f8f9e2aa51c7624ae8d8cef34209b11ef719984a1fc6`,
`ev_9177545a3f38a8b8c2ce52f2211f395983b9d291254b93c21bc8b60556f32631`]

## Text-entry consumer

When `DAT_00556fa0` is non-null, `EnginePreTick` scans all 256 key-state
indices. It accepts a nonzero current byte only when the prior-frame byte is
different, passes the index through `FUN_00411f00`, then sends the returned
value to `FUN_0048e380` and appends the resulting character while the current
string length is below `0x0b`. At the end of that path it copies current state
to the mirror and clears current state. This is the text-entry edge path; it
does not read `MSG.wParam` or consume a `WM_CHAR` argument in this function.
[Evidence: `ev_9177545a3f38a8b8c2ce52f2211f395983b9d291254b93c21bc8b60556f32631`]

`FUN_0048e380` uses the active keyboard-layout handle when present, consults
the byte/scancode tables, and calls `FUN_00493ca0` for translation. With no
layout handle it only uppercases the ASCII range `a`–`z`; therefore the layout
aware text behavior is conditional on `DAT_00549180` being non-null.
[Evidence: `ev_65c66bb0b3dc7f163e9968f41906323c70347645dff8c5d871a238ced28cd2da`]

## Caveats

- The exact symbolic DirectInput data format and the human-facing meaning of
  every byte index remain unresolved.
- This is static evidence from the shipped executable, not a runtime trace of
  keyboard hardware or OS layout behavior.
- The key table is sampled after the Win32 queue pump in the main loop (finding
  #58); the evidence does not establish a precise OS event-to-frame latency.
