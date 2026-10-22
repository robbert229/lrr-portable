# 46 — Objective sidecar records pass an additional append gate

The `.osf` completion sidecar is populated from an entity-removal path, but an
entity entering that path is not sufficient by itself to produce a record.
`FUN_0043bc40` checks a type, a flag in the entity's byte at `+0x3ec`, and a
removal-state bit before calling `FUN_00459190`; `OsfAdd` then requires at least
one of three fields to be nonzero before appending its 20-byte entry.

**Observations from** REA/Ghidra 12.1.4 over `source/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence: `docs/evidence/legorr-campaign-expansion.bundle`.
Focused IDs: `ev_cb67dd44b4a1a803dee82f4abe7eb06a9d4e5aad251bbfab9e77bcd31969d456`
(`FUN_0043bc40`), `ev_c0dc781076e42aa3f46d487936401d3ecf3c392a6a9e522087a00de55abf7186`
(`FUN_00459190` / `OsfAdd`), and `ev_b73586a2f09bfe12391547a21bb7271a9cee6a7807440c98d2c501a4fceb980d`
(`FUN_004593e0` / `OsfWrite`).

## Observed append conditions and record

In the entity routine's `0x800000` state branch, the call to `OsfAdd` occurs
only when `entity.type == 2` and byte `entity+0x3ec` has bit `0x08` set. The
`OsfAdd` body applies a second gate: it returns without adding anything unless
at least one of `entity+0x3f0`, `entity+0x330`, or `entity+0x08` is nonzero.

When admitted, the routine grows capacity in blocks of ten if count equals
capacity, increments the record count, copies `entity+0x3f0` and
`entity+0x330` into the first two dwords, zeroes the next three dwords, and
copies at most eleven bytes from the string at `entity+0x08` into that area.
The writer serializes the resulting count and 20-byte entries to the `.osf`
sidecar when the save-slot path writes that file.

The existing save finding describes the entity type and bit gate. This adds the
inner nonzero test and exact fields captured, without assigning semantic names
to those fields.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| The outer entity path gates `OsfAdd` on type `2` and entity byte bit `0x08` | observed in `FUN_0043bc40` |
| `OsfAdd` requires one of the three observed source fields to be nonzero | observed in the complete body |
| Entry captures two dwords plus a bounded 11-byte string region in 20 bytes | observed |
| Semantic meanings of the two dwords and string | unknown |
| Whether every object passing these gates corresponds to a user-visible completed objective | unknown |
