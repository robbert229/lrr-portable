// SPDX-License-Identifier: LGPL-3.0-only
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#if defined(_MSC_VER)
#include <new.h>
extern "C" int __cdecl _callnewh(size_t);
extern "C" int __cdecl _heap_init(void);
extern "C" void* __cdecl _heap_alloc(size_t);
extern "C" void* __cdecl _nh_malloc(size_t, int);
#endif

#if defined(LEGORRR_BACKEND_WINAPI)
#include <windows.h>
#elif !defined(_MSC_VER)
#include <sys/mman.h>
#include <unistd.h>
#endif

#include "../gods98/heap.hpp"
#include "crt.hpp"

/* CRT small-block heap and engine allocation entry points — deviations are
 * documented in gods98/heap.hpp. */

#if !defined(_MSC_VER)
/* Portable small-block heap implementation. MSVC uses the matching helpers
 * from VC5 LIBC.LIB's sbheap.obj through _heap_init/_heap_alloc. */
extern "C" {
void* engine_pool_arena_create(void);
void* engine_pool_chunk_try_allocate(void* page_pointer, uint32_t free_units,
                                     uint32_t size_units);
void small_heap_page_free_slot_update(void* arena_pointer, void* page_base,
                                      unsigned char* allocation_tag);
void* engine_pool_allocate(uint32_t size_units);
}

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

/* LIBC.LIB data ___small_block_heap at 0x004affb8 (src/crt/libc_symbols.csv). */
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
/* LIBC.LIB static data at 0x004b1fd8: not annotated (finding 540). */
static CrtHeapArena* s_preferred_arena = &s_pool_root;
/* LIBC.LIB data ___sbh_threshold at 0x004b1fdc (src/crt/libc_symbols.csv). */
static uint32_t s_pool_small_threshold = 480u;
/* LIBC.LIB static data at 0x00549150: not annotated (finding 540). */
static int32_t s_free_small_page_count;
#if defined(LEGORRR_BACKEND_WINAPI)
/* LIBC.LIB data __crtheap at 0x0076dec4 (src/crt/libc_symbols.csv). */
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

/* sbheap.obj supplies this region initializer on the MSVC build. */
// LIBRARY: LEGORR 0x004915a0 SYMBOL
// ___sbh_new_region
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
#endif

// LIBRARY: LEGORR 0x00491560 SYMBOL
// __heap_init
#if !defined(_MSC_VER) || !defined(_M_IX86)
int crt_heap_initialize(void)
{
#if defined(_MSC_VER)
    return _heap_init();
#else
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
    s_heap_initialized = 1;
    return 1;
#endif
}
#endif

/* VC5 LIBC.LIB sbheap.obj's region-release routine. */
// LIBRARY: LEGORR 0x00491710 SYMBOL
// ___sbh_release_region
#if !defined(_MSC_VER)
void crt_heap_block_free(void* block)
{
    CrtHeapArena* arena = (CrtHeapArena*)block;
#if defined(LEGORRR_BACKEND_WINAPI)
    VirtualFree((LPVOID)(uintptr_t)arena->reserve_base, 0, MEM_RELEASE);
#else
    crt_pool_release(CRT_POOL_RESERVE_BASE(arena));
#endif
    if (s_preferred_arena == arena) s_preferred_arena = arena->previous;
    if (arena != &s_pool_root) {
        arena->previous->next = arena->next;
        arena->next->previous = arena->previous;
#if defined(LEGORRR_BACKEND_WINAPI)
        HeapFree(s_process_heap, 0, arena);
#else
        crt_pool_descriptor_free(arena);
#endif
    }
    else {
#if defined(LEGORRR_BACKEND_WINAPI)
        s_pool_root.reserve_base = 0xffffffffu;
#else
        s_pool_root.reserve_base = (unsigned char*)-1;
#endif
    }
}

/* VC5 LIBC.LIB sbheap.obj's free-page decommitter. */
// LIBRARY: LEGORR 0x00491770 SYMBOL
// ___sbh_decommit_pages
void crt_heap_release_unused_pages(int page_count)
{
    CrtHeapArena* arena = s_pool_root.previous;
    CrtHeapArena* next;
    do {
        int released = 0;
        int page_index;
        if (CRT_POOL_HAS_RESERVATION(arena)) {
            for (page_index = 1023; page_index >= 0; --page_index) {
                CrtHeapPageEntry* entry = &arena->pages[page_index];
                if (entry->free_units == 0xf0 &&
                    crt_pool_decommit(CRT_POOL_RESERVE_BASE(arena) + (uint32_t)page_index * 0x1000u,
                                      0x1000u)) {
                    entry->free_units = -1;
                    --s_free_small_page_count;
                    if (arena->first_uncommitted == NULL ||
                        arena->first_uncommitted > entry) arena->first_uncommitted = entry;
                    ++released;
                    --page_count;
                    if (page_count == 0) break;
                }
            }
        }
        next = arena->previous;
        if (released != 0 && arena->pages[0].free_units == -1) {
            for (page_index = 1; page_index < 1024; ++page_index) {
                if (arena->pages[page_index].free_units != -1) break;
            }
            if (page_index == 1024) crt_heap_block_free(arena);
        }
        arena = next;
    } while (arena != &s_pool_root && page_count > 0);
}

/* VC5 LIBC.LIB sbheap.obj's allocation-pointer lookup. */
// LIBRARY: LEGORR 0x00491840 SYMBOL
// ___sbh_find_block
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

/* VC5 LIBC.LIB sbheap.obj's per-page allocation routine. */
// LIBRARY: LEGORR 0x00491b40 SYMBOL
// ___sbh_alloc_block_from_page
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

/* VC5 LIBC.LIB sbheap.obj's small-block release routine. */
// LIBRARY: LEGORR 0x004918a0 SYMBOL
// ___sbh_free_block
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

/* VC5 LIBC.LIB sbheap.obj's small-block allocator. */
// LIBRARY: LEGORR 0x00491900 SYMBOL
// ___sbh_alloc_block
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
#endif

// LIBRARY: LEGORR 0x0048dc40 SYMBOL
// __heap_alloc
#if !defined(_MSC_VER) || !defined(_M_IX86)
void* engine_heap_alloc_aligned(long size)
{
#if defined(_MSC_VER)
    return _heap_alloc((size_t)size);
#else
    /* The corpus body: 16-byte round-up, then the small pool below its
     * 480-byte threshold, followed by the process heap. */
    unsigned rounded = ((unsigned)size + 15u) & 0xfffffff0u;
    unsigned size_units = rounded >> 4;
#if !defined(LEGORRR_BACKEND_WINAPI)
    if (rounded == 0u) {
        rounded = 16u; /* host aligned_alloc requires a nonzero allocation */
    }
#endif
    if (rounded <= s_pool_small_threshold) {
        void* block = engine_pool_allocate(size_units);
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
#endif
}
#endif

// LIBRARY: LEGORR 0x0048dbf0 SYMBOL
// __nh_malloc
#if !defined(_MSC_VER) || !defined(_M_IX86)
void* engine_heap_alloc_retry(long size, int retry_via_new_handler)
{
#if defined(_MSC_VER)
    return _nh_malloc((size_t)size, retry_via_new_handler);
#else
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
#endif
}
#endif

/* LIBC.LIB data __newmode at 0x00549154 (src/crt/libc_symbols.csv). */
static int s_heap_alloc_retry_enabled;

#if !defined(_MSC_VER) || !defined(_M_IX86)
// LIBRARY: LEGORR 0x0048dbd0 SYMBOL
// _malloc
void* engine_heap_alloc(long size)
{
#if defined(_MSC_VER)
    return malloc((size_t)size);
#else
    /* Corpus: EngineHeapAllocateWithRetry(size, DAT_00549154). The original
     * retry global is zero-initialized in the image. */
    return engine_heap_alloc_retry(size, s_heap_alloc_retry_enabled);
#endif
}
#endif

// LIBRARY: LEGORR 0x0048db80 SYMBOL
// _free
#if !defined(_MSC_VER) || !defined(_M_IX86)
void engine_heap_free(void* ptr)
{
#if defined(_MSC_VER)
    free(ptr);
#else
    if (ptr != NULL) {
        void* arena;
        void* page_base;
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
#endif
}
#endif

/* --- the retry-handler pair (see heap.hpp) ----------------------------------
 * VC5 LIBC.LIB owns the reference build's allocation-failure handler. */
/* LIBC.LIB data ?_pnhHeap@@3P6AHI@ZA at 0x00549158 (src/crt/libc_symbols.csv). */
#if !defined(_MSC_VER)
static int (*s_new_handler)(long);
#endif

// LIBRARY: LEGORR 0x00491d90 SYMBOL
// __callnewh
#if !defined(_MSC_VER) || !defined(_M_IX86)
int crt_new_handler_invoke(long size)
{
#if defined(_MSC_VER)
    return _callnewh((size_t)size);
#else
    if (s_new_handler != NULL) {
        if (s_new_handler(size) != 0) {
            return 1; /* the handler asked for another attempt */
        }
    }
    return 0;
#endif
}
#endif

#if !defined(_MSC_VER) || !defined(_M_IX86)
void crt_new_handler_set(int (*handler)(long))
{
#if !defined(_MSC_VER)
    s_new_handler = handler;
#else
    (void)_set_new_handler((_PNH)handler);
#endif
}
#endif

/* LIBC.LIB's operator new (new.obj): the matching build takes it from the
 * library, and gods98/avi_player.cpp's new-expressions call it (finding 608).
 * engine_calloc is the native port's copy. */
// LIBRARY: LEGORR 0x0048ead0 SYMBOL
// ??2@YAPAXI@Z
#if !defined(_MSC_VER)
// PORT-ONLY: the native port's operator new; the matching build uses LIBC's
void* engine_calloc(long size)
{
    return engine_heap_alloc_retry(size, 1); /* corpus pushes the retry flag */
}
#endif

/* VC5 LIBC.LIB's calloc entry uses the runtime allocator and zero-fill path. */
// LIBRARY: LEGORR 0x004997c0 SYMBOL
// _calloc
#if !defined(_MSC_VER) || !defined(_M_IX86)
void* crt_calloc_aligned(int count, int size)
{
#if defined(_MSC_VER)
    return calloc((size_t)count, (size_t)size);
#else
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
#endif
}
#endif
