# 47 — Entity event transitions feed type-specific incident messages

At one entity-state transition in `FUN_0043bc40`, the game submits a shared
message ID `0x26` and then selects one additional message ID by entity type:
type `2` → `0x0a`, type `1` → `0x0b`, and type `4` → `0x0c`. Finding 22's
catalog maps those latter three IDs to `Info_LegoManDeath`,
`Info_VehicleDeath`, and `Info_BuildingDeath` respectively.

**Observations from** REA/Ghidra 12.1.4 over `reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence: `docs/evidence/legorr-campaign-expansion.bundle`.
Focused IDs: `ev_cb67dd44b4a1a803dee82f4abe7eb06a9d4e5aad251bbfab9e77bcd31969d456`
(`FUN_0043bc40`). For catalog mapping, see finding 22 §1.

## Observed dispatch

After the function's common event work, it calls `FUN_00419b80(0x26, 0, 0,
position)`. It then branches on entity type: `2` calls the dispatcher with ID
`10`, `1` with `11`, and `4` with `12`, using the same position pointer.
Other entity types do not take one of these three specific-ID branches.

This links the existing message catalog to one producer path, rather than
inferring message production from the names alone. It does not establish that
the dispatcher displays or plays each configured message every time; the
message bus, config gates, and runtime state still apply.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| The transition path submits shared ID `0x26` | observed in `FUN_0043bc40` |
| Types `2`, `1`, and `4` select IDs `0x0a`, `0x0b`, and `0x0c` | observed |
| Those three IDs correspond to minifigure, vehicle, and building death catalog names | mapped by finding 22's catalog ordering |
| Semantic meaning of shared ID `0x26` at this call site | unknown |
| Whether the bus displays/audio-plays the notification in a particular runtime state | unknown; static producer path only |
