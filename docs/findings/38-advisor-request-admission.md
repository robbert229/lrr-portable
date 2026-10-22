# 38 — Advisor animation requests pass three admission gates

`FUN_004018c0` admits a request only when its slot is marked available, the
shared gate `DAT_00556ec8` is nonzero, and the controller's active bit is clear.
Accepted requests set an active slot and encode one of two request modes in a
separate controller state bit.

**Observations from** REA/Ghidra 12.1.4 over `source/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-21. Evidence: `docs/evidence/legorr-advisor-animation.bundle` (65
records). Focused IDs: `ev_00851f2d5dd5ae373e6ea3d1f8308354666d0f0d393fcdd80399ef08d5019853`
(`FUN_004018c0`) and `ev_925a7577485ce34af4cf975c97ab478c67ead976d0a793d8ea64c94ff8b98baa`
(resolved callers).

## Admission and state

The slot is addressed through a `0x24`-byte record stride. Admission tests
record flag `0x20000`, `DAT_00556ec8 != 0`, and `(DAT_004b31bc & 1) == 0`.
On success, the routine stores the slot in `DAT_004b31a8` and calls
`FUN_00401930(slot)` to reset its scene state. When the second argument is
nonzero it ORs `3` into `DAT_004b31bc`; otherwise it clears bit `1` and sets
bit `0`. The function returns one only for an accepted request.

Resolved callers are `0x41c4f0`, `0x456830`, `0x458be0`, and `0x45c670`.
Their argument sources differ; the constants and state transitions do not
establish user-facing names for the two bits or the shared gate.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| All three admission conditions must pass | observed complete body |
| Accepted request records slot, resets it, and changes state bits | observed complete body |
| Four resolved callsites reach the request function | observed caller inventory |
| Semantic names for the state bits and shared gate | unknown |
