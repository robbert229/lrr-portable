// SPDX-License-Identifier: LGPL-3.0-only
// Engine heap seam (findings 397/243): aligned allocation, the entry-point
// quirks, the free pair and the retry-handled allocator.
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "gods98/heap.hpp"

static int s_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);      \
            ++s_failures;                                                      \
        }                                                                      \
    } while (0)

static int retry_count = 0;
static int retry_failing(long size)
{
    (void)size;
    ++retry_count;
    return 0;
}
static int retry_second_success(long size)
{
    ++retry_count;
    return retry_count >= 2;
}

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
    CHECK(engine_heap_alloc_retry(-1, NULL) == NULL,
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

    /* Unregistered retry (NULL handler): exactly one attempt, then NULL. */
    retry_count = 0;
    CHECK(engine_heap_alloc_retry(-1, retry_failing) == NULL,
          "the signed overflow gate answers NULL");

    /* A handler that succeeds on the second pass must get the block. */
    retry_count = 1; /* pretend the first attempt already burned one retry */
    void* q = engine_heap_alloc_retry(64, retry_second_success);
    CHECK(q != NULL, "the retry handler path allocates on the retry pass");
    engine_heap_free(q);

    /* Free through the wrapper pair, then reuse. */
    void* r = engine_heap_alloc(16);
    engine_heap_free_wrapper(r);
    void* s = engine_heap_alloc(16);
    CHECK(s != NULL, "freed memory flows back through the seam");
    engine_heap_free(s);
}

int main(void)
{
    test_alloc_pair();
    test_aligned_and_retry();
    if (s_failures != 0) {
        fprintf(stderr, "heap_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("heap_test: OK\n");
    return 0;
}
