# 50 — Tile texture refresh replaces a surface before walking its slots

*Target: `LegoRR.exe` (SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`), Ghidra 12.1.4 via REA.*

Finding 13 identified `FUN_00462e60` as the update helper used by the world texture-set manager. Its body gives a more precise refresh contract:

1. If the global current surface `DAT_00552194` is nonnull, release it through `FUN_00474710`.
2. Resolve the replacement using `FUN_004744f0(param_1, 1, 0, 0)` and store the result back in `DAT_00552194`.
3. If resolution fails, return `0` without walking the update slots.
4. Otherwise query the slot count from `FUN_00474a20(DAT_00552180)` and call `FUN_00474da0(DAT_00552180, i, DAT_00552194)` for each `i` in `[0, count)`, then return `1`.

`FUN_00474710` releases the two surface references held in its record (at offsets `0` and `+4`), repeating each `Release` call until it returns zero. `FUN_00474da0` has two observed update routes: when a nested pointer at `+0xc` exists it delegates to `FUN_004830c0`; otherwise it obtains a surface through `FUN_00474c00` and invokes a vtable method at `+0x54` with the slot index and resolved surface.

`FUN_00462e60` has one resolved caller, `FUN_0042bb60`, the texture-set manager, itself called from `FUN_0042b510`. The code proves indexed slot refresh after successful surface resolution. It does not establish the visible meaning of the slot indices or whether the alternate update route represents a particular tile animation mode.

## Evidence

- `ev_3fe0280b61453fe7ec5fc0f11e35a9c7536fcab816ad7435a52f89c10e371d73` — refresh order, return branches, and unique caller.
- `ev_7eaf82510d697dd8953400c361165cdda642ed77f233e0ccde80cd0454a438f7` — release behavior for the two held surface references.
- `ev_e95b969556a7217876e98aa937da56ea052ac8a5043eaf3ff57fe0667d597b25` — update-slot count source.
- `ev_72c950d1bb47fea8b9da20d9e2b20cc9759ef14af3c25cfc07ea9cbbf2967822` — per-slot alternate update paths.
- `ev_f23b95a2d48b3ef7614457b96bfeefb39c92cfb79e07826a6dfd372949792883` — texture-set manager callsite.

The records are retained in [`legorr-engine-expansion.bundle`](../evidence/legorr-engine-expansion.bundle). This extends the loader/cache map in finding 13 with refresh ordering and explicitly leaves slot semantics open.
