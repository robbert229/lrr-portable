# Finding 224 — Predug Cell Values Feed a Capped Coordinate Pool

The predug layer distinguishes two cell values that feed a shared coordinate
pool. A value whose bias-adjusted code is 3 sets cell `+0x0c` bit `0x20` and
immediately appends its coordinates to the pool. A value whose adjusted code
is 4 sets bit `0x200` instead. When `FUN_00430530` later updates a cell with
that bit, it clears `0x200`, sets `0x20`, appends the cell coordinates to the
same pool, and reruns `FUN_0042f6f0` for the cell. The cell initializer maps
bit `0x20` to tile selector `0x30`.

The pool holds at most 20 coordinates and is cleared during the game-state
initialization path. A timer-driven call in `FUN_004261c0` invokes
`FUN_0043ad50` when the level's configured count is not `0x14`; that routine
chooses a random coordinate from the pool and passes it to an entity creation
routine. Other consumers can select the nearest pool coordinate. This
connects the predug values to a later spawn-location system, but the static
trace alone does not identify the created entity's in-game name or establish
which events cause each deferred `0x200` cell to be updated.

**Evidence.** In shipped `reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
`FUN_0042bd20` (`0x42bd20`) loads the predug layer and distinguishes
adjusted codes 3 and 4. The code-3 branch writes `+0x0c | 0x20` and calls
`FUN_0044a4b0` (`0x44a4b0`); the code-4 branch writes `+0x0c | 0x200`.
`FUN_00430530` (`0x430530`) consumes `0x200` as described and calls both
`FUN_0044a4b0` and `FUN_0042f6f0`. `FUN_0042f6f0` (`0x42f6f0`) chooses
tile selector `0x30` when `+0x0c & 0x20` is set. `FUN_0044a4b0` appends
coordinates only while the count is below `0x14`; `FUN_004372a0`
(`0x4372a0`) clears the pool. `FUN_004261c0` (`0x4261c0`) is the timer
caller of `FUN_0043ad50` (`0x43ad50`), which selects a random pool coordinate
and passes it to `FUN_00438610`. These functions are decompiled in
`docs/decompiles/full/LEGORR.full.c`.
