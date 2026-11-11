// SPDX-License-Identifier: LGPL-3.0-only
// CallbackRecordAllocateInitialize (FUN_004628e0; findings 366/393). The
// steered-group stack is tested by src/game/test_steering.cpp.
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "gods98/ptrstack.hpp"
#include "gods98/heap.hpp"

static int s_failures = 0;

static uint32_t dword_at(const void* block, long offset)
{
    uint32_t v;
    __builtin_memcpy(&v, (const char*)block + offset, sizeof v);
    return v;
}

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);      \
            ++s_failures;                                                      \
        }                                                                      \
    } while (0)

int main(void)
{
    {
        void* out = (void*)(uintptr_t)0xdeadbeef;
        void* rec = callback_record_allocate_initialize(&out, 2u, 3u, 4u, 5u,
                                                        6u);
        CHECK(rec != NULL && out == rec,
              "the out slot rides even past the pins (pinned quirk)");
        CHECK(dword_at(rec, 0x00) == 3u && dword_at(rec, 0x08) == 3u,
              "param_3 rides dwords 0 and 2 (pinned)");
        CHECK(dword_at(rec, 0x04) == 2u && dword_at(rec, 0x0c) == 4u
                  && dword_at(rec, 0x10) == 5u && dword_at(rec, 0x14) == 6u,
              "the payload dwords land in order");
        CHECK(dword_at(rec, 0x18) == 0u && dword_at(rec, 0x1c) == 0u,
              "the zeroed dwords stay clear");
        void* slot = NULL;
        __builtin_memcpy(&slot, (char*)rec + 0x20, sizeof slot);
        CHECK(slot != NULL, "the link slot allocates");
        void* buffer = NULL;
        __builtin_memcpy(&buffer, slot, sizeof buffer);
        CHECK(buffer != NULL, "the slot front points at the buffer");
        int zeroed = 1;
        for (int off = 0; off < 0x20; off += 4) {
            zeroed = zeroed && dword_at(buffer, off) == 0u;
        }
        CHECK(zeroed, "the 0x20 buffer zeroes through");
        engine_heap_free(buffer);
        engine_heap_free(slot);
        engine_heap_free(rec);

        /* the guarded out slot (deviation: the corpus strides it) */
        void* rec2 = callback_record_allocate_initialize(NULL, 1u, 1u, 1u, 1u,
                                                         1u);
        CHECK(rec2 != NULL, "the guarded out slot still builds");
        void* slot2 = NULL;
        __builtin_memcpy(&slot2, (char*)rec2 + 0x20, sizeof slot2);
        void* buffer2 = NULL;
        __builtin_memcpy(&buffer2, slot2, sizeof buffer2);
        engine_heap_free(buffer2);
        engine_heap_free(slot2);
        engine_heap_free(rec2);
    }

    if (s_failures != 0) {
        fprintf(stderr, "ptrstack_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("ptrstack_test: PASS\n");
    return 0;
}
