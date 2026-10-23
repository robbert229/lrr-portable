# Finding 370 — Screen Effects, Surface, and UI Helpers

This pass names a screen-effect record setter, UI list/bounds operations, indexed-surface lifecycle, info-panel string initialization, visibility forwarding, and deferred-sound timer helpers.

| Address | Name | Observed behavior |
| --- | --- | --- |
| `0x00435ebb` | `ScreenEffectRecordSet` | Writes four fields at offsets +4 through +0x10, using a helper for one field; field meanings are unresolved. |
| `0x0045bf70` | `UIEntryBoundsGet` | Copies two stored coordinates and computes width and height by subtracting the origin from the far edges. |
| `0x0045d470` | `UIListEntriesSwap` | Swaps two indexed entries, updates their positions, refreshes a related row, and triggers a redraw. |
| `0x0045e400` | `EntityTypeAndStateFlagPredicate` | Rejects a fixed set of entity types, then returns one bit derived from a helper result; bit meaning is unresolved. |
| `0x00462da0` | `IndexedSurfaceResourceAllocate` | Creates a surface, allocates and initializes a width-by-height array of 8-byte entries, and records dimensions/state. |
| `0x004634f0` | `IndexedSurfaceEntryHideAndRecord` | Computes an indexed entry, hides its resource, and records its address for later restoration. |
| `0x00463540` | `IndexedSurfaceEntriesRestore` | Iterates the recorded entries, makes each visible, and clears the recorded count. |
| `0x004636b0` | `InfoPanelStringEntriesInitialize` | Reads a packed argument sequence and initializes entries at stride 0x20, resolving each supplied string. |
| `0x00464720` | `SurfaceVisibilitySet` | Forwards a visibility value and surface handle to the visibility helper. |
| `0x00464e80` | `DeferredSoundPlayExpire` | When a pending sound index and positive timer exist, submits the sound and clears both fields. |
| `0x00464ec0` | `DeferredSoundTimerSetIfIdle` | When no positive timer is active, clears the pending index, stores the supplied timer, and reports success; otherwise reports failure. |
| `0x0046d1c0` | `AnimationDurationForLoopCount` | Reads duration from one or two resource handles and adjusts the selected duration by repeated-loop duration. |
| `0x0046f1d0` | `EntityIndexedFloatLookupWithMinusOneFallback` | Looks up a float through entity-derived table indices and uses a default table value when the selected entry is -1.0. |

Direct pseudocode evidence includes `ev_6f7b344282de8408eabbc0b3e13e4475ca816a3dcaa7793b4c85873e5b88be28` (`0x00435ebb`), `ev_794fbcb1dbdd2788e987eabba684d25ba4235923143a9fb241608a114fce10af` (`0x0045bf70`), `ev_18225d870e994d453ff2a0d43101c03bb6a6b50c733b51f7dde5b6869d52c8c8` (`0x0045d470`), and `ev_009fe106ab7b51307c5cba1b83c2b3b8dbfd0122ff2f20825605027b3f31b933` / `ev_66cafe7015e88dea690c66f602db90b62c4b49d5d3cc2496e9380bee0f0cb22d2` (`0x0045e400`). Indexed-surface records include `ev_359478663d42b08f3bad5fa97ef9eae3b53aa9e90d0b54bd438070416acbc4ca`, `ev_caf573b52b490d5f0ff1eddf9065fedf53dc36b44449ba2cb638e267b79ff875`, and caller record `ev_c1e2e51acfbd310e37f69465934027fb445b987f8afadfb5e57946b5095c3263`. Info-panel and surface visibility records are `ev_1ae6aef1bbd8dd46685345ad5bdf7cdb985f6992e0780951dc34395048317aa8` and `ev_b63960cc8124693a94f31994e8c9878e28f8b7b1b23742816bc49154057551d5`. Remaining entries are supported by full pseudocode and callers.

Complete pseudocode is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c); named excerpts are in [engine-region.c](../decompiles/legacy/engine-region.c). Numeric record kinds remain literal where semantics are unresolved.
