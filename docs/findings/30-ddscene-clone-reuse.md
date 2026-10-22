# 30 — DDiScene ACT/ANIM clones are parked and reused

Continues finding 29's teardown path. Kind-3 (`ANIM`) and kind-4 (`ACT`)
objects with an auxiliary record do not take the ordinary DDiScene free-list
path when destroyed in mode 0. Teardown marks their auxiliary slot inactive;
the clone factory later finds that slot, reattaches the object to a scene, and
draw-steps it before returning it.

**Observations from** REA/Ghidra 12.1.4 over `source/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-21. Evidence: `docs/evidence/legorr-ddscene-reuse.bundle` (5 records),
plus the teardown wrapper in `docs/evidence/legorr-ddscene-lifecycle.bundle`.
The focused Evidence IDs are `ev_d2df670bd0aa34b21d5f9bf8a5abe97c70d690b4b8592e8e564eefff5d9da789`
(`FUN_00472a50`), `ev_5c73cde6dfc0d98ba688ebda86d3d53564b800308c7700457892fcaf2d08d7a3`
(`FUN_00473690`), and `ev_93e9e098d4c87fbb35e563f8ac894b458fc82e1ba0b2a1696d6f7b07856b2484`
(`FUN_00437d00`).

## 1. Teardown parks the object

`FUN_00472a50(object, 0)` first calls `FUN_00475710(object, 0)` for kinds other
than 6. For kind 3 or 4 with a non-null auxiliary record at `object+0x24`, it
writes zero to `aux+0x10` and returns. That return precedes channel/resource
release and the ordinary pool insertion at `DAT_0076add0`.
Evidence: `ev_d2df670bd0aa34b21d5f9bf8a5abe97c70d690b4b8592e8e564eefff5d9da789`;
the paired scene-step call is detailed in finding 29.

This is a dormant-slot transition, not evidence that the object record was
freed. The record retains its other fields, including the owner value at
`+0x20`; the inactive marker is in the auxiliary record.

## 2. Clone creation can reactivate a parked object

`FUN_00473690(source)` handles kind 3/4 sources with auxiliary state. It scans
the auxiliary pointer array (count at `aux+8`, entries at `aux+4`) for a
non-null object whose own auxiliary `+0x10` is zero. When it finds one, it:

1. writes one back to that object's `aux+0x10`;
2. calls `FUN_00475710(reused, FUN_004757a0(source))` to attach it to the
   source's current scene context; and
3. calls `FUN_00475140(reused, 0)` before returning the reused object.

If no inactive object is found, the routine allocates and appends another
object to the auxiliary array. The inactive entry therefore acts as a reuse
slot for a scene-specific clone.
Evidence: `ev_5c73cde6dfc0d98ba688ebda86d3d53564b800308c7700457892fcaf2d08d7a3`.

The general entity creator `FUN_00437d00` uses this factory path, stores its
return value in entity `+0x1c`, then assigns that object the new entity as its
owner. Other direct callers of `FUN_00473690` exist, so that owner assignment
ordering is specific to this entity-creation path.
Evidence: `ev_93e9e098d4c87fbb35e563f8ac894b458fc82e1ba0b2a1696d6f7b07856b2484`.

## What this establishes

The kind-3/4 mode-0 teardown path does not permanently remove every object
from future DDiScene use: a later clone request can select an inactive entry,
reattach it, and run its draw-at-time routine. This explains why the early
return in finding 29 preserves more than an allocator detail.

It does not establish that the parked object remains reachable to ordinary
scene drawing between teardown and reuse, nor that a scene-end callback runs
during the forced time-zero draw. Finding 31 checks that callback boundary.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| Mode-0 destroy of kind 3/4 with auxiliary state writes `aux+0x10=0` and returns before ordinary free-list insertion | observed (`FUN_00472a50`) |
| `FUN_00473690` scans the source auxiliary array for an inactive kind-3/4 object | observed |
| A selected inactive object is marked active, attached to the source scene context, then passed to `FUN_00475140(object, 0)` | observed |
| `FUN_00437d00` stores the returned object on the new entity and sets its owner afterward | observed |
| The parked object is reachable to every draw path before a later clone request | unknown; static references do not establish runtime reachability |
