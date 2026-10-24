# Finding 411 — Interface Records and DirectX Diagnostics

This pass connects generic COM-style interface wrappers and records to the
resource-tree helpers and DirectX diagnostic path. The numeric object fields,
GUIDs, and several vtable slots remain opaque where their callers do not
establish a stronger meaning.

| Address | Name | Observed behavior and relationship |
| --- | --- | --- |
| `0x00474760` | `InterfaceArrayForwardSync` | Chooses one of two interfaces from a record-kind field, enumerates values through vtable slot `+0x80`, and forwards them through slots `+0x48` and `+0xa0`; it also mirrors values to a child interface and updates bit 2 in the record's `+0x14` state word. `ResourceListUpdateMatchingKey` reaches it through renderer update relays. See Finding 258 for the full relay and selector behavior. |
| `0x00477310` | `ComPropertyResultGet` | Calls the interface at record `+0x10` through vtable slot `+0x64`, initializes a two-word local result, releases temporary interface references, and invokes the record callback. The decompile shows the function returning the local result buffer; its lifetime and ABI interpretation need caller evidence before assigning stronger semantics. |
| `0x0047a540` | `ObjectFieldBE4DwordGet` | Returns the dword at object offset `+0xbe4`; the field's meaning is unresolved. See Finding 361. |
| `0x0047e300` | `InterfaceMethod18StructApply` | Builds a zeroed `0x7c`-byte request with type `7`, copies the supplied values into its request fields, and submits it through global interface `DAT_0076ac84` vtable slot `+0x18`. A nonzero result is sent to `DirectXResultDiagnosticDispatch` with the embedded source location; on success it calls the supplied interface's slot `+0x14` and initializes a result record through `InterfaceMethodResultRecordInitialize`. The vtable semantics remain unidentified. |
| `0x0047e3e0` | `InterfaceMethodResultRecordInitialize` | Ensures a global lookup table is initialized, then fills an eight-word record with the supplied object/context values, two helper lookup results, a zero field, and a self pointer at `+0x1c`. Its higher-level record purpose is unresolved. See Finding 374. |
| `0x0047e460` | `ComObjectDestroyAndOptionallyFree` | Runs the object cleanup routine, which switches to a destructor vtable, releases non-null interface fields at `+0x18` and `+0x10`, and clears them. It then frees the enclosing object when input bit 0 is set. See Finding 384. |
| `0x0047ed70` | `InterfaceInvokeOneIfNonNull` | If the supplied interface pointer is non-null, invokes its first vtable method with argument `1`; the interface method's meaning is not established. |
| `0x0047f600` | `DebugOutputFormat` | Formats variadic arguments into a 1024-byte stack buffer and passes the resulting string to `OutputDebugStringA`. Multiple file and resource error paths use it. |
| `0x00480780` | `ZeroResultIsSuccess` | Calls the global interface `DAT_00540580` through vtable slot `+0x0c` with the supplied value and returns whether that call returns zero. See Finding 373. |
| `0x00482210` | `PointerValueGet` | Returns the first pointer-sized value at the supplied address; its callers use it as a generic indirection helper. See Finding 381. |
| `0x00485ea0` | `InterfaceSlot58PairStoreClamp` | Stores positive values from the supplied four-float rectangle as lower bounds, submits a zeroed `0x7c`-byte request through global interface `DAT_0076ac8c` slot `+0x58`, then stores the returned pair and clamps its upper bounds to the supplied rectangle. The device slot's exact operation is unresolved. |
| `0x00487d50` | `ComInterfaceQueryAndContextDispatch` | When record bit 0 at `+0x28` is clear, selects an interface using the record's 16-bit index, obtains a child through slot `+0x80`, queries GUID `DAT_0049fb48`, releases the temporary interface, obtains a value through slot `+0x1c`, and dispatches it with context at `+0x10`. The queried interface's domain is unresolved. See Finding 386. |
| `0x00488730` | `CaseInsensitivePairTableFind` | Scans `count` entries of two dwords each, compares the key at entry `+0` case-insensitively, and returns the paired dword at `+4`, or zero when absent. A stored zero is therefore indistinguishable from a miss. See Finding 371. |
| `0x00488790` | `CaseInsensitivePairTableAppend` | Duplicates the supplied key through `StringDuplicate` (`0x00477550`), appends the copy and paired value to the next eight-byte slot, and increments the count. See Finding 371. |
| `0x00488a70` | `LoadRecordInterfacesReleaseFree` | For records whose flag bit 0 at `+0x2a` is clear, queries the indexed interface slot, releases temporary interfaces, and passes the result through `FUN_004820d0`. When fields `+0x2c` and `+0x30` are both zero, it frees the record's base strings and, when the 16-bit field at `+0x26` is nonzero, two additional strings. The field meanings remain unresolved. See Finding 256 for its destroy call chain. |
| `0x004894c0` | `ComInterfaceQueryOrFallback` | Queries the requested interface on global object `DAT_00505914`; when that succeeds, queries the same identifier on a supplied fallback object and dispatches the result through a wrapper's vtable slot `+0x30`. It returns null when the first query fails. See Finding 381. |
| `0x00489bc0` | `ComChildItemsVisit` | Uses interface slot `+0x80` to obtain a child count and child array, then invokes slot `+0x48` on each returned child. It is called from the object-hierarchy clone path; slot meanings are unresolved. See Finding 386. |
| `0x00489d70` | `ObjectField38Release` | If object offset `+0x38` is non-null, releases that pointer through the engine heap-free helper. The pointed-to type is unresolved. See Finding 373. |
| `0x00489d90` | `DirectXErrorMessageFormat` | Formats a shared diagnostic string from the current result, source location, module/category label, and supplied symbolic error name and message. The output buffer and context are held in globals. See Finding 227. |
| `0x00489dd0` | `DirectXResultDiagnosticDispatch` | Stores the result, source location, category, and incremented diagnostic counter in globals, then maps recognized result codes to symbolic names and text for categories 1 (D3DRM), 2 (DirectInput), and 3 (DirectDraw) through `DirectXErrorMessageFormat`. Categories 4–6 report unsupported handling, category 7 reports a query-interface error, and other categories/codes use generic diagnostics. It returns the original result. See Findings 225 and 254. |

## Shared behavior

The functions fall into three connected paths. Interface wrappers construct or
query COM-style values, release temporary references, and dispatch selected
results to callbacks or context records. Resource-tree and load-record helpers
enumerate child interfaces, keep case-insensitive key/value tables, and release
interfaces or heap-backed fields according to literal record checks. The
diagnostic pair records the failing code and source context, translates known
DirectInput, DirectDraw, and D3DRM results, and formats a shared message.

These paths share conventions, but the evidence does not establish a common
record type or universal meaning for their numeric vtable slots and flags.
Those meanings should remain tied to the specific call paths above.

## Evidence and related findings

The complete checked-in Ghidra pseudocode for the 20 assigned functions and
their callers is in [`LEGORR.full.c`](../decompiles/full/LEGORR.full.c).
Related traces are [Finding 225](225-terrain-objective-level-and-diagnostic-helpers.md),
[Finding 227](227-newly-identified-engine-and-terrain-helpers.md),
[Finding 254](254-command-sweeps-cell-issuing-config-priorities-statistics-and-directdraw-create-helpers.md),
[Finding 256](256-command-band-step-issuers-flag-behavior-timers-and-steering-helpers.md),
[Finding 258](258-interface-method-wrappers-overlay-refresh-and-deferred-set-relays.md),
[Finding 361](361-sound-window-and-resource-interface-wrappers.md),
[Finding 371](371-record-pools-pair-tables-and-buffered-streams.md),
[Finding 372](372-incident-messages-options-and-resource-helpers.md),
[Finding 373](373-cursor-simulation-renderer-and-file-helpers.md),
[Finding 374](374-terrain-interface-and-media-helpers.md),
[Finding 381](381-selection-resource-runtime-and-grid-helpers.md),
[Finding 384](384-crt-formatting-com-cleanup-and-spawn-helpers.md), and
[Finding 386](386-spatial-ui-com-and-crt-runtime.md).
