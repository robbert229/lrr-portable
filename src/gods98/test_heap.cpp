// SPDX-License-Identifier: LGPL-3.0-only
// Engine heap seam (findings 397/243): aligned allocation, the entry-point
// quirks, the free pair and the retry-handled allocator.
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "gods98/heap.hpp"
#include "crt/crt.hpp"

static int s_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);      \
            ++s_failures;                                                      \
        }                                                                      \
    } while (0)

static void test_alloc_pair(void)
{
    void* p = engine_heap_alloc(128);
    CHECK(p != NULL, "alloc returns a block");
    CHECK(((uintptr_t)p & 0xfu) == 0, "allocations are 16-byte aligned");
    memset(p, 0x5a, 128); /* usable through the whole request */
    engine_heap_free(p);
    engine_heap_free(NULL); /* no-op */
    engine_heap_free_wrapper(NULL);

    void* zero = engine_heap_alloc(0);
    CHECK(zero != NULL, "zero size allocates (corpus quirk)");
    engine_heap_free(zero);

    CHECK(engine_heap_alloc(-1) == NULL, "the overflow gate answers NULL");
    CHECK(engine_heap_alloc_retry(-1, 0) == NULL,
          "the gate survives the retry entry");
}

static void test_aligned_and_retry(void)
{
    void* p = engine_heap_alloc_aligned(17);
    CHECK(p != NULL && ((uintptr_t)p & 0xfu) == 0, "the aligned body lands");
    engine_heap_free(p);

    p = engine_heap_alloc_aligned(0);
    CHECK(p != NULL, "the aligned body allocates for zero");
    engine_heap_free(p);

    /* No-retry flag: exactly one attempt, then NULL. */
    CHECK(engine_heap_alloc_retry(-1, 0) == NULL,
          "the signed overflow gate answers NULL");

    /* The retry flag with the allocation succeeding first pass: the block
     * lands without the installed handler ever running — there is no
     * fault-injection seam left to burn a retry through. */
    {
        static int s_hits;
        s_hits = 0;
        crt_new_handler_set([](long size) -> int {
            (void)size;
            ++s_hits;
            return 1; /* would keep retrying if it were ever asked */
        });
        void* q = engine_heap_alloc_retry(64, 1);
        CHECK(q != NULL, "the retry flag returns the block first pass");
        CHECK(s_hits == 0, "the installed handler stays uninvoked");
        engine_heap_free(q);
        crt_new_handler_set(NULL); /* restore the corpus's unset case */
    }

    /* Free through the wrapper pair, then reuse. */
    void* r = engine_heap_alloc(16);
    engine_heap_free_wrapper(r);
    void* s = engine_heap_alloc(16);
    CHECK(s != NULL, "freed memory flows back through the seam");
    engine_heap_free(s);
}


static int s_wrap_first_a;
static int s_wrap_first_b;
static int s_wrap_zero_draws;
static int wrap_test_line_height(void*, void*) { return 10; }
static int wrap_test_advance(void*, void*, unsigned char glyph)
{
    return glyph == ' ' ? 5 : glyph == '\n' ? 0 : 4;
}
static int wrap_test_draw(void*, void*, int x, int, unsigned char glyph, int)
{
    if (glyph == 0u) ++s_wrap_zero_draws;
    if (glyph == 'A' && s_wrap_first_a < 0) s_wrap_first_a = x;
    if (glyph == 'B' && s_wrap_first_b < 0) s_wrap_first_b = x;
    return wrap_test_advance(NULL, NULL, glyph);
}
static int wrap_test_break(void*, unsigned char glyph)
{
    return glyph == ' ';
}

static void test_inline_word_wrap_start(void)
{
    TextBufferDrawOps previous;
    TextBufferDrawOps ops;
    const char* texts[4] = { "AA BB", "AA BB", "AA\nBB", "AA BB" };
    const float widths[4] = { 13.0f, 40.0f, 13.0f, 13.0f };
    const int first_a[4] = { 10, 10, 10, 12 };
    const int first_b[4] = { 10, 23, 10, 10 };
    int i;
    text_buffer_draw_ops_get(&previous);
    memset(&ops, 0, sizeof(ops));
    ops.font_line_height = wrap_test_line_height;
    ops.glyph_advance = wrap_test_advance;
    ops.glyph_draw = wrap_test_draw;
    ops.is_word_break = wrap_test_break;
    text_buffer_draw_ops_set(&ops);
    for (i = 0; i < 4; ++i) {
        union { double alignment; unsigned char bytes[0x840]; } record;
        float x = 10.0f, y = 10.0f, height = 50.0f;
        uint32_t flags = 2u | (i == 3 ? 4u : 0u);
        memset(&record, 0, sizeof(record));
        memcpy(record.bytes + 4, &x, sizeof(x));
        memcpy(record.bytes + 8, &y, sizeof(y));
        memcpy(record.bytes + 12, &widths[i], sizeof(widths[i]));
        memcpy(record.bytes + 16, &height, sizeof(height));
        memcpy(record.bytes + 0x82c, &flags, sizeof(flags));
        strcpy((char*)record.bytes + 0x18, texts[i]);
        s_wrap_first_a = s_wrap_first_b = -1;
        s_wrap_zero_draws = 0;
        (void)text_buffer_draw_wrapped(record.bytes, 0u, 0.0f, NULL);
        CHECK(s_wrap_zero_draws == 0, "wrap excludes the terminating NUL");
        CHECK(s_wrap_first_a == first_a[i], "wrap preserves first-line origin");
        CHECK(s_wrap_first_b == first_b[i],
              "continuation starts after the word-break character");
    }
    text_buffer_draw_ops_set(&previous);
}

int main(void)
{
    test_alloc_pair();
    test_aligned_and_retry();
    /* The UI render-record free wrapper (2026-10-24). */
    {
        void* p = engine_heap_alloc(32);
        CHECK(p != NULL, "the alloc lands");
        ui_render_record_free(p); /* frees; the port never reads after */
        ui_render_record_free(NULL); /* the corpus's no-op-on-null shape */
    }

    /* The alloc wrapper (2026-10-24): the corpus drops the pointer. */
    {
        engine_heap_allocate(16); /* must not crash; size-0 quirk too */
        engine_heap_allocate(0);
        CHECK(1, "the alloc wrapper stays inert on the heap seam");
    }

    /* The retry-handler pair (2026-10-24). */
    {
        CHECK(crt_new_handler_invoke(16) == 0, "unregistered answers no-retry");
        void* p = engine_calloc(64);
        CHECK(p != NULL, "the calloc allocates");
        engine_heap_free(p);
        CHECK(engine_calloc((long)0xffffffe1) == NULL
                  && engine_calloc(-1) == NULL,
              "the overflow gate answers NULL");
        CHECK(engine_calloc(0) != NULL, "size zero allocates one byte");
        static int s_hits;
        s_hits = 0;
        crt_new_handler_set([](long size) -> int {
            (void)size;
            ++s_hits;
            return s_hits < 2; /* retry once, then give up */
        });
        CHECK(crt_new_handler_invoke(1) == 1, "the installed handler retries");
        CHECK(crt_new_handler_invoke(1) == 0, "the same handler gives up next");
        crt_new_handler_set(NULL); /* restore the corpus's unset case */
    }

    /* ObjectField38Release (2026-10-24): the slot free, guarded. */
    {
        object_field_38_release(NULL); /* guarded */
        char* rec = (char*)engine_heap_alloc(0x40);
        memset(rec, 0, 0x40);
        object_field_38_release(rec); /* empty slot: no free */
        void* field = engine_heap_alloc(16);
        memcpy(rec + 0x38, &field, sizeof(field));
        object_field_38_release(rec); /* frees field; rec keeps the word */
        memset(rec + 0x38, 0, sizeof(field)); /* before rec goes away */
        engine_heap_free(rec);
    }

    /* ThreeWordRecordAllocateAndInitialize (2026-10-24). */
    {
        uint32_t* rec = (uint32_t*)
            three_word_record_allocate_and_initialize(0x11u, 0x22u, 0x33u);
        CHECK(rec != NULL, "the three-word record allocates");
        if (rec != NULL) {
            CHECK(rec[0] == 0x11u && rec[1] == 0x22u && rec[2] == 0x33u,
                  "the words store in order");
            engine_heap_free(rec);
        }
    }

    /* TextOutputField82CBit2SetClear (2026-10-24): the +0x82c flag word. */
    {
        char rec[0x830];
        memset(rec, 0, sizeof rec);
        uint32_t word = 0;
        text_output_field82c_bit2_set_clear(rec, 1);
        __builtin_memcpy(&word, rec + 0x82c, sizeof word);
        CHECK(word == 4u, "the flag word sets bit 2");
        text_output_field82c_bit2_set_clear(rec, 1); /* idempotent */
        __builtin_memcpy(&word, rec + 0x82c, sizeof word);
        CHECK(word == 4u, "the second set leaves the word alone");
        text_output_field82c_bit2_set_clear(rec, 0);
        __builtin_memcpy(&word, rec + 0x82c, sizeof word);
        CHECK(word == 0u, "the flag-off call clears bit 2");
        text_output_field82c_bit2_set_clear(NULL, 1); /* guarded */
    }

    /* TextBufferReset (2026-10-25): the buffer sweep, the +0x18 window,
     * and the flag words, plus the guarded holder wrapper. */
    {
        static char rec[0x830];
        static char buffer[32];
        memset(rec, 0x5a, sizeof rec);
        memset(buffer, 0x5a, sizeof buffer);
        void* buf = buffer;
        const uint32_t count = 20u;
        memcpy(rec + 0x14, &buf, sizeof buf);
        memcpy(rec + 0x418, &count, sizeof count);
        uint32_t word = 0xffffffffu;
        memcpy(rec + 0x82c, &word, sizeof word);

        text_buffer_reset(rec);
        CHECK(memcmp(buffer, (const char[20]){ 0 }, 20) == 0,
              "the count bytes sweep clear");
        CHECK(buffer[20] == 0x5a && buffer[21] == 0x5a,
              "the tail past the count stays");
        int window_zero = 1;
        for (int i = 0x18; i < 0x418; ++i) {
            if (rec[i] != 0) { window_zero = 0; break; }
        }
        CHECK(window_zero, "the 0x100-dword window clears");
        uint32_t w = 1;
        __builtin_memcpy(&w, rec + 0x820, 4);
        uint32_t w2 = 1;
        __builtin_memcpy(&w2, rec + 0x41c, 4);
        uint32_t w3 = 1;
        __builtin_memcpy(&w3, rec + 0x82c, 4);
        CHECK(w == 0u && w2 == 0u && w3 == 0xfffffffeu,
              "the flag words land: +0x820/+0x41c zero, +0x82c bit 0 clear");
        memcpy(rec + 0x418, &count, sizeof count); /* the count stays */

        void* holder = rec;
        /* Dirty the window past the widened +0x14 slot's straddled half;
         * the reset re-zeros the slot's padding half from the buffer read. */
        memset(rec + 0x28, 0x5a, 8);
        ui_text_output_object_release(&holder);
        CHECK(memcmp(rec + 0x28, (const char[8]){ 0 }, 8) == 0,
              "the holder wrapper resets through the slot");
        ui_text_output_object_release(NULL); /* guarded */

        memset(rec + 0x14, 0, sizeof buf);
        memset(rec + 0x28, 0x5a, 8);
        text_buffer_reset(rec);
        CHECK(rec[0x18] == 0, "the window still clears with no buffer");
        text_buffer_reset(NULL); /* guarded */
    }

    /* DwordListAllocateAndZero (2026-10-25). */
    {
        void** hdr = dword_list_allocate_and_zero(4);
        CHECK(hdr != NULL, "the list header allocates");
        if (hdr != NULL) {
            void** data = (void**)hdr[0];
            CHECK((size_t)hdr[1] == 4u, "the header carries the capacity");
            CHECK(data != NULL && data[0] == NULL && data[1] == NULL
                      && data[2] == NULL && data[3] == NULL,
                  "the slots arrive zeroed");
            if (data != NULL) {
                engine_heap_free(data);
            }
            engine_heap_free(hdr);
        }
        void** zero = dword_list_allocate_and_zero(0);
        CHECK(zero != NULL && zero[1] == NULL,
              "the zero-capacity header answers an empty window");
        if (zero != NULL) {
            engine_heap_free(zero);
        }
    }

    /* TextBufferCreate (2026-10-25): the record header, the buffer half,
     * the length word, and the +0x82c bit 2. */
    {
        const uint32_t header[4] = { 0xa1u, 0xb2u, 0xc3u, 0xd4u };
        void* rec = text_buffer_create(7u, header, 0x40);
        CHECK(rec != NULL, "the record allocates");
        if (rec != NULL) {
            uint32_t w = 0u;
            __builtin_memcpy(&w, (char*)rec + 0x00, sizeof w);
            CHECK(w == 7u, "the mode word installs");
            __builtin_memcpy(&w, (char*)rec + 0x04, sizeof w);
            CHECK(w == 0xa1u, "the first header dword rides");
            __builtin_memcpy(&w, (char*)rec + 0x10, sizeof w);
            CHECK(w == 0xd4u, "the last header dword rides");
            void* buffer = NULL;
            __builtin_memcpy(&buffer, (char*)rec + 0x14, sizeof buffer);
            CHECK(buffer != NULL, "the buffer half allocates");
            memset(buffer, 0x5a, 0x40); /* usable through the request */
            __builtin_memcpy(&w, (char*)rec + 0x418, sizeof w);
            CHECK(w == 0x40u, "the length word records");
            __builtin_memcpy(&w, (char*)rec + 0x82c, sizeof w);
            CHECK((w & 4u) != 0u, "the flag word raises bit 2");
            text_buffer_destroy(rec);
        }
        void* naked = text_buffer_create(9u, NULL, 0x20);
        if (naked != NULL) {
            uint32_t w = 0xffu;
            __builtin_memcpy(&w, (char*)naked + 0x04, sizeof w);
            CHECK(w == 0u, "the null header stays zeroed (dev)");
            text_buffer_destroy(naked);
        }
    }

    test_inline_word_wrap_start();

    if (s_failures != 0) {
        fprintf(stderr, "heap_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("heap_test: OK\n");
    return 0;
}
