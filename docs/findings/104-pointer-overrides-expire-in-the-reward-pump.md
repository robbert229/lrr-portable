# 104 — Timed cursor overrides hold through reward-screen input polling

The cursor keeps an explicitly selected pointer while its override counter is
positive. The reward-screen polling loop decrements that counter by a fixed
`1.0` on each pump call, requests pointer slot zero, then draws the selected
pointer at the sampled screen coordinates. Once the counter is nonpositive, the
normal slot-zero request can take effect.

**Observations from** REA/Ghidra 12.1.4 over `source/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence: `docs/evidence/legorr-pointer-cursor-config-and-override.bundle`.
Focused IDs: `ev_74f3f24a26bbc9457ddfdd61356b21c7578a7234ede6126a7b4de381a802a9ae`
(`FUN_0045cc20`), `ev_0ec7a909715c0dcfaaeb31c4aae45aee77d526a3e31036328f99ac7993a73330`
(`FUN_0045cc40`), `ev_58848cb5c1a2aee548fc24f0e549601d027a1723fa2fee7eb26d6a6e4daa2486`
(`FUN_0045cd90`), `ev_468e8a011275d928ac59bb612cc8bce4c8da7e48f2bc463d4507239078f3f96c`
(`FUN_0045cc70`), `ev_1557291d2d071bc83850ff55a4ea649b63c4abd6aec986ec69d6aab3b49c48af`
(`FUN_00462700`), and `ev_36ab47af724a52b29241605087cd3d0edead27afee8cad0bdbfcda5d263a0ca9`
(`FUN_00461410`).

`FUN_0045cc40(index, duration)` sets both the selected slot and its countdown.
`FUN_0045cd90(delta)` subtracts from that counter, while `FUN_0045cc20(index)`
changes the selection only when the counter is nonpositive. The selected
resource is drawn by `FUN_0045cc70`: static slots use the bitmap blit path;
animated slots add the configured hotspot offsets and use the animation path.
`FUN_00462700`, called inside the `FUN_00461410` reward-screen loop, passes
`1.0` to the countdown helper before requesting slot zero and drawing at the
current cursor coordinates. Thus this modal loop measures the override in
pump iterations rather than using its frame-delta argument.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| Explicit selection stores a duration; ordinary selection is ignored while it is positive | observed across `FUN_0045cc40` and `FUN_0045cc20` |
| Reward polling decrements the counter by `1.0` per call, then requests slot zero and draws | observed across `FUN_00461410` and `FUN_00462700` |
| Static and animated cursor slots use separate draw paths; animated draw includes the stored offsets | observed in `FUN_0045cc70` |
| Override duration in real elapsed time during the modal reward loop | unknown; the loop passes a fixed unit per iteration |
