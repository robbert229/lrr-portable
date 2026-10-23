# 27 — The scene-done flag in the entity tick: what `entity+0x3e4 bit 0x10000` drives

Continues finding 26's open ledger. Finding 26 showed the DDiScene scene-end
hook (`LAB_0043f130`, boot-registered by `FUN_00472900`) ORs **bit
`0x10000` into the owning entity's `+0x3e4` flag word** when an LWS scene
finishes. This finding locates and decodes both *readers* of that bit: they
live in the ~15 KB per-entity flag-word behaviour evaluator that finding 10
identified at entry `0x43c810` (left unattributed by Ghidra). Both consumers
connect animation completion to gameplay sound/impact behaviour, which is the
missing half of "the scene ended" signal's purpose.

**Observations from** REA/Ghidra 12.1.4 over `reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-21. Assembly windows read from the shipped image (`0x43d8f0–0x43ddc0`)
and cross-checked against the finding-10/14/26 corpus names. Evidence:
`docs/evidence/legorr-scene-done.bundle` (42 records).

## 1. Reader A — placement sound by work-object type (`0x43dd10`)

```
43dcf5:  test $0x80000, %edx          ; (context: gated/queue mode?)
43dcfb:  je  out                       ; → 0x43ddc5, no placement handling
43dd01:  push %esi; call 0x44a200      ; work-object resolver (finding 10)
43dd0b:  call 0x475300                 ; DDiScene ActFrame timing update
43dd10:  mov 0x3e4(%esi),%eax          ; ---- entity+0x3e4
43dd19:  test $0x10000,%eax            ; scene-done bit
43dd1e:  je  out                       ; not done → skip
43dd20:  mov 0x3e0(%esi),%eax
43dd26:  test $0x4,%ah                 ; +0x3e0 bit 0x400 (type-5 gated mode, finding 10)
43dd29:  je  out
43dd2b:  mov 0x308(%esi),%eax          ; work-object / placed-target handle
43dd31:  test %eax,%eax; je  out
43dd35:  lea 0x40(%esp),%ecx; push ...  ; frame to FUN_004754c0 (move/pose)
43dd3d:  call 0x44a200; call 0x4754c0
43dd4b:  mov 0x308(%esi),%edx
43dd54:  mov (%edx),%eax               ; *target — the placed object's type code
43dd56:  cmp $0x7,%eax
43dd5b:  (type 7)  → push 0x24 → jmp play
43dd68:  cmp $0x6,%eax
43dd6d:  (type 6)  → push 0x25 → jmp play
43dd7a:  (other)   → push 0x23
43dd85:  call 0x465050                 ; SFX submit (finding 26)
```

Reading the type dispatch against the object-type codes (finding 10 §`.ol`
special kinds: PowerCrystal **6**, Ore **7**) and the SFX bank (finding 26
§2: slot `0x23` = `SFX_Place`, `0x24` = `SFX_PlaceOre`, `0x25` =
`SFX_PlaceCrystal`):

> **When an entity's LWS placement animation ends and it is in gated/placing
> mode (`+0x3e0 & 0x400`) with a target handle (`+0x308`), the engine plays
> `SFX_PlaceOre` (type 7), `SFX_PlaceCrystal` (type 6), or `SFX_Place`
> (anything else) via `FUN_00465050` → `FUN_0047ad70`.**

The `0x40/24/25` push values are the *bank indices* (verifiable against the
`FUN_00464740` slot order), not raw file paths — the scene-done flag is what
turns "I finished the drop animation" into the audible "clunk".

## 2. Reader B — wake/impact follow-up (`0x43d903`)

```
43d903:  mov 0x3e4(%esi),%eax
43d90e:  test $0x10000,%eax      ; scene-done bit
43d913:  je  after
43d915:  push %esi
43d916:  call 0x4475c0           ; the "wake/impact" probe (see below)
43d91e:  test %eax,%eax; je  after
43d922:  and $0xfff7ffff,0x3e4(%esi)   ; clear scene-done bit
after:
43d928:  flds 0x2d4(%esi) ...          ; then the shared timer-expiry path
```

When the scene-done bit is set, the tick probes `FUN_004475c0(ent)`; if it
"wakes" (returns nonzero) the bit is **cleared** (`AND 0x3e4, ~0x80000`) so
the notification fires once per animation.

`FUN_004475c0` (cross-checked against finding 10/22 tails) is the impact/wake
handler: it reads the linked/attached entity (the `+0x2fc` slot), checks the
`+0x3e4` bit `0x100000` (impact in progress), walks the terrain-speed probe
(`FUN_00469e80`), and branches on entity type 3 → message `5`
(`FUN_00419b80(5,0,target,0)` — the boulder-roll/impact bus, finding 22), adds
roll speed to `+0x340`, or applies a shake (`FUN_0043f6a0` + bit
`0x40000000`). So a boulder finishing its LWS tumble can immediately tell the
tick "I landed" → impact FX + message.

## 3. Why this matters

Finding 26 proved the *writer* (DDiScene hook → `+0x3e4 bit 0x10000`); this
finding proves the *readers* live in the entity-tick flag evaluator and drive
**audio (`SFX_Place*`) and impact/wake (`FUN_004475c0`)**. That closes the
loop: LWS scene completion is how the game synchronizes "the 3D animation
finished" with gameplay feedback, and it runs through the same `+0x3e4` word
finding 10 already ports in `src/game/ai.cpp`'s flag-word evaluator.

New flag-bit facts appended to finding 10's `+0x3e4` map:

| `+0x3e4` bit | role (observed) |
| --- | --- |
| `0x10000` | **LWS scene ended** (set by DDiScene hook, find 26) |
| `0x80000` | cleared alongside scene-done after a successful `FUN_004475c0` wake |
| `0x100000` | impact-in-progress (checked by `FUN_004475c0`) |
| `0x40000000` | shake raised by `FUN_0043f6a0` on impact |

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| Reader A at `0x43dd10`: scene-done + gated mode (`+0x3e0&0x400`) + target (`+0x308`) → `SFX_PlaceOre/Crystal/Place` by type (`7`/`6`/other) via `FUN_00465050` | observed (assembly decode + bank order + type codes) |
| Reader B at `0x43d903`: scene-done → `FUN_004475c0` probe → clear bit | observed (assembly decode) |
| `FUN_004475c0` = impact/wake handler (message 5, roll speed, shake) | observed (pseudocode) |
| Reader bodies live inside the finding-10 `0x43c810` flag-evaluator blob | observed (addresses fall in the blob range; blob itself unattributed) |
| `+0x308` precise nature (target vs built-work handle) | inferred (read in both readers; not fully traced) |
| Exact SFX submit flags for `FUN_00465050` args in Reader A | inferred (index field pinned; flags not fully decoded) |
