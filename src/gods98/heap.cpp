// SPDX-License-Identifier: LGPL-3.0-only
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#if defined(LEGORRR_BACKEND_WINAPI)
#include <windows.h>
#elif !defined(_MSC_VER)
#include <sys/mman.h>
#include <unistd.h>
#endif

#include "heap.hpp"
#include "string.hpp"

/* Engine heap seam implementation — deviations in heap.hpp. */

typedef struct CrtHeapPageEntry {
    int32_t free_units;
    uint32_t search_limit;
} CrtHeapPageEntry;

#if defined(LEGORRR_BACKEND_WINAPI)
typedef uint32_t CrtHeapAddress;
#else
typedef unsigned char* CrtHeapAddress;
#endif

typedef struct CrtHeapArena {
    struct CrtHeapArena* next;
    struct CrtHeapArena* previous;
    CrtHeapPageEntry* allocation_cursor;
    CrtHeapPageEntry* first_uncommitted;
    CrtHeapAddress reserve_base;
    CrtHeapAddress reserve_end;
    CrtHeapPageEntry pages[1024];
    uint32_t tail_words[2];
} CrtHeapArena;

#if defined(LEGORRR_BACKEND_WINAPI)
#define CRT_POOL_RESERVE_BASE(arena) ((unsigned char*)(uintptr_t)(arena)->reserve_base)
#define CRT_POOL_RESERVE_END(arena) ((unsigned char*)(uintptr_t)(arena)->reserve_end)
#define CRT_POOL_SET_RESERVE(arena, base, end) \
    ((arena)->reserve_base = (uint32_t)(uintptr_t)(base), \
     (arena)->reserve_end = (uint32_t)(uintptr_t)(end))
#define CRT_POOL_HAS_RESERVATION(arena) ((arena)->reserve_base != 0xffffffffu)
#else
#define CRT_POOL_RESERVE_BASE(arena) ((arena)->reserve_base)
#define CRT_POOL_RESERVE_END(arena) ((arena)->reserve_end)
#define CRT_POOL_SET_RESERVE(arena, base, end) \
    ((arena)->reserve_base = (base), (arena)->reserve_end = (end))
#define CRT_POOL_HAS_RESERVATION(arena) \
    (CRT_POOL_RESERVE_BASE(arena) != (unsigned char*)-1)
#endif

// GLOBAL: LEGORR 0x004affb8
static CrtHeapArena s_pool_root = {
    &s_pool_root,
    &s_pool_root,
    &s_pool_root.pages[0],
    &s_pool_root.pages[0],
#if defined(LEGORRR_BACKEND_WINAPI)
    0xffffffffu,
    0xffffffffu,
#else
    (unsigned char*)-1,
    (unsigned char*)-1,
#endif
    { { 0xf0, 0xf1u } },
    { 0u, 0u }
};
// GLOBAL: LEGORR 0x004b1fd8
static CrtHeapArena* s_preferred_arena = &s_pool_root;
// GLOBAL: LEGORR 0x004b1fdc
static uint32_t s_pool_small_threshold = 480u;
// GLOBAL: LEGORR 0x00549150
static int32_t s_free_small_page_count;
#if defined(LEGORRR_BACKEND_WINAPI)
// GLOBAL: LEGORR 0x0076dec4
static HANDLE s_process_heap;
#else
static void* s_process_heap;
#endif
static int s_heap_initialized;

static void* crt_pool_descriptor_allocate(void)
{
#if defined(LEGORRR_BACKEND_WINAPI)
    return HeapAlloc(s_process_heap, 0, 0x2020u);
#else
    return calloc(1, sizeof(CrtHeapArena));
#endif
}

static void crt_pool_descriptor_free(void* descriptor)
{
#if defined(LEGORRR_BACKEND_WINAPI)
    HeapFree(s_process_heap, 0, descriptor);
#else
    free(descriptor);
#endif
}

static void* crt_pool_reserve(void)
{
#if defined(LEGORRR_BACKEND_WINAPI)
    return VirtualAlloc(NULL, 0x400000u, MEM_RESERVE, PAGE_READWRITE);
#elif !defined(_MSC_VER)
    void* base = mmap(NULL, 0x400000u, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return (base == MAP_FAILED) ? NULL : base;
#else
    return NULL;
#endif
}

static void* crt_pool_commit(void* address, uint32_t size)
{
#if defined(LEGORRR_BACKEND_WINAPI)
    return VirtualAlloc(address, size, MEM_COMMIT, PAGE_READWRITE);
#elif !defined(_MSC_VER)
    if (mprotect(address, size, PROT_READ | PROT_WRITE) != 0) return NULL;
    memset(address, 0, size);
    return address;
#else
    (void)address;
    (void)size;
    return NULL;
#endif
}

static int crt_pool_decommit(void* address, uint32_t size)
{
#if defined(LEGORRR_BACKEND_WINAPI)
    return VirtualFree(address, size, MEM_DECOMMIT) != 0;
#elif !defined(_MSC_VER)
    if (mprotect(address, size, PROT_NONE) != 0) return 0;
    madvise(address, size, MADV_DONTNEED);
    return 1;
#else
    (void)address;
    (void)size;
    return 0;
#endif
}

static int crt_pool_release(void* address)
{
#if defined(LEGORRR_BACKEND_WINAPI)
    return VirtualFree(address, 0, MEM_RELEASE) != 0;
#elif !defined(_MSC_VER)
    return munmap(address, 0x400000u) == 0;
#else
    (void)address;
    return 0;
#endif
}

static int crt_pool_page_index(CrtHeapArena* arena, CrtHeapPageEntry* page)
{
    return (int)(page - arena->pages);
}

static unsigned char* crt_pool_page_cursor_get(unsigned char* page, uint32_t value)
{
#if defined(LEGORRR_BACKEND_WINAPI)
    (void)page;
    return (unsigned char*)(uintptr_t)value;
#else
    return page + value;
#endif
}

static uint32_t crt_pool_page_cursor_set(unsigned char* page, unsigned char* cursor)
{
#if defined(LEGORRR_BACKEND_WINAPI)
    (void)page;
    return (uint32_t)(uintptr_t)cursor;
#else
    return (uint32_t)(cursor - page);
#endif
}

// FUNCTION: LEGORR 0x004915a0
void* engine_pool_arena_create(void)
{
    CrtHeapArena* arena;
    unsigned char* base;
    unsigned char* page;
    int i;
    int is_static;

    if (s_pool_root.next == NULL) s_pool_root.next = &s_pool_root;
    if (s_pool_root.previous == NULL) s_pool_root.previous = &s_pool_root;
    is_static = !CRT_POOL_HAS_RESERVATION(&s_pool_root);
    if (is_static) arena = &s_pool_root;
    else {
        arena = (CrtHeapArena*)crt_pool_descriptor_allocate();
        if (arena == NULL) return NULL;
    }
    base = (unsigned char*)crt_pool_reserve();
    if (base == NULL) {
        if (!is_static) crt_pool_descriptor_free(arena);
        return NULL;
    }
    if (crt_pool_commit(base, 0x10000u) == NULL) {
        crt_pool_release(base);
        if (!is_static) crt_pool_descriptor_free(arena);
        return NULL;
    }

    if (is_static) {
        arena->next = &s_pool_root;
        arena->previous = &s_pool_root;
        arena->allocation_cursor = &arena->pages[0];
        arena->first_uncommitted = &arena->pages[16];
    }
    else {
        arena->next = &s_pool_root;
        arena->previous = s_pool_root.previous;
        s_pool_root.previous->next = arena;
        s_pool_root.previous = arena;
        arena->allocation_cursor = &arena->pages[0];
        arena->first_uncommitted = &arena->pages[16];
    }
    CRT_POOL_SET_RESERVE(arena, base, base + 0x400000u);
    for (i = 0; i < 1024; ++i) {
        arena->pages[i].free_units = (i < 16) ? 0xf0 : -1;
        arena->pages[i].search_limit = 0xf1u;
    }
    memset(base, 0, 0x10000u);
    for (i = 0; i < 16; ++i) {
        uint32_t* header = (uint32_t*)(base + (uint32_t)i * 0x1000u);
        header[0] = crt_pool_page_cursor_set(base + (uint32_t)i * 0x1000u,
                                             base + (uint32_t)i * 0x1000u + 8u);
        header[1] = 0xf0u;
        base[(uint32_t)i * 0x1000u + 0xf8u] = 0xffu;
    }
    return arena;
}

// FUNCTION: LEGORR 0x00491560
int crt_heap_initialize(void)
{
#if defined(LEGORRR_BACKEND_WINAPI)
    s_process_heap = HeapCreate(1u, 0x1000u, 0u);
#else
    s_process_heap = (void*)1;
#endif
    if (s_process_heap == NULL) return 0;
    if (engine_pool_arena_create() == NULL) {
#if defined(LEGORRR_BACKEND_WINAPI)
        HeapDestroy(s_process_heap);
#endif
        return 0;
    }
    if (s_preferred_arena == NULL) s_preferred_arena = &s_pool_root;
    s_heap_initialized = 1;
    return 1;
}

// FUNCTION: LEGORR 0x00491710
void crt_heap_block_free(void* block)
{
    CrtHeapArena* arena = (CrtHeapArena*)block;
    if (arena == NULL) return;
#if defined(LEGORRR_BACKEND_WINAPI)
    VirtualFree((LPVOID)(uintptr_t)arena->reserve_base, 0, MEM_RELEASE);
#else
    crt_pool_release(CRT_POOL_RESERVE_BASE(arena));
#endif
    if (s_preferred_arena == arena) s_preferred_arena = arena->previous;
    if (arena != &s_pool_root) {
        arena->previous->next = arena->next;
        arena->next->previous = arena->previous;
        crt_pool_descriptor_free(arena);
    }
    else {
        CRT_POOL_SET_RESERVE(arena, (unsigned char*)-1, (unsigned char*)-1);
    }
}

// FUNCTION: LEGORR 0x00491770
void crt_heap_release_unused_pages(int page_count)
{
    CrtHeapArena* arena = s_pool_root.previous;
    CrtHeapArena* next;
    do {
        int released = 0;
        int page_index;
        next = (arena == &s_pool_root) ? s_pool_root.previous : arena->previous;
        if (CRT_POOL_HAS_RESERVATION(arena)) {
            for (page_index = 1023; page_index >= 0; --page_index) {
                CrtHeapPageEntry* entry = &arena->pages[page_index];
                if (entry->free_units == 0xf0 &&
                    crt_pool_decommit(CRT_POOL_RESERVE_BASE(arena) + (uint32_t)page_index * 0x1000u,
                                      0x1000u)) {
                    entry->free_units = -1;
                    --s_free_small_page_count;
                    if (arena->first_uncommitted == NULL ||
                        entry < arena->first_uncommitted) arena->first_uncommitted = entry;
                    ++released;
                    --page_count;
                    if (page_count == 0) break;
                }
            }
            if (released != 0 && arena->pages[0].free_units == -1) {
                for (page_index = 1; page_index < 1024; ++page_index) {
                    if (arena->pages[page_index].free_units != -1) break;
                }
                if (page_index == 1024) crt_heap_block_free(arena);
            }
        }
        if (page_count < 1) return;
        if (arena == &s_pool_root) return;
        arena = next;
    } while (1);
}

// FUNCTION: LEGORR 0x00491840
void* crt_heap_locate_allocation_page(void* allocation, void** arena_out,
                                     void** page_base_out)
{
    CrtHeapArena* arena = &s_pool_root;
    unsigned char* address = (unsigned char*)allocation;
    if (arena->next == NULL) return NULL;
    do {
        if (CRT_POOL_HAS_RESERVATION(arena) && address > CRT_POOL_RESERVE_BASE(arena) &&
            address < CRT_POOL_RESERVE_END(arena)) {
            if (((uintptr_t)address & 0xfu) != 0u ||
                ((uintptr_t)address & 0xfffu) < 0x100u) return NULL;
            if (arena_out != NULL) *arena_out = arena;
            if (page_base_out != NULL) {
                *page_base_out = (void*)((uintptr_t)address & ~(uintptr_t)0xfffu);
            }
            return (void*)(((uintptr_t)address & ~(uintptr_t)0xfffu) + 8u +
                           ((((uintptr_t)address & 0xfffu) - 0x100u) >> 4));
        }
        arena = arena->next;
    } while (arena != &s_pool_root);
    return NULL;
}

// FUNCTION: LEGORR 0x00491b40
void* engine_pool_chunk_try_allocate(void* page_pointer, uint32_t free_units,
                                     uint32_t size_units)
{
    unsigned char* page = (unsigned char*)page_pointer;
    uint32_t* header = (uint32_t*)page;
    unsigned char* tags = page + 8;
    unsigned char* cursor = crt_pool_page_cursor_get(page, header[0]);
    unsigned char* end = page + 0xf8;
    unsigned char* scan;
    uint32_t available = free_units;
    uint32_t run;
    if (size_units <= header[1]) {
        *cursor = (unsigned char)size_units;
        if (cursor + size_units < end) {
            header[0] = crt_pool_page_cursor_set(page, cursor + size_units);
            header[1] -= size_units;
        }
        else {
            header[1] = 0;
            header[0] = crt_pool_page_cursor_set(page, tags);
        }
        return page + 0x100u + (uint32_t)(cursor - tags) * 16u;
    }
    scan = cursor;
    if (scan[header[1]] != 0) scan += header[1];
    while (scan + size_units < end) {
        if (*scan == 0) {
            run = 1;
            while (scan[run] == 0) ++run;
            if (size_units <= run) {
                *scan = (unsigned char)size_units;
                if (scan + size_units < end) {
                    header[0] = crt_pool_page_cursor_set(page, scan + size_units);
                    header[1] = run - size_units;
                }
                else {
                    header[0] = crt_pool_page_cursor_set(page, tags);
                    header[1] = 0;
                }
                return page + 0x100u + (uint32_t)(scan - tags) * 16u;
            }
            if (scan == cursor) header[1] = run;
            else {
                available -= run;
                if (available < size_units) return NULL;
            }
            scan += run;
        }
        else scan += *scan;
    }
    scan = tags;
    cursor = crt_pool_page_cursor_get(page, header[0]);
    while (scan < cursor && scan + size_units < end) {
        if (*scan == 0) {
            run = 1;
            while (scan[run] == 0) ++run;
            if (size_units <= run) {
                if (scan + size_units < end) {
                    header[0] = crt_pool_page_cursor_set(page, scan + size_units);
                    header[1] = run - size_units;
                }
                else {
                    header[0] = crt_pool_page_cursor_set(page, tags);
                    header[1] = 0;
                }
                *scan = (unsigned char)size_units;
                return page + 0x100u + (uint32_t)(scan - tags) * 16u;
            }
            available -= run;
            if (available < size_units) return NULL;
            scan += run;
        }
        else scan += *scan;
    }
    return NULL;
}

// FUNCTION: LEGORR 0x004918a0
void small_heap_page_free_slot_update(void* arena_pointer, void* page_base,
                                      unsigned char* allocation_tag)
{
    CrtHeapArena* arena = (CrtHeapArena*)arena_pointer;
    int index = (int)(((unsigned char*)page_base - CRT_POOL_RESERVE_BASE(arena)) >> 12);
    CrtHeapPageEntry* entry = &arena->pages[index];
    entry->free_units += *allocation_tag;
    *allocation_tag = 0;
    entry->search_limit = 0xf1u;
    if (entry->free_units == 0xf0) {
        ++s_free_small_page_count;
        if (s_free_small_page_count == 0x20) crt_heap_release_unused_pages(0x10);
    }
}

// FUNCTION: LEGORR 0x00491900
void* engine_pool_allocate(uint32_t size_units)
{
    CrtHeapArena* arena;
    CrtHeapArena* start;
    int arena_created = 0;
    if (!s_heap_initialized && !crt_heap_initialize()) return NULL;
    start = s_preferred_arena;
    if (start == NULL) start = &s_pool_root;
    arena = start;
    do {
        int cursor_index;
        int pass;
        if (CRT_POOL_HAS_RESERVATION(arena)) {
            cursor_index = crt_pool_page_index(arena, arena->allocation_cursor);
            if (cursor_index < 0 || cursor_index >= 1024) cursor_index = 0;
            for (pass = 0; pass < 2; ++pass) {
                int first = (pass == 0) ? cursor_index : 0;
                int last = (pass == 0) ? 1024 : cursor_index;
                int i;
                for (i = first; i < last; ++i) {
                    CrtHeapPageEntry* entry = &arena->pages[i];
                    unsigned char* page;
                    void* result;
                    if ((int32_t)size_units > entry->free_units ||
                        size_units >= entry->search_limit) continue;
                    page = CRT_POOL_RESERVE_BASE(arena) + (uint32_t)i * 0x1000u;
                    result = engine_pool_chunk_try_allocate(page, (uint32_t)entry->free_units,
                                                            size_units);
                    if (result != NULL) {
                        entry->free_units -= (int32_t)size_units;
                        arena->allocation_cursor = entry;
                        s_preferred_arena = arena;
                        return result;
                    }
                    entry->search_limit = size_units;
                }
            }
        }
        arena = arena->next;
    } while (arena != start);

    arena = &s_pool_root;
    do {
        if (CRT_POOL_HAS_RESERVATION(arena) && arena->first_uncommitted != NULL) break;
        arena = arena->next;
    } while (arena != &s_pool_root);
    if (arena == &s_pool_root &&
        (!CRT_POOL_HAS_RESERVATION(&s_pool_root) || s_pool_root.first_uncommitted == NULL)) {
        arena = (CrtHeapArena*)engine_pool_arena_create();
        if (arena == NULL) return NULL;
        arena_created = 1;
    }
    if (arena_created) {
        unsigned char* page = CRT_POOL_RESERVE_BASE(arena);
        uint32_t* header = (uint32_t*)page;
        unsigned char* tags = page + 8u;
        tags[0] = (unsigned char)size_units;
        header[0] = crt_pool_page_cursor_set(page, tags + size_units);
        header[1] = 0xf0u - size_units;
        arena->pages[0].free_units -= (int32_t)(size_units & 0xffu);
        arena->allocation_cursor = &arena->pages[0];
        s_preferred_arena = arena;
        return page + 0x100u;
    }
    if (arena->first_uncommitted != NULL) {
        CrtHeapPageEntry* first = arena->first_uncommitted;
        int first_index = crt_pool_page_index(arena, first);
        int page_count = 0;
        int i;
        unsigned char* commit_address = CRT_POOL_RESERVE_BASE(arena) + (uint32_t)first_index * 0x1000u;
        while (first_index + page_count < 1024 && page_count < 16 &&
               arena->pages[first_index + page_count].free_units == -1) ++page_count;
        if (page_count == 0 || crt_pool_commit(commit_address,
                                               (uint32_t)page_count * 0x1000u) != commit_address) {
            return NULL;
        }
        memset(commit_address, 0, (size_t)page_count * 0x1000u);
        for (i = 0; i < page_count; ++i) {
            int page_index = first_index + i;
            unsigned char* page = commit_address + (uint32_t)i * 0x1000u;
            uint32_t* header = (uint32_t*)page;
            arena->pages[page_index].free_units = 0xf0;
            arena->pages[page_index].search_limit = 0xf1u;
            header[0] = crt_pool_page_cursor_set(page, page + 8u);
            header[1] = 0xf0u;
            page[0xf8] = 0xffu;
        }
        first_index += page_count;
        arena->first_uncommitted = (first_index < 1024) ? &arena->pages[first_index] : NULL;
        arena->allocation_cursor = first;
        s_preferred_arena = arena;
        {
            void* result = engine_pool_chunk_try_allocate(commit_address, 0xf0u, size_units);
            if (result != NULL) arena->pages[crt_pool_page_index(arena, first)].free_units -= (int32_t)size_units;
            return result;
        }
    }
    return NULL;
}

// FUNCTION: LEGORR 0x0048dc40
void* engine_heap_alloc_aligned(long size)
{
    /* The corpus body: 16-byte round-up, then the small pool below its
     * 480-byte threshold, followed by the process heap. */
    unsigned rounded = ((unsigned)size + 15u) & 0xfffffff0u;
#if !defined(LEGORRR_BACKEND_WINAPI)
    if (rounded == 0u) {
        rounded = 16u; /* host aligned_alloc requires a nonzero allocation */
    }
#endif
#if defined(LEGORRR_BACKEND_WINAPI)
    if (!s_heap_initialized) crt_heap_initialize();
#else
    if (!s_heap_initialized) crt_heap_initialize();
#endif
    if (s_heap_initialized && rounded <= s_pool_small_threshold) {
        void* block = engine_pool_allocate(((unsigned)size + 15u) >> 4);
        if (block != NULL) return block;
    }
#if defined(LEGORRR_BACKEND_WINAPI)
    if (s_heap_initialized) return HeapAlloc(s_process_heap, 0, rounded);
    {
        void* base = malloc(rounded + 16u + sizeof(void*));
        uintptr_t aligned;
        if (base == NULL) return NULL;
        aligned = ((uintptr_t)base + sizeof(void*) + 15u) & ~(uintptr_t)15u;
        ((void**)aligned)[-1] = base;
        return (void*)aligned;
    }
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

// GLOBAL: LEGORR 0x00549154
static int s_heap_alloc_retry_enabled;

// FUNCTION: LEGORR 0x0048dbd0
void* engine_heap_alloc(long size)
{
    /* Corpus: EngineHeapAllocateWithRetry(size, DAT_00549154). The original
     * retry global is zero-initialized in the image. */
    return engine_heap_alloc_retry(size, s_heap_alloc_retry_enabled);
}

// FUNCTION: LEGORR 0x0048db80
void engine_heap_free(void* ptr)
{
    if (ptr != NULL) {
        void* arena = NULL;
        void* page_base = NULL;
        unsigned char* allocation_tag = (unsigned char*)
            crt_heap_locate_allocation_page(ptr, &arena, &page_base);
        if (allocation_tag != NULL) {
            small_heap_page_free_slot_update(arena, page_base, allocation_tag);
            return;
        }
#if defined(LEGORRR_BACKEND_WINAPI)
        if (s_heap_initialized) HeapFree(s_process_heap, 0, ptr);
        else free(((void**)ptr)[-1]);
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

/* CrtCallocAligned (FUN_004997c0): unsigned product, 16-byte size rounding,
 * and zero-fill. engine_heap_alloc_aligned selects the small-block pool or
 * the process heap using the corpus threshold. */
// FUNCTION: LEGORR 0x004997c0
void* crt_calloc_aligned(int count, int size)
{
    uint32_t bytes = (uint32_t)count * (uint32_t)size;

    if (bytes < 0xffffffe1u) {
        if (bytes == 0u) {
            bytes = 16u;
        }
        else {
            bytes = (bytes + 15u) & 0xfffffff0u;
        }
    }
    for (;;) {
        void* block = NULL;
        if (bytes < 0xffffffe1u) {
            block = engine_heap_alloc_aligned((long)bytes);
        }
        if (block != NULL) {
            memset(block, 0, bytes);
            return block;
        }
        if (s_heap_alloc_retry_enabled == 0 ||
            crt_new_handler_invoke((long)bytes) == 0) {
            return NULL;
        }
    }
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
