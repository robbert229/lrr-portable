# 88 — A neighbor scan consumes terrain work flags through mutation mode 1

`FUN_00431580` scans adjacent cell coordinates for flag `0x20000`. When it
finds one, it calls `FUN_00430530` with mode `1`, then clears the scanned
flag after the call. The scanner therefore consumes a marked neighbor through
the terrain mutation routine, with the clear ordered after that call. The
flag's purpose and the conditions that establish it are not named here.

**Observations from** REA/Ghidra 12.1.4 over the shipped `reference/LegoRR.exe`
(SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence bundle:
[`legorr-terrain-mutation-focused-expansion.bundle`](../evidence/legorr-terrain-mutation-focused-expansion.bundle).
Focused records: `ev_82dd79d7fd80c4db498abc9580b85507b5d7b16e3150570557e32deec33e7a89`
(`FUN_00431580`) and
`ev_99560db2af9ba1fe846cbf89db7f51241aa0cf4dcfea68590a277a3a8f985fd8`
(`FUN_00430df0`).

The adjacent-cell scanner and mutation call are static observations. The
meaning of the flag, ordering under live scheduling, and resulting appearance
remain unknown.
