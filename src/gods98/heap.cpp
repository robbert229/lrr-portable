// SPDX-License-Identifier: LGPL-3.0-only
#include <stdlib.h>
#include <string.h>

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

/* TextOutputField82CBit2SetClear (FUN_00485160) — see heap.hpp. */
void text_output_field82c_bit2_set_clear(void* record, int set_bit)
{
    if (record == NULL) {
        return; /* deviation: the corpus strides the record blindly */
    }
    uint32_t word = 0;
    __builtin_memcpy(&word, (char*)record + 0x82c, sizeof word);
    word = (set_bit != 0) ? (word | 4u) : (word & ~4u);
    __builtin_memcpy((char*)record + 0x82c, &word, sizeof word);
}

/* ObjectField38Release (FUN_00489d70) — see heap.hpp. */
void object_field_38_release(void* record)
{
    if (record == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    void* field = NULL;
    __builtin_memcpy(&field, (char*)record + 0x38, sizeof(field));
    if (field != NULL) {
        engine_heap_free(field);
    }
}

/* ThreeWordRecordAllocateAndInitialize (FUN_00410b60) — see heap.hpp. */
void* three_word_record_allocate_and_initialize(uint32_t a, uint32_t b,
                                                uint32_t c)
{
    uint32_t* rec = (uint32_t*)engine_heap_alloc(12);
    if (rec == NULL) {
        return NULL; /* deviation: the corpus strides the record blindly */
    }
    rec[0] = a;
    rec[1] = b;
    rec[2] = c;
    return rec;
}

/* DwordListAllocateAndZero (FUN_00414070) — see heap.hpp. */
void** dword_list_allocate_and_zero(uint32_t count)
{
    void** hdr = (void**)engine_heap_alloc(2 * sizeof(void*));
    if (hdr == NULL) {
        return NULL; /* deviation: the corpus strides both halves blindly */
    }
    hdr[0] = NULL;
    hdr[1] = NULL;
    void* data = engine_heap_alloc((long)(count * sizeof(void*)));
    hdr[0] = data;
    /* Corpus shape: the zero-pass sweeps `count & 0x3fffffff` slots and the
     * second pass is dead code (loop seed 0) — kept faithfully. */
    void** p = (void**)data;
    const uint32_t n = count & 0x3fffffffu;
    for (uint32_t i = 0; i != n; ++i) {
        p[i] = NULL;
    }
    if (data == NULL && count != 0) {
        engine_heap_free(hdr); /* deviation: the corpus leaves both behind */
        return NULL;
    }
    hdr[1] = (void*)(size_t)count;
    return hdr;
}

/* TextBufferReset (FUN_004859b0) — see heap.hpp. The corpus zero-pass is
 * dword words then leftover bytes over the buffer, then the record's own
 * 0x100-dword window and the three flag words; the port folds the sweep
 * into byte-granular zeroing (identical observable bytes) and guards the
 * record (deviation: the corpus strides both blindly). */
void text_buffer_reset(void* record)
{
    if (record == NULL) {
        return; /* deviation: the corpus strides the record blindly */
    }

    uint32_t count = 0;
    void* buffer = NULL;
    __builtin_memcpy(&count, (char*)record + 0x418, sizeof(count));
    __builtin_memcpy(&buffer, (char*)record + 0x14, sizeof(buffer));
    if (buffer != NULL) {
        /* The corpus sweeps count>>2 dwords then count&3 bytes; the
         * observable effect is count zero bytes. */
        memset(buffer, 0, count);
    }
    /* Deviation: the corpus's buffer slot is a dword ending before the
     * window, so the window zeroing never touches it; the port's widened
     * slot straddles the window start, so the slot is restored from the
     * pre-reset buffer value (a null slot stays zeroed either way). */
    memset((char*)record + 0x18, 0, 0x100 * 4);
    __builtin_memcpy((char*)record + 0x14, &buffer, sizeof(buffer));

    const uint32_t zero = 0u;
    __builtin_memcpy((char*)record + 0x820, &zero, sizeof(zero));
    __builtin_memcpy((char*)record + 0x41c, &zero, sizeof(zero));
    uint32_t word = 0;
    __builtin_memcpy(&word, (char*)record + 0x82c, sizeof(word));
    word &= ~1u;
    __builtin_memcpy((char*)record + 0x82c, &word, sizeof(word));
}

/* UITextOutputObjectRelease (FUN_0045a620) — see heap.hpp. */
void ui_text_output_object_release(void* const* holder)
{
    if (holder == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    text_buffer_reset(*holder);
}
