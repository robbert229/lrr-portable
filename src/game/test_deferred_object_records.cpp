// SPDX-License-Identifier: LGPL-3.0-or-later
/* Deferred object-list records, their owned names and selective creation
 * on the existing neutral grid and catalog. */
#include <string>
#include <vector>
#include "gods98/directdraw.h"
#include "game/lego_unit.h"
#include "game/legoobject_unit.h"
#include "game/terrain_map.h"
#include "test/lrr_test.h"
#include "test/synthetic_initializer_fixture.h"
#include "game/lego_globs.h"
#include "game/legoobject_globs.h"

namespace {
constexpr char Stats[] =
    " Figure { Levels 1 CollRadius 3 CrossLand TRUE NumOfToolsCanCarry 2 } "
    " VehicleA { Levels 1 CrossLand TRUE CanBeDriven TRUE CollRadius 4 } ";

void Queue(void* model, int type, int x, int y, float health,
           const char* name, const char* link = nullptr)
{
    float world[2];
    terrain_map_block_to_world(legoGlobs.currLevel->map, x, y, &world[0], &world[1]);
    legoobject_deferred_olist_queue_add(model, type, 0, world, 0, health, name, link);
}

int Census(Lego_Object* obj, void* data)
{
    static_cast<std::vector<Lego_Object*>*>(data)->push_back(obj);
    return 0;
}

std::vector<Lego_Object*> Objects()
{
    std::vector<Lego_Object*> result;
    legoobject_run_through_lists(Census, &result, 1);
    return result;
}

void CheckRecordCleared(unsigned i)
{
    const auto& rec = objectGlobs.recordObjs[i];
    CHECK_EQ(rec.name, nullptr);
    CHECK_EQ(rec.linkName, nullptr);
    CHECK_EQ(rec.model, nullptr);
}

void Scenario()
{
    legoobject_grid_object_records_release();
    CHECK_EQ(objectGlobs.recordObjsCount, 0u);
    const auto initial = Objects();
    char figureName[] = "Source";
    char vehicleName[] = "Target";
    char link[] = "tArGeT";
    Queue(legoGlobs.miniFigureData, 2, 2, 3, 82, figureName, link);
    Queue(legoGlobs.vehicleData, 1, 2, 3, 61, vehicleName);
    Queue(legoGlobs.contCrystal, 6, 2, 3, 53, nullptr);
    Queue(legoGlobs.contOresTable, 7, 1, 4, 45, "Other");
    CHECK_EQ(objectGlobs.recordObjsCount, 4u);
    CHECK_EQ(Objects().size(), initial.size());
    if (CHECK(objectGlobs.recordObjs[0].name && objectGlobs.recordObjs[0].linkName,
              "deferred names owned")) {
        figureName[0] = vehicleName[0] = link[0] = 'X';
        CHECK_STREQ(objectGlobs.recordObjs[0].name, "Source");
        CHECK_STREQ(objectGlobs.recordObjs[0].linkName, "tArGeT");
        CHECK_STREQ(objectGlobs.recordObjs[1].name, "Target");
    }
    int wrong[] = {1, 2};
    legoobject_deferred_olist_create_and_link(wrong);
    CHECK_EQ(objectGlobs.recordObjsCount, 4u);
    CHECK_EQ(Objects().size(), initial.size());
    int requested[] = {2, 3};
    legoobject_deferred_olist_create_and_link(requested);
    CHECK_EQ(objectGlobs.recordObjsCount, 1u);
    CHECK_STREQ(objectGlobs.recordObjs[0].name, "Other");
    for (unsigned i = 1; i < 4; ++i) CheckRecordCleared(i);
    const auto after = Objects();
    CHECK_EQ(after.size(), initial.size() + 3u);
    Lego_Object* figure = nullptr;
    Lego_Object* vehicle = nullptr;
    Lego_Object* resource = nullptr;
    for (auto* obj : after) {
        if (obj->type == 2 && obj->health == 82) figure = obj;
        if (obj->type == 1 && obj->health == 61) vehicle = obj;
        if (obj->type == 6 && obj->health == 53) resource = obj;
    }
    if (CHECK(figure && vehicle && resource, "selectively created three objects")) {
        CHECK_EQ(figure->routeToObject, vehicle);
        CHECK_EQ(figure->flags2 & 4u, 4u);
        CHECK_EQ(figure->abilityFlags & 4u, 4u);
        CHECK_EQ(vehicle->flags4 & 0x40u, 0u);
        for (auto* obj : {figure, vehicle, resource}) {
            int x = -1, y = -1;
            CHECK_EQ(legoobject_position_to_terrain_cell(obj, &x, &y), 1);
            CHECK_EQ(x, 2);
            CHECK_EQ(y, 3);
            CHECK_EQ(obj->flags4 & 0x200u, 0x200u);
        }
    }
    // Reusing a compacted slot without a name must not retain a freed
    // link or accidentally link to one of the earlier objects.
    Queue(legoGlobs.contCrystal, 6, 1, 4, 37, nullptr);
    CHECK_EQ(objectGlobs.recordObjsCount, 2u);
    CHECK_EQ(objectGlobs.recordObjs[1].name, nullptr);
    CHECK_EQ(objectGlobs.recordObjs[1].linkName, nullptr);
    int next[] = {1, 4};
    legoobject_deferred_olist_create_and_link(next);
    CHECK_EQ(objectGlobs.recordObjsCount, 0u);
    CheckRecordCleared(0);
    CheckRecordCleared(1);
    CHECK_EQ(Objects().size(), initial.size() + 5u);
    legoobject_deferred_olist_create_and_link(next);
    CHECK_EQ(Objects().size(), initial.size() + 5u);

    // Cancellation releases every queued name and empties the full table;
    // the next level can reuse even slots beyond the early record range.
    float outside[] = {legoGlobs.currLevel->map->worldX - 80.0f,
                       legoGlobs.currLevel->map->worldY};
    legoobject_deferred_olist_queue_add(legoGlobs.contCrystal, 6, 0, outside, 0, 100, "Outside", nullptr);
    CHECK_EQ(objectGlobs.recordObjsCount, 0u);
    const unsigned capacity = sizeof(objectGlobs.recordObjs) / sizeof(objectGlobs.recordObjs[0]);
    for (unsigned i = 0; i < capacity; ++i)
        Queue(legoGlobs.contCrystal, 6, 1, 4, 100, "Pending", "Missing");
    CHECK_EQ(objectGlobs.recordObjsCount, capacity);
    Queue(legoGlobs.contCrystal, 6, 2, 3, 60, "Extra");
    CHECK_EQ(objectGlobs.recordObjsCount, capacity);
    CHECK_STREQ(objectGlobs.recordObjs[capacity - 1].name, "Pending");
    legoobject_grid_object_records_release();
    CHECK_EQ(objectGlobs.recordObjsCount, 0u);
    for (unsigned i = 0; i < capacity; ++i) CheckRecordCleared(i);
    legoobject_grid_object_records_release();
    CHECK_EQ(Objects().size(), initial.size() + 5u);
}
}

int main()
{
    if (!synthetic_fixture::initialise_full_game("deferred_object_records", "DeferredRecords", Stats))
        return lrr_test_result();
    Scenario();
    lego_engine_shutdown();
    CHECK_EQ(legoGlobs.currLevel, nullptr);
    directdraw_shutdown();
    return lrr_test_result("deferred_object_records");
}
