// SPDX-License-Identifier: LGPL-3.0-or-later
/* Existing command admission and resource setup on invented records. The
 * icons are the neutral fixture's two solid bitmaps; no UI is reproduced. */
#include <string>
#include "gods98/directdraw.h"
#include "gods98/input_keys.h"
#include "game/aitask.h"
#include "game/aitask_globs.h"
#include "game/aitask_record.h"
#include "game/construction.h"
#include "game/entity_stats.h"
#include "game/interface_unit.h"
#include "game/lego_unit.h"
#include "game/legoobject_unit.h"
#include "game/message_unit_link_names.h"
#include "game/message_unit.h"
#include "game/terrain_map.h"
#include "test/lrr_test.h"
#include "test/synthetic_initializer_fixture.h"
#include "game/lego_globs.h"

extern "C" void legoobject_activity_grant(Lego_Object*, int);
extern "C" int legoobject_activity_scan(Lego_Object*, int);

namespace {
constexpr char Stats[] =
    " Figure { Levels 1 CollRadius 3 CrossLand TRUE RouteSpeed 5 NumOfToolsCanCarry 2 } "
    " Structure { Levels 1 CollRadius 3 SelfPowered TRUE ToolStore TRUE TrainDriver TRUE TrainRepair TRUE TrainScanner TRUE TrainPilot TRUE TrainSailor TRUE } ";
constexpr char Resources[] =
    " InterfaceImages { "
    " Interface_MenuItem_GetDrill L\\panel.bmp:L\\load.bmp:L\\panel.bmp:Ready|SFX_NULL:Blocked|SFX_NULL:KEY_A:TRUE "
    " Interface_MenuItem_GetSpade L\\load.bmp:L\\panel.bmp:L\\load.bmp:Other:Unavailable:KEY_B "
    " } InterfaceBuildImages { VehicleA L\\panel.bmp:L\\load.bmp:L\\panel.bmp:KEY_C "
    " Structure L\\load.bmp:L\\panel.bmp:L\\load.bmp:KEY_D } "
    " InterfaceSurroundImages { 1 L\\panel.bmp:3:7:11:13:L\\load.bmp:17:19 "
    " 2 NULL:0:0:0:0:L\\panel.bmp:23:29 } "
    " InterfaceBackButton 12:9:L\\panel.bmp:L\\load.bmp:Sample_Back ";

int Item(const char* name)
{
    int type = -1;
    CHECK_EQ(interface_get_menu_item_type(name, &type), 1);
    return type;
}

void ResourcesLoaded()
{
    const int a = Item("Interface_MenuItem_GetDrill");
    const int b = Item("Interface_MenuItem_GetSpade");
    if (a < 0 || b < 0) return;
    auto* ready = interfaceGlobs.menuItemIcons[a];
    auto* inactive = interfaceGlobs.menuItemIconsDisabled[a];
    if (CHECK(ready && inactive, "generated icons loaded")) {
        CHECK_EQ(ready->width, 16u);
        CHECK_EQ(inactive->width, 32u);
    }
    CHECK(interfaceGlobs.menuItemIconsPressed[a] != nullptr, "pressed icon loaded");
    CHECK_STREQ(interfaceGlobs.menuItemTooltips[a], "Ready");
    CHECK_STREQ(interfaceGlobs.menuItemTooltipsDisabled[a], "Blocked");
    CHECK_EQ(interfaceGlobs.menuItemTooltipsUseName[a], 1);
    CHECK_EQ(interfaceGlobs.menuItemTooltipsUseName[b], 0);
    CHECK_EQ(interfaceGlobs.menuItemF2keys[a], 0x1eu);
    CHECK_EQ(interfaceGlobs.menuItemF2keys[b], 0x30u);
    CHECK(interfaceGlobs.vehicleItemIcons[0] != nullptr, "configured vehicle icon");
    CHECK_EQ(interfaceGlobs.vehicleItemIcons[1], nullptr);
    CHECK_EQ(interfaceGlobs.vehicleItemIconsDisabled[1], nullptr);
    CHECK_EQ(interfaceGlobs.vehicleItemIconsPressed[1], nullptr);
    CHECK_EQ(interfaceGlobs.menuItemIcons[Item("Interface_MenuItem_GetHammer")], nullptr);
    CHECK_EQ(interfaceGlobs.vehicleItemF2keys[0], 0x2eu);
    CHECK_EQ(interfaceGlobs.vehicleItemF2keys[1], 0u);
    CHECK(interfaceGlobs.buildingItemIcons[0] != nullptr, "configured building icon");
    CHECK_EQ(interfaceGlobs.buildingItemF2keys[0], 0x20u);
    CHECK(interfaceGlobs.surroundImages[0] != nullptr, "generated surround loaded");
    CHECK(interfaceGlobs.objectSurroundImages[0] != nullptr, "generated overlay loaded");
    CHECK_EQ(interfaceGlobs.surroundImages[1], nullptr);
    CHECK(interfaceGlobs.objectSurroundImages[1] != nullptr, "overlay without surround");
    CHECK_EQ(interfaceGlobs.surroundOffsets[0].x, 3.0f);
    CHECK_EQ(interfaceGlobs.surroundOffsets[0].y, 7.0f);
    CHECK_EQ(interfaceGlobs.backButtonOffsets[0].x, 11.0f);
    CHECK_EQ(interfaceGlobs.backButtonOffsets[0].y, 13.0f);
    CHECK_EQ(interfaceGlobs.objectSurroundOffsets[1].x, 23.0f);
    CHECK_EQ(interfaceGlobs.objectSurroundOffsets[1].y, 29.0f);
    CHECK_EQ(interfaceGlobs.backButtonWidth, 12u);
    CHECK_EQ(interfaceGlobs.backButtonHeight, 9u);
    CHECK_STREQ(interfaceGlobs.backButtonText, "Sample Back");
}

Lego_Object* Figure(int x, int y)
{
    float wx, wy;
    terrain_map_block_to_world(legoGlobs.currLevel->map, x, y, &wx, &wy);
    auto* obj = legoobject_create_in_world(legoGlobs.miniFigureData, 2, 0, 0, wx, wy, 0);
    if (CHECK(obj != nullptr, "instantiate neutral object")) obj->flags3 |= 0x20400160;
    return obj;
}

void Available(int item, bool expected)
{
    CHECK_EQ(interface_is_item_available(item), int(expected));
    interface_update_item_enabled(item);
    CHECK_EQ(bool(interface_is_item_disabled(item, 0)), !expected);
}

void Permissions()
{
    auto* first = Figure(1, 3);
    auto* other = Figure(2, 3);
    if (!first || !other) return;
    message_select_object2(first, 1, 0);
    message_select_object2(other, 1, 0);
    CHECK_EQ(message_get_num_selected_units(), 2u);
    const int toolItems[] = {49, 50, 51, 52};
    CHECK_EQ(interface_any_object_with_flags2_100000(), 0);
    for (int item : toolItems) Available(item, false);
    const auto* rejectedQueue = aiTaskGlobs.globalHead;
    CHECK_EQ(interface_do_action(toolItems[2]), 0);
    CHECK_EQ(aiTaskGlobs.globalHead, rejectedQueue);
    CHECK_EQ(message_get_num_selected_units(), 2u);

    int pos[] = {1, 4};
    auto* station = static_cast<Lego_Object*>(construction_place_building(0, pos, 0, pos, 1, 0));
    if (!CHECK(station != nullptr, "create generic facility")) return;
    legoobject_kind_four_post_create_resolve(station);
    CHECK(legoobject_shared_eligibility_check(station, 0), "active facility");
    CHECK_EQ(interface_any_object_with_flags2_100000(), 1);

    // Objects are born with a first tool. Fill both inventory slots and
    // exercise replacement by oldest; availability scans all selections.
    CHECK_EQ(first->numCarriedTools, 1u);
    CHECK_EQ(legoobject_activity_scan(first, 0), 1);
    Available(toolItems[0], false);
    for (int i = 1; i < 4; ++i) Available(toolItems[i], true);
    legoobject_activity_grant(first, 1);
    CHECK_EQ(first->numCarriedTools, 2u);
    Available(toolItems[1], true); // other still lacks tool
    legoobject_activity_grant(other, 1);
    Available(toolItems[1], false);
    legoobject_activity_grant(first, 2);
    CHECK_EQ(first->numCarriedTools, 2u);
    CHECK_EQ(legoobject_activity_scan(first, 0), 0);
    CHECK_EQ(legoobject_activity_scan(first, 1), 1);
    CHECK_EQ(legoobject_activity_scan(first, 2), 1);
    Available(toolItems[0], true);
    Available(toolItems[2], true);
    legoobject_activity_grant(other, 2);
    Available(toolItems[2], false);
    Available(toolItems[3], true);

    // Only units lacking the selected tool enqueue a task. Both lack the
    // fourth tool, so this dispatch inserts exactly two real requests.
    const auto* before = aiTaskGlobs.globalHead;
    CHECK_EQ(interface_do_action(toolItems[3]), 1);
    unsigned added = 0;
    for (auto* task = aiTaskGlobs.globalHead; task && task != before; task = task->next) {
        CHECK_EQ(task->type, 0x13);
        CHECK_EQ(task->priority, 2);
        CHECK_EQ(reinterpret_cast<uintptr_t>(task->field_38), uintptr_t(3));
        ++added;
    }
    CHECK_EQ(added, 2u);
    message_update();
    CHECK_EQ(message_get_num_selected_units(), 0u);

    // A selected object's eligibility depends on its state and the
    // facility's training capability, not on the drawn menu.
    message_select_object2(first, 1, 0);
    message_select_object2(other, 1, 0);
    first->energy = other->energy = 100;
    Available(31, false);
    other->energy = 60;
    Available(31, true);
    first->health = other->health = 100;
    Available(36, false);
    other->health = 75;
    first->abilityFlags = 16;
    Available(36, true);
    first->abilityFlags = 0;
    first->field_3d4 = 0;
    Available(58, false);
    first->field_3d4 = legoGlobs.currLevel->trainTime;
    Available(58, true);
    first->abilityFlags |= 4;
    Available(58, false);
    first->abilityFlags = 0;
    Available(59, true);
    Available(60, true);
    Available(61, true);
    Available(62, true);
    first->abilityFlags = 0x3f;
    Available(57, false);
    first->abilityFlags = 0x1f;
    Available(57, true);
    message_clear_selected_units();
}
}

int main(int argc, char** argv)
{
    const std::string mode = argc == 2 ? argv[1] : "resources";
    if (!CHECK(mode == "resources" || mode == "permissions", "known scenario") ||
        !synthetic_fixture::initialise_full_game(("command_permissions_" + mode).c_str(),
                                                 "CommandPermissions", Stats, Resources)) return lrr_test_result();
    if (mode == "resources") ResourcesLoaded();
    else Permissions();
    lego_engine_shutdown();
    CHECK_EQ(legoGlobs.currLevel, nullptr);
    directdraw_shutdown();
    return lrr_test_result("command_permissions");
}
