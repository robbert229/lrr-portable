# Finding 355 — Bit, Pointer, Surface, and Window Wrappers

This pass names a bit counter, inverse bad-pointer checks, a surface-description wrapper, DirectDraw enumeration callback storage, a D3DRM rotation wrapper, and a window-rectangle helper.

| Address | Name | Observed behavior |
| --- | --- | --- |
| `0x0047d3f0` | `BitCount32` | Counts set bits by repeatedly clearing the lowest set bit. |
| `0x00499690` | `PointerRangeIsReadable` | Returns the inverse of IsBadReadPtr for the supplied pointer range. |
| `0x004996b0` | `PointerRangeIsWritable` | Returns the inverse of IsBadWritePtr for the supplied pointer range. |
| `0x004996d0` | `FunctionPointerIsExecutable` | Returns the inverse of IsBadCodePtr for the supplied code pointer. |
| `0x0047cdd0` | `SurfaceDescriptionGet` | Zero-initializes a descriptor, calls a renderer interface method to fill two outputs, and returns whether the call succeeded. |
| `0x0047c1c0` | `DirectDrawEnumerateAndStoreCallbackResult` | Stores callback input globally, invokes DirectDrawEnumerateA, copies the global result through the output slot, and returns 1. |
| `0x00479510` | `D3DRMVectorRotate` | Forwards the vector, quaternion, and output pointers to D3DRMVectorRotate. |
| `0x00478310` | `WindowRectAdjustForWindowedMode` | When the fullscreen bit is clear, calls AdjustWindowRect with the configured window style and FALSE for the menu flag. |

Caller records: `legorr-engine-helper-audit-text-io-expansion-9.bundle` (`ev_a66190a7acf1aaf1973f997f924032cc628dbccfb763eab60c5b59d483f2e6c8`, `ev_3f44d2535e1263ff50b6f12148e923868a50e3bad5e461c9293b407c05a935f6`, `ev_ff6f1f1c5e2014e3b3c5b3aeac54ec500d76ddcb6302ffa1b0f2b0c47aa3de1c`, `ev_1adbcc0746dd491e3abc48eff6ea6ce3853cfe8270a4100b73e508246a9ded0e`). Direct pseudocode records: `legorr-engine-helper-audit-text-io-expansion-9.bundle` (`ev_42e0175b96234a53249f89f142172779fe1e9f79f4bb8800def18faa1558dd40`, `ev_8632f0abdb686b0f49a92205b8c10864e560f1506e741040b92502f9dce52117`, `ev_543f2b4c68950ac3e8de2128302eb2d8df9bab052b4eba1aa147e20131b5dbd9`). The surface descriptor method name is retained at an operation level because its vtable slot meaning is not independently established.

The complete checked-in pseudocode is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c); named excerpts are in [engine-region.c](../decompiles/legacy/engine-region.c). Names preserve unresolved command codes, fields, flags, and interface semantics.
