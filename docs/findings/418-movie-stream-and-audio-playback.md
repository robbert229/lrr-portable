# Finding 418 — Movie, Stream, and Audio Playback Boundaries

This finding maps the 13 `media-playback` functions assigned by Issue #194. The behavior below is based on the checked-in decompilation and named call sites; function names are descriptive labels, not proof of undocumented DirectShow or audio-library semantics.

## Playback paths

| Address | Function | Observed behavior |
| --- | --- | --- |
| `0x415700` | `MoviePlayWait` | Calls `DirectShowSampleUpdateAndBlit` (`0x4726d0`) at scale `1.0`, pumps the frame loop, increments a local frame counter, and compares it with `VideoFrameBoundRefresh` (`0x4726c0`). When its second argument is nonzero, it also returns early for the quit probe or either of two global quit flags. |
| `0x4157c0` | `MoviePlayLoop` | Resolves a configured duration and optional image key, loads a bitmap surface through `BitmapSurfaceLoad` (`0x47d9d0`), blits it while pumping frames with a 100 ms sleep until the time limit or quit request, then releases the surface. This path is a timed surface display, distinct from the DirectShow frame pump. |
| `0x415910` | `MoviePlayNamed` | Resolves a named media path, requires the path-check helper to succeed, creates a DirectShow wrapper with `DirectShowRenderSurfaceCreate` (`0x472560`), calls `MoviePlayWait`, destroys the wrapper, and releases the path record. |
| `0x415990` | `MoviePlayLevelVideo` | Resolves the `Video` key below a supplied level record, then uses the same path-check, DirectShow wrapper, wait, and cleanup sequence as `MoviePlayNamed`. |
| `0x415cf0` | `SaveSlotSelectionClear` | Sets `DAT_005574d0` to `-1`, clearing the selected save-slot index used by `SavBuildCtx`, `SavGetSlot`, and `SavSetSlot` (`0x417e70`–`0x417ea0`). Its caller, `GameConfigInit`, resets this context before constructing the front-end state. |
| `0x416150` | `StreamPlayLevel` | Formats `Stream_LevelName_Level_%d`, resolves it through the sound/stream table, and submits the resulting handle with mode `0`. |
| `0x4161a0` | `StreamPlayTutorial` | Formats `Stream_LevelName_TLevel_%d`, resolves it through the same table, and submits the resulting handle with mode `0`. |
| `0x42efc0` | `DirectShowVideoPlaybackLoop` | Gets a video interface from the supplied object. With a null second argument it requests frames without a destination rectangle and pumps the ordinary frame loop; otherwise it reads the rectangle, restores the render surface, and pumps with that destination. It closes the video interface afterward. |

`GameConfigInit @ 0x41fb50` calls `MoviePlayNamed` for the Lego, DDI, and Rock Raiders AVI keys, and calls `MoviePlayLoop` for the DDI and LMI logo keys, gated by the front-end and AVI checks. `LevelLoad @ 0x429890` calls `MoviePlayLevelVideo` for the level's `Video` key. `RewardScreenTick @ 0x461410` also calls `MoviePlayWait`; the supplied value is a DirectShow wrapper, although the wrapper's media source is outside this function. `LevelLoad` and `ObjectiveTick @ 0x458be0` call `DirectShowVideoPlaybackLoop` for their video objects. The source paths therefore include a DirectShow sample pump and a separate timed bitmap-surface display.

The two stream helpers have no direct call site in the full corpus. The legacy region contains one call to `StreamPlayTutorial` from `SavObjCallbackB @ 0x417700`; it does not contain a direct `StreamPlayLevel` call. This is a limit of the available call-site evidence, not evidence that the functions are unreachable in the executable.

## Audio controls and action cues

| Address | Function | Observed behavior |
| --- | --- | --- |
| `0x428800` | `ActionFeedbackSoundDispatch` | Saves the current batch mode, temporarily sets batch mode to `0`, maps event values `0`–`5` to cue IDs (`0x13`, `0x14`, `0x1f`, `0x1e`, `0x20`, `0x21`) and sets the paired pointer-cursor override (`0x14` or `0x15`, with value `0x41480000`). It submits the cue and restores the saved batch mode. Other values skip cue submission and still restore the mode. |
| `0x4295f0` | `SoundMinimumDistanceUpdateMode` | Mode `0` stores the supplied entity/context, updates several render/cell-distance values and related state; its third argument selects the stored scalar (`0.9` or `0.6`). Mode `1` clears the stored context and restores the corresponding cell/render values. Other mode values only update the mode global. The exact higher-level purpose of this state remains uncertain. |
| `0x4297b0` | `CDAudioWatchdogModeUpdate` | Enable sets bit `8` in `DAT_00556ecc` and schedules a watchdog callback using a time-derived slot. Disable clears the bit, removes the scheduled callback, and invokes the watchdog tick with `0`. |
| `0x429810` | `AudioPlaybackToggle` | Sets or clears bit `0x10` in `DAT_00556ecc` and forwards the same enabled value to the sound subsystem helper at `0x4652a0`. |
| `0x4351a0` | `ActionCodeToSoundAlias` | Maps action codes `1`–`5`, `8`, and `10`–`11` to cue IDs `0x2b`–`0x2f`, `0x32`, and `0x34`–`0x35`, then submits the cue. Other codes do nothing. |

`ActionFeedbackSoundDispatch` is called from UI hit testing, pointer/simulation interaction, placement previews, advisor scrolling, and objective/steering paths. `ActionCodeToSoundAlias` is called from `MainSimInteractionAndCellUpdate @ 0x426520` for action-record codes. `SoundMinimumDistanceUpdateMode` is used during level setup/teardown and selection changes; its helpers also update the cell list's distance scale (see Finding 255). `CDAudioWatchdogModeUpdate` is toggled from simulation mode transitions, input-device release, level reset, and the objective tick. `AudioPlaybackToggle` is called during initialization and from the simulation loop when the corresponding global flag changes.

## Evidence limits and follow-ups

- The checked-in pseudocode shows the DirectShow wrapper calls and frame-update loop, but does not establish the decoder's codec behavior.
- The stream helpers' formatted keys and submission path are observed; the table's data source and stream lifetime policy are outside these functions.
- `DAT_005574d0` is reset to `-1`, but its readers are not established in this slice.
- `SoundMinimumDistanceUpdateMode` has callers and concrete state changes, but the name does not explain the full gameplay mode; avoid inferring more from it without tracing the data consumers.
- Sound cue ID meanings beyond the observed mappings should be checked against the configured sound table before assigning semantic names.

**Related findings:** [23 — Front-End & Menus](23-front-end-and-menu.md), [255 — Visual Channel Relays and Scene Helpers](255-visual-channel-relays-resource-tree-callback-walks-scene-pose-and-registry-helpers.md), and [378 — Input, Selection, and Action State Helpers](378-input-selection-and-action-state-helpers.md).
