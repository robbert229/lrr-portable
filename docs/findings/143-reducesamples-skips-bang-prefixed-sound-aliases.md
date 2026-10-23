# Finding 143 — `-reducesamples` Skips `!`-Prefixed Sound Aliases

The command-line parser `FUN_00477bf0` recognizes `-reducesamples` and sets feature bit `DAT_00505f84 & 0x100000`. When game data is initialized, `FUN_0041fb50` calls the `Samples` loader `FUN_00434a50` with the Boolean `((DAT_00505f84 & 0x100000) == 0)`. Thus the default is to admit `!`-prefixed sample keys; the switch changes the argument to false.

`FUN_00434a50` strips a leading `!` from a key, then skips the key entirely when that Boolean is false. The skip occurs before `FUN_00464c70` resolves the name or `FUN_00464d00` registers its sample variants. With `-reducesamples`, a `!` alias is absent from the runtime sample registry rather than loaded with its marker intact. This applies to both WAV aliases and `@` streamed aliases. It is the `Samples`-specific meaning of `!`; the separate `!` gate in `MenuWipe` is controlled by `-reduceanimation`.

| Startup mode | `FUN_00434a50` argument | `!`-prefixed `Samples` key |
| --- | ---: | --- |
| Default | `1` | Strip `!`, resolve alias, register variants |
| `-reducesamples` | `0` | Skip before alias resolution and registration |

**Evidence.** Shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. Switch parsing and flag assignment are in `docs/decompiles/full/LEGORR.full.c` at `FUN_00477bf0` (`-reducesamples` string at `0x4aad28`); the startup argument is at `FUN_0041fb50`; key filtering is at `FUN_00434a50`. The startup call and filter are recorded in `docs/evidence/legorr-samples-config.bundle` (`ev_9cf0ef11e49abe4974600709d27cddc78521eee9c94f6ba65c1a2b8c2731b1f4`, `ev_cc490ae6f1334b0737f0182e39efc6ad88660b522280e000da906a95e88bdb35`). The same config loader is detailed in finding 107.
