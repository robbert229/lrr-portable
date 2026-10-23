# Finding 320 — CRT Descriptor and Extended-Float Helpers

Four runtime helpers now have operation-level names based on their exact slot and word behavior. The 96-bit names describe a three-word intermediate and do not imply a hardware floating-point format.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x499150 | CrtFileDescriptorCloseSlot | Validates a descriptor-table slot, clears a matching standard handle when applicable, and marks the OS handle invalid. |
| 0x49a050 | ExtendedFloat96BitShiftLeftOne | Shifts a three-word value left one bit with carry propagation. |
| 0x49a080 | ExtendedFloat96BitShiftRightOne | Shifts a three-word value right one bit with carry propagation. |
| 0x49a0b0 | CrtDecimalDigitSequenceToExtended96 | Accumulates a sequence of decimal characters into a three-word value and normalizes it. |

Bodies and callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). Descriptor-close and shift-operation evidence is caller-only in retained bundles; no direct routine record was found. The digit-sequence accumulator has direct decompile evidence in `legorr-crt-runtime-audit-2026-10-23.bundle`, record `ev_5f40ad265b4b0bd0ce1c6ddacf3541ad0db4de1ff86e04b0fe3a32fd68244431`; its caller is linked in `legorr-engine-helper-audit-text-io-expansion-9.bundle`, record `ev_1dba483dde5cf461dac162a37eda445c957718db9b47c5d66dca6686a7130985`. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
