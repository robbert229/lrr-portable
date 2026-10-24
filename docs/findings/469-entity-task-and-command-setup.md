# Finding 469 — Entity Task and Command Setup

This finding maps the 21 procedures assigned to entity-task-command-setup. They form four connected groups: a task-search predicate, build-placement command constructors, entity and item name lookups, and task-target eligibility/command helpers. The evidence is the canonical pseudocode in [LEGORR.full.c](../decompiles/full/LEGORR.full.c); cited addresses identify procedures. Function names are descriptive labels, and state meanings are limited to observed accesses and callers.

## Task matching and build-placement commands

TaskEntityFlagsAndCellTaskMatch @ 0x439240 is the callback used by TaskKindFilteredSearch @ 0x439200 (Finding 449). It first requires EntityStateFlagsGet(entity) & 8, then calls EntityActionKindSupported(entity, taskKind, 0, 0) and accepts only a nonzero result. The caller uses it while searching candidates; the callback itself does not mutate the entity.

EntityBuildPlacementCommandSetup @ 0x439370 and EntityBuildPlacementAlternateCommandSetup @ 0x4394b0 share the same coordinate handling but enqueue different action opcodes. With no explicit coordinate pointer they call FUN_00431570 with the cell coordinates, pass its result to FUN_004500d0, calculate an offset using two values returned by FUN_00450890, and convert the resulting point; otherwise they use the supplied pair. The first creates a kind-6 command through EntityCreateWithPosition and enqueues opcode 0x16; the alternate creates kind 7 and enqueues 0x1c, then dispatches an additional 0x13 operation. When the final argument is nonzero, the first calls FUN_00419b80 and dispatches operation 0x11 before incrementing the cell record at +0x2c; the alternate dispatches operation 0x13 and increments its +0x30 field.

EntityBuildPlacementCommandPairCreate @ 0x439340 invokes both constructors for a coordinate. EntityBuildPlacementCommandBatchExpand @ 0x439280 asks FUN_0042c5b0 for four counts, then emits that many of each constructor across its two variants. The command dispatcher SelectionRoot @ 0x4520d0 later handles opcodes 0x15, 0x1b, 0x34, and 0x36 through these same setup routines (0x39370, 0x394b0, 0x39340, and 0x39280). Direct evidence is that both constructors call FUN_00452060 with their opcode and created command, and SelectionRoot later drains records and dispatches these opcodes. It is an inference that these calls form the placement-command enqueue path. The decompile establishes the opcode/count flow; the exact gameplay meaning of the four FUN_0042c5b0 outputs remains unclear.

## Names and resource lookups

EntityDisplayNameLookup @ 0x4395e0 selects display-name entries by entity kind. Kinds 1–4 and 10 index kind-specific tables using fields from the entity record; kinds 6–9, 11–13, and 15 return fixed entries or a small indexed set; unsupported kinds return null.

ItemNameByKindLookup @ 0x4396c0 returns a hard-coded item string for several kinds (including PowerCrystal, ProcessedOre, Dynamite, Barrier, ElectricFence, SpiderWeb, and OohScary), indexes the corresponding tables for kinds 1–4, and returns null for unhandled kinds. Its call sites include SceneActivityUpdateDispatch and EntityType4IndexListAppend (Finding 251).

LocalizedObjectTheNameGet @ 0x439790 and LocalizedObjectNameGet @ 0x439860 select localized ObjectTheNames and ObjectNames entries, respectively, by kind and index, returning null for unsupported kinds. Finding 419 connects the former table to its configuration loader; Finding 381 records the paired lookup tables. ToolNameStringResourcesLoad @ 0x439950 loads the ToolNames resource entries into the global table beginning at DAT_00556228. CopyTwoDwordPairToOutputs @ 0x439930 is a two-word copy helper; its callers include item/resource selection paths (Finding 454).

## Task target checks and command issue

TaskTargetCellEligibilityCheck @ 0x439b80 returns true for non-kind-1 entities. For kind 1, it rejects a target record with a nonzero second word; when FUN_00469ea0(entity) lacks bit 0x200, it asks FUN_00438ae0 whether the entity can use the target's kind and indexed entry. If that flag is set, the function falls through to true. This helper's local behavior is direct evidence; the semantic meaning of the target record's second word is unresolved.

EntityAcceptsTargetForTaskCheck @ 0x439bd0 applies broader gates: kind 3 requires bit 0x08000000 in the entity field at +0x3e8; other kinds require byte bit 0x20 there. It rejects entity flag bit 0 at +0x3e0, optionally rejects entities for which FUN_0043baf0 is nonzero, and enforces two target-kind restrictions: flag mask 0x4 rejects targets whose kind is not 6, and flag mask 0x2 rejects targets whose kind is not 7. Finally, when the entity's +0x324 value is nonzero, the referenced value at +0x308 must match the target kind. The helper does not mutate either record.

EntityFlag400AndHealthThresholdCheck @ 0x4399c0 returns false unless bit 0x400 is set in the entity field at +0x3e0; for kind 1 it also applies a comparison involving the entity's +0x328, +0x324, a value returned by FUN_00469bb0, and the float at +0x3d4 against 75.0. GatedStepCommandIssueWithTargetRecord @ 0x439a20 uses this as a rejection gate, then requires EntityAcceptsTargetForTaskCheck(entity, target, 1). It converts both positions to cells; if they coincide it creates a route directly, otherwise it asks FUN_004410f0 for a route and fails if that helper returns zero. On success it writes step opcode 3 into the entity's current step record and stores the target pointer at +0x2fc. The callers documented in Finding 256 are the command sweep and CoordinateListCallbackVisit.

## State and flag helpers

EntityActivitySceneStateUpdate @ 0x439990 sets state bits in _DAT_004dee14: if bit 0x10 is already set it raises bit 8 and returns; otherwise it raises bit 4 and calls FUN_0041f8c0. The name suggests a scene/activity update, but the decompile does not identify the state bits' consumer beyond that call.

EntityIndexedFlagSetAndRefresh @ 0x439c80 acts only on kind 1. It reads a flag word through helpers using the entity's +0x3fc component, sets the bit selected by the low five bits of the byte at +0x404, writes the word back, refreshes the entity through FUN_00469c10, and dispatches operation 0x19. EntityAnimationStateSet @ 0x439cd0 is a thin wrapper around FUN_00469c10.

EntityStatsWordFlag10Test @ 0x439cf0, EntityStatsWordFlag20Test @ 0x439d10, and EntityStatsWordFlag40Test @ 0x439d30 return only the corresponding 0x10, 0x20, or 0x40 mask from EntityStateFlagsGet. Their callers use them together as predicates when deciding whether an entity meets a flag combination; these helpers do not establish higher-level meanings for those bits.

## Unresolved questions and follow-up slices

- Identify the four FUN_0042c5b0 counts and the source records they expand into build-placement commands.
- Trace the consumers of _DAT_004dee14 bits 2/3/4 and establish what scene/activity transition they represent.
- Resolve the target-record layouts used by TaskTargetCellEligibilityCheck and EntityAcceptsTargetForTaskCheck, including the role of +0x324/+0x308.
- Trace the placement counters at cell-record +0x2c/+0x30 to their consumers and determine whether they are counts, pending-work markers, or both.

**Evidence.** docs/decompiles/full/LEGORR.full.c, procedures 0x439240–0x439d30 and SelectionRoot @ 0x4520d0; related caller and table context in Findings 449, 251, 256, 381, 419, 454, and 6. The function-area catalog assigns all 21 procedures to entity-task-command-setup.
