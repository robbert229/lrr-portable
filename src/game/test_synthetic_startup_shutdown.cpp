// SPDX-License-Identifier: LGPL-3.0-or-later
#include <string>
#include "gods98/config.h"
#include "gods98/container_link_names.h"
#include "gods98/container_unit.h"
#include "gods98/directdraw.h"
#include "gods98/viewports_link_names.h"
#include "gods98/viewports.h"
#include "game/camera_control.h"
#include "game/interface_unit.h"
#include "game/reward.h"
#include "platform/native/test_fixture.h"
#include "test/lrr_test.h"
#include "test/synthetic_level_fixture.h"

extern "C" LegoCamera* camera_create(void* root, int type);
extern "C" void camera_free(LegoCamera* camera);

using namespace synthetic_fixture;

static void check_reward(bool missingImage)
{
    block_text_token_pool_init();
    const std::string config =
        std::string("Fixture { Reward { Images { Score ") +
        (missingImage ? "L\\missing.bmp" : "L\\panel.bmp") +
        "|3|4 } Text { Score Sample_text|5|6 } Fonts { Score L\\font.bmp } } }";
    BlockFile* block = block_parse(config.c_str(), config.size());
    if (!CHECK(block != nullptr, "parse reward data")) return;
    rewardGlobs.config = block;
    rewardGlobs.gameName = "Fixture";
    if (!CHECK(reward_initialise(), "initialise reward records")) return;
    CHECK_EQ(rewardGlobs.itemPart->itemCount, REWARD_ITEM_COUNT);
    const auto& score = rewardGlobs.itemPart->items[REWARD_ITEM_SCORE];
    CHECK_EQ((score.flags & REWARD_ITEM_FLAG_IMAGE) != 0, !missingImage);
    CHECK_EQ(score.image != nullptr, !missingImage);
    CHECK(score.flags & REWARD_ITEM_FLAG_TEXT, "configured text loaded");
    CHECK(score.font != nullptr, "generated font loaded");
    CHECK_EQ(score.imageX, 3);
    CHECK_EQ(score.textY, 6);
    CHECK_STREQ(score.text, "Sample text");
    reward_shutdown();
    block_close(block);
}

static void check_interface()
{
    interface_initialise(12, 18, nullptr);
    CHECK_EQ(interfaceGlobs.currMenuPosition.x, 12.0f);
    CHECK_EQ(interfaceGlobs.currMenuPosition.y, 18.0f);
    int item = -1;
    CHECK(interface_get_menu_item_type("Interface_MenuItem_BackToDefault", &item), "find registered item");
    CHECK_EQ(item, 0);
    CHECK_EQ(interfaceGlobs.menus[0].count, 4u);
    if (CHECK(interfaceGlobs.menus[0].items != nullptr, "main menu has allocated items")) {
        CHECK_EQ(interfaceGlobs.menus[0].items[0], 1);
    }
    interface_shutdown();
}

static void check_scene()
{
    Container* root = Container_Initialise("Fixture");
    if (!CHECK(root != nullptr, "create root")) return;
    viewport_initialise();
    LegoCamera* camera = camera_create(root, 3);
    if (!CHECK(camera != nullptr, "create camera")) return;
    Viewport* view = viewport_create(0, 0, 1, 1, static_cast<Container*>(camera->contCam));
    if (!CHECK(view != nullptr, "create viewport")) return;
    CHECK(viewport_get_camera(view) == camera->contCam, "viewport uses created camera");
    unsigned long width = 0, height = 0;
    viewport_get_size(view, &width, &height);
    CHECK_EQ(width, 320ul);
    CHECK_EQ(height, 200ul);
    Viewport* inset = viewport_create_pixel(4, 6, 80, 60, static_cast<Container*>(camera->contCam));
    if (!CHECK(inset != nullptr, "create inset viewport")) return;
    CHECK(viewport_get_camera(inset) == camera->contCam, "inset uses created camera");
    viewport_get_size(inset, &width, &height);
    CHECK_EQ(width, 80ul);
    CHECK_EQ(height, 60ul);
    viewport_shutdown();
    camera_free(camera);
    Container_Shutdown();
    directdraw_shutdown();
}

int main(int argc, char** argv)
{
    if (!CHECK(argc == 2, "select a subsystem scenario")) return lrr_test_result();
    const std::string scenario = argv[1];
    if (!CHECK(scenario == "reward" || scenario == "reward-missing-image" ||
               scenario == "interface" || scenario == "scene", "known scenario"))
        return lrr_test_result();
    const std::string directory = "synthetic_subsystems_" + scenario;
    if (!CHECK(fixture_enter(directory.c_str()) && fixture_mkdir("Data") && fixture_mkdir("Data/L"),
               "enter isolated scratch directory")) return lrr_test_result();
    if (scenario == "reward" || scenario == "reward-missing-image") {
        if (!CHECK(write_bmp("Data/L/font.bmp", 80, 190), "generate grayscale font placeholder"))
            return lrr_test_result();
        if (scenario == "reward" && !CHECK(write_bmp("Data/L/panel.bmp", 16, 16), "generate grayscale image"))
            return lrr_test_result();
    }
    if (!initialise_headless("SyntheticSubsystems")) return lrr_test_result();

    if (scenario == "scene") {
        check_scene();
    } else {
        if (scenario == "interface") check_interface();
        else check_reward(scenario == "reward-missing-image");
        directdraw_shutdown();
    }
    return lrr_test_result();
}
