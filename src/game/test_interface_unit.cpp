// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_interface_unit — switching menus after the outgoing menu clears,
 * clamping slide positions, and independent fixed, vehicle and building icons. */
#include "game/interface_unit.h"
#include "test/lrr_test.h"

extern "C" int __wrap_sound_alias_play(int, int)
{
    return 0;
}

int main(void)
{
    constexpr int mainMenu = 0;
    constexpr int groundMenu = 1;
    interfaceGlobs.flags = INTERFACE_FLAG_IN | INTERFACE_FLAG_OPEN;
    interfaceGlobs.currMenuType = mainMenu;
    interfaceGlobs.currMenuPosition.x = 8.0f;
    interface_set_slide_positions(8u, 0u, 16u, 0u, 25.0f);

    const Point2I selection = { 4, 7 };
    interface_set_menu(groundMenu, &selection);
    interface_update_slide(4.0f);
    CHECK_EQ(interfaceGlobs.currMenuPosition.x, 12.0f);
    CHECK_EQ(interfaceGlobs.currMenuType, mainMenu);
    interface_update_slide(8.0f);
    CHECK_EQ(interfaceGlobs.currMenuPosition.x, 16.0f);
    CHECK_EQ(interfaceGlobs.currMenuType, groundMenu);
    CHECK_EQ(interfaceGlobs.selBlockPos.x, 4);
    CHECK_EQ(interfaceGlobs.selBlockPos.y, 7);
    interface_update_slide(16.0f);
    CHECK_EQ(interfaceGlobs.currMenuPosition.x, 8.0f);
    CHECK((interfaceGlobs.flags & INTERFACE_FLAG_IN) != 0, "the incoming menu is fully open");

    constexpr int item = 2;
    constexpr int vehicle = 1, building = 4;
    unsigned int vehicleFlags[3] = {}, buildingFlags[3] = {};
    unsigned int vehicleClicks[3] = {}, buildingClicks[3] = {};
    interfaceGlobs.vehicleItemFlags = vehicleFlags;
    interfaceGlobs.buildingItemFlags = buildingFlags;
    interfaceGlobs.vehicleItemClicks = vehicleClicks;
    interfaceGlobs.buildingItemClicks = buildingClicks;
    interface_set_item_pressed(item, 1);
    interface_set_object_item_pressed(building, item, 1);
    CHECK(interface_is_item_pressed(item) != 0, "the fixed icon is pressed");
    CHECK(interface_is_object_item_pressed(building, item) != 0, "the building icon is pressed");
    CHECK_EQ(interface_is_object_item_pressed(vehicle, item), 0u);
    interface_set_item_pressed(item, 0);
    CHECK_EQ(interface_is_item_pressed(item), 0u);
    CHECK_EQ(interface_set_item_clicks(item, 3u), 0u);
    CHECK_EQ(interface_set_object_item_clicks(vehicle, item, 5u), 0u);
    CHECK_EQ(interface_set_object_item_clicks(vehicle, item, 7u), 5u);
    CHECK_EQ(interface_get_item_clicks(item), 3u);
    CHECK_EQ(interface_get_object_item_clicks(vehicle, item), 7u);
    CHECK_EQ(interface_get_object_item_clicks(building, item), 0u);
    return lrr_test_result("interface_unit");
}
