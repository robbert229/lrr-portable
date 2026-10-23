# Finding 281 — Advisor, Terrain, and Task Helper Names

These five helpers are named from direct pseudocode behavior. Terrain flag meanings stay numeric where the code does not establish a higher-level concept.

| Address | Name | Observed behavior and uncertainty |
| --- | --- | --- |
| 0x434da0 | AdvisorTooltipAndTooltipSoundConfigLoad | Reads `[ToolTips]` mappings into the tooltip configuration helper and `[ToolTipsSFXs]` mappings into the tooltip sound helper. Called during advisor UI initialization. |
| 0x434a00 | AdvisorTutorialIconImageLoad | Reads `TutorialIcon`, resolves its scene value, loads an image handle, and initializes it. Called during advisor UI initialization. |
| 0x435010 | TerrainCellWorkFlagCircleSet | Visits map-bounded cells inside a radius and sets work flags through a per-cell helper. The specific use of work flag bit 4 is unknown. |
| 0x4332f0 | TerrainCellWorkFlagSet | Bounds-checks a cell and ORs bit 4 into its terrain-cell flags. The semantic purpose of bit 4 is unresolved. |
| 0x439b80 | TaskTargetCellEligibilityCheck | For task kind 1, rejects a nonzero second target coordinate; unless object flag `0x200` is set, it delegates a cell check to `0x438ae0`. The domain meaning of the flag and coordinates is unresolved. |

The corresponding bodies and call paths are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). `0x434da0` and `0x434a00` run from advisor UI initialization (`0x41fb50`); `0x435010` calls `0x4332f0` for each accepted cell; and `0x439b80` is called by task/target eligibility path `0x404fc0`. The tooltip configuration body is also in [legorr-engine-helper-audit-mid-expansion-6.bundle](../evidence/legorr-engine-helper-audit-mid-expansion-6.bundle); terrain flag evidence is in [legorr-sim-terrain-third-pass-2026-10-23.bundle](../evidence/legorr-sim-terrain-third-pass-2026-10-23.bundle). Other body details are recorded in the checked-in full pseudocode corpus. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
