// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_objective_unit — resource thresholds, reaching a tutorial block
 * or constructing a matching object, and timer success and failure. */
#include "game/lego_level_record.h"
#include "game/legoobject_record.h"
#include "game/objective_unit.h"
#include "test/lrr_test.h"

static Lego_Object object;
static int objectBlock[2];

extern "C" int __wrap_legoobject_run_through_lists_skip_upgrade_parts(
    int (*callback)(Lego_Object*, void*), void* data)
{
    return callback(&object, data);
}

extern "C" int __wrap_legoobject_position_to_terrain_cell(Lego_Object*, int* x, int* y)
{
    *x = objectBlock[0];
    *y = objectBlock[1];
    return 1;
}

int main(void)
{
    Lego_Level level = {};
    int completed = -1;
    const int goal[] = { 3, 5 };
    objective_set_cry_ore_objectives(&level, 4, 6);
    objective_set_block_objective(&level, goal);
    objective_set_construction_objective(&level, 4, 2);

    object.type = 2; // Raider
    objectBlock[0] = 3;
    objectBlock[1] = 5;
    level.f_08c = 3; // Stored crystals
    level.crystals = 6; // Stored ore
    CHECK_EQ(objective_check_completed(&level, &completed, 0.0f), 0);
    level.f_08c = 4;
    level.crystals = 5;
    CHECK_EQ(objective_check_completed(&level, &completed, 0.0f), 0);
    level.crystals = 6;
    CHECK_EQ(objective_check_completed(&level, &completed, 0.0f), 1);
    CHECK_EQ(completed, 1);

    objectBlock[1] = 4;
    CHECK_EQ(objective_check_completed(&level, &completed, 0.0f), 0);
    object.type = 4; // Building
    object.id = 1;
    CHECK_EQ(objective_check_completed(&level, &completed, 0.0f), 0);
    object.id = 2;
    CHECK_EQ(objective_check_completed(&level, &completed, 0.0f), 1);

    object.id = 1;
    objective_set_timer_objective(&level, 2.0f, 0);
    CHECK_EQ(objective_check_completed(&level, &completed, 0.5f), 0);
    CHECK_EQ(level.objective.timer, 1.5f);
    CHECK_EQ(objective_check_completed(&level, &completed, 1.5f), 1);
    CHECK_EQ(completed, 1);

    objective_set_timer_objective(&level, 1.0f, 1);
    CHECK_EQ(objective_check_completed(&level, &completed, 2.0f), 1);
    CHECK_EQ(completed, 0);
    return lrr_test_result();
}
