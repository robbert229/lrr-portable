# Finding 161 — Gameplay Uses Positional and Object-Attached SFX Submissions

Several gameplay callers submit a sound alias with an explicit position vector through `FUN_00465050(..., route=0, position)`. The wrapper forwards that vector through `FUN_00465090` to the positional playback mode in `FUN_0047ad70`. Other callers pass a sound-record/object value with `route=1`; the wrapper resolves the object and forwards that object through the other spatial mode. The sample-bank table maps these indices to named aliases:

| Alias | Bank index | Caller | Submission context |
| --- | ---: | --- | --- |
| `SFX_FallIn` | `0x28` | `FUN_0040f330` | Explicit position vector |
| `SFX_Stamp` | `0x04` | `FUN_00431780` | Explicit position vector |
| `SFX_BoulderHit` | `0x22` | `FUN_00447ea0` | Explicit impact position |
| `SFX_Laser` | `0x1b` | `FUN_0046ffb0` | Explicit firing position |
| `SFX_LaserHit` | `0x1c` | `FUN_0046ffb0` | Explicit impact position |
| `SFX_RockMonster2` | `0x0a` | `FUN_0043af30` | Object-attached route |
| `SFX_YesSir` | `0x11` | `FUN_004526e0` | Object-attached route |
| `SFX_Lava` | `0x26` | `FUN_00465b00` | Sound-instance updater supplies a position; constructor mapping is in finding 137 |

Finding 121 shows that loaded sound buffers request DirectSound 3D control, and finding 62 traces the global distance settings. This caller inventory connects those engine paths to specific gameplay aliases. It establishes the arguments submitted to the spatial wrapper, not audible output, exact coordinate units, or whether every device honors 3D processing.

**Evidence.** Shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. The gameplay call sites and `FUN_00465050`/`FUN_00465090` are in `docs/decompiles/full/LEGORR.full.c` at the listed function addresses and `0x465050`/`0x465090`. The sound-bank index-to-name mapping is initialized in `FUN_00464740` and recorded in `docs/evidence/legorr-advisor-animation.bundle` (`ev_ada2764cfb26e1819be00f3f3aa0e0d1be95295d7a3a5e3a1cc814bf1ced13a8`). The `SFX_Lava` sound-instance call and its constructor mapping are traced in finding 137.
