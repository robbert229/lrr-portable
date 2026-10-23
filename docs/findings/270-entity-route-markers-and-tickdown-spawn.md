# Finding 270 — Entity Route Markers and TickDown Spawn

Three more command-sweep route issuers and one record-kind-gated marker spawn
have mechanically grounded names. The command-sweep call instructions are in
unassigned code regions, so their exact higher-level owner remains open.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x449520 | EntityTargetCellRouteMarker14 | Resolves current and target entity cells, dispatches directly when equal, otherwise pathfinds and commits the route, frees route buffers, and writes path-slot marker `0x14` on success. Called at 0x4046ea. |
| 0x449240 | EntityWalkableTargetCellRouteIssue | Requires the walkable-cell predicate and an approach-weight gate at `3.0f`; on success commits a route, writes path-slot marker `2`, and stores target-cell coordinates as floats at entity `+0x2f4/+0x2f8`. Called at 0x404601. |
| 0x449010 | EntityKind6RecordApproachRouteIssue | Gates on entity type 2, entity `+0xf8` bit `0x400`, and a kind-6 record with its high bit set; checks a `3.0f` approach threshold, then writes marker `0x12` and coordinates at `+0xbd/+0xbe`. Called at 0x4046c2. |
| 0x4489a0 | EntityKind6TickDownMarkerSpawn | For a kind-6 record, reads the source pose/cell, creates an entity of kind `0xd` there, marks the cell, selects the `TickDown` scene activity, and sets entity `+0x3e8` bit `0x10000`. A wrapper gates the call on kind 6; broader caller ownership is unresolved. |

The three route call sites lie in the command-sweep area described in finding
256, but have no assigned Ghidra caller procedure. The route markers and
coordinate writes are directly observed. The TickDown helper's mechanics are
direct, while its feature-level caller is less certain.

**Evidence.** All target pseudocode and the relevant caller instructions are
in [legorr-sim-terrain-fourth-pass-2026-10-23.bundle](../evidence/legorr-sim-terrain-fourth-pass-2026-10-23.bundle):
`ev_2dbca758cb4d3b36b6bad8b733fb0a5bf433a5c96726bca09f35566a45581aeb`
(route marker `0x14`),
`ev_eb6d208b94a54a1741fae57018ba1d65b3d3d8d121761b53b5c692c1809ea78f`
(its call site),
`ev_ce634dcad51a3c94a54f3656bd8fc917598ad032774a564663a252f8e064e249`
(walkable route),
`ev_23a16639ec1abf7d4bf188b8aa41119722a050d0665ba46ae967ed68ee80efa4`
(its call site),
`ev_fcc161f252a0f4decbe4abd05c86b5e3c9f744d1aa4ed48a4eeea24d05b49a39`
(kind-6 route),
`ev_5487bdf80abd03439f6852b6ecd25cff7452b870cfabae665dbd6698864170b2`
(its call site), and
`ev_ed81dae9f5116aad799e15f1e9f659616f9730157b7d043aa51f28e141cef779`
(TickDown spawn). All observations are static analysis of shipped
`reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`).
