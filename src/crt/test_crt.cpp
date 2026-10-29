// SPDX-License-Identifier: LGPL-3.0-only
// CRT/compiler-runtime leaves (findings 260/381/393/394): memmove, the
// __ftol float->int thunk and the callable no-op.
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "crt/crt.hpp"
#include "gods98/heap.hpp"

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

/* Bit view for the floating-leaf checks. */
static uint64_t crt_bits(double d)
{
    uint64_t b = 0;
    __builtin_memcpy(&b, &d, sizeof(b));
    return b;
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
    CHECK(double_exponent_bits_or_special(0u, 0x40080000u) == 0x40000000u,
          "ordinary value answers its exponent field");
    CHECK(double_exponent_bits_or_special(0u, 0x3ff00000u) == 0x3ff00000u,
          "one's exponent is untouched");
    CHECK(double_exponent_bits_or_special(0u, 0x7ff00000u) == 0x7ff00000u,
          "infinity answers the full high word");
    CHECK(double_exponent_bits_or_special(0u, 0x7ff80000u) == 0x7ff80000u,
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

    /* DoubleBuildWithExponent (FUN_00493a40): the bit recipe, checked as
     * bit patterns (the value's meaning stays with the frexp caller). */
    {
        /* Recipe: top = ((high >> 16) & 0x800f) | ((exp + 0x3fe) << 4). */
        CHECK(crt_bits(double_build_with_exponent(0x12345678u, 0x43218abcu, 0))
                  == 0x3FE18ABC12345678ull, "exp 0 bias word");
        CHECK(crt_bits(double_build_with_exponent(0x12345678u, 0x43218abcu, 1))
                  == 0x3FF18ABC12345678ull, "exponent 1 steps the bias word");
        CHECK(crt_bits(double_build_with_exponent(0x12345678u, 0x43218abcu, -1))
                  == 0x3FD18ABC12345678ull, "exponent -1 steps it down");
        /* The mask keeps the sign bit and the fraction nibble. */
        CHECK(crt_bits(double_build_with_exponent(0u, 0x80000000u, 1))
                  == 0xBFF0000000000000ull, "sign bit survives the mask");
        CHECK(crt_bits(double_build_with_exponent(0x12345678u, 0x000f0abcu, 0))
                  == 0x3FEF0ABC12345678ull, "the fraction nibble survives");
        /* 16-bit truncation of the bias shift (exp = -0x3ff -> 0xFFF0). */
        CHECK(crt_bits(double_build_with_exponent(1u, 0u, -0x3ff))
                  == 0xFFF0000000000001ull, "the bias word truncates to 16 bits");
    }

    {
        const double neg_zero = x87_round_to_integral(-0.4);
        CHECK(neg_zero == 0.0 && 1.0 / neg_zero < 0.0,
              "the integral of a small negative keeps -0");
    }
}

/* Constant and DAT-global band (findings 380/383/386/392/399/406/440). */
static void test_constant_band(void)
{
    CHECK(constant_zero_float_get() == 0.0, "zero float answers zero");
    double z = constant_zero_float_get();
    CHECK(z == 0.0 && memcmp(&z, "\0\0\0\0\0\0\0\0", 8) == 0,
          "the zero float is bit-exact +0.0");
    CHECK(constant_negative_10000_get() == -10000,
          "the constant reads as negative ten thousand");
    uint32_t raw = (uint32_t)constant_negative_10000_get();
    CHECK(raw == 0xffffd8f0u, "the dword view is the corpus's 0xffffd8f0");
    CHECK(processor_feature_fallback_false() == 0u, "fallback answers zero");
    CHECK(flh_magic_uu_noop() == 0u, "the UU noop answers zero");
}

static void test_static_globals(void)
{
    /* Store-and-return: the return value is the stored dword. */
    uint32_t v = 0x1234abcdu;
    CHECK(global_value_004ff9bc_store_and_return(&v) == v,
          "stores and returns the pointed dword");
    v = 0x77777777u;
    CHECK(global_value_004ff9bc_store_and_return(&v) == v,
          "a second store returns the new value");
    CHECK(global_value_004ff9bc_store_and_return(NULL) == 0u,
          "NULL answers zero (deviation)");

    /* The unresolved-meaning global: faithful read of static-backed
     * storage (x87 float10 deviation — double). */
    CHECK(global_float_field_00556f00_get() == 0.0,
          "the global reads zero until a writer lands");

    /* The unowned counter/accumulator band (2026-10-24): each leaf is a
     * single read-modify-write over its static-backed global. */
    {
        const int base_counter = (int)crt_counter_004ff990_peek();
        global_counter_004ff990_increment();
        global_counter_004ff990_increment();
        CHECK(crt_counter_004ff990_peek() == (uint32_t)(base_counter + 2),
              "the increment bumps by one per call");
        global_counter_004ff990_reset();
        CHECK(crt_counter_004ff990_peek() == 0u, "the reset zeroes");
    }

    {
        const float base_a = crt_float_00554adc_peek();
        const float base_e = crt_float_00554efc_peek();
        global_float_at_00554adc_add_one();
        global_float_at_00554adc_add_one();
        global_float_at_00554efc_add_one();
        CHECK(crt_float_00554adc_peek() == base_a + 2.0f
                  && crt_float_00554efc_peek() == base_e + 1.0f,
              "the add-one leaves raise each float once per call");
    }

    {
        const unsigned b4 = crt_counter_00555324_peek();
        const unsigned b8 = crt_counter_00555328_peek();
        const unsigned bc = crt_counter_0055532c_peek();
        global_counter_at_00555328_increment();
        CHECK(crt_counter_00555324_peek() == b4
                  && crt_counter_00555328_peek() == b8 + 1u
                  && crt_counter_0055532c_peek() == bc,
              "each counter bumps independently");
    }

    {
        const float base_3 = crt_float_00555330_peek();
        const float base_1 = crt_float_00555120_peek();
        global_float_accumulator_00555330_add(1.5f);
        global_float_accumulate_00555120(-0.5f);
        CHECK(crt_float_00555330_peek() == base_3 + 1.5f
                  && crt_float_00555120_peek() == base_1 - 0.5f,
              "the accumulators add the argument once per call");
    }
}

/* Shims for the entry cache get-or-load test: counting loaders. */
static int s_bmp_loads = 0;
static int s_font_loads = 0;
static void* s_bmp_loader(const char* key)
{
    (void)key;
    ++s_bmp_loads;
    return (void*)0xb0;
}
static void* s_font_loader(const char* key)
{
    (void)key;
    ++s_font_loads;
    return (void*)0xf0;
}

int main(void)
{
    test_memory_move();
    test_float_to_long();
    test_noop_callback();
    test_bit_numeric();
    test_pair_conversions();
    test_crt_leaves();
    test_constant_band();
    test_static_globals();
    /* The display-resolution filter tuples (2026-10-24): the seeded shape,
     * the guarded window, and the overflow count. */
    {
        CHECK(display_resolution_filter_tuple_count() == 0,
              "the table starts empty (no seeding seam)");
        display_resolution_filter_tuple_append(640, 480, 16);
        display_resolution_filter_tuple_append(800, 600, 16);
        display_resolution_filter_tuple_append(1024, 768, 16);
        int w = 0, h = 0, b = 0;
        display_resolution_filter_tuple_get(1, &w, &h, &b);
        CHECK(display_resolution_filter_tuple_count() == 3 && w == 800
                  && h == 600 && b == 16,
              "the seeded tuple lands in its slot");
        for (int i = 0; i < 13; ++i) {
            display_resolution_filter_tuple_append(7, 7, 7);
        }
        display_resolution_filter_tuple_get(15, &w, &h, &b);
        CHECK(display_resolution_filter_tuple_count() == 16 && w == 7
                  && h == 7 && b == 7,
              "the sixteenth slot fills");
        display_resolution_filter_tuple_append(1, 1, 1);
        CHECK(display_resolution_filter_tuple_count() == 17,
              "the corpus keeps counting past the window");
        w = 5;
        display_resolution_filter_tuple_get(16, &w, &h, &b);
        CHECK(w == 0, "the guarded read answers zero (deviation)");
    }

    /* The remaining unowned DAT one-liners (2026-10-24). */
    CHECK(global_flag_004ff978_get() == 0u, "the flag word starts clear");
    global_value_004ffe48_set(0xABu); /* the setter stores (no getter shape) */
    global_value_004ffe48_set(0u);    /* restore for later tests */

    /* The unowned DAT word-store band (2026-10-24). */
    {
        CHECK(global_flag_bit0_is_set() == 0u, "the advisor word starts clear");
        advisor_state_word_set(1u);
        CHECK(global_flag_bit0_is_set() == 1u, "bit 0 reads through");
        advisor_state_word_set(2u);
        CHECK(global_flag_bit0_is_set() == 0u, "bit 1 does not count");
        advisor_state_word_set(0u);

        CHECK(global_task_refresh_flag_get() == 0u, "the flag starts clear");
        state_flag_toggle_when_zero();
        CHECK(global_task_refresh_flag_get() == 1u, "zero becomes one");
        state_flag_toggle_when_zero();
        CHECK(global_task_refresh_flag_get() == 0u, "one returns to zero");

        level_state_stored_value_set(5u);
        CHECK(level_stored_word_peek() == 5u, "the stored word lands");
        global_mode_zero_input_set(1);
        CHECK(mode_zero_input_flag_peek() == 0u, "nonzero clears the flag");
        global_mode_zero_input_set(0);
        CHECK(mode_zero_input_flag_peek() == 1u, "zero raises the flag");
        rockfall_style_table_base_set(3u);
        CHECK(rockfall_table_base_peek() == 3u, "the base index lands");

        global_flag_bit0_clear(); /* the clear is a no-op on the zero word */
        CHECK(ui_flag_word_peek() == 0u, "the word stays clear");

        global_config_value_set(3u);
        CHECK(global_config_word_peek() == 3u, "the erosion config lands");
        global_config_value_set(0u);
        level_state_stored_value_set(0u);
        rockfall_style_table_base_set(0u);
    }

    /* Display dimensions + request-record predicates (2026-10-24). */
    {
        CHECK(display_width_get() == 0 && display_height_get() == 0,
              "the extent pair starts zero");
        display_dimensions_set(640, 480);
        CHECK(display_width_get() == 640 && display_height_get() == 480,
              "the hook stores the pair");
        display_dimensions_set(0, 0);

        CHECK(request_record_ready_flag4(3) == 0
                  && request_record_flag8_is_set(3) == 0,
              "the record window starts clear");
        request_record_flags_set(4, 4u);
        CHECK(request_record_ready_flag4(4) != 0, "flag 4 without 8 is ready");
        request_record_flags_set(4, 0xCu);
        CHECK(request_record_ready_flag4(4) == 0
                  && request_record_flag8_is_set(4) == 8,
              "flag 8 withholds either predicate");
        request_record_flags_set(4, 2u);
        CHECK(request_record_ready_flag2(4) != 0, "flag 2 without 8 is ready");
        request_record_flags_set(5, 0u);
        CHECK(request_record_ready_flag2(5) == 0
                  && request_record_flag8_is_set(5) == 0,
              "a cleared record answers nothing");
        CHECK(request_record_ready_flag4(-1) == 0
                  && request_record_flag8_is_set(18) == 0,
              "the guarded window answers outside 18 entries");
        request_record_flags_set(4, 0u);
    }

    /* The remaining DAT one-liner band (2026-10-24). */
    {
        global_value_0050080c_set(6u);
        CHECK(global_value_0050080c_peek() == 6u, "the store lands");
        global_value_0050080c_set(0u);
        CHECK(indexed_global_value_at_get(0) == 0u
                  && indexed_global_value_at_get(16) == 0u
                  && indexed_global_value_at_get(-1) == 0u,
              "the font window starts clear and guards clean");
        font_cursor_word_set(3, 7u);
        CHECK(indexed_global_value_at_get(3) == 7u, "the indexed read lands");
        font_cursor_word_set(-1, 4u);
        CHECK(global_cursor_word_get() == 4u, "the cursor word reads back");
        font_cursor_word_set(3, 0u);
        font_cursor_word_set(-1, 0u);
    }

    /* The stub pair + advisor word clear (2026-10-24). */
    {
        crt_no_op_stub();
        crt_no_op_stub_variant(); /* the corpus is pure return */
        advisor_state_word_set(3u);
        advisor_clear_active_state();
        CHECK(advisor_state_word_peek() == 1u, "the active bit drops alone");
        advisor_state_word_set(0u);
    }

    /* ByteRangeSwap (2026-10-24). */
    {
        char x[4] = { 1, 2, 3, 4 };
        char y[4] = { 9, 8, 7, 6 };
        byte_range_swap(x, y, 4);
        CHECK(x[0] == 9 && x[1] == 8 && x[2] == 7 && x[3] == 6
                  && y[0] == 1 && y[1] == 2 && y[2] == 3 && y[3] == 4,
              "the ranges exchange bytes");
        byte_range_swap(x, x, 4); /* same-pointer shortcut */
        CHECK(x[0] == 9, "the shortcut keeps the first range");
        byte_range_swap(NULL, y, 2); /* guarded */
    }

    /* The DAT window band (2026-10-24). */
    {
        indexed_pair_set(2, 11u, 22u);
        indexed_pair_set(100, 1u, 1u); /* guarded */
        uint32_t x = 0u, y = 0u;
        indexed_pair_get(2, &x, &y);
        CHECK(x == 11u && y == 22u, "the pair window round-trips");
        indexed_pair_get(-1, &x, &y);
        indexed_pair_get(2, NULL, &y); /* guarded */
        CHECK(x == 11u && y == 22u, "the guards skip");

        indexed_float_pair_set(1, 3u, 8u);
        float fx = 0, fy = 0;
        indexed_float_pair_peek(1, &fx, &fy);
        CHECK(fx == 3.0f && fy == 8.0f, "the float pair casts land");

        char metric[0x28];
        memset(metric, 0, sizeof(metric));
        indexed_record_pointer_set(3, metric);
        indexed_text_metrics_pair_set(3, 6u, 7u);
        CHECK(*(const uint32_t*)(metric + 0x24) == 1u, "the set raises the flag");
        indexed_text_metrics_pair_get(3, &x, &y);
        CHECK(x == 6u && y == 7u, "the metrics pair reads");
        indexed_text_metrics_pair_set(4, 1u, 1u); /* empty slot: inert */
        indexed_text_metrics_pair_get(9, &x, &y); /* guarded */

        const uint32_t xy[2] = { 5u, 6u };
        region_bounds_set(xy, 640u, 480u);
        uint32_t rx[2] = { 0, 0 };
        float rw[2] = { 0, 0 };
        region_bounds_peek(rx, rw);
        CHECK(rx[0] == 5u && rx[1] == 6u && rw[0] == 640.0f && rw[1] == 480.0f,
              "the region quad lands");
        region_bounds_set(NULL, 1u, 1u); /* guarded */

        char block[0x2c * 3];
        memset(block, 0, sizeof(block));
        panel_draw_records_set(0, block);
        CHECK(panel_draw_records_peek(0) == (void*)block, "the hook lands");
        panel_item_flag16_set(0, 1, 1);
        uint32_t w = 0u;
        __builtin_memcpy(&w, block + 0x28 + 0x2c, 4);
        CHECK(w == 0x10u, "the item word raises bit 16");
        panel_item_flag32_set(0, 1, 1);
        __builtin_memcpy(&w, block + 0x28 + 0x2c, 4);
        CHECK(w == 0x30u, "bit 32 stacks");
        panel_item_flag16_set(0, 1, 0);
        __builtin_memcpy(&w, block + 0x28 + 0x2c, 4);
        CHECK(w == 0x20u, "bit 16 drops alone");
        panel_item_flag16_set(7, 0, 1); /* empty panel: guarded */
        panel_item_flag32_set(0, -1, 1); /* guarded */

        request_record_flags_set(5, 2u);
        panel_state_flags_toggle(5);
        CHECK(request_record_flags_peek(5) == 0xcu, "the branch-two fold");
        request_record_flags_set(5, 8u);
        panel_state_flags_toggle(5);
        CHECK(request_record_flags_peek(5) == (8u & ~4u) | 0xau,
              "the branch-one fold");
        panel_state_flags_toggle(100); /* guarded */
        request_record_flags_set(5, 0u);
    }

    /* RewardScoreAdd (2026-10-24). */
    {
        const float base = reward_score_total_peek();
        const uint32_t n = reward_score_count_peek();
        reward_score_add(3.5f);
        CHECK(reward_score_total_peek() == base + 3.5f, "the score accumulates");
        CHECK(reward_score_count_peek() == n + 1u, "the count rises once");
    }

    /* The small global primitives band (2026-10-24). */
    {
        uint32_t limbs[3] = { 1u, 2u, 3u };
        uint96_zero(limbs);
        CHECK(limbs[0] == 0u && limbs[1] == 0u && limbs[2] == 0u,
              "the limbs clear");
        CHECK(uint96_is_zero(limbs) == 1, "the sibling agrees");
        uint96_zero(NULL); /* guarded */

        global_value_word_set(5);
        global_value_clear_if_equal(7);
        CHECK(global_value_word_peek() == 5, "a mismatched release keeps");
        global_value_clear_if_equal(5);
        CHECK(global_value_word_peek() == 0, "the matching release clears");

        char node[0x20];
        memset(node, 0, sizeof(node));
        linked_list_push_at_global_head(node);
        CHECK(linked_list_global_head_peek() == (void*)node, "the push lands");
        void* up = NULL;
        __builtin_memcpy(&up, node + 0x1c, sizeof(up));
        CHECK(up == NULL, "the tail link is null after the first push");
        char node2[0x20];
        memset(node2, 0, sizeof(node2));
        linked_list_push_at_global_head(node2);
        __builtin_memcpy(&up, node2 + 0x1c, sizeof(up));
        CHECK(up == (void*)node && linked_list_global_head_peek() == (void*)node2,
              "the second push relinks");
        linked_list_push_at_global_head(NULL); /* guarded */

        global_record_list_push_head(node2);
        CHECK(global_record_list_head_peek() == (void*)node2, "the record push lands");

        const uint32_t expect_zero[3] = { 0u, 0u, 0u };
        CHECK(scene_effect_registry_reset() == 1, "the reset answers one");
        uint32_t words[3] = { 9u, 9u, 9u };
        scene_effect_registry_words_peek(words);
        CHECK(words[0] == 0u && words[1] == 0u && words[2] == 0u,
              "the registry words clear");
        scene_effect_registry_words_peek(NULL); /* guarded */

        static int s_fatal_hits;
        s_fatal_hits = 0;
        crt_fatal_exit_observer_set([](void) -> void { ++s_fatal_hits; });
        crt_fatal_message_exit();
        CHECK(s_fatal_hits == 1, "the observer stands in for the abort");
        crt_fatal_exit_observer_set(NULL); /* restore the aborting default */
    }

    /* The panel/view blit band (2026-10-24). */
    {
        /* The flag-8 enable gate. */
        static char fblock[0x2c * 2];
        memset(fblock, 0, sizeof(fblock));
        uint32_t* w = (uint32_t*)(fblock + 0x28);
        panel_draw_records_set(7, fblock);
        panel_item_flag8_set_if_enabled(7, 0, 1);
        CHECK(*w == 0u, "the disabled gate keeps bit 8 clear");
        *w = 0x20u;
        panel_item_flag8_set_if_enabled(7, 0, 1);
        CHECK(*w == 0x28u, "the enabled gate takes bit 8");
        panel_item_flag8_set_if_enabled(7, 0, 0);
        CHECK(*w == 0x20u, "clearing drops bit 8");
        *w = 0x10u;
        panel_item_flag8_set_if_enabled(7, 0, 1);
        CHECK(*w == 0x10u, "bit 16 alone is not the enable gate");
        panel_draw_records_set(7, NULL);

        /* Allocate: the stream, the size sums, the gather, the ids, the
         * three texture streams through the modeled seam. */
        crt_surface_calls_clear();
        const float stream[2][4] = { { 1.0f, 2.0f, 3.0f, 4.0f },
                                     { 5.0f, 6.0f, 7.0f, 8.0f } };
        const int slot[2] = { 1, 0 };
        const int ids[2] = { 0x111, 0x222 };
        const int t1[2] = { 7, 0 };
        const int t2[2] = { 0, 8 };
        const int t3[2] = { 9, 10 };
        panel_texture_slots_allocate(3, 2, slot, &stream[0][0], t1, t2, t3,
                                     ids);
        CHECK(panel_draw_count_peek(3) == 2, "the count lands");
        const char* blk = (const char*)panel_draw_records_peek(3);
        CHECK(blk != NULL, "the block lands");
        const float* r1 = (const float*)(blk + 0x2c);
        CHECK(r1[0] == 1.0f && r1[1] == 2.0f && r1[2] == 4.0f
                  && r1[3] == 6.0f,
              "the stream lands with the size sums");
        uint32_t bits = 0u;
        __builtin_memcpy(&bits, r1 + 8, 4);
        CHECK(bits == 0x111u, "the id carries bit for bit");
        __builtin_memcpy(&bits, r1 + 6, 4);
        CHECK(bits != 0u, "the first texture stream lands at +18");
        __builtin_memcpy(&bits, r1 + 4, 4);
        CHECK(bits == 0u, "the second stream leaves +16 clear");
        const float* r0 = (const float*)blk;
        __builtin_memcpy(&bits, r0 + 6, 4);
        CHECK(bits == 0u, "the absent second item first stream stays clear");
        __builtin_memcpy(&bits, r0 + 5, 4);
        CHECK(bits != 0u, "the third stream lands at +14");
        CHECK(crt_surface_calls_count() == 4, "the seam records the loads");
        int transforms = 0;
        for (int ci = 0; ci < crt_surface_calls_count(); ++ci) {
            uint32_t h; float cx, cy;
            crt_surface_call_get(ci, &h, &cx, &cy);
            (void)h;
            if (cx == 0.0f && cy == 0.0f) {
                ++transforms;
            }
        }
        CHECK(transforms == 4, "the loads land as transforms");
        panel_draw_records_set(3, NULL);
        panel_draw_count_set(3, 0);
        panel_texture_slots_allocate(3, 0, NULL, NULL, NULL, NULL, NULL,
                                     NULL); /* count zero: inert */
        CHECK(panel_draw_count_peek(3) == 0, "the zero count stays inert");

        /* The view blit: origin, the two rect records, the metrics finish. */
        static float rect_a[8] = { 1.0f, 2.0f, 0, 0, 5.0f, 6.0f, 0, 0 };
        static float rect_b[8] = { 3.0f, 3.0f, 0, 0, 0, 9.0f, 0, 0 };
        static float metrics[8] = { 1.0f, 1.0f, 0, 0, 1.0f, 1.0f, 3.0f, 0 };
        const uint32_t fl_a = 2u, fl_b = 4u;
        __builtin_memcpy(rect_a + 6, &fl_a, 4); /* bit 2, rec+16 set */
        __builtin_memcpy(rect_b + 6, &fl_b, 4); /* size-b pair path */
        indexed_pair_set(4, 100u, 50u);        /* the region offsets */
        indexed_float_pair_set(2, 10u, 20u);   /* the view's own pair */
        view_blit_window_set(2, 777u, rect_a, rect_b, 4);
        indexed_record_pointer_set(2, metrics);
        crt_surface_calls_clear();
        view_blit_rect_positions_apply(2);
        CHECK(crt_surface_calls_count() == 4, "the view walks handle, rects, metrics");
        uint32_t h = 0u;
        float x = 0, y = 0;
        crt_surface_call_get(0, &h, &x, &y);
        CHECK(h == 777u && x == 110.0f && y == 70.0f, "the view handle lands");
        crt_surface_call_get(1, &h, &x, &y);
        CHECK(h == 0x40A00000u && x == 111.0f && y == 72.0f,
              "the rect handle carries its size bits");
        crt_surface_call_get(3, &h, &x, &y);
        CHECK(h == 0x40400000u && x == 112.0f && y == 72.0f,
              "the metrics finish adds both offsets");
        view_blit_window_set(1, 1u, NULL, NULL, 100);
        crt_surface_calls_clear();
        view_blit_rect_positions_apply(1); /* region guard */
        view_blit_rect_positions_apply(-1);
        view_blit_rect_positions_apply(99);
        CHECK(crt_surface_calls_count() == 0, "the guards keep the seam quiet");
    }

    /* CrtNoOpStubVariant2 (2026-10-24): the empty body answers nothing. */
    {
        const int before = crt_surface_calls_count();
        crt_no_op_stub_variant2();
        CHECK(crt_surface_calls_count() == before,
              "the stub stays inert");
    }

    /* The linked-record chain free (2026-10-24): the null guard, the
     * single-node release, and the global clear. */
    {
        linked_record_chain_free(NULL); /* guarded */
        linked_list_global_head_set(NULL); /* start from a clean global
                                            * list: earlier blocks left
                                            * stack nodes behind */
        const size_t k = 0x28;
        char* solo = (char*)engine_heap_alloc(k);
        memset(solo, 0, k);
        linked_record_chain_free(solo); /* single node, freed in place */

        char* second = (char*)engine_heap_alloc(k);
        char* third = (char*)engine_heap_alloc(k);
        memset(second, 0, k);
        memset(third, 0, k);
        linked_list_push_at_global_head(second);
        linked_list_push_at_global_head(third); /* third links to second */
        linked_list_global_aux_set((void*)3);
        global_linked_record_chain_clear();
        CHECK(linked_list_global_head_peek() == NULL,
              "the clear empties the head");
        CHECK(linked_list_global_aux_peek() == NULL,
              "the aux word clears with it");
    }

    /* ReleaseLinkedObjectList (2026-10-24): owner free + chain recycle. */
    {
        void* prev = global_record_list_head_peek();
        char header[0x20];
        char node[0x20];
        memset(header, 0, sizeof header);
        memset(node, 0, sizeof node);
        void* next = node;
        __builtin_memcpy(header + 0x14, &next, sizeof next);
        linked_object_list_release(header);
        CHECK(global_record_list_head_peek() == (void*)node,
              "the chain recycles the last node to the head");
        void* link = NULL;
        __builtin_memcpy(&link, node + 0x1c, sizeof link);
        CHECK(link == (void*)header, "the recycled nodes chain through +0x1c");
        __builtin_memcpy(&link, header + 0x1c, sizeof link);
        CHECK(link == prev, "the chain tail reaches the prior head");
        linked_object_list_release(NULL); /* guarded */
    }

    /* ModelRenderListClear + CrtOwnedStreamBufferRelease (2026-10-24). */
    {
        char* solo = (char*)engine_heap_alloc(0x50);
        char* tail = (char*)engine_heap_alloc(0x50);
        memset(solo, 0, 0x50); /* the +0x44 terminator reads in-bounds */
        memset(tail, 0, 0x50);
        __builtin_memcpy(solo + 0x44, &tail, sizeof tail);
        model_render_list_head_set(solo);
        model_render_list_clear();
        CHECK(model_render_list_head_peek() == NULL, "the list head clears");
        model_render_list_clear(); /* the cleared head is inert */

        char rec[4 * sizeof(void*) + 4];
        memset(rec, 0, sizeof rec);
        void* buf = engine_heap_alloc(64);
        __builtin_memcpy(rec + 2 * sizeof(void*), &buf, sizeof buf);
        uint32_t flags = 0x88u;
        __builtin_memcpy(rec + 3 * sizeof(void*), &flags, sizeof flags);
        crt_owned_stream_buffer_release(rec);
        void* p = (void*)1;
        __builtin_memcpy(&p, rec + 2 * sizeof(void*), sizeof p);
        CHECK(p == NULL, "the owned buffer slot clears");
        __builtin_memcpy(&flags, rec + 3 * sizeof(void*), sizeof flags);
        CHECK(flags == 0x80u, "bits 3 and 11 clear, bit 7 survives");
        flags = 0x82u; /* bit 3 clear: the gate fails, record untouched */
        __builtin_memcpy(rec + 3 * sizeof(void*), &flags, sizeof flags);
        crt_owned_stream_buffer_release(rec);
        __builtin_memcpy(&flags, rec + 3 * sizeof(void*), sizeof flags);
        CHECK(flags == 0x82u, "the gate-false record survives intact");
        crt_owned_stream_buffer_release(NULL); /* guarded */
    }

    /* RendererCommandNodePrepend (2026-10-25): the node window, the kind
     * word, the payload copy, the head link — then the twin clear frees. */
    {
        model_render_list_head_set(NULL);
        uint32_t payload[16];
        for (int i = 0; i < 16; ++i) {
            payload[i] = 0x10u + (uint32_t)i;
        }
        renderer_command_node_prepend(7u, payload);
        void* one = model_render_list_head_peek();
        CHECK(one != NULL, "the prepend raises the head");
        uint32_t kind = 0u;
        __builtin_memcpy(&kind, (char*)one + 0x00, sizeof kind);
        CHECK(kind == 7u, "the kind word lands");
        int ok = 1;
        for (int i = 0; i < 16; ++i) {
            uint32_t w = 0u;
            __builtin_memcpy(&w, (char*)one + 0x04 + (size_t)i * 4u,
                             sizeof w);
            if (w != payload[i]) {
                ok = 0;
            }
        }
        CHECK(ok, "the sixteen payload dwords copy in");
        void* link = (void*)1;
        __builtin_memcpy(&link, (char*)one + 0x44, sizeof link);
        CHECK(link == NULL, "the first node terminates the chain");
        renderer_command_node_prepend(9u, NULL);
        void* two = model_render_list_head_peek();
        CHECK(two != NULL && two != one,
              "the second prepend raises the head");
        __builtin_memcpy(&link, (char*)two + 0x44, sizeof link);
        CHECK(link == one, "the new head links to the old one");
        uint32_t w0 = 0xdeadbeefu;
        __builtin_memcpy(&w0, (char*)two + 0x04, sizeof w0);
        CHECK(w0 == 0u, "the null payload leaves the fields zero");
        model_render_list_clear();
        CHECK(model_render_list_head_peek() == NULL,
              "the prepend chain clears through the widened walk");
    }

    /* SharedTextureDirectorySet (2026-10-24): dup + install. */
    {
        shared_texture_directory_clear();
        char path[] = "TEXTURES";
        shared_texture_directory_set(path);
        char* got = shared_texture_directory_peek();
        CHECK(got != NULL && got != path, "the directory is duplicated");
        CHECK(got != NULL && strcmp(got, "TEXTURES") == 0,
              "the duplicate keeps the path");
        path[1] = '\0'; /* the corpus's copy is independent */
        CHECK(shared_texture_directory_peek() != NULL
                  && strcmp(shared_texture_directory_peek(), "TEXTURES") == 0,
              "the source edit stays out of the copy");
        shared_texture_directory_clear();
        CHECK(shared_texture_directory_peek() == NULL, "the clear frees out");
        shared_texture_directory_set(NULL); /* guarded by the dup */
        CHECK(shared_texture_directory_peek() == NULL, "NULL is guarded");
    }

    /* SelectionMarkerResourcesRelease (2026-10-24): the table release. */
    {
        void* slots[3];
        for (int i = 0; i < 3; ++i) {
            slots[i] = engine_heap_alloc(8);
            CHECK(slots[i] != NULL, "the slot records allocate");
            selection_marker_slot_set(i, slots[i]);
        }
        void* desc = engine_heap_alloc(8);
        selection_marker_descriptor_set(desc);
        selection_marker_count_set(3u);
        selection_marker_resources_release();
        CHECK(selection_marker_slot_peek(0) == NULL
                  && selection_marker_slot_peek(2) == NULL,
              "the freed slots clear out");
        CHECK(selection_marker_count_peek() == 0u, "the count resets");
        CHECK(selection_marker_descriptor_peek() == NULL,
              "the descriptor frees out");
        selection_marker_resources_release(); /* double release: no-op */
    }

    /* OwnedResourceTreeRelease (2026-10-24): the payload + child release. */
    {
        void** child = (void**)engine_heap_alloc(3 * sizeof(void*));
        void** payload = (void**)engine_heap_alloc(8);
        child[0] = payload;
        child[1] = NULL;
        child[2] = NULL;
        void** payload2 = (void**)engine_heap_alloc(8);
        void** parent = (void**)engine_heap_alloc(3 * sizeof(void*));
        parent[0] = payload2;
        parent[1] = (void*)child;
        parent[2] = NULL;
        owned_resource_tree_release(parent, 0); /* node stays, tree frees */
        engine_heap_free(parent);
        owned_resource_tree_release(NULL, 1); /* guarded */
        void** leaf = (void**)engine_heap_alloc(3 * sizeof(void*));
        leaf[0] = engine_heap_alloc(8);
        leaf[1] = NULL;
        leaf[2] = engine_heap_alloc(8);
        owned_resource_tree_release(leaf, 1); /* payloads and node free */
        CHECK(1, "the release walk completes");
    }

    /* StringPrefixReserve (2026-10-24): the in-place shift. */
    {
        char buf[32];
        memset(buf, 0, sizeof buf);
        strcpy(buf, "abc");
        string_prefix_reserve(buf, 2);
        CHECK(strcmp(buf + 2, "abc") == 0, "the string lands after the gap");
        string_prefix_reserve(buf, 0);
        CHECK(strcmp(buf + 2, "abc") == 0, "a zero count leaves it alone");
        string_prefix_reserve(NULL, 2); /* guarded */
    }

    /* CrtStreamBufferAllocate (2026-10-24): the buffer install. */
    {
        /* Record window: seven field words plus the internal 0x1000 buffer
         * riding from field 5 — sized for the fallback shape (the heap path
         * lands a real allocation, so the window fields are what we pin). */
        static char rec[5 * sizeof(void*) + 0x1000];
        memset(rec, 0, sizeof rec);
        stream_allocation_count_reset();
        crt_stream_buffer_allocate(rec);
        void* buffer = NULL;
        __builtin_memcpy(&buffer, rec + 2 * sizeof(void*), sizeof buffer);
        uint32_t flags = 0u;
        __builtin_memcpy(&flags, rec + 3 * sizeof(void*), sizeof flags);
        void* front = NULL;
        __builtin_memcpy(&front, rec + 0 * sizeof(void*), sizeof front);
        CHECK(buffer != NULL && front == buffer,
              "the heap buffer becomes the stream window");
        CHECK((flags & 8u) != 0u && stream_allocation_count_peek() == 1,
              "the owned-buffer flag raises and the counter ticks");
        void* pos = NULL;
        __builtin_memcpy(&pos, rec + 1 * sizeof(void*), sizeof pos);
        CHECK(pos == NULL, "the window position starts clear");
        crt_stream_buffer_allocate(rec); /* overwrite leaks (faithful) */
        CHECK(stream_allocation_count_peek() == 2, "the second tick lands");
        crt_stream_buffer_allocate(NULL); /* guarded */
    }

    /* UInt96Add (2026-10-24): the three-limb carry chains. */
    {
        uint32_t dst[3], src[3];

        dst[0] = 0xFFFFFFFFu; dst[1] = 0xFFFFFFFFu; dst[2] = 5u;
        src[0] = 1u; src[1] = 0u; src[2] = 0u;
        crt_uint96_add(dst, src);
        CHECK(dst[0] == 0u && dst[1] == 0u && dst[2] == 6u,
              "the limb-0 double carry bumps limb two");

        dst[0] = 0u; dst[1] = 0xFFFFFFFFu; dst[2] = 0u;
        src[0] = 0u; src[1] = 1u; src[2] = 0u;
        crt_uint96_add(dst, src);
        CHECK(dst[0] == 0u && dst[1] == 0u && dst[2] == 1u,
              "the mid-limb carry bumps limb two");

        dst[0] = 1u; dst[1] = 1u; dst[2] = 0xFFFFFFFFu;
        src[0] = 1u; src[1] = 1u; src[2] = 1u;
        crt_uint96_add(dst, src);
        CHECK(dst[0] == 2u && dst[1] == 2u && dst[2] == 0u,
              "the high carry out is discarded");

        uint32_t same[3] = { 1u, 2u, 3u };
        crt_uint96_add(same, same);
        CHECK(same[0] == 2u && same[1] == 4u && same[2] == 6u,
              "the in-place double lands");

        crt_uint96_add(NULL, same); /* guarded */
        crt_uint96_add(same, NULL);
        CHECK(same[0] == 2u && same[1] == 4u && same[2] == 6u,
              "the guards leave the sum alone");
    }

    /* UInt96IncrementAtBit (2026-10-24): masks and the downward carry. */
    {
        uint32_t bits[3] = { 0u, 0u, 0u };
        crt_uint96_increment_at_bit(bits, 0);
        CHECK(bits[0] == 0x80000000u && bits[1] == 0u && bits[2] == 0u,
              "the bit-0 mask lands high in limb 0");
        crt_uint96_increment_at_bit(bits, 95);
        CHECK(bits[2] == 1u, "the last bit lands low in limb 2");
        crt_uint96_increment_at_bit(bits, 63);
        CHECK(bits[1] == 1u, "the bit-63 mask rides limb 1");
        bits[2] = 0xffffffffu;
        crt_uint96_increment_at_bit(bits, 95);
        CHECK(bits[2] == 0u && bits[1] == 2u,
              "the limb-2 overflow carries +1 downward");
        crt_uint96_increment_at_bit(bits, 200);
        CHECK(bits[0] == 0x80000000u && bits[1] == 2u && bits[2] == 0u,
              "the out-of-range limb guard stays quiet (deviation)");
    }

    /* UInt96HasNoBitsAtOrAbove (FUN_00497900) + UInt96ClearFromBit
     * (FUN_004979e0) — corpus MSB-first numbering: bit n sits in limb
     * n / 32 at value bit 31 - n % 32. */
    {
        uint32_t vA[3] = { 1u, 0u, 0u }; /* bit 31 only */
        CHECK(crt_uint96_has_no_bits_at_or_above(vA, 31) == 1u,
              "nothing lives above bit 31 in the first limb");
        CHECK(crt_uint96_has_no_bits_at_or_above(vA, 30) == 0u,
              "bit 31 is above bit 30");
        CHECK(crt_uint96_has_no_bits_at_or_above(vA, 32) == 1u,
              "the second limb starts empty");
        CHECK(crt_uint96_has_no_bits_at_or_above(vA, 96) == 1u,
              "the out-of-range limb guard reads all-clear (deviation)");
        CHECK(crt_uint96_has_no_bits_at_or_above(NULL, 5) == 1u,
              "the NULL limb guard reads all-clear (deviation)");

        uint32_t vB[3] = { 0x80000002u, 0u, 0u }; /* bits 0 + 31 */
        CHECK(crt_uint96_clear_from_bit(vB, 0) == 0u
                  && vB[0] == 0x80000000u && vB[1] == 0u && vB[2] == 0u,
              "the increment branch bumps bit - 1, keeps the at-or-above "
              "mask, and the corpus's undefined return is pinned 0");
        uint32_t vC[3] = { 1u, 0u, 0u };
        CHECK(crt_uint96_clear_from_bit(vC, 31) == 0u
                  && vC[0] == 1u && vC[1] == 0u && vC[2] == 0u,
              "a top-of-limb bit survives its own clear (the mask keeps "
              "the at-or-above half)");
        uint32_t vD[3] = { 0u, 1u, 1u }; /* bits 63 + 95 */
        CHECK(crt_uint96_clear_from_bit(vD, 63) == 0u
                  && vD[0] == 0u && vD[1] == 3u && vD[2] == 0u,
              "the bit-62 bump lands in limb 1 and limb 2 zeroes out");
        uint32_t vE[3] = { 0xC0000000u, 0u, 0u }; /* bits 0 + 1 */
        CHECK(crt_uint96_clear_from_bit(vE, 1) == 0u
                  && vE[0] == 0xC0000000u && vE[1] == 0u && vE[2] == 0u,
              "clear-from-bit-1 erases bits 2 up but keeps 0 and 1");
    }

    /* The renderer flag word + scene hook band (2026-10-25). */
    {
        renderer_flag_bit6_set(1);
        CHECK((renderer_flag_word_peek() & 0x40u) != 0u,
              "the flag fold raises bit 6");
        renderer_flag_bit6_set(1); /* idempotent */
        CHECK((renderer_flag_word_peek() & 0x40u) != 0u,
              "the second set leaves bit 6 raised");
        renderer_flag_bit6_set(0);
        CHECK((renderer_flag_word_peek() & 0x40u) == 0u,
              "the flag fold clears bit 6");

        char end_obj = 0, end_val = 0, sample_obj = 0, sample_val = 0;
        scene_end_hook_register(&end_obj, &end_val);
        CHECK(scene_end_hook_peek() == &end_obj
                  && scene_end_hook_value_peek() == &end_val,
              "the end hook stores its pair");
        renderer_flag_bit6_set(0);
        scene_sample_hook_register(&sample_obj, &sample_val);
        CHECK(scene_sample_hook_peek() == &sample_obj
                  && scene_sample_hook_value_peek() == &sample_val
                  && (renderer_flag_word_peek() & 0x40u) != 0u,
              "the sample hook stores its pair and raises bit 6");
        renderer_flag_bit6_set(0); /* return the word to its zero-init state */
    }

    /* The string registry pair (2026-10-25): intern + case-insensitive
     * find over the module-static chain. */
    {
        string_registry_reset();
        CHECK(string_registry_intern(NULL) == NULL,
              "the null key answers null");
        CHECK(string_registry_intern("") == NULL,
              "the empty key answers null");
        char key_buf[] = "MENUITEM";
        void* entry = string_registry_intern(key_buf);
        CHECK(entry != NULL, "the miss allocates an entry");
        key_buf[1] = '\0'; /* the copy is independent */
        CHECK(string_keyed_cache_entry_find("menuitem") == entry
                  && string_keyed_cache_entry_find("MENUITEM") == entry,
              "the find folds case both ways");
        CHECK(string_registry_intern("MenuItem") == entry,
              "the known key re-answers its entry");
        CHECK(string_keyed_cache_entry_find("other") == NULL,
              "the unknown key answers null");
        void* second = string_registry_intern("OTHER");
        CHECK(second != NULL && second != entry, "the second miss appends");
        void* next_link = NULL; /* widened entry: field 3 is the next link */
        __builtin_memcpy(&next_link, (char*)second + 3 * sizeof(void*),
                         sizeof next_link);
        CHECK(next_link == entry, "the append links the new entry at the head");
        string_registry_reset();
        CHECK(string_keyed_cache_entry_find("MENUITEM") == NULL
                  && string_registry_intern("MENUITEM") != NULL,
              "the reset empties the chain and intern restarts");
        string_registry_reset();
    }

    /* The entry cache get-or-load pair (2026-10-25): loader seams ride
     * hooks; uninstalled the load answers NULL. */
    {
        string_registry_reset();
        s_bmp_loads = 0;
        s_font_loads = 0;
        CHECK(string_cache_lookup_or_load("KEY1") == NULL
                  && resource_name_cache_get_or_load("KEY1") == NULL,
              "the uninstalled seams answer NULL");
        string_cache_bitmap_loader_set(s_bmp_loader);
        void* v = string_cache_lookup_or_load("KEY1");
        CHECK(v == (void*)0xb0 && s_bmp_loads == 1,
              "the loader runs once on the bitmap miss");
        v = string_cache_lookup_or_load("key1");
        CHECK(v == (void*)0xb0 && s_bmp_loads == 1,
              "the hit answers from the slot without reloading");
        string_cache_font_loader_set(s_font_loader);
        CHECK(resource_name_cache_get_or_load("KEY1") == (void*)0xf0
                  && s_font_loads == 1, "the font slot rides the font seam");
        CHECK(resource_name_cache_get_or_load("KEY1") == (void*)0xf0
                  && s_font_loads == 1, "the font slot caches too");
        string_registry_reset(); /* the slots drop with the entry */
        CHECK(string_cache_lookup_or_load("KEY1") == (void*)0xb0
                  && s_bmp_loads == 2, "the post-reset intern reloads");
        string_cache_bitmap_loader_set(NULL);
        string_cache_font_loader_set(NULL);
        string_registry_reset();
        CHECK(string_cache_lookup_or_load("KEY1") == NULL,
              "the reset entries answer null again");
    }

    /* The pair-table window pair (2026-10-25). */
    {
        static void* window[16];
        memset(window, 0, sizeof window);
        int count = 0;
        CHECK(pair_table_find(window, 0u, "key") == NULL,
              "the empty window answers null");
        char key1[] = "FIRSTKEY";
        char key2[] = "SECONDKEY";
        pair_table_append(window, &count, key1, (void*)7);
        pair_table_append(window, &count, key2, (void*)9);
        CHECK(count == 2, "the appends bump the count");
        CHECK(pair_table_find(window, 2u, "firstkey") == (void*)7
                  && pair_table_find(window, 2u, "SECONDKEY") == (void*)9,
              "the fold-find answers the paired values");
        CHECK(pair_table_find(window, 2u, "nothere") == NULL,
              "the unknown key answers null");
        char* name = NULL;
        __builtin_memcpy(&name, (void* const*)window, sizeof name);
        CHECK(name != NULL && name != key1
                  && strcmp(name, "FIRSTKEY") == 0,
              "the append duplicates the key");
        pair_table_append(window, &count, key1, (void*)5); /* key re-append */
        CHECK(count == 3 && pair_table_find(window, 3u, "firstkey") == (void*)7,
              "the find stays first-wins past the re-append");
        CHECK(pair_table_find(NULL, 1u, "x") == NULL
                  && pair_table_find(window, 1u, NULL) == NULL,
              "the window and key guards hold");
    }

    /* Renderer-word band (2026-10-25): zero-init statics with the
     * install hooks. */
    {
        CHECK(renderer_word_a_get() == 0u && renderer_word_b_get() == 0u
                  && renderer_word_c_get() == 0u
                  && renderer_global_dword_get() == 0u
                  && render_surface_secondary_get() == 0u
                  && render_feature_flags_get() == 0u,
              "the renderer words start zero");
        renderer_words_set(1u, 2u, 3u);
        renderer_global_dword_set(0xdeadu);
        render_surface_secondary_set(7u);
        render_feature_flags_set(8u);
        CHECK(renderer_word_a_get() == 1u && renderer_word_b_get() == 2u
                  && renderer_word_c_get() == 3u
                  && renderer_global_dword_get() == 0xdeadu
                  && render_surface_secondary_get() == 7u
                  && render_feature_flags_get() == 8u,
              "the getters answer their installed halves");
    }

    /* Engine feature word band (2026-10-25): corpus half-store. */
    {
        engine_feature_word_install(0u);
        low_texture_flag_set();
        CHECK(engine_feature_word_peek() == 0x20u,
              "the setter turns on bit 5 while bit 0x40000 is clear");
        engine_feature_word_install(0x120u);
        low_texture_flag_set();
        CHECK(engine_feature_word_peek() == 0x120u,
              "the already-set half stays");
        engine_feature_word_install(0x40020u);
        low_texture_flag_set();
        CHECK(engine_feature_word_peek() == 0x40020u,
              "the gated half skips the store");
        engine_feature_word_install(0u);
    }

    /* Integer/string helper band (2026-10-25): the uint96 dword copy, the
     * allocator duplicate, the radix digits, and the ASCII folds. */
    {
        const uint32_t src[3] = { 0x11u, 0x22u, 0x33u };
        uint32_t dst[3] = { 0, 0, 0 };
        uint96_copy(dst, src);
        CHECK(dst[0] == 0x11u && dst[1] == 0x22u && dst[2] == 0x33u,
              "the three dwords copy in order");
        /* Corpus overlap quirk: dst = src + 1 word, forward walk — the
         * second and third words read the already-written ones. */
        uint32_t mix[3] = { 7u, 8u, 9u };
        uint96_copy(mix + 1, mix);
        CHECK(mix[1] == 7u && mix[2] == 7u,
              "the overlap copies word-by-word in the corpus's order");

        char* dup = crt_string_duplicate("ab");
        CHECK(dup != NULL && strcmp(dup, "ab") == 0, "the duplicate lands");
        if (dup != NULL) {
            engine_heap_free(dup);
        }
        CHECK(crt_string_duplicate(NULL) == NULL, "the null duplicate is guarded");

        char buf[32];
        crt_unsigned_format_radix(123u, buf, 10, 0);
        CHECK(strcmp(buf, "123") == 0, "the decimal digits reverse into place");
        crt_unsigned_format_radix(0u, buf, 10, 0);
        CHECK(strcmp(buf, "0") == 0, "zero writes one digit");
        crt_unsigned_format_radix(255u, buf, 16, 0);
        CHECK(strcmp(buf, "ff") == 0, "digits past nine fold lowercase");
        crt_unsigned_format_radix(4294967295u, buf, 16, 0);
        CHECK(strcmp(buf, "ffffffff") == 0, "the full dword spans eight digits");
        crt_unsigned_format_radix(9u, buf, 1, 0);
        CHECK(strcmp(buf, "0") == 0, "the radix guard answers zero (deviation)");

        crt_signed_to_string(-42, buf, 10);
        CHECK(strcmp(buf, "-42") == 0, "the negative decimal takes the sign half");
        crt_signed_to_string(-2147483648, buf, 10);
        CHECK(strcmp(buf, "-2147483648") == 0,
              "INT_MIN rides the corpus's wrapping magnitude");
        crt_signed_to_string(42, buf, 16);
        CHECK(strcmp(buf, "2a") == 0, "the positive half folds lowercase");
        CHECK(crt_signed_to_string(7, buf, 10) == buf, "the buffer answers");

        char ascii[] = "Hello World123";
        CHECK(crt_string_to_upper(ascii) == ascii
                  && strcmp(ascii, "HELLO WORLD123") == 0,
              "the uppercase fold runs the whole string");
        crt_string_to_lower(ascii);
        CHECK(strcmp(ascii, "hello world123") == 0, "the lowercase fold follows");
        char empty[] = "";
        CHECK(crt_string_to_upper(empty) == empty, "the empty string returns");
        CHECK(crt_string_to_upper(NULL) == NULL
                  && crt_string_to_lower(NULL) == NULL,
              "the fold guards answer null (deviation)");
    }

    /* Math error band (2026-10-25): the error-code storage and the
     * exception-code status fold. */
    {
        crt_math_error_code_set(5);
        CHECK(crt_math_error_code_peek() == 0u, "an untouched param stays");
        crt_math_error_code_set(1);
        CHECK(crt_math_error_code_peek() == 0x21u, "param one stores 33");
        crt_math_error_code_set(2);
        CHECK(crt_math_error_code_peek() == 0x22u, "param two stores 34");
        crt_math_error_code_set(3);
        CHECK(crt_math_error_code_peek() == 0x22u, "param three re-stores");
        crt_math_error_code_set(0);
        CHECK(crt_math_error_code_peek() == 0x22u, "the zero rides past");

        CHECK(crt_math_exception_code_from_status(0x20u) == 5,
              "the 0x20 field answers five");
        CHECK(crt_math_exception_code_from_status(0x28u) == 5,
              "the 0x20 field outranks the eight");
        CHECK(crt_math_exception_code_from_status(8u) == 1, "bit 8 answers one");
        CHECK(crt_math_exception_code_from_status(4u) == 2, "bit 4 answers two");
        CHECK(crt_math_exception_code_from_status(1u) == 3, "bit 1 answers three");
        CHECK(crt_math_exception_code_from_status(2u) == 4,
              "the lone bit 2 doubles to four");
        CHECK(crt_math_exception_code_from_status(0u) == 0,
              "the clean status answers zero");

        global_flag_004ff978_install(0x123u);
        CHECK(global_flag_004ff978_get() == 0x123u, "the install hook lands");
        global_flag_004ff978_install(0u);
    }

    /* Handler table + 64-bit multiply thunk (2026-10-25). */
    {
        crt_math_exception_handler_install(3, 77, 0xa1a2);
        CHECK(crt_math_exception_handler_get(77) == 0xa1a2u,
              "the id match answers the handler");
        CHECK(crt_math_exception_handler_get(78) == 0u,
              "an unmatched id answers zero");
        crt_math_exception_handler_install(99, 78, 0xbeef);
        CHECK(crt_math_exception_handler_get(78) == 0u,
              "the guarded window drops the out-of-range install (dev)");
        crt_math_exception_handler_install(3, 77, 0u);
    }

    {
        CHECK(crt_integer64_multiply(3, 0, 5, 0) == 15ull,
              "the plain word multiply lands");
        CHECK(crt_integer64_multiply(0, 1, 2, 0) == 0x200000000ull,
              "the high-word fold scales (2^32 x 2)");
        CHECK(crt_integer64_multiply(1, -1, 1, 0) == 0xffffffff00000001ull,
              "the signed cross-word half folds the bits");
        CHECK(crt_integer64_multiply(0xffffffffu, 0, 2, 0) == 0x1fffffffeull,
              "the wide unsigned pair stays unsigned");
        CHECK(crt_integer64_multiply(2, 0, 3, 1)
                  == (uint64_t)6 + ((uint64_t)2 << 32),
              "hi_b contributes lo_a*hi_b");
        CHECK(crt_integer64_multiply(3, 3, 5, 2)
                  == (((uint64_t)(15u + 6u) << 32) | 15u),
              "hi_a*lo_b + lo_a*hi_b fold");
    }

    /* CrtInteger64Divide / CrtUnsigned64Divide (FUN_0048e9e0 /
     * FUN_00498bf0; 2026-10-25): the word-pair division helpers fold to
     * the LP64 division, truncating toward zero. */
    {
        CHECK(crt_integer64_divide(7u, 0u, 2u, 0u) == 3ull,
              "the plain signed pair divides");
        CHECK(crt_integer64_divide(7u, 0u, 0xfffffffeu, 0xffffffffu)
                  == 0xfffffffffffffffdull,
              "the negative divisor truncates toward zero (-7/2)");
        CHECK(crt_integer64_divide(0u, 0x80000000u, 0u, 0x80000000u) == 1ull,
              "the equal extremes divide to one");
        CHECK(crt_integer64_divide(0u, 0x80000000u, 0xffffffffu, 0xffffffffu)
                  == 0x8000000000000000ull,
              "the INT64_MIN / -1 quotient keeps the wrap quirk "
              "(the double normalization never negates)");
        CHECK(crt_integer64_divide(0u, 0x80000000u, 1u, 0u)
                  == 0x8000000000000000ull,
              "INT64_MIN over one rides through unchanged");
        CHECK(crt_integer64_divide(1u, 1u, 0u, 0u) == 0ull,
              "the zero divisor answers zero (deviation)");

        CHECK(crt_unsigned64_divide(7u, 0u, 2u, 0u) == 3ull,
              "the plain unsigned pair divides");
        CHECK(crt_unsigned64_divide(0u, 0x80000000u, 0u, 2u) == 0x40000000ull,
              "the high-word shift scales (2^32 / 2)");
        CHECK(crt_unsigned64_divide(0xffffffffu, 1u, 2u, 0u) == 0xffffffffull,
              "the pair fold answers the scaled quotient");
        CHECK(crt_unsigned64_divide(2u, 1u, 0u, 1u) == 1ull,
              "the pair-scale division answers with the pair quotient");
        CHECK(crt_unsigned64_divide(5u, 0u, 0u, 0u) == 0ull,
              "the zero divisor answers zero (deviation)");
    }

    /* The file-descriptor flag window (2026-10-25): 32 eight-byte entries
     * per block, the flag byte at entry+4, bit 0x40, count-bounded. */
    {
        static unsigned char block0[32 * 8];
        static unsigned char block1[32 * 8];
        memset(block0, 0, sizeof block0);
        memset(block1, 0, sizeof block1);
        block0[7 * 8u + 4u] = 0x40u;           /* fd 7 set */
        block0[8 * 8u + 4u] = 0x20u;           /* fd 8: other bit */
        block1[(1 * 8u) + 4u] = 0x40u;         /* fd 33 set */
        crt_file_descriptor_flag_block_set(0, block0);
        crt_file_descriptor_flag_block_set(1, block1);
        crt_file_descriptor_flag_block_set(63, block1);
        crt_file_descriptor_flag_block_set(64, block1); /* guarded (dev) */
        crt_file_descriptor_count_set(64);
        CHECK(crt_file_descriptor_count_peek() == 64,
              "the count hook round-trips");
        CHECK(crt_file_descriptor_flag_block_peek(1) == block1,
              "the block hook round-trips");
        CHECK(crt_file_descriptor_flag_block_peek(63) == block1,
              "the last block slot is in range");
        CHECK(crt_file_descriptor_flag_block_peek(64) == NULL,
              "the block hook guards the first out-of-range slot (dev)");
        CHECK(crt_file_descriptor_flag_block_peek(-1) == NULL,
              "the guarded peek answers null (dev)");
        CHECK(crt_file_descriptor_flag40_get(7u) == 1u,
              "the flag byte answers the bit");
        CHECK(crt_file_descriptor_flag40_get(8u) == 0u,
              "the other bit answers clear");
        CHECK(crt_file_descriptor_flag40_get(33u) == 1u,
              "the second block answers");
        CHECK(crt_file_descriptor_flag40_get(64u) == 0u,
              "the count bound answers clear");
        crt_file_descriptor_count_set(4);
        CHECK(crt_file_descriptor_flag40_get(7u) == 0u,
              "the shrunk count bounds the lookup");

        /* the null-block and uninstalled-window deviations */
        crt_file_descriptor_count_set(64);
        CHECK(crt_file_descriptor_flag40_get(5u) == 0u,
              "the flagless entry answers clear");
        crt_file_descriptor_flag_block_set(1, NULL);
        CHECK(crt_file_descriptor_flag40_get(33u) == 0u,
              "the null block answers clear (dev)");

        /* leave the window clean */
        crt_file_descriptor_flag_block_set(0, NULL);
        crt_file_descriptor_flag_block_set(1, NULL);
        crt_file_descriptor_flag_block_set(63, NULL);
        crt_file_descriptor_count_set(0);
        CHECK(crt_file_descriptor_flag40_get(0u) == 0u,
              "the zero count answers clear");
    }

    /* The CRT control-flags / signal leaves (2026-10-25): the bit fold,
     * the pinned-locale wide-half, and the signal-record hunt. */
    {
        CHECK(crt_control_flags_translate(1u) == 0x20010u,
              "bit 0 crosses over (pair-clear rides)");
        CHECK(crt_control_flags_translate(2u) == 0x0a0000u,
              "bit 1 pairs high");
        CHECK(crt_control_flags_translate(4u) == 0x20008u,
              "bit 2 crosses over");
        CHECK(crt_control_flags_translate(8u) == 0x20004u,
              "bit 3 crosses over");
        CHECK(crt_control_flags_translate(0x10u) == 0x20002u,
              "bit 4 crosses over");
        CHECK(crt_control_flags_translate(0x20u) == 0x20001u,
              "bit 5 crosses over");
        CHECK(crt_control_flags_translate(0x3fu) == 0x0a001fu,
              "the low six fold together");
        CHECK(crt_control_flags_translate(0u) == 0x20000u,
              "the empty fold answers the pair-clear word");
        CHECK(crt_control_flags_translate(0x200u) == 0x10000u,
              "the half pair answers 0x10000");
        CHECK(crt_control_flags_translate(0x100u) == 0u,
              "the stray pair bit answers nothing");
        CHECK(crt_control_flags_translate(0x400u) == 0x20100u,
              "the pair-2 fold lands 0x100+pair-clear");
        CHECK(crt_control_flags_translate(0x800u) == 0x20200u,
              "the pair-2 fold lands 0x200+pair-clear");
        CHECK(crt_control_flags_translate(0xc00u) == 0x20300u,
              "the pair-2 fold lands 0x300+pair-clear");
        CHECK(crt_control_flags_translate(0x1000u) == 0x60000u,
              "bit 12 rides with the pair-clear");

        static char marker_a;
        static char marker_b;
        char out = 'Z';
        CHECK(crt_wide_char_to_multibyte(NULL, 0x41u) == 0,
              "the null destination answers 0");
        CHECK(crt_wide_char_to_multibyte(&out, 0x62u) == 1 && out == 'b',
              "the pinned C-locale half writes the low byte");
        out = 'Z';
        CHECK(crt_wide_char_to_multibyte(&out, 0x10100u) == -1 && out == 'Z',
              "the truncated half silences the store");
        CHECK(crt_math_error_code_peek() == 0x2au,
              "the failure half stores the EILSEQ-shaped slot");

        /* CrtStrncpy / CrtWideCharToMultiByteCompat (FUN_0048e770 /
         * FUN_0049abe0; 2026-10-25): the copy fold, the padded window,
         * and the pinned C-locale conversion lanes. */
        {
            char buf[16];
            memset(buf, 0x5a, sizeof buf);
            crt_string_copy_n(buf, "abc", 6u);
            CHECK(memcmp(buf, "abc\0\0\0", 6) == 0
                      && (unsigned char)buf[6] == 0x5au,
                  "the NUL-padded window copies and pads (2026-10-25)");
            memset(buf, 0x5a, sizeof buf);
            crt_string_copy_n(buf, "abcdefghij", 5u);
            CHECK(memcmp(buf, "abcde", 5) == 0
                      && (unsigned char)buf[5] == 0x5au,
                  "the expired count never lands a NUL");
            CHECK(crt_string_copy_n(buf, "x", 0u) == buf && buf[0] == 'a',
                  "the zero count answers the destination untouched");
            memset(buf, 0x5a, sizeof buf);
            crt_string_copy_n(buf, "mid", 5u);
            CHECK(memcmp(buf, "mid\0\0", 5) == 0,
                  "the exact-length copy still pads its NUL");
        }

        {
            static const uint16_t ascii_wide[] = {'h', 'i', 0};
            static const uint16_t far_wide[] = {0x120, 0x41, 0};
            static const uint16_t long_wide[] = {'a', 'b', 'c', 'd', 0};
            char dest[8];
            memset(dest, 0x5a, sizeof dest);
            CHECK(crt_wide_char_to_multi_byte_compat(NULL, ascii_wide,
                                                     8u) == 2,
                  "the null destination answers the wide length");
            CHECK(crt_wide_char_to_multi_byte_compat(dest, ascii_wide, 0u)
                      == 0 && dest[0] == 0x5a,
                  "the zero count stays silent (deviation-free lane)");
            CHECK(crt_wide_char_to_multi_byte_compat(dest, ascii_wide, 8u)
                      == 2 && dest[0] == 'h' && dest[1] == 'i',
                  "the store lane copies to the NUL and answers the len");
            memset(dest, 0x5a, sizeof dest);
            CHECK(crt_wide_char_to_multi_byte_compat(dest, long_wide, 3u)
                      == 3 && dest[0] == 'a' && dest[2] == 'c',
                  "the bounded lane answers the count");
            memset(dest, 0x5a, sizeof dest);
            CHECK(crt_wide_char_to_multi_byte_compat(dest, far_wide, 8u)
                      == -1 && (unsigned char)dest[0] == 0x5au,
                  "the over-0xff char silences and errors (2026-10-25)");
            CHECK(crt_math_error_code_peek() == 0x2au,
                  "the compat half stores the EILSEQ-shaped slot");
        }

        crt_signal_record_install(0, 0xaau, 7, &marker_a);
        crt_signal_record_install(1, 0u, 9, &marker_b);
        crt_signal_record_install(16, 0u, 3, NULL); /* guarded (dev) */
        crt_signal_record_count_set(2);
        CHECK(crt_signal_record_count_peek() == 2, "the count hook lands");
        void* hit7 = crt_signal_handler_record_find(7);
        void* hit9 = crt_signal_handler_record_find(9);
        CHECK(hit7 != NULL && hit9 != NULL && hit7 != hit9,
              "the record hunt answers the entries");
        CHECK(crt_signal_handler_record_find(11) == NULL,
              "the miss answers null");
        crt_signal_record_count_set(1);
        CHECK(crt_signal_handler_record_find(9) == NULL,
              "the count bounds the hunt");
        CHECK(crt_signal_handler_record_find(7) != NULL,
              "the entry-zero hit rides the bound");
        crt_signal_record_count_set(0);
        CHECK(crt_signal_handler_record_find(7) == NULL,
              "the empty table bounds even the entry-zero hit");
        uint32_t w = 0u;
        int id = 0;
        const void* h = NULL;
        crt_signal_record_install(0, 0u, 0, NULL); /* leave clean */
        crt_signal_record_install(1, 0u, 0, NULL);
        crt_signal_record_peek(0, &w, &id, &h);
        CHECK(w == 0u && id == 0 && h == NULL, "the clean peek rides");
    }

    if (s_failures != 0) {
        fprintf(stderr, "crt_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("crt_test: OK\n");
    return 0;
}
