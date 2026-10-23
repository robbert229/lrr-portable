# Finding 266 — Terrain Triangle Selection and Command Gate

Two formerly deferred entity/terrain helpers now have mechanically grounded
names. Their surrounding code includes regions that Ghidra does not assign
function boundaries, so caller evidence is described with that limitation.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x44fab0 | TerrainCellSurfaceTriangleVerticesSelect | Finds the terrain cell for world X/Z, reads its surface vertices, compares the point against a cell boundary using the record's +0x10 float, then copies one of two three-vertex triplets from offsets +0 or +0xc. The exact meaning of the threshold remains unclear. |
| 0x43c3e0 | EntityType26CommandCompletionGate | Requires entity type 2 and an attached record of kind 0x1a, then checks a radius/distance predicate and a second interaction predicate. Success resets the entity action sequence. The second predicate's semantics are unresolved. |

**Caller context.** The only direct call to 0x44fab0 is at 0x44b97c inside an
unregistered callback body. That callback is passed from 0x44bc30 and 0x44bf00;
0x44bf00 is called during the named entity/scene update path at 0x43b270.
Evidence for the direct body and callback chain is in
[legorr-deferred-simulation-pass-2-2026-10-23.bundle](../evidence/legorr-deferred-simulation-pass-2-2026-10-23.bundle):
`ev_329e36aebfb4cd71eabbd40109b0089525092dddb5737b58721aabd778182db`,
`ev_83bcc567ef1a168648587d0db076f33dfe71cfd2b2ce060fcdeb8b16898531de`,
`ev_41c50aaa35f8ff91c30f22dcb3ead365af7c43d8c37defcd91b1458780561728`, and
`ev_3a6860334e480a0d13dc8b740cb83ccc0704a4330ac5c77b075e9ce8b946ad12`.

The direct call to 0x43c3e0 at 0x43de03 is also inside an unregistered code
region, here within the `EntityFlagWordBehaviorEvaluate` body. The narrow
gate behavior is direct, but exact caller ownership is not established. The
decompile and call-path evidence is
`ev_7145e1dfefcde488eea3c2243071958c4dcacd2dcfb094ff40d80b5b3bbea1d6`,
`ev_324759a4650904e3a78ddc332ec267d298afb2729d34f261955153a6b7770385`, and
`ev_6ce3a0b8d4a0d4a195132f950746ba82110d7c4958527287ae2862e808d2f17c`.
All observations use static analysis of the shipped `reference/LegoRR.exe`
(SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`).
