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

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_CRT_HPP */
