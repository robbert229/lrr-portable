# 25 — The DDiScene object runtime: what MenuWipe actually renders through

Finding 24 identified the menu wipe as an LWS (LightWave scene) object. This
finding maps the runtime it runs on: a game-wide, D3DRM-style **scene-graph
object system** (16 core functions around `0x4727xx`–`0x477xxx`). The wipe's
`DAT_005574ec` is one instance of this object model; the same `FUN_00475140`
draw step has **25 callers** across entity, camera, movie, and menu code, so
this finding explains how most "animated 3D thing" calls in the game work.

**Observations from** REA/Ghidra 12.1.4 over `reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-21. Evidence: `docs/evidence/legorr-ddscene.bundle` (64 records).

## 1. The kind dispatch is a string table

`FUN_00472710 @ 0x472710` initialises the object-system globals; among them
the **kind-name table** at `DAT_0076add8` (`FUN_00472710` writes, in order):

| slot | string | addr | factory branch in `FUN_00472cd0` |
| --- | --- | --- | --- |
| 0 | `NULL` | `0x4a13f4` | — (matches nothing; default) |
| 1 | `MESH` | `0x4a4014` | `iVar2==1` → `FUN_00476850` (mesh/image) |
| 2 | `FRAME` | `0x4aaabc` | `iVar2==2` → `FUN_004767e0` |
| 3 | `ANIM` | `0x4a1ddc` | `iVar2==3` → `FUN_004765c0` (scene) |
| 4 | `ACT` | `0x4a19b4` | `iVar2==4` → activities branch |
| 5 | `-` | `0` | — |
| 6 | `-` | `0` | — |
| 7 | `LWS` | `0x4a13e0` | `iVar2==7` → `FUN_004765c0` (scene, matches finding 24) |
| 8 | `LWO` | `0x4a1d5c` | `iVar2==8` → `FUN_004809e0` |

`FUN_00475930` copies the input string, tokenizes on a delimiter
(`DAT_004a1cd4`, `FUN_00477440`), and does a case-insensitive walk of the
table returning the slot index (−1 if absent). `MenuWipe`'s factory call passes
`&PTR_DAT_004a13e0` = the bytes `"LWS\0"`, so it yields **`7`** → the LWS
scene-loader branch (finding 24). This is the same table finding 12 §`entity`
covered for `.ae` files; this finding adds the complete loader map.

## 2. The object record (`FUN_00472950`)

`FUN_00472950` is the object allocator. It:

1. lazily grows the **object pool** via `FUN_004759f0` — a free list at
   `DAT_0076add0` of `0x2c`-byte nodes, grown in doubling blocks sized
   `1<<DAT_0076cd90` (block counter, first call `DAT_0076cd90=0` → 64 bytes ÷
   0x2c). Pool grows through `DAT_0076ad80` bank slots.
2. pops one node from `DAT_0076add0`, clears its 11 dwords (`0x2c` bytes).
3. creates its three **channels** via `FUN_00475f70(*rec,…)`,
   `FUN_00475f70(rec[1],…)`, `FUN_00475f70(rec[2],…)` — each channel carries a
   position/rotation/scale frame (the wipe's per-frame transform set in
   `MenuTick` via `FUN_004754x0`).
4. registers the record's name fields through the vtable system global
   `DAT_00505914` (delegate calls at `+0x10`, `+0x18`, `+0x24`…).

Object layout (from `FUN_00472a50` destroy + `FUN_00472cd0` branch writes):

| +off | meaning (observed) |
| --- | --- |
| `+0x00` | channel 0 / vtable root |
| `+0x04` | channel 1 |
| `+0x08` | channel 2 |
| `+0x0c` | secondary handle (0x10-byte record, `FUN_00475c00`) |
| `+0x10` | **kind** (1,2,3,4,5,7,8 — the dispatch index) |
| `+0x14` | flags byte (wipe: `& ~3` clear on end; `& 1` run, `& 2` frozen) |
| `+0x24` | name string (freed on destroy) |
| `+0x28` | free-list next |

`FUN_00475f70` is the **prop-setter** — it attaches a `0x28`-byte record to an
object via its vtable `+0x18`/`+0x1c` accessors, filling `[1]=name`,
`[2]`..`[9]` = the 8 varargs (scale, transform refs, …). `MenuTick` calls it
with `&PTR`-style field references to write the wipe's per-frame transform.

## 3. The draw/step API (what the wipe runs each frame)

| fn | role (observed) |
| --- | --- |
| `FUN_00475710` | **step** — attach/refresh an object's channels against a scene context (`param_2`); kind `+0x10==3/4/6` paths interact with that context |
| `FUN_00475140` | **draw/advance at a time** — kind-4 (LWS-ACT) & kind-3 (scene) paths: read frame length (`FUN_00475390`), frame count (`FUN_00476200`), dispatch the registered animation hooks (`DAT_0076cd78`, `DAT_0076cd80`), and return elapsed time |
| `FUN_00477010` | **begin scene draw** (vtable `+0x78..+0x98` device setup, `__ftol` viewport math) |
| `FUN_00477150` | **end scene draw** — commit/copy back to the caller's surface (`FUN_00482d60`), error path logs via `s_C__Dev_SourceSafe_gods98_dx6_god_…` = **`gods98/dx6`** — confirms this is the gods98 DDiScene layer |
| `FUN_00475390` | scene length (float): kind 4 → `FUN_004761a0(...)`, kind 3 → `FUN_004761a0(ch)` |
| `FUN_004753f0` | frame/position: kind 4 → `FUN_00476180(...)`, kind 3 → `FUN_00476180(ch)` |
| `DDiSceneObjectArgumentsNormalize @ 0x475900` | normalizes the object argument and supplies the default scene context when null |
| `DDiSceneObjectPoseMethodDispatch @ 0x475470` | normalizes the object arguments through `FUN_00475900`, then dispatches the pose/transform arguments through the object's vtable slot `+0xec` |
| `FUN_00476160/180/1a0/1e0/200` | channel accessors via vtable `+0x1c` (get record) → read `+0xc/0x10/0x18/0x1c/0x20` |
| `FUN_00473b40` | toggle scene-wide flag (`flag&8`) around the render |

`DAT_0076cd80/84` and `DAT_0076cd78/7c` are **animation hooks** registered by
`FUN_00472900` (boot init calls `FUN_00472900(&LAB_0043f130, 0)` at
`0x41fd..`) — the scene-end/step callbacks the game's animation controller uses
to know when an LWS scene finished. The wipe itself polls the half/full length
in `MenuTick` rather than relying on these (finding 24), but entity animations
use the hook path.

## 4. The scene loader (`FUN_004765c0`) and file open

- `FUN_004800a0`/`FUN_004800c0` = **file open** (each opens via the resource
  table; `FUN_0047f6e0`/`FUN_0047fb10`; cache-free read `FUN_0047f980`).
- `FUN_004765c0(path, …)`: `param_4==0` → **image path** (`FUN_004800a0`);
  otherwise **scene path** that builds object/bench nodes through
  `DAT_00505914` and loads `.lws`/`.lwo` via the resource layer.
- `FUN_00476770` searches for `"DDiScene"`, advances through the following
  bytes until `FUN_0048f1d0` reports the mask-`4` character test, then parses
  the remaining text with `FUN_0048db70`. It returns the parsed value, or 1
  when the result is zero. `FUN_0048df40` is not a function entry: it lies
  inside `FUN_0048de20`, a generic array sort that takes a comparator callback.
  The class name and caller context support a DDiScene kind lookup, but the
  meaning of the character mask and parsed suffix is not established here.
- `FUN_00473b40`/`FUN_00477240` manage scene flags and the "current object"
  pointer the wipe draw uses.

## 5. How the wipe fits (cross-ref to finding 24)

```
MenuTick (0x412c00) every frame while wipe active:
  FUN_00477010(screen, 1)                    ; begin scene
  FUN_00475710(DAT_005574ec, DAT_005560d8)   ; step object vs scene context
  FUN_00475430/DDiSceneObjectPoseMethodDispatch(…) ; per-frame transform (kind 3 chan)
  FUN_00475140(DAT_005574ec, DAT_005574f4)   ; draw at accumulator
  FUN_00477150(time, DAT_005560d8, 0)        ; end scene
  … advance DAT_005574f4 by dt …
  at half length → release menu swap; at full → FUN_00410440 clears flags
```

So the "rock wipe" is: a pooled scene object (kind `LWS`=7), its three TRS
channels fed per-frame with the wipe timing, stepped and drawn through the
gods98 DDiScene layer, gated by the object flags byte at `+0x14`, and ended by
clearing that byte. Reproducing the wipe faithfully = implementing this small
DDiScene object model (or the `.flh` overlay stand-in from finding 24).

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| Kind table at `DAT_0076add8`, 9 slots, `NULL MESH FRAME ANIM ACT - - LWS LWO` | observed (init + strings) |
| `FUN_00475930` returns slot index; `MenuWipe` → `7` (LWS) | observed |
| Object record `0x2c` bytes, pooled, kind at `+0x10`, flags at `+0x14` | observed (allocator + destroy) |
| Pool free list `DAT_0076add0`, doubling blocks `1<<DAT_0076cd90` | observed (FUN_004759f0) |
| `FUN_00475140` = draw-at-time with length/frame reads + hook dispatch | observed |
| `FUN_00477010/00477150` = begin/end scene; error string names `gods98/dx6` | observed |
| `DAT_0076cd78/80` hooks = animation-end/step callbacks (boot-registered) | observed (registration) / inferred (exact callback semantics) |
| `FUN_004765c0` scene-vs-image branch, `.lws` via resource layer | observed |
| `DDiScene` string = the scene-graph class name lookup | observed (string + read site) |
| Object channel field semantics (`+0x0c/0x10/0x18/0x1c/0x20` role per channel) | inferred (paired accessors; no writer traces) |
| What `LAB_0043f130` (the boot-registered hook) does internally | unknown (address not a function entry) |
