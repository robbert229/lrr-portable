# 29 — DDiScene teardown: unlink before entity reuse

Continues finding 28's owner-link trace. The entity destructor routes
scene-backed entities through the DDiScene destroy wrapper before putting the
entity record on its own free list. The DDiScene destroy path unlinks scene
interfaces, but the analyzed body does not explicitly clear the owner field
at `object+0x20` on its ordinary pooled-object path.

**Observations from** REA/Ghidra 12.1.4 over `reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-21. Evidence: `docs/evidence/legorr-ddscene-lifecycle.bundle` (8
records).

## 1. Entity removal hands its scene object to DDiScene

`FUN_00437540` is the entity teardown path. For the general scene-backed
branch, when entity `+0x3e8` has bit `0x20000`, it calls
`FUN_00472a40(entity+0x1c)`. After that returns, the entity record is linked
onto the entity free list at `DAT_004de810`.

`FUN_00472a40(object)` is a wrapper for `FUN_00472a50(object, 0)`. The latter
first takes `FUN_00475710(object, 0)`'s null-argument path for every kind
except kind 6. That path queries and updates the paired scene interfaces; in
the observed teardown context this is the unlink step before the destroy
routine releases the object's resources.

## 2. Ordinary pooled-object disposal does not clear `+0x20`

On the ordinary completion path, `FUN_00472a50` releases the kind-specific
resources and interface references, then links the 0x2c-byte object record
onto the DDiScene free list by writing its `+0x28` link and updating
`DAT_0076add0`. Its observed body contains no explicit clear of the owner
field at `+0x20`. Finding 28 showed that `FUN_00472950` clears the record
before reusing it, so a reused record starts with a zero owner field.

There is a distinct kind-3/kind-4 case. If `object+0x24` points to an
auxiliary record and `FUN_00472a50` is called with mode 0, it clears that
auxiliary record's `+0x10` field and returns early after the unlink step.
That branch does not reach the object's normal free-list insertion, and it
also does not explicitly clear `object+0x20` in the observed pseudocode.

## 3. Pool-wide DDiScene cleanup

`FUN_00472800` walks the allocated DDiScene pool blocks and calls
`FUN_00472a50(object, 1)` for each live object, then frees the blocks, resets
`DAT_0076add0`, and clears `DAT_0076cd94` (the scene-hook enable word from
finding 26). This cleanup path retires the records and disables the
registered callback gate after releasing the pool storage.

## What this proves and leaves open

The static path establishes the ordering: the entity's scene object enters
the DDiScene teardown routine before the entity record is returned to its
free list. It also establishes that the ordinary DDiScene free-list path
does not explicitly zero `+0x20`, while allocation zeroes the record before
reuse.

It does not establish that no scene-end callback can run after unlinking. The
available static evidence does not prove whether every detached kind-3/kind-4
object becomes unreachable to all draw paths, or whether a callback can race
the entity's removal during runtime.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| General entity teardown calls `FUN_00472a40(entity+0x1c)` when entity `+0x3e8 & 0x20000` is set, before linking the entity onto its free list | observed (`FUN_00437540`) |
| `FUN_00472a40` dispatches to `FUN_00472a50(object, 0)` | observed |
| The destroy path invokes `FUN_00475710(object, 0)` before ordinary cleanup; interpreted as scene-interface unlinking from its call effects | observed calls; unlink role is inferred from the interface operations |
| Ordinary DDiScene record disposal updates the pool link at `+0x28` and has no explicit `+0x20` clear in the observed body | observed (complete decompiler body) |
| Reallocation clears the owner field before reuse | observed (finding 28, `FUN_00472950`) |
| Kind-3/kind-4 mode-0 auxiliary-record path returns before ordinary pool insertion and does not explicitly clear `+0x20` | observed (`FUN_00472a50`) |
| A scene-end callback cannot run after the unlink or after entity reuse | unknown; static analysis does not prove draw-path reachability or runtime ordering |
