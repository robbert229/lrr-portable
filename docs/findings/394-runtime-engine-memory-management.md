# Finding 394 — Runtime and Engine Memory Management

This finding maps the 24 functions assigned to `platform-and-runtime/runtime-memory-management`. Evidence is the canonical full decompile corpus (`docs/decompiles/full/LEGORR.full.c`); direct caller relationships are observed in that corpus. Pool thresholds, metadata values, and global meanings are left literal where their wider lifecycle is not established.

## Allocation and release entry points

- `EngineHeapAllocate @ 0x0047ff30` forwards its size argument to `EngineHeapAlloc @ 0x0048dbd0`; `EngineHeapFree @ 0x0047ff40` forwards to `EngineHeapFree @ 0x0048db80`. `GameFileHandleAllocate @ 0x0047fed0` and `GameFileHandleRelease @ 0x0047ff50` use these wrappers.
- `EngineHeapAlloc @ 0x0048dbd0` calls `EngineHeapAllocateWithRetry @ 0x0048dbf0`, passing the allocation size and global `DAT_00549154` as the retry-enable flag.
- `EngineHeapAllocateWithRetry @ 0x0048dbf0` rejects sizes at or above `0xffffffe1`, converts zero to one byte, and rounds/allocates through `EngineHeapAllocateAligned @ 0x0048dc40`. On failure it returns null if retries are disabled; otherwise it invokes `CrtNewHandlerInvokeForAllocationRetry @ 0x00491d90` and retries while that callback reports success.
- `EngineHeapAllocateAligned @ 0x0048dc40` rounds to 16-byte alignment. If the aligned size is at most `DAT_004b1fdc`, it first tries the small pool through `EnginePoolAllocate @ 0x00491900`; otherwise, or if the pool has no slot, it calls `HeapAlloc` on `DAT_0076dec4`. The exact threshold stored at `DAT_004b1fdc` is unresolved here.
- `EngineHeapFree @ 0x0048db80` ignores null. It asks `CRTHeapLocateAllocationPage @ 0x00491840` whether the pointer belongs to the engine pool. Pool allocations are returned through `SmallHeapPageFreeSlotUpdate @ 0x004918a0`; other pointers are passed to `HeapFree(DAT_0076dec4, ...)`.
- `EngineCalloc @ 0x0048ead0` calls the retry allocator with flag 1. Its visible body does not clear returned memory; zero initialization is explicit in `CrtCallocAligned @ 0x004997c0`, so the assigned alias `EngineCalloc` should not be taken as evidence of calloc semantics. Callers include `DirectShowRenderSurfaceCreate @ 0x00472560`, `AviFrameRetrieverOpen @ 0x0047e800`, and `AviPlayerCreate @ 0x0047ec90`.
- `CrtCallocAligned @ 0x004997c0` multiplies element count and element size, rounds nonzero byte counts up to 16 (using 16 bytes for a zero product), then allocates from the pool or process heap. It zeroes pool memory explicitly and asks `HeapAlloc` for zeroed memory on the heap path. On allocation failure it retries through the new-handler callback while `DAT_00549154` is nonzero. The decompile does not show a multiplication-overflow check before rounding.
- `CrtNewHandlerInvokeForAllocationRetry @ 0x00491d90` invokes callback `DAT_00549158` with the attempted size and returns whether it requested another allocation attempt. It is called by the normal allocator, the reallocator, and `CrtCallocAligned`.

## Engine pool and heap lifecycle

- `CrtHeapInitialize @ 0x00491560` creates the process heap with `HeapCreate(1, 0x1000, 0)`, stores it in `DAT_0076dec4`, and initializes an engine pool arena. It destroys the process heap and fails if arena initialization fails. The corpus directly calls it from `CrtNlgFrameStateRecord @ 0x0048efe6`.
- `EnginePoolArenaCreate @ 0x004915a0` allocates or selects an arena descriptor, reserves `0x400000` bytes with `VirtualAlloc`, and commits the first `0x10000` bytes. It links dynamic descriptors into the circular list rooted at `PTR_LOOP_004affb8` (or initializes the static descriptor), initializes per-page metadata and free-space sentinels, and initializes page headers. `CrtHeapInitialize` and `EnginePoolAllocate` call it.
- `EnginePoolAllocate @ 0x00491900` receives a requested size in 16-byte units. It searches arena page descriptors and calls `EnginePoolChunkTryAllocate @ 0x00491b40`; on success it updates the free-run metadata and preferred-arena cursor `PTR_LOOP_004b1fd8`. If no free page is available, it creates an arena or commits a run of pages with `VirtualAlloc`, initializes their page metadata, and allocates from the first one. Callers include `EngineHeapAllocateAligned`, `GrowHeapArray`, and `CrtCallocAligned`.
- `EnginePoolChunkTryAllocate @ 0x00491b40` searches a page’s byte metadata for a run large enough for the requested number of 16-byte units, updates the run and free-space cursor, and returns a computed pointer into the chunk. Its metadata byte stores the allocation size in 16-byte units, which the free path adds back to the page’s free-run count; the wider meaning of all sentinel values should be cross-checked across allocate, free, and resize paths before assigning names.
- `CRTHeapLocateAllocationPage @ 0x00491840` scans the arena list to find a pointer strictly inside a pool’s reserved address range; it does not independently confirm that the containing page is committed. It rejects pointers not 16-byte aligned and pointers in the first `0x100` bytes of a 4 KB page. For an accepted pointer it returns the arena descriptor, page base, and computed slot metadata address. It is used by the allocator’s free and reallocation paths.
- `SmallHeapPageFreeSlotUpdate @ 0x004918a0` finds page metadata from the arena descriptor and page base, adds the released block’s recorded size to the free-run count, clears that block’s size byte, and resets the page cursor. When a page reaches `0xf0` free units it increments `DAT_00549150`; at `0x20` free pages it asks `CRTHeapReleaseUnusedPages @ 0x00491770` to release up to 16.
- `CRTHeapReleaseUnusedPages @ 0x00491770` scans arena page metadata and decommits free 4 KB pages until the requested number are released or it runs out. It updates free-page metadata and the `DAT_00549150` count. If an arena has no remaining committed pages, it calls `CrtHeapBlockFree @ 0x00491710`.
- `CrtHeapBlockFree @ 0x00491710` releases an arena’s reserved virtual address range, removes a dynamic descriptor from the circular arena list and frees it, or marks the static descriptor as uninitialized by setting `DAT_004affc8` to `-1`.

## Reallocation and memory primitives

- `GrowHeapArray @ 0x0048dc80` handles null input by allocating, and zero new size by freeing and returning null. It rounds ordinary sizes to 16 bytes. For process-heap pointers it uses `HeapReAlloc`; for pool pointers it first tries `SmallHeapBlockResizeInPlace @ 0x00491cc0`, then tries a new pool block or heap allocation and copies the smaller of old capacity and requested size before returning the old pool slot. Allocation failures invoke the new handler when retries are enabled. Direct callers span `ListAppend @ 0x00411260`, `SaveMenuListEntryAppend @ 0x00410740`, `OsfAdd @ 0x00459190`, and `BindModelTextures @ 0x00481820`.
- `SmallHeapBlockResizeInPlace @ 0x00491cc0` uses the stored block-size byte (units of 16) to resize a pool block without moving it. Shrinking updates the size tag and returns slots to the page’s free-run count. Growing succeeds only if the intervening bytes are free and the expanded block remains within the chunk; otherwise it returns failure for the caller to allocate/copy. `GrowHeapArray` is its direct caller.
- `StackProbeAllocate @ 0x0048e250` walks downward through stack pages in `0x1000` byte steps and writes the return address at the final target, forcing the requested stack range to be touched. It is called before large stack allocations in save, terrain, panel, objective, vehicle, and WAD loading paths; this is stack probing, not heap allocation.
- `CrtMemoryMove @ 0x00498000` returns the destination after copying the requested byte count. It copies backward for overlapping ranges where destination begins inside the source range, and forward otherwise, with aligned word loops and byte tails. Callers include `StringPrefixReserve @ 0x004907c0` and `CxxCatchObjectBindOrCopy @ 0x004957d0`.

## Pointer-range checks

- `PointerRangeIsReadable @ 0x00499690`, `PointerRangeIsWritable @ 0x004996b0`, and `FunctionPointerIsExecutable @ 0x004996d0` wrap `IsBadReadPtr`, `IsBadWritePtr`, and `IsBadCodePtr`, respectively, and invert the result. `CxxCatchObjectBindOrCopy @ 0x004957d0` calls all three; `CxxExceptionHandlerSearch @ 0x00495160` calls the readable-range wrapper. These checks describe the CRT’s validation attempt, not a durable guarantee about later access.

## State flow and cross-area relationships

- **Small-allocation flow (directly visible):** `EngineHeapAlloc` → retry loop → aligned allocator → pool threshold → `EnginePoolAllocate` → per-page run allocator. Free reverses the path by classifying the pointer, then updating pool page metadata or calling `HeapFree`.
- **Reallocation flow (directly visible):** `GrowHeapArray` classifies the old pointer, attempts an in-place resize for pool blocks, otherwise allocates a replacement, copies the preserved prefix, and releases the old pool slot. This supports the use of the engine pool by generic lists and save/config/resource consumers without making those callers allocator owners.
- **Allocation-failure flow (directly visible):** normal allocation, reallocation, and aligned zero allocation can call the global new-handler callback and retry. The meaning and initialization site of `DAT_00549154` and `DAT_00549158` should be established from a broader startup trace.
- **Process heap and arena lifetime (partly inferred):** `DAT_0076dec4` is the `HeapCreate` result passed to heap allocation/free APIs. The arena list and `DAT_004b1fdc` threshold govern the separate small-block pool. The complete process shutdown path and whether all arenas are destroyed are outside these function bodies.

## Open questions and focused follow-ups

1. Trace initialization and mutation of `DAT_004b1fdc`, `DAT_00549150`, `DAT_00549154`, and `DAT_00549158` to establish pool threshold, free-page counter, and retry/new-handler semantics.
2. Verify all small-pool metadata sentinels and units (`0xf0`, `0xf1`, `-1`) across allocate, free, resize, and decommit paths; determine chunk boundaries and allocation-header encoding.
3. Trace shutdown to determine whether the process heap and every arena reservation are released on normal and abnormal termination.
4. Check `CrtCallocAligned` product multiplication and `GrowHeapArray` size arithmetic for overflow behavior at the actual target ABI; the pseudocode has limits checks after some arithmetic.
5. Confirm the role/name of `EngineCalloc`: its assigned alias suggests zeroing, but its body only enables allocator retry and does not clear memory.
6. Reconcile the decompiler’s `void` signatures for `EngineHeapAlloc` and `EngineHeapAllocateAligned` with callers that consume allocation results; the allocation path appears to rely on the return register, but the displayed pseudocode omits explicit pointer returns.
7. Determine whether `IsBadReadPtr`/`IsBadWritePtr`/`IsBadCodePtr` validation callers use the results only as best-effort guards, and whether their TOCTOU limitations affect exception handling.

## Coverage index impact

All 24 assigned addresses already have `runtime-memory-management` rows in `docs/coverage/function-areas.csv`; this finding does not change their names or area assignments. No coverage CSV or generated `function-areas.md` update is required. The new finding is indexed in the repository README.
