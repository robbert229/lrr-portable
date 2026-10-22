# 31 — Cached-clone draw precedes the new entity owner assignment

Finding 30 showed that `FUN_00473690` reactivates a dormant kind-3/4 clone and
calls `FUN_00475140(clone, 0)` before returning it. The general entity creator
assigns the new entity to the returned object's `+0x20` owner field only after
that return. The draw-step's scene-end threshold bounds what can happen during
this brief ordering window.

**Observations from** REA/Ghidra 12.1.4 over `source/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-21. Evidence: `docs/evidence/legorr-ddscene-reuse.bundle` (5 records);
callback implementation details cross-reference findings 26 and 28.
Focused Evidence IDs: `ev_5c73cde6dfc0d98ba688ebda86d3d53564b800308c7700457892fcaf2d08d7a3`
(`FUN_00473690`), `ev_93e9e098d4c87fbb35e563f8ac894b458fc82e1ba0b2a1696d6f7b07856b2484`
(`FUN_00437d00`), `ev_7f7ad309108f21dd6c5ba9ede20a2eab387ea8cfaae9e753f25ba18a60b74625`
(`FUN_00475140`), and `ev_d2df670bd0aa34b21d5f9bf8a5abe97c70d690b4b8592e8e564eefff5d9da789`
(`FUN_00472a50`). The owner setter is `ev_1421e13191d7ffe216de29c1b3830afa01cb0024a7fb8b9e3bf38b8e939e0b59`
in the owner-link bundle.

## 1. Ordering on the entity-creation path

In the general branch of `FUN_00437d00`, the sequence is:

```c
scene = FUN_00473690(source);
entity->scene = scene;             // entity +0x1c
FUN_00473da0(scene, entity);       // scene +0x20 = new entity
```

On the cached-clone path inside `FUN_00473690`, `FUN_00475140(clone, 0)` runs
before the factory returns. Since the mode-0 teardown path from finding 30
does not clear `clone+0x20`, that field can still hold its previous owner
during this synchronous draw-step. The evidence establishes the ordering and
retained field; it does not by itself establish that the old entity record has
already been reused at this exact point.

## 2. The forced draw-step cannot deliver the scene-end hook for positive ends

`FUN_00475140` invokes the scene-end callback only when its end-frame value is
nonzero, the scene length is below that value, and the end-frame value is
less than or equal to the requested time. The cached-clone call supplies time
zero. For a positive end-frame value, `end <= 0` is false, so this forced
reactivation draw cannot call the scene-end hook for that animation.

The separate per-frame callback has its own flag and gate and may run during
this draw-step. Finding 26 shows that callback receives the frame name and
object; its traced stub hashes the frame name for the SFX bank, without reading
the object's owner field. This distinction limits the owner-ordering concern
to later scene-end dispatches, after the entity factory has assigned the new
owner, on this particular creation path. Evidence:
`ev_51afddaa801032f6f744909ff1d2c0e5f0889424e6bbf4aeb5d7226b8a2f532f` in the
hooks bundle.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| Cached-clone reactivation invokes `FUN_00475140(clone, 0)` before returning | observed (`FUN_00473690`) |
| General entity creation assigns the new owner after `FUN_00473690` returns | observed (`FUN_00437d00`, `FUN_00473da0`) |
| Mode-0 cached teardown does not clear the clone's `+0x20` owner field | observed complete body (`FUN_00472a50`) |
| A positive animation end-frame cannot satisfy the scene-end callback's `end <= time` check when time is zero | derived from `FUN_00475140` |
| Other `FUN_00473690` callers assign a valid owner before a later scene-end callback | unknown; this finding does not audit each caller's lifecycle |
