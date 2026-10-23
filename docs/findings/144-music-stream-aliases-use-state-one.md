# Finding 144 — Music Stream Aliases Use Stream State One

The sound queue contains direct requests for two streamed music aliases with selector `1`: `FUN_00416c80` queues bank index `0x2c`, and the gameplay/summary paths `FUN_00461410`, `FUN_00461dd0`, `FUN_00462390`, and `FUN_00462780` queue index `0x2c` or `0x29`. The sound-name table maps `0x2c` to `SFX_AmbientMusicLoop` and `0x29` to `SFX_MusicLoop`; the shipped `Samples` entries for both use `@` paths (`Atmosdel` and `stats`).

`FUN_00464f60(alias, 1)` stores these requests in the deferred sound list. The drain in `FUN_004651a0` forwards the stored `1` to `FUN_00464fa0`, which passes it as the stream-state selector to `FUN_0047ad70`; because the aliases are marked `@`, the request reaches streamed playback and selects state one. This confirms the second stream state is used for shipped music aliases. Findings 136 and 141 describe replacement behavior and the queue rotation, respectively.

| Alias | Bank index | Observed callers | Selector |
| --- | ---: | --- | ---: |
| `SFX_AmbientMusicLoop` | `0x2c` | Menu entry (`FUN_00416c80`) and conditional gameplay/return path (`FUN_00461410`) | `1` |
| `SFX_MusicLoop` | `0x29` | Summary/state paths (`FUN_00461dd0`, `FUN_00462390`, `FUN_00462780`) | `1` |

These are static call-site observations. They establish the selected state and shipped alias configuration, not the exact audible transition timing for every caller.

**Evidence.** Shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. Call sites and `FUN_004651a0` are in `docs/decompiles/full/LEGORR.full.c`; `FUN_00416c80` and the FLIC queue path are recorded in `docs/evidence/legorr-menu-background-audio.bundle` (`ev_905d7182b9bea8909f756dbaf5a403d93784e6032cb9b34f03f14751d3ac5214`, `ev_14f7c534354ae9e09711d9ebace148e131a39cb00d8cc944872beb8c77475799`). The alias-name indices are from `FUN_00464740` in `docs/evidence/legorr-advisor-animation.bundle` (`ev_ada2764cfb26e1819be00f3f3aa0e0d1be95295d7a3a5e3a1cc814bf1ced13a8`). The `@` entries are in `Lego.cfg` within `reference/LegoRR1.wad`; the extracted-config hashes are recorded in finding 134.
