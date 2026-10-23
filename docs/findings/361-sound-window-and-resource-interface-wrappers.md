# Finding 361 — Sound, Window, and Resource Interface Wrappers

This pass names a sound-volume forwarding helper, audio shutdown, an object field accessor, a render transform wrapper, a callback record update, and resource-interface slot wrappers. The virtual-slot operations are named by slot because their individual semantics are unresolved.

| Address | Name | Observed behavior |
| --- | --- | --- |
| `0x0047b550` | `SoundBufferVolumeSetMinus300` | When the sound system is initialized, stores 0xfffffed4 and passes it through interface slot +0x3c; the value’s units and meaning are unresolved. |
| `0x0047a800` | `DirectSoundObjectsShutdown` | When the audio initialization flag is set, cleans up one object, releases two interfaces, clears a global pointer, and resets the initialization flag. |
| `0x0047a540` | `ObjectFieldBE4DwordGet` | Returns the dword at object offset +0xbe4. |
| `0x004772b0` | `RenderInterfaceTransformPoint` | Forwards two pointers through interface slot +0x50; callers consume the outputs as homogeneous coordinates. |
| `0x00475e10` | `CallbackRecordSetFlag10AndRefresh` | Gets a callback record, sets bit 0x10 at record offset +0x14, then calls the refresh helper; flag meaning is unresolved. |
| `0x00476f50` | `ResourceInterfaceCallSlot48` | Forwards the dword at the supplied pointer through the wrapped interface slot +0x48. |
| `0x00476fd0` | `ResourceInterfaceCallSlot8C` | Calls wrapped interface slot +0x8c. |
| `0x00476ff0` | `ResourceInterfaceCallSlot90` | Calls wrapped interface slot +0x90. |
| `0x00477250` | `ResourceInterfaceCallSlot40AndClearField14` | Calls wrapped interface slot +0x40 with the supplied value, then clears the wrapper field at +0x14. |
| `0x00477270` | `ResourceInterfaceCallSlot88` | Calls wrapped interface slot +0x88. |

Direct evidence: `legorr-sound-duplicate-selection.bundle` (`ev_1d6630a8e7c03dbe0d54889e6c448eecfc1bf8a7e74a22a9406839be85ab9d02`) for `0x0047b550`; `legorr-engine-helper-audit-high-expansion-6.bundle` (`ev_7a32ee86033f5cb891e5c82fc7d437226177208c1309bc81cc5b2869e35687ec`) for `0x0047a800`; `legorr-engine-helpers-expansion-9.bundle` (`ev_b658def7afa4372c8eccae16aee5cdf6bbc57c4135d9d5adeb95d5c85c57156c`) for `0x0047a540`; `legorr-engine-helper-audit-render-expansion-8.bundle` (`ev_e332e0179ccf97f360bf1a9bc3fe661a3eac96042d4a0132c362eb9fbdee3551`) for `0x00475e10`; and `legorr-sound-instance-spatial-expansion.bundle` (`ev_e56a96b67810198e09b52235915ee16b7e89d1712bd8ae7c2a7432c9421593cf`) for `0x00476f50` and `0x00477250`. Other entries are supported by checked-in pseudocode and caller patterns.

Complete pseudocode is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c); named excerpts are in [engine-region.c](../decompiles/engine-region.c). Field and interface meanings remain unspecified where callers do not establish them.
