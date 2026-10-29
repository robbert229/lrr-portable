// SPDX-License-Identifier: LGPL-3.0-only
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "heap.hpp"
#include "string.hpp"

/* Engine heap seam implementation — deviations in heap.hpp. */

// FUNCTION: LEGORR 0x0048dc40
void* engine_heap_alloc_aligned(long size)
{
    /* The corpus body: 16-byte round-up, then (pool absent) the heap
     * allocation. No gate of its own — the entry points check first. */
    unsigned rounded = ((unsigned)size + 15u) & 0xfffffff0u;
    if (rounded == 0u) {
        rounded = 16u; /* size 0 allocates one byte through the heap */
    }
#if defined(LEGORRR_BACKEND_WINAPI)
    void* base = malloc(rounded + 16u + sizeof(void*));
    if (base == NULL) {
        return NULL;
    }
    uintptr_t aligned = ((uintptr_t)base + sizeof(void*) + 15u) & ~15u;
    ((void**)aligned)[-1] = base;
    return (void*)aligned;
#else
    return aligned_alloc(16u, rounded);
#endif
}

// FUNCTION: LEGORR 0x0048dbf0
void* engine_heap_alloc_retry(long size, int retry_via_new_handler)
{
    /* Corpus order: the unsigned bound test runs before the zero-size
     * replacement, and the loop re-tests it before every allocation
     * (dead — the size is loop-invariant — but present in the corpus). */
    if ((unsigned)size > 0xffffffe0u) {
        return NULL;
    }
    if (size == 0) {
        size = 1; /* corpus zero-size quirk */
    }
    for (;;) {
        void* p;
        if ((unsigned)size <= 0xffffffe0u) {
            p = engine_heap_alloc_aligned(size);
        } else {
            p = NULL;
        }
        if (p != NULL) {
            return p;
        }
        if (retry_via_new_handler == 0) {
            return NULL; /* no-retry flag (corpus parity) */
        }
        if (crt_new_handler_invoke(size) == 0) {
            return NULL;
        }
    }
}

// FUNCTION: LEGORR 0x0048dbd0
void* engine_heap_alloc(long size)
{
    /* Corpus: EngineHeapAllocateWithRetry(size, DAT_00549154) with the
     * retry global unset in the port — single attempt, flag clear. */
    return engine_heap_alloc_retry(size, 0);
}

// FUNCTION: LEGORR 0x0048db80
void engine_heap_free(void* ptr)
{
    if (ptr != NULL) {
#if defined(LEGORRR_BACKEND_WINAPI)
        free(((void**)ptr)[-1]);
#else
        free(ptr);
#endif
    }
}

// FUNCTION: LEGORR 0x0047ff40
void engine_heap_free_wrapper(void* ptr)
{
    engine_heap_free(ptr);
}

// FUNCTION: LEGORR 0x0045daf0
void ui_render_record_free(void* record)
{
    engine_heap_free(record);
}

// FUNCTION: LEGORR 0x0047ff30
void engine_heap_allocate(long size)
{
    (void)engine_heap_alloc(size);
}

/* --- the retry-handler pair (see heap.hpp) ----------------------------------
 * The retry flag on EngineHeapAllocateWithRetry says "retry through that
 * handler"; the handler itself arrives through the corpus handler word
 * (DAT_00549158, registered through the CRT's set_new_handler machinery —
 * unported), which lands as zero-init storage with the port-side setter.
 * Nothing registers while that seam is absent, so the invoke answers 0 and
 * EngineCalloc behaves like the plain entry — the corpus's own
 * unset-handler reading. */
// GLOBAL: LEGORR 0x00549158
static int (*s_new_handler)(long);

// FUNCTION: LEGORR 0x00491d90
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

// FUNCTION: LEGORR 0x0048ead0
void* engine_calloc(long size)
{
    return engine_heap_alloc_retry(size, 1); /* corpus pushes the retry flag */
}

/* OptionalResourceDestroy (FUN_0040f7b0) — see heap.hpp. */
// FUNCTION: LEGORR 0x0040f7b0
void optional_resource_destroy(void* record)
{
    if (record != NULL) {
        engine_heap_free(record);
    }
}

/* TextBufferDestroy (FUN_00485190) — see heap.hpp. */
// FUNCTION: LEGORR 0x00485190
void text_buffer_destroy(void* record)
{
    void* buffer = NULL;
    __builtin_memcpy(&buffer, (char*)record + 0x14, sizeof(buffer));
    engine_heap_free(buffer);
    engine_heap_free(record);
}

/* TextOutputField82CBit2SetClear (FUN_00485160) — see heap.hpp. */
// FUNCTION: LEGORR 0x00485160
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
// FUNCTION: LEGORR 0x00489d70
void object_field_38_release(void* record)
{
    if (record == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    void* field = *(void**)((char*)record + 0x38);
    if (field != NULL) {
        engine_heap_free(field);
    }
}

/* ThreeWordRecordAllocateAndInitialize (FUN_00410b60) — see heap.hpp. */
// FUNCTION: LEGORR 0x00410b60
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
// FUNCTION: LEGORR 0x00414070
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
 * record and the null buffer slot (deviation: the corpus strides both
 * blindly). The memset calls render the corpus's rep stos pairs. */
// FUNCTION: LEGORR 0x004859b0
void text_buffer_reset(void* record)
{
    if (record == NULL) {
        return; /* deviation: the corpus strides the record blindly */
    }

    uint32_t count = *(uint32_t const*)((char*)record + 0x418);
    char* buffer = *(char* const*)((char*)record + 0x14);
    if (buffer != NULL) {
        /* Pinned deviation (test_heap resets with a null slot): the corpus
         * sweeps count>>2 dwords then count&3 bytes blind through the slot;
         * the observable effect is count zero bytes. */
        memset(buffer, 0, count);
    }
    /* Deviation: the corpus's buffer slot is a dword ending before the
     * window, so the window zeroing never touches it; the port's widened
     * slot straddles the window start, so the slot is restored from the
     * pre-reset buffer value (a null slot stays zeroed either way). */
    memset((char*)record + 0x18, 0, 0x100 * 4);
    *(void**)((char*)record + 0x14) = buffer;

    *(uint32_t*)((char*)record + 0x820) = 0u;
    *(uint32_t*)((char*)record + 0x41c) = 0u;
    uint32_t word = *(uint32_t*)((char*)record + 0x82c);
    word &= ~1u;
    *(uint32_t*)((char*)record + 0x82c) = word;
}

/* UITextOutputObjectRelease (FUN_0045a620) — see heap.hpp. */
// FUNCTION: LEGORR 0x0045a620
void ui_text_output_object_release(void* const* holder)
{
    if (holder == NULL) {
        return; /* pinned deviation: the corpus dereferences blindly */
    }
    text_buffer_reset(*holder);
}

/* TextBufferCreate (FUN_004850f0) — see heap.hpp. */
// FUNCTION: LEGORR 0x004850f0
void* text_buffer_create(uint32_t mode, const uint32_t* header, long size)
{
    char* record = (char*)engine_heap_alloc(0x830);
    if (record == NULL) {
        return NULL; /* corpus: je — the allocation guard exists in the original */
    }
    memset(record, 0, 0x830); /* the corpus's 0x20c-dword sweep */
    __builtin_memcpy(record + 0x00, &mode, sizeof mode);
    if (header != NULL) {
        for (int i = 0; i != 4; ++i) {
            __builtin_memcpy(record + 0x04 + i * 4, &header[i], 4);
        }
    }
    /* Deviation: the header pointer is guarded (the corpus strides it).
     * The lea-based destination base and early header load of the corpus
     * are unreachable with the guard (the mode store precedes the check);
     * the unroll/polarity variants scored lower — tested (b34: plain
     * typed stores also scored lower, reverted). */
    void* buffer = engine_heap_alloc(size);
    __builtin_memcpy(record + 0x14, &buffer, sizeof buffer);
    const uint32_t len = (uint32_t)size;
    __builtin_memcpy(record + 0x418, &len, sizeof len);
    text_output_field82c_bit2_set_clear(record, 1);
    return record;
}

/* TextBufferAppendVFormat (FUN_00485250): append formatted bytes to the
 * text-output circular buffer, optionally starting after `skip_bells` BEL
 * markers. The record layout follows TextBufferCreate/Reset. */
// FUNCTION: LEGORR 0x00485250
void text_buffer_append_vformat(void* record, int skip_bells,
                                const char* format, va_list args)
{
    if (record == NULL || format == NULL) return;

    char formatted[1024];
    const uint32_t length = formatted_text_buffer_build(
        formatted, sizeof(formatted), format, args);

    uint32_t capacity = 0;
    uint32_t cursor = 0;
    uint32_t flags = 0;
    void* buffer = NULL;
    memcpy(&buffer, (char*)record + 0x14, sizeof(buffer));
    memcpy(&capacity, (char*)record + 0x418, sizeof(capacity));
    memcpy(&cursor, (char*)record + 0x41c, sizeof(cursor));
    memcpy(&flags, (char*)record + 0x82c, sizeof(flags));
    if (buffer == NULL || capacity == 0 || cursor >= capacity) return;

    size_t start = 0;
    int bells = 0;
    while (start < length && bells < skip_bells) {
        if (formatted[start] == '\a') ++bells;
        ++start;
    }
    for (uint32_t i = (uint32_t)start; i < length; ++i) {
        ((char*)buffer)[cursor++] = formatted[i];
        if (cursor == capacity) {
            cursor = 0;
            flags |= 1u;
        }
    }
    memcpy((char*)record + 0x41c, &cursor, sizeof(cursor));
    memcpy((char*)record + 0x82c, &flags, sizeof(flags));
}

/* TextBufferAppendFormat (FUN_00485230): variadic wrapper with bell-skip
 * count fixed at zero, matching the decompiled call to TextBufferAppendVFormat. */
// FUNCTION: LEGORR 0x00485230
void text_buffer_append_format(void* record, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    text_buffer_append_vformat(record, 0, format, args);
    va_end(args);
}

/* FormattedTextAppendToRingBuffer (FUN_00485210): variadic face of the
 * text-buffer append path, forwarding its argument list to the VFormat body. */
// FUNCTION: LEGORR 0x00485210
void formatted_text_append_to_ring_buffer(void* record, int skip_bells,
                                          const char* format, ...)
{
    va_list args;
    va_start(args, format);
    text_buffer_append_vformat(record, skip_bells, format, args);
    va_end(args);
}
