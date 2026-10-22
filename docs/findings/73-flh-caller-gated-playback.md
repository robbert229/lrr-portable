# 73 — `.flh` frame stepping is caller-gated; drawing continues while frozen

Finding 21 closes the FLIC-family container and decoder. This finding isolates
the runtime contract around `FUN_00484070`: each allowed call advances at most
one frame, but the overlay still draws the current surface when advancement is
denied. The frame player's cadence and hold behavior therefore come from its
caller, not a per-frame clock stored in the `.flh` stream.

**Observations from** REA/Ghidra 12.1.4 over `source/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence: `docs/evidence/legorr-flh-playback-state-machine.bundle`.
Focused IDs: `ev_7fd7c31c36015c936e28d92b93cf85589d3374705fc96b39f9ff17eb87cd564e`
(`FUN_00484070`), `ev_31d4c800a41a1e716d7a0a377f41dfa810124f477f6596ba9df204cf4862a15`
(tick assembly), `ev_85917df29bc1f5de07a4713f44fcf41cc0086397d247370db6a81e116141fd10`
(RAM stepper), `ev_dc32d9812f840004f6d2b7d2fbf03bea582f8e5cd986ba1edc71457e12d98645`
(stream stepper), `ev_b00de1cd0cd4d1b0a3dfbb4b035e9ba611a11f21ce77e62ecea63ea3e83c60a8`
(overlay caller), `ev_5c94fcf74470a3c30367dd9866366dd1a0eb1d70948f534e6e0ccfa2c889ff33`
(reward caller), and `ev_453784f813b453b34327526d1f6d9a1e22a4a5e8c0eebde5208a62e232b55f47`
(complete resolved direct-caller inventory for the tick).

## Tick contract

`FUN_00484070` converts the four caller-supplied rectangle floats, locks the
surface, and evaluates a caller-supplied advance gate. On its first frame it
steps regardless of that gate; after the frame index is nonzero, it calls
exactly one mode-specific stepper only when the gate is nonzero. It then
unlocks and blits the surface on either branch. The fourth call argument is
forwarded to the blit flags as `0x8000` when nonzero. The tick body contains no
clock read; its four float conversions are rectangle conversion, not elapsed
time measurement.

Each stepper calls the shared chunk/frame walker once and increments the frame
index once. A stepper reports completion at the non-loop boundary; in loop
mode, the RAM and streaming paths rewind differently as documented in finding
21. A hold chunk still occupies a frame step even though its zero-record body
does not change pixels. `FUN_00484070` returns true when its stepper did not
report completion, including when advancement was gated off, so the return
value does not mean that the displayed frame changed.

## Two caller policies

| caller | third argument to tick | consequence |
| --- | --- | --- |
| `FUN_0045a730`, pointer/overlay table | `(~DAT_00556ecc >> 20) & 1` | only advances while this global bit expression is nonzero; the wrapper still positions and draws the current frame when zero |
| `FUN_00461790`, reward task display | `frame_index < 0x23` | advances its non-loop clip through the last frame; at index 35 the gate closes and later draws keep the last decoded frame (index 34) |

The pointer overlay caller supplies `1` as the fourth argument; the reward
caller supplies `0`, selecting distinct blit-flag behavior without changing
the advance-gate rule. The complete resolved-caller query lists the startup,
pointer-overlay, screen-overlay, and reward-screen paths, but does not account
for unresolved indirect calls.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| One stepper call per permitted tick; the surface is blitted even when the step is skipped | observed (`FUN_00484070`, both steppers) |
| FLIC tick itself has no time-based frame scheduler | observed in the tick body and assembly; caller cadence is outside this function |
| Reward path stops stepping at index 35 after decoding the final index-34 frame | derived from the caller predicate and the non-loop stepper boundary; runtime display duration was not replayed |
| Meaning of `DAT_00556ecc` bit 20 in the pointer path | unknown here; only its value as the advance gate is established |
| Actual frame rate and screen presentation cadence | unknown; static call sites do not establish runtime cadence |
