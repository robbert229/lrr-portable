# Finding 386 — Spatial Queries, UI State, COM, and CRT Runtime

This pass names 106 functions across all remaining mid and high addresses. Names describe visible control flow and caller behavior; unclear flags, table semantics, and COM fields remain qualified or generic.

| Address | Name | Observed behavior | Evidence and caller context |
| --- | --- | --- | --- |
| `0x00435580` | `GlobalHandlePositionApplyIfPresent` | Applies a zero-position handle update when the handle global is set; returns success. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00435aa0` | `PanelSelectionHighlightToggle` | Toggles a panel item, plays sound on activation, and starts a short timer. | `LEGORR.full.c` pseudocode and caller references. |
| `0x004374d0` | `TaskCounterIncrementByBranch` | Increments one of two global task counters from a boolean. | `LEGORR.full.c` pseudocode and caller references. |
| `0x004374f0` | `EntityCallbackDispatchUnlessTypeFive` | Enumerates entities through a callback unless type is 5. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00438a60` | `SpatialObjectQueryCenterMask01` | Spatial callback query with criterion mask 0x1 and radius 10000. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00438ae0` | `SpatialObjectQueryCenterMask02` | Spatial callback query with criterion mask 0x2 and radius 10000. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00438b80` | `SpatialObjectQueryCenterMaskFromArgument` | Spatial callback query with caller-supplied criterion mask. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00438bf0` | `SpatialObjectQueryCenterMask80` | Spatial callback query with criterion mask 0x80. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00438cd0` | `SpatialObjectQueryCenterMask20` | Spatial callback query with criterion mask 0x20. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00438d30` | `SpatialObjectQueryCenterMask10` | Spatial callback query with criterion mask 0x10. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00438d90` | `SpatialObjectQueryCenterMask200` | Spatial callback query with criterion mask 0x200. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00438e50` | `SpatialObjectQueryCenterMask100` | Spatial callback query with criterion mask 0x100. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00439200` | `TaskKindFilteredSearch` | Rejects task kinds 1, 2, and 4, then runs a callback search. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00439240` | `TaskEntityFlagsAndCellTaskMatch` | Requires entity flag bit 8 and a matching task at the cell. | `LEGORR.full.c` pseudocode and caller references. |
| `0x0043b360` | `TaskCountAtGridCoordinateQuery` | Queries task count at a grid coordinate through a callback. | `LEGORR.full.c` pseudocode and caller references. |
| `0x0043b820` | `TypeTwoEntityEventDispatch` | Dispatches an event through the callback/state field for a type-2 entity or parent. | `LEGORR.full.c` pseudocode and caller references. |
| `0x0043b850` | `TypeTwoEntityPositionedCallbackDispatch` | Obtains position if needed and dispatches a type-2 callback. | `LEGORR.full.c` pseudocode and caller references. |
| `0x0043c490` | `TaskTypeAndConditionCheck` | Checks a task type and a second condition. | `LEGORR.full.c` pseudocode and caller references. |
| `0x004407b0` | `SpatialEntityCallbackQuery` | Packages entity position and parameters into a spatial callback query. | `LEGORR.full.c` pseudocode and caller references. |
| `0x004408c0` | `SpatialEntityCallbackQueryAtOptionalCoordinate` | Packages entity position or supplied coordinates into a spatial callback query. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00440c30` | `GridRouteSearchAdapter` | Forwards route-search arguments with two optional outputs null. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00442540` | `GridOperationArgumentAdapter` | Reorders three arguments into a grid-operation call. | `LEGORR.full.c` pseudocode and caller references. |
| `0x004456e0` | `EntityBehaviorCallbackTick` | Runs type-specific callbacks under flag/timer conditions. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00445d70` | `EntityFlagGatedCallbackDispatch` | Dispatches callbacks gated by entity flags 0x100 and 0x80. | `LEGORR.full.c` pseudocode and caller references. |
| `0x0044702d` | `RegisterRelativeStateUpdateFragment` | Fragmentary decompiler body performs register-relative state writes; higher-level role unknown. | `LEGORR.full.c` pseudocode and caller references. |
| `0x004474f0` | `LinkedEntityProximityResponse` | Clears a link, compares positions/radii, adjusts direction and activates an action when close. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00447900` | `TypeFourEntityCallbackDispatch` | Invokes callback only for entity type 4. | `LEGORR.full.c` pseudocode and caller references. |
| `0x0044adc0` | `ScatterSpawnGateSet` | Sets or clears global scatter-spawn bit 0x20. | `LEGORR.full.c` pseudocode and caller references. |
| `0x0044af90` | `EntityAttachedSpawnRecordProcess` | Uses an attached record at +0x348 to invoke a spawn helper. | `LEGORR.full.c` pseudocode and caller references. |
| `0x0044b4c0` | `TypeTwoEntityTargetResolveAndApply` | For type-2 entities, resolves and applies a target. | `LEGORR.full.c` pseudocode and caller references. |
| `0x0044bae0` | `SceneObjectResourceAndCallbackStateDispose` | Releases object resource at +0x98 and disposes callback state. | `LEGORR.full.c` pseudocode and caller references. |
| `0x0044c550` | `GlobalScatterSpawnEnableAndDrain` | Sets global bit 0x40, processes entities, and clears related globals. | `LEGORR.full.c` pseudocode and caller references. |
| `0x0044f490` | `GridCellOperationApplyAll` | Applies an operation to every cell in a width-by-height region. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00450560` | `RandomReachableGridPointSelect` | Samples map points until the grid query accepts one or retries expire. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00452580` | `ActiveContextRecordGet` | Returns active context and optional handle, or clears output when inactive. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00454880` | `GlobalCounter_004ff990Increment` | Increments global counter at 0x004ff990. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00454890` | `GlobalCounter_004ff990Reset` | Resets global counter at 0x004ff990. | `LEGORR.full.c` pseudocode and caller references. |
| `0x004567c0` | `GlobalValue_004ff9bcStoreAndReturn` | Stores and returns the value at global 0x004ff9bc. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00459f50` | `IndexedPanelResourcePositionApply` | Looks up and applies indexed panel resource at tile-derived position. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00459fd0` | `PanelResourcePositionApplyIfEnabled` | Applies configured panel resource at tile position when enabled. | `LEGORR.full.c` pseudocode and caller references. |
| `0x0045a3b0` | `PanelSlotCoordinateHitTest` | Checks panel table bounds and an adjusted coordinate. | `LEGORR.full.c` pseudocode and caller references. |
| `0x0045a630` | `UIRecordCallbackForward` | Forwards a dereferenced handle and two arguments to a UI record helper. | `LEGORR.full.c` pseudocode and caller references. |
| `0x0045aac0` | `PanelItemFlag10SetClear` | Sets or clears panel item flag 0x10. | `LEGORR.full.c` pseudocode and caller references. |
| `0x0045ab00` | `PanelStateFlagsToggle` | Toggles panel state flags used by item controls. | `LEGORR.full.c` pseudocode and caller references. |
| `0x0045abf0` | `PanelItemFlag8SetIfEnabled` | Sets panel item flag 0x8 when enabled. | `LEGORR.full.c` pseudocode and caller references. |
| `0x0045b610` | `GlobalUIFlags_00500860ClearBits_1_2` | Clears bits 1 and 2 in global UI flags at 0x00500860. | `LEGORR.full.c` pseudocode and caller references. |
| `0x0045bfb0` | `ThreeSurfaceResourcesLoadAndInitialize` | Loads three resource handles and initializes each surface. | `LEGORR.full.c` pseudocode and caller references. |
| `0x0045c340` | `PanelResourceLayoutStore` | Stores resource handles, dimensions, and offsets. | `LEGORR.full.c` pseudocode and caller references. |
| `0x0045c4b0` | `PanelResourcePositionUpdateIfEnabled` | Applies configured surface at panel position under a global enable flag. | `LEGORR.full.c` pseudocode and caller references. |
| `0x0045c520` | `PanelResourceHandleAndDimensionsStore` | Stores panel resource handle and dimensions. | `LEGORR.full.c` pseudocode and caller references. |
| `0x0045cf00` | `PriorityLabelLayoutApply` | Transfers saved coordinates and updates priority label position/visibility. | `LEGORR.full.c` pseudocode and caller references. |
| `0x0045d3c0` | `PriorityPanelRowMoveDispatch` | Moves or swaps a row according to mode bits. | `LEGORR.full.c` pseudocode and caller references. |
| `0x0045d8a0` | `ObjectField14Set` | Stores a value at object offset +0x14. | `LEGORR.full.c` pseudocode and caller references. |
| `0x0045db00` | `TexturedSceneObjectDrawAtPosition` | Sets render context, positions a scene object, derives size/color, then draws. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00460910` | `ResolvedEntityOperationDispatch` | Resolves an entity and delegates the operation. | `LEGORR.full.c` pseudocode and caller references. |
| `0x004628a0` | `GlobalFloatAccumulator_00555330Add` | Adds a float to global accumulator at 0x00555330. | `LEGORR.full.c` pseudocode and caller references. |
| `0x004628e0` | `CallbackRecordAllocateInitialize` | Allocates callback record and an eight-entry zeroed array. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00464c50` | `GlobalFlag_00502a2cBit2SetClear` | Sets or clears bit 2 in global 0x00502a2c. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00465380` | `GlobalTriplet_00552128_30Reset` | Clears three adjacent globals beginning at 0x00552128. | `LEGORR.full.c` pseudocode and caller references. |
| `0x0046a950` | `TextLayoutGeometryStoreAndApply` | Stores text/rectangle/scale fields and applies text layout. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00472670` | `DirectDrawSurfaceDescriptionGet` | Queries a DirectDraw surface description and returns selected fields. | `LEGORR.full.c` pseudocode and caller references. |
| `0x004756d0` | `VideoObjectAspectRatioGet` | Combines two interface values into an aspect ratio. | `LEGORR.full.c` pseudocode and caller references. |
| `0x004757f0` | `TypedChildResourceScalarGet` | Type-checks an object, resolves a nested child, and reads a scalar. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00477310` | `ComPropertyResultGet` | Calls a COM method, manages its returned interface, and invokes a callback. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00485a00` | `ObjectStatePairGet` | Returns fields at object offsets +0x820 and +0x824. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00487d50` | `ComInterfaceQueryAndContextDispatch` | Queries a COM interface and passes result/context to an operation helper. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00488bf0` | `CdAudioStopAndCloseIfActive` | Stops and closes cdaudio through MCI when initialized. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00489bc0` | `ComChildItemsVisit` | Recursively enumerates child COM interfaces and invokes a method. | `LEGORR.full.c` pseudocode and caller references. |
| `0x0048ebb0` | `CrtExceptionHandlerTransferAfterUnlink` | Unlinks FS exception registration and dispatches to a handler. | `LEGORR.full.c` pseudocode and caller references. |
| `0x0048ebf0` | `CrtExceptionHandlerTransferWithAtomicPrefix` | Performs lock/unlock-prefixed exception transfer and indirect dispatch. | `LEGORR.full.c` pseudocode and caller references. |
| `0x0048ec60` | `CrtExceptionDispatchForwarder` | Forwards arguments to the common CRT exception dispatcher. | `LEGORR.full.c` pseudocode and caller references. |
| `0x0048ed00` | `CrtExceptionFrameFilterInvoke` | Reconstructs dispatcher arguments from an exception registration frame. | `LEGORR.full.c` pseudocode and caller references. |
| `0x0048ed30` | `CrtExceptionFrameInstallAndDispatch` | Installs an FS exception frame, calls handler, and restores prior chain. | `LEGORR.full.c` pseudocode and caller references. |
| `0x0048ee00` | `CrtExceptionScopeFilterAndUnwind` | Filters exception flags, dispatches scope handler, or unwinds. | `LEGORR.full.c` pseudocode and caller references. |
| `0x0048efe6` | `CrtNlgFrameStateRecord` | Stores ECX, EAX, and EBP into NLG globals. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00490230` | `ProcessorFeatureFallbackFalse` | Returns zero as processor-feature fallback. | `LEGORR.full.c` pseudocode and caller references. |
| `0x004918a0` | `SmallHeapPageFreeSlotUpdate` | Updates freed-byte counts and recycles small-heap pages at thresholds. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00491cc0` | `SmallHeapBlockResizeInPlace` | Adjusts block size/free-list state when resize fits its page. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00491f80` | `CrtOneArgumentMathErrorHandlingStart` | Passes numeric argument through conversion/error handling and returns a double. | `LEGORR.full.c` pseudocode and caller references. |
| `0x0049200c` | `CrtFpuPrecisionModeCodeGet` | Tests FPU control and returns precision mode code 7 or 1. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00492068` | `DoubleExponentBitsGetOrSpecialValue` | Returns exponent bits or the full high word for an all-ones exponent. | `LEGORR.full.c` pseudocode and caller references. |
| `0x0049221e` | `CrtFloatingPointExceptionResultDispatch` | Normalizes floating exception result and dispatches conversion/error handling. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00492225` | `CrtFloatingPointExceptionResultDispatchVariant` | Classifies FPU status and dispatches conversion/error handling. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00492bc6` | `CrtDoubleRemainderReduction` | Reduces double remainder through exponent-aligned software path. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00493170` | `CrtMathExceptionHandleSingleResult` | Handles a math result under global math mode/error policy. | `LEGORR.full.c` pseudocode and caller references. |
| `0x004931c0` | `CrtMathExceptionHandleResultWithStatus` | Handles math result together with status under global math mode. | `LEGORR.full.c` pseudocode and caller references. |
| `0x004938f0` | `CrtMathExceptionDispatch` | Maps exception codes, clears FPU flags, calls app handler, maps fallback. | `LEGORR.full.c` pseudocode and caller references. |
| `0x004939a0` | `CrtMathErrorCodeSet` | Sets global math error code according to exception class. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00493c40` | `CrtNoOpStubVariant2` | Returns without visible work. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00495410` | `CrtSehExceptionDispatcher` | Searches exception handlers and dispatches matching handler. | `LEGORR.full.c` pseudocode and caller references. |
| `0x004954e0` | `CrtSehLocalUnwindToScope` | Unwinds local state to requested scope. | `LEGORR.full.c` pseudocode and caller references. |
| `0x004955c0` | `CrtSehUnwindAndInvokeHandler` | Unwinds to scope and invokes its exception handler. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00495650` | `CrtSehHandlerCallWithFrame` | Calls exception handler using active registration frame. | `LEGORR.full.c` pseudocode and caller references. |
| `0x0049573a` | `CrtSehAbnormalTerminationCleanup` | Runs cleanup associated with abnormal SEH termination. | `LEGORR.full.c` pseudocode and caller references. |
| `0x004959e0` | `CrtSehDestructorInvoke` | Invokes destructor through active SEH cleanup path. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00495ae0` | `CrtTerminateHandlerInvokeAndAbort` | Invokes registered terminate handler and aborts. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00495b70` | `CrtUnexpectedHandlerInvokeAndTerminate` | Invokes registered unexpected handler and follows terminate path. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00496895` | `CrtLocalUnwindFromFrame` | Calls __local_unwind2 with function/scope indexes from frame. | `LEGORR.full.c` pseudocode and caller references. |
| `0x004968b0` | `CrtRuntimeErrorMessageEnterExit` | Outputs message markers 0xfc/0xff around a callback. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00497770` | `CrtFpuControlWordUpdate` | Translates/merges current x87 control word and writes it. | `LEGORR.full.c` pseudocode and caller references. |
| `0x004977b0` | `CrtFpuControlWordUpdateMask` | Clears mask 0x80000 before control-word update. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00497d60` | `CrtExtendedFloatConvertFormatA` | Converts 80-bit input through format table at 0x004b28a8. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00497d80` | `CrtExtendedFloatConvertFormatB` | Converts 80-bit input through format table at 0x004b28c0. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00497da0` | `CrtDoubleToExtendedFloatConvertA` | Builds 80-bit temporary from double and delegates to format A. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00497de0` | `CrtDoubleToExtendedFloatConvertB` | Builds 80-bit temporary from double and delegates to format B. | `LEGORR.full.c` pseudocode and caller references. |
| `0x00498ce0` | `CrtScanfNumericConversionAssign` | Selects numeric conversion, builds internal number, updates FPU/errno. | `LEGORR.full.c` pseudocode and caller references. |

Supporting REA records for the audited regions include `legorr-engine-helper-audit-terrain-expansion-9.bundle`, `legorr-engine-helpers-expansion-17.bundle`, `legorr-ui-render-audit-2026-10-23.bundle`, `legorr-ddscene.bundle`, `legorr-engine-helper-audit-high.bundle`, `legorr-engine-helper-audit-text-io-expansion-9.bundle`, `legorr-engine-helpers-expansion-18.bundle`, `legorr-crt-third-pass-2026-10-23.bundle`, and `legorr-crt-runtime-audit`.

Complete pseudocode is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c), with named excerpts in [engine-region.c](../decompiles/engine-region.c).
