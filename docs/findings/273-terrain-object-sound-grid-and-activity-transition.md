# Finding 273 — Terrain Object Sound Grid and Activity Transition

Two helpers expose a terrain-object sound table populated from configuration
and queried by cell. A separate entity helper performs a mechanically clear
activity/flag transition, though its caller is in an unassigned region.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x44acc0 | TerrainObjectTypeSfxGridConfigLoad | Clears a 300-entry table, parses `ObjTtSFXs` grid coordinates and sound aliases, and stores resolved handles at `DAT_004de814[y + x*15]`. The dimensions are initialized as 20 by 15. Called from the startup initialization area at 0x421317. |
| 0x44ad80 | TerrainObjectTypeSfxAtCellGet | Converts an input position to cell coordinates through 0x439930 and returns the corresponding table entry. Called from pointer/cell interaction at 0x425d90 and the entity-description builder at 0x428020. |
| 0x449a70 | EntityActivity23StateTransition | For type-2 entities sets `+0xf9` bit `0x800`; when `+0xf8` bit `0x400` is clear, sets `+0x3e0` bit `0x40000000`, requests activity `0x23`, dispatches the scene activity update, and calls 0x403f30. The observed caller at 0x41f628 has no assigned Ghidra procedure. |

The sound-grid loader and getter form a direct producer/consumer pair. Caller
contexts connect lookup to terrain interaction and entity text/SFX. The
activity helper's mechanics are direct; no feature-level caller name is
claimed.

**Evidence.** The target decompiles and caller records are in
[legorr-sim-terrain-fifth-pass-2026-10-23.bundle](../evidence/legorr-sim-terrain-fifth-pass-2026-10-23.bundle):
`ev_f1102fa8b360a73d675f13b023bb71eda0411b3a93ce741d0ae695df37cbdc06`
(grid loader),
`ev_c580f8e935f8a3d398b8d80d406791a444967f077b76d6a65b7e11dad402475a`
(loader caller),
`ev_62d7a6bd50c88d9899e9178ad0bdca60c15ab29637389ae1ec05e191b219e682`
(cell getter),
`ev_44fb21f3d3fd4f59907dfd79be2a8d5d706e5eeb193372f4bd6f48c630448dcd`
(getter call site),
`ev_81c95d5884c7bd235326622b47037ed09540c19cdb5655a71bdacd9831a06ba8`
(entity-description caller),
`ev_559cac176ca98a5cec9de8cbcb506950ef66cd53c7daf287571684ec6117cc12`
(activity transition), and
`ev_5b52b8e22f260c0f0b555a5f8b96157ba088d7cf52717bb3f2c14872113225fe`
(activity call site). All observations are static analysis of shipped
`reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`).
