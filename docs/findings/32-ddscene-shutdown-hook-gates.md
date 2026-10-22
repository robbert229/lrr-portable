# 32 — DDiScene shutdown clears the sample-hook gate, not the scene-end hook

Findings 26 and 29 mapped the two animation callback slots and pool shutdown.
Combining their dispatch checks with a fresh shutdown decompile shows that the
global `DAT_0076cd94` reset gates only the per-frame sample hook. The scene-end
hook has a separate pointer and time check, with no read of that gate in
`FUN_00475140`.

**Observations from** REA/Ghidra 12.1.4 over `source/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-21. Fresh evidence: `docs/evidence/legorr-ddscene-reuse.bundle`
(`FUN_00472800`, `FUN_00475140`); registration and callback-slot evidence is in
`docs/evidence/legorr-ddscene-hooks.bundle`.
Fresh Evidence IDs: `ev_98458be3e26d38b582e13afe99ad848ee0d26081100f1a73b94d638dd81f78bc`
(`FUN_00472800`) and `ev_7f7ad309108f21dd6c5ba9ede20a2eab387ea8cfaae9e753f25ba18a60b74625`
(`FUN_00475140`).

## Observed shutdown and dispatch checks

`FUN_00472800` walks the DDiScene pool blocks, destroys each live record with
`FUN_00472a50(object, 1)`, frees the blocks, sets the free-list head
`DAT_0076add0` to zero, and writes zero to `DAT_0076cd94`. Its observed body
does not clear the callback function/user slots at `DAT_0076cd78/7c` or
`DAT_0076cd80/84`.

`FUN_00475140` checks `DAT_0076cd78 != 0` and `DAT_0076cd94 & 0x40` before
calling the per-frame hook. The scene-end path checks `DAT_0076cd80 != 0` and
its frame/time conditions, but does not test `DAT_0076cd94`.

Therefore clearing `DAT_0076cd94` during shutdown disables the per-frame
sample callback path but is not itself a scene-end callback gate. Shutdown
also destroys the pooled objects and frees their blocks; this static trace
does not establish whether any external reference can still invoke
`FUN_00475140` after that storage is retired.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| Pool shutdown destroys live records, frees blocks, resets the free-list head, and clears `DAT_0076cd94` | observed (`FUN_00472800`) |
| Shutdown body does not write the two callback function/user slots | observed complete body |
| Per-frame hook requires callback pointer plus `DAT_0076cd94 & 0x40` | observed (`FUN_00475140`) |
| Scene-end hook requires callback pointer and its end-frame/time predicate but does not read `DAT_0076cd94` | observed (`FUN_00475140`) |
| A scene-end callback can actually run after pool shutdown | unknown; no surviving-reference or runtime evidence is established here |
