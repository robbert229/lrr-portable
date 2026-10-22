# 55 — Terrain cell material changes use a ten-entry transition-record pool

*Target: `LegoRR.exe` (SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`), Ghidra 12.1.4 via REA.*

`FUN_0044e6d0` handles a requested terrain-cell material change. It compares the cell's current byte at per-cell record offset `+0x18` with the requested value. When they differ and a count gate for the old value is zero, it scans ten fixed-size records for one whose active bit is clear.

For a free record, the function either creates its backing entry or resets an existing one, copies two words from each of the cell's four current vertices, resolves a surface using the old cell byte, and stores the cell coordinates. It also writes the floating-point constant `0.05` into a record field and sets the record's active bit. If no free record is found, this setup is skipped. The function calls `FUN_0044f230` after this branch in either case.

`FUN_0044f230` bounds-checks the cell coordinates, writes the requested byte to cell offset `+0x18`, resolves and installs the corresponding surface, and—unless cell flag `0x10` is set—rewrites four vertices. The four initial values `{1, 3, 2, 0}` are indexed with a rotation derived from the supplied byte; each selected value is split into its high bit (`value >> 1`) and low bit (`value & 1`) and written into two fields per vertex. Those pairs resemble corner coordinates, but the field semantics are not named in the code.

The direct callers are `FUN_0042f6f0` and `FUN_0044e6d0`; the former is the cell-initialization pass described in finding 07. This shows that material replacement updates the cell's surface and its four-vertex data immediately, with a separate ten-entry record path when its gates allow. It does not establish the visual effect or timing meaning of the `0.05` field, or the rendering meaning of flag `0x10`.

## Evidence

- `ev_a2b1fb60b477a6da4873a81ec0d21746ac2edf2f9127f730d3397a498be42406` — material-change gates, ten-record scan, old-vertex/surface capture, record activation, and unconditional update-helper call.
- `ev_3cf042fcee310b73e1c89fb461a20495dc9d59106080f3f41b2229405a0f36cb` — cell bounds check, byte and surface update, flag-gated four-vertex rewrite, and rotated two-component values.
- `ev_8651f66181c696bf518171be16f3cc7f1b3efeaf39ef05d8fdd3e72dee85b50d` — lookup formula for the surface table indexed by the byte's high/low nibbles.
- `ev_d398dc8003e5d7e05d536536a1af8edca9fd2f48c1a48c7bd06d480f11acaf1b` — resolved direct callers of the cell-update function.

The records are retained in [`legorr-terrain-geometry-expansion.bundle`](../evidence/legorr-terrain-geometry-expansion.bundle). This is static shipped-artifact evidence; the record fields' visual and timing semantics remain unresolved.
