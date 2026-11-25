// SPDX-License-Identifier: LGPL-3.0-or-later
#include <string>
#include <miniwin/d3drm.h>

#include "gods98/container_link_names.h"
#include "gods98/container_unit.h"
#include "gods98/directdraw.h"
#include "gods98/config.h"
#include "gods98/lws.h"
#include "gods98/font_api.h"
#include "gods98/viewports_link_names.h"
#include "gods98/viewports.h"
#include "game/camera_control.h"
#include "game/lego_unit.h"
#include "game/reward.h"
#include "game/interface_unit.h"
#include "game/panel_unit.h"
#include "platform/native/test_fixture.h"
#include "test/lrr_test.h"
#include "test/synthetic_level_fixture.h"
#include "game/lego_globs.h"

extern "C" LegoCamera* camera_create(void* root, int type);
extern "C" void camera_free(LegoCamera* camera);

static void check_parent_first()
{
    Container* root = Container_Initialise("Fixture");
    if (!CHECK(root != nullptr, "create root")) return;
    Container* child = Container_Create(root);
    if (!CHECK(child != nullptr, "create child")) return;
    Container* grandchild = Container_Create(child);
    if (!CHECK(grandchild != nullptr, "create grandchild")) return;
    CHECK_EQ(Container_GetParent(child), root);
    CHECK_EQ(Container_GetParent(grandchild), child);
    IDirect3DRMFrameArray* retainedChildren = nullptr;
    if (!CHECK(Container_GetMasterFrame(root)->GetChildren(&retainedChildren) == DD_OK &&
               retainedChildren, "retain root child array")) return;
    Container_Remove(root);
    CHECK_EQ(Container_GetParent(child), nullptr);
    CHECK_EQ(Container_GetParent(grandchild), child);
    retainedChildren->Release();
    Container_Remove(child);
    CHECK_EQ(Container_GetParent(grandchild), nullptr);
    Container_Remove(grandchild);
    Container_Shutdown();
}

static void check_cameras(bool rootFirst)
{
    Container* root = Container_Initialise("Fixture");
    if (!CHECK(root != nullptr, "create root")) return;
    viewport_initialise();
    LegoCamera* cameras[3] = {};
    Viewport* views[3] = {};
    for (int i = 0; i < 3; ++i) {
        auto* camera = cameras[i] = camera_create(root, i + 1);
        if (!CHECK(camera != nullptr, "create camera")) return;
        auto* cameraContainer = static_cast<Container*>(camera->contCam);
        if (!CHECK(cameraContainer != nullptr, "camera container")) return;
        CHECK_EQ(Container_GetParent(cameraContainer),
                 i == 2 ? root : static_cast<Container*>(camera->contTilt));
        Viewport* view = views[i] = viewport_create_pixel(0, 0, 80, 60, cameraContainer);
        if (!CHECK(view != nullptr, "create viewport")) return;
        CHECK_EQ(viewport_get_camera(view), cameraContainer);
    }
    if (rootFirst) {
        // Match the scene part of engine shutdown: the primary viewport
        // goes first, then the root and cameras, then both pools.
        viewport_remove(views[0]);
        Container_Remove(root);
    } else {
        viewport_shutdown();
    }
    for (auto* camera : cameras) camera_free(camera);
    Container_Shutdown();
    if (rootFirst) viewport_shutdown();
}

static DWORD child_count(Container* container)
{
    IDirect3DRMFrameArray* children = nullptr;
    if (!CHECK(Container_GetMasterFrame(container)->GetChildren(&children) == DD_OK && children,
               "get child frames")) return 0;
    const DWORD count = children->GetSize();
    children->Release();
    return count;
}

static void check_rejection_cleanup()
{
    using namespace synthetic_fixture;
    block_text_token_pool_init();
    lws_initialise(nullptr, nullptr, nullptr, nullptr);
    Container* root = Container_Initialise("Fixture");
    if (!CHECK(root != nullptr, "create root")) return;
    const DWORD initialChildren = child_count(root);
    for (const char* type : {"LWO", "LWS", "ACT", "MESH"}) {
        CHECK_EQ(Container_Load(root, "Missing", type, 1), nullptr);
        CHECK_EQ(child_count(root), initialChildren);
    }
    if (!CHECK(write_triangle("Data/triangle.lwo") &&
               write_null_scene("Data/motion.lws"), "write generic scene resources")) return;
    auto* mesh = Container_Load(root, "triangle", "LWO", 1);
    auto* animation = Container_Load(root, "motion", "LWS", 1);
    if (!CHECK(mesh && animation, "load valid resources after rejections")) return;
    CHECK_EQ(child_count(root), initialChildren + 2);
    CHECK_EQ(Container_GetParent(mesh), root);
    CHECK_EQ(Container_GetParent(animation), root);
    // The pool owns live resources and must be able to destroy the root
    // before their retained child frames, just as on startup rejection.
    Container_Shutdown();
    lws_shutdown();

    // Starting a fresh pool in the same process must still succeed.
    root = Container_Initialise("Fixture");
    if (!CHECK(root != nullptr, "reinitialise scene pool")) return;
    CHECK_EQ(child_count(root), initialChildren);
    Container_Shutdown();
}

static void check_initializer_rejection()
{
    using namespace synthetic_fixture;
    // The initializer reads these paths before it reaches the deliberately
    // missing lighting config. Every bitmap is just a solid grayscale image.
    if (!CHECK(fixture_mkdir("Data/Interface") && fixture_mkdir("Data/Interface/Fonts") &&
               fixture_mkdir("Data/Interface/MessagePanel"), "create placeholder directories")) return;
    for (const char* name : {"FONT5_HI.bmp", "font5_HI.bmp", "ToolTipFont.bmp",
                             "DeskTopFont.bmp", "MBriefFONT.bmp", "MBriefFONT2.bmp", "RSFont.bmp"}) {
        const std::string path = std::string("Data/Interface/Fonts/") + name;
        if (!CHECK(write_bmp(path.c_str(), 80, 190), "write solid font placeholder")) return;
    }
    if (!CHECK(write_bmp("Data/Interface/MessagePanel/MsgPanel.bmp", 16, 16) &&
               write_bmp("Data/screen.bmp", 16, 16) &&
               write_text("Data/Lego.cfg",
                   "Fixture { Main { BuildingUpgradeCostOre 17 LoadScreen screen.bmp "
                   "ShutdownScreen screen.bmp ProgressBar screen.bmp ProgressWindow R:0,0,16,4 } }"),
               "write incomplete startup config")) return;
    block_text_token_pool_init();
    lego_game_config_callback_registry_init("Fixture");
    CHECK_EQ(lego_game_config_init(), 0);
    CHECK_EQ(legoGlobs.BuildingUpgradeCostOre, 17);
    CHECK(legoGlobs.currLevel == nullptr, "rejected startup installs no level");
    CHECK(legoGlobs.cameraFP != nullptr, "initializer reached camera setup");
    // Failure shuts down the container pool but leaves viewports allocated.
    viewport_shutdown();
    Container* root = Container_Initialise("Fixture");
    if (!CHECK(root != nullptr, "scene pool can be reinitialised after rejection")) return;
    Container_Shutdown();
}

static void check_engine_shutdown()
{
    using namespace synthetic_fixture;
    if (!CHECK(make_fixture("scene_teardown_engine_shutdown"), "prepare existing artificial fixture") ||
        !initialise_headless("EngineShutdown") || !boot_game()) return;
    // The partial boot helper does not initialise these shutdown owners.
    rewardGlobs.config = static_cast<BlockFile*>(legoGlobs.config);
    rewardGlobs.gameName = legoGlobs.gameName;
    if (!CHECK(reward_initialise(), "initialise reward records")) return;
    legoGlobs.fontStandard = font_atlas_record_create("L\\font.bmp");
    legoGlobs.fontToolTip = font_atlas_record_create("L\\font.bmp");
    if (!CHECK(legoGlobs.fontStandard && legoGlobs.fontToolTip, "create owned fonts")) return;
    interface_initialise(0, 0, static_cast<TextureSurface*>(legoGlobs.fontToolTip));
    const unsigned types[] = {0, 1, 2};
    const float areas[12] = {};
    const int tips[3] = {};
    panel_texture_slots_allocate(PANEL_TOP, 3, types, areas, nullptr, nullptr, nullptr, tips);
    if (!CHECK(lego_level_load("Level01"), "load existing artificial level")) return;
    if (!CHECK(legoGlobs.currLevel != nullptr, "level installed")) return;
    lego_engine_tick(1.0f);
    lego_engine_shutdown();
    CHECK(legoGlobs.currLevel == nullptr, "engine shutdown unloads current level");
    directdraw_shutdown();
}

int main(int argc, char** argv)
{
    const std::string scenario = argc == 2 ? argv[1] : "parent-first";
    if (!CHECK(scenario == "parent-first" || scenario == "camera-first" ||
               scenario == "camera-root-first" || scenario == "rejection" ||
               scenario == "initializer-rejection" || scenario == "engine-shutdown", "known scenario")) return lrr_test_result();
    if (scenario == "engine-shutdown") {
        check_engine_shutdown();
        return lrr_test_result();
    }
    const std::string directory = "scene_teardown_" + scenario;
    if (!CHECK(fixture_enter(directory.c_str()) && fixture_mkdir("Data"), "isolated scratch directory") ||
        !synthetic_fixture::initialise_headless("SceneTeardown")) return lrr_test_result();
    if (scenario == "parent-first") check_parent_first();
    else if (scenario == "rejection") check_rejection_cleanup();
    else if (scenario == "initializer-rejection") check_initializer_rejection();
    else check_cameras(scenario == "camera-root-first");
    directdraw_shutdown();
    return lrr_test_result();
}
