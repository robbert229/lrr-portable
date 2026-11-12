// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-037's inline percentage of a reward item. The original expands it
 * eight times in Reward_CalculateScore (reward_calculate_score) and once
 * more for the score; written as a macro, MSVC keeps each result in a
 * stack temporary across the score loops instead of reloading it
 * (finding 560). Only reward.c includes this file, just before that
 * function; in a header it adds no annotation-lint debt, as
 * advisor_maths.h. */
#ifndef LEGORRR_GAME_REWARD_MATHS_H
#define LEGORRR_GAME_REWARD_MATHS_H

/* An item's percentage of its total, at most 100. */
static __inline void reward_item_percent(RewardLevelItem* item)
{
    item->percent = (item->total != 0.0f) ? (100.0f / item->total) * item->actual : 100.0f;
    item->percent = (item->percent > 100.0f) ? 100.0f : item->percent;
}

#endif /* LEGORRR_GAME_REWARD_MATHS_H */
