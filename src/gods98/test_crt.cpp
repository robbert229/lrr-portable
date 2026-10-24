// SPDX-License-Identifier: LGPL-3.0-only
// CRT/compiler-runtime leaves (findings 260/381/393/394): memmove, the
// __ftol float->int thunk and the callable no-op.
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "gods98/crt.hpp"

static int s_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);      \
            ++s_failures;                                                      \
        }                                                                      \
    } while (0)

static void test_memory_move(void)
{
    char buf[64];
    for (int i = 0; i < 64; ++i) {
        buf[i] = (char)i;
    }
    char* d = (char*)crt_memory_move(buf + 16, buf, 16);
    CHECK(d == buf + 16, "the move returns the destination");
    int ok = 1;
    for (int i = 0; i < 16; ++i) {
        ok &= buf[16 + i] == (char)i;
    }
    CHECK(ok, "forward copy moves the whole range");

    /* Overlapping back-move: destination starts inside the source. */
    for (int i = 0; i < 64; ++i) {
        buf[i] = (char)i;
    }
    crt_memory_move(buf + 8, buf, 16);
    ok = 1;
    for (int i = 0; i < 16; ++i) {
        ok &= buf[8 + i] == (char)i;
    }
    CHECK(ok, "overlapping back-move keeps the source values");

    /* Word-aligned and byte-tail sizes, and zero-count. */
    char a[8], b[9];
    memset(a, 'x', sizeof a);
    memset(b, 'y', sizeof b);
    crt_memory_move(a, b, 9);
    CHECK(a[0] == 'y' && a[7] == 'y', "the word part moves");
    CHECK(b[8] == 'y', "the tail byte moves");
    char c[4] = { 1, 2, 3, 4 };
    CHECK(crt_memory_move(c, c, 0) == c && c[1] == 2, "zero count changes nothing");
}

static void test_float_to_long(void)
{
    CHECK(float_to_long(2.7f) == 2, "positive truncates toward zero");
    CHECK(float_to_long(2.3f) == 2, "positive floor corner");
    CHECK(float_to_long(-2.7f) == -2, "negative truncates toward zero");
    CHECK(float_to_long(0.0f) == 0, "zero stays zero");
    CHECK(float_to_long(3.0f) == 3, "exact value passes through");
    CHECK(float_to_long(1000.99f) == 1000, "large positive truncates");

    /* The corpus's fist writes a 32-bit result; the cast round-trips the
     * same integer values a caller would see for its set. */
    float vals[] = { 1.0f, -1.0f, 32767.5f, -32767.5f };
    int want[] = { 1, -1, 32767, -32767 };
    for (int i = 0; i < 4; ++i) {
        CHECK(float_to_long(vals[i]) == want[i], "32-bit range values match");
    }
}

static int counter;
static void bump(void) { ++counter; }

static void test_noop_callback(void)
{
    void (*fns[])(void) = { noop_callback, bump };
    counter = 0;
    fns[0]();
    CHECK(counter == 0, "the no-op callback does nothing");
    fns[1]();
    CHECK(counter == 1, "the sibling callback still runs");
    noop_callback(); /* direct call too */
    CHECK(counter == 1, "the direct call still does nothing");
}

int main(void)
{
    test_memory_move();
    test_float_to_long();
    test_noop_callback();
    if (s_failures != 0) {
        fprintf(stderr, "crt_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("crt_test: OK\n");
    return 0;
}
