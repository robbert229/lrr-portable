# Finding 138 — Damage-Hurt Sound Submits with Selector Zero

The damage handler `FUN_0043fb40` resolves the `SND_Hurt` alias with `FUN_00464c70`, obtains a sound-record value through `FUN_0044a200`, and submits it via `FUN_00465050`. The call-site assembly at `0x43fbd8–0x43fbed` confirms the five arguments reaching `FUN_00465050` are `(soundRecord, aliasIndex, 0, 1, 0)`. `FUN_00465050` forwards arguments two through five to `FUN_00465090`. On the immediate path, `FUN_00465090` passes its third argument—zero here—as the fourth argument to `FUN_0047ad70`, the stream-state selector. Its fourth argument (`1` here) selects the immediate playback route.

The shipped `Samples` entry for `SND_Hurt` is a four-item comma-delimited list of ordinary WAV paths (`Sounds\Minifigure\Hurt1`, `Hurt2`, then those two again). It has no `@` marker. Thus this call submits the hurt sound through the in-memory WAV path; its selector-zero value does not select a streamed playback state for the shipped configuration. The parser's sticky `@` mode for comma-delimited variants is documented in finding 135, but does not apply to this alias.

This adds another selector-zero caller to finding 137's map. Static call-flow and config evidence do not establish runtime variant choice, audible overlap, or timing.

| Caller path | Alias | Fourth argument to `FUN_0047ad70` | Shipped mode |
| --- | --- | ---: | --- |
| Damage handler `FUN_0043fb40` | `SND_Hurt` | `0` | In-memory WAV variants; no `@` |

**Evidence.** Shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. The handler body is in `docs/decompiles/full/LEGORR.full.c` at `FUN_0043fb40`; its call-site stack setup and cleanup at `0x43fbd8–0x43fc03` were decoded from the executable. `FUN_00465050` and `FUN_00465090` are in the full decompile at `0x465050` and `0x465090`; the forwarding into `FUN_0047ad70` is described in finding 137 and recorded in `docs/evidence/legorr-sound-instance-spatial-expansion.bundle` (`ev_7da0a488bdbf46abfc69ed8002c9fc64f46e9742a0fc060034aedd5bca77cae`). The `SND_Hurt` entry is from `Lego.cfg` in `reference/LegoRR1.wad`; WAD SHA-256 `5a967d3147533c61390120f923cea2bd275cc29bc7a8563de21c62bd9de6229d`, extracted config SHA-256 `20cbf383bc672b0c330fe45e5fc509371de93d0b46958af82ac1b71f10fccb4f`.
