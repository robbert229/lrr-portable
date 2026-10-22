# 39 — Accepting an Advisor request resets its scene and submits two sounds

After request admission, `FUN_00401930` resets the selected scene and submits
two distinct audio events: one keyed by the slot's sound value and a queued
`SFX_CaptainSlide` event. The latter is submitted through the bounded pending
sound queue.

**Observations from** REA/Ghidra 12.1.4 over `source/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-21. Evidence: `docs/evidence/legorr-advisor-animation.bundle` (65
records). Focused IDs: `ev_051b90cc9739dbd16ee5d6820ad21fe7743cc2f68befb1e313c56ac10f7bdb98`
(`FUN_00401930`), `ev_be8e1e65cbfba0701c22725feb605a1782556c1323859d2ca1f54a393cf230fa`
(`FUN_00464fa0`), `ev_fbc624c0dcf6fb79a7fa07cd5d03d2473a3afc6434b1d81e19571d7ba5930d7c`
(`FUN_00464f60`), and `ev_ada2764cfb26e1819be00f3f3aa0e0d1be95295d7a3a5e3a1cc814bf1ced13a8`
(sound-name table).

## Reset effects

For a non-null scene pointer, the reset calls `FUN_00473b40(scene,0)`,
`FUN_00475140(scene,0)`, and `FUN_00464fa0(*(slot_record+0x0c),0)`. It then calls
`FUN_00464f60(0x2a,0)`. The sound-name table maps index `0x2a` to
`SFX_CaptainSlide`. `FUN_00464f60` appends an `(index,argument)` pair only while
the pending queue has fewer than ten entries.

The direct sound helper has its own bank and positional-queue gates. Static
submission does not prove either sound is audible in every accepted request.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| Accepted request reset calls the scene helpers at time zero | observed (`FUN_00401930`) |
| Reset submits the slot sound and sound index `0x2a` | observed (`FUN_00401930`) |
| Sound index `0x2a` names `SFX_CaptainSlide` | observed (name table) |
| Each submitted sound is actually played | unknown; downstream gates apply |
