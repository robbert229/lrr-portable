// SPDX-License-Identifier: LGPL-3.0-or-later
/* Real construction, resource ownership and network recalculation on an
 * artificial grid. Every catalog entry shares the generated null scene. */
#include "gods98/config.h"
#include "gods98/container_link_names.h"
#include "gods98/container_unit.h"
#include "gods98/lws.h"
#include "game/animation_object_record.h"
#include "game/aitask.h"
#include "game/aitask_globs.h"
#include "game/aitask_record.h"
#include "game/building_model_record.h"
#include "game/bubbles_link_names.h"
#include "game/bubbles.h"
#include "game/construction_link_names.h"
#include "game/construction.h"
#include "game/entity_cell_effects_link_names.h"
#include "game/entity_cell_effects.h"
#include "game/entity_stats_link_names.h"
#include "game/entity_stats.h"
#include "game/helpwindow_link_names.h"
#include "game/helpwindow.h"
#include "game/lego_unit.h"
#include "game/legoobject_unit.h"
#include "game/terrain_map.h"
#include "platform/native/test_fixture.h"
#include "test/lrr_test.h"
#include "test/synthetic_level_fixture.h"
#include "game/lego_globs.h"
#include "crt/vc5names.h"

extern "C" {
int terrain_cell_state_update_current(const int*);
int terrain_cell_variant_flag_propagate_current(const int*);
void map_cell_tracking_flags_clear_and_refresh_current(const int*);
}

namespace {
using namespace synthetic_fixture;
constexpr unsigned Zone = 0x100000, Occupied = 0x400, Path = 0x20000000;
constexpr unsigned Powered = 0x100, SourceCell = 1, Placed = 0x100;
constexpr unsigned Teleporting = 0x200000;
constexpr int Producer = 0, Consumer = 1;

Lego_Level* Level() { return legoGlobs.currLevel; }
Lego_Block& Cell(int x, int y) { return Level()->blocks[y * Level()->width + x]; }

bool LoadCatalog()
{
    if (!CHECK(fixture_mkdir("Data/Primitive") &&
        write_null_scene("Data/Primitive/motion.lws") &&
        write_text("Data/Primitive/Primitive.ae",
            "Lego { Activities { Activity_Stand Motion Activity_TeleportIn Arrival "
            "Activity_Unpowered Motion } "
            "Motion { FILE motion LWSFILE TRUE LOOPING TRUE } "
            "Arrival { FILE motion LWSFILE TRUE LOOPING FALSE } "
            "PowerLevelScene motion }"), "generate generic building model")) return false;
    constexpr char catalog[] =
        "Lego { BuildingTypes { Producer Primitive Consumer Primitive } "
        "Stats { Producer { Levels 1 CollRadius 4 PowerBuilding TRUE } "
        "Consumer { Levels 1 CollRadius 4 CostOre 1 CostCrystal 1 CrystalDrain 2 } "
        "Ore { Levels 1 CollRadius 5 PickSphere 10 } "
        "PowerCrystal { Levels 1 CollRadius 5 PickSphere 10 } } "
        "HelpWindow { TextWindowArea 0,0,16,16 } }";
    BlockFile* config = block_parse(catalog, sizeof(catalog) - 1);
    if (!CHECK(config != nullptr, "parse neutral catalog")) return false;
    auto* oldConfig = legoGlobs.config;
    legoGlobs.config = config;
    lws_initialise(nullptr, nullptr, nullptr, nullptr);
    const bool ok = CHECK(lego_entity_building_types_load(), "load building templates") &&
                    CHECK(stats_initialise(config, "Lego"), "load costs and power stats");
    if (ok) help_window_initialise(config, "Lego");
    legoGlobs.config = oldConfig;
    block_close(config);
    return ok;
}

struct Search {
    int type;
    int x, y;
    Lego_Object* result = nullptr;
    unsigned count = 0;
};

int FindCallback(Lego_Object* obj, void* data)
{
    auto& search = *static_cast<Search*>(data);
    if (obj->type != search.type) return 0;
    int x = -1, y = -1;
    if (search.x >= 0 && (!legoobject_position_to_terrain_cell(obj, &x, &y) ||
        x != search.x || y != search.y)) return 0;
    ++search.count;
    search.result = obj;
    return 0;
}

Search Find(int type, int x = -1, int y = -1)
{
    Search search{type, x, y};
    legoobject_run_through_lists_skip_upgrade_parts(FindCallback, &search);
    return search;
}

void Place(Lego_Object* obj, int x, int y)
{
    float wx, wy;
    terrain_map_block_to_world(Level()->map, x, y, &wx, &wy);
    legoobject_world_position_set(obj, wx, wy, 0, 1);
}

unsigned Requests(unsigned handle, bool canceled)
{
    unsigned count = 0;
    for (AITask* t = aiTaskGlobs.globalHead; t; t = t->next)
        if (t->type == 7 && t->zone == static_cast<int>(handle) &&
            bool(t->flags & 0x20) == canceled) ++count;
    return count;
}

bool Deliver(unsigned handle, Lego_Object* resource, int x, int y)
{
    // Select the real requested task. A neutral courier supplies the
    // carrying record at the pickup-completion boundary, then follows
    // the real delivery route to the zone.
    AITask* prev = nullptr;
    AITask* task = aiTaskGlobs.globalHead;
    while (task && !(task->type == 7 && task->zone == static_cast<int>(handle) &&
           task->field_20 == resource->type && task->field_24 == resource->id)) {
        prev = task;
        task = task->next;
    }
    if (!CHECK(task != nullptr, "matching delivery request")) return false;
    StatsData stats = {};
    stats.flags1 = 0x2000000; // Traverses dry terrain.
    stats.RouteSpeed = 4;
    stats.RubbleCoef = stats.PathCoef = 1;
    AnimationObject model = {};
    model.container = Container_Create(static_cast<Container*>(legoGlobs.rootCont));
    if (!CHECK(model.container != nullptr, "generic courier frame")) return false;
    Lego_Object courier = {};
    courier.type = 2;
    courier.stats = &stats;
    courier.miniFigure = &model;
    courier.health = courier.energy = 100;
    courier.flags3 = 0x20400060;
    const int startX = x == 1 ? 2 : 1;
    Place(&courier, startX, y);
    Place(resource, startX, y);
    courier.flags1 = 0x400;
    courier.numCarriedObjects = 1;
    courier.carriedObjects[0] = resource;
    resource->carryingThisObject = &courier;
    // Standing in for a completed pickup, retire its pending collection work.
    aitask_pending_record_callback_dispatch(resource);
    aitask_entity_record_attach_and_relink(task, prev, &courier);
    CHECK_EQ(courier.aiTask, task);
    float elapsed = 1.0f;
    aitask_cmd_entity_command_chain_tick(&courier, &elapsed);
    if (!CHECK(courier.routeBlocks != nullptr, "real delivery route")) return false;
    for (unsigned frame = 0; courier.routeBlocks && frame < 100; ++frame)
        legoobject_path_step_advance_and_arrival_check(&courier, elapsed);
    CHECK_EQ(courier.routeBlocks, nullptr);
    CHECK(courier.flags1 & 0x80000, "delivery arrival requests drop");
    int bx = -1, by = -1;
    CHECK(legoobject_position_to_terrain_cell(&courier, &bx, &by), "courier within map");
    CHECK_EQ(bx, x);
    CHECK_EQ(by, y);
    // A generic frame has no carry animation/null. Supply the resource
    // transform at its destination, then run real drop completion.
    Place(resource, x, y);
    legoobject_attached_object_list_detach(&courier, 1);
    CHECK_EQ(courier.numCarriedObjects, 0u);
    CHECK_EQ(courier.carriedObjects[0], nullptr);
    CHECK_EQ(resource->carryingThisObject, nullptr);
    CHECK_EQ(courier.aiTask, nullptr);
    CHECK(resource->flags4 & Placed, "delivery transfers resource to zone");
    bubble_remove_object(&courier);
    Container_Remove(static_cast<Container*>(model.container));
    return true;
}

Lego_Object* BuildImmediate(int id, int x, int y)
{
    int origin[] = {x, y};
    int shape[] = {x, y};
    auto* obj = static_cast<Lego_Object*>(construction_place_building(id, origin, 0, shape, 1, 0));
    if (!CHECK(obj != nullptr, "real building created")) return nullptr;
    CHECK_EQ(obj->type, 4);
    CHECK_EQ(obj->id, id);
    CHECK(Cell(x, y).flags1 & Occupied, "footprint occupied");
    CHECK(obj->building != &static_cast<BuildingModel*>(legoGlobs.buildingData)[id], "live model cloned");
    CHECK_EQ(Container_GetUserData(static_cast<Container*>(obj->building->contAct)), obj);
    return obj;
}

Lego_Object* BuildWithResources()
{
    auto* ore = Find(7).result;
    auto* crystal = Find(6).result;
    if (!CHECK(ore && crystal, "generated loose resources")) return nullptr;

    // Cancel a partially delivered zone; the resource is released, the
    // placement request is canceled and the cell can be reused.
    int cancelPos[] = {2, 5};
    unsigned canceled = construction_zone_start_building(Consumer, cancelPos, 0, cancelPos, 1);
    construction_zone_request_resource(cancelPos, 7, 0, 0, 1);
    construction_zone_request_resource(cancelPos, 6, 0, 0, 1);
    CHECK_EQ(Requests(canceled, false), 2u);
    if (!Deliver(canceled, ore, 2, 5)) return nullptr;
    CHECK_EQ(Requests(canceled, false), 1u);
    auto* zone = construction_zone_find_by_handle_or_at_block(cancelPos, nullptr);
    if (!CHECK(zone != nullptr, "partial zone")) return nullptr;
    CHECK_EQ(zone->placedCount, 1u);
    CHECK_EQ(zone->placedObjects[0], ore);
    CHECK_EQ(zone->flags & CONSTRUCTION_FLAG_READY, 0u);
    construction_zone_cancel_building(cancelPos);
    CHECK_EQ(construction_zone_exists_at_block(cancelPos), 0);
    CHECK_EQ(Cell(2, 5).construct, nullptr);
    CHECK_EQ(Cell(2, 5).flags1 & Zone, 0u);
    CHECK(ore->health > 0, "released ore survives cancellation");
    CHECK_EQ(ore->flags4 & Placed, 0u);
    CHECK_EQ(Requests(canceled, true), 1u);
    unsigned refunds = 0;
    for (AITask* t = aiTaskGlobs.globalHead; t; t = t->next)
        if (t->type == 3 && t->object == ore && !(t->flags & 0x20)) ++refunds;
    CHECK_EQ(refunds, 1u);

    // Request and deliver both materials through the live zone.
    int origin[] = {1, 4};
    unsigned handle = construction_zone_start_building(Consumer, origin, 0, origin, 1);
    construction_zone_request_resource(origin, 7, 0, 0, 1);
    construction_zone_request_resource(origin, 6, 0, 0, 1);
    CHECK_EQ(Requests(handle, false), 2u);
    zone = construction_zone_find_by_handle_or_at_block(origin, nullptr);
    if (!CHECK(zone != nullptr, "construction zone exists")) return nullptr;
    CHECK_EQ(Cell(1, 4).construct, zone);
    CHECK_EQ(zone->requestCount, 2u);
    CHECK_EQ(construction_zone_needs_more_of_resource(handle, 7, 0), 1);
    if (!Deliver(handle, ore, 1, 4)) return nullptr;
    CHECK_EQ(Requests(handle, false), 1u);
    CHECK_EQ(construction_zone_needs_more_of_resource(handle, 7, 0), 0);
    construction_update_all(1);
    CHECK_EQ(Find(4, 1, 4).count, 0u);
    if (!Deliver(handle, crystal, 1, 4)) return nullptr;
    CHECK_EQ(Requests(handle, false), 0u);
    CHECK_EQ(zone->placedCount, 2u);
    CHECK_EQ(zone->placedObjects[0], ore);
    CHECK_EQ(zone->placedObjects[1], crystal);
    CHECK(zone->flags & CONSTRUCTION_FLAG_READY, "all resources delivered");
    // A real loose object blocking the origin delays creation.
    float x, y;
    terrain_map_block_to_world(Level()->map, 1, 4, &x, &y);
    auto* blocker = legoobject_create_in_world(legoGlobs.contOresTable, 7, 0, 0, x, y, 0);
    if (!CHECK(blocker != nullptr, "physical obstruction")) return nullptr;
    construction_update_all(1);
    CHECK_EQ(construction_zone_exists_at_block(origin), 1);
    CHECK_EQ(Find(4, 1, 4).count, 0u);
    Place(blocker, 2, 2);
    construction_update_all(1);
    CHECK_EQ(construction_zone_exists_at_block(origin), 0);
    auto* obj = Find(4, 1, 4).result;
    if (!CHECK(obj != nullptr, "construction creates building")) return nullptr;
    CHECK_EQ(Find(4, 1, 4).count, 1u);
    CHECK_EQ(obj->id, Consumer);
    CHECK_EQ(Container_GetUserData(static_cast<Container*>(obj->building->contAct)), obj);
    CHECK(obj->building->contPowerLevel != nullptr, "constructed power scene cloned");
    construction_update_all(1);
    CHECK_EQ(Find(4, 1, 4).count, 1u);
    CHECK(Cell(1, 4).flags1 & Occupied, "constructed footprint occupied");
    CHECK_EQ(ore->health, -1.0f);
    CHECK_EQ(crystal->health, -1.0f);
    CHECK(obj->flags1 & Teleporting, "construction begins arrival");
    for (unsigned i = 0; i < 8 && (obj->flags1 & Teleporting); ++i)
        legoobject_update_all(20.0f);
    CHECK_EQ(obj->flags1 & Teleporting, 0u);
    CHECK_EQ(Find(7).count, 1u); // Only the obstruction moved aside.
    CHECK_EQ(Find(6).count, 0u);
    return obj;
}

void MakePath(int x, int y)
{
    int pos[] = {x, y};
    for (unsigned i = 0; i < 4 && !(Cell(x, y).flags1 & 0x200000); ++i)
        terrain_cell_state_update_current(pos);
    CHECK_EQ(terrain_cell_variant_flag_propagate_current(pos), 1);
    CHECK(Cell(x, y).flags1 & Path, "path created");
}

void RefreshPower()
{
    legoobject_activity_scene_state_update();
    legoobject_update_all(1.0f);
}

bool Active(Lego_Object* obj) { return legoobject_shared_eligibility_check(obj, 0) != 0; }

void CheckPower(Lego_Object* source, Lego_Object* a, Lego_Object* b)
{
    Level()->f_08c = 5;
    RefreshPower();
    CHECK(Active(source), "source active");
    CHECK(Active(a) && Active(b), "both consumers connected");
    CHECK_EQ(Level()->f_098, 4);
    CHECK_EQ(noop_callback(0), 1);
    CHECK_EQ(noop_callback(1), 5);
    RefreshPower();
    CHECK_EQ(Level()->f_098, 4);
    CHECK_EQ(Level()->f_08c, 5);
    CHECK(Cell(1, 2).flags2 & Powered, "first connector powered");
    CHECK(Cell(1, 3).flags2 & Powered, "second connector powered");

    // Removing one connector breaks the network, restoring it restores
    // both demand and the same budget (there is no double debit).
    int cut[] = {1, 2};
    map_cell_tracking_flags_clear_and_refresh_current(cut);
    RefreshPower();
    CHECK(!Active(a) && !Active(b), "disconnected consumers inactive");
    CHECK_EQ(Level()->f_098, 0);
    CHECK_EQ(Cell(1, 3).flags2 & Powered, 0u);
    MakePath(1, 2);
    RefreshPower();
    CHECK(Active(a) && Active(b), "reconnected consumers active");
    CHECK_EQ(Level()->f_098, 4);

    Level()->f_08c = 3;
    RefreshPower();
    CHECK_EQ(unsigned(Active(a)) + unsigned(Active(b)), 1u);
    CHECK_EQ(Level()->f_098, 2);
    CHECK_EQ(noop_callback(0), 1);
    Level()->noDrain = 1;
    RefreshPower();
    CHECK(Active(a) && Active(b), "no-drain mode bypasses budget");
    CHECK_EQ(Level()->f_098, 0);
    CHECK_EQ(Level()->f_08c, 3);
    Level()->noDrain = 0;
    Level()->f_08c = 5;

    legoobject_flag80000000_set_or_clear(source, 0);
    RefreshPower();
    CHECK(!Active(source) && !Active(a) && !Active(b), "source disabled");
    CHECK_EQ(Level()->f_098, 0);
    legoobject_flag80000000_set_or_clear(source, 1);
    RefreshPower();
    CHECK(Active(source) && Active(a) && Active(b), "source restored");
    CHECK_EQ(Level()->f_098, 4);
}

void Scenario()
{
    Level()->f_08c = 5;
    auto* source = BuildImmediate(Producer, 1, 1);
    if (!source) return;
    CHECK(Cell(1, 1).flags2 & SourceCell, "source cell registered");
    MakePath(1, 2);
    MakePath(1, 3);
    auto* a = BuildWithResources();
    if (!a) return;
    auto* b = BuildImmediate(Consumer, 2, 4);
    if (!b) return;
    CheckPower(source, a, b);

    // Removal releases the real footprint and returns the configured cost.
    const auto oreBefore = Find(7).count;
    const auto crystalBefore = Find(6).count;
    building_remove_from_terrain(a);
    CHECK_EQ(Cell(1, 4).flags1 & Occupied, 0u);
    CHECK_EQ(Find(7).count, oreBefore + 1);
    CHECK_EQ(Find(6).count, crystalBefore + 1);
    building_drop_cost(a);
    CHECK_EQ(Find(7).count, oreBefore + 1);
    CHECK_EQ(Find(6).count, crystalBefore + 1);
    object_tracked_replace_and_remove(a);
    CHECK_EQ(Find(4, 1, 4).count, 0u);
    CHECK_EQ(Find(4).count, 2u);
    RefreshPower();
    CHECK_EQ(Level()->f_098, 0); // The other consumer was connected through a.
    CHECK(!Active(b), "removed connector no longer supplies the other consumer");
    CHECK(Active(source), "source remains operational");
    building_remove_from_terrain(source);
    object_tracked_replace_and_remove(source);
    CHECK_EQ(Cell(1, 1).flags2 & SourceCell, 0u);
    CHECK_EQ(Cell(1, 1).flags1 & Occupied, 0u);
    CHECK_EQ(Find(4).count, 1u);
    CHECK_EQ(Find(7).count, oreBefore + 1);
    CHECK_EQ(Find(6).count, crystalBefore + 1);
    RefreshPower();
    CHECK_EQ(Cell(1, 2).flags2 & Powered, 0u);
    CHECK_EQ(Level()->f_098, 0);
}
} // namespace

int main()
{
    if (!CHECK(make_fixture("live_building_power_fixture"), "generate neutral fixture") ||
        !initialise_headless("LiveBuildingPower") || !boot_game()) return lrr_test_result();
    crt_srand(912);
    if (CHECK(lego_level_load("Level01"), "load generated level") && LoadCatalog())
        Scenario();
    return lrr_test_result("live_building_power");
}
