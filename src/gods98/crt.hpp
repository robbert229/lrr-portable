// SPDX-License-Identifier: LGPL-3.0-only
/* CRT and compiler-runtime leaves (findings 260/381/393/394): the small
 * runtime thunks the corpus calls directly.
 *
 * Deviations (documented): the corpus's word-at-a-time memmove loops are
 * byte-equivalent memmove behavior, so the port calls memmove. The
 * corpus's FloatToLong is the compiler's __ftol helper — under the x87
 * control word it sets, that is C++ truncation toward zero; the port uses
 * the C++ cast (the corpus's 32-bit fist result vs the host cast is the
 * usual x87/IEEE deviation; overflow values are outside the corpus's
 * caller set). NoOpCallback's return value is undefined per finding 381 —
 * callers only use it as a callable no-op. */
#ifndef LEGORRR_GODS98_CRT_HPP
#define LEGORRR_GODS98_CRT_HPP

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* FUN_00498000: memmove — copies the byte range with overlap handling,
 * backward when the destination starts inside the source range, forward
 * otherwise. Returns the destination. */
void* crt_memory_move(void* dest, const void* src, size_t count);

/* FUN_00417430: __ftol — float to 32-bit integer, truncated toward zero
 * (the corpus's 32-bit fist result; the port mirrors the width). */
int float_to_long(float value);

/* FUN_0041f8e0: the callable no-op installed by several engine paths. */
void noop_callback(void);

/* FUN_0047d3f0: bit count (corpus: clear-lowest-set-bit loop). */
int bit_count32(uint32_t value);

/* FUN_0047d700: bit count by position loop — same answer, corpus keeps the
 * separate body. */
int pop_count32(uint32_t value);

/* FUN_0047d720: lowest set bit's index, or 0xffffffff when value is zero. */
uint32_t find_first_set_bit32(uint32_t value);

/* FUN_0047e7e0: the position-loop bit count again (the corpus's second
 * identical body; kept distinct to mirror the address map). */
int bit_count32_by_position(uint32_t value);

/* FUN_00497ab0: zero test over 3 dwords (the "96-bit" pair of 32-bit limbs
 * plus its high word — the corpus checks exactly 3). */
int uint96_is_zero(const uint32_t limbs[3]);

/* FUN_0048e870: the double finite test — exponent field not all ones,
 * so true for finite values and false for Inf/NaN. */
int double_is_finite(double value);

/* FUN_0045b900: divide an integer pair by `divisor`, both operands cast to
 * float before the divide; results land in the float pair. */
void integer_pair_divide_to_float_pair(int x, int y, int divisor,
                                       float* out_x, float* out_y);

/* FUN_0045b930: __ftol applied twice — the float pair truncated toward
 * zero into the integer pair (same deviation as float_to_long). */
void float_pair_to_integer_pair(float x, float y, int* out_x, int* out_y);

/* FUN_00499fb0: unsigned 32-bit add with carry — stores the sum and
 * answers 1 when the addition carried (finding 302; the primitive the
 * corpus's 96-bit increment/add/multiply helpers build on). */
int crt_unsigned_add_with_carry(uint32_t a, uint32_t b, uint32_t* sum);

/* FUN_00493a80: the double special-value classifier over the raw bit
 * pair — 1 positive infinity, 2 negative infinity, 3 quiet-NaN-shaped
 * (exponent all ones, quiet bit set), 4 signaling-NaN-shaped (exponent
 * all ones, quiet bit clear, mantissa nonzero), 0 finite (finding
 * 399). Sign does not steer cases 3/4, matching the corpus. */
int double_classify(uint32_t lo, uint32_t hi);

/* FUN_00499710: UTF-16 code units before the NUL terminator (finding 377
 * — the corpus walks 16-bit units the same way strlen walks bytes). */
int wide_string_length(const uint16_t* text);

/* FUN_0049add0: scan at most `bound` wide characters; answer the
 * terminator index plus one (the count including the NUL) when found
 * inside the bound, otherwise the bound itself — and the bound for a
 * zero bound, as in the corpus (findings 342/397). */
int bounded_wide_string_length_including_null(const uint16_t* text,
                                              int bound);

/* FUN_00492068: double high-word exponent selector — the exponent field
 * for ordinary values, the untouched high word when the exponent is all
 * ones (Inf/NaN) (findings 386/399; CrtAsin's pre-dispatch probe). */
uint32_t double_exponent_bits_or_special(uint32_t high_word);

/* Format-argument cursor reads (FUN_00491430 / FUN_00491470 /
 * FUN_00491450; findings 231/395 — EngineFormatStringToStream's
 * width/precision/value walk): each reads the value at the cursor and
 * advances it. The word read returns the zero-extended 16-bit value
 * (the corpus's CONCAT upper half is the pointer's own high word — zero
 * in 32-bit space). Reads go through memcpy — the cursor may sit at an
 * unaligned continuation of a previous wider read. */
uint32_t format_argument_read_dword(uint8_t** cursor);
uint32_t format_argument_read_word(uint8_t** cursor);
double format_argument_read_double(uint8_t** cursor);

/* FUN_00493150: x87 round-to-integral (FRNDINT) — the nearest integer
 * under the round-to-nearest-even mode, as a floating value. Ported as
 * double (the port has no x87 80-bit register; every x87 leaf here is a
 * documented single/double deviation). */
double x87_round_to_integral(double value);

/* FUN_00493a40 (DoubleBuildWithExponent, findings 383/399): rebuild the
 * 8-bit-exponent floating value from the two significand words and the
 * exponent bias input — the corpus's exact bit recipe: top word =
 * ((param_2 >> 16) & 0x800f) | ((param_3 + 0x3fe) << 4), significand =
 * (param_2 & 0xffff) : param_1. Ported as double (x87 deviation, as
 * above); the bits are built with integer arithmetic and bitcast. */
double double_build_with_exponent(uint32_t significand_low,
                                  uint32_t significand_high,
                                  int16_t exponent);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_CRT_HPP */
