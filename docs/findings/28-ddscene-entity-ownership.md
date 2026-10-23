# 28 — DDiScene owner links: how scene objects reach their entity

Continues findings 25–27. Finding 26 showed the scene-end callback reading
`object+0x20` to find an entity, then setting `entity+0x3e4` bit `0x10000`.
This finding closes the writer side: the field is a DDiScene object’s owner
link, and the entity-spawn path installs it on scene objects and selected
model-backed scene trees.

**Observations from** REA/Ghidra 12.1.4 over `reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-21. Evidence: `docs/evidence/legorr-ddscene-owner-link.bundle` (20
records).

## 1. The field accessors

`FUN_00473da0(object, owner)` is a one-store setter:

```c
*(undefined4 *)(object + 0x20) = owner;
```

Its paired getter, `FUN_00473db0(object)`, returns that same dword. The scene
object allocator `FUN_00472950` clears the complete 0x2c-byte pooled record
before use, so a newly allocated object's `+0x20` starts at zero. The scene
end hook at `LAB_0043f130` calls the getter and, when it returns an owner,
sets bit `0x10000` in the owner's `+0x3e4` word (finding 26).

## 2. Entity creation installs the owner link

`FUN_00437d00` creates an entity and dispatches setup by entity type. In the
general scene-object branch, it calls `FUN_00473690(param_1)`, stores the
returned scene object in the entity record at `+0x1c`, then calls
`FUN_00473da0(scene_object, entity)`. The value later returned by the getter
is therefore the owning entity record used by the scene-end callback.

The type-specific model paths use wrappers that install the same link:

- For entity type 1, `FUN_00437d00` loads a model description and calls
  `FUN_0046d140(description, entity)`. That helper assigns the entity pointer
  to the scene objects referenced by description `+8`, optional `+0xc`, and
  each non-null entry in the array beginning at `+0x10` with count `+0x40`.
- For entity types 2, 3, and 4, setup calls `FUN_00408380(model, entity)`.
  The wrapper reads a scene object from `model+4` and assigns its owner link.

These paths show that `+0x20` is used to route scene callbacks back to their
entity. They do not establish that every possible DDiScene object is owned by
an entity; the generic factory and menu-wipe objects can have no owner.

## 3. Setter call-site coverage

Ghidra's incoming-reference inventory reports five direct calls to
`FUN_00473da0`: one from the general entity branch in `FUN_00437d00`, three
from the type-1 model-description walk in `FUN_0046d140`, and one from the
type-2/3/4 wrapper `FUN_00408380`. The two wrapper functions are themselves
called by the entity spawner. This bounds the observed direct setter use in
the analyzed executable; indirect or unresolved control flow is outside that
call-site inventory.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| `FUN_00473da0` writes its second argument to `object+0x20`; `FUN_00473db0` reads it | observed (Ghidra pseudocode and assembly) |
| General entity creation stores the scene object at entity `+0x1c` and assigns the entity as owner | observed (`FUN_00437d00`) |
| Type-1 model description propagates an owner to listed scene objects; type-2/3/4 wrapper assigns one owner | observed (`FUN_0046d140`, `FUN_00408380`, and call sites) |
| A non-null owner lets the scene-end callback set entity `+0x3e4` bit `0x10000` | observed (finding 26 hook plus this getter/setter path) |
| Every DDiScene object has an entity owner | unknown; only the listed creation paths are established |
| Runtime timing and behavior of all owner-linked scenes | unknown; this is static analysis, not runtime observation |
