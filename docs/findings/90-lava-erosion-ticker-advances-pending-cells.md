# 90 — A lava-erosion ticker advances pending cells through timed stages

The ticker `FUN_0040eab0` checks the configured trigger interval and scans the
pending-coordinate table for cells eligible to advance. It dispatches event
`0x1e` for a selected cell, updates its erosion stage and timer, and manages a
lock interval that prevents immediate reprocessing. The candidate gate in
`FUN_0040f000` checks the cell state and four neighbors while excluding cells
already queued or locked. `FUN_0040e9c0` derives the next-stage delay, and
`FUN_0040cce0` selects and places an effect variant.

**Observations from** REA/Ghidra 12.1.4 over the shipped `reference/LegoRR.exe`
(SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence bundle:
[`legorr-lava-erosion-focused-expansion.bundle`](../evidence/legorr-lava-erosion-focused-expansion.bundle).
Focused records: `ev_4c40e0572b48cadc98f523ab1ceca969eb70e3a11fff648eaa3fd8ec035f758d`
(`FUN_0040eab0`), `ev_a7305baa852cc1e669e55d9b2cf33c72ac185329057b27252452c48f6aba6f32`
(`FUN_0040f000`), `ev_0b131a4de0069f69b323e6c12b5cff19e8e37cec0bb599c0424333bbe4aa58ac`
(`FUN_0040e9c0`), and
`ev_8a91dc3fb9433e8764a52bac53a1688226c76c70f4b8a9df4bcb9c600137091c`
(`FUN_0040cce0`).

The stage names, event's visible outcome, and timing under an active level remain
unknown. This is static shipped-artifact evidence; no runtime erosion was
observed.
