// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_objectrecall — the live raider stack and its snapshot. File images
 * and OSF loading/saving already have coverage in test_savegame.cpp. */
#include "game/legoobject_record.h"
#include "game/objectrecall.h"
#include "test/lrr_test.h"

static char recalledName[12];

extern "C" int __wrap_entity_building_index_set(Lego_Object* object, unsigned int level)
{
    object->objLevel = level;
    return 1;
}

extern "C" void __wrap_legoobject_rename(Lego_Object* object, const char* name)
{
    std::snprintf(recalledName, sizeof(recalledName), "%s", name);
    object->customName = recalledName;
}

int main(void)
{
    Lego_Object raider = {};
    raider.type = 2;
    Lego_Object vehicle = {};
    vehicle.type = 1;
    vehicle.abilityFlags = 0x80;
    CHECK_EQ(object_recall_is_loaded(), 0);
    CHECK_EQ(object_recall_restore(), 0);
    object_recall_store_minifigure(&vehicle);
    object_recall_store_minifigure(&raider);
    CHECK_EQ(object_recall_recall_minifigure(&raider), 0);

    char firstName[] = "FIRST-RAIDER";
    raider.customName = firstName;
    raider.objLevel = 3;
    raider.abilityFlags = 0x5;
    object_recall_store_minifigure(&raider);
    firstName[0] = 'X';

    char secondName[] = "Second";
    raider.customName = secondName;
    raider.objLevel = 1;
    raider.abilityFlags = 0x8;
    object_recall_store_minifigure(&raider);
    object_recall_snapshot();
    CHECK_EQ(object_recall_is_loaded(), 1);

    Lego_Object restored = {};
    restored.type = 2;
    CHECK_EQ(object_recall_recall_minifigure(&vehicle), 0);
    CHECK_EQ(object_recall_recall_minifigure(&restored), 1);
    CHECK_EQ(restored.objLevel, 1u);
    CHECK_EQ(restored.abilityFlags, 0x8u);
    CHECK_STREQ(restored.customName, "Second");

    CHECK_EQ(object_recall_recall_minifigure(&restored), 1);
    CHECK_EQ(restored.objLevel, 3u);
    CHECK_EQ(restored.abilityFlags, 0x5u);
    CHECK_STREQ(restored.customName, "FIRST-RAIDE");
    CHECK_EQ(object_recall_recall_minifigure(&restored), 0);

    object_recall_reset();
    CHECK_EQ(object_recall_restore(), 1);
    CHECK_EQ(object_recall_recall_minifigure(&restored), 1);
    CHECK_STREQ(restored.customName, "Second");
    CHECK_EQ(object_recall_recall_minifigure(&restored), 1);
    CHECK_STREQ(restored.customName, "FIRST-RAIDE");
    object_recall_reset();
    return lrr_test_result();
}
