# 41 — Advisor action windows stop or wrap based on a request bit

`FUN_00401a70` advances the selected scene by a delta and compares its
remaining-time result with the controller's second state bit. The bit chooses
whether a positive remainder ends the transition or wraps the draw time back
into the slot's configured action window.

**Observations from** REA/Ghidra 12.1.4 over `reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-21. Evidence: `docs/evidence/legorr-advisor-animation.bundle` (65
records). Focused IDs: `ev_45182b736c7ab2c821293c026339cd41e60466598ad3d01ae1d6cc581d996327`
(`FUN_00401a70`), `ev_1218d653f6b3228a259532869613ba1e8809577179f0e5c302bac1df8b98b1dd`
(`FUN_00401a70` assembly), `ev_9b6e30a77ddcbba7d1356b06fc4744c4edc1bbfac6a21a9843d3ab2075d84c84`
(`FUN_00475120` assembly), and `ev_d185d72438dc44e2ce909755804836ec0a3d5c04b36284d4107b25b64371a0c2`
(`FUN_00475390`).

## Window behavior

The helper calls `FUN_00475120(scene,delta)` and inspects the returned
remaining-time value. With bit `1` clear, a positive value returns zero to
signal completion. With bit `1` set, a positive value draws at slot start plus
the overflow and remains active. When the value is nonpositive, the helper
queries scene duration; if the configured slot end is within it, it draws at
`scene_length - slot_end + slot_start`. These paths return one.

There is a type mismatch in Ghidra's decompiler: it declares `FUN_00475120`
void, while the caller consumes an x87 floating-point return. The instruction
evidence shows the call followed by floating-point use, and the helper body
delegates to `FUN_00475140`; the API's source-level type is not recovered.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| State bit `1` selects completion vs overflow behavior | observed (`FUN_00401a70`) |
| Wrapped draw times use configured slot start/end and scene duration | observed complete body |
| `FUN_00475120` returns a source-level float | unknown; machine-level x87 result is consumed |
