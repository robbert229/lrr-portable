# Finding 383 — CRT, Global Accessors, and Light Effects

This pass names 38 routines across the remaining high and mid ranges, plus small state helpers. Names follow visible operations and caller context; unknown global meanings remain address-qualified.

| Address | Name | Observed behavior | Evidence and caller context |
| --- | --- | --- | --- |
| `0x00490280` | `CrtFloatingPointFeatureProbe` | Queries `IsProcessorFeaturePresent(0)` and selects a fallback when unavailable. | Pseudocode only. |
| `0x004902b0` | `CrtNormalizeDecimalPoint` | Rewrites a numeric text buffer using the locale decimal separator. | Pseudocode only. |
| `0x004907c0` | `StringPrefixReserve` | Moves a NUL-terminated string right by the requested byte count to reserve a prefix. | Pseudocode only. |
| `0x004913b0` | `CrtWriteRepeatedChar` | Emits one character repeatedly through an output helper until failure. | Pseudocode only. |
| `0x004913f0` | `CrtWriteStringBounded` | Emits at most the requested number of string bytes through an output helper. | Pseudocode only. |
| `0x004914a0` | `StrChr` | Returns a pointer to the first matching character, or null. | Pseudocode only. |
| `0x00491560` | `CrtHeapInitialize` | Creates the private CRT heap and initializes its allocator state. | Pseudocode only. |
| `0x00491710` | `CrtHeapBlockFree` | Releases a heap block with `VirtualFree` and unlinks its allocator node. | Pseudocode only. |
| `0x00492dcc` | `CrtFmodExtendedPrecision` | Computes a floating-point remainder and routes exceptional cases through a helper. | Pseudocode only. |
| `0x004939d0` | `CrtMathExceptionHandlerGet` | Returns the registered math-exception handler. | Pseudocode only. |
| `0x00493a00` | `CrtMathExceptionCodeFromStatus` | Maps floating-point status to a CRT math exception code. | Pseudocode only. |
| `0x00493a40` | `DoubleBuildWithExponent` | Builds a double from a significand and exponent representation. | Pseudocode only. |
| `0x00493a80` | `DoubleClassify` | Classifies double special values including infinity and NaN. | Pseudocode only. |
| `0x00495d90` | `IsDbcsLeadByte` | Checks whether a byte is a lead byte in the active code page. | Delegates to a code-page helper; caller context is visible in locale parsing. |
| `0x004964e0` | `CrtCodePageGet` | Returns the requested OEM or ANSI code page, including special selector handling. | Pseudocode only. |
| `0x004974d0` | `CrtStandardStreamBufferInstall` | Installs a 4 KB buffer for one of two fixed standard-stream records. | Pseudocode only. |
| `0x00497ec0` | `CrtLocaleDataGetOrInitialize` | Returns locale data, initializing it when the global cache is empty. | Pseudocode only. |
| `0x00498340` | `CrtFatalMessageExit` | Calls the CRT fatal-message exit path with code 2. | Direct call to `__amsg_exit(2)`. |
| `0x00499860` | `CrtTimeZoneInitializeOnce` | Initializes time-zone state behind a one-time guard. | Pseudocode only. |
| `0x0048e890` | `MarkupIntegerTokenParse` | Parses an integer token using a scanf-like conversion helper. | Caller evidence `ev_b85e67b084f3e4f58104808358264bbf35d8a22b04e4917f254dcf99157f38da` in `legorr-objective-advisor-inline-markup.bundle`. |
| `0x0043f560` | `EntityPoolFindWithLocalPredicate` | Calls the entity-pool callback search with a local predicate. | Callers `FUN_00437030` and `FUN_00437768`; predicate body is local to this wrapper. |
| `0x00456640` | `GlobalFlagGet_004ff978` | Returns the global value stored at `0x004ff978`. | Caller `FUN_0046ad00` uses it as a gate; exact meaning is unresolved. |
| `0x00458650` | `GlobalFlagGet_004ffe30` | Returns the global value stored at `0x004ffe30`. | Caller `FUN_0045c4b0` checks it as a gate; exact meaning is unresolved. |
| `0x00458660` | `GlobalValueSet_004ffe48` | Stores the supplied value at `0x004ffe48`. | Direct pseudocode; the global’s meaning is unresolved. |
| `0x0045c4a0` | `GlobalValueSet_0050080c` | Stores the supplied value at `0x0050080c`. | Direct pseudocode; the global’s meaning is unresolved. |
| `0x0045cc10` | `IndexedGlobalValueGet_00500a9c` | Returns an indexed value from the table beginning at `0x00500a9c`. | Callers use the value in UI state setup; exact field semantics are unknown. |
| `0x0045cc60` | `GlobalValueGet_00500a98` | Returns the global value stored at `0x00500a98`. | Caller `FUN_0045c4b0`; field semantics remain unknown. |
| `0x004628c0` | `GlobalFloatAccumulate_00555120` | Adds the supplied float to the global accumulator at `0x00555120`. | Caller `FUN_004628a0`; unit and purpose are unresolved. |
| `0x0047c160` | `ConstantNegative10000Get` | Returns the constant `-10000`. | Direct pseudocode. |
| `0x0048c030` | `RegistryEntryAccessByIndex` | Calls the registry-entry accessor for the supplied index and discards its result. | Direct pseudocode; the accessor returns a registry slot value. |
| `0x0048e1aa` | `CrtMathIntrinsicDispatch` | Calls the CRT x87 intrinsic dispatcher. | Direct pseudocode; caller uses it in math paths. |
| `0x00491ff5` | `CrtNoOpStub` | Returns without performing visible work. | Empty pseudocode body. |
| `0x00495bde` | `CrtAbortAfterCallback` | Runs the CRT abort callback path and then aborts. | Wrapper calls `FUN_00495ae0`, which invokes a registered callback then `_abort`. |
| `0x00497870` | `CrtNoOpStubVariant` | Returns without performing visible work. | Empty pseudocode body. |
| `0x0044c7c0` | `LightEffectsConfigurationLoad` | Loads configured blink, fade, and move light-effect settings. | Called by `GameConfigInit`; blink evidence `ev_81e53f10ab1b07600981f692ee5dda8a71d15e2284e8824fe7e2eece0dfdc564` in expansion-27; fade/move context in expansion-17. |
| `0x0044cef0` | `LightEffectsMoveParametersStore` | Stores parsed movement-effect parameters and sets the associated state bit. | Caller `FUN_0044cca0`; context `ev_50ac433a55f0fe9d174a1fccee1298b2eddb40d022f7beef590478017d4dd352` in `legorr-engine-helpers-expansion-17.bundle`. |
| `0x0044d710` | `WaypointVectorRandomScaleApply` | Scales target-vector components by a random factor. | Called by timer-driven `FUN_0044d280`; context `ev_34b2943ae4eec65ce70d2ab6788f10b428d3020b5a9215af77e94d265fafa8dd` in expansion-30. |
| `0x0046abb0` | `TextSlotMessageWithImageAssign` | Assigns formatted text and creates/stores a surface for the text slot. | Called by `TextMessagesConfigLoad` for `TextMessagesWithImages`; context `ev_fb654e9ce69c90085cda97e49f50a18cc194e4faf49dda19c52176f3299c7377` in expansion-20. |

Complete pseudocode is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c), with named excerpts in [engine-region.c](../decompiles/legacy/engine-region.c).
