# Finding 443 — Help, Tutorial, and Interface Resources

This finding maps the 24 procedures assigned by Issue #168 across several related UI paths: level/tutorial helpers, the HelpWindow, configured interface imagery, the advisor overlay, and tutorial/tooltip resources. These procedures share configuration and presentation state, but do not form one single runtime sequence.

**Target and evidence.** Static analysis of shipped `reference/LegoRR.exe` (SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`); bodies and call instructions are in [`LEGORR.full.c`](../decompiles/full/LEGORR.full.c). Existing focused context: findings [23](23-front-end-and-menu.md), [241](241-terrain-quad-run-upgrade-and-scene-slot-helpers.md), [244](244-config-file-text-layout-and-record-emit-helpers.md), [245](245-region-route-light-config-and-surface-helpers.md), [247](247-config-graphics-camera-tween-and-pool-spawn-helpers.md), [249](249-config-band-loaders-poly-variant-build-panel-and-cell-registration-layout-and-activity-helpers.md), [250](250-config-band-loaders-cell-corner-scan-action-command-record-instantiation-and-render-state-helpers.md), [252](252-config-band-texture-loaders-route-commands-and-surface-menu-helpers.md), [281](281-advisor-terrain-task-helper-names.md), [375](375-entity-ui-and-render-tree-helpers.md), [381](381-selection-resource-runtime-and-grid-helpers.md), and [429](429-advisor-and-message-ui-state.md). These include retained evidence records where available; this pass also checked each assigned body in the complete pseudocode corpus.

## Level, tutorial, and HelpWindow paths

| Address | Function | Observed behavior |
| --- | --- | --- |
| `0x00409300` | `TutorialPathDescriptorCreate` | If `FUN_00433090` reports no existing descriptor for the supplied object, allocates and links a `0xd4`-byte path record through `FUN_00409350`, sets mask `0x2` at record offset `+0xd0`, calls `FUN_0041cfb0` with the object, and returns success. The descriptor's complete purpose is not established by this body. |
| `0x00418000` | `ShowLevelOverlayForCurrentSelection` | Gets the current selected level through `FUN_00417e90(0)`, passes it to `FUN_00417bd0`, then calls `FUN_004173e0` with the current overlay/resource context. The caller is `FUN_00462030`; the descriptor's exact UI role remains unclear (finding 381). |
| `0x004181b0` | `HelpWindowResourcesInit` | Orchestrates HelpWindow setup: loads info strings, optional background bitmap and its dimensions, button layout/resources, and TextWindowArea geometry; publishes three build/transport message strings with `_` converted to spaces; sets state bits `1` and `2`, then calls `FUN_00419200(0,0)`. |
| `0x00418450` | `HelpWindowInfoConfigStringsLoad` | Builds `HelpWindow::HelpWindowInfo::<catalog-name>L<index>` keys for three catalog tables and stores up to 16 text entries per catalog, replacing underscores with spaces (finding 244). |
| `0x004185f0` | `HelpWindowButtonLayoutLoad` | Parses Continue and CancelFuture button records: four layout numbers, two image handles, and a text-entry value. The text values are sent to UI text slots 8 and 9; the button hit-test uses the resulting rectangles (finding 249). |
| `0x00418800` | `ModeFlagPairApplyIfBit2` | When state bit `4` is set, sets/clears the low two bits of `DAT_004dc650` according to its argument. |
| `0x00418da0` | `HelpWindowButtonHitTest` | While state bits `1` and `2` are both set, clears bits `4` and `5`, checks the special corner region, then checks the two configured button rectangles (Continue and CancelFuture). A hit records the button index and state bit `0x10`, requests the mode transition, and may update feedback/action sound; the special region requests timed effect `0x21` (finding 250). |
| `0x00418fc0` | `UiModeTransitionRequest` | Updates the current mode (`DAT_004dc64c`) subject to the supplied condition and current mode; sets event bit `0x20` on accepted transitions and returns whether the prior/requested mode matched. The mode values' broader names are unresolved (finding 374). |
| `0x00419030` | `HelpWindowButtonFeedbackUpdate` | Button indices 0 and 1 invoke reset/toggle helpers; indices 2 and 3 adjust a shared interpolation value by `param_2 * 0.2`, clamped at its endpoints, with index 3 invoking an additional reset helper at the endpoint (finding 375). |
| `0x00419100` | `UiModeResourcePositionsApply` | Iterates the two button slots, asks `FUN_00419150` for the active resource handle, and applies the slot's stored coordinate pair when a handle is available. |
| `0x004191f0` | `UiModeResourceBit40Get` | Returns bit `0x40` of `DAT_004dc650`. |
| `0x00419200` | `UiModeResourceBit40SetClear` | Sets or clears bit `0x40`; a nonzero first argument preserves the current bit as the operation input, while a zero first argument uses the second argument. |

The direct HelpWindow path is `HelpWindowResourcesInit` → info-string and button-layout loaders → `HelpWindowButtonHitTest` during interaction. `HelpWindowButtonHitTest` and `HelpWindowButtonFeedbackUpdate` implement different parts of the interaction: hit selection and state transitions versus visual/scroll feedback. The raw state bits have only local, body-supported meanings here.

## Interface images and runtime advisor overlay

| Address | Function | Observed behavior |
| --- | --- | --- |
| `0x0041ab00` | `InterfaceBuildImagesConfigLoad` | Allocates parallel image-handle arrays for the two catalog groups, loads three configured surfaces per present entry, clears the three slots for a missing entry, and records optional catalog indices. `InterfaceImagesConfigLoad` calls it as a fallback (finding 245). |
| `0x0041ada0` | `InterfaceImagesConfigLoad` | If the `InterfaceImages` section is absent, calls the build-image and back-button loaders. Otherwise it resolves each image name to a fixed slot, parses its multi-field value, loads the configured image resources, and stores associated flags and optional values (finding 244). |
| `0x0041afd0` | `InterfaceSurroundImagesConfigLoad` | Iterates numbered `InterfaceSurroundImages` entries; for each non-`NULL` side, loads a bitmap, initializes it, and stores its handle and parsed dimensions in parallel tables (finding 247). |
| `0x0041b1b0` | `InterfaceBackButtonConfigLoad` | Parses the back-button rectangle, two resources, and an action ID; loads the handles and initializes UI text entry 5 (finding 374). |
| `0x0041b270` | `InterfaceButtonImagesLoad` | Loads each supplied image resource and initializes the corresponding non-null UI image handle (finding 374). |
| `0x0041b490` | `AdvisorOverlayUpdate` | Returns unless overlay state bit `4` is set. It selects the active catalog/overlay variant, positions its resource, updates interface imagery and Advisor animation/request state, clears related input flags, dispatches catalog callbacks, draws the position tracker when requested, and either lays out stacked dialogs or resets the interaction timer. The precise meanings of the controlling state bits remain partly unresolved (findings 249, 251, 252, and 429). |
| `0x0041ba10` | `FUN_0041ba10` | If a pending value differs from the last value, records it, resets a 25-unit timer and clears the pending field. Otherwise counts the timer down; after expiry, if its event field is clear, calls `FUN_00464ef0` with the pending value, then clears that field. The semantic names and units are unknown. The engine-tick caller is documented in finding 03. |
| `0x0041c1c0` | `AdvisorOverlayHitResolve` | First checks the selected overlay's rectangular bounds; a hit returns category 0. Otherwise dispatches through `FUN_0041b680` and copies the resolved callback result out. Its caller is `SimulationPointerActionDispatch` (finding 252). |
| `0x0041eca0` | `AdvisorDialogStackLayout` | When the level/position pair changes, resets the related timer/flag state. With overlay bit `0x800` set and a nonempty dialog list, places the header and dialog text records vertically, accounting for attached text and advancing a line counter for attached dialogs (finding 249). |

`AdvisorOverlayUpdate` is the runtime coordinator in this group, while interface-image loaders and the `AdvisorDialogStackLayout` helper supply resources and layout. The complete update uses helpers outside the assigned set, so this grouping reflects observed calls and shared globals rather than a claim that these are the only participants.

## Tooltip, tutorial icon, and spawn requests

| Address | Function | Observed behavior |
| --- | --- | --- |
| `0x00427e00` | `ToolTipInfoStringsLoad` | Copies nine `ToolTipInfo` config values into tooltip globals: `HealthText`, `EnergyText`, `CrystalsText`, `OreText`, `StudsText`, `ToolsText`, `CarryObjectText`, `DrivenByText`, and `OreRequiredText`. `GameConfigInit` calls this loader; finding 250 is corrected in this change to include the ninth value. |
| `0x00434a00` | `AdvisorTutorialIconImageLoad` | Reads `TutorialIcon`, loads a bitmap handle when the config entry exists, and initializes the loaded image; called during advisor UI initialization (finding 281). |
| `0x00434e80` | `InterfaceSpawnRequestEnqueue` | Clears request bit `0x80`, updates HelpWindow mode state, and for type 4 or 11 stores the request and derives one of four orientation values from a vector. Other requests are routed by entity flags `0x1000`, `0x800`, `0x2000`, or `0x400000` to distinct request handlers. The handler meanings and ownership/retry behavior are not established by this entry point alone. |

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| HelpWindow initialization calls the string and button loaders and publishes message strings/state | observed (`0x004181b0`) |
| Button hit testing uses configured rectangles and updates selected button/mode state | observed (`0x00418da0`) |
| Interface image loading has a section-present path and a fallback path | observed (`0x0041ada0`) |
| Advisor overlay update gates on state and invokes positioning, request, callback, tracker, and dialog helpers | observed complete body (`0x0041b490`) |
| Tooltip loader reads nine named config values | observed complete body (`0x00427e00`) |
| Semantic names for most state bits/modes, timer units, and some parsed resource fields | unknown |
| Full semantic purpose of the tutorial path descriptor and spawn-request handlers | unknown |

Focused follow-up slices: identify the raw HelpWindow mode/state bits from their complete read/write callers; trace `TutorialPathDescriptorCreate` into the path-record consumers; and resolve each `InterfaceSpawnRequestEnqueue` destination and its request lifetime.

Coverage note: all 24 assigned functions already have generated function-area and coverage-ledger rows. This finding updates the root README findings range/index only; generated coverage indexes need no changes.
