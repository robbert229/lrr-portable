// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_entity_cell_effects — dropping crystal and ore costs on the
 * building's tile, and marking the refund handled even when disabled. */
#include "game/entity_cell_effects_link_names.h"
#include "game/entity_cell_effects.h"
#include "game/legoobject_record.h"
#include "game/legoobject_unit.h"
#include "test/lrr_test.h"

struct Drops {
    unsigned int crystals;
    unsigned int ore;
};
static Drops drops[2];

extern "C" int __wrap_legoobject_position_to_terrain_cell(Lego_Object* object, int* x, int* y)
{
    *x = object->id;
    *y = 3;
    return 1;
}

extern "C" int __wrap_entity_indexed_record_word_at_b0_get(int type, int id, unsigned int level)
{
    return type == 4 && id == 0 && level == 1u ? 1 : 0;
}

extern "C" int __wrap_entity_indexed_record_word_at_b4_get(int type, int id, unsigned int level)
{
    return type == 4 && level == 1u ? id + 2 : 0;
}

extern "C" int __wrap_stats_table_word_at_b8(int, int, unsigned int)
{
    return 0;
}

extern "C" void __wrap_legoobject_build_placement_command_setup(int* cell, int,
                                                                LegoObjGE_Point2F*, int)
{
    if (cell[0] >= 0 && cell[0] < 2 && cell[1] == 3)
        ++drops[cell[0]].crystals;
}

extern "C" void __wrap_legoobject_build_placement_alternate_command_setup(int* cell, int,
                                                                          LegoObjGE_Point2F*, int)
{
    if (cell[0] >= 0 && cell[0] < 2 && cell[1] == 3)
        ++drops[cell[0]].ore;
}

int main(void)
{
    Lego_Object first = {};
    first.objLevel = 1;
    building_set_cost_drop_disabled(0);
    building_drop_cost(&first);
    CHECK_EQ(drops[0].crystals, 2u);
    CHECK_EQ(drops[0].ore, 1u);
    building_drop_cost(&first);
    CHECK_EQ(drops[0].crystals, 2u);
    CHECK_EQ(drops[0].ore, 1u);

    Lego_Object second = {};
    second.id = 1;
    second.objLevel = 1;
    building_set_cost_drop_disabled(1);
    building_drop_cost(&second);
    CHECK_EQ(drops[1].crystals, 0u);
    building_set_cost_drop_disabled(0);
    building_drop_cost(&second);
    CHECK_EQ(drops[1].crystals, 0u);
    return lrr_test_result("entity_cell_effects");
}
