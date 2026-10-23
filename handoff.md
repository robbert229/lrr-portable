# Handoff: lrr-portable reverse engineering

## Continuing objective

Continue identifying anonymous `FUN_*` routines in `lrr-portable`, using the `reverse-engineer-anything` skill and the newly added `docs/findings/*` as starting evidence, then document defensible names in the Ghidra target script, named pseudocode corpus, ledger, findings, and REA evidence bundles.

## Repository and artifact

- Working directory: `/Users/johnrowl/source/lrr-portable`
- Current branch: `codex/reverse-engineering`
- User moved the reference asset directory from `source` to `reference`. Use `/Users/johnrowl/source/lrr-portable/reference/LegoRR.exe`.
- Binary SHA-256: `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
- User instruction: never merge/squash/auto-merge a PR; leave merge to the human.
- Do not modify `Saves/`. No tests unless asked.

## Latest completed work

Added findings 237–239 and 13 named targets in the latest turn. Current counts were reconciled: 977 unique Ghidra targets, 977 corpus sections, 2564 ledger functions, with 716 named and 1504 anonymous. `git -c core.whitespace=cr-at-eol diff --check` passed. No formal tests were run.

- [`tools/ghidra/CreateEngineFunctions.java`](/Users/johnrowl/source/lrr-portable/tools/ghidra/CreateEngineFunctions.java): latest targets include reward/progress, terrain rendering, simulation, mesh segment, and CRT scan helpers.
- [`docs/findings/237-buffered-stream-and-win32-file-helpers.md`](/Users/johnrowl/source/lrr-portable/docs/findings/237-buffered-stream-and-win32-file-helpers.md)
- [`docs/findings/238-reward-terrain-and-simulation-helpers.md`](/Users/johnrowl/source/lrr-portable/docs/findings/238-reward-terrain-and-simulation-helpers.md)
- [`docs/findings/239-mesh-segment-and-scan-runtime.md`](/Users/johnrowl/source/lrr-portable/docs/findings/239-mesh-segment-and-scan-runtime.md)
- Named pseudocode is appended in [`docs/decompiles/engine-region.c`](/Users/johnrowl/source/lrr-portable/docs/decompiles/engine-region.c); ledger updates are in [`docs/coverage/ledger.csv`](/Users/johnrowl/source/lrr-portable/docs/coverage/ledger.csv).

Strong identifications from the latest sweep:
- `0x405c10` PendingCommandEntityAssignmentTick; action/eligibility flag meanings unresolved.
- `0x41baa0` SimulationPointerActionDispatch; exact action IDs and flags unresolved.
- `0x407d60` BuildingTypeConfigRecordParse; reads BuildingTypes null names/frames, pivots, shapes, and power-level scene.
- `0x433e80` TerrainProjectedCellStateRefresh; projection/state work is clear, bit meanings unresolved.
- `0x45dbc0` TerrainViewTileGeometryBuild, with `0x45e9e0` style resolution and `0x45e940` view-to-screen transform.
- `0x46d380` AttachedModelOrientationUpdate.
- `0x460950` CheckpointBlobTaskProgressRecalculate, `0x460f00` RewardStatsPanelDraw, `0x461070` RewardStatsTextFormat.
- `0x470760` MeshBuildEightSidedSegment; beam-effect interpretation is inferred from its caller.
- `0x493ef0` CrtFormattedScanV.

## Evidence bundles in the working tree

The current working tree has modified README, coverage docs/ledger, corpus, and target script, plus these untracked artifacts/findings. Preserve the work; do not reset or switch branches without checking status first.

- `docs/evidence/legorr-engine-helpers-expansion-10.bundle` — direct reward/terrain/simulation batch and callers (12 records).
- `docs/evidence/legorr-engine-helper-audit-render-expansion-10.bundle`
- `docs/evidence/legorr-engine-helper-audit-simulation-expansion-10.bundle`
- `docs/evidence/legorr-engine-helper-audit-text-io-expansion-10.bundle`
- `docs/evidence/legorr-engine-helpers-expansion-11.bundle` — mesh + CRT direct decompiles and callers (3 records).
- `docs/evidence/legorr-engine-helper-audit-text-io-expansion-9.bundle` supports finding 237.

## Next useful work

Continue from the remaining high-confidence anonymous helpers in the third audit bundles. Candidate follow-ups included the mesh/terrain/scene pipeline and simulation command/path code. The text/CRT sweep found `0x4802d0`, `0x480390`, and `0x480570` as a token-list plus recursive directory scan whose case-insensitive path comparison has no observable effect; purpose is unknown, so avoid assigning a feature-level name without more evidence. Inspect candidate decompiles and callers, then add only names supported by direct behavior. Keep evidence bundles attached to findings and keep README, target script, corpus, and ledger counts aligned.

## Suggested skills

- `reverse-engineer-anything` — continue REA workflows against the shipped executable and preserve evidence.
- `handoff` — use again if another compact continuation note is needed.


