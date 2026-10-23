# Finding 382 — CRT, Extended Precision, and Entity Routing

This pass names 24 routines. Names follow the visible operations and caller evidence; unknown event and field semantics remain unspecified.

| Address | Name | Observed behavior | Evidence and caller context |
| --- | --- | --- | --- |
| `0x0047a1b0` | `FormattedTextDraw` | Calls text layout and drawing helpers that process inline address tokens, tabs, and line breaks. | Caller `FUN_0045a460`; operation follows the formatted-text callee behavior in checked-in pseudocode. |
| `0x0048d940` | `CrtMathDispatchTableInitialize` | Initializes six floating-point math dispatch pointers. | Called by `__fpmath`; procedure evidence `ev_8688f97f74e41e6b24df6574e6d58ff1bbba7de05d148ffb35389f6e71190a8b` in `legorr-engine-helper-audit-text-io-expansion-9.bundle`. |
| `0x0048e090` | `CrtAsin` | Computes arcsine using x87 input and handles special values. | Caller context `ev_767971267f63cf3ca68b647c6a0ed30ad4322b9e17c66984b32a5b5fe596fb6d` in `legorr-ui-render-third-pass-2026-10-23.bundle`. |
| `0x0048e480` | `CrtFloatingPointRuntimeInitialize` | Sets up floating-point math dispatch and initializes two callback ranges. | Pseudocode only. |
| `0x0048e4b0` | `CrtExit` | Calls the CRT exit path with the supplied status and zeroed flags. | Direct wrapper body calls `FUN_0048e4f0`. |
| `0x0048ec00` | `SehFrameUnwind` | Calls `RtlUnwind` and restores exception-frame state. | Pseudocode only. |
| `0x0048eca0` | `CallSettingFrame12` | Establishes an exception frame, calls `__CallSettingFrame_12`, and restores the prior frame. | Pseudocode only. |
| `0x0048ee90` | `SehScopeTableLookup` | Searches scope records for the entry covering an instruction address. | Pseudocode only. |
| `0x0048fa00` | `CurrentDirectoryGet` | Retrieves the current directory into a supplied or allocated buffer. | Caller context `ev_5a362a827bafd4531c670c1f0d5856c094e1a289f6aa0710b48f14884c5f707b` in `legorr-registry-accessor.bundle`. |
| `0x0048fd20` | `CrtFlushStreamBuffer` | Writes buffered output and resets stream state. | Caller context `ev_1829104c31f50f01d7d14a17bfdfd934e343609c360d5c08ad33a5505152d7af` in `legorr-save-write-results-and-count-reset.bundle`. |
| `0x00493ec0` | `BoundedStringLength` | Returns the string length up to the supplied limit, or the limit when no terminator is found. | Pseudocode only. |
| `0x00493ae0` | `DoubleFrexpDecompose` | Normalizes a double and returns its exponent through an output parameter. | Pseudocode only. |
| `0x00493be0` | `FpuStatusWordGetVariantA` | Returns the x87 status word. | Pseudocode only; identical operation to the neighboring variant. |
| `0x00493bf0` | `FpuStatusWordGetVariantB` | Returns the x87 status word. | Pseudocode only; identical operation to the neighboring variant. |
| `0x00497aa0` | `UInt96Zero` | Clears a three-word unsigned value. | Pseudocode and callers in extended-precision conversion routines. |
| `0x00497ab0` | `UInt96IsZero` | Tests whether a three-word unsigned value is zero. | Pseudocode and callers in extended-precision conversion routines. |
| `0x00497a80` | `UInt96Copy` | Copies a three-word unsigned value. | Pseudocode and callers in extended-precision conversion routines. |
| `0x00497ad0` | `UInt96ShiftRight` | Shifts a three-word unsigned value right by a bit count. | Pseudocode and callers in extended-precision conversion routines. |
| `0x00497900` | `UInt96HasNoBitsAtOrAbove` | Tests whether a three-word value has any set bits at or above a specified position. | Pseudocode and callers in extended-precision conversion routines. |
| `0x00497970` | `UInt96IncrementAtBit` | Adds a bit at the specified position with carry propagation. | Pseudocode and callers in extended-precision conversion routines. |
| `0x004979e0` | `UInt96ClearFromBit` | Clears bits at and above a position, with an adjacent-value adjustment used for rounding. | Pseudocode and callers in extended-precision conversion routines. |
| `0x00438df0` | `EntityLookupAtWorldPosition` | Looks up the entity at a world position derived from another entity’s transform. | Caller context `ev_7005976a7376513f56f99ffc8d6918b953623c6f6cc5e99ed535affa74cdb3ac` and caller record `ev_e58780a7707f42b50eb899f56f6a99d705d24a6209479c96c9a806967be6126e` in `legorr-engine-helpers-expansion-28.bundle`; target name is based on caller use. |
| `0x00442210` | `EntityActionRouteStateInitialize` | Resets action state, issues event `0x4d`, conditionally sets an entity flag, and resets a float field to `-1.0`. | Pseudocode plus caller context `ev_e959cb3ca6814716839bb8ec7d3d52040fb96fb1a4f11f7482fda4eb8cecfc` in `legorr-engine-helpers-expansion-24.bundle`; exact event meaning is unknown. |
| `0x0044c970` | `LightEffectsBlinkParametersApply` | Applies parameters passed from configuration for color maximum, change scale, and a two-value blink interval. | Caller helper context `ev_81e53f10ab1b07600981f692ee5dda8a71d15e2284e8824fe7e2eece0dfdc564` in `legorr-engine-helpers-expansion-27.bundle`; target body was not in that bundle. |

Complete pseudocode is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c), with named excerpts in [engine-region.c](../decompiles/engine-region.c).
