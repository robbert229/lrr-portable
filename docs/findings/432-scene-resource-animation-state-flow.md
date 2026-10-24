# Finding 432 — Scene Resource and Animation State Flow

Issue #185 assigns 31 procedures connecting config parsing, scene resource slots, paired animation channels, entity activity changes, and attached-resource cleanup. The individual procedures are already mapped in several earlier findings; this finding joins their state flow and supplies a complete address-to-finding cross-reference. Field meanings and callback polarity remain literal where the available evidence does not resolve them.

## Configuration and scene-channel registration

`UpgradeLevelTableParse` (`0x0046c160`) reads the `Upgrades` config entries into a level-mask table and is called from building and vehicle definition parsing (finding 241). `VehicleTypeDefinitionParse` (`0x0046c6f0`) populates each vehicle-type record, including the model/wheel resources; the vehicle-type initializer then sets visibility and render-lock state for its scene handles (findings 225 and 409). `SceneRecordTemplateInstantiate` (`0x0046cff0`) copies the configured template, resolves and duplicates visual handles, initializes resource slots, and attaches listed child resources. `SceneAssignOwnerFromModel` (`0x0046d140`) links the scene object to its model owner (finding 28). These paths connect configuration records to the shared scene-object runtime described in findings 25 and 28.

`SceneActivityPairedChannelUpdate` (`0x0046c3f0`) coordinates the two activity/animation channels. It calls `SceneChannelSlotsRebuild` (`0x0046c510`), which clears parallel registration arrays and their count, walks the scene's registered entity list, derives each node's paired resource handles, applies the selected scene state, and either clears or re-registers the channel slots. The registered child and handle values are stored in parallel arrays on the scene record. The caller distinguishes reset from re-registration through its mode argument; the exact user-visible meaning of each channel remains unresolved (findings 35, 36, and 241).

`IndexedRecordListRebuildForValue` (`0x0046c340`) releases and rebuilds the linked list for matching indexed records. `RecordIndexedEntriesRebuildAndRefresh` (`0x0046cf90`) combines that rebuild with refresh of the associated slots. `RecordFieldD8EqualsOne` (`0x0046c3d0`) is a structural predicate on record field `+0xd8`; `RecordFlagBitTest` (`0x0046cf40`) tests the requested bit at `+0x130` subject to an associated mask; `EntityTypeMaskGet` (`0x0046cf80`) and `EntityTypeFlagGetBit1` (`0x0046d330`) expose type/mask state used by the neighboring dispatch paths (findings 357 and 367). These low-level predicates do not by themselves establish domain meanings for their fields.

## Resource slots, visibility, and animation

`SceneResourceSlotsRebuild` (`0x0046d260`) resolves or creates the six resource entries held by a scene record and stores them in the slot region beginning at `+0x28`. `SceneResourceVisibilitySet` (`0x0046d2c0`) changes raw bit `0x2` at `+0x1e8` and applies the visibility state to the primary handles and populated slots. `SceneResourceNamedEntryResolveWithFallback` (`0x0046d9f0`) tries a secondary resource handle first and falls back to the primary handle when the lookup fails. `SceneResourceCachedSlotB4Get`, `SceneResourceCachedSlotB8Get`, and `SceneResourceCachedSlotC0Get` (`0x0046da90/0x0046dac0/0x0046daf0`) lazily resolve and cache the corresponding named entries. The cached values are resource handles; their precise semantic names are not established by the field offsets (finding 367).

`SceneEntityAnimationTracksSetEnabled` (`0x0046cf20`) toggles the active argument on the primary and optional secondary animation tracks. The target body shows the argument forwarding, but the boolean polarity is not independently confirmed (finding 373). `AnimationDurationForLoopCount` (`0x0046d1c0`) reads a duration from one or two resource handles and adjusts it for repeated loops (finding 370). `SceneResourceDrawAtOrigin` (`0x0046d350`) forwards the resource handle to the scene-object transform path with fixed origin and rotation inputs. `AttachedModelOrientationUpdate` (`0x0046d380`) derives orientation from model geometry and applies it to attached objects (findings 238 and 379).

## Ownership and activity transitions

The resource ownership path includes several separate operations. `SceneObjectResourceGet` (`0x0046d9e0`) exposes the resource handle held by a scene object. `PairedResourceStateSet` (`0x0046ced0`) applies one state update to a resource/object pair and repeats it for the optional second pair; `SixResourceSlotsSetState` (`0x0046cfc0`) forwards the supplied state across six populated slots (finding 354). `EntityPayloadType1Release` (`0x0046ce10`) releases the payload-specific resource during entity teardown (findings 03 and 84). `SceneObjectAttachedResourceRelease` (`0x0046d1a0`) releases the object-attached resource, while `SceneObjectResourceRelease` (`0x0046db70`) releases and clears the scene-resource handle (findings 372 and 374). `AttachedEntitySlotObjectGet` (`0x0046db20`) resolves the object stored in an attached-entity slot, and `ObjectRecordField11CGet` (`0x0046db60`) returns the literal field at `+0x11c` (findings 351 and 372).

`EntityActivityChangeDispatch` (`0x0046da50`) dispatches the activity transition into the scene-channel update path. The adjacent `SceneActivityUpdateDispatch` call sites set or clear several entity bits and may re-register scene slots; those call paths are described in findings 35, 70, 241, and 251. This establishes the static ordering between activity change and resource/slot updates, but not the runtime timing of every animation callback or the full semantic contract of each activity code.

## Unresolved questions

- What are the semantic names and consumers of the cached resource fields `+0xb4`, `+0xb8`, and `+0xc0`?
- What does each paired animation channel represent, and which runtime conditions choose clear versus re-register mode?
- Does the animation-track enable argument use the same boolean polarity at every caller?
- Which constructor and teardown paths own the duplicated resource handles, especially when template instantiation fails partway through?

These questions do not yet identify a bounded implementation slice; this pass creates no implementation follow-up issue.

## Assigned function cross-reference

All 31 assigned addresses are below, linked to existing behavior maps and evidence. Generated function-area and coverage-ledger rows already existed for the assigned addresses; the README findings index/range is the only index updated here.

| Address | Function | Existing behavior map |
| --- | --- | --- |
| `0x0046c160` | `UpgradeLevelTableParse` | [241](241-terrain-quad-run-upgrade-and-scene-slot-helpers.md) |
| `0x0046c340` | `IndexedRecordListRebuildForValue` | [367](367-scene-resources-entity-predicates-and-offset-accessors.md) |
| `0x0046c3d0` | `RecordFieldD8EqualsOne` | [367](367-scene-resources-entity-predicates-and-offset-accessors.md) |
| `0x0046c3f0` | `SceneActivityPairedChannelUpdate` | [35](35-ddscene-paired-channel-update.md), [36](36-ddscene-activity-name-cache.md), [70](70-activity-change-dispatch-and-consumption.md), [241](241-terrain-quad-run-upgrade-and-scene-slot-helpers.md) |
| `0x0046c510` | `SceneChannelSlotsRebuild` | [241](241-terrain-quad-run-upgrade-and-scene-slot-helpers.md), [372](372-incident-messages-options-and-resource-helpers.md), [250](250-config-band-loaders-cell-corner-scan-action-command-record-instantiation-and-render-state-helpers.md), [254](254-command-sweeps-cell-issuing-config-priorities-statistics-and-directdraw-create-helpers.md) |
| `0x0046c6f0` | `VehicleTypeDefinitionParse` | [225](225-terrain-objective-level-and-diagnostic-helpers.md), [409](409-game-configuration-entity-type-and-object-name-tables.md), [241](241-terrain-quad-run-upgrade-and-scene-slot-helpers.md), [249](249-config-band-loaders-poly-variant-build-panel-and-cell-registration-layout-and-activity-helpers.md), [252](252-config-band-texture-loaders-route-commands-and-surface-menu-helpers.md) |
| `0x0046ce10` | `EntityPayloadType1Release` | [03](03-engine-tick.md), [84](84-entity-teardown-dispatches-type-specific-payload-cleanup.md) |
| `0x0046ced0` | `PairedResourceStateSet` | [354](354-record-vector-and-resource-state-helpers.md), [258](258-interface-method-wrappers-overlay-refresh-and-deferred-set-relays.md) |
| `0x0046cf20` | `SceneEntityAnimationTracksSetEnabled` | [373](373-cursor-simulation-renderer-and-file-helpers.md) |
| `0x0046cf40` | `RecordFlagBitTest` | [367](367-scene-resources-entity-predicates-and-offset-accessors.md) |
| `0x0046cf80` | `EntityTypeMaskGet` | [357](357-entity-fields-and-global-counter-helpers.md) |
| `0x0046cf90` | `RecordIndexedEntriesRebuildAndRefresh` | [367](367-scene-resources-entity-predicates-and-offset-accessors.md), [241](241-terrain-quad-run-upgrade-and-scene-slot-helpers.md) |
| `0x0046cfc0` | `SixResourceSlotsSetState` | [354](354-record-vector-and-resource-state-helpers.md), [251](251-command-band-kind-dispatch-item-name-lookup-route-and-record-helpers.md) |
| `0x0046cff0` | `SceneRecordTemplateInstantiate` | [250](250-config-band-loaders-cell-corner-scan-action-command-record-instantiation-and-render-state-helpers.md), [410](410-map-and-visual-resource-records.md), [253](253-terrain-scatter-erosion-propagation-cell-queries-menu-records-and-buffer-helpers.md) |
| `0x0046d140` | `SceneAssignOwnerFromModel` | [28](28-ddscene-entity-ownership.md) |
| `0x0046d1a0` | `SceneObjectAttachedResourceRelease` | [372](372-incident-messages-options-and-resource-helpers.md) |
| `0x0046d1c0` | `AnimationDurationForLoopCount` | [370](370-screen-effects-surface-and-ui-helpers.md) |
| `0x0046d260` | `SceneResourceSlotsRebuild` | [367](367-scene-resources-entity-predicates-and-offset-accessors.md), [250](250-config-band-loaders-cell-corner-scan-action-command-record-instantiation-and-render-state-helpers.md), [254](254-command-sweeps-cell-issuing-config-priorities-statistics-and-directdraw-create-helpers.md) |
| `0x0046d2c0` | `SceneResourceVisibilitySet` | [367](367-scene-resources-entity-predicates-and-offset-accessors.md), [409](409-game-configuration-entity-type-and-object-name-tables.md) |
| `0x0046d330` | `EntityTypeFlagGetBit1` | [357](357-entity-fields-and-global-counter-helpers.md) |
| `0x0046d350` | `SceneResourceDrawAtOrigin` | [367](367-scene-resources-entity-predicates-and-offset-accessors.md), [250](250-config-band-loaders-cell-corner-scan-action-command-record-instantiation-and-render-state-helpers.md) |
| `0x0046d380` | `AttachedModelOrientationUpdate` | [238](238-reward-terrain-and-simulation-helpers.md), [379](379-terrain-ui-and-crt-support-helpers.md), [242](242-path-step-terrain-and-pending-record-helpers.md), [244](244-config-file-text-layout-and-record-emit-helpers.md) |
| `0x0046d9e0` | `SceneObjectResourceGet` | [357](357-entity-fields-and-global-counter-helpers.md) |
| `0x0046d9f0` | `SceneResourceNamedEntryResolveWithFallback` | [367](367-scene-resources-entity-predicates-and-offset-accessors.md), [241](241-terrain-quad-run-upgrade-and-scene-slot-helpers.md), [254](254-command-sweeps-cell-issuing-config-priorities-statistics-and-directdraw-create-helpers.md) |
| `0x0046da50` | `EntityActivityChangeDispatch` | [70](70-activity-change-dispatch-and-consumption.md) |
| `0x0046da90` | `SceneResourceCachedSlotB4Get` | [367](367-scene-resources-entity-predicates-and-offset-accessors.md) |
| `0x0046dac0` | `SceneResourceCachedSlotB8Get` | [367](367-scene-resources-entity-predicates-and-offset-accessors.md) |
| `0x0046daf0` | `SceneResourceCachedSlotC0Get` | [367](367-scene-resources-entity-predicates-and-offset-accessors.md), [256](256-command-band-step-issuers-flag-behavior-timers-and-steering-helpers.md) |
| `0x0046db20` | `AttachedEntitySlotObjectGet` | [351](351-text-output-task-table-and-indexed-record-helpers.md), [247](247-config-graphics-camera-tween-and-pool-spawn-helpers.md) |
| `0x0046db60` | `ObjectRecordField11CGet` | [372](372-incident-messages-options-and-resource-helpers.md) |
| `0x0046db70` | `SceneObjectResourceRelease` | [374](374-terrain-interface-and-media-helpers.md) |
