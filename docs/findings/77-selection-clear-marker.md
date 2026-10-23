# 77 — A zero-mode selection request clears the current set through opcode 3

The selection manager has a distinct path when an entity is already selected or
has a marker at `entity+0x3ec` bit 1. In `FUN_004526e0`, a zero-valued mode
routes that entity to `FUN_0044c4a0` and returns without adding it. The helper
then enqueues command opcode 3 and resets HUD context 0; the queue dispatches
opcode 3 to a whole-selection clear routine. This connects an entity-level
selection request to a global selection reset and marker transfer.

**Observations from** REA/Ghidra 12.1.4 over `reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence bundle:
[`legorr-selection-clear-expansion.bundle`](../evidence/legorr-selection-clear-expansion.bundle).

## Path

1. **Zero-mode branch.** `FUN_004526e0` normalizes an indirect entity pointer
   when the field at `+0x304` is nonzero, then runs its candidate prefilter. If
   its mode argument is zero, it checks `FUN_00452690`: that helper returns
   true when the entity is already in the selection or when the byte at
   `+0x3ec` has bit 1 set. A true result calls `FUN_0044c4a0(entity)` and
   returns zero without taking the add path. Otherwise the routine checks for a
   duplicate, enforces the count limit of 100, appends the entity, and sets
   selected bit `0x200000` in the dword at `+0x3e8`.
   Evidence: `ev_55bbc27367562205fe44d9664c9ee2ec32b32eba750a2f1d7cb692fd01f57d89`,
   `ev_abe96a437ce61de4312961ddb88085c902c5deeb4b0f57d1cdfdd3ea19306338`,
   `ev_637c51176204c672575fe103d5b6e28d7722d3b5fa012a93ed9da91e519ac789`.
2. **Request enqueue and HUD reset.** `FUN_0044c4a0` may set marker bits after
   checking entity state, but it unconditionally calls `FUN_0041e750`.
   `FUN_0041e750` enqueues `(3,0,0,0)` through `FUN_00452060`, then calls
   `FUN_0041b2d0(0,0)`. Evidence:
   `ev_8909f2bb8655bc104294de4a8e0597850dcc2f327fe169faeb1c4b16aabb558a`,
   `ev_2cb4b43963f339b78f88c6c54e8d9ecdea1278dd69aa91b3feb3744223f09b1b`.
3. **Whole-set dispatch and marker transfer.** In `FUN_004520d0`, opcode 3
   calls `FUN_00452be0`. The latter walks the active selection array. For each
   entry still carrying selected bit `0x200000` at `+0x3e8`, it sets bit 1 in
   `+0x3ec`, clears that selected bit, and clears bit 0 in `+0x3ec`. It then
   sets the active selection count to zero. The count is cleared; this body does
   not show the backing pointer array being zeroed.
   Evidence: `ev_4ac1a8328dd2f35d8e57a6fd23a719dcbbc264a330afa63a0cde80ef8532a2bb`,
   `ev_fddc078c8acbefeeb7a523f82995bd973683f850c3868d6cbf4bd773cbe0b0e1`.

`FUN_004526e0` has a resolved direct caller at `FUN_004526c0`, which in turn is
dispatched by opcode 1 in `FUN_004520d0`; the direct caller inventory is
complete for those function bodies. Evidence:
`ev_9c7231fc7cef1bb8c9f2f9653bf04986919a75cfb5591b86974e0518350916c0`,
`ev_de0301202d6ab4b45cc97369422e9e7cc2c07832dc9b58eede1745eef71f05e`.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| With mode zero, an already-selected or marker-bit entity takes the helper path and is not appended | observed (`FUN_004526e0`, `FUN_00452690`) |
| The helper enqueues opcode 3 and resets HUD context 0 | observed (`FUN_0044c4a0`, `FUN_0041e750`) |
| Opcode 3 marks each still-selected entity, clears its selected bit, and sets the logical selection count to zero | observed (`FUN_004520d0`, `FUN_00452be0`) |
| User-facing meaning of the mode value, marker bits, and the conditional state checks in `FUN_0044c4a0` | unknown |
| Whether the queue path executes for a particular user interaction | not runtime-verified |

This is static shipped-artifact evidence. It establishes the code path and
writes, not which input event supplies mode zero or what the marker means to
the game UI.
