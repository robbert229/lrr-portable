// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_reward_score — weighted score and the time bonus, recalculation
 * and score limits. test_reward.cpp already covers the saved record. */
#include "game/lego_level_record.h"
#include "game/reward.h"
#include "test/lrr_test.h"

static Lego_Level level;
static Lego_Level* activeLevel;
static RewardLevel reward;

extern "C" Lego_Level* __wrap_level_current_owner_get(void) { return activeLevel; }

int main(void)
{
    CHECK_EQ(reward_calculate_score(), 0);
    activeLevel = &level;
    CHECK_EQ(reward_calculate_score(), 0);
    rewardGlobs.current = &reward;

    RewardLevelItem* items = reward.itemsArray;
    items[REWARD_ITEM_CRYSTALS].actual = 4.0f;
    items[REWARD_ITEM_CRYSTALS].total = 8.0f;
    items[REWARD_ITEM_CRYSTALS].importance = 2.0f;
    items[REWARD_ITEM_TIMER].percent = 75.0f;
    items[REWARD_ITEM_TIMER].importance = 2.0f;
    items[REWARD_ITEM_ORE].actual = 10.0f;
    items[REWARD_ITEM_ORE].total = 8.0f;
    items[REWARD_ITEM_ORE].importance = 1.0f;
    reward.modifier = 5;
    CHECK_EQ(reward_calculate_score(), 1);
    CHECK_EQ(items[REWARD_ITEM_CRYSTALS].percent, 50.0f);
    CHECK_EQ(items[REWARD_ITEM_ORE].percent, 100.0f);
    CHECK_EQ(items[REWARD_ITEM_SCORE].actual, 150.0f);
    CHECK_EQ(items[REWARD_ITEM_SCORE].total, 400.0f);
    CHECK_EQ(items[REWARD_ITEM_SCORE].percent, 42.5f);

    reward.modifier = 90;
    CHECK_EQ(reward_calculate_score(), 1);
    CHECK_EQ(items[REWARD_ITEM_SCORE].total, 400.0f);
    CHECK_EQ(items[REWARD_ITEM_SCORE].percent, 100.0f);

    reward.modifier = 0;
    items[REWARD_ITEM_CRYSTALS].importance = 0.0f;
    items[REWARD_ITEM_TIMER].percent = 125.0f;
    CHECK_EQ(reward_calculate_score(), 1);
    CHECK_EQ(items[REWARD_ITEM_SCORE].actual, 0.0f);
    CHECK_EQ(items[REWARD_ITEM_SCORE].total, 200.0f);
    CHECK_EQ(items[REWARD_ITEM_SCORE].percent, 0.0f);
    return lrr_test_result();
}
