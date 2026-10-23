# 87 — Other voxel states run a separate neighborhood-flag refresh path

When `FUN_00430530` receives a target whose voxel byte at `+0x02` is not 8 or
10, it first ensures the target and four adjacent cells have nonzero leading
bytes. It updates flag words in an eight-cell neighborhood, sets target flag
`0x08`, calls `FUN_00431450`, clears target flag `0x20`, sets flag `0x400000`,
and runs nearby-cell update passes. If the target's `+0x0c` word has bit
`0x200`, the routine replaces it with `0x20`, submits the coordinates to
`FUN_0044a4b0`, and calls `FUN_0042f6f0` for the target.

**Observations from** REA/Ghidra 12.1.4 over the shipped `reference/LegoRR.exe`
(SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence bundle:
[`legorr-terrain-mutation-focused-expansion.bundle`](../evidence/legorr-terrain-mutation-focused-expansion.bundle).
Focused record: `ev_d8d04a9318bc07017b92ffb0f8609b3733303b09962dcd3acdddaf0017eb132e`
(`FUN_00430530`).

Finding 224 establishes that the `+0x0c & 0x200` branch converts a deferred
predug coordinate into the capped coordinate pool and reinitializes that
cell. The initiating gameplay action for later updates and the exact visible
result remain unknown. This is static shipped-artifact evidence; no runtime
transition was observed.
