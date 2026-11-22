// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_weapons — named weapon configuration, default and level-specific
 * damage, vertical hit bounds and per-slot timers on a vehicle model. */
#include "game/legoobject_record.h"
#include "game/vehicle_model_record.h"
#include "game/weapons.h"
#include "gods98/config.h"
#include "game/lego_globs.h"
#include "test/lrr_test.h"

extern "C" {
int weapons_initialise(BlockRec* config, const char* gameName);
float weapons_get_damage_for_object(int weaponType, Lego_Object* object);
float weapon_type_discharge_rate_get_target(int type);
float weapon_type_range_get_target(int type);
float weapon_type_wall_destroy_time_get_target(int type, int wallType);
int weapons_z_within_bounds(Weapons_Vector3F* origin, Weapons_Vector3F* direction,
                            float height, float top, float distance);
void weapons_model_timer_set(Lego_Object* object, float time, unsigned int slot);
float weapons_model_timer_get(Lego_Object* object, unsigned int slot);
}

extern "C" int __wrap_lego_entity_type_pair_lookup(const char* name, int* type,
                                                  unsigned int* id, void**)
{
    if (std::strcmp(name, "TestVehicle") != 0) return 0;
    *type = 1;
    *id = 3;
    return 1;
}

extern "C" uint32_t __wrap_building_upgrade_admission_check(int type, int id)
{
    return type == 1 && id == 3 ? 2u : 0u;
}

int main(void)
{
    struct ConfigEntry {
        char key[16];
        char value[12];
        unsigned int depth;
    } entries[] = {
        { "LegoRR", "{", 0 },
        { "WeaponTypes", "{", 1 },
        { "TestLaser", "{", 2 },
        { "DefaultDamage", "8", 3 },
        { "RechargeTime", "2.5", 3 },
        { "DischargeRate", "0.5", 3 },
        { "TestVehicle", "2:4", 3 },
        { "End", "", 0 },
    };
    constexpr size_t count = sizeof(entries) / sizeof(entries[0]);
    BlockRec config[count] = {};
    for (size_t i = 0; i < count; ++i) {
        config[i].key = entries[i].key;
        config[i].value = entries[i].value;
        config[i].depth = entries[i].depth;
        if (i != 0) config[i].prev = &config[i - 1];
        if (i + 1 < count) config[i].next = &config[i + 1];
    }
    for (const char*& name : legoGlobs.langSurface_name) name = "Lego_SurfaceType_Test";
    if (!CHECK_EQ(weapons_initialise(config, "LegoRR"), 1)) return lrr_test_result();

    CHECK_EQ(object_type_name_index_find("testLASER"), 0u);
    CHECK_EQ(object_type_name_index_find("missing"), 2u);
    CHECK_EQ(weapon_type_recharge_time_get_target(0), 2.5f);
    CHECK_EQ(weapon_type_discharge_rate_get_target(0), 0.5f);
    CHECK_EQ(weapon_type_range_get_target(0), 150.0f);
    CHECK_EQ(weapon_type_wall_destroy_time_get_target(0, 1), 5.0f);

    Lego_Object target = {};
    target.type = 1;
    target.id = 3;
    CHECK_EQ(weapons_get_damage_for_object(0, &target), 2.0f);
    target.objLevel = 1;
    CHECK_EQ(weapons_get_damage_for_object(0, &target), 4.0f);
    target.id = 4;
    CHECK_EQ(weapons_get_damage_for_object(0, &target), 8.0f);

    Weapons_Vector3F origin = { 0.0f, 0.0f, 8.0f };
    Weapons_Vector3F direction = { 1.0f, 0.0f, -2.0f };
    CHECK_EQ(weapons_z_within_bounds(&origin, &direction, 4.0f, 6.0f, 0.5f), 0);
    CHECK_EQ(weapons_z_within_bounds(&origin, &direction, 4.0f, 6.0f, 1.0f), 1);
    CHECK_EQ(weapons_z_within_bounds(&origin, &direction, 4.0f, 6.0f, 2.0f), 1);
    CHECK_EQ(weapons_z_within_bounds(&origin, &direction, 4.0f, 6.0f, 3.0f), 1);
    CHECK_EQ(weapons_z_within_bounds(&origin, &direction, 4.0f, 6.0f, 3.5f), 0);

    VehicleModel vehicle = {};
    target.vehicle = &vehicle;
    weapons_model_timer_set(&target, 4.0f, 1);
    weapons_model_timer_set(&target, 6.0f, 2);
    CHECK_EQ(weapons_model_timer_get(&target, 0), 0.0f);
    CHECK_EQ(weapons_model_timer_get(&target, 1), 4.0f);
    CHECK_EQ(weapons_model_timer_get(&target, 2), 6.0f);
    return lrr_test_result();
}
