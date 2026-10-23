# Full decompile corpus

Complete Ghidra pseudocode for **LegoRR.exe** (2564 local functions).

- `LEGORR.full.c` -- every manifest function in address order, one block per
  function; headings show human aliases and preserve original Ghidra symbols
- `manifest.csv` -- per-function index (address / original symbol / size)
- `LEGORR.supplement.c` -- 11 named entries retained from the legacy
  `../legacy/engine-region.c` whose entry addresses are absent from this manifest;
  these do not count toward coverage

The full export is canonical for complete pseudocode and coverage. The legacy
`engine-region.c` remains available because many overlapping function bodies
differ from the complete export and have not yet been reconciled. Findings may
still cite those variants. Do not delete the legacy file until those differences
have been reviewed and any useful variants migrated.

Regenerate the full export with REA (`open_binary` -> `batch_decompile` ->
`export_evidence_bundle`), then run:

    python3 tools/layout_decompiles.py <bundle.json> docs/decompiles/full
    python3 tools/annotate_decompile_names.py
    python3 tools/coverage_ledger.py

The annotation step changes only function-heading comments. It restores aliases
from the existing `docs/coverage/ledger.csv` while retaining each original
Ghidra symbol. The coverage step then rebuilds the ledger from the full corpus.
Keep `LEGORR.supplement.c` in sync if the legacy corpus changes; it records
entries missing from the full manifest, not alternate versions of shared
functions.
