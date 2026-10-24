// SPDX-License-Identifier: LGPL-3.0-only
#include <math.h>
#include <string.h>

#include "crt.hpp"

/* FUN_00498000: the corpus's memmove with word loops and byte tails —
 * byte-equivalent to the host memmove (findings 260/394). */
void* crt_memory_move(void* dest, const void* src, size_t count)
{
    return memmove(dest, src, count);
}

/* FUN_00417430: __ftol under the truncate-toward-zero control word the
 * compiler switches to — a C++ cast (findings 381/393). Overflow values
 * are outside the corpus's caller set (deviation in crt.hpp). */
int float_to_long(float value)
{
    return (int)value;
}

/* FUN_0041f8e0: empty body; the corpus's callable no-op (finding 381). */
void noop_callback(void)
{
}

/* FUN_0047d3f0: popcount via the clear-lowest-set-bit loop. */
int bit_count32(uint32_t value)
{
    int n = 0;
    while (value != 0u) {
        value &= value - 1u;
        ++n;
    }
    return n;
}

/* FUN_0047d700: popcount via the per-position test loop. */
int pop_count32(uint32_t value)
{
    int n = 0;
    for (unsigned i = 0; i < 32u; ++i) {
        if ((value & (1u << (i & 31u))) != 0u) {
            ++n;
        }
    }
    return n;
}

/* FUN_0047d720: index of the lowest set bit, 0xffffffff for zero. */
uint32_t find_first_set_bit32(uint32_t value)
{
    for (uint32_t i = 0; i < 32u; ++i) {
        if ((value & (1u << (i & 31u))) != 0u) {
            return i;
        }
    }
    return 0xffffffffu;
}

/* FUN_0047e7e0: the corpus's second identical position-loop body. */
int bit_count32_by_position(uint32_t value)
{
    return pop_count32(value);
}

/* FUN_00497ab0: zero over exactly 3 dwords. */
int uint96_is_zero(const uint32_t limbs[3])
{
    return limbs[0] == 0u && limbs[1] == 0u && limbs[2] == 0u;
}

/* FUN_0048e870: the double's exponent field not all-ones. */
int double_is_finite(double value)
{
    uint64_t bits;
    __builtin_memcpy(&bits, &value, sizeof bits);
    return ((bits >> 52u) & 0x7ffu) != 0x7ffu;
}

/* FUN_0045b900: the corpus casts the int pair and the divisor to float
 * before both divides (the results land in a caller float pair). */
void integer_pair_divide_to_float_pair(int x, int y, int divisor,
                                       float* out_x, float* out_y)
{
    *out_x = (float)x / (float)divisor;
    *out_y = (float)y / (float)divisor;
}

/* FUN_0045b930: the compiler's two-shot __ftol — each float truncates
 * toward zero into its int output (same deviation as float_to_long). */
void float_pair_to_integer_pair(float x, float y, int* out_x, int* out_y)
{
    *out_x = (int)x;
    *out_y = (int)y;
}

/* FUN_00499fb0: the corpus's carry test — the sum below either operand
 * means the addition carried out of the 32-bit result. */
int crt_unsigned_add_with_carry(uint32_t a, uint32_t b, uint32_t* sum)
{
    const uint32_t s = a + b;
    *sum = s;
    return (s < a || s < b) ? 1 : 0;
}

/* FUN_00493a80: the exponent/quiet-bit pattern classifier over the two
 * bit words, corpus masks verbatim (finding 399). The corpus masks the
 * high word's top 16 bits with 0x7ff8 — exponent plus quiet bit. */
int double_classify(uint32_t lo, uint32_t hi)
{
    const uint32_t exp_quiet = (hi >> 16) & 0x7ff8u;
    if (hi == 0x7ff00000u && lo == 0u) {
        return 1;                                   /* +infinity */
    }
    if (hi == 0xfff00000u && lo == 0u) {
        return 2;                                   /* -infinity */
    }
    if (exp_quiet == 0x7ff8u) {
        return 3;                                   /* quiet-NaN-shaped */
    }
    if (exp_quiet == 0x7ff0u && ((hi & 0x7ffffu) != 0u || lo != 0u)) {
        return 4;                                   /* signaling-NaN-shaped */
    }
    return 0;                                       /* finite */
}

/* FUN_00499710: the 16-bit strlen — code units counted up to the NUL. */
int wide_string_length(const uint16_t* text)
{
    const uint16_t* p = text;
    while (*p != 0) {
        ++p;
    }
    return (int)(p - text);
}

/* FUN_0049add0: bounded scan — the terminator index plus one when found
 * within `bound` (count including the NUL), the bound otherwise; a zero
 * bound returns the bound without reading. */
int bounded_wide_string_length_including_null(const uint16_t* text,
                                              int bound)
{
    if (bound != 0) {
        const uint16_t* p = text;
        int left = bound;
        while (*p != 0) {
            ++p;
            --left;
            if (left == 0) {
                break;
            }
        }
        if (left != 0 && *p == 0) {
            return (int)(p - text) + 1;
        }
    }
    return bound;
}

/* DoubleExponentBitsGetOrSpecialValue (FUN_00492068, findings 386/399):
 * exponent field for ordinary values; the untouched high word when the
 * exponent is all ones (Inf/NaN) so the caller sees the full pattern.
 * The corpus takes the double's high word as its only live argument
 * (the first parameter is the unused low word). */
uint32_t double_exponent_bits_or_special(uint32_t high_word)
{
    if ((high_word & 0x7ff00000u) != 0x7ff00000u) {
        return high_word & 0x7ff00000u;
    }
    return high_word;
}

/* FormatArgumentReadDword (FUN_00491430, finding 395): read the dword
 * at the cursor, then advance it by four. */
uint32_t format_argument_read_dword(uint8_t** cursor)
{
    uint32_t v;
    memcpy(&v, *cursor, sizeof(v));
    *cursor = *cursor + 4;
    return v;
}

/* FormatArgumentReadWord (FUN_00491470, finding 395): read the word,
 * advance by two, return zero-extended (the corpus's CONCAT of the
 * advanced pointer's high half — zero for 32-bit cursors). */
uint32_t format_argument_read_word(uint8_t** cursor)
{
    uint16_t v;
    memcpy(&v, *cursor, sizeof(v));
    *cursor = *cursor + 2;
    return v;
}

/* FormatArgumentReadDouble (FUN_00491450, finding 395): read eight
 * bytes, advance by eight. */
double format_argument_read_double(uint8_t** cursor)
{
    double v;
    memcpy(&v, *cursor, sizeof(v));
    *cursor = *cursor + 8;
    return v;
}

/* X87RoundToIntegral (FUN_00493150, finding 379): FRNDINT under the
 * default control word — nearest-even integral, still floating.
 * rint() matches it exactly (and keeps -0, per the x87 answer). */
double x87_round_to_integral(double value)
{
    return rint(value);
}

/* DoubleBuildWithExponent (FUN_00493a40, findings 383/399): the corpus
 * builds the floating value from two significand words and the
 * exponent-bias input; Ghidra renders the recipe as CONCAT chains,
 * which decompose exactly to: top word = ((high >> 16) & 0x800f) |
 * ((exponent + 0x3fe) << 4), and the 16-bit halves of `high` plus `low`
 * under it. The sign bit and the fraction nibble of `high` survive the
 * mask, the 16-bit ops truncate like the corpus's, and the assembled
 * bits are bitcast to the port's double (x87 deviation, as documented
 * across this band). What the built value MEANS stays with the caller
 * (the frexp band); this leaf is the faithful bit construction. */
double double_build_with_exponent(uint32_t significand_low,
                                  uint32_t significand_high,
                                  int16_t exponent)
{
    const uint32_t bias_shifted =
        (uint32_t)(uint16_t)((uint32_t)(int32_t)(exponent + 0x3fe)) << 4;
    const uint16_t top = (uint16_t)(
        ((significand_high >> 16) & 0x800fu) | bias_shifted);
    const uint64_t bits = ((uint64_t)top << 48)
        | ((uint64_t)(uint16_t)(significand_high & 0xffffu) << 32)
        | (uint64_t)significand_low;
    double value = 0.0;
    __builtin_memcpy(&value, &bits, sizeof(value));
    return value;
}

/* ------------------------------------------------------------------ *
 *  Constant and DAT-global band (see crt.hpp). */

double constant_zero_float_get(void)
{
    return 0.0;
}

int32_t constant_negative_10000_get(void)
{
    return -10000;
}

uint32_t processor_feature_fallback_false(void)
{
    return 0u;
}

uint32_t flh_magic_uu_noop(void)
{
    return 0u;
}

/* The two DAT globals, static-backed until the corpus's writers (all
 * still unported) land — message.cpp's s_system_state precedent. */
static double   s_global_float_00556f00 = 0.0;
static uint32_t s_global_dword_004ff9bc = 0u;

double global_float_field_00556f00_get(void)
{
    return s_global_float_00556f00;
}

uint32_t global_value_004ff9bc_store_and_return(const void* value)
{
    if (value == NULL) {
        return 0u; /* deviation: corpus dereferences blindly */
    }
    uint32_t v = 0u;
    memcpy(&v, value, sizeof(v));
    s_global_dword_004ff9bc = v;
    return v;
}
