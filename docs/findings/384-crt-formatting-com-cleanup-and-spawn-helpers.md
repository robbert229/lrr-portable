# Finding 384 — CRT Formatting, COM Cleanup, and Spawn Helpers

This pass names 15 routines from the high and mid ranges. Names follow visible operations and caller evidence; uncertain flags and object semantics remain generic.

| Address | Name | Observed behavior | Evidence and caller context |
| --- | --- | --- | --- |
| `0x0048e280` | `CrtRoundToIntegralWithMathErrorHandling` | Rounds a floating-point input to an integral value and routes failures through CRT math-error handling. | Procedure evidence `ev_e959cb3ca6814716839bb8ec7d3d52040fb96fb1a4f11f7482fda4eb8cecfc` in `legorr-engine-helpers-expansion-24.bundle`; rounds through `FUN_00493150`. |
| `0x0048e8e0` | `CrtRoundToIntegralWithMathErrorHandlingVariant` | Rounds a floating-point input through the same conversion helper with a distinct math-error operation code. | Procedure evidence `ev_81e53f10ab1b07600981f692ee5dda8a71d15e2284e8824fe7e2eece0dfdc564` in `legorr-engine-helpers-expansion-27.bundle`. |
| `0x004906f0` | `CrtScientificFloatFormatGuarded` | Sets a formatting guard and invokes the scientific-format helper. | Pseudocode only; operation follows the formatter call. |
| `0x00490720` | `CrtFixedDecimalFormatGuarded` | Sets a formatting guard and invokes the fixed-decimal formatter. | Pseudocode only. |
| `0x004908a0` | `CrtFileStreamClose` | Flushes a stream, releases its buffers, and closes its descriptor. | Caller `FUN_0048eae0`; behavior visible in pseudocode. |
| `0x00491d90` | `CrtNewHandlerInvokeForAllocationRetry` | Calls the registered allocation-failure handler with the requested size and returns its retry decision. | Pseudocode shows use by the heap allocator retry path. |
| `0x00494c30` | `CrtScanFormatCharNormalize` | Normalizes a character using locale classification for the scan-format parser. | Pseudocode and parser caller context. |
| `0x0047e460` | `ComObjectDestroyAndOptionallyFree` | Calls object cleanup, then frees the outer object when input bit 0 is set. | Callee `FUN_0047e480` switches to destructor vtable, releases two held interfaces, and clears both fields. |
| `0x00485210` | `FormattedTextAppendToRingBuffer` | Formats text, skips a requested number of bell-delimited fields, then appends the remainder to a circular buffer and updates wrap state. | Pseudocode only. |
| `0x0048fed0` | `CrtInputWithStreamBufferSetup` | Sets up a stream buffer, invokes the input parser, and performs cleanup. | Pseudocode only. |
| `0x0048b280` | `WindowModeFlagSet` | Stores the supplied value in a global used by window state handling. | Caller `FUN_004780b0` passes the value before window-state setup; exact flag meaning is uncertain. Evidence `ev_8688f97f74e41e6b24df6574e6d58ff1bbba7de05d148ffb35389f6e71190a8b` in `legorr-engine-helper-audit-text-io-expansion-9.bundle`. |
| `0x0047df50` | `PixelChannelMaskGet` | Derives a bit mask that the caller uses to mask and compare pixel bytes. | Caller behavior; source vtable field meaning remains unknown. |
| `0x0044ade0` | `EntityKindOneRandomSpawnTrigger` | Checks entity kind and flags, optionally selects a linked entity, invokes a scatter-spawn helper, then marks the entity. | Caller record `ev_5c9a218316de8e60a3f1ef52eb2c2b08ba2b4a619e6377fb5da7cf32f8dc0ce5`; helper context `ev_81e53f10ab1b07600981f692ee5dda8a71d15e2284e8824fe7e2eece0dfdc564`; finding 255. |
| `0x0044bfd0` | `ConfiguredMonsterSpawnBehaviorTick` | Calls a configured spawn helper, updates entity flags, and calls `0x00462880`. | Finding 257; helper evidence `ev_d4647dc0f66e7d02dd910bbd1bd88a44d04613beefcebe719c6320e071c4b9b2` in expansion-29. |
| `0x0045a240` | `ScrollPanelControlsInitialize` | Initializes scroll-panel components and sets the associated scroll flags. | Finding 262; caller context `ev_067463ec1683414c4610318da44c5a28d87abdd5ff2f8f0f5c6092f9293ead6d` in `legorr-ui-render-audit-2026-10-23.bundle`. |

Complete pseudocode is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c), with named excerpts in [engine-region.c](../decompiles/legacy/engine-region.c).
