# 37 — Advisor animation controller owns a companion scene and request labels

The Advisor animation controller builds a separate DDiScene companion and
registers a compact set of named actions for map-pointing and talking. Its
request path is a distinct scene client layered on the shared DDiScene system.

**Observations from** REA/Ghidra 12.1.4 over `source/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-21. Evidence: `docs/evidence/legorr-advisor-animation.bundle` (65
records). Focused IDs: `ev_79ee0e34716177ba6b878eb73432eed4ae1b8411b369cd3808039f2bc324e9a1`
(`FUN_00401000`) and `ev_48b6921e30e46ea64b7974c469f8b7e208bcdac8bb707dd8112ef3d7b37165e7`
(`FUN_0041fb50` callsite).

## Observed setup

`FUN_00401000` populates action-name slots with `Advisor_Anim_PointToMap`,
directional `Advisor_Anim_Point_{N,NE,E,SE,S,SW,W,NW}`, `Advisor_Anim_Point_Up`,
and `Advisor_Anim_Talk_Top`. It also creates a companion scene from the object
at `param_2+0x24`, attaches it using the root object, initializes it, and sets
the controller state word `DAT_004b31bc` to zero. The identified initializer
`FUN_0041fb50` calls this setup routine.

The strings establish labels and setup inputs, not which gameplay situations
select each action or what the animation looks like at runtime.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| Controller registers the listed action-name strings | observed (`FUN_00401000`) |
| Setup creates and initializes a companion DDiScene | observed (`FUN_00401000`) |
| `FUN_0041fb50` calls the setup routine | observed callsite |
| Exact gameplay meaning and visual content of each action | unknown |
