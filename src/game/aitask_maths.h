// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-002's copy of gods98 Maths.h's inline 2D vector distance
 * (Maths_Vector2DDistance) over AITaskPoint2F.
 * The original unit expands it in place, so it has no address of its
 * own: 0x00404280's distance test keeps the difference's x in a
 * stack slot, as the out-of-line copy at 0x00401240 does (finding 675).
 * Only aitask.c includes this file, after AITaskPoint2F.
 *
 * It is a static inline function, not a macro: as a macro the expansion
 * compiles differently (finding 544). In a header it adds no Reccmp lint
 * debt. */
#ifndef LEGORRR_GAME_AITASK_MATHS_H
#define LEGORRR_GAME_AITASK_MATHS_H

static inline float aitask_vector2d_distance(const AITaskPoint2F* a,
                                               const AITaskPoint2F* b)
{
    return (float)sqrt((a->x - b->x) * (a->x - b->x) + (a->y - b->y) * (a->y - b->y));
}

#endif /* LEGORRR_GAME_AITASK_MATHS_H */
