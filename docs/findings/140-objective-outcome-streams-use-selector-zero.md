# Finding 140 — Objective-Outcome Voice Streams Use Selector Zero

`FUN_00458670` constructs an objective-voice alias when a result screen is prepared. For achieved, failed, and failed-crystal outcomes, it formats a `Stream_Objective...` name from the outcome and objective/level identifiers, interns that name with `FUN_00477550`, and stores it in `DAT_004ffe38`. `FUN_00458be0` later consumes the pending name when its result-screen gate is active: it resolves the alias through `FUN_00464c70` and calls `FUN_00464fa0(aliasIndex, 0)`. The second argument is the stream-state selector, so these objective streams use state zero.

The shipped `Samples` section contains matching `@` aliases under `Stream_Objective_Levels::...`, `Stream_ObjectiveAcheived_Levels::...`, `Stream_ObjectiveFailed_Levels::...`, and `Stream_ObjectiveFailedCrystals`. The misspelling `Acheived` is present in both the code's outcome string and the config key. The level and tutorial keys appear under the appropriate outcome families, linking generated result-screen voice names to the streamed entries parsed by `FUN_00434a50`. The same screen setup also has states with no stream alias, and actual submission is conditional on the stored-name/result-screen gates.

| Objective result | Alias family | Selector | Status |
| --- | --- | ---: | --- |
| No result prefix / tutorial intro | `Stream_Objective_Levels::...` | `0` | Observed generated-name and submit path |
| Achieved | `Stream_ObjectiveAcheived_Levels::...` | `0` | Observed; `Acheived` spelling matches the binary/config |
| Failed | `Stream_ObjectiveFailed_Levels::...` | `0` | Observed generated-name and submit path |
| Failed crystals | `Stream_ObjectiveFailedCrystals` | `0` | Config alias present; consumer can generate the `FailedCrystals` outcome name |
| Training result lines | `Stream_ObjectiveAcheived_Levels::` tutorial keys | `0` | Present in shipped config; runtime selection depends on objective identifiers |

This extends the state-zero caller set from finding 137. Static evidence does not establish which individual objective keys resolve in every campaign or the audible timing of a result-screen voice line.

**Evidence.** Shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. The result-screen name construction and consumer are `FUN_00458670` and `FUN_00458be0` in `docs/decompiles/full/LEGORR.full.c`; both are recorded in `docs/evidence/legorr-campaign-expansion.bundle` (`ev_5568aa04bfcb7d32b016c98d71f8d7efdbb75734988e226709f6254ba50334b7`, `ev_509a5c13e8c5b080ff314d3a2c76ad8486a6a3afa70c97f8d136f649ca8bae23`). Alias parsing and streamed marker behavior are in `docs/evidence/legorr-samples-config.bundle` (`ev_a8240835fd2d908965cb4899b9200b2f1c37af02294f80ed8bbd46de607846af`). The shipped aliases are from `Lego.cfg` in `reference/LegoRR1.wad`, WAD SHA-256 `5a967d3147533c61390120f923cea2bd275cc29bc7a8563de21c62bd9de6229d`; extracted config SHA-256 `20cbf383bc672b0c330fe45e5fc509371de93d0b46958af82ac1b71f10fccb4f`.
