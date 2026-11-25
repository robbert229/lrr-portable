// SPDX-License-Identifier: LGPL-3.0-or-later
/* Player input to real selection, message and simulation units, using
 * generated neutral terrain and generic scene objects. */
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>
#include "gods98/input_key_names.h"
#include "gods98/input_keys.h"
#include "gods98/input_globs.h"
#include "gods98/main_globs.h"
#include "gods98/directdraw.h"
#include "gods98/images.h"
#include "gods98/viewports_link_names.h"
#include "gods98/viewports.h"
#include "game/aitask.h"
#include "game/camera_control.h"
#include "game/aitask_globs.h"
#include "game/aitask_record.h"
#include "game/entity_stats.h"
#include "game/lego_unit.h"
#include "game/legoobject_unit.h"
#include "game/objective_unit_link_names.h"
#include "game/objective_unit.h"
#include "game/message_unit_link_names.h"
#include "game/message_unit.h"
#include "game/pointer_link_names.h"
#include "game/pointer.h"
#include "game/ptl.h"
#include "test/synthetic_level_fixture.h"
#include "game/terrain_map.h"
#include "test/lrr_test.h"
#include "test/synthetic_initializer_fixture.h"
#include "game/lego_globs.h"
#include "crt/vc5names.h"

extern "C" {
void camera_set_topdown_position(LegoCamera*, float, float);
void camera_set_tilt(LegoCamera*, float);
void camera_set_zoom(LegoCamera*, float);
void camera_set_zoom_range(LegoCamera*, float, float);
void camera_set_tilt_range(LegoCamera*, float, float);
}
struct Vector3F { float x, y, z; };

namespace {
using namespace synthetic_fixture;
constexpr unsigned Selected = 0x200000, Selectable = 0x100, Paused = 0x100000;
constexpr unsigned Dry = 0x2000000, Driven = 0x20000;
constexpr char Stats[] =
    " Figure { Levels 1 CollRadius 7 CollHeight 4 PickSphere 9 CrossLand TRUE RouteSpeed 8 "
    " CanClearRubble TRUE NumOfToolsCanCarry 2 } ";

Lego_Level* Level() { return legoGlobs.currLevel; }

Lego_Object* Figure(int x, int y)
{
    float wx, wy;
    terrain_map_block_to_world(Level()->map, x, y, &wx, &wy);
    auto* obj = legoobject_create_in_world(legoGlobs.miniFigureData, 2, 0, 0, wx, wy, 0);
    CHECK(obj != nullptr, "create real neutral figure");
    return obj;
}

void Select(Lego_Object* obj)
{
    message_select_object2(obj, 1, 0);
    CHECK(obj->flags3 & Selected, "selection membership bit set");
}

void CheckOnly(Lego_Object* obj)
{
    CHECK_EQ(message_get_num_selected_units(), 1u);
    CHECK_EQ(message_get_primary_selected_unit(), obj);
    CHECK(obj->flags3 & Selected, "retained selection membership");
}

void CheckMove(Lego_Object* obj, int x, int y)
{
    if (CHECK(obj->aiTask != nullptr, "command attached")) {
        AITask* task = obj->aiTask;
        while (task && task->type != 0) task = task->next;
        if (CHECK(task != nullptr, "movement in command chain")) {
            CHECK_EQ(task->blockX, x);
            CHECK_EQ(task->blockY, y);
        }
    }
}

void SelectionAndQueue()
{
    auto* a = Figure(1, 1);
    auto* b = Figure(2, 3);
    if (!a || !b) return;

    // Test the heterogeneous selection classifier with neutral records.
    StatsData dry = {}, floating = {}, drivable = {}, station = {};
    dry.flags1 = Dry;
    drivable.flags1 = Dry | Driven;
    Lego_Object vehicle = {}, waterVehicle = {}, empty = {}, building = {}, fence = {};
    for (Lego_Object* obj : {&vehicle, &waterVehicle, &empty, &building, &fence}) {
        obj->health = 100;
        obj->flags3 = Selectable;
    }
    vehicle.type = waterVehicle.type = empty.type = 1;
    vehicle.stats = &dry;
    waterVehicle.stats = &floating;
    empty.stats = &drivable;
    building.type = 4;
    building.stats = &station;
    fence.type = 11;
    fence.stats = &station;
    fence.flags2 = 0x2000000;

    CHECK_EQ(message_can_select(a), 1);
    a->flags2 |= 8;
    CHECK_EQ(message_can_select(a), 0);
    message_select_object2(a, 1, 0);
    CHECK_EQ(message_get_num_selected_units(), 0u);
    a->flags2 &= ~8u;
    a->health = -1;
    CHECK_EQ(message_can_select(a), 0);
    a->health = 100;
    empty.flags1 = 0x40000;
    CHECK_EQ(message_can_select(&empty), 0);
    empty.flags1 = 0;

    // On-foot units take precedence over vehicle/building/fence classes.
    Select(&empty);
    Select(&building);
    Select(a);
    Select(b);
    message_post_event(MESSAGE_REDUCESELECTION, nullptr, 0, nullptr);
    message_update();
    CHECK_EQ(message_get_num_selected_units(), 2u);
    CHECK_EQ(empty.flags3 & Selected, 0u);
    CHECK_EQ(building.flags3 & Selected, 0u);
    CHECK(message_is_unit_selected(a, nullptr), "first figure retained");
    CHECK(message_is_unit_selected(b, nullptr), "second figure retained");

    Lego_Object** snapshot = nullptr;
    unsigned snapshotCount = 0;
    CHECK_EQ(message_copy_selected_units(&snapshot, &snapshotCount), 1);
    CHECK_EQ(snapshotCount, 2u);
    message_post_event(MESSAGE_DESELECT, a, 0, nullptr);
    message_update();
    CheckOnly(b);
    CHECK_EQ(a->flags3 & Selected, 0u);
    if (CHECK(snapshot != nullptr, "snapshot allocated")) {
        CHECK(snapshot[0] == a || snapshot[1] == a, "snapshot independent of deselection");
        free(snapshot);
    }
    message_post_event(MESSAGE_CLEARSELECTION, nullptr, 0, nullptr);
    message_update();
    CHECK_EQ(message_get_num_selected_units(), 0u);

    // The selection reduction orders vehicle categories, then structures
    // and stationary fence records. Exercise each surviving class.
    Select(&waterVehicle); Select(&vehicle); Select(&empty); Select(&building);
    message_reduce_selected_units();
    CHECK_EQ(message_get_num_selected_units(), 2u);
    CHECK(message_is_unit_selected(&vehicle, nullptr), "preferred vehicle retained");
    CHECK(message_is_unit_selected(&waterVehicle, nullptr), "other non-drivable vehicle retained");
    CHECK_EQ(empty.flags3 & Selected, 0u);
    CHECK_EQ(building.flags3 & Selected, 0u);
    message_clear_selected_units();
    Select(&waterVehicle); Select(&empty); Select(&building);
    message_reduce_selected_units();
    CheckOnly(&waterVehicle);
    message_clear_selected_units();
    Select(&empty); Select(&building); Select(&fence);
    message_reduce_selected_units();
    CheckOnly(&empty);
    message_clear_selected_units();
    Select(&fence); Select(&building);
    message_reduce_selected_units();
    CheckOnly(&building);
    message_clear_selected_units();
    Select(&fence);
    message_reduce_selected_units();
    CheckOnly(&fence);
    message_clear_selected_units();

    // A selection predicate must distinguish an empty list from one with
    // an eligible member, and must continue past an ineligible first member.
    CHECK_EQ(selected_entities_any_predicate_match(4, 3, 1), 0);
    CHECK_EQ(lego_any_object_can_occupy_terrain_cell(3, 3), 0);
    const auto aFlags = a->flags3;
    const auto bFlags = b->flags3;
    const auto statsFlags = a->stats->flags1;
    const float drillTime = a->stats->DrillTimes[4];
    a->stats->flags1 |= 1;
    a->stats->DrillTimes[4] = 5;
    Select(a);
    a->flags3 &= ~6u;
    CHECK_EQ(selected_entities_any_predicate_match(4, 3, 1), 0);
    CHECK_EQ(lego_any_object_can_occupy_terrain_cell(3, 3), 0);
    Select(b);
    b->flags3 |= 6;
    CHECK_EQ(selected_entities_any_predicate_match(4, 3, 1), 1);
    CHECK_EQ(selected_entities_any_predicate_match(2, 5, 1), 0);
    CHECK_EQ(lego_any_object_can_occupy_terrain_cell(3, 3), 1);
    CHECK_EQ(lego_any_object_can_occupy_terrain_cell(0, 5), 0);
    message_clear_selected_units();
    a->flags3 = aFlags;
    b->flags3 = bFlags;
    a->stats->flags1 = statsFlags;
    a->stats->DrillTimes[4] = drillTime;

    // Commands from actual selected objects, including clearing selection
    // after dispatch. The queue owns a copy of the target coordinates.
    Select(a);
    Select(b);
    int goal[] = {1, 5};
    message_post_event(MESSAGE_USERGOTO, nullptr, 1, goal);
    message_post_event(MESSAGE_CLEARSELECTION, nullptr, 0, nullptr);
    goal[1] = 1;
    CHECK_EQ(a->aiTask, nullptr);
    CHECK_EQ(b->aiTask, nullptr);
    message_update();
    CHECK_EQ(message_get_num_selected_units(), 0u);
    CheckMove(a, 1, 5);
    CheckMove(b, 1, 5);

    // A queued direct command canceled before dispatch cannot append.
    int next[] = {2, 1};
    AITask* aTail = aitask_cmd_chain_walk(a);
    message_post_event(MESSAGE_GOTO, a, 0, next);
    message_remove_events_with_object(a);
    message_post_event(MESSAGE_GOTO, b, 0, next);
    message_update();
    CHECK_EQ(aitask_cmd_chain_walk(a), aTail);
    auto* bTail = aitask_cmd_chain_walk(b);
    if (CHECK(bTail != nullptr, "second command queued")) {
        CHECK_EQ(bTail->blockX, 2);
        CHECK_EQ(bTail->blockY, 1);
    }
    message_update();
    CHECK_EQ(aitask_cmd_chain_walk(b), bTail);

    // Install a synthetic event translation for the two completion types.
    // Resource generation posts follow-up collection events. They are
    // dispatched from the other queue bank on the next tick, exactly once.
    if (!CHECK(write_text("Data/commands.ptl",
        "Lego { GenerateCrystalComplete CollectCrystal GenerateOreComplete CollectOre }"),
        "generate command translation table") ||
        !CHECK(ptl_initialise("commands.ptl", "Lego"), "load synthetic translation table")) return;
    int resourceCell[] = {1, 5};
    auto& cell = Level()->blocks[5 * Level()->width + 1];
    const auto crystalCount = cell.f_2c;
    const auto oreCount = cell.f_30;
    crt_srand(2468);
    message_post_event(MESSAGE_GENERATECRYSTALANDORE, nullptr, 0, resourceCell);
    message_update();
    CHECK_EQ(cell.f_2c, crystalCount + 1);
    CHECK_EQ(cell.f_30, oreCount + 1);
    Lego_Object* resources[2] = {};
    legoobject_run_through_lists([](Lego_Object* obj, void* data) {
        if (obj->type != 6 && obj->type != 7) return 0;
        int x, y;
        legoobject_position_to_terrain_cell(obj, &x, &y);
        if (x == 1 && y == 5) static_cast<Lego_Object**>(data)[obj->type - 6] = obj;
        return 0;
    }, resources, 0);
    if (!CHECK(resources[0] && resources[1], "generated pair exists at queued coordinates")) return;
    auto pending = [&](Lego_Object* resource) {
        unsigned count = 0;
        for (auto* task = aiTaskGlobs.globalHead; task; task = task->next)
            if (task->type == 3 && task->object == resource) ++count;
        return count;
    };
    for (auto* resource : resources) CHECK_EQ(pending(resource), 0u);
    message_update();
    for (auto* resource : resources) CHECK_EQ(pending(resource), 1u);
    message_update();
    for (auto* resource : resources) CHECK_EQ(pending(resource), 1u);
}

struct Pixel { unsigned x, y; };

bool PixelFor(int bx, int by, Lego_Object* object, Pixel& out)
{
    auto* map = Level()->map;
    Vector3F world = {};
    terrain_map_block_to_world(map, bx, by, &world.x, &world.y);
    world.z = terrain_map_world_z_get(map, world.x, world.y);
    if (object) world.z -= object->stats->CollHeight * 0.5f;
    Point2F screen = {};
    viewport_world_to_screen(static_cast<Viewport*>(legoGlobs.viewMain), &screen, &world);
    if (!CHECK(std::isfinite(screen.x) && std::isfinite(screen.y), "target projects to finite screen position")) return false;
    // Pick a nearby integral pixel using the real terrain and object
    // pickers, rather than relying on a fixed window size or coordinates.
    for (int dy = -4; dy <= 4; ++dy) {
        for (int dx = -4; dx <= 4; ++dx) {
            int x = int(std::round(screen.x)) + dx;
            int y = int(std::round(screen.y)) + dy;
            if (x < 0 || y < 0 || x >= int(mainGlobs.appWidth) || y >= int(mainGlobs.appHeight)) continue;
            unsigned px = 0, py = 0;
            TerrainMapVector3 picked = {};
            int pickedCell = terrain_map_screen_pick(map, legoGlobs.viewMain, x, y, &px, &py, &picked);
            if (!pickedCell || px != unsigned(bx) || py != unsigned(by)) continue;
            Lego_Object* hit = nullptr;
            legoobject_pick_at_screen_position(x, y, &hit);
            if (hit != object) continue;
            out = {unsigned(x), unsigned(y)};
            return true;
        }
    }
    std::fprintf(stderr, "cell %d,%d projected to %f,%f (screen %u,%u)\n", bx, by, screen.x, screen.y, mainGlobs.appWidth, mainGlobs.appHeight);
    return CHECK(false, "projected cell and object can be picked");
}

void PointerFrame(Pixel p, bool pressed, bool shift = false, int dx = 0, int dy = 0)
{
    inputGlobs.msx = p.x;
    inputGlobs.msy = p.y;
    inputGlobs.mslb = pressed;
    inputGlobs.msrb = 0;
    inputGlobs.diffX = dx;
    inputGlobs.diffY = dy;
    lego_main_sim_interaction_and_cell_update(1, 1, 0, 0, shift);
}

void KeyStep()
{
    int t = -1, r = -1, shift = -1;
    CHECK_EQ(lego_main_sim_step(1, 1, &t, &r, &shift), 1);
    CHECK_EQ(t, 0);
    CHECK_EQ(r, 0);
}

void PointerAndTime()
{
    // The shared fixture's GridA disables rectangle selection; allow
    // the normal multi-selection path in this scenario.
    s_panel_highlight_flags &= ~0x2000u;
    auto* a = Figure(2, 3);
    auto* b = Figure(2, 4);
    if (!a || !b) return;
    float cx, cy;
    terrain_map_block_to_world(Level()->map, 2, 4, &cx, &cy);
    camera_set_topdown_position(legoGlobs.cameraMain, cx, cy);
    camera_set_tilt_range(legoGlobs.cameraMain, 0, 1);
    camera_set_zoom_range(legoGlobs.cameraMain, 100, 400);
    camera_set_tilt(legoGlobs.cameraMain, 0.3f);
    camera_set_zoom(legoGlobs.cameraMain, 180);
    viewport_set_back_clip(static_cast<Viewport*>(legoGlobs.viewMain), 1000);
    CHECK(lego_engine_tick(1), "initial camera/render tick");
    objective_stop_showing_objective();
    CHECK_EQ(objective_is_ended(), 0);
    Pixel ap{}, bp{}, floor{}, wall{};
    if (!PixelFor(2, 3, a, ap) || !PixelFor(2, 4, b, bp) ||
        !PixelFor(2, 5, nullptr, floor) || !PixelFor(3, 4, nullptr, wall)) return;

    PointerFrame(ap, false);
    PointerFrame(ap, true);
    CHECK_EQ(message_get_num_selected_units(), 0u);
    PointerFrame(ap, false);
    CHECK_EQ(message_get_num_selected_units(), 0u);
    message_update();
    CheckOnly(a);
    pointer_update(20);
    PointerFrame(bp, true, true);
    PointerFrame(bp, false, true);
    message_update();
    CHECK_EQ(message_get_num_selected_units(), 2u);
    CHECK(message_is_unit_selected(a, nullptr), "shift keeps first selection");
    CHECK(message_is_unit_selected(b, nullptr), "shift adds second selection");

    // Drag a rectangle around the two projected centers. The selection
    // is committed on release, not while the mouse is held.
    message_clear_selected_units();
    pointer_update(20);
    Pixel start{ap.x - 16, ap.y - 16};
    Pixel end{bp.x + 16, bp.y + 16};
    PointerFrame(start, false);
    PointerFrame(start, true);
    PointerFrame(end, true, false, int(end.x) - int(start.x), int(end.y) - int(start.y));
    CHECK(legoGlobs.flags1 & 0x100, "drag entered rectangle mode");
    CHECK_EQ(message_get_num_selected_units(), 0u);
    PointerFrame(end, false);
    message_update();
    CHECK_EQ(message_get_num_selected_units(), 2u);
    CHECK(message_is_unit_selected(a, nullptr), "rectangle contains first object");
    CHECK(message_is_unit_selected(b, nullptr), "rectangle contains second object");

    // Reject a solid cell without changing either selected unit or queue.
    s_global_low_word = 2;
    pointer_update(20);
    PointerFrame(wall, false);
    CHECK_EQ(pointer_get_current_type(), 0x25);
    auto* aBefore = a->aiTask;
    auto* bBefore = b->aiTask;
    PointerFrame(wall, true);
    PointerFrame(wall, false);
    message_update();
    CHECK_EQ(a->aiTask, aBefore);
    CHECK_EQ(b->aiTask, bBefore);
    CHECK_EQ(message_get_num_selected_units(), 2u);

    // Explicit movement mode. A valid floor hover and release dispatch
    // movement to both selected objects through the original message bank.
    s_global_low_word = 2;
    pointer_update(20);
    PointerFrame(floor, false);
    CHECK_EQ(pointer_get_current_type(), 0x27);
    PointerFrame(floor, true);
    PointerFrame(floor, false);
    CHECK_EQ(pointer_get_current_type(), 0x14);
    message_update();
    CHECK_EQ(message_get_num_selected_units(), 0u);
    CheckMove(a, 2, 5);
    CheckMove(b, 2, 5);

    // A held click while paused must not turn into a command on resume.
    Select(a);
    s_global_low_word = 2;
    lego_transition_control(0, 1);
    PointerFrame(floor, true);
    lego_transition_control(0, 0);
    AITask* tail = aitask_cmd_chain_walk(a);
    PointerFrame(floor, false);
    message_update();
    CHECK_EQ(aitask_cmd_chain_walk(a), tail);

    // Real P-key edge toggles pause exactly once; held key does not
    // repeat. A registered command hotkey is gated while paused.
    mainGlobs.active = 1;
    std::memset(s_global_current_keys, 0, 256);
    std::memset(s_global_previous_keys, 0, 256);
    s_global_current_keys[0x19] = 1;
    KeyStep();
    CHECK(legoGlobs.flags1 & Paused, "P pauses simulation");
    s_global_previous_keys[0x19] = 1;
    KeyStep();
    CHECK(legoGlobs.flags1 & Paused, "held key does not toggle");
    int keyGoal[] = {2, 1};
    message_add_message_action(0x18, MESSAGE_GOTO, b, 0, keyGoal);
    s_global_current_keys[0x18] = 1;
    auto* before = aitask_cmd_chain_walk(b);
    message_update();
    CHECK_EQ(aitask_cmd_chain_walk(b), before);
    s_global_current_keys[0x19] = 0;
    KeyStep();
    s_global_previous_keys[0x19] = 0;
    s_global_current_keys[0x19] = 1;
    KeyStep();
    CHECK_EQ(legoGlobs.flags1 & Paused, 0u);
    message_update();
    auto* keyTail = aitask_cmd_chain_walk(b);
    if (CHECK(keyTail != nullptr && keyTail != before, "hotkey dispatched after resume")) {
        CHECK_EQ(keyTail->blockX, 2);
        CHECK_EQ(keyTail->blockY, 1);
    }
    std::memset(s_global_current_keys, 0, 256);
    std::memset(s_global_previous_keys, 0, 256);

    // The same wall rejected as a movement destination is admissible for
    // a drilling command only after a selected object has the capability.
    message_clear_selected_units();
    Select(a);
    a->stats->DrillTimes[4] = 5;
    a->flags3 &= ~2u;
    s_global_low_word = 0x20;
    pointer_update(20);
    auto& target = Level()->blocks[4 * Level()->width + 3];
    CHECK_EQ(target.flags1 & 0x800000, 0u);
    PointerFrame(wall, false);
    CHECK_EQ(pointer_get_current_type(), 0x1d);
    PointerFrame(wall, true);
    PointerFrame(wall, false);
    message_update();
    CHECK_EQ(target.flags1 & 0x800000, 0u);
    CheckOnly(a);
    a->flags3 |= 2;
    pointer_update(20);
    PointerFrame(wall, false);
    CHECK_EQ(pointer_get_current_type(), 0x1f);
    PointerFrame(wall, true);
    PointerFrame(wall, false);
    message_update();
    CHECK(target.flags1 & 0x800000, "accepted wall command highlights target");
    CHECK_EQ(message_get_num_selected_units(), 0u);
    AITask* drill = aiTaskGlobs.globalHead;
    while (drill && (drill->type != 8 || drill->blockX != 3 || drill->blockY != 4))
        drill = drill->next;
    CHECK(drill != nullptr, "real drilling job queued for picked cell");
}
} // namespace

int main(int argc, char** argv)
{
    const std::string scenario = argc == 2 ? argv[1] : "selection";
    if (!CHECK(scenario == "selection" || scenario == "pointer", "known scenario")) return lrr_test_result();
    const std::string directory = "selection_commands_" + scenario;
    if (!initialise_full_game(directory.c_str(), "SelectionCommands", Stats)) return lrr_test_result();
    if (scenario == "selection") SelectionAndQueue();
    else PointerAndTime();
    message_clear_selected_units();
    lego_engine_shutdown();
    CHECK(Level() == nullptr, "shutdown releases level");
    directdraw_shutdown();
    return lrr_test_result("selection_commands");
}
