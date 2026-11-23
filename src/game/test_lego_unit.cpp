// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_lego_unit — resource accounting on the current level, and
 * case-insensitive lookup of configured and built-in object types. */
#include "game/building_model_record.h"
#include "game/lego_unit.h"
#include "game/lego_globs.h"
#include "test/lrr_test.h"

extern "C" void __wrap_ui_overlay_animation_advance(int, unsigned int)
{
}

int main(void)
{
    Lego_Level level = {};
    legoGlobs.currLevel = &level;
    level.ore = 10;
    level.crystals = 8;
    lego_level_resource_balance_add_and_sound(1);
    lego_level_resource_balance_add_and_sound(0);
    CHECK_EQ(lego_upgrade_resource_check(1), 11);
    CHECK_EQ(lego_upgrade_resource_check(0), 9);
    lego_upgrade_resource_debit(1, 4);
    lego_upgrade_resource_debit(0, 2);
    CHECK_EQ(lego_upgrade_resource_check(1), 7);
    CHECK_EQ(lego_upgrade_resource_check(0), 7);

    level.f_08c = 5;
    global_object_field8c_increment();
    global_object_field8c_subtract(2);
    global_object_field98_add(3);
    CHECK_EQ(noop_callback(1), 4);
    CHECK_EQ(noop_callback(0), 1);
    global_object_field98_clear_current();
    CHECK_EQ(noop_callback(0), 4);
    legoGlobs.currLevel = nullptr;
    CHECK_EQ(lego_upgrade_resource_check(0), 0);

    char base[] = "Base", lift[] = "Lift";
    char* buildingNames[] = { base, lift };
    BuildingModel buildings[2] = {};
    legoGlobs.typeCounts[3] = 2;
    legoGlobs.buildingName = buildingNames;
    legoGlobs.buildingData = buildings;
    int studModel = 0;
    legoGlobs.contProcessedOre = &studModel;
    int type = -1;
    unsigned int id = 0;
    void* model = nullptr;
    CHECK_EQ(lego_entity_type_pair_lookup("LiFt", &type, &id, &model), 1);
    CHECK_EQ(type, 4);
    CHECK_EQ(id, 1u);
    CHECK_EQ(model, &buildings[1]);
    CHECK_EQ(lego_entity_at(type, id, &model), 1);
    CHECK_EQ(model, &buildings[1]);

    CHECK_EQ(lego_entity_type_pair_lookup("processedore", &type, &id, &model), 1);
    CHECK_EQ(type, 7);
    CHECK_EQ(id, 1u);
    CHECK_EQ(model, &studModel);
    CHECK_EQ(lego_entity_type_pair_lookup("TVcamera", &type, &id, &model), 1);
    CHECK_EQ(type, -1);
    CHECK_EQ(model, nullptr);
    CHECK_EQ(lego_entity_type_pair_lookup("missing", &type, &id, &model), 0);
    return lrr_test_result("lego_unit");
}
