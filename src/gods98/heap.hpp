// SPDX-License-Identifier: LGPL-3.0-only
/* Corpus engine heap seam (findings 397/243): the allocation entry points
 * the whole engine allocates through.
 *
 * Corpus chain: EngineHeapAlloc @ 0x47ff40/0x48db80 (free), 0x48dbd0 (alloc,
 * retry flag DAT_00549154), EngineHeapAllocateWithRetry @ 0x48dbf0 (zero→1
 * size, allocate through the aligned path, retry through the registered new
 * handler), EngineHeapAllocateAligned @ 0x48dc40 (16-byte round-up; small
 * allocations first through the pool, otherwise the OS HeapAlloc).
 *
 * Port deviations (documented; see docs/coverage/blocked.md):
 *  - the corpus's pool allocator, allocation-page bookkeeping and new-handler
 *    registry are unported — the port maps the seam onto the host allocator,
 *    so every "attempt" is a single aligned malloc and failures answer NULL
 *    (the retry handler is absent, exactly the corpus's unregistered case);
 *  - allocations are 16-byte aligned because the corpus round-ups do (the
 *    aligned path is shared by both entry points). */
#ifndef LEGORRR_GODS98_HEAP_HPP
#define LEGORRR_GODS98_HEAP_HPP

#ifdef __cplusplus
extern "C" {
#endif

/* FUN_0048dbf0: allocate `size` bytes; size 0 allocates 1 (corpus quirk);
 * sizes at/above 0xffffffe1 fail with NULL (corpus overflow gate). With a
 * NULL retry handler the port makes the single attempt the corpus makes
 * when nothing is registered: on failure, NULL — no spin. */
void* engine_heap_alloc_retry(long size, int (*retry_handler)(long));

/* FUN_0048dbd0: the plain entry — same path with the corpus's retry-flag
 * global, which the port leaves unregistered (single attempt). */
void* engine_heap_alloc(long size);

/* FUN_0048dc40: the aligned allocator body. The corpus tries its pool under
 * the small-size gate first (TODO(corpus) — absent here) and then the OS
 * heap; the port goes straight to the aligned host allocation. */
void* engine_heap_alloc_aligned(long size);

/* FUN_0048db80: free a previous allocation (NULL is a no-op). */
void engine_heap_free(void* ptr);

/* FUN_0047ff40: the resource-layer free wrapper (identical call). */
void engine_heap_free_wrapper(void* ptr);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_HEAP_HPP */
