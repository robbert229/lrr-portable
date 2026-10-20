# Finding 09 — Reward/Task Manager (Mission Progress & Post-Mission Screen)

*Anchors: `RewardInit @ 0x45f7f0`, `TaskBlockInit @ 0x45f030`, `TaskBlockFinish @ 0x45f750`,
task field loaders `0x45f1f0/0x45f230/0x45f290/0x45f3e0/0x45f530/0x45f5f0`,
`TaskImportanceRefresh @ 0x4600a0`, `TaskQuotaRefresh @ 0x460140`,
`RewardDisplayTick @ 0x461790`, `RewardZoneCheck @ 0x461dd0`, `FUN_00462780`,
`CrystalQuotaSetup @ 0x45c040`, `RewardScoreAdd @ 0x462800`, `CheckpointBlobBuild @ 0x45fda0`.*

## TL;DR

Three docs kept pointing here: the **task/reward block** at `DAT_00552990` is the live
mission-progress data that the checkpoint blob snapshots (doc 08) and that the post-mission
reward screen renders. Ten named tasks (Crystals, Ore, Diggable, Constructions, Caverns,
Figures, RockMonsters, Oxygen, Timer, Score) are seeded from per-level config keys, and a
second config group (`Reward{Images,Text,BoxImages,Fonts,Flics,Sounds,...}`) drives the
display-only reward overlay.

## Init / teardown

`LevelInit @ 0x422xxx` calls, back-to-back (0x422649 / 0x422651):

1. `RewardInit(cfgHandle, cfgDB)` — stores the two config DB pointers in globals
   (`DAT_00555b54/58`), loads screen-level keys: `Wallpaper`, `Display`, `CentreText`,
   `ScrollSpeed`, `VertSpacing`, `TitleFont`/`BackFont`/base font (`FUN_00479ee0` resolves
   font handles), `CompleteText`/`FailedText`/`QuitText`, `TextPos`, and button widgets
   `SaveButton`/`AdvanceButton` (widget tuples at `DAT_00555b68…b88` — these are the
   **zones** `RewardZoneCheck` hit-tests, see below).
2. `TaskBlockInit()` — allocates the 0x318c task block (`DAT_00552990`), zero-fills, sets
   10 task slots (stride `0x210`, base stored as self-ref at block+`0x1CE8`), names them
   `Crystals, Ore, Diggable, Constructions, Caverns, Figures, RockMonsters, Oxygen, Timer,
   Score`, then runs the six per-task field loaders + two refreshers.

`TaskBlockFinish` (from `EngineShutdown`) walks active/completed tasks flushing their
image/font handles, then **frees** the block.

## Task record (stride 0x210 = 528 bytes)

| Off | Loaded from config | Function |
| --- | --- | --- |
| +0x00 | flag byte: `&1` completed, `&2` active | `TaskBlockFinish` reads; loaders `|=` `0x31` |
| +0x04 | task name (default list or config value) | `TaskBlockInit` |
| +0x24 | `Reward.Importance.<name>` float | `TaskImportanceRefresh` |
| +0x28 | `Reward.Quota.<name>` int | `TaskQuotaRefresh` |
| +0x54 | BoxImage filename + 2 coords (from `BoxImages.<name>` `file\|x\|y`) | `TaskBoxImages`/`TaskBoxRect` |
| +0x57/58 | `Images.<name>` coords | `TaskImages` |
| +0x5b | `Flics.<name>` path | `TaskFlicPath` |
| +0x7b..7e | flic rect (4 uints) | `TaskFlicPath` |
| +0x200 | font handle (default `DAT_00555b40`) | `TaskFontLoad` |
| +0x204 | `Sounds.<name>` | `TaskSoundsLoad` |

Config format for every image entry: `file.bmp|x|y` (pipe-delimited triples in `Lego.cfg`).
`CrystalQuotaSetup` additionally reads `Quota.Crystals` capped to `≤26` (26 = crystal
display budget).

## Display + zones

- `RewardDisplayTick` (per frame, gated by `Reward.Display` and task flags): for tasks with
  the image/flic flags it positions blit targets via `FUN_0047dcb0` using the per-task
  handles/coords from the table above; draws `Complete/Failed/Quit` text.
- `RewardZoneCheck` + `FUN_00462780`: hit-test the mouse against the `SaveButton` /
  `AdvanceButton` widget rects (`DAT_00555b60..b98`). Either handler ends by calling
  **`CheckpointStep`** — i.e. clicking those reward-screen buttons triggers the `.sav` commit
  documented in doc 08. `FUN_00462780` also spins the `FUN_00477f30(1)` pump while input
  buttons are held.
- `RewardScoreAdd` bumps the mission score counters (`DAT_0055573c += 1`, float score
  `DAT_00555738 += x`) — also called by ObjectiveTick's `FUN_00459050` path with
  `param_3` as the increment.

## Per-level reward config (from `Lego.cfg`)

Level blocks nest a full `Reward` group; example (`Level01`):

```
Reward {
  Enable TRUE
  Modifier 2
  Importance { Crystals 60  Timer 20  Oxygen 10  Caverns 10 }
  Quota     { Crystals 5   Timer 900 Caverns 1 }
}
```

`TaskImportanceRefresh`/`TaskQuotaRefresh` are re-run over all 10 slots by
`CheckpointBlobBuild` before the blob copy, so the blob always holds up-to-date
importance/quota floats inside the task table.

## Cross-links

- Blob copies the task table verbatim (`doc 08`), meaning `.sav` records persist importance/
  quota per task.
- `RewardZoneCheck` calls `CheckpointStep` (`doc 08` save path) — the reward screen's save
  button is literally the savegame trigger.
- `FUN_00431560` (`LevelStatePtr`) supplies the current-level name used for the per-level
  `Reward` config group lookup in all refreshers.

## Still open

- How live task *values* (progress) reach the records — the flags/coords/paths are config,
  the running counts likely live in the `DAT_005557xx` score cluster.
- `FUN_0047dbf0` widget-reset semantics (called with all-zero args on zone init).
- The exact flic playback (`FUN_00483c80`/`FUN_00484070`) — renderer lane.
