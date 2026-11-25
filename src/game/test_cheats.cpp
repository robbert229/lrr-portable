// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_cheats — the [cheats] keys at their units' sites, off (the
 * original) and on, set in-process (LRR_SetSetting), on every host:
 * - cheats:unlimited_oxygen (docs/divergences.md, "Cheat: unlimited
 *   oxygen"): Lego.c's air supply update (00424560) drains the level's air
 *   by a breathing object's OxygenCoef, and refills it by a Support
 *   Station's; with the cheat it never drains;
 * - cheats:unlimited_raiders (docs/divergences.md, "Cheat: unlimited
 *   raiders"): LegoObject.c's oxygen check (0043c200) refuses a raider the
 *   level's Support Stations cannot give air; with the cheat it never
 *   refuses.
 * The objects' stats are synthetic records; the object lists are empty. */
#include "game/lego_unit.h"
#include "game/legoobject_unit.h"
/* After the units' headers, whose declarations its macros would rename. */
#include "game/entity_stats.h"
#include "game/lego_globs.h"
#include "game/legoobject_record.h"
#include "platform/native/config.h"
#include "test/lrr_test.h"

namespace {

/* The level's air after one second of an object whose OxygenCoef is
 * `coef`, from `start`. */
float AirAfter(float coef, float start)
{
    Lego_Level level = {};
    StatsData stats = {};
    Lego_Object obj = {};

    level.oxygenLevel = start;
    level.oxygenRate = 1.0f;
    stats.OxygenCoef = coef;
    obj.type = 1;
    obj.stats = &stats;
    legoGlobs.currLevel = &level;
    lego_air_supply_amount_update(&obj, 25.0f);
    legoGlobs.currLevel = nullptr;
    return level.oxygenLevel;
}

void UnlimitedOxygen()
{
    /* A missing key keeps the original's air supply. */
    LRR_SetSetting(LRR_SETTING_UNLIMITED_OXYGEN, nullptr);
    CHECK_NEAR(AirAfter(-1.0f, 50.0f), 49.0f, 1e-4f);
    CHECK_NEAR(AirAfter(1.0f, 50.0f), 51.0f, 1e-4f);

    LRR_SetSetting(LRR_SETTING_UNLIMITED_OXYGEN, "false");
    CHECK_NEAR(AirAfter(-1.0f, 50.0f), 49.0f, 1e-4f);

    /* With the cheat, a breathing raider leaves the air as it is, and a
     * Support Station still refills it. */
    LRR_SetSetting(LRR_SETTING_UNLIMITED_OXYGEN, "true");
    CHECK_NEAR(AirAfter(-1.0f, 50.0f), 50.0f, 1e-4f);
    CHECK_NEAR(AirAfter(1.0f, 50.0f), 51.0f, 1e-4f);
    CHECK_NEAR(AirAfter(1.0f, 99.5f), 100.0f, 1e-4f);
    LRR_SetSetting(LRR_SETTING_UNLIMITED_OXYGEN, nullptr);
}

void UnlimitedRaiders()
{
    /* A raider breathes 1; the teleporter's request adds another. */
    StatsData raider = {};
    StatsData* raiderStats[] = { &raider };
    Stats_Globs* stats = entity_stats_port_globs();
    StatsData** saved = stats->objectStats[1];
    stats->objectStats[1] = raiderStats;

    raider.OxygenCoef = -1.0f;
    LRR_SetSetting(LRR_SETTING_UNLIMITED_RAIDERS, nullptr);
    CHECK_EQ(legoobject_state_pair_predicate(1, 0, 1, 0), 0);
    LRR_SetSetting(LRR_SETTING_UNLIMITED_RAIDERS, "false");
    CHECK_EQ(legoobject_state_pair_predicate(1, 0, 1, 0), 0);
    LRR_SetSetting(LRR_SETTING_UNLIMITED_RAIDERS, "true");
    CHECK_EQ(legoobject_state_pair_predicate(1, 0, 1, 0), 1);

    /* An object that does not breathe is never refused. */
    raider.OxygenCoef = 0.0f;
    LRR_SetSetting(LRR_SETTING_UNLIMITED_RAIDERS, "false");
    CHECK_EQ(legoobject_state_pair_predicate(1, 0, 1, 0), 1);

    LRR_SetSetting(LRR_SETTING_UNLIMITED_RAIDERS, nullptr);
    stats->objectStats[1] = saved;
}

} // namespace

int main(void)
{
    UnlimitedOxygen();
    UnlimitedRaiders();
    return lrr_test_result("cheats");
}
