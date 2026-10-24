// SPDX-License-Identifier: LGPL-3.0-only
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
