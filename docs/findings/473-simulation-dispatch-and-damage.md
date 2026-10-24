# Finding 473 — Simulation Dispatch and Damage

This finding maps the 21 procedures assigned to simulation-dispatch-damage. The procedures cover a queued entity update path, a bounded spawn-request queue, damage and low-energy updates, target routing, event dispatch, spatial callback queries, and small entity/tile adapters. Unless marked otherwise, behavior descriptions are direct observations of the pseudocode. Terms such as “energy-like” are explicitly tentative interpretations; opaque flags and predicates remain unresolved.

Caller traces connect the helpers to broader flows: `EntityStateReset @ 0x00441b30` calls `SimulationEntityQueueFlush @ 0x0043eea0` during a flagged cleanup path; `InterfaceSpawnRequestEnqueue @ 0x00434e80` calls `EntitySpawnRequestQueueAdd @ 0x0043f150` to enqueue request pairs; and `EntityEnergyLowTargetRouteCommandTick @ 0x0043f7d0` calls `EntityDamageApply @ 0x0043f6a0` with computed damage. `EntityCommandUnlinkByKind @ 0x00403d10`, `OListEntityRecordParse @ 0x0042e020`, and `DeferredOListEntityCreateAndLink @ 0x00437940` call `EntityLinkedActivityEventDispatch @ 0x0043f5b0`, showing that event masks reach the same operation dispatcher from cleanup, record parsing, and deferred creation. `TutorialPathMenuAction @ 0x0041d040` consults `EntityActionTargetCellIsValid @ 0x0043fdc0`; `UiActionDispatch @ 0x0041dca0` invokes `TerrainCellToWorldPosition @ 0x004401b0` for operations 0x19 and 0x1a. These are direct call sites in the canonical pseudocode.

## Entity queue and spawn requests

`SimulationEntityQueueFlush @ 0x0043eea0` walks records attached to an entity, clears their back-pointer, then takes one of two paths. Depending on entity flag tests, it clears record state and either releases kind-7 records or consumes records through `EntityActionRecordConsumeAndCleanup`; in the other branch it links each record into the referenced entity's +0x308 array, assigns its index/back-pointer, sets bit 0x400, and queues operation 0x1a. It clears the source list and selected state at the end. `LinkedEntityRouteFlagClear @ 0x0043f100` clears bit 0x40 from an entity and its +0x2fc target when present.

`EntitySpawnRequestQueueAdd @ 0x0043f150` stores a pair of request values in parallel global arrays while the pending count is below 9, returning success; at capacity it returns failure. `EntitySpawnRequestQueueProcess @ 0x0043f190` only processes a request when the receiver passes a predicate, has no active +0x3cc child (or that child lacks bit 0x200000), and has neither relationship bit 0x40 nor 0x80 set. It scans queued request pairs for a type-specific mask match and checks capacity and a class comparison. On a match it resolves a new entity record, creates it, associates it at +0x3cc, updates resource counts, sets reciprocal relationship bits and the child's +0x2fc link, copies position/orientation, initializes its pose, sets child bit 0x200000, requests activity 0x4e, and updates scene state. It removes the consumed queue entry by shifting later entries. The request mask meanings and entity types are unresolved.

`EntityPoolFindWithLocalPredicate @ 0x0043f560` is a wrapper that dispatches `LAB_0043f580` through the entity traversal helper. The predicate body is outside the assigned range, so the search criterion cannot be established here.

## Damage, energy, and events

`EntityDamageApply @ 0x0043f6a0` applies damage only when `FUN_004385b0(entity, 1)` succeeds and +0x304 is null. It has a kind-4 full-health special case, kind-specific stat updates, subtracts damage from health at +0x340, and for positive damage with the third argument set, sets state bits, adds to a damage accumulator, and updates a record. A helper-derived bit can trigger a global counter update. This routine does not clamp health itself.

`EntityEnergyLowTargetRouteCommandTick @ 0x0043f7d0` skips kinds 5 and 10. Otherwise it decreases health/energy-like fields using helper-provided rates, handles a terrain-type-6 transition for kinds 6 and 7, applies damage returned by the cell/entity check, and calls a helper for another state-gated condition. For kind 2 it may search four neighboring cells around a related entity and issue an operation when a route exists and its timers/flags pass. It then bounds +0x3d4 by +0x340, calls `DamageHurtSoundSubmit`, and clears or consumes pending damage flags. The names of the two decremented fields and helper predicates remain uncertain.

`DamageHurtSoundSubmit @ 0x0043fb40` is kind-2-only and requires a nonzero damage amount. If the hurt sound resource exists, it starts playback when its timer is zero; otherwise it advances that timer by 4% of the frame delta and clears the timer and stored duration when it exceeds twice the duration. The function returns 1 in all paths.

`EntityLinkedActivityEventDispatch @ 0x0043f5b0` is kind-2-only. It ORs the event mask into the entity field +0x3f0 and a global mask, then gives the lowest matching bit precedence: bits 1, 2, 4, 8, 0x10, and 0x20 dispatch operations 0x17, 0x18, 0x14, 0x15, 0x16, and 0x19 respectively. For the first five it resets +0x3d4 and returns; the sixth also resets +0x3d4.

`RecordPairPoseCopyAndActivityRaise @ 0x0043fc20` checks the linked record at +0x304, obtains a pose for that record, and requires the derived cell to pass a terrain predicate. If both entities have transforms, it clears their reciprocal links and the record entity's state/count, copies position and orientation, updates/registers the record entity, requests activity 0x34 on the source, registers it, then links the source to the record and sets source bit 0x8000. It returns whether this transition occurred.

## Target checks and route setup

`EntityActionTargetCellIsValid @ 0x0043fdc0` requires the entity capability bit 1, entity flag 0x400, and a +0x308 target whose kind is 1. It resolves the target pose to a cell and returns whether that cell passes the route/terrain predicate.

`EntityRouteToExplicitTarget @ 0x0043fe70` requires source capability bit 2, target capability bit 1, and no existing source +0x304 record. It resolves both cells; when they differ it requests a path, while matching cells use a one-cell route. Both cases call `EntityRouteSetupAndMarkerAssociate`. On success it stores the explicit target at +0x2fc, writes step kind 0x15 to the final route entry, updates the target record, and returns 1. `EntityRouteMarkerSetup @ 0x0043fff0` instead obtains a target through `FUN_004389e0(entity, 1)`, resolves its pose, finds or builds a route, installs it, associates that target at +0x2fc, and writes step kind 0x13 to the final entry.

`EntityUpdateAndUnregisterAtTile @ 0x00440130` scans a global tracked-entity array, finds entries whose current cell equals the supplied cell, updates their +0x2fc target to the supplied object, calls `LinkedEntityFlagAndCommandUpdate`, and removes each matched entry from the array. `TerrainCellToWorldPosition @ 0x004401b0` operates on entities with bit 0x20000 and a linked entity at +0x36c. It may detach the pair when the linked entity has state bit 1, or on route/target conditions; the detach path clears both links, updates the linked entity's pose, clears state, requests activity 0, and updates scene state. Otherwise it may run a helper to update the current entity or move the linked kind-4 target's pose. The helper name does not establish a general cell-to-world conversion contract.

`LevelDebugCellRetargetDispatch @ 0x004403d0` is a kind-2 receiver path with gates on its current target, route, linked entity, and accumulated event bits. Depending on receiver/target flags it either resolves the target's pose and computes a cardinal direction or uses a candidate route search; it then installs a route to the target, stores the target at +0x2fc, and writes step kind 0xc plus direction/offset flags. The meaning of the event-mask gates is unresolved.

## Spatial callbacks and small adapters

`SpatialEntityCallbackQuery @ 0x004407b0` packages an entity's current pose and three arguments, then invokes `LAB_00440800` through the entity traversal helper. `SpatialEntityCallbackQueryAtOptionalCoordinate @ 0x004408c0` packages either a supplied coordinate or the entity's current pose with one argument, then dispatches `LAB_00440920`. The spatial filters and callback effects are in those external helpers, not visible in these wrappers.

`EntityActivityRequestSet @ 0x004409e0` writes the selected activity's global table value to +0x2e8, stores its second argument at +0x2d8, and clears +0x2d4. `AttachedEntityDirectionVectorUpdate @ 0x00440a10` walks attached records and, for those not marked with bit 0x2000, resolves a pose/origin based on the owning entity kind. It updates the attached entity's transform and computes a normalized 3D vector and length from its stored point to that origin. `ObjectListFieldAssignAll @ 0x00440bf0` only acts on kind 5: it obtains the object's list length and writes the supplied value into every entry.

## Unresolved questions and follow-up slices

- Name the request-queue masks, entity +0x340/+0x3d4 fields, and predicates used by damage and spawn processing.
- Trace `LAB_0043f580`, `LAB_00440800`, and `LAB_00440920` to establish the pool and spatial-query criteria.
- Determine the full lifecycle of the +0x3cc child relationship, including which update path invokes spawn processing and how the pair is released.
- Trace which conditions in `UiActionDispatch` choose operations 0x19/0x1a and clarify the relationship between those branches, the helper flags, linked-entity detach, and pose updates.
- Confirm which event bits and operation codes in `EntityLinkedActivityEventDispatch` have externally documented meanings.

**Evidence.** Canonical pseudocode in [LEGORR.full.c](../decompiles/full/LEGORR.full.c), procedures 0x0043eea0–0x00440bf0. The assigned addresses are listed in [function-areas.csv](../coverage/function-areas.csv) under `simulation-dispatch-damage`.
