# 113 — Focus loss suppresses keyboard sampling while the frame callback continues

The window callback path reaches `FUN_004786c0`. Its `WM_ACTIVATEAPP` handler (message 0x1c) stores `wParam` in `DAT_00505808`; `WM_CLOSE` and `WM_QUIT` set the separate quit flag `DAT_0050580c`. The `WM_ACTIVATE` message (0x06) returns 0.

In `FUN_0047eef0` (`EnginePreTick`), a zero activation flag—unless feature bit `DAT_00505f84 & 2` forces the active path—clears the current and previous 256-byte key snapshots, calls an input-interface method at vtable offset +0x20, and returns before the normal keyboard-state read. The active path copies the current snapshot to the previous one, clears current state, and requests a 256-byte read through offset +0x24.

The outer loop `FUN_004777a0` continues to call pre-tick, update cursor deltas, and invoke the registered tick callback until the separate quit flag is set. Thus focus loss suppresses this keyboard sample path while the frame callback continues; it does not prove that all simulation or media subsystems advance.

When feature bit 2 is set, `FUN_00478010` filters out `WM_ACTIVATEAPP` and system-key messages, and the main loop sets `DAT_00505808 = 1` before pre-tick. That alternate path bypasses the inactive-input branch.

**Evidence.** REA/Ghidra 12.1.4, shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. Bundle: `docs/evidence/legorr-window-activation-key-poll.bundle`. Focused records: `ev_a89e45f6e2b1b9b8444bdee14eadf40ad5c6e5100e5ef64e833e48fa2436981e` (message handler); `ev_5742b69f99eefe2f7720198b58cea5fb4a9cb582b0c1675c1bb4b58f31908398` (keyboard gate); `ev_dab0eeeb42997d0b8f6a70f83b3b33f2b0d631471c4e432d655868d720bf00c7` (outer loop); `ev_5ab1af74c20aad04979597cf374961f3fa361648a5092ee2ef49ceb17fadd210` (alternate-mode filter); and `ev_ff4ba9933c6fff353709c08343449a826182a654f17b14fa725d559fe85941cf` (callback-to-handler call).

**Limitations.** The handler body is split at a non-function label and its dispatch edge is supported by a separate instruction observation. The input vtable methods remain unnamed. No runtime capture was made; the result does not show whether every media subsystem advances during focus loss or whether the separate pause-menu choice always pauses simulation.
