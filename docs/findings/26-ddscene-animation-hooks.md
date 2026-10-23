# 26 — The DDiScene animation hooks: action-entry SFX sample + scene-end entity flag

Continues finding 25's open ledger. The wipe (finding 24) and every LWS/entity
animation steps through `FUN_00475140`, and that draw step doesn't just render —
it can fire **two registered callbacks** the game wires up at boot:

- `DAT_0076cd78/7c` — the **sample hook** (animation "sample" sound), and
- `DAT_0076cd80/84` — the **scene-end hook** (entity "animation done" flag).

This finding decodes both callbacks, names their registrars, and closes the
loop to the SFX bank (finding 14) and the entity flag-word evaluator (finding
10).

**Observations from** REA/Ghidra 12.1.4 over `source/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-21. Evidence: `docs/evidence/legorr-ddscene-hooks.bundle` (46 records).

## 1. The draw step that fires them (`FUN_00475140`)

The kind-4 (LWS-ACT) path of `FUN_00475140`:

```c
if (kind(obj) == 4) {
    fVar1 = FUN_00475e40(obj, **obj+0xc, 0);            // first frame record
    if (fVar1 && (rec = FUN_00476160(fVar1), obj[5] & 2)) {   // flag bit 1
        name = FUN_004761e0(fVar1);                     // frame record +0x18
        if (name && DAT_0076cd78 && (DAT_0076cd94 & 0x40))
            (*DAT_0076cd78)(name, obj, DAT_0076cd7c);   // sample hook
        obj[5] &= ~2;                                   // consume pending action-entry bit
    }
}
...
if (rec) {
    uVar4 = obj[5] & 8;
    vl = FUN_00475390(obj);                             // scene length
    n   = FUN_00476180(rec);
    ...
    if (uVar4 == 0)
        end = FUN_00476200(rec);                        // last-frame count
        if ((n != 1) && (FUN_004897e0(rec, t, 0), DAT_0076cd80) &&
            (end != 0.0) && (vl < end && end <= t))     // time past last frame
            (*DAT_0076cd80)(obj, DAT_0076cd84);         // ---------------- scene-end hook
}
```

So on the **next qualifying kind-4 step after action selection** (pending flag
bit `2`) it may call `DAT_0076cd78(frame_name, object, user)`, and **at scene
end** (accumulated time past the object's last frame) it calls
`DAT_0076cd80(object, user)`.

## 2. The sample hook = animation SFX sample (`LAB_00464c20`, registered by the SFX-bank init)

`FUN_00464740` builds the game's **45-slot SFX bank** at `DAT_00502a20`
(`FUN_00477670(name,0,1)` hashes each `SFX_*` string into a slot) — then, at the
end, registers the sample hook:

```c
void FUN_00464740(void) {                    // SFX bank init
    DAT_00502a20 = alloc(0x7bc);             // 45 × hash handles
    *DAT_00502a20        = sfx("SFX_NULL");
    DAT_00502a20[1]      = sfx("SFX_Stamp");
    ...                                        // 45 named slots, indices 0x00..0x2c
    DAT_00502a20[0x27]   = sfx("SFX_RockWipe");   // ** index 0x27 = the menu wipe rumble
    ...
    DAT_00502a20[0x2c]   = sfx("SFX_AmbientMusicLoop");
    FUN_00472920(&LAB_00464c20,0);           // register sample hook
}
```

The hook (`0x464c20`, a standalone 47-byte stub — not a Ghidra procedure
entry) is:

```c
// cdecl hook(sample_name, object); user data (DAT_0076cd7c) ignored
alias = sample_name;
rc = FUN_00464c70(sample_name, &alias);      // hash name; write matching slot
if (rc) FUN_00465050(object, alias, 0, 1, 0);
```

- `FUN_00464c70(name, &slot)` hashes `name` via `FUN_00477670(name,0,1)` and
  linear-searches `DAT_00502a20` (the 45-slot bank) for a matching hash,
  returning the **slot index**; if the name isn't present it returns 0 (unless
  `DAT_00502a2c & 2`, the dynamic-add path).
- The 47-byte stub implements the output in place: it passes the address of
  its first stack argument (`[esp+4]`) as `FUN_00464c70`'s output pointer.
  On success, the slot index overwrites the consumed name pointer in that
  argument slot; the stub then reads that slot and passes the numeric index to
  `FUN_00465050`. This is why the wrapper receives an alias index even though
  the stub has no separate local slot allocation.
- `FUN_00465050(object, aliasIndex, 0, 1, 0)` → `FUN_00465090(*object, aliasIndex, 0, 1, 0)`
  (the object's first word = its position/pointer) → `FUN_0047ad70`, the
  3D positional DSound submit (bank match/defer logic in `DAT_00502a2c`).

**So `SAMPLE` keys on `.ae`/`.lws` animation frames (finding 12 §4) play through
this hook:** after a named action is selected, the next qualifying kind-4 step
hashes its frame's sample name against the global 45-slot SFX bank and may
submit a positional sound. The wipe's own
start-rumble is the same bank's slot `0x27` = `SFX_RockWipe` (found only now —
finding 24 saw the call `FUN_00464f60(0x27,0)` at `FUN_004103f0` but not the
name). `FUN_00464f60` is the **SFX queue submit** (pushes `(index, arg)` into
the 10-entry `DAT_00502b30`/`DAT_00502b58` pending queue).

## 3. The scene-end hook = entity "animation done" flag (`LAB_0043f130`, registered at boot)

`FUN_0041fb50` (boot/init hub, finding 02) registers the scene-end hook with
`FUN_00472900(&LAB_0043f130, 0)` at `0x41fd58`. The hook (`0x43f130`,
20-byte stub) is:

```c
void onSceneEnd(DDiSceneObject* obj) {
    if (obj) {                                   // FUN_00473db0(obj)
        DDiObjectOwner* owner = *(DDiObjectOwner**)((char*)obj + 0x20);
        if (owner) owner->flags |= 0x10000;       // OR 0x10000 into owner+0x3e4
    }
}
```

- `FUN_00473db0(obj)` = `*(obj + 0x20)` — the object's **owner/entity link**
  (the object is attached to an entity by the factory/activity layer).
- The write is `OR dword [eax+0x3e4], 0x10000` (verified instruction decode),
  i.e. **bit `0x10000` of the entity's `+0x3e4` flag word** — the same 32-bit
  word the entity-tick flag evaluator reads for behaviours (finding 10
  §3: wake timer `+0x3e4` bit `0x8000000`, grant `0x40`, shake
  `0x40000000`, …). Bit `0x10000` is previously undocumented in the corpus.

So when an LWS scene finishes, the engine marks the owning entity with
`+0x3e4 bit 0x10000` — an in-band "animation complete" signal the per-entity
behaviour evaluator can test on its next tick (the sampler in
`FUN_00475140` polls the same way the wipe's MenuTick polls its own
accumulator, finding 24).

## 4. The two registrars

| fn | registers | data | called from | role |
| --- | --- | --- | --- | --- |
| `FUN_00472900` | `DAT_0076cd80/84` | user `0` | boot hub `FUN_0041fb50 @ 0x41fd58` | scene-end hook |
| `FUN_00472920` | `DAT_0076cd78/7c` | user `0`, plus `DAT_0076cd94 \|= 0x40` (hook-enabled bit the draw step tests) | SFX-bank init `FUN_00464740 @ 0x464c11` | action-entry sample hook |

`FUN_00475140` gates the sample callback on the pending action-entry bit,
frame/name availability, and `DAT_0076cd78 != 0 && DAT_0076cd94 & 0x40`; it
clears the pending bit on the qualifying frame path even when the inner hook
conditions fail. The scene-end callback has its separate pointer/time gate.
Finding 42 documents the precise consumption conditions and the no-frame case.

## Key globals

| addr | role (observed) |
| --- | --- |
| `DAT_0076cd78/7c` | action-entry SFX-sample hook + its user data |
| `DAT_0076cd80/84` | scene-end entity-flag hook + its user data |
| `DAT_0076cd94` | hook-enable flag word (bit `0x40`) |
| `DAT_00502a20` | 45-slot SFX name-hash bank (`SFX_NULL`…`SFX_AmbientMusicLoop`; `0x27` = `SFX_RockWipe`) |
| `DAT_00502a2c` | SFX bank flags (bit0 play-enabled, bit1 dynamic-add, bit3 queue-defer) |
| `DAT_00502b20/50/30` | positional-SFX bookkeeping/defer queues |
| `DAT_00502bd0/30/58` | 10-entry SFX pending queue (`FUN_00464f60` submit) |
| `entity+0x3e4` | flag word extended: **bit `0x10000` = LWS scene ended** |

## Port relevance

- `src/gods98/sound.*` (finding 14 §1–2) already models the sound bank; the
  **name-hash bank (`DAT_00502a20`) and its `0x27` = `SFX_RockWipe`** mean the
  wipe's start-rumble (finding 24) has a concrete slot, and the action-entry
  `SAMPLE` playback path is now identified end to end.
- `src/game/ai.cpp` (finding 10) ports the `+0x3e4` flag-word evaluator; the
  scene-end hook's **bit `0x10000`** is a candidate for the "animation
  completed" transition if `.ae`/LWS entity animations are wired to the same
  flag word.
- Reproducing the wipe faithfully (finding 24) now requires: the LWS scene
  step (`FUN_00475140` + `FUN_00477010`/`FUN_00477150`), the boot hook
  registrations (`FUN_00472900`/`FUN_00472920`), and either the SFX-name-hash
  bank or a direct sample lookup for the `SFX_RockWipe` slot.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| Sample hook is eligible on the next qualifying kind-4 step after action selection (pending bit `2`) | observed (`FUN_00475140`; detailed in finding 42) |
| Sample hook = hash frame-name against 45-slot SFX bank, submit positional sound | observed (stub + `FUN_00464c70`/`FUN_00465050`/`FUN_0047ad70`) |
| Scene-end hook fires when time > last frame | observed (draw-step compare) |
| Scene-end hook ORs `0x10000` into `*(obj+0x20)+0x3e4` (owner flag) | observed (instruction decode + `FUN_00473db0`) |
| `SFX_RockWipe` = bank slot `0x27`, `FUN_00464f60(0x27,0)` = pending-SFX submit | observed (bank order + call at `FUN_004103f0`) |
| Registrars: `FUN_00472900` (boot) / `FUN_00472920` (SFX init, sets `DAT_0076cd94 \|0x40`) | observed |
| Where object `+0x20` (owner link) is set by the entity activity layer | unknown (getter observed; setter not traced) |
| Who consumes `entity+0x3e4` bit `0x10000` next tick | unknown (flag word observed set; no reader pinned yet) |
