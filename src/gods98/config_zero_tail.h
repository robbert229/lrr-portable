// SPDX-License-Identifier: LGPL-3.0-or-later
/* The unread 0.0f that closes Config.c's literal pool in the original
 * (0049e90c, after Maths.h's unread 1.0f): an unused inline copy standing
 * for a removed Config.h helper.
 *
 * No code of the rebuilt unit creates a 0.0f literal; the unit's own zero
 * is a named constant, which MSVC 5.0 emits before the literals. A value
 * getter that falls back to zero (`value ? *value : 0.0f`) creates it. As
 * for maths_normalise_head.h (finding 693), an `extern __inline` function
 * is compiled although nothing calls it; /OPT:REF then removes the code but
 * not the literal. The helper itself is not evidenced, only its literal and
 * the literal's position. Included after maths_normalise_head.h at the end
 * of the unit. Only the MSVC 5.0 matching build sees it. */
#ifndef LEGORRR_GODS98_CONFIG_ZERO_TAIL_H
#define LEGORRR_GODS98_CONFIG_ZERO_TAIL_H

#if defined(_MSC_VER) && _MSC_VER < 1200
extern __inline float config_float_or_zero(const float* value)
{
    return value ? *value : 0.0f;
}
#endif

#endif /* LEGORRR_GODS98_CONFIG_ZERO_TAIL_H */
