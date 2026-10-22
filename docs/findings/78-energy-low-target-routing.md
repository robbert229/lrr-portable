# 78 — Energy decay clamps the per-entity resource field using type stats

*Target: `LegoRR.exe` (SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`), Ghidra 12.1.4 via REA.*

The `Stats` loader `FUN_004667e0` reads `EnergyDecayRate` and stores its parsed float at offset `+0x84` in the per-type/per-level statistics record. The entity update `FUN_0043f7d0` reaches that value through `FUN_00469de0`. For entity kinds other than 5 and 10, a positive entity field at `+0x344` is reduced by `EnergyDecayRate × 0.04 × elapsed`; the update skips this subtraction when the entity's `+0x3e8` high bit is set. The field is clamped to zero when nonpositive, then capped at the entity's `+0x340` field. These offsets are kept literal because the binary does not name the instance fields.

This finding is limited to the update's energy-decay arithmetic and its guards. It does not assign semantic names to the `+0x344` or `+0x340` fields or establish the user-visible effect. No runtime behavior was observed.

## Evidence

- `ev_21260f0b066b53f88a4cfa2a5c5015867936133cda3196945aea47ba7f0910d6` — stats parsing, including `EnergyDecayRate` and its per-record `+0x84` store.
- `ev_bb9901580051455453170ce2dcc9ee4c4bfa01013424d91011afe8d18a967d59` — entity resource decay arithmetic and the adjacent low-resource branch.
- `ev_b15723b77fcfe55e18d435216589a5bea453f5a0818477df9d890ad3e85ba6d7` — `FUN_00469de0` reads the per-entity stats pointer's `+0x84` float.

The records are retained in [`legorr-energy-low-target-routing.bundle`](../evidence/legorr-energy-low-target-routing.bundle). This is static shipped-artifact evidence; the instance-field meanings and user-visible effect remain unresolved.
