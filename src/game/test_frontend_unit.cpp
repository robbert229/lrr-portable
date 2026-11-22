// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_frontend_unit — inclusive and centered menu hit bounds, menu
 * item type lookup, label conversion, and expansion of the item list. */
#include <cstdlib>

#include "game/frontend_unit.h"
#include "test/lrr_test.h"

int main(void)
{
    CHECK_EQ(front_maths_is_point_inside_rect(10, 20, 10, 20, 5, 3), 1);
    CHECK_EQ(front_maths_is_point_inside_rect(15, 23, 10, 20, 5, 3), 1);
    CHECK_EQ(front_maths_is_point_inside_rect(10, 24, 10, 20, 5, 3), 0);
    CHECK_EQ(front_maths_is_point_inside_rect_centered(8, 20, 10, 20, 5, 3, 1), 1);
    CHECK_EQ(front_maths_is_point_inside_rect_centered(8, 20, 10, 20, 5, 3, 0), 0);
    CHECK_EQ(front_maths_is_point_inside_rect_centered(14, 20, 10, 20, 5, 3, 1), 0);
    CHECK_EQ(front_menu_item_parse_type_string("ReAlSlIdEr"), Front_MenuItem_RealSlider);
    CHECK_EQ(front_menu_item_parse_type_string("NEXT"), Front_MenuItem_Next);
    CHECK_EQ(front_menu_item_parse_type_string("unknown"), -1);
    CHECK_STREQ(front_util_replace_text_spaces("Load_Saved_Game"), "Load Saved Game");
    CHECK_STREQ(front_util_replace_text_spaces("Quit"), "Quit");

    Front_Menu menu = {};
    Front_MenuItem items[16] = {};
    CHECK_EQ(front_menu_add_menu_item(&menu, nullptr), 0);
    CHECK_EQ(menu.itemCount, 0);
    for (auto& item : items) {
        if (!CHECK_EQ(front_menu_add_menu_item(&menu, &item), 1)) {
            std::free(menu.items);
            return lrr_test_result("frontend_unit");
        }
    }
    CHECK_EQ(menu.itemCount, 16);
    for (int i = 0; i < menu.itemCount; ++i)
        CHECK_EQ(menu.items[i], &items[i]);
    std::free(menu.items);
    return lrr_test_result("frontend_unit");
}
