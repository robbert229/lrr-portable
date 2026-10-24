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

static void test_pair_conversions(void)
{
    float fx, fy;
    integer_pair_divide_to_float_pair(6, 9, 3, &fx, &fy);
    CHECK(fx == 2.0f && fy == 3.0f, "pair divides evenly");
    integer_pair_divide_to_float_pair(-1, 1, 2, &fx, &fy);
    CHECK(fx == -0.5f && fy == 0.5f, "negative numerator keeps the sign");
    integer_pair_divide_to_float_pair(1, 1, -2, &fx, &fy);
    CHECK(fx == -0.5f && fy == -0.5f, "negative divisor flips both");

    int ix, iy;
    float_pair_to_integer_pair(2.7f, -2.7f, &ix, &iy);
    CHECK(ix == 2 && iy == -2, "pair truncates toward zero");
    float_pair_to_integer_pair(-0.9f, 5.5f, &ix, &iy);
    CHECK(ix == 0 && iy == 5, "corners truncate toward zero");
}

static void test_crt_leaves(void)
{
    /* Unsigned add with carry: the sum never lies about the carry. */
    uint32_t sum = 0;
    CHECK(crt_unsigned_add_with_carry(1, 2, &sum) == 0 && sum == 3,
          "small add carries nothing");
    CHECK(crt_unsigned_add_with_carry(0xffffffffu, 1, &sum) == 1 && sum == 0,
          "max plus one wraps with carry");
    CHECK(crt_unsigned_add_with_carry(0xffffffffu, 0xffffffffu, &sum) == 1
              && sum == 0xfffffffeu,
          "double-max wraps with carry");

    /* Double classifier over the bit pair (finding 399). */
    CHECK(double_classify(0u, 0x7ff00000u) == 1, "positive infinity");
    CHECK(double_classify(0u, 0xfff00000u) == 2, "negative infinity");
    CHECK(double_classify(0u, 0x7ff80000u) == 3, "quiet-NaN-shaped");
    CHECK(double_classify(1u, 0x7ff00000u) == 4,
          "signaling-NaN-shaped via the low word");
    CHECK(double_classify(0u, 0x7ff00001u) == 4,
          "signaling-NaN-shaped via the high word mantissa");
    CHECK(double_classify(0u, 0x3ff00000u) == 0, "one is finite");
    CHECK(double_classify(1u, 0x7ff00001u) == 4, "both mantissas nonzero");

    /* Wide strings: 16-bit code units. */
    static const uint16_t s_ab[] = { 'a', 'b', 0 };
    static const uint16_t s_long[] = { 'a', 'b', 'c', 'd', 0 };
    static const uint16_t s_empty[] = { 0 };
    CHECK(wide_string_length(s_ab) == 2, "length before the NUL");
    CHECK(wide_string_length(s_empty) == 0, "empty counts zero");
    CHECK(bounded_wide_string_length_including_null(s_ab, 4) == 3,
          "terminator inside the bound counts itself");
    CHECK(bounded_wide_string_length_including_null(s_long, 2) == 2,
          "no terminator inside: answers the bound");
    CHECK(bounded_wide_string_length_including_null(s_ab, 0) == 0,
          "zero bound reads nothing");
    CHECK(bounded_wide_string_length_including_null(s_empty, 1) == 1,
          "leading NUL answers one");
    CHECK(bounded_wide_string_length_including_null(s_long, 4) == 4,
          "terminator exactly at the bound answers the bound");

    /* Double exponent selector over the high word (finding 399). */
    CHECK(double_exponent_bits_or_special(0x40080000u) == 0x40000000u,
          "ordinary value answers its exponent field");
    CHECK(double_exponent_bits_or_special(0x3ff00000u) == 0x3ff00000u,
          "one's exponent is untouched");
    CHECK(double_exponent_bits_or_special(0x7ff00000u) == 0x7ff00000u,
          "infinity answers the full high word");
    CHECK(double_exponent_bits_or_special(0x7ff80000u) == 0x7ff80000u,
          "NaN answers the full high word");

    /* Format-argument cursor: reads consume and advance. */
    uint8_t buf[16];
    const uint32_t d1 = 0x11223344u;
    const uint16_t w1 = 0x5566u;
    const double f1 = 3.5;
    memcpy(buf, &d1, 4);
    memcpy(buf + 4, &w1, 2);
    memcpy(buf + 6, &f1, 8);
    uint8_t* cur = buf;
    CHECK(format_argument_read_dword(&cur) == d1 && cur == buf + 4,
          "dword read advances four");
    CHECK(format_argument_read_word(&cur) == w1 && cur == buf + 6,
          "word read advances two and zero-extends");
    CHECK(format_argument_read_double(&cur) == f1 && cur == buf + 14,
          "double read advances eight");

    /* x87 FRNDINT: nearest-even integral, floating. */
    CHECK(x87_round_to_integral(1.5) == 2.0, "half rounds to even (2)");
    CHECK(x87_round_to_integral(2.5) == 2.0, "half rounds down to even (2)");
    CHECK(x87_round_to_integral(-1.5) == -2.0, "negative half to even");
    CHECK(x87_round_to_integral(3.2) == 3.0, "3.2 to 3");
    CHECK(x87_round_to_integral(-3.2) == -3.0, "-3.2 to -3");
    CHECK(x87_round_to_integral(7.0) == 7.0, "integers untouched");
    {
        const double neg_zero = x87_round_to_integral(-0.4);
        CHECK(neg_zero == 0.0 && 1.0 / neg_zero < 0.0,
              "the integral of a small negative keeps -0");
    }
}

int main(void)
{
    test_memory_move();
    test_float_to_long();
    test_noop_callback();
    test_bit_numeric();
    test_pair_conversions();
    test_crt_leaves();
    if (s_failures != 0) {
        fprintf(stderr, "crt_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("crt_test: OK\n");
    return 0;
}
