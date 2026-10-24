# Finding 433 — Scene Objects, Effects, Motion, and Light State

Issue #184 assigns 26 procedures spanning entity-to-scene synchronization,
temporary attached objects, entity movement, configured light effects, and
selection/scene state. This finding connects those procedures to their callers
and earlier behavior maps. It keeps the meanings of unresolved flags, fields,
and callbacks literal.

**Target and evidence.** Static analysis of shipped `reference/LegoRR.exe`,
SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
Function bodies are in [`LEGORR.full.c`](../decompiles/full/LEGORR.full.c);
named legacy excerpts are in
[`engine-region.c`](../decompiles/legacy/engine-region.c). Direct body
observations are distinguished from caller-context interpretations below.

## Entity motion, scene nodes, and deferred cleanup

`EntityMotionToSceneNodeSync` (`0x0044bc30`) is the central entity-to-scene
update in this slice. It runs when entity flag `+0x3e4` bit `0x200000` is set.
Subject to the adjacent `0x400000` gate and a scene-object check, it advances a
scene-node timer by the frame delta, expires a command when the entity timer
ends, copies the entity's pose to the scene node, updates its scale and callback
state, and walks its child-object list to update transforms. The child-list
callbacks are also available as `SceneObjectListUpdateTransforms` and related
helpers in finding 380; the complete operation chain is in finding 246.

`EntityNodeScaleUpdate` (`0x0044bb30`), called by that sync path, derives
multiple values through entity-stat accessors. Its mode argument selects
different multiplication factors before the values are written to the node's
vector fields and published through the global vector/listener helper. The
available evidence establishes the formulas and writes, not the product
meaning of each scale channel (findings 381, 391, and 392).
`MatrixApplyRotationAboutZWithOffset` (`0x0044b7a0`) temporarily clears the
matrix translation, applies a quarter-turn rotation, restores the saved
components with the final component adjusted by the supplied offset, then
applies the matrix to the scene object. The matrix convention and offset's
domain remain unresolved (finding 380).

The teardown path preserves callback-backed scene objects when needed.
`EntityAuxiliaryPayloadRelease` (`0x0044be30`) immediately disposes the scene
object when entity bit `+0x3e4 & 0x08000000` is already set or allocation of a
deferred record fails. Otherwise it pushes a record containing the object,
entity flags, and a scale value onto global list `DAT_004eadd0`, then resets
part of the object's transform/callback state. The entity type-3 teardown
dispatch calls this helper (finding 84). `SceneObjectResourceAndCallbackStateDispose`
(`0x0044bae0`) invokes the object's callback-state walk, resets and releases
the resource at object `+0x98`, clears that field, and disposes the callback
state list (finding 377).

`GlobalTimedCallbackDispatch` (`0x0044bf00`) runs over that deferred list from
`EntityFrameUpdateDispatch` (`0x0043b270`). For each record it applies the
frame delta and saved callback state to the object, then calls
`CallbackOperationRunAndReturnStatus` (`0x00410280`). A zero result retains the
record; a nonzero result unlinks the record, disposes its scene object, and
frees the record. The callback operation's domain meaning is not established
by its return value alone (findings 10 and 377).

## Entity effects, movement, and selection

`ConfiguredMonsterSpawnBehaviorTick` (`0x0044bfd0`) is dispatched from the
entity flag-word behavior evaluator. When the entity timer at `+0x2d4` is
positive, it clears `+0x3e0` bit `0x00100000`, invokes the configured tiny
monster spawn/teardown helpers, raises `+0x3e8` bit `0x00800000`, and calls
`0x00462880`; otherwise it returns zero. The spawn helper's type-name
resolution, bounded retries, and route-command failure cleanup are detailed in
finding 257 (also finding 384).

The anonymous `FUN_0044c030` (`0x0044c030`) is a separate one-shot effect
attachment called by the type-3 branch of `EffectApplyByKindBranch`
(`0x0046f770`, finding 258). For type-3 entities whose `+0x3e4` bit
`0x08000000` is clear, it sets that bit, stores `param_2 * 25` at `+0x368`,
creates child kind `0x12` at the entity's position and orientation, starts its
`"Start"` scene action, sets child flag `+0x3e0` bit `0x100`, and links the
child back to its owner at `+0x364`. It returns one only on this creation path;
the child kind's semantic name is not established here. `AttachedEntityWake`
(`0x0044c110`) follows the reverse owner link: unless the owner has
`+0x3e0` bit `0x200000`, it writes `100.0` to owner `+0x340` and sets owner
`+0x3e8` bit `0x800000`. Finding 10 places this in the entity wake-timer path.

`EntityPairScalarFieldsSet` (`0x0044c150`) accepts entity kinds 1–3, calls a
pair-state helper, stores the supplied vector components and scalar at
`+0x3b4`, `+0x3b8`, and `+0x3bc`, and sets `+0x3e4` bit `0x4000`. The
simulation path consumes that state in `EntityHeadingStepMove`
(`0x0044c1b0`): it advances by the stored vector times frame delta, probes the
candidate grid position, and on a blocked step snaps the vector to a dominant
cardinal direction before retrying. Depending on the route/movement checks it
applies the position through the movement helper or clears the active bit; it
decrements the remaining scalar by the attempted step distance and clears the
bit when that value is exhausted. The setter is also used in effect-kind 2's
path (finding 258); the user-visible identity of this movement mode is
unresolved. `EntityCanDoOrRideCheck` (`0x0044c530`) is a type-2-only wrapper
around `0x0043c440`; finding 10 traces that probe to the permitted-activity
list and its drill/teleport/reinforce results.

`SelectionClearMarkerAndResetHud` (`0x0044c4a0`) may set entity marker bits
after checking selection and state words, then unconditionally queues the
selection-clear command and resets HUD context. The following command path
clears the active selection and transfers marker state (finding 77).
`GlobalScatterSpawnEnableAndDrain` (`0x0044c550`) is reached by the
UI/command dispatcher for case 9, value 2. It raises global bit `0x40`,
walks entities with callback `LAB_0044c5f0`, and updates/clears global state
`DAT_004eabc4` and `DAT_004eabc8` across repeated passes. The callback's exact
spawn-selection policy and the globals' meanings remain unknown from the
available decompile.

## Scene transform and configured light effects

`SceneObjectTransformStateInit` (`0x0044c710`) stores two object handles and
a position triple in globals, initializes current and configured channel
values from that triple, and applies the transform. `ConfiguredSceneTransformEnable`
(`0x0044c790`) sets global flag `DAT_004eaec8` bit `0x100` and reapplies the
configured transform when enabled; disabling it only clears that bit. Input
handlers call these helpers, while the exact feature represented by the
toggle is unresolved (finding 376). `ScenePositionResetAndEffectMotionCancel`
(`0x0044cf70`) copies the current owner transform back to the configured
position and clears bit `0x80` in the same global flag word (finding 385).

`LightEffectsConfigurationLoad` (`0x0044c7c0`) calls the blink, fade, and move
parsers in sequence; their return values do not gate later parsers. The blink
parser (`0x0044c7f0`) reads `LightEffects/BlinkRGBMax`,
`MaxChangeAllowed`, and `RandomRangeForTimeBetweenBlinks`, converts the
change limit to a 0–1 scale when nonzero, scales the interval endpoints by
25, and hands them to `LightEffectsBlinkParametersApply` (`0x0044c970`). That
sink stores the values and sets global enable bit `0x1`.

The fade parser (`0x0044c9c0`) requires `FadeRGBMin`, `FadeRGBMax`, and the
`RandomRangeForTimeBetweenFades`, `RandomRangeForFadeTimeFade`, and
`RandomRangeForHoldTimeOfFade` pairs. It scales the three time ranges by 25
and stores the twelve values through `LightEffectFadeParametersStore`
(`0x0044cc10`), which sets bit `0x4`. The move parser (`0x0044cca0`) reads
`RandomRangeForTimeBetweenMoves`, `RandomRangeForSpeedOfMove`,
`RandomRangeForDistOfMove`, and the three-value `MoveLimit`; it scales the two
time ranges by 25 and calls `LightEffectsMoveParametersStore`
(`0x0044cef0`), which also samples the current configured object transform
and sets bit `0x40`. These are direct parser/store relationships already
mapped in findings 245 and 383.

`FUN_0044cfa0` (`0x0044cfa0`) is called from the engine frame path with the
delta time. When global flag bit `0x100` is clear and the scene object handle
is nonzero, it advances the drift, waypoint, jitter, and three-channel tween
helpers, then calls `UiElementRgbColorSetClamped` (`0x0044cff0`). That sink
clamps three channel values to `[0,1]` and writes them to the configured
object with the fourth component set to `1.0`. The frame ordering and
unresolved channel meanings are described in finding 390; this static path
does not establish that each configured blink/fade value is consumed by this
particular tick routine.

## Open questions

- What do the remaining scene-node callback opcodes and callback completion
  status represent, and under which runtime conditions does deferred disposal
  complete?
- What are the precise ownership and lifecycle rules for the type-`0x12`
  attached object and its owner link?
- Which operation consumes each blink/fade/move enable bit, and what are the
  semantic names and units of the shared light-effect channels?
- What does the scatter callback choose or spawn, and what do its two global
  state words represent?

These are unresolved behaviors rather than bounded implementation defects;
this investigation creates no implementation follow-up ticket.

## Assigned function cross-reference

All 26 assigned addresses have existing generated function-area and coverage
ledger rows. This finding adds the README findings-index entry; no generated
index changes are required.

| Address | Assigned name | Existing behavior map |
| --- | --- | --- |
| `0x0044b7a0` | `MatrixApplyRotationAboutZWithOffset` | [380](380-scene-objects-terrain-and-matrix-helpers.md) |
| `0x0044bae0` | `SceneObjectResourceAndCallbackStateDispose` | [377](377-callback-registry-and-runtime-helpers.md) |
| `0x0044bb30` | `EntityNodeScaleUpdate` | [381](381-selection-resource-runtime-and-grid-helpers.md), [391](391-vector-geometry-and-pointer-state-primitives.md) |
| `0x0044bc30` | `EntityMotionToSceneNodeSync` | [246](246-entity-move-route-command-and-object-clone-helpers.md) |
| `0x0044be30` | `EntityAuxiliaryPayloadRelease` | [84](84-entity-teardown-dispatches-type-specific-payload-cleanup.md) |
| `0x0044bf00` | `GlobalTimedCallbackDispatch` | [10](10-entity-command-primitives.md), [377](377-callback-registry-and-runtime-helpers.md) |
| `0x0044bfd0` | `ConfiguredMonsterSpawnBehaviorTick` | [257](257-configured-tiny-monster-spawning-and-mutual-activity-pairing.md), [384](384-crt-formatting-com-cleanup-and-spawn-helpers.md) |
| `0x0044c030` | `FUN_0044c030` | [258](258-interface-method-wrappers-overlay-refresh-and-deferred-set-relays.md) |
| `0x0044c110` | `AttachedEntityWake` | [10](10-entity-command-primitives.md) |
| `0x0044c150` | `EntityPairScalarFieldsSet` | [258](258-interface-method-wrappers-overlay-refresh-and-deferred-set-relays.md) |
| `0x0044c1b0` | `EntityHeadingStepMove` | This finding |
| `0x0044c4a0` | `SelectionClearMarkerAndResetHud` | [77](77-selection-clear-marker.md) |
| `0x0044c530` | `EntityCanDoOrRideCheck` | [10](10-entity-command-primitives.md) |
| `0x0044c550` | `GlobalScatterSpawnEnableAndDrain` | This finding |
| `0x0044c710` | `SceneObjectTransformStateInit` | [376](376-selection-cell-and-overlay-state-helpers.md) |
| `0x0044c790` | `ConfiguredSceneTransformEnable` | [376](376-selection-cell-and-overlay-state-helpers.md) |
| `0x0044c7c0` | `LightEffectsConfigurationLoad` | [383](383-crt-global-accessors-and-light-effects.md) |
| `0x0044c7f0` | `LightEffectsBlinkConfigApply` | This finding |
| `0x0044c970` | `LightEffectsBlinkParametersApply` | [245](245-region-route-light-config-and-surface-helpers.md), [255](255-visual-channel-relays-resource-tree-callback-walks-scene-pose-and-registry-helpers.md), [383](383-crt-global-accessors-and-light-effects.md) |
| `0x0044c9c0` | `LightEffectsFadeConfigLoad` | [245](245-region-route-light-config-and-surface-helpers.md) |
| `0x0044cc10` | `LightEffectFadeParametersStore` | [245](245-region-route-light-config-and-surface-helpers.md) |
| `0x0044cca0` | `LightEffectsMoveConfigLoad` | [245](245-region-route-light-config-and-surface-helpers.md) |
| `0x0044cef0` | `LightEffectsMoveParametersStore` | [245](245-region-route-light-config-and-surface-helpers.md), [383](383-crt-global-accessors-and-light-effects.md) |
| `0x0044cf70` | `ScenePositionResetAndEffectMotionCancel` | [385](385-stream-surface-avi-and-route-state-helpers.md) |
| `0x0044cfa0` | `FUN_0044cfa0` | [258](258-interface-method-wrappers-overlay-refresh-and-deferred-set-relays.md), [390](390-motion-config-and-resource-utilities.md) |
| `0x0044cff0` | `UiElementRgbColorSetClamped` | [390](390-motion-config-and-resource-utilities.md) |
