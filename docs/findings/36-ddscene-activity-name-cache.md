# 36 — Activity-name cache suppresses retries after a miss

`FUN_00447c40` deduplicates activity updates by comparing the requested-name
pointer with a cached pointer. For type-1 wrappers, the updater's result is the OR of the
primary and optional companion action-name setters. Regardless of that result,
the dispatcher caches the request before returning. A later call with the same
name exits at the initial equality check without trying either scene again.

**Observations from** REA/Ghidra 12.1.4 over `source/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-21. Evidence: `docs/evidence/legorr-ddscene-shutdown-reachability.bundle`
(33 records). Focused IDs: `ev_7fec5899952b84c0f471166cd4a7b2ef493cedfd23521aec268f47ce11844eb7`
(`FUN_00447c40`), `ev_38685e504de3c713468ba05dc65eb3ae0011b8c1c04347dd198254e728eeab6b`
(`FUN_0046c3f0`), and `ev_54685b4639c266087fbaea3a5d39cf90fcf38dbcc979cee47a70be03b3ccb700`
(`FUN_00473370`).

## Type-1 request path

`FUN_00447c40` returns immediately when the pointer in `param_1[0xba]` equals
the pointer in `param_1[0xbb]`. For a different pointer and wrapper type 1, it stores the result of
`FUN_0046c3f0(param_1[3], param_1[0xba], param_1[0xb5])` in its local result.
`FUN_0046c3f0` returns the OR of the primary and companion
`FUN_00473370` results; that helper returns zero when the requested action name
is not found on that scene.

After the type-specific update, `FUN_00447c40` clears `param_1[0xb5]`, copies
the requested name to `param_1[0xbb]`, and returns the local result. These
assignments do not depend on whether either scene accepted the name. Therefore,
if neither type-1 scene contains the action, a repeated request using the same
pointer is suppressed before the scene setters run again. Equal string contents
at a different address are not established to take this early return. This describes the
dispatcher state transition; it does not establish whether the caller retries
through a different name or resets the cached field elsewhere.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| Identical current/cached request pointers short-circuit before scene update | observed (`FUN_00447c40`) |
| Type-1 return value ORs primary and companion action-name setter results | observed (`FUN_0046c3f0`) |
| The requested name is cached regardless of the type-1 update result | observed complete body (`FUN_00447c40`) |
| Other code can invalidate/change the cached pointer before the next request | not audited here |
