// SPDX-License-Identifier: LGPL-3.0-or-later
/* The unread 1/255 that closes Container.c's literal pool in the original
 * (0049e898, after the 255.0f its colour setters read): an unused inline
 * copy standing for a removed colour helper.
 *
 * No code of the rebuilt unit creates the literal. A byte-to-unit colour
 * conversion (`c * (1.0f / 255.0f)`, the inverse of the `* 255.0f` the
 * unit's setters do) creates it. As for maths_normalise_head.h (finding
 * 693), an `extern __inline` function is compiled although nothing calls
 * it; /OPT:REF then removes the code but not the literal. The helper itself
 * is not evidenced, only its literal and the literal's position. Included
 * after the unit's last function. Only the MSVC 5.0 matching build sees
 * it. */
#ifndef LEGORRR_GODS98_CONTAINER_COLOUR_TAIL_H
#define LEGORRR_GODS98_CONTAINER_COLOUR_TAIL_H

#if defined(_MSC_VER) && _MSC_VER < 1200
extern __inline float container_colour_byte_to_unit(unsigned char c)
{
    return c * (1.0f / 255.0f);
}
#endif

#endif /* LEGORRR_GODS98_CONTAINER_COLOUR_TAIL_H */
