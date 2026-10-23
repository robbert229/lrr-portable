# Finding 157 — Music Volume Slider Collapses Its Top Two Values

The shipped Options menu defines `Music_Volume` from 0 through 10 and binds it to `FUN_00414df0`. That callback divides the selected integer by 9 and passes the result as both left and right normalized mixer values to `FUN_00478980`. The mixer helper `FUN_004789c0` clamps each value to the range 0.0 through 1.0 before setting the mixer control.

Consequently, slider values 9 and 10 both reach the mixer as 1.0 and request the same maximum value. Values 0 through 8 map to `index / 9`; value 10 computes above 1.0 and is clamped. This is independent of the nonlinear DirectSound `SFX_Volume` table documented in finding 156.

| `Music_Volume` index | Normalized value after clamp |
| ---: | ---: |
| 0–8 | `index / 9` |
| 9 | 1.0 |
| 10 | 1.0 (clamped from `10 / 9`) |

The static path establishes the duplicate top setting; it does not establish how a particular mixer device quantizes its own control values.

**Evidence.** Shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. `FUN_00414df0` and `FUN_00478980`/`FUN_004789c0` are in `docs/decompiles/full/LEGORR.full.c`; the callback is recorded in `docs/evidence/legorr-options-context-expansion.bundle` (`ev_b90345d663c586434f1d1fb120cddc3733c5844841dfa6161c72a85d7cfa6ee5`). The slider's `0:10` range and `Music_Volume` label are in `Lego.cfg` inside `reference/LegoRR1.wad` (SHA-256 `5a967d3147533c61390120f923cea2bd275cc29bc7a8563de21c62bd9de6229d`).
