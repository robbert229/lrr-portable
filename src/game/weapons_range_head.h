// SPDX-License-Identifier: LGPL-3.0-or-later
/* The unread 100.0f and 150.0f before weapons_model_aim_update's 10000.0f
 * in the original's Weapons.c (0049e870-0049e874): an unused inline copy
 * standing for a removed helper.
 *
 * No live code of the unit reads either literal; weapons_initialise stores
 * its default range of 150 as an immediate, which creates no literal. A
 * range check that falls back to that default
 * (`range < 100.0f ? 150.0f : range`) creates both, in the original's
 * order. As for maths_normalise_head.h (finding 693), an `extern __inline`
 * function is compiled although nothing calls it; /OPT:REF then removes the
 * code but not the literals. The helper itself is not evidenced, only its
 * literals and their position. A definition costs two symbols of MSVC's
 * counter at every function after it (finding 538). Only the MSVC 5.0
 * matching build sees it. */
#ifndef LEGORRR_GAME_WEAPONS_RANGE_HEAD_H
#define LEGORRR_GAME_WEAPONS_RANGE_HEAD_H

#if defined(_MSC_VER) && _MSC_VER < 1200
extern __inline float weapons_range_or_default(float range)
{
    return range < 100.0f ? 150.0f : range;
}
#endif

#endif /* LEGORRR_GAME_WEAPONS_RANGE_HEAD_H */
