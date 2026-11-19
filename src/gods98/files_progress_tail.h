// SPDX-License-Identifier: LGPL-3.0-or-later
/* The unread 0.0f that closes Files.c's literal pool in the original
 * (0049e9cc, after Maths.h's unread 1.0f): an unused inline copy standing
 * for a removed Files.h helper.
 *
 * No code of the rebuilt unit creates a float literal of its own. A load
 * fraction that falls back to zero for an empty file
 * (`total ? done / total : 0.0f`) creates it. As for
 * maths_normalise_head.h (finding 693), an `extern __inline` function is
 * compiled although nothing calls it; /OPT:REF then removes the code but
 * not the literal. The helper itself is not evidenced, only its literal and
 * the literal's position. Included after maths_normalise_head.h at the end
 * of the unit. Only the MSVC 5.0 matching build sees it. */
#ifndef LEGORRR_GODS98_FILES_PROGRESS_TAIL_H
#define LEGORRR_GODS98_FILES_PROGRESS_TAIL_H

#if defined(_MSC_VER) && _MSC_VER < 1200
extern __inline float files_load_fraction(int done, int total)
{
    return total ? (float)done / (float)total : 0.0f;
}
#endif

#endif /* LEGORRR_GODS98_FILES_PROGRESS_TAIL_H */
