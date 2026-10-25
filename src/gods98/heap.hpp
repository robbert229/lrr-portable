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
#include <stdint.h>

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

/* ThreeWordRecordAllocateAndInitialize (FUN_00410b60): allocate a 0xc-byte
 * record and store the three arguments in order. Deviation: the corpus's
 * void return leaves the record unobservable, so the port answers it for
 * test pinning (the corpus's effect — the store — is identical). */
void* three_word_record_allocate_and_initialize(uint32_t a, uint32_t b,
                                                uint32_t c);


/* FUN_0047ff40: the resource-layer free wrapper (identical call). */
void engine_heap_free_wrapper(void* ptr);

#ifdef __cplusplus
}
#endif

/* UiRenderRecordFree (FUN_0045daf0): the corpus's UI-side record free is
 * a bare tail call into the engine heap free — faithful wrapper. */
void ui_render_record_free(void* record);

/* EngineHeapAllocate (FUN_0047ff30): the corpus's plain alloc wrapper is
 * a bare call into the heap's alloc entry — faithful. */
void engine_heap_allocate(long size);

/* --- the retry-handler pair (2026-10-24; finding 394) ------------------------
 * CrtNewHandlerInvokeForAllocationRetry (FUN_00491d90): dispatches the
 * attempted size to the registered allocation-failure handler and answers
 * whether it asked for another attempt. The corpus registry (the
 * set_new_handler word) is unported — the port models it with its own
 * setter hook; unregistered answers 0 (no retry), the corpus's unset
 * case.
 * EngineCalloc (FUN_0048ead0): the corpus's retry-enabled allocation —
 * EngineHeapAllocateWithRetry(size, 1) — returning the block. */
int   crt_new_handler_invoke(long size);
void  crt_new_handler_set(int (*handler)(long)); /* port hook */
void* engine_calloc(long size);                  /* FUN_0048ead0 */

/* OptionalResourceDestroy (FUN_0040f7b0): frees the optional resource
 * record only when one is there — the corpus's gate over the engine free. */
void optional_resource_destroy(void* record);

/* TextBufferDestroy (FUN_00485190): frees the buffer held in the text
 * buffer record's +0x14 slot, then the record itself. The corpus's slot
 * is a 32-bit dword; the port carries a host-width pointer there
 * (documented deviation). A null record lands guarded — the corpus's
 * dereferences blindly. */
void text_buffer_destroy(void* record);

/* TextOutputField82CBit2SetClear (FUN_00485160): the text-output record's
 * +0x82c flag-word bit 2 set or clear. The corpus strides the record's
 * dword word blindly; the port guards the record and folds through the
 * widened model's dword window. */
void text_output_field82c_bit2_set_clear(void* record, int set_bit);
                                                /* FUN_00485160 */

/* ObjectField38Release (FUN_00489d70): frees the allocation held in the
 * record's +0x38 slot while one is there. The corpus's slot is a 32-bit
 * dword; the port carries a host-width pointer (documented deviation). */
void object_field_38_release(void* record);

#endif /* LEGORRR_GODS98_HEAP_HPP */
