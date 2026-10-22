# 43 — DDiScene end-hook path has no local one-shot latch

The scene-end callback path in `FUN_00475140` is gated by state, callback
pointer, and a frame-time crossing condition. That path does not update a
per-object “already fired” flag before returning, so repeated qualifying calls
remain eligible in the static control flow.

**Observations from** REA/Ghidra 12.1.4 over `source/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-21. Evidence: `docs/evidence/legorr-advisor-animation.bundle` (65
records) and `docs/evidence/legorr-ddscene-hooks.bundle`. Focused IDs:
`ev_7f7ad309108f21dd6c5ba9ede20a2eab387ea8cfaae9e753f25ba18a60b74625`
(`FUN_00475140`) and `ev_f7a2df2d724421de3f52394404ec38869bf96c1810b031751c4019d61bb58853`
(resolved callers and coverage).

## Observed callback condition

In the state-bit-`8`-clear branch, the step calls `FUN_004897e0` and, when its
result is not `1`, tests the end-hook pointer, a nonzero end frame, and the
crossing `scene_length < last_frame && last_frame <= requested_time`. When all
hold it calls `DAT_0076cd80(owner, userdata)` and returns early. The callback
path itself does not set a local flag that disables the same condition later.

This establishes static eligibility for another call under the same
conditions, not observed duplicate runtime callbacks. `FUN_00475140` has 24
resolved callers and four omitted; caller coverage does not establish whether
any particular scene is stepped again after crossing its end frame.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| End callback requires the stated time and pointer predicates | observed (`FUN_00475140`) |
| Callback path sets a local per-object one-shot latch | contradicted by complete path |
| Repeated qualifying steps actually dispatch duplicate callbacks at runtime | unknown; runtime sequence not observed |
