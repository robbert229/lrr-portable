# Finding 401 — Task Callback Record Allocator

This finding maps the single function assigned to `engine-callback-records`. Its recovered body allocates a fixed-size record, performs virtual calls through an object/interface pointer, stores callback and ownership-related fields, then sets a mode-dependent flag. Several callers use it while creating render, sound, and timed visual resources; the mode values and object field semantics are not fully identified.

| Address | Name | Observed behavior | Caller/state flow |
| --- | --- | --- | --- |
| `0x00480870` | `TaskCallbackRecordAllocateAndInitialize` | Acquires a zeroed 0x34-byte record through `FixedSizePoolRecordAcquire` (`0x004807a0`), calls `FUN_00472940`, and makes three virtual calls using the acquired record and other pointers. It stores `LAB_004827f0` at record `+0x10`, a pointer to its `+0x0c` slot at `+0x14`, the record itself at `+0x18`, and the recovered `unaff_EDI` value at `+0x28`. It sets `+0x20` to 1 and conditionally sets flag 2 or 4 when `unaff_EDI` equals 1 or 2. The meaning of this value is unresolved. | Callers pass five apparent arguments, but Ghidra recovered no explicit parameters; the body uses `unaff_EDI` for the equality tests. Preserve that calling-convention uncertainty when interpreting fields and modes. Callers include render-record, sound-stream, timed-visual, scene-object-mode, and LWO model-loading paths. |

## Caller evidence

- `QuadMeshRenderRecordCreate` (`0x0040a4b0`) supplies mode `0x3000`, callback `LAB_0040a740`, an owner record, and trailing zero. It stores the returned handle in its render record before creating four mesh elements.
- `SoundStreamSelectorUpdate` (`0x00465430`) supplies mode `0x4800`, callback `LAB_00465ee0`, and its current sound/resource record; it stores the returned handle at record offset `0xa4` before building segment geometry.
- `TimedEntitySegmentVisualCreate` (`0x00470690`) makes two mode `0x4400` calls with null callback/context values and stores both returned handles in global per-slot arrays before configuring the visuals.
- `DDiSceneObjectKindModeSetup` (`0x00473560`) passes one of `0x2000`, `0x3000`, or `0x2800` for scene-object modes 2, 3, or 4 and passes the resulting handle to `FUN_00475c00`.
- `LwoModelEntryLoad` (`0x004809e0`) passes mode `0x3000` and trailing value 2, then forwards the returned handle to `MaterialPass` (`0x00480ac0`).

## Data paths and boundaries

- The pool helper `0x004807a0` grows the pool through `0x004807f0` when its free list is empty, pops a 0x34-byte record, clears 13 dwords, and sets the record's self-link at `+0x30`. Thus the returned record starts zeroed before `0x00480870` fills selected fields.
- `0x00480870` stores `unaff_EDI` at `+0x28` and compares that same recovered value with 1 and 2. Ordinary callers shown above pass other mode-like constants. Since parameters are not recovered, the relationship between caller arguments and `unaff_EDI` is unknown; avoid equating those values without instruction-level evidence.
- The callback stored at `+0x10` is always `LAB_004827f0` in the recovered body. Some call sites also visibly pass callback addresses, but the decompiled signature prevents confirming how those arguments relate to the stored field.
- The vtable calls at offsets `+0x58`, `+0x48`, and `+0x18` are indirect. Their exact interface contracts, ownership effects, and relationship to the record's lifecycle remain unresolved.

## Open questions and focused follow-up

- Recover the exact calling convention and parameter/register mapping for `0x00480870`, including why callers pass five values while the decompiler recovers no explicit parameters.
- Identify the interface behind `DAT_00505914`, the object stored at record `+0x28`, and the contracts for its virtual calls.
- Trace registration, callback execution, release, and pool-return paths for this record type; determine the meanings of offsets `+0x10` through `+0x30` and flags 2/4.
- Reconcile the caller-supplied callback values with the fixed `LAB_004827f0` field store using disassembly or runtime evidence.
