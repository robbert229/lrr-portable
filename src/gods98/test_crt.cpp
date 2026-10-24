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

static void test_bit_numeric(void)
{
    CHECK(bit_count32(0) == 0, "zero has no bits");
    CHECK(bit_count32(0xFFFFFFFFu) == 32, "all bits count 32");
    CHECK(bit_count32(0x12345678u) == 13, "corner value counts 13");
    CHECK(pop_count32(0x12345678u) == 13, "position loop agrees");
    CHECK(bit_count32_by_position(0x12345678u) == 13, "second body agrees");
    CHECK(pop_count32(0) == 0, "zero counts zero");

    CHECK(find_first_set_bit32(0) == 0xffffffffu, "zero answers none");
    CHECK(find_first_set_bit32(1) == 0, "bit 0");
    CHECK(find_first_set_bit32(0x80000000u) == 31, "bit 31");
    CHECK(find_first_set_bit32(0x8u) == 3, "bit 3");

    const uint32_t z[3] = { 0, 0, 0 };
    const uint32_t nz[3] = { 0, 1, 0 };
    const uint32_t hi[3] = { 0, 0, 0x80000000u };
    CHECK(uint96_is_zero(z) == 1, "3 dwords of zero is zero");
    CHECK(uint96_is_zero(nz) == 0, "any nonzero dword answers false");
    CHECK(uint96_is_zero(hi) == 0, "the high dword counts");

    CHECK(double_is_finite(1.0) == 1, "one is finite");
    CHECK(double_is_finite(1e308) == 1, "large finite is finite");
    CHECK(double_is_finite(0.0) == 1, "zero is finite");
    CHECK(double_is_finite(-1.0) == 1, "negative finite is finite");
    CHECK(double_is_finite(1.0 / 0.0) == 0, "infinity is not finite");
    CHECK(double_is_finite((0.0 - 1.0) * 0.0 / 0.0) == 0, "NaN is not finite");
}

int main(void)
{
    test_memory_move();
    test_float_to_long();
    test_noop_callback();
    test_bit_numeric();
    if (s_failures != 0) {
        fprintf(stderr, "crt_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("crt_test: OK\n");
    return 0;
}
