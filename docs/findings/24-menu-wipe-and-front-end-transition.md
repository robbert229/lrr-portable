# 24 — Menu Wipe: the LWS rock-slide front-end transition

Closes the wipe/fade gap left open in finding 23 (`MenuRunLoop` "handling the
wipe/fade transitions" was named but not decomposed). `MenuWipe` is a
**config-driven menu-transition animation**: when the front-end switches
between menus, the game plays a full-screen LightWave (LWS/LWSC) "rock wipe"
scene that covers the old menu and is released mid-scene to reveal the new one.

**Observations from** REA/Ghidra 12.1.4 over `source/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-21, on the wipe slice of the menu-core region `0x4103d0`–`0x413e11`
plus the object factory at `0x472cd0`. Complementary data reads from the
shipped `Lego.cfg` (unpacked from `LegoRR0.wad`) and the unpacked
`Rock_wipe` asset directory. All addresses are virtual (image base `0x400000`).
Evidence: `docs/evidence/legorr-menu-wipe.bundle` (18 records).

## The config key

Shipped `Lego.cfg`, inside `Lego* → Menu`:

```
!MenuWipe    Interface\FrontEnd\Rock_Wipe\RockWipe
```

- The engine reads it twice: first plain `MenuWipe`, and on a miss the `!`
  toast spelling `!MenuWipe` (strings at `0x4a26d0` / `0x4a26c4`).
- The `!` prefix is the **reduce-animation gate** (feature bit `0x400000` of
  `DAT_00505f84`, the `-reduceanimation` switch, finding 12 §`!`-gate): with
  `-reduceanimation` set, the `!MenuWipe` fallback is **skipped** and no wipe
  object is created (the `LoadMenuBuild` branch joins straight to
  `LAB_004162aa`).
- A value of literal `"Null"` (case-insensitive first 4 chars, compared against
  `DAT_004a23f4` = `"Null"`) also disables the wipe.

## Loading — `LoadMenuBuild @ 0x4161f0`

```
uVar2 = FUN_00478f50(DAT_005560c4,&DAT_004a2544, "MenuWipe", 0);
pcVar3 = (char *)FUN_00479050(DAT_005560c0,uVar2);      // resolve config string
if (pcVar3 == 0) {                                     // plain key absent
  uVar2 = FUN_00478f50(DAT_005560c4,&DAT_004a2544, "!MenuWipe", 0);
  pcVar3 = ...;
  if (pcVar3 == 0) goto LAB_004162aa;                  // no wipe configured
  if ((DAT_00505f84 & 0x400000) == 0) {                // !-gate: skip if reduced
    iVar4 = __strnicmp(pcVar3,"Null",4);               // "Null" disables
    goto joined;
  }
} else {
  iVar4 = __strnicmp(pcVar3,"Null",4);
joined:
  if (iVar4 != 0)
    DAT_005574ec = FUN_00472cd0(0,pcVar3,&PTR_DAT_004a13e0,1);
}
FUN_0048db80(pcVar3);                                  // free config string
```

The factory call is the interesting part: `FUN_00472cd0` (the generic object
factory, finding 12 §`FUN_00472cd0`) is given a **kind-name string** as its 3rd
argument — `&PTR_DAT_004a13e0` is the address of the four bytes `4c 57 53 00`
= **`"LWS"`**. The factory dispatches through `FUN_00475930` (kind table
`DAT_0076add8`, initialised by `FUN_00472710` to the names
`NULL MESH FRAME ANIM ACT - - LWS LWO`); matching `"LWS"` lands in the
`iVar2 == 3 || 7` scene branch → `FUN_004765c0`, the **LWSC scene loader**
(finding 11 §`LoadLWSCScene`), building `<path>` → `<path>.lws` via the
`"%s.%s"` format at `0x4aaac8`. The resulting scene object is stored in the
global **`DAT_005574ec`**.

So `MenuWipe` = "create an LWS scene from this path and hold it as the menu
wipe." The shipped target is `Interface\FrontEnd\Rock_Wipe\RockWipe` → the
unpacked asset directory `Interface/FrontEnd/Rock_wipe/`:

| asset | role |
| --- | --- |
| `RockWipe.lws` | LWSC text scene, **FirstFrame 1, LastFrame 35, 25 fps** |
| `Boulder1/2.lwo`, `BoulderSMALL(2).lwo`, `RockdustX.lwo` | LightWave objects |
| `NewRock.bmp`, `NewRock2.bmp` | rock textures |
| `Boulder*.uv` | per-corner UV sidecars (finding 19) |

(plus `RockWipeRFwrocks3A.lws` and `delme.lwo` — engine-unused scene/model
leftovers in the same directory). The wipe is the boulders tumbling toward the
camera over 35 frames, filling the screen.

## Running it — the wipe state machine

| addr | name | role (observed) |
| --- | --- | --- |
| `0x413b80` | `MenuRunLoop` | triggers the wipe on each menu change; blocks the swap until it completes |
| `0x4103f0` | `FUN_004103f0` (wipe start) | if `DAT_005574ec≠0`: `DAT_005574f4=0`, `DAT_005574f0\|=3`, `_DAT_005574f8=timeGetTime()`, plays SFX `FUN_00464f60(0x27,0)` (the rock-rumble) |
| `0x412c00` | `MenuTick` | per-frame advance + draw while `DAT_005574ec≠0 && (DAT_005574f0&1)`; releases the menu at half-time |
| `0x410440` | `FUN_00410440` | wipe end: `DAT_005574f0 &= ~3` |
| `0x411a00` | `FUN_00411a00` (menu input tick) | **early-returns while `DAT_005574f0 & 2`** — input frozen during the wipe |

`MenuRunLoop` flow (observed):

```c
while (closeflag == 0) {
  MenuTick(fVar2, param_1, &local_98);          // runs the current menu + wipe
  ...
  if ((!bVar1) && (local_94 != param_1)) {      // menu selection changed
    FUN_004103f0();                             // start the wipe (SFX 0x27)
    bVar1 = true;
    if (DAT_005574ec == 0) local_98 = 1;        // no wipe → release immediately
  }
  if (bVar1 && local_98 != 0) {                 // wipe finished
    ... reset scroll (DAT_005574e0/dc), DAT_0055706c ...
    bVar1 = false; local_98 = 0; param_1 = iVar4; // swap to the new menu
  }
}
FUN_00410440();                                  // clear wipe flags on exit
```

`MenuTick` (wipe slice): while the wipe is active it renders the scene
(`FUN_00477010` begin, `FUN_00475710(DAT_005574ec, DAT_005560d8)` advance,
matrix/colour setup `FUN_00475430/5470`, `FUN_00475140(DAT_005574ec,
DAT_005574f4)` draw at the accumulator), then accumulates
`dt = clamp((timeGetTime()-_DAT_005574f8)*0.001*25*2, …, 3.0)`; when the
accumulator crosses **half the scene length** (`DAT_005574f4` vs
`FUN_004753f0()*0.5`) it sets `*param_3 = 1` (the `local_98` release signal)
and clears the selected-item globals; when past the full length it calls
`FUN_00410440()`.

**Net effect:** the rock slide plays over the outgoing menu while input is
frozen (`DAT_005574f0 & 2`), the menu swap happens at the scene's halfway
point, and only afterwards does the new menu accept input. The sound seam is
`FUN_00464f60(0x27, 0)` — matching the shipped `Lego.cfg` `Menu` key
`!SFX_RockWipe Sounds\FrontEnd\MenuRock`.

## Key globals

| addr | role (observed) |
| --- | --- |
| `DAT_005574ec` | the LWS wipe scene object (0 when disabled/absent) |
| `DAT_005574f0` | wipe flags — bit0 run, bit1 input-freeze (start `\|=3`, end `&=~3`) |
| `DAT_005574f4` | wipe time accumulator (float) |
| `_DAT_005574f8` | last `timeGetTime()` stamp |
| `DAT_005574fc` | render-state handle used around wipe draws |
| `DAT_005560d8` | scene/context handle passed to the LWS step |

## Port relevance

- Finding 23 named the wipe/fade transitions but not `MenuWipe`; this finding
  pins the config key, the `!` reduce-animation gate, the `"Null"` disable, the
  LWS kind dispatch, and the run/block/release timing — everything the port's
  `src/game/frontend.*` would need to reproduce the transition exactly.
- The port has **no wipe handling yet** (no `wipe` references under `src/` as
  of this finding). Two faithful options:
  1. **LWS path** — port the LWSC scene step (`FUN_004765c0` +
     `FUN_00475710`/`FUN_00475140`, finding 11) so the actual `RockWipe.lws`
     boulder scene plays; highest fidelity, most work.
  2. **Overlay path** — reuse the already-ported `.flh` FLIC lane (finding 21,
     `src/gods98/flic.*`) with a wipe-style full-screen clip and the same
     run/block/release timing + input freeze; approximate but cheap.
- The behavioural contract to match: config query `Menu::MenuWipe` →
   `!MenuWipe` (skip under `-reduceanimation`), `"Null"` disables, SFX on
   start, input frozen while active, menu swap at half-time, flags cleared at
   end.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| `MenuWipe`/`!MenuWipe` read in `LoadMenuBuild @ 0x4161f0` under `Menu` | observed (pseudocode + cfg) |
| Third arg to factory = `"LWS"` string; kind table slot maps to LWS scene loader | observed (bytes + `FUN_00475930`/`FUN_00472710`) |
| Shipped value = `RockWipe.lws` (35 frames @ 25 fps) + boulder LWO assets | observed (cfg + unpacked assets) |
| Wipe gated by `-reduceanimation` (bit `0x400000`) and `"Null"` disable | observed (branch) |
| Triggers on menu change; blocks swap until half-scene; input frozen while active | observed (MenuRunLoop + MenuTick + FUN_00411a00) |
| Start SFX `FUN_00464f60(0x27,0)` ↔ `!SFX_RockWipe`/`MenuRock` | observed (call + cfg) |
| Exact scene draw must go through LWSC loader (`FUN_004765c0`) | observed (dispatch) / inferred (draw helpers not fully decompiled) |
| `RockWipeRFwrocks3A.lws`, `delme.lwo` roles | unknown (present in dir, no obvious xrefs) |
