# 79 — Special terrain cells advance through a four-step mutation gate

*Target: `LegoRR.exe` (SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`), Ghidra 12.1.4 via REA.*

`FUN_00430530(level, x, y, mode)` is a terrain-cell mutation routine. It rejects the outer border, cells whose voxel flag word at `+0x08` already has bit `0x08`, and otherwise follows one of two state paths based on the byte at voxel `+0x02`.

When the target voxel byte at `+0x02` is `8` or `10`, the routine clears its
four bytes at `+0x14..+0x17`, then adjusts the leading byte of the four cells in
an adjacent 2×2 corner set. Zero entries become `5`; other entries are
decremented unless already `1`, with `FUN_0044f380` called using one quarter
of the level step value. A short counter at target offset `+0x22` increments.
Before it reaches four, `FUN_00433380` runs and the routine returns. At four,
the target byte at `+0x02` becomes `5`, the four visited leading bytes become
`1`, and the routine recurses on the target.

The code establishes this four-step mutation and completion gate, but it does
not name the voxel-byte values or counter. It does not establish which action
schedules the routine or its in-game appearance. This is static evidence; no
runtime transition was observed.

## Evidence

- `ev_d8d04a9318bc07017b92ffb0f8609b3733303b09962dcd3acdddaf0017eb132e` — `FUN_00430530`: the `+0x02` values 8/10 path, four-step counter, recursive completion, and direct callers.

The records are retained in [`legorr-terrain-mutation-focused-expansion.bundle`](../evidence/legorr-terrain-mutation-focused-expansion.bundle). The evidence is decompiler-derived from the shipped executable; field semantics and runtime effects remain unknown.
