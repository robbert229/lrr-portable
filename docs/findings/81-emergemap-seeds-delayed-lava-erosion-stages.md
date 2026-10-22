# 81 — EmergeMap seeds delayed erosion cells in a bounded pending table

*Target: `LegoRR.exe` (SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`), Ghidra 12.1.4 via REA.*

The `EmergeMap` loader `FUN_0042bf40` writes a derived byte to each voxel at `+0x06`. For each nonzero even source value, it also chooses a randomized phase from 0 through 3 and calls `FUN_0040ea10`. That helper records the cell coordinates, writes the phase to voxel byte `+0x07`, derives a timer from the value at `+0x06` and the configured erosion interval, and registers the cell in the pending-coordinate table. The table accepts at most 2,000 entries and marks registered cells with voxel flag bit `0x80000000` to prevent duplicate registration.

Level loading supplies the trigger, erosion, and lock intervals through
`ErodeTriggerTime`, `ErodeErodeTime`, and `ErodeLockTime`. This finding covers
the loader-side queue seeding only; finding 90 documents the timer-driven
consumer path. The voxel-byte meanings and in-game cadence remain unknown.
This is static shipped-artifact evidence; no runtime transition was observed.

## Evidence

- `ev_36650f13e8dca7628d2530cba509f637b7864a97089d99b1b67b906037ad0415` — `FUN_0042bf40`: EmergeMap value transform and per-cell queue registration for nonzero even values.
- `ev_5c3624618249fbc447537e4615d49ff3e32035ad17a5968583747d7e1f14779f` — `FUN_0040ea10`: phase initialization, voxel `+0x07` write, derived timer, active-stage registration, and pending-cell registration.
- `ev_beba0e5c5382568f60186ceb53e0254058f7bad49341fcd728d5f105e63b6020` — `FUN_0040ee50`: 2,000-entry pending-coordinate bound, duplicate flag, and release path.
- `ev_a27cc2cb66787dcd36dd8af2f078036b9a5bdd28e80efc8f2fc5b33e1e667009` — `FUN_00429890`: reads `ErodeTriggerTime`, `ErodeErodeTime`, and `ErodeLockTime`, then initializes the erosion timers.

The records are retained in [`legorr-lava-erosion-focused-expansion.bundle`](../evidence/legorr-lava-erosion-focused-expansion.bundle). This is decompiler-derived static evidence; runtime cadence and visible effects remain unknown.
