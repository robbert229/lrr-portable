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
 *  - the pool allocator now reserves 4 MiB arenas and commits pages on demand;
 *    non-Win32 builds use native virtual-memory calls as a compatibility path;
 *  - the new-handler registry remains a port hook and is unset by default;
 *  - allocations are 16-byte aligned because the corpus round-ups do (the
 *    aligned path is shared by both entry points). */
#include <stdint.h>
#include <stdarg.h>
#if defined(_MSC_VER) && defined(_M_IX86)
#include <stdlib.h>
extern "C" void* __cdecl _heap_alloc(size_t size);
extern "C" void* __cdecl _nh_malloc(size_t size, int retry_via_new_handler);
#endif

#ifndef LEGORRR_GODS98_HEAP_HPP
#define LEGORRR_GODS98_HEAP_HPP

#ifdef __cplusplus
extern "C" {
#endif

/* FUN_0048dbf0: allocate `size` bytes; size 0 allocates 1 (corpus quirk);
 * sizes at/above 0xffffffe1 fail with NULL (corpus overflow gate). The
 * retry flag is the corpus's second argument: zero makes the single
 * attempt the corpus makes with the flag off — on failure, NULL, no spin;
 * nonzero sends each failure through crt_new_handler_invoke and retries
 * while that answers nonzero. */
#if defined(_MSC_VER) && defined(_M_IX86)
#define engine_heap_alloc_retry(size, retry) \
    _nh_malloc((size_t)(size), (int)(retry))
#else
void* engine_heap_alloc_retry(long size, int retry_via_new_handler);
#endif

/* FUN_0048dbd0: the plain VC5 `_malloc` entry — same path with the corpus's
 * retry-flag global, which the port leaves unregistered (single attempt).
 * Route reference-build call sites to the runtime symbol directly. */
#if defined(_MSC_VER) && defined(_M_IX86)
#define engine_heap_alloc(size) malloc((size_t)(size))
#else
void* engine_heap_alloc(long size);
#endif

/* FUN_0048dc40: the aligned allocator body. Small allocations use the CRT
 * pool below the corpus threshold; larger allocations use the process heap. */
#if defined(_MSC_VER) && defined(_M_IX86)
#define engine_heap_alloc_aligned(size) _heap_alloc((size_t)(size))
#else
void* engine_heap_alloc_aligned(long size);
#endif

/* FUN_0048db80: free a previous allocation (NULL is a no-op). Route
 * reference-build call sites to the VC5 runtime symbol directly. */
#if defined(_MSC_VER) && defined(_M_IX86)
#define engine_heap_free(ptr) free(ptr)
#else
void engine_heap_free(void* ptr);
#endif

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
void* engine_calloc(long size);                  /* FUN_0048ead0 */

/* OptionalResourceDestroy (FUN_0040f7b0): frees the optional resource
 * record only when one is there — the corpus's gate over the engine free. */
void optional_resource_destroy(void* record);

/* TextBufferDestroy (FUN_00485190): frees the buffer held in the text
 * buffer record's +0x14 slot, then the record itself. The corpus's slot
 * is a 32-bit dword; the port carries a host-width pointer there
 * (documented deviation). The corpus dereferences the record blindly and
 * so does the port — callers own non-null. */
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
void object_field_38_release_target(void* record); /* unchecked target body */

/* TextBufferReset (FUN_004859b0): clears the text-output record in place.
 * The record's +0x418 count of buffer bytes and +0x14 buffer pointer
 * (widened like the destroy slot) zero `count` buffer bytes, the
 * record's +0x18 dword window of 0x100 dwords zeroes, and the words
 * +0x820 / +0x41c clear while +0x82c loses bit 0. The corpus strides
 * the record and buffer blindly; the port guards the record and skips
 * the sweep on a NULL buffer slot. Deviation: the corpus slot is a
 * dword ending before the window, so the port's widened slot straddles
 * the window start — the reset rewrites the slot after zeroing,
 * keeping the record usable (the corpus-visible padding bytes stay
 * zeroed for any pointer below 4 GiB). */
void text_buffer_reset(void* record);           /* FUN_004859b0 */
void text_buffer_reset_target(void* record);    /* unchecked retail body */

/* TextBufferCreate (FUN_004850f0): allocates the 0x830-byte text-output
 * record, zero-clears it, installs the caller's font-resource handle and four header
 * dwords, allocates the buffer half (the port keeps the widened native
 * pointer slot at +0x14 like the reset/destroy twins), records the length
 * as a dword at +0x418, and raises the +0x82c bit 2. Deviations: the
 * buffer-slot widening, and the record half answering NULL where the
 * corpus strides the allocation blindly. */
void* text_buffer_create(uintptr_t font_handle, const uint32_t* header,
                         long size);
void text_buffer_append_vformat(void* record, int skip_bells,
                                const char* format, va_list args);
void text_buffer_append_format(void* record, const char* format, ...);
void formatted_text_append_to_ring_buffer(void* record, int skip_bells,
                                          const char* format, ...);

/* TextBufferDrawWrapped uses the font-resource handle stored at record +0.
 * Portable gameplay installs atlas-backed metrics and glyph blits through
 * this interface; `glyph_draw` returns the same advance as the native
 * draw-and-measure call. */
typedef struct TextBufferDrawOps {
    void* context;
    int (*font_line_height)(void* context, void* font);
    int (*glyph_advance)(void* context, void* font, unsigned char glyph);
    int (*glyph_draw)(void* context, void* font, int x, int y,
                      unsigned char glyph, int draw_mode);
    int (*is_word_break)(void* context, unsigned char glyph);
} TextBufferDrawOps;
void text_buffer_draw_ops_set(const TextBufferDrawOps* ops);
/* Install the software bitmap-atlas renderer used by portable gameplay. */
void text_buffer_draw_software_install(void);
int text_buffer_draw_wrapped(void* record, uint32_t start_line,
                             float line_scale, int* height_out);
int text_wrap_measure_and_draw_lines(void* record, float elapsed,
                                     int* height_out);

/* UITextOutputObjectRelease (FUN_0045a620): target resets the text-output
 * record held in the caller's pointer slot with blind accesses. The public
 * adapter guards both the holder and its record pointer. */
void ui_text_output_object_release(void* const* holder);
void ui_text_output_object_release_target(void* const* holder);
                                                /* FUN_0045a620 */

/* --- dword-list allocator (2026-10-25) -----------------------------------
 * ListAlloc (FUN_00414070): builds a caller-owned two-slot header
 * {data, capacity} over a fresh zero-filled array of `count` dwords. The
 * corpus zero-pass covers `count & 0x3fffffff` slots and its second pass
 * is dead code (loop seed 0); the port keeps both faithfully — slots the
 * mask skips stay uninitialized, exactly as in the corpus's partial sweep.
 * Deviations: the header slots and array entries are widened (the corpus
 * strides dwords); a NULL allocation from either half releases the other
 * half and answers NULL (the corpus strides blindly). */
void** dword_list_allocate_and_zero(uint32_t count); /* FUN_00414070 */

#endif /* LEGORRR_GODS98_HEAP_HPP */
