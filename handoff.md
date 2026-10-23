# Handoff: lrr-portable reverse engineering

## Continuing objective

Continue identifying anonymous `FUN_*` routines in `lrr-portable` using the
`reverse-engineer-anything` skill and the retained Ghidra pseudocode/evidence.
For each defensible name, keep `tools/ghidra/CreateEngineFunctions.java`,
`docs/decompiles/engine-region.c`, `docs/coverage/ledger.csv`,
`docs/coverage/README.md`, `README.md`, and `docs/findings/*` aligned.

The user explicitly requested three subagents for extra parallelism in a loop.
For each audit round, delegate disjoint ranges: below `0x430000`,
`0x430000–0x46ffff`, and `0x470000` or above. Integrate only candidates still
anonymous and supported by direct behavior evidence. Do not run tests or modify
`Saves/`. Never merge, squash-merge, or auto-merge a PR.

## Repository and binary

- Working directory: `/var/home/johnrowl/source/lrr-portable`
- Current branch: `codex/reverse-engineering`
- Binary: `reference/LegoRR.exe`
- Binary SHA-256: `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`
- Preserve all existing user changes; do not reset or switch branches.

## Current coverage state

- `docs/coverage/ledger.csv`: 2,564 functions — 2,220 named, 0 anonymous,
  293 ported, 46 runtime/import, 5 anchored.
- The Ghidra target array contains 2,481 unique addresses after the latest batch;
  `engine-region.c` has 2,481 named pseudocode sections. The address sets match
  after normalizing leading zeroes.
- Latest integrated findings are 280–386. Finding 386 names all 106 functions that remained anonymous across the mid and high ranges.
- Full coverage is complete: the ledger has zero anonymous functions.
- `python3 tools/coverage_ledger.py` regenerates the coverage ledger and its
  README. Use `git -c core.whitespace=cr-at-eol diff --check` for whitespace
  validation. Do not run project tests unless the user asks.

## Evidence and limitations

The checked-in `docs/decompiles/full/LEGORR.full.c` is the source for complete
Ghidra pseudocode and caller context. Retained REA bundles live under
`docs/evidence/`; cite exact bundle records when a matching direct analysis is
available. Some routines have only the checked-in pseudocode and callers, so
keep names operation-level and state when no matching direct bundle record was
found. Do not infer unresolved enum, bit, or field meanings.

Recent bundles used include `legorr-engine-helper-audit-low-expansion-6.bundle`,
`legorr-engine-helper-audit-mid-expansion-6.bundle`,
`legorr-engine-helper-audit-render-expansion-9.bundle`,
`legorr-engine-helper-audit-text-io-expansion-9.bundle`,
`legorr-engine-helper-audit-text-io-expansion-10.bundle`,
`legorr-engine-helpers-expansion-17.bundle`,
`legorr-engine-helpers-expansion-28.bundle`,
`legorr-sim-terrain-third-pass-2026-10-23.bundle`,
`legorr-sim-terrain-fourth-pass-2026-10-23.bundle`,
`legorr-deferred-simulation-pass-2-2026-10-23.bundle`, and
`legorr-graphics-mode-surfaces.bundle`.

## Next action

The coverage goal is complete: the ledger has zero anonymous functions, all 2,481 Ghidra targets have matching named pseudocode sections, and the latest finding records the final 106 names. No further audit round is required. Keep the no-tests and no-`Saves/` constraints in force for any future follow-up.
