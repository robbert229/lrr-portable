# 37 — Advisor animation controller owns a companion scene and request labels

The Advisor animation controller builds a separate DDiScene companion and
registers a compact set of named actions for map-pointing and talking. Its
request path is a distinct scene client layered on the shared DDiScene system.

**Observations from** REA/Ghidra 12.1.4 over `reference/LegoRR.exe` (SHA-256
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

The eleven names at `DAT_004b30f8` map through `AdvisorAnimationNameToIndex`
to eleven three-dword records beginning at `DAT_004b3124`. The first dword
holds the loaded scene action handle; `AdvisorConfigSlotLookup` stores two
parsed timing values in the next two dwords. The transition helpers index the
record by name-derived index, use its action handle for scene operations, and
consult the timing values during the completion/update path. The exact meaning
of each timing value remains unresolved.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| Controller registers the listed action-name strings | observed (`FUN_00401000`) |
| Setup creates and initializes a companion DDiScene | observed (`FUN_00401000`) |
| `FUN_0041fb50` calls the setup routine | observed callsite |
| Exact gameplay meaning and visual content of each action | unknown |
