# Finding 438 — Front-End Flow, Save Slots, and Option State

This finding follows the assigned front-end functions from menu construction through save/load selection, level start, and option-state transfer. It complements [Finding 23](23-front-end-and-menu.md), which documents the menu grammar and boot path, and [Finding 66](66-front-end-options-round-trip-through-six-save-contexts.md), which traces option fields through six save contexts. The descriptions below distinguish operations visible in checked-in pseudocode from conclusions based on callers or field names; unresolved field meanings remain open.

## Menu setup and frame flow

`LoadMenuBuild @ 0x004161f0` is the front-end assembly point: it reads optional wipe configuration, initializes the save/load text and panel layout, copies prompt strings, builds the main and save menus, constructs campaign/tutorial chains, fills save-menu records, then builds paused and options menus. It initializes the three option globals to `1`, `8`, and `5`, derives the initial SFX and music values, and reads screen-count and thumbnail-size settings. The layout stage allocates two `0x728`-byte text-buffer records: `DAT_00557594` for save-menu text and `DAT_00557598` for level-menu text. `MenuHoverStatusTextDraw` resets and appends to the first buffer while drawing save-slot hover status. `MenuBuild @ 0x004140c0` parses each menu's configuration into menu/item records; its grammar and item variants are detailed in Finding 23.

`MenuShow @ 0x00413e20` clears the close flag (`+0x28`) for each menu in the selected menu set, then calls `MenuRunLoop @ 0x00413b80` for the requested menu. The loop initializes thumbnail resources and its transition/input globals, ticks the menu with `MenuTick @ 0x00412c00`, updates scrolling and rendering, and continues until the active menu's close flag changes. `HasType6ItemAtIndex @ 0x004114f0` checks that an indexed item exists, has type 6, and passes the menu-coordinate helper; `HasActiveResource @ 0x004119d0` requires a non-null resource at item offset `+0x38` and a nonzero result from `SystemStateFlags(0)`.

## Assigned-function map

The table summarizes all 31 functions assigned to Issue #171. “Observed” refers to calls, branches, accesses, or constants visible in the checked-in pseudocode. Names that imply user-facing meanings remain provisional where the data flow does not establish them.

| Address | Function | Observed responsibility and flow |
| --- | --- | --- |
| `0x004105a0` | `CampaignLevelFallbackLabel` | Allocates a string-sized buffer and copies the supplied NUL-terminated string. `LevelChainBuild` uses it when a level has no configured `FullName`; it is a copy helper, not itself a config lookup. |
| `0x00410740` | `SaveMenuListEntryAppend` | Grows a list of `0x20`-byte entries and associated index buffers. Its bitmap-list path splits three column names and loads their resources; the plain-entry path copies the label and stores text/column metadata. See [Finding 240](240-cursor-interaction-overlay-menu-and-scene-light-helpers.md). |
| `0x004114f0` | `HasType6ItemAtIndex` | Checks the indexed item’s type field (`+0x14 == 6`) and asks the menu-coordinate helper whether the item is active at the current pointer position. |
| `0x004119d0` | `HasActiveResource` | Returns true only when the passed record has a non-null resource at `+0x38` and `SystemStateFlags(0)` is nonzero. |
| `0x00412c00` | `MenuTick` | Advances the selected menu’s input/state and draw work for one frame; it uses the widget hit tests and callbacks described in Findings 23 and 437. |
| `0x00413a60` | `SaveMenuThumbnailConfigLoad` | Loads the configured save-image resources and scans five configured save records for thumbnail-bearing entries after the first eight, accumulating four units per nonzero thumbnail field. |
| `0x00413b80` | `MenuRunLoop` | Initializes menu-loop state, repeatedly calls `MenuTick`, updates scroll/render/audio state, and waits for the selected menu’s close flag. A menu transition starts the configured wipe path. |
| `0x00413e20` | `MenuShow` | Clears each menu item's close flag, selects the requested menu record, and starts the modal loop. |
| `0x004140c0` | `MenuBuild` | Reads `MenuCount` and each `Menu%i` configuration, then constructs menu records, items, resources, and callbacks. See Finding 23 for the observed grammar. |
| `0x00414ce0` | `AvisEnabled` | Enables configured intro AVI playback only when `DontPlayAvis` is false and the current mode value is below 3. |
| `0x00414d30` | `BrightnessSet` | Scales the supplied integer by `0.1` and applies derived grayscale/gamma values to the two display surfaces. |
| `0x00414de0` | `GameSpeedSelect` (assigned name) | Calls `SoundVolumeIndexSet @ 0x0047b530`, which maps indices 0–10 through the DirectSound attenuation table. This is the SFX volume callback according to its direct callee and menu binding, despite the assigned function label; see [Finding 156](156-options-sfx-volume-uses-nonlinear-directsound-table.md). |
| `0x00414df0` | `VolumeSet` | Divides the supplied integer by 9 and passes the same normalized value to both channels of the music mixer. The mixer clamps its input; see [Findings 157](157-music-volume-slider-collapses-top-two-values.md) and [159](159-music-volume-targets-compact-disc-mixer-source.md). |
| `0x004150b0` | `SaveMenuRefresh` | Resets the current slot/tree context, then rebuilds the campaign and tutorial selection trees from their respective level arrays. |
| `0x00415150` | `OptionsApplyFromSlot` | Resolves the current save context and, if present, sends fields `+0x08`, `+0x10`, and `+0x04` to the SFX, brightness, and music callbacks. The producer/meaning of the `+0x04` field remains unresolved in Finding 66. |
| `0x00415220` | `FrontEndStartLevel` | Formats the pending level name, starts transition/load helpers, calls `LevelLoad`, marks all main-menu items closed, clears the options flag, sets the in-game state bit, and runs a follow-up state helper. |
| `0x004154b0` | `LevelChainBuild` | Follows `NextLevel` links and allocates parallel name/display arrays. It copies a level key when `FullName` is absent; when `FullName` exists, underscores in that display string are replaced with spaces. |
| `0x00415a10` | `SaveMenuBuild` | Allocates campaign/tutorial tree records, creates save/load list controls and six slot summaries, appends configured entries, then registers callbacks to populate the level and tutorial trees. |
| `0x00415d00` | `SaveMenuLayoutLoad` | Copies configured label strings into a fixed layout record and, when valid rectangles/resources are present, creates the window surface and panel image with their coordinates. |
| `0x004161f0` | `LoadMenuBuild` | Loads menu-wipe and save/load layout configuration, copies prompt text, builds the main/save/paused/options menus, calls `LevelChainBuild` twice and `SaveMenuBuild`, then initializes option globals and display/thumbnail settings. |
| `0x00416910` | `OptionsWriteToSlot` | Writes three runtime option globals into the current save context at offsets `+0x0c`, `+0x08`, and `+0x10`; Finding 66 traces their round trip. |
| `0x00416940` | `OptionsLoadOrDefault` | Optionally clears front-end selection flags; when asked to load options it reads the current slot context if available, otherwise sets the three tracked globals to `1`, `8`, and `5`. |
| `0x004169c0` | `FrontEndStateChange` | Switches among main, new-game, load, options, and quit states. Cases 0–2 reset options and rebuild campaign/tutorial trees; case 3 resets selection state for options; case 4 prepares quit-flow selection. Each path refreshes the front-end display. |
| `0x00416d00` | `FrontEndActive` | Returns true only when the `FrontEnd` config flag is true and the current mode value is less than 3. |
| `0x00416d60` | `IsInGame` (assigned name) | Directly tests whether `DAT_00557024 != -1`. `FrontEndSelectedAction` uses this as the sentinel for a selected tutorial/load entry, so the assigned name should not be read as proof of a general in-game state. |
| `0x00416d70` | `FrontEndSelectedAction` | If a save slot is open, maps its selection through the campaign tree; otherwise, when `DAT_00557024` is not `-1`, maps through the tutorial tree. Returns the associated level name or null. |
| `0x00416dd0` | `QuitMenuProbe` | Returns the quit-request global. |
| `0x00416de0` | `LevelStartFlag` | Tests the nonzero level-start/advance flag. |
| `0x00416df0` | `MenuOptionsFlag` | Tests the nonzero options-overlay flag. |
| `0x00417f20` | `OptionsContextSixFieldSet` | Copies six dwords from its argument into the current context at offsets `+0x1c` through `+0x30`. Field semantics are not established by this copy alone. |
| `0x004180c0` | `OptionsContextCopyAndReplaceResource` | Frees the current context resource at `+0x34` when present, copies `0x2e` dwords from the supplied record, then stores its resource pointer from source offset `+0x34`. |

## Flow and data boundaries

`LevelChainBuild` is the bridge from configured level progression to front-end rows: it follows `StartLevel` and each `NextLevel`, retaining the config records and display strings in separate arrays. `SaveMenuBuild` uses those arrays for the visible save/load controls. `SaveMenuRefresh` rebuilds the trees when front-end state changes, and `FrontEndSelectedAction` resolves the selected row back to a level key. `FrontEndStartLevel` then passes that pending key to the existing level-loading path. These operations are visible in pseudocode; the user-facing labels and behavior of opaque flags are inferences where the table identifies them.

Option values move through per-slot contexts rather than a demonstrated global-preferences file: `OptionsLoadOrDefault` reads the current context or applies defaults; the menu callbacks apply values; `OptionsWriteToSlot` writes the tracked globals back to the context; `OptionsApplyFromSlot` reapplies context fields on the load path. Findings 66 and 67 trace the context and save serialization. The six copied context dwords and the resource pointer are raw record operations; their full semantics remain open.

## Open questions

- What produces the slot-context field consumed at `+0x04` by `OptionsApplyFromSlot`, and does it represent a music value distinct from the three globals written by `OptionsWriteToSlot`?
- Is `DAT_00557024` exclusively the tutorial/load selection sentinel, and which event sets or clears it on every front-end path?
- How do the thumbnail capacity/count fields relate to the six slot records and entries after slot eight?
- The assigned labels for `0x00414de0` (`GameSpeedSelect`) and `0x00416d60` (`IsInGame`) do not match the direct callback/state evidence summarized above; their intended naming provenance is unknown.

Evidence is the checked-in pseudocode for the cited functions and adjacent callers. Findings 23, 66, 156–160, and 240 provide deeper traces for the menu grammar, option context, sound callbacks, and save-list records. No user interaction or successful save/load session was observed as part of this static analysis.
