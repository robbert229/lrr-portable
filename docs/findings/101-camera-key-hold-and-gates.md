# 101 — Camera key adjustments repeat while held behind gameplay and input gates

REA/Ghidra 12.1.4 analysis of the shipped `reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`). Evidence
is in [`legorr-camera-control-focused.bundle`](../evidence/legorr-camera-control-focused.bundle).

`FUN_004250c0` reaches the camera-key block only when the outer activity mask
`DAT_00556ecc & 0x08100000` is zero, the current activity global
`DAT_00556ec8` equals 1, and `FUN_00456240(0)` has bit `0x1000` clear. Under
that gate, six nonzero key-state bytes dispatch into three adjustment routines:

- `DAT_0076ac50` / `DAT_0076ac48` call `FUN_00436900` with `dt * ±0.02`.
- `DAT_0076ac4b` / `DAT_0076ac4d` call `FUN_004369f4` with `dt * ±0.02`.
- `DAT_0076ab8c` / `DAT_0076ab8d` call `FUN_00436aa5` with `dt * ±3.0`.

These checks use the current bytes directly; unlike nearby edge-triggered
branches in this same function, they do not compare against a previous-frame
snapshot. The called wrappers add each delta to one of three stored camera
values before their setters apply the result, subject to per-value bounds unless
a context flag bypasses the bound. This supports held-state, per-frame camera
adjustments while the gates permit them. [Evidence:
`ev_12a3069b5225afed9f559a123c78f81fc351b8f40cbab4535fc4eee97ece02b7`,
`ev_34e1204d6c705edbcfd12f7a17227e972c02b61c0067154d7b0f694f944e344f`,
`ev_0d4b9f8c62b836937d7129a436c4cd09e157ef9736e053df285a39d60d92b744`,
`ev_9e9e6af157143857662d5815b8e6079d9ef7bcddcc1221e4e4d427d43b2a958a`,
`ev_e27b15df2ea5ee963440ed679e57e59d4d8d4bdfd44f20ee9a69011fa14f2ad4`,
`ev_b347c64f07edefb153eb0d789e1b4849a303a5aaf69bd777a2598b7df6df7dd7`,
`ev_17b002593a6a25d5ad17b6c9208a2527908fb73dd8d4848fd3fa08e73682c3cb`]

## Limits

The executable does not identify these six byte globals with user-facing key
names in this path, and the activity-mask bit meanings are unresolved. This is
static code evidence, not a runtime observation of a key press.
