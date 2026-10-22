# 84 — Entity teardown dispatches cleanup by the stored type

After common entity cleanup, `FUN_00437540` branches on the entity's first
dword and passes a different in-record pointer slot to a type-specific cleanup
routine for raw types 1 through 4. The type-3 branch also performs an extra
operation when the entity stats word contains bit `0x200000`. These paths
describe the destructor's dispatch and pointer flow; they do not identify the
payload types or prove exclusive ownership.

**Observations from** REA/Ghidra 12.1.4 over `source/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence: `docs/evidence/legorr-entity-destruction-chain-and-payload-expansion.bundle`.

## Type dispatch in the entity destructor

After the shared cleanup calls, the destructor branches on `*entity` and
executes these operations in order (`ev_0238a063832421cd8d9f5cc9a49104ddbcb1863f5ed3a1b50e8164ade69bf119`):

| Stored type | Payload slot and cleanup sequence |
| --- | --- |
| `1` | Pass `entity+0x0c` to `FUN_0046ce10`, then pass that same pointer to `FUN_0048db80` (`ev_a768eb2be89a74c5154b2435c6557c634f6daeac8d75987b9e9c7ed46a73c78c`). |
| `2` | Pass `entity+0x10` to `FUN_00406ec0`, then free that pointer through `FUN_0048db80` (`ev_35b471900120a616c77de4b2e0225746e6f418945416fc88e261b99e9ed90544`). |
| `3` | If `FUN_00469e60(entity) & 0x200000` is nonzero, first call `FUN_0044be30(entity)`; then pass `entity+0x14` to `FUN_00406ec0` and free it through `FUN_0048db80` (`ev_5c9f40a63ed569247ca67e597f3c7dfbeb27e1cf2a937e922f2e0a871ae3fc60`, `ev_35b471900120a616c77de4b2e0225746e6f418945416fc88e261b99e9ed90544`). |
| `4` | Pass `entity+0x18` to `FUN_00408940`, then free that pointer through `FUN_0048db80` (`ev_51d9cf752c2485173e172324cfeb8bd5b2a24cd1c637d629a743b3619abe8be0`). |

For type 2/3, `FUN_00406ec0` itself releases three subslots, conditionally
frees five additional pointers when payload byte `+0x70` bit 0 is set, and
passes payload `+0x04` to `FUN_00472a40`. For type 4, `FUN_00408940` calls
`FUN_004089a0(payload, -1)`, conditionally disposes payload `+0x7c`, then
passes payload `+0x04` to `FUN_00472a40`. This nested cleanup is visible in
the helper bodies; no semantic names are inferred for these slots
(`ev_35b471900120a616c77de4b2e0225746e6f418945416fc88e261b99e9ed90544`,
`ev_51d9cf752c2485173e172324cfeb8bd5b2a24cd1c637d629a743b3619abe8be0`).

The raw entity record is linked to `DAT_004de810` only after the type switch
and its pointer cleanup calls. This is an observed order in this destructor,
not proof that all type-specific subobjects were allocated by the same path.

## Boundary

Entity types are kept numeric because this trace does not establish their
human-facing names. The function has multiple resolved callers, and Ghidra's
caller inventory omits one incoming source; this finding does not claim which
callers always perform teardown. Calling `FUN_0048db80` on the selected field
shows the cleanup action here, not where the pointer was allocated or whether
another object shares it. The type-3 gated helper's broader semantic effect
also remains unknown.
