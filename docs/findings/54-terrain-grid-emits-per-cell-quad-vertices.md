# 54 — Terrain initialization emits one four-vertex buffer entry per map cell

*Target: `LegoRR.exe` (SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`), Ghidra 12.1.4 via REA.*

Finding 07 documents the terrain's `0x28`-byte per-cell records. The constructor `FUN_0044e0c0` creates an object stored in `param_1[9]`, allocates a separate `width * height * 0x28` table at `param_1[10]`, and initializes each cell's geometry from the map grid.

`FUN_0044e4d0` walks rows and columns and constructs four vertices for each cell. For each vertex it computes horizontal coordinates from the cell coordinates, a corner offset, the terrain scale, and an origin offset. It reads the height byte at `cell_record + 0x1a` and multiplies it by a terrain scale before storing the resulting vertical coordinate. It then calls `FUN_00474d30` with the row-major cell index, start vertex zero, and count four.

`FUN_00474d30` selects between two buffer-update paths. If the nested backing pointer exists, it calls `FUN_004823e0`, which copies per-vertex words from the temporary records into a cell buffer at `cell_index * 0x70`, with destination vertices spaced `0x20` bytes apart. Otherwise it resolves the backing object and invokes its method at vtable offset `+0x3c` with the same cell index, vertex start, count, and source record. The helper's copied fields include two three-word groups and two additional words; the remaining source-record word is not copied by this path.

This establishes per-cell quad construction and buffer submission during terrain creation. The code does not establish the graphics API's primitive interpretation, the semantic names of every copied vertex field, or how the renderer later consumes this buffer.

## Evidence

- `ev_15e789ce5adc6c0da67c030ea033e152a559b78e1d07cf03af24b7bebc0acd2b` — terrain constructor: grid-derived dimensions, per-cell allocation, and calls to the terrain geometry helper.
- `ev_bfc70529e3ec26c29b28e12f3f87d7852e9a7517666406566055adfbb95f7a11` — four per-cell vertices, coordinate calculation, height-byte read, and geometry-buffer update call.
- `ev_fb26a9d04f06be90054eb2fbf2ead5ea66e4ac99d2713ef75b715425e337ccc5` — buffer-update dispatch through the backing pointer or object method.
- `ev_0c1c7c65949f675daa142529883702c79b05a62e574bb1f02411d5b1c5181bf5` — indexed destination stride and per-vertex field-copy layout.

The records are retained in [`legorr-terrain-geometry-expansion.bundle`](../evidence/legorr-terrain-geometry-expansion.bundle). This is static shipped-artifact evidence; no rendering execution was observed.
