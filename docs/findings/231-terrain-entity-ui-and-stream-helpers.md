# Finding 231 — Terrain, Entity, UI, and Stream Helpers

Caller-backed audits of the remaining low, middle, and high address ranges
identified thirteen more useful procedures. Their names and pseudocode are
now included in the Ghidra target list and generated corpus.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x402510 | TerrainCellCommandCreate | Validates a terrain cell and command eligibility, allocates command kind 8 with the cell coordinates, sets flags, and enqueues it under queue kind 3. The mapping from command kind 8 to a player action remains unknown. |
| 0x402ae0 | TerrainCellRefreshDispatch | Queues a callback with cell coordinates and a mode, then resets the cell's surface type. The callback's exact effect remains unresolved. |
| 0x403c00 | EntityActivityRecordRemoveByType | Finds and unlinks an entity activity record by type, performs type-specific cleanup for selected records, notifies the shared activity state, then releases the record. |
| 0x41a7d0 | InputKeyGroupRegister | Copies a count of variadic key/action codes into a per-group allocated table. The input initializer registers nineteen groups, which the input scan later reads. |
| 0x439e70 | EntityAttachedObjectListDetach | Walks an entity's attached-object list, clears relationship links, and removes or resets associated activity records according to flags and mode. |
| 0x441b30 | EntityStateReset | Resets action-sequence state, attachment relationships, activity flags, and related callbacks. Its second parameter controls whether final cleanup callbacks run. |
| 0x46b520 | UiTextEntryLoad | Loads a configured UI text value, converts underscores and escaped newlines, handles @-prefixed resource values, and initializes per-entry state. Callers load tooltip and interface-button text. |
| 0x46f0d0 | ObjectTypeNameIndexFind | Case-insensitively searches the global object/resource name table and returns its index, or a count-based sentinel when absent. |
| 0x4859b0 | TextBufferReset | Clears a text buffer, its 256 state slots, cursor values, and pending flag; callers use it in text, UI, and scene paths. |
| 0x491430 | FormatArgumentReadDword | Reads a 32-bit value from the formatter's argument cursor and advances it by four bytes. |
| 0x491450 | FormatArgumentReadInt64 | Reads a raw 64-bit variadic argument into EDX:EAX and advances the formatter cursor by eight bytes. |
| 0x491470 | FormatArgumentReadWord | Reads the low 16-bit value and advances the formatter cursor by one four-byte variadic slot. |
| 0x496c00 | Win32FileDescriptorSeek | Applies a pending seek through SetFilePointer, validates descriptor state, reports Win32 errors, and clears the deferred-seek flag. |

**Evidence.** Direct pseudocode for all thirteen procedures is in
ev_b45088856e52b9f85258f86a6c6da84d719f3af168d8ff877959706d4955701f,
preserved in
[legorr-engine-helpers-expansion-6.bundle](../evidence/legorr-engine-helpers-expansion-6.bundle).
Caller and address-range evidence is preserved in the [low](../evidence/legorr-engine-helper-audit-low-expansion-6.bundle),
[simulation](../evidence/legorr-engine-helper-audit-mid-expansion-6.bundle), and
[resource](../evidence/legorr-engine-helper-audit-high-expansion-6.bundle) audit
bundles. The analyses used shipped reference/LegoRR.exe, SHA-256
c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f.
