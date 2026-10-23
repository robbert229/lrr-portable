# Finding 239 — Segment Mesh Builder and Scan Runtime

Two additional helpers now have mechanical names grounded in their instruction
and caller behavior.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x470760 | MeshBuildEightSidedSegment | Normalizes the vector between two endpoints, constructs a perpendicular frame, builds eight radial offsets using the octagonal diagonal factor, translates the ring to both endpoints, and passes the resulting vertices to mesh setup helpers. Its caller builds two differently sized and colored segments over the same endpoints; interpreting the pair as a beam effect is plausible but remains an inference. |
| 0x493ef0 | CrtFormattedScanV | Implements the shared formatted-input conversion loop: format whitespace and literals, suppression, widths and length state, integer bases, floating point, character/string and scanset conversions, and output assignment. This is CRT support rather than game-specific logic. |

**Evidence.** Direct decompiles and caller observations are in
[legorr-engine-helpers-expansion-11.bundle](../evidence/legorr-engine-helpers-expansion-11.bundle).
The broader CRT helper audit, including byte-reader and pushback helpers, is in
[legorr-engine-helper-audit-text-io-expansion-10.bundle](../evidence/legorr-engine-helper-audit-text-io-expansion-10.bundle).
The game-specific beam interpretation for `0x470760` remains an inference from
its caller `0x470690`; the function's octagonal segment construction is directly
visible. Analyses used shipped `reference/LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
