# 34 — Failed initialization leaves the scene-end hook registered

The initializer installs the scene-end callback before later config/resource
work can fail. Its failure exit destroys the DDiScene pool, but the shutdown
body does not clear the scene-end callback pointer or user-data slot. This
extends finding 32's distinction between the sample-hook gate and scene-end
dispatch to a concrete partial-initialization path.

**Observations from** REA/Ghidra 12.1.4 over `reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-21. Evidence: `docs/evidence/legorr-ddscene-shutdown-reachability.bundle`
(`FUN_0041fb50`, `FUN_00472800`) and
`docs/evidence/legorr-ddscene-hooks.bundle` (`FUN_00472900`, `FUN_00475140`).
Focused IDs: `ev_9cf0ef11e49abe4974600709d27cddc78521eee9c94f6ba65c1a2b8c2731b1f4`
(`FUN_0041fb50`), `ev_9ede83824ce256e65897184804ea58e962962f15b181a71e77c729dd34a9ee79`
(`FUN_00472900`), `ev_98458be3e26d38b582e13afe99ad848ee0d26081100f1a73b94d638dd81f78bc`
(`FUN_00472800`), and `ev_7f7ad309108f21dd6c5ba9ede20a2eab387ea8cfaae9e753f25ba18a60b74625`
(`FUN_00475140`).

## Registration/cleanup order

After `FUN_00472710` creates the scene system, `FUN_0041fb50` calls
`FUN_00472900(&LAB_0043f130, 0)`. The registrar writes the callback to
`DAT_0076cd80` and the user value to `DAT_0076cd84`. A later initialization
failure reaches `LAB_00422818` and calls `FUN_00472800`.

The complete shutdown body destroys pooled records, frees the pool blocks,
resets the free-list head, and writes zero to `DAT_0076cd94`. It does not write
`DAT_0076cd80` or `DAT_0076cd84`. The scene-end path in `FUN_00475140` checks
the callback pointer and its time/frame predicates, but does not test
`DAT_0076cd94` (finding 32).

This establishes that the failed-init path can leave the global scene-end
callback registered after pool teardown. It does not establish that another
draw step occurs after the failed initializer returns, or that a stale scene
object remains reachable to trigger the callback.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| The scene-end hook pointer/user pair is installed before later initialization work | observed (`FUN_0041fb50`, `FUN_00472900`) |
| The identified failure path calls pool shutdown | observed (`FUN_0041fb50`) |
| Pool shutdown does not clear `DAT_0076cd80/84` | observed complete body (`FUN_00472800`) |
| Scene-end dispatch ignores the sample-hook gate `DAT_0076cd94` | observed (`FUN_00475140`; finding 32) |
| A scene-end callback can run after this failed-init cleanup | unknown; no post-failure runtime or surviving-reference evidence |
