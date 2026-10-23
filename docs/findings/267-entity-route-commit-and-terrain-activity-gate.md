# Finding 267 — Entity Route Commit and Terrain Activity Gate

Two entity-route helpers now have conservative names tied to their direct
record writes. A third helper has a mechanically clear terrain-eligibility
transition, though the surrounding caller has no assigned Ghidra boundary.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x449430 | EntityTargetCellRouteAndAssociate | Resolves the entity's current and target cells; takes a direct path when they match, otherwise builds and commits a route. On success it marks the path record with `0x11` and stores the target entity at entity `+0x2fc`. |
| 0x449610 | EntityCoordinateRouteAndMark | Uses a coordinate pair as the route target. On success it marks the path record with `0x10` and stores the coordinates at entity `+0x2f4/+0x2f8`. |
| 0x449980 | EntityActivityTerrainEligibilityTransition | Requires entity `+0x3e0` bit `0x40`, checks tile eligibility through 0x432a20, updates activity/entity state, and clears tile flag `0x8000000`. Its caller at 0x43e548 lies in an unassigned code region, so the broader feature-level interpretation is unresolved. |

The route helpers are called from the command-sweep body at 0x404774 and
0x4046d6 respectively. Those call instructions and the destination metadata
writes are direct observations; the command-sweep caller itself has no
Ghidra-assigned procedure. The activity transition is kept separate because
its mechanics are direct but its caller ownership is less certain.

**Evidence.** The decompiles are in
[legorr-sim-terrain-third-pass-2026-10-23.bundle](../evidence/legorr-sim-terrain-third-pass-2026-10-23.bundle):
`ev_179aa3ee106eac37d75127eb7698ab12b43dcc46ab1e6b0509d4baa36bb04f9b`.
Caller evidence for the route helpers is
`ev_651d4211e3f709c4f631641de6b6470aee81f2e759a43f0dd9dc2648aa12af91`
and `ev_50b78d601b639e438d2a5afb4ad76e4b7069339ff9158677dea8459feb024967`.
The terrain-transition caller evidence is
`ev_bd7f06536012e0c0cb4170df7934b6276f49f64c9ada5a502ff164182117dc3d`.
All observations are static analysis of the shipped `reference/LegoRR.exe`
(SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`).
