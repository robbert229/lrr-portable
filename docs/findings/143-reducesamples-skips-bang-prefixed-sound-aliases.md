# Finding 143 — `-reducesamples` Skips `!`-Prefixed Sound Aliases

The command-line parser `FUN_00477bf0` recognizes `-reducesamples` and sets feature bit `DAT_00505f84 & 0x100000`. When game data is initialized, `FUN_0041fb50` calls the `Samples` loader `FUN_00434a50` with the Boolean `((DAT_00505f84 & 0x100000) == 0)`. Thus the default is to admit `!`-prefixed sample keys; the switch changes the argument to false.

`FUN_00434a50` strips a leading `!` from a key, then skips the key entirely when that Boolean is false. The skip occurs before `FUN_00464c70` resolves the name or `FUN_00464d00` registers its sample variants. With `-reducesamples`, a `!` alias is absent from the runtime sample registry rather than loaded with its marker intact. This applies to both WAV aliases and `@` streamed aliases. It is the `Samples`-specific meaning of `!`; the separate `!` gate in `MenuWipe` is controlled by `-reduceanimation`.

The shipped `Lego.cfg` has 446 active `Samples` entries; 242 begin with `!` and contain 263 sample-path values. Of those conditional entries, 37 are streamed: 25 level-name lines, 8 tutorial level-name lines, and 4 front-end FLIC overlay sounds. Thus reduced-sample mode removes those aliases along with the conditional WAV groups.

| Startup mode | `FUN_00434a50` argument | `!`-prefixed `Samples` key |
| --- | ---: | --- |
| Default | `1` | Strip `!`, resolve alias, register variants |
| `-reducesamples` | `0` | Skip before alias resolution and registration |

| Shipped-config inventory | Count |
| --- | ---: |
| Active `Samples` entries | 446 |
| `!`-prefixed entries | 242 |
| Path values under `!` entries | 263 |
| `!` entries with `@` streamed paths | 37 |

**Evidence.** Shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. Switch parsing and flag assignment are in `docs/decompiles/full/LEGORR.full.c` at `FUN_00477bf0` (`-reducesamples` string at `0x4aad28`); the startup argument is at `FUN_0041fb50`; key filtering is at `FUN_00434a50`. The startup call and filter are recorded in `docs/evidence/legorr-samples-config.bundle` (`ev_9cf0ef11e49abe4974600709d27cddc78521eee9c94f6ba65c1a2b8c2731b1f4`, `ev_cc490ae6f1334b0737f0182e39efc6ad88660b522280e000da906a95e88bdb35`). The config inventory was counted from `Lego.cfg` in `reference/LegoRR1.wad`; its WAD and extracted-config hashes are in finding 134. The same config loader is detailed in finding 107.
