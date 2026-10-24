// SPDX-License-Identifier: LGPL-3.0-only
// CRT/compiler-runtime leaves (findings 260/381/393/394): memmove, the
// __ftol float->int thunk and the callable no-op.
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "gods98/crt.hpp"
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

    if (s_failures != 0) {
        fprintf(stderr, "crt_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("crt_test: OK\n");
    return 0;
}
