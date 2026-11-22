// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_upgrade — parse upgrade slots and weapon names, then rebuild
 * the linked objects with exactly the parts for each requested level. */
#include "game/legoobject_record.h"
#include "game/upgrade_link_names.h"
#include "game/upgrade.h"
#include "game/upgrade_record.h"
#include "test/lrr_test.h"

#include <cstdlib>

static int model;
static Lego_Object objects[3];
static Upgrade_PartModel liveParts[3];

extern "C" void* __wrap_lego_upgrade_name_entry_find(const char*) { return &model; }

extern "C" Lego_Object* __wrap_legoobject_create(void*, int type, int id)
{
    Lego_Object& object = objects[id];
    liveParts[id] = {};
    object.type = type;
    object.id = id;
    object.health = 100.0f;
    object.upgradePart = &liveParts[id];
    return &object;
}

extern "C" int __wrap_legoobject_remove(Lego_Object* object)
{
    object->health = -1.0f;
    return 1;
}

int main(void)
{
    struct ConfigEntry {
        char key[16];
        char value[24];
        unsigned int depth;
    } entries[] = {
        { "LegoRR", "{", 0 },
        { "Upgrades", "{", 1 },
        { "Level0001", "{", 2 },
        { "DrillPart", "Front,2,Laser", 3 },
        { "GuardPart", "Side,1", 3 },
        { "Level0100", "{", 2 },
        { "PowerPart", "Rear,3", 3 },
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

    Upgrade_Model upgrades = {};
    upgrade_load(&upgrades, config, "LegoRR");
    if (CHECK_EQ(upgrades.count, 3u) && CHECK(upgrades.parts != nullptr, "parts were loaded")) {
        CHECK_EQ(upgrades.levels, 5u);
        CHECK_EQ(upgrades.parts[0].level, 1u);
        CHECK_EQ(upgrades.parts[0].nullFrameNo, 1u);
        CHECK_STREQ(upgrades.parts[0].nullName, "Front");
        CHECK_STREQ(upgrades.parts[0].weaponName, "Laser");
        CHECK_EQ(upgrades.parts[0].upgradeData, static_cast<void*>(&model));
        CHECK_EQ(upgrades.parts[1].weaponName, nullptr);
        CHECK_EQ(upgrades.parts[2].level, 4u);
        CHECK_EQ(upgrades.parts[2].nullFrameNo, 2u);

        upgrade_set_upgrade_level(&upgrades, 1);
        CHECK_EQ(upgrades.currentLevel, 1u);
        CHECK_EQ(upgrades.objectList, &objects[0]);
        CHECK_EQ(objects[0].type, 10);
        CHECK_EQ(liveParts[0].partInfo, &upgrades.parts[0]);
        CHECK_EQ(liveParts[0].nextObject, &objects[1]);
        CHECK_EQ(liveParts[1].partInfo, &upgrades.parts[1]);
        CHECK_EQ(liveParts[1].nextObject, nullptr);

        upgrade_set_upgrade_level(&upgrades, 4);
        CHECK_EQ(upgrades.currentLevel, 4u);
        CHECK_EQ(upgrades.objectList, &objects[2]);
        CHECK_EQ(objects[0].health, -1.0f);
        CHECK_EQ(objects[1].health, -1.0f);
        CHECK_EQ(objects[2].health, 100.0f);
        CHECK_EQ(liveParts[2].partInfo, &upgrades.parts[2]);
        CHECK_EQ(liveParts[2].nextObject, nullptr);

        upgrade_set_upgrade_level(&upgrades, 0);
        CHECK_EQ(upgrades.objectList, nullptr);
        CHECK_EQ(objects[2].health, -1.0f);
    }
    for (unsigned int i = 0; i < upgrades.count; ++i) {
        std::free(upgrades.parts[i].nullName);
        std::free(upgrades.parts[i].weaponName);
    }
    std::free(upgrades.parts);
    return lrr_test_result();
}
