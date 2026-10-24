// SPDX-License-Identifier: LGPL-3.0-only
#include <stdlib.h>

#include "heap.hpp"

/* Engine heap seam implementation — deviations in heap.hpp. */

void* engine_heap_alloc_aligned(long size)
{
    /* The corpus body: 16-byte round-up, then (pool absent) the heap
     * allocation. No gate of its own — the entry points check first. */
    unsigned rounded = ((unsigned)size + 15u) & 0xfffffff0u;
    if (rounded == 0u) {
        rounded = 16u; /* size 0 allocates one byte through the heap */
    }
    return aligned_alloc(16u, rounded);
}

void* engine_heap_alloc_retry(long size, int (*retry_handler)(long))
{
    if ((unsigned)size == 0u) {
        size = 1; /* corpus zero-size quirk */
    }
    if ((unsigned)size >= 0xffffffe1u) {
        return NULL;
    }
    for (;;) {
        void* p = engine_heap_alloc_aligned(size);
        if (p != NULL) {
            return p;
        }
        if (retry_handler == NULL) {
            return NULL; /* unregistered handler: no retry (corpus parity) */
        }
        if (retry_handler(size) == 0) {
            return NULL;
        }
    }
}

void* engine_heap_alloc(long size)
{
    /* Corpus: EngineHeapAllocateWithRetry(size, DAT_00549154) with the
     * retry global unset in the port — single attempt. */
    return engine_heap_alloc_retry(size, NULL);
}

void engine_heap_free(void* ptr)
{
    if (ptr != NULL) {
        free(ptr);
    }
}

void engine_heap_free_wrapper(void* ptr)
{
    engine_heap_free(ptr);
}

void ui_render_record_free(void* record)
{
    engine_heap_free(record);
}

void engine_heap_allocate(long size)
{
    (void)engine_heap_alloc(size);
}

/* --- the retry-handler pair (see heap.hpp) ----------------------------------
 * The corpus handler word (DAT_00549158, registered through the CRT's
 * set_new_handler machinery — unported) lands as zero-init storage with
 * the port-side setter; nothing registers while that seam is absent, so
 * the invoke answers 0 and EngineCalloc behaves like the plain entry —
 * the corpus's own unset-handler reading. */
static int (*s_new_handler)(long);

int crt_new_handler_invoke(long size)
{
    if (s_new_handler != NULL) {
        if (s_new_handler(size) != 0) {
            return 1; /* the handler asked for another attempt */
        }
    }
    return 0;
}

void crt_new_handler_set(int (*handler)(long))
{
    s_new_handler = handler;
}

void* engine_calloc(long size)
{
    return engine_heap_alloc_retry(size, crt_new_handler_invoke);
}

/* OptionalResourceDestroy (FUN_0040f7b0) — see heap.hpp. */
void optional_resource_destroy(void* record)
{
    if (record != NULL) {
        engine_heap_free(record);
    }
}

/* TextBufferDestroy (FUN_00485190) — see heap.hpp. */
void text_buffer_destroy(void* record)
{
    if (record == NULL) {
        return; /* deviation: the corpus strides the record blindly */
    }
    void* buffer = NULL;
    __builtin_memcpy(&buffer, (char*)record + 0x14, sizeof(buffer));
    engine_heap_free(buffer);
    engine_heap_free(record);
}
