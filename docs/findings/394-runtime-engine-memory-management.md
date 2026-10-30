# Finding 394 — Runtime and Engine Memory Management

This finding maps the 24 functions assigned to `platform-and-runtime/runtime-memory-management`. Evidence is the canonical full decompile corpus (`docs/decompiles/full/LEGORR.full.c`); direct caller relationships are observed in that corpus. Pool thresholds, metadata values, and global meanings are left literal where their wider lifecycle is not established.

## Allocation and release entry points

- `EngineHeapAllocate @ 0x0047ff30` forwards its size argument to `EngineHeapAlloc @ 0x0048dbd0`; `EngineHeapFree @ 0x0047ff40` forwards to `EngineHeapFree @ 0x0048db80`. `GameFileHandleAllocate @ 0x0047fed0` and `GameFileHandleRelease @ 0x0047ff50` use these wrappers.
- `EngineHeapAlloc @ 0x0048dbd0` is VC5 `LIBC.LIB`'s `_malloc` entry. MSVC call sites alias directly to `malloc`; its runtime path uses `__nh_malloc @ 0x0048dbf0` with the current retry mode, then `__heap_alloc @ 0x0048dc40` for the aligned block.
- `__nh_malloc @ 0x0048dbf0` rejects sizes at or above `0xffffffe1`, converts zero to one byte, and retries through `__heap_alloc @ 0x0048dc40`; on failure it invokes `__callnewh @ 0x00491d90` when retries are enabled. `engine_heap_alloc_retry` now aliases directly to this VC5 entry on MSVC.
- `__heap_alloc @ 0x0048dc40` rounds to 16-byte alignment. If the aligned size is at most `DAT_004b1fdc` (480 bytes), it first tries the small pool through `EnginePoolAllocate @ 0x00491900`; otherwise, or if the pool has no slot, it calls `HeapAlloc` on `DAT_0076dec4`. MSVC call sites now alias directly to `_heap_alloc`; other backends retain the local pool implementation.
- `EngineHeapFree @ 0x0048db80` links to VC5 `LIBC.LIB`'s `_free`; other backends retain the local pool-page check and platform heap release.
- `CRTHeapInitialize @ 0x00491560` aliases directly to VC5 `LIBC.LIB`'s `__heap_init` on MSVC; other backends retain the explicit process-heap and arena initialization.
- `EngineCalloc @ 0x0048ead0` retains its four-byte target wrapper because no VC5 library entry matches this function. Its MSVC body calls VC5 `__nh_malloc(size, 1)` directly; the runtime path does not clear returned memory, while zero initialization is explicit in `CrtCallocAligned @ 0x004997c0`. Callers include `DirectShowRenderSurfaceCreate @ 0x00472560`, `AviFrameRetrieverOpen @ 0x0047e800`, and `AviPlayerCreate @ 0x0047ec90`.
- `CrtCallocAligned @ 0x004997c0` aliases directly to VC5 `LIBC.LIB`'s `_calloc`, which supplies the reference build's product calculation, aligned allocation, zero-fill, and allocation-failure behavior. Other backends retain the local pool-and-zero-fill implementation.
- `CrtNewHandlerInvokeForAllocationRetry @ 0x00491d90` aliases directly to VC5 `LIBC.LIB`'s `__callnewh`; the `crt_new_handler_set` hook now calls `_set_new_handler` directly on MSVC, while other backends retain the local callback slot. It is called by the normal allocator, the reallocator, and `CrtCallocAligned`.

## Engine pool and heap lifecycle

- `CrtHeapInitialize @ 0x00491560` links to VC5 `LIBC.LIB`'s `_heap_init` on MSVC, which initializes the CRT heap and its small-block heap. Other backends create the process heap and initialize the portable pool. The corpus directly calls it from `CrtNlgFrameStateRecord @ 0x0048efe6`.
- `EnginePoolArenaCreate @ 0x004915a0` is VC5 `sbheap.obj`'s `__sbh_new_region`; MSVC gets the runtime implementation, while other backends retain the local arena setup. It reserves `0x400000` bytes, commits the initial pages, and initializes page metadata.
- `EnginePoolAllocate @ 0x00491900` is VC5 `sbheap.obj`'s `__sbh_alloc_block`. The runtime scans region page descriptors and uses `__sbh_alloc_block_from_page`; portable builds retain the local search, commit, and allocation loops. Callers include `EngineHeapAllocateAligned`, `GrowHeapArray`, and `CrtCallocAligned`.
- `EnginePoolChunkTryAllocate @ 0x00491b40` is VC5 `sbheap.obj`'s `__sbh_alloc_block_from_page`. It searches a page’s byte metadata for a run large enough for the requested number of 16-byte units, updates the run and free-space cursor, and returns a pointer into the chunk. Other backends retain the local equivalent.
- `CRTHeapLocateAllocationPage @ 0x00491840` is VC5 `sbheap.obj`'s `__sbh_find_block`. It locates a pointer within the small-block heap and returns its region, page base, and allocation tag. Other backends retain the local range and alignment checks.
- `SmallHeapPageFreeSlotUpdate @ 0x004918a0` is VC5 `sbheap.obj`'s `__sbh_free_block`; the runtime releases the block and updates its page metadata. Other backends retain the local update path.
- `CRTHeapReleaseUnusedPages @ 0x00491770` is VC5 `sbheap.obj`'s `__sbh_decommit_pages`; it scans and decommits free 4 KB pages, updating runtime metadata and releasing an empty region when needed. Other backends retain the local decommit loop.
- `CrtHeapBlockFree @ 0x00491710` is VC5 `sbheap.obj`'s `__sbh_release_region`, which releases a reserved region and updates the CRT region list. Other backends retain the local implementation.

## Reallocation and memory primitives

- `GrowHeapArray @ 0x0048dc80` is VC5 `LIBC.LIB`'s `_realloc` entry from `realloc.obj`. The runtime handles null input by allocating, and zero new size by freeing and returning null. It rounds ordinary sizes to 16 bytes. For process-heap pointers it uses `HeapReAlloc`; for pool pointers it first tries `SmallHeapBlockResizeInPlace @ 0x00491cc0`, then tries a new pool block or heap allocation and copies the smaller of old capacity and requested size before returning the old pool slot. Allocation failures invoke the new handler when retries are enabled. Direct callers span `ListAppend @ 0x00411260`, `SaveMenuListEntryAppend @ 0x00410740`, `OsfAdd @ 0x00459190`, and `BindModelTextures @ 0x00481820`. MSVC builds now link this runtime body; non-MSVC builds retain the ledger placeholder.
- `SmallHeapBlockResizeInPlace @ 0x00491cc0` is VC5 `sbheap.obj`'s `__sbh_resize_block`; MSVC links the runtime entry, while other builds retain the stub until the portable resize path is implemented. `GrowHeapArray` is its direct caller.
- `StackProbeAllocate @ 0x0048e250` is VC5's compiler stack-probe helper, not game allocation code. The target instruction sequence exactly matches `__alloca_probe` / `__chkstk` in VC5 `LIBC.LIB`'s `chkstk.obj`: it touches each 4 KB page, moves ESP to the requested location, and restores the caller's return path. The linker pulls this helper in automatically for large stack frames, so it is tracked as a runtime-library entry rather than ported or stubbed.
- `CrtMemoryMove @ 0x00498000` returns the destination after copying the requested byte count. It copies backward for overlapping ranges where destination begins inside the source range, and forward otherwise, with aligned word loops and byte tails. The MSVC build now routes callers directly to VC5 `memmove` (100% Reccmp match); portable builds retain the forwarding implementation. Callers include `StringPrefixReserve @ 0x004907c0` and `CxxCatchObjectBindOrCopy @ 0x004957d0`.

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

All 24 assigned addresses remain in `runtime-memory-management`. `GrowHeapArray @ 0x0048dc80` and the eight `sbheap.obj` entries are now classified as `runtime-library` in the coverage ledger; their area rows use `import-symbol` evidence. The generated coverage summary and function-area index have been refreshed.
