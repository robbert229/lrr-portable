# 107 — The `Samples` config section is a sound-alias and variant registry

The shipped `Lego* Samples` group is consumed by `FUN_00434a50`, which enumerates child keys and resolves each name through `FUN_00464c70` before passing its value list to `FUN_00464d00`. This connects config aliases such as `SFX_Drill`, `SND_RMrokstep`, and `Stream_Objective_Levels::LevelNN` to runtime sound groups.

`FUN_00464d00` splits values on literal commas, strips a leading `*` or `@` from each sample path, then links successful sound-bank slots into that alias's variant chain. `@` selects the streamed-playback path. The `*` marker is forwarded as the duplicate-buffer request: on the ordinary WAV path it makes the loader create two DirectSound duplicates and enables three-buffer rotation at playback (finding 134). It does not serve as the loader's loop argument. `FUN_00464e20` resolves an alias by selecting a chain member with `random_value % variant_count`.

Keys beginning with `!` are also conditional: `FUN_00434a50` removes the marker and skips the key when its mode argument is zero. The caller supplies that mode from a feature-bit test; this finding does not assign a semantic label to it.

Shipped examples include `SFX_Drill -> *Sounds\drtdrillc`, six alternatives on `!SFX_Drip`, five on `SND_RMrokstep`, and `@Sounds\Streamed\Objectiv\MisObj01` for `Stream_Objective_Levels::Level01`.

**Evidence.** REA/Ghidra 12.1.4, shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. Bundle: `docs/evidence/legorr-samples-config.bundle`. Focused records: `ev_cc490ae6f1334b0737f0182e39efc6ad88660b522280e000da906a95e88bdb35` (`0x434a50`); `ev_0827c47223be9e7b1c8ac5500ab135596534fe61297d3e0c8773e143da96809b` (`0x464c70`); `ev_a8240835fd2d908965cb4899b9200b2f1c37af02294f80ed8bbd46de607846af` (`0x464d00`); `ev_46f5b9126f83345e10fd26d750daf0c1cbe537ac12513b93a12ca127c045bfe4` (`0x477440`); `ev_ee2cbb6e6321d2818c917957cc59e8e8ff32819e13e51842789c5ef43c5222dd` (`0x464e20); and `ev_9cf0ef11e49abe4974600709d27cddc78521eee9c94f6ba65c1a2b8c2731b1f4` (initializer caller).

**Limitations.** The shipped config has no active `#...#` example; the marker's signed-volume behavior is traced in finding 142, but its authored use is not demonstrated. The random seed and lifecycle are outside this trace. The `!` mode is described only as observed control flow.
