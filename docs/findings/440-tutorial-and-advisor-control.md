# Finding 440 — Tutorial and Advisor Control

This finding follows level setup into the NERP script interpreter, advisor text/token rendering, camera-focus updates, and the linked coordinate lists built for tutorial paths. It documents direct pseudocode observations and marks the tutorial command table's higher-level meanings as unresolved. Related traces include [Finding 75](75-olistfile-entry-placement.md), [Finding 140](140-objective-outcome-streams-use-selector-zero.md), [Finding 247](247-config-graphics-camera-tween-and-pool-spawn-helpers.md), [Finding 256](256-command-band-step-issuers-flag-behavior-timers-and-steering-helpers.md), [Finding 354](354-record-vector-and-resource-state-helpers.md), [Finding 374](374-terrain-interface-and-media-helpers.md), [Finding 397](397-crt-strings-locale-and-formatting.md), and [Finding 429](429-advisor-and-message-ui-state.md).

## Level script and advisor token setup

`TutorialNerpScriptFileLoad @ 0x00452df0` clears the 45-dword block beginning at `DAT_00555d40`, clears an 11-dword block beginning at `DAT_004ff958`, resets adjacent script state, and calls the message-overlay reset helper `0x0046aa90`. It sets the global returned by `GlobalFlagGet_004ff978` to `1` through `EngineModeSet`, then loads the supplied script resource into `DAT_00555d6c` and its length into `DAT_00555d70`; its return value reports whether the loaded handle is nonzero. `LevelLoad @ 0x00429890` calls this loader and then `ObjectiveAdvisorTokenLoad @ 0x00452e70`.

`ObjectiveAdvisorTokenLoad` scans the supplied text buffer up to its length. It changes underscores to spaces, splits ordinary text entries into an indexed string array, and builds separate name/handle tables for entries introduced by `:` and `$` at the start of a text line. The `:` path resolves an image resource; the `$` path resolves a sound alias. The exact authored NERP grammar and how every token form is used are not established by these helpers alone. `ObjectiveAdvisorTokenLookup @ 0x00453200` bounds-checks an index against the ordinary-text count and returns the corresponding pointer, or zero when out of range. `AdvisorScrollStepAdvance @ 0x00453920` uses this lookup before submitting a selected token to the advisor text-request path described in Finding 429.

`AdvisorDisplayStateResourcesRelease @ 0x00453220` releases the loaded script buffer, the token text buffer and pointer array, and each retained handle in the `$` table; it clears the primary token globals and the 45-dword state block, then calls the message-overlay reset helper again. `PendingLevelName @ 0x0042f0c0` invokes it during level teardown. The direct body does not show a corresponding free of the `:` image-table allocation; whether another owner releases it remains an open question.

## NERP dispatch, timers, and camera focus

`TutorialScriptCommandDispatch @ 0x00453320` runs from `EngineTick @ 0x004232e0` when the tutorial-disable bit `0x01000000` is clear (the caller is preserved in `LEGORR.supplement.c` and the legacy region). It walks the loaded script as 32-bit words. The interpreter uses high bits to distinguish callback, comparison, and control-flow records, takes callback identifiers from the low 16 bits, consults the callback metadata/table at `0x004a5948`–`0x004a5950`, and invokes callbacks with zero to three resolved arguments. `IndirectHandleResolveInPlace @ 0x004532e0` resolves values in the `0x0002xxxx` tagged form through the same callback table when its resolver slot is unset. Comparison results are carried between records and can alter the next script position; optional debug output formats callback names. The bytecode's complete opcode names and user-facing semantics are not recovered here. After processing the script, the interpreter calls `CameraFocusTweenUpdate @ 0x00453da0` with the frame delta.

The script buffer is stored at `DAT_00555d6c` and its byte length at `DAT_00555d70`; the dispatcher walks `length / 4` words, and `AdvisorDisplayStateResourcesRelease` frees the loaded buffer.

`CameraFocusTweenUpdate` follows either a tracked object index or an explicit object handle while its focus gate is active, updates the stored cell/position pair, and clamps the viewport target. Separate zoom and tilt gates advance their values toward stored targets and clear when they enter their completion windows; the helper dispatches the resulting camera zoom/tilt updates. When the current scene record has its `+0x27c` field set, it also submits `{100, 0}` through `CameraFocusScrollScaleCommit @ 0x004538a0`. Finding 247 traces the camera tween's object resolution and state fields.

`CameraFocusScrollScaleCommit` skips its updates when its first value is zero and the scene record's `+0x27c` is nonzero. Otherwise it stores the second value in `DAT_004de548`, passes the first value as a percentage to `ScrollScaleSetAndClamp @ 0x00425ce0`, then writes the second value again. The latter helper accepts increases or updates while the stored guard is zero, applies the scale, and clamps it to `[0,3]`.

`FrameTimerArrayAdvanceAndAdvisorClear @ 0x00453bb0` advances four floats at `DAT_004ff9a0`–`DAT_004ff9ac` by `delta * 40`. It also advances or reduces the advisor-duration value at `DAT_004ff98c`, with its decrement/re-arm path gated by the deferred-callback and bookkeeping globals. When `DAT_00555df0` is set and the duration falls below `1000`, it calls `AdvisorClearActiveState @ 0x00401b20`. Finding 256 describes the timer path and its `EngineTick` call in the legacy snapshot.

The separate callback relay is small but stateful. `DeferredCallbackSet @ 0x00453900` stores its argument in `DAT_004a673c`; `GameConfigInit` supplies whether the corresponding message resource was found. `DeferredCallbackInvokeIfSet @ 0x00453910` checks only whether that stored value is nonzero, then calls `SoundAliasPlayGlobal @ 0x00456810`. That helper plays the alias currently stored in `DAT_004a6768` when it is not `-1`. The UI interaction caller is in `MainSimInteractionAndCellUpdate @ 0x00426520`; it reaches the relay for the configured message region. Finding 354 records the setter/invoker pair.

`AdvisorTutorialIconImageLoad @ 0x00434a00` loads the configured `TutorialIcon` bitmap and retains its resource handle in `DAT_0055645c`. `RandomVariantCellScatter @ 0x00435300` blits that image above marked cells with a sinusoidal vertical offset. `FUN_00454600 @ 0x00454600` uses the same image for an indexed-object overlay after resolving the last object in `DAT_00555dec`, setting its `+0x3f8` field, and deriving screen coordinates; the exact trigger and final arguments remain uncertain.

## Advisor text and global state

`AdvisorInlineTokenRender @ 0x00456830` copies text directly when there are no registered inline-token tables. Otherwise it scans `<...>` and `#...#` spans while copying ordinary characters to the output buffer. A matching `<...>` entry selects a retained image handle for drawing. A matching `#...#` entry selects a sound alias; when the selected alias changes and the sound gate is active, it updates the duration/scroll globals and plays that alias. A nonzero third argument stores the final duration in `DAT_004ff98c`. `AdvisorTextRequestSetup` and `AdvisorTextDisplayWorker` call this helper; Finding 429 traces those callers and their text-buffer flow.

`ObjectiveVoiceAliasRaiseCommit @ 0x00454b70` forwards selector `1` to `FUN_00458670`; `ObjectiveVoiceAliasRaiseGateBit27 @ 0x00454b80` forwards selector `2` only when bit `0x08000000` of `DAT_00556ecc` is clear. `MainSimStep @ 0x004250c0` calls these helpers when its tracked pilot-related byte values change. Finding 140 traces how `FUN_00458670` constructs objective voice aliases and how the result-screen path consumes them; the full semantics of selectors 1 and 2 in this caller remain unresolved.

The remaining accessors affect nearby engine state but do not establish a tutorial-only mode. `EngineModeSet @ 0x004565f0` stores a value in `DAT_004ff978`; for value `1` it calls two reset helpers and clears two globals. `GlobalFlagGet_004ff978 @ 0x00456640` returns that same value, which `AdvisorTextDisplayWorker` uses as a gate. `SystemStateFlags @ 0x00456240` returns the separate bitfield at `DAT_004ff980`, and `SetSystemStateFlags @ 0x00456550` copies a pointed-to value into it. `GlobalValue_004ff9bcStoreAndReturn @ 0x004567c0` stores and returns its argument in `DAT_004ff9bc`; its observed caller is `InputDeviceRelease`. `GlobalCounter_004ff990Increment/Reset @ 0x00454880/0x00454890` increment or zero the counter; the observed callers are the deferred object-list creation and record-release paths documented in Finding 386. `FUN_00455300` resolves a supplied type name through `ObjectListEntityTypeResolve @ 0x0042e8b0`, then returns the selected `GridRecordCountGet @ 0x004374a0` value; `StartLevelPilots` and `StartLevelSelected` pass it the string `"Pilot"`. `FlhMagicUuNoop @ 0x00456210` always returns zero; its FLIC and CRT call sites do not reveal a tutorial-specific role.

## Tutorial path coordinate lists

`TutorialPathCoordinateIndex @ 0x00456b80` clears 55 list heads stored at `param_1 + 0x114`, scans the object's width-by-height cell grid, and for each nonzero cell byte allocates a 12-byte node containing the cell's x/y and the previous head for the byte-indexed list. It prepends the node to that list and clears the source byte. `LevelLoad` calls it after loading map state. `ObjectPoolListHeadGet @ 0x00456c30` returns a nonempty head only for indices `0`–`54`. `ObjectPoolListsFreeAll @ 0x00456c60` obtains the current context, frees nodes from all 55 lists, and clears each head; `PendingLevelName` calls it at teardown. `CoordinateListCallbackVisit @ 0x00456cb0` walks the selected list, calling the supplied callback with the node, its two stored coordinates, and a caller value, and stops when the callback returns zero. No direct caller for this visitor is present in the full pseudocode corpus.

## Assigned-function map

| Address | Function | Observed responsibility |
| --- | --- | --- |
| `0x00452df0` | `TutorialNerpScriptFileLoad` | Resets tutorial/advisor state, selects mode `1`, loads the script resource, and stores its length. |
| `0x00452e70` | `ObjectiveAdvisorTokenLoad` | Parses ordinary text plus separate image and sound token tables from the supplied buffer. |
| `0x00453200` | `ObjectiveAdvisorTokenLookup` | Bounds-checks and returns an ordinary text token. |
| `0x00453220` | `AdvisorDisplayStateResourcesRelease` | Releases script/text state and retained sound-token handles; resets advisor state. |
| `0x004532e0` | `IndirectHandleResolveInPlace` | Resolves a tagged `0x0002xxxx` handle through the callback resolver table. |
| `0x00453320` | `TutorialScriptCommandDispatch` | Interprets script records, dispatches callbacks/conditions, then advances camera-focus tweens. |
| `0x004538a0` | `CameraFocusScrollScaleCommit` | Applies a guarded percentage scroll-scale update and stores the associated guard value. |
| `0x00453900` | `DeferredCallbackSet` | Stores the configured callback-presence value in `DAT_004a673c`. |
| `0x00453910` | `DeferredCallbackInvokeIfSet` | Plays the current sound alias when the stored callback-presence value is nonzero. |
| `0x00453920` | `AdvisorScrollStepAdvance` | Selects the next advisor token and submits it to the text-request path. |
| `0x00453bb0` | `FrameTimerArrayAdvanceAndAdvisorClear` | Advances four timers and clears active advisor state when its duration expires. |
| `0x00453da0` | `CameraFocusTweenUpdate` | Follows a tracked entity and advances gated zoom/tilt tweens. |
| `0x00454600` | `FUN_00454600` | Advances a sine phase, resolves an indexed object, and blits a resource at derived screen coordinates. |
| `0x00454880` | `GlobalCounter_004ff990Increment` | Increments `DAT_004ff990`. |
| `0x00454890` | `GlobalCounter_004ff990Reset` | Resets `DAT_004ff990` to zero. |
| `0x00454b70` | `ObjectiveVoiceAliasRaiseCommit` | Requests objective-voice selector `1`. |
| `0x00454b80` | `ObjectiveVoiceAliasRaiseGateBit27` | Requests selector `2` only when global bit `0x08000000` is clear. |
| `0x00455300` | `FUN_00455300` | Resolves a type name and returns a selected grid-record count. |
| `0x00456210` | `FlhMagicUuNoop` | Returns zero without changing state. |
| `0x00456240` | `SystemStateFlags` | Returns the bitfield stored at `DAT_004ff980`. |
| `0x00456550` | `SetSystemStateFlags` | Stores a pointed-to value in `DAT_004ff980`. |
| `0x004565f0` | `EngineModeSet` | Stores `DAT_004ff978` and runs reset helpers for mode `1`. |
| `0x00456640` | `GlobalFlagGet_004ff978` | Returns the separate global at `DAT_004ff978`. |
| `0x004567c0` | `GlobalValue_004ff9bcStoreAndReturn` | Stores and returns a value in `DAT_004ff9bc`. |
| `0x00456810` | `SoundAliasPlayGlobal` | Plays `DAT_004a6768` when it is not the `-1` sentinel. |
| `0x00456830` | `AdvisorInlineTokenRender` | Copies plain text and resolves inline image/sound tokens when configured. |
| `0x00456b80` | `TutorialPathCoordinateIndex` | Converts nonzero tutorial-grid bytes into 55 coordinate-linked lists. |
| `0x00456c30` | `ObjectPoolListHeadGet` | Returns a nonempty list head for an index in `[0,54]`. |
| `0x00456c60` | `ObjectPoolListsFreeAll` | Frees all 55 coordinate lists and clears their heads. |
| `0x00456cb0` | `CoordinateListCallbackVisit` | Visits one coordinate list until the callback returns zero. |

## Open questions

- What is the complete opcode and callback-name mapping for the NERP table at `0x004a5948`–`0x004a5950`, including its branch/skip records?
- What do the colon- and dollar-prefixed token declarations mean in shipped NERP text, and who owns/releases the `:` image table?
- What triggers `FUN_00454600`, and what are the meanings of its position and final arguments when it draws the TutorialIcon resource?
- What semantics distinguish objective-voice selectors `1`, `2`, and `3` in the `MainSimStep` callers?
- What do the two global fields `DAT_004ff978` and `DAT_004ff980` represent beyond their observed setter/getter and gate uses?
- What are the 55 tutorial coordinate-list category meanings, and which callback consumers use `CoordinateListCallbackVisit`?

Evidence is the checked-in canonical pseudocode plus the `EngineTick` caller preserved in `docs/decompiles/full/LEGORR.supplement.c` and `docs/decompiles/legacy/engine-region.c`; related findings above provide the captured caller, token, timer, and cleanup context. This was static analysis only; no game runtime or authored tutorial script was executed.
