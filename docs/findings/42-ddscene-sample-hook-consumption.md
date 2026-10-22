# 42 — DDiScene sample hook is consumed by action selection, not every draw

The sample-hook pending bit is set when an action name is selected. On the next
qualifying kind-4 draw-step with a resolvable frame, the step checks the hook's
name, slot, and global gate, then clears the pending bit. This makes the hook
an action-entry event rather than an unconditional per-frame callback.

**Observations from** REA/Ghidra 12.1.4 over `source/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-21. Evidence: `docs/evidence/legorr-advisor-animation.bundle` (65
records) and `docs/evidence/legorr-ddscene-hooks.bundle`. Focused IDs:
`ev_e383e254c55c097730b2555e7fc36017e6f8cf51f6a9ebf9ed0458c0cca8efed`
(`FUN_00473b40` flag helper), `ev_7f7ad309108f21dd6c5ba9ede20a2eab387ea8cfaae9e753f25ba18a60b74625`
(`FUN_00475140`), and `ev_54685b4639c266087fbaea3a5d39cf90fcf38dbcc979cee47a70be03b3ccb700`
(`FUN_00473370` action setter).

## Pending-bit lifecycle

`FUN_00473370` sets scene flag bit `2` after a named action resolves. During
`FUN_00475140`, the hook path is reached only for kind `4` when a frame resolves
and bit `2` is set. It invokes the registered sample hook only when the frame
name, callback slot, and global `DAT_0076cd94 & 0x40` gate are all present.
After those inner checks, it clears bit `2`.

Thus a missing callback or disabled global gate still consumes the pending bit
once the qualifying frame path is reached. If no frame resolves, the outer
block is skipped and this evidence does not establish consumption.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| Named action selection sets scene flag bit `2` | observed (`FUN_00473370`) |
| Qualifying kind-4 draw-step checks hook inputs and clears the bit | observed (`FUN_00475140`) |
| Callback fires every draw-step | contradicted by observed pending-bit gate |
| Bit consumption when no frame resolves | unknown; qualifying outer path is not entered |
