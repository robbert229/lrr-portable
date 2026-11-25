// SPDX-License-Identifier: LGPL-3.0-or-later
/* Original abstract menu records: load resource-backed menu definitions,
 * typed items and navigation links without reproducing a shipped screen. */
#include <cstdlib>
#include "platform/native/test_fixture.h"
#include "gods98/config.h"
#include "gods98/directdraw.h"
#include "gods98/images_link_names.h"
#include "gods98/images_unit.h"
#include "game/frontend_unit.h"
#include "game/sfx_link_names.h"
#include "game/sfx.h"
#include "test/lrr_test.h"
#include "test/synthetic_level_fixture.h"
#include "game/lego_globs.h"

namespace {
void integer_changed(int) {}
void real_changed(float) {}
void activated() {}

constexpr char MenuConfig[] =
    "Fixture { Menu { Example { MenuCount 2\n"
    " Menu1 { Title Sample_A FullName Example_first Position 13:17 "
    "  MenuFont font.bmp LoFont font.bmp HiFont font.bmp AutoCenter TRUE "
    "  DisplayTitle TRUE CanScroll TRUE Anchored 11:19 MenuImage menu.bmp:2:3 "
    "  Overlay1 tile.bmp:Unknown:5:7 PlayRandom TRUE ItemCount 5\n"
    "  Item1 Cycle:10:20:35:20:Choice:2:One_Value:Other_Value\n"
    "  Item2 Slider:10:30:35:30:Count:1:9\n"
    "  Item3 RealSlider:10:40:35:40:Scale:0.25:2:0.25\n"
    "  Item4 Trigger:10:50:Perform_Action:0\n"
    "  Item5 Next:10:60:Second:Menu2\n"
    " }\n"
    " Menu2 { Title Sample_B FullName Example_second Position 21:23 "
    "  MenuFont font.bmp LoFont font.bmp HiFont font.bmp AutoCenter FALSE "
    "  DisplayTitle FALSE CanScroll FALSE ItemCount 2\n"
    "  Item1 Slider:1:2:3:4:Count:0:10:tile.bmp:tile.bmp:tile.bmp:tile.bmp:tile.bmp:tile.bmp:tile.bmp:tile.bmp\n"
    "  Item2 Next:12:13:First:Menu1:1\n"
    " } } } }";

void check_menu(Front_MenuSet* set, int* choice, int* count, float* scale,
                int* trigger, int* other)
{
    if (!CHECK(set != nullptr, "load generated menu set") || !CHECK_EQ(set->menuCount, 2)) return;
    auto* first = set->menus[0];
    auto* second = set->menus[1];
    if (!CHECK(first && second, "two distinct menu records")) return;
    CHECK_STREQ(first->title, "Sample A");
    CHECK_STREQ(second->fullName, "Example second");
    CHECK_EQ(front_get_menu_id_by_name(set, "sample b"), 1);
    CHECK_EQ(front_get_menu_id_by_name(set, "missing"), -1);
    CHECK_EQ(first->xPos, 13);
    CHECK_EQ(first->yPos, 17);
    CHECK_EQ(first->autoCenter, 1);
    CHECK_EQ(first->displayTitle, 1);
    CHECK_EQ(first->anchored, 1);
    CHECK_EQ(first->anchoredX, 11);
    CHECK_EQ(first->anchoredY, 19);
    CHECK_EQ(first->flags & 3u, 3u);
    CHECK_NEAR(first->imagePos[0], 2.0f, 0.0001f);
    CHECK_NEAR(first->imagePos[1], 3.0f, 0.0001f);
    CHECK(first->menuImage != nullptr, "generated background loaded");
    CHECK(second->menuImage == nullptr, "optional background absent");
    CHECK_EQ(first->menuFont, second->menuFont);
    if (CHECK(first->overlays != nullptr, "generated overlay record")) {
        CHECK_EQ(first->overlays->overlayType, 2);
        CHECK_EQ(first->overlays->x, 5);
        CHECK_EQ(first->overlays->y, 7);
        CHECK_STREQ(first->overlays->filename, "tile.bmp");
    }
    if (!CHECK_EQ(first->itemCount, 5) || !CHECK_EQ(second->itemCount, 2)) return;
    const int expected[] = {Front_MenuItem_Cycle, Front_MenuItem_Slider,
                           Front_MenuItem_RealSlider, Front_MenuItem_Trigger, Front_MenuItem_Next};
    for (int i = 0; i < 5; ++i) CHECK_EQ(first->items[i]->itemType, expected[i]);
    auto* cycle = static_cast<Front_CycleData*>(first->items[0]->itemData);
    if (CHECK(cycle != nullptr, "cycle data")) {
        CHECK_EQ(cycle->valuePtr, choice);
        CHECK(cycle->callback == integer_changed, "cycle callback binding");
        CHECK_EQ(cycle->nameCount, 2);
        CHECK_STREQ(cycle->nameList[0], "One Value");
        CHECK_STREQ(cycle->nameList[1], "Other Value");
    }
    auto* slider = static_cast<Front_SliderData*>(first->items[1]->itemData);
    if (CHECK(slider != nullptr, "slider data")) {
        CHECK_EQ(slider->valuePtr, count);
        CHECK_EQ(slider->valueMin, 1);
        CHECK_EQ(slider->valueMax, 9);
        CHECK(slider->imageBarOff == nullptr, "absent optional slider images");
    }
    auto* real = static_cast<Front_RealSliderData*>(first->items[2]->itemData);
    if (CHECK(real != nullptr, "real slider data")) {
        CHECK_EQ(real->valuePtr, scale);
        CHECK(real->callback == real_changed, "real slider callback binding");
        CHECK_EQ(real->valueMin, 0.25f);
        CHECK_EQ(real->valueMax, 2.0f);
        CHECK_EQ(real->step, 0.25f);
    }
    auto* action = static_cast<Front_TriggerData*>(first->items[3]->itemData);
    if (CHECK(action != nullptr, "trigger data")) {
        CHECK_EQ(action->valuePtr, trigger);
        CHECK(action->callback == activated, "trigger callback binding");
        CHECK_EQ(action->end, 0);
    }
    CHECK_STREQ(first->items[3]->banner, "Perform Action");
    CHECK_EQ(first->items[4]->itemData, second);
    CHECK_EQ(second->items[1]->itemData, first);
    CHECK_EQ(second->items[1]->notInTuto, 1);
    auto* imageSlider = static_cast<Front_SliderData*>(second->items[0]->itemData);
    if (CHECK(imageSlider != nullptr, "resource-backed slider data")) {
        CHECK_EQ(imageSlider->valuePtr, other);
        CHECK(imageSlider->imageBarOff && imageSlider->imageBarOn && imageSlider->imagePlusHi,
              "generated slider images loaded");
        CHECK_EQ(imageSlider->imageBarOff, imageSlider->imageBarOn);
    }
}
}

int main()
{
    using namespace synthetic_fixture;
    if (!CHECK(fixture_enter("menu_resources_fixture") && fixture_mkdir("Data") &&
               write_bmp("Data/font.bmp", 80, 190) && write_bmp("Data/menu.bmp", 16, 16) &&
               write_bmp("Data/tile.bmp", 8, 8), "generate isolated placeholder resources") ||
        !initialise_headless("MenuResources")) return lrr_test_result();
    block_text_token_pool_init();
    Image_Initialise();
    sfx_initialise();
    legoGlobs.gameName = "Fixture";
    legoGlobs.config = block_parse(MenuConfig, sizeof(MenuConfig) - 1);
    if (!CHECK(legoGlobs.config != nullptr, "parse abstract menu config")) return lrr_test_result();
    int choice = 0, count = 4, trigger = 0, other = 5;
    float scale = 1.0f;
    auto* set = front_load_menu_set(static_cast<BlockRec*>(legoGlobs.config), "Example",
                                   &choice, integer_changed, &count, integer_changed,
                                   &scale, real_changed, &trigger, activated, &other, integer_changed);
    check_menu(set, &choice, &count, &scale, &trigger, &other);
    if (set != nullptr) {
        for (int i = 0; i < set->menuCount; ++i) {
            auto* menu = set->menus[i];
            if (menu == nullptr) continue;
            // Release each independently allocated overlay; shared cached
            // surfaces are intentionally left to the isolated process.
            while (menu->overlays != nullptr) {
                auto* overlay = menu->overlays;
                menu->overlays = overlay->next;
                std::free(overlay->filename);
                std::free(overlay);
            }
            front_menu_free_menu(menu);
        }
        std::free(set->menus);
        std::free(set);
    }
    block_close(static_cast<BlockRec*>(legoGlobs.config));
    directdraw_shutdown();
    return lrr_test_result();
}
