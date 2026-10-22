# 40 — Advisor transition worker coordinates the selected and companion scenes

`FUN_00401990` advances an accepted Advisor transition across the selected
scene and its controller companion. It applies a per-slot transform, advances
an action window, and restarts both scenes when the transition helper reports
completion.

**Observations from** REA/Ghidra 12.1.4 over `source/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-21. Evidence: `docs/evidence/legorr-advisor-animation.bundle` (65
records). Focused IDs: `ev_3fcdebd967d4f2cf24b0059e4db108bd911cc20a613821c70668f0bffcb9ec6e`
(`FUN_00401990`), `ev_bf1360b4fe12780f09c9c4ed23853601ddcff90a8934ce7a5ff7d6030658c922`
(`FUN_00401b30`), and `ev_2ee0c96724d2f29cbec0e7a6667890f16bbd172e6727eb9329063205f71e5fc9`
(call instruction at `0x423c2c`).

## Observed transition work

When state bit `0` is set and `DAT_00556ec8` is nonzero, the worker optionally
calls `FUN_0046ac60` for slot flag `0x40000`, updates both scene objects through
`FUN_00473b40`, links the companion with the selected scene using
`FUN_004734c0`, applies `FUN_00401b30`'s slot-derived transform, then calls
`FUN_00401a70(actionIndex,delta)`. A nonzero result leaves the active state
set. A zero result clears bit `0` and restarts both scenes in mode `1`.

The transform helper combines per-slot offsets at `+0x14/+0x18` with an
optional query, transforms the result, and writes it to the selected scene.
Ghidra identifies a call to the worker at `0x423c2c`, but assigns that address
no containing procedure; the worker's complete caller/reachability context is
therefore not established.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| Worker coordinates selected scene and companion during active state | observed (`FUN_00401990`) |
| Completion clears active bit and restarts both scenes | observed complete body |
| Transform is derived from per-slot fields and written to selected scene | observed (`FUN_00401b30`) |
| Containing procedure and reachability of callsite `0x423c2c` | unknown |
