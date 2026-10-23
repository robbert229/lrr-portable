# Finding 156 — Options SFX Volume Uses a Nonlinear DirectSound Table

The shipped Options menu defines `SFX_Volume` as a slider from 0 to 10 and binds it to `FUN_00414de0`. That callback calls `FUN_0047b530`, which accepts indices 0 through 10 and passes the selected entry in `DAT_004aae30` to `FUN_0047b500`. The latter validates the value and sets it on the DirectSound buffer at vtable slot `+0x3c` (`SetVolume`).

The eleven table entries are not evenly spaced. They map the slider index to these attenuation values in hundredths of a decibel:

| Slider index | SetVolume value | Attenuation |
| ---: | ---: | ---: |
| 0 | -10,000 | -100 dB (silence) |
| 1 | -5,000 | -50 dB |
| 2 | -3,500 | -35 dB |
| 3 | -2,000 | -20 dB |
| 4 | -1,500 | -15 dB |
| 5 | -1,000 | -10 dB |
| 6 | -700 | -7 dB |
| 7 | -400 | -4 dB |
| 8 | -200 | -2 dB |
| 9 | -80 | -0.8 dB |
| 10 | 0 | 0 dB (unattenuated) |

This is separate from the `Music_Volume` slider, whose `FUN_00414df0` callback routes through `FUN_00478980` to the WinMM mixer. This corrects the earlier callback label in finding 23: `FUN_00414de0` applies the SFX volume table; it is not the game-speed callback.

**Evidence.** Shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. `FUN_00414de0`, `FUN_00414df0`, `FUN_0047b530`, and `FUN_0047b500` are in `docs/decompiles/full/LEGORR.full.c`; the callback and setter are also recorded in `docs/evidence/legorr-options-context-expansion.bundle` (`ev_7cdfffa6286b9f7b3750aade0dfa67c1cc9f03990c55499bce9654729f16aead`, `ev_5d00584f61f5d8bfc8fb37b291bfa142633cf80e066effcb5ddf7f128c27320c`, and `ev_b01c781f3bc985df7a665514dfa4953ab9648781ea72e48d17ccaf1ac4e3b526`). The menu bindings and slider labels are in `Lego.cfg` inside `reference/LegoRR1.wad` (SHA-256 `5a967d3147533c61390120f923cea2bd275cc29bc7a8563de21c62bd9de6229d`). The 11 little-endian values were read from `LegoRR.exe` at VA `0x4aae30` (file offset `0xa9c30`). Microsoft documents the [SetVolume units and valid range](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/mt708939%28v%3Dvs.85%29).
