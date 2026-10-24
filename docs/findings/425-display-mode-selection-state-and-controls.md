# Finding 425 — Display-Mode Selection State and Controls

This finding expands the three `player-interface/display-mode-selection` functions from Issue #179. The canonical full decompile provides their bodies and neighboring control flow; the graphics-mode evidence bundle retains a direct analysis of `DisplayResolutionComboPopulate`, and the caller bundle records the available static call edges. Windows dialog callback dispatch can be indirect, so an empty static caller set does not establish that a routine is unused.

## Assigned functions

| Address | Function | Observed behavior |
| --- | --- | --- |
| `0x0049d780` | `DisplayResolutionComboPopulate` | Clears the list at dialog control `0x3ea`, asks `DirectDrawDeviceCreate @ 0x0047c2e0` to populate shared mode records, adds each mode whose record flag at `+0x100` contains `0x2000`, remembers the last marked row, stores its corresponding record pointer in `DAT_00568050`, and selects that row. On helper failure it adds the literal “Error: DirectX6 not installed.” and selects row zero. |
| `0x0049d870` | `DisplayResolutionFilterTupleAppend` | Writes three supplied values into the next filter record at `DAT_00568060` (stride `0x44` 32-bit words, or `0x110` bytes), then increments tuple count `DAT_005754e0`. `DirectDrawDeviceInitialize @ 0x0049d030` seeds `(640, 480, 16)` and, when its second argument is nonzero, also `(800, 600, 16)` and `(1024, 768, 16)`. |
| `0x0049d9d0` | `DisplayRefreshRateControlsSync` | Checks bit `0x20` of the selected adapter record at `DAT_0056804c + 0x110`. It enables or disables dialog control `0x3ed`, updates controls `0x3ec`/`0x3ed` with `BM_SETCHECK` messages, and calls `DisplayModeDialogSetModeAndRefreshChoices @ 0x0049d5e0` with mode `1` or `0` when its state requires a refresh-mode change. |

## Filter and dialog state flow

- `DirectDrawDeviceInitialize @ 0x0049d030` clears the selected adapter/mode pointers, initializes two mode-state globals to `1`, resets the filter tuple count, and appends the default 640×480×16 tuple. It conditionally appends two additional tuples. Its dialog path is supplied to `DialogBoxParamA` through callback address `0x0049d2f0`; the callback body is not present as a named manifest function in the canonical full corpus.
- The selection pointers represent three distinct record families: `DAT_0056804c` points to the chosen adapter descriptor in the 0x114-byte adapter table beginning at `0x005580ac`; `DAT_00568050` points to the chosen 0x114-byte display-resolution record beginning at `0x0055963c`; and `DAT_00568054` points to a chosen enumerated mode record in the 0x110-byte array beginning at `0x0055abcc`. The adapter pointer is passed to enumeration helpers and its capability flags at `+0x110` are checked by `DisplayRefreshRateControlsSync`. The resolution pointer is passed to the renderer initializer. The enumerated-mode pointer supplies dimensions and format when that initializer applies a mode.
- `DAT_00568058` selects the display path: value `1` initializes full-screen mode, while `0` initializes windowed mode. The same selector is passed to DirectDraw mode enumeration and controls whether the empty-list message refers to screen modes or bit modes.
- `DisplayResolutionFilterTupleAppend` stores the tuple fields literally. `DisplayModeMatchesFilterTuple @ 0x0049d8d0` consumes the tuple list: each field is compared with the corresponding enumerated mode value, and zero fields act as wildcards. `DirectDrawDeviceInitialize` calls the matcher while searching enumerated modes; `DisplayModeListPopulate @ 0x0049d630` also uses it when rebuilding the dialog list.
- `DisplayResolutionComboPopulate` operates on a separate list control and shared record buffer. The 0x114-byte record stride, the tested `+0x100` flag, and the chosen pointer are visible directly; the wider record schema is not established by this function alone. Its `DirectDrawDeviceCreate` call passes the adapter pointer and the shared buffer/count addresses. The callee’s decompiled prototype omits those output arguments, so their detailed contract remains based on the call site and data flow.
- `DisplayRefreshRateControlsSync` does not directly enumerate refresh rates. It delegates to `DisplayModeDialogSetModeAndRefreshChoices`, which stores its mode argument, calls `DisplayModeListPopulate`, and updates the text of control `0x3ee`. `DisplayModeListPopulate` obtains the mode list through `DirectDrawEnumeratedDeviceCreate @ 0x0047c4b0`, filters its records with `DisplayModeMatchesFilterTuple`, repopulates control `0x3eb`, and disables the default dialog action when there are no supported modes.
- Available caller evidence shows `DisplayResolutionFilterTupleAppend` called by `DirectDrawDeviceInitialize`, and `DisplayModeDialogSetModeAndRefreshChoices` called by `DisplayRefreshRateControlsSync`. No static callers are recorded for `DisplayResolutionComboPopulate` or `DisplayRefreshRateControlsSync`; the dialog callback path may reach them indirectly. The callback address `0x0049d2f0` is a clear next point to inspect before assigning concrete control meanings or event order.

## Evidence and limits

- `docs/decompiles/full/LEGORR.full.c`: direct pseudocode for the three assigned bodies and neighboring functions at `0x0049d030`, `0x0049d5e0`, `0x0049d630`, and `0x0049d8d0`.
- `docs/evidence/legorr-graphics-mode-surfaces.bundle`: direct `DisplayResolutionComboPopulate` analysis `ev_f91f8dc45acc36ce5ded2d6056a155b0fc9474a9301e40a6d3ead482026d8537`.
- `docs/evidence/legorr-engine-helper-audit-text-io-expansion-9.bundle`: caller observations `ev_1064a599e52d6f62fd8f74e562d1ceb0012b38b6a8e420237364213d8639e9ec` (`0x49d870`), `ev_c764178d49c6918ac218b1dbd4ade13767075e63a624c5d30649e3fe263037da` (`0x49d780`), `ev_e15d949989899b1d7acfa3c9b036ec4084cbf8bcb29cf0d8510461d3f0355305` (`0x49d9d0`), and `ev_063bdde652592c453554a48c8d747df0428e2f540e7d892383383b6b5fd92ff1` (`0x49d5e0`).
- Earlier Finding 282 records the original naming pass for this display-mode helper cluster. This finding adds the assigned behavior and the filter/control relationships; it does not infer user-facing meanings for the undocumented state globals.

## Open questions and focused follow-ups

1. Recover and trace dialog callback `0x0049d2f0` to establish the event paths that populate the resolution list and synchronize refresh controls.
2. Identify the structure passed through the display-mode buffers, including the meaning of flag `0x2000`, the capability bit `0x20`, and the fields used for width/height/depth matching.
3. Trace initialization and mutation of `DAT_0056805c` through dialog creation and user events before assigning a role to that remaining mode-state value.

## Coverage index impact

All three assigned addresses already have `display-mode-selection` rows in `docs/coverage/function-areas.csv`, `docs/coverage/function-areas.md`, and `docs/coverage/ledger.csv`. No function-area or ledger update is required; the new finding is added to the README findings index.
