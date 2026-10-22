# Full decompile corpus

Complete Ghidra pseudocode for **LegoRR.exe** (2564 local functions).

- `LEGORR.full.c` -- every function in address order, one block per function
- `manifest.csv` -- per-function index (address / symbol / size)
- SHA-256: `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`

Regenerate with REA (`open_binary` -> `batch_decompile` -> `export_evidence_bundle`), then:

    python3 tools/layout_decompiles.py <bundle.json> <out-dir>
