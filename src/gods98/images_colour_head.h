// SPDX-License-Identifier: LGPL-3.0-or-later
/* The unread 1/255 in Images.c's literal pool in the original (0049e994,
 * between Maths.h's unread 1.0f and the 255.0f Image_SetupTrans reads): an
 * unused inline copy standing for a removed Images.h colour helper.
 *
 * No code of the rebuilt unit creates the literal. A byte-to-unit colour
 * conversion (`c * (1.0f / 255.0f)`, the inverse of Image_SetupTrans's
 * `* 255.0f`) creates it. As for maths_normalise_head.h (finding 693), an
 * `extern __inline` function is compiled although nothing calls it;
 * /OPT:REF then removes the code but not the literal. The helper itself is
 * not evidenced, only its literal and the literal's position. The unit
 * includes it where the literal is created, before Image_SetupTrans; a
 * definition costs two symbols of MSVC's counter at every function after
 * it (finding 538). Only the MSVC 5.0 matching build sees it. */
#ifndef LEGORRR_GODS98_IMAGES_COLOUR_HEAD_H
#define LEGORRR_GODS98_IMAGES_COLOUR_HEAD_H

#if defined(_MSC_VER) && _MSC_VER < 1200
extern __inline float image_colour_byte_to_unit(unsigned char c)
{
    return c * (1.0f / 255.0f);
}
#endif

#endif /* LEGORRR_GODS98_IMAGES_COLOUR_HEAD_H */
