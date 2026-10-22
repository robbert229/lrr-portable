# 33 — DDiScene pool shutdown sits on the initializer's failure exit

The DDiScene shutdown routine is reached from the large initialization/config
routine on a failure path. In that routine, initialization creates the scene
pool, registers the scene-end hook, and then proceeds through config/resource
setup. Failures that converge on `LAB_00422818` call `FUN_00472800` and return
zero; the successful path returns one without that call.

**Observations from** REA/Ghidra 12.1.4 over `source/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-21. Evidence: `docs/evidence/legorr-ddscene-shutdown-reachability.bundle`
(33 records). Focused IDs: `ev_9cf0ef11e49abe4974600709d27cddc78521eee9c94f6ba65c1a2b8c2731b1f4`
(`FUN_0041fb50`), `ev_92523a5680a81ede5d19bd4640f6cec03c2d325e4297d24cf0a2debc060708f6`
(`FUN_00472800` incoming references), and
`ev_d61d154897d86771d69fca31db073c4234a2f0f2864004da5333fe91551c4c7d`
(`FUN_00472800` resolved caller inventory).

## Observed path

`FUN_0041fb50` first obtains the scene-system handle from
`FUN_00472710(DAT_005560c4)`. If that succeeds, it registers the scene-end
callback with `FUN_00472900(&LAB_0043f130, 0)`. Later config and resource
failures branch to `LAB_00422818`, whose body calls `FUN_00472800()` and
returns zero. The successful path reaches `FUN_00464fa0(7,1)` and returns one
without calling pool shutdown.

The incoming-reference inventory identifies the shutdown call at `0x422818`
inside `FUN_0041fb50`. It also records a second direct call at `0x42503f`.
Ghidra decodes that instruction as `CALL 0x00472800`, but assigns no containing
procedure to the source address. The caller inventory reports one omitted call
reference. This finding therefore describes the identified initializer path;
it does not classify every shutdown call in the executable.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| The identified initializer registers the scene-end hook before later config/resource work | observed (`FUN_0041fb50`) |
| Its `LAB_00422818` failure exit calls pool shutdown and returns zero | observed (`FUN_0041fb50`) |
| Its successful return path does not call pool shutdown | observed complete decompile |
| The direct call at `0x42503f` belongs to a reachable procedure/path | unknown; Ghidra reports no containing procedure |
| Every executable call to pool shutdown is failure cleanup | unknown; caller coverage is partial |
