// SPDX-License-Identifier: LGPL-3.0-or-later
/* The unread 100.0f that closes Wad.c's literal pool in the original
 * (0049ead8, after Maths.h's unread 1.0f): an unused inline copy standing
 * for a removed Wad.h helper.
 *
 * No code of the rebuilt unit creates a float literal of its own. A
 * percentage, such as how much of a wad has been read
 * (`done * 100.0f / total`), creates it. As for maths_normalise_head.h
 * (finding 693), an `extern __inline` function is compiled although nothing
 * calls it; /OPT:REF then removes the code but not the literal. The helper
 * itself is not evidenced, only its literal and the literal's position.
 * Included after maths_normalise_head.h at the end of the unit. Only the
 * MSVC 5.0 matching build sees it. */
#ifndef LEGORRR_GODS98_WAD_PERCENT_TAIL_H
#define LEGORRR_GODS98_WAD_PERCENT_TAIL_H

#if defined(_MSC_VER) && _MSC_VER < 1200
extern __inline float wad_percent(int done, int total)
{
    return done * 100.0f / total;
}
#endif

#endif /* LEGORRR_GODS98_WAD_PERCENT_TAIL_H */
