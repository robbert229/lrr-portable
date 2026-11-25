// SPDX-License-Identifier: LGPL-3.0-or-later
/* Shared full initializer fixture, using only generated neutral inputs. */
#include <fstream>
#include <iterator>
#include <string>
#include "platform/native/test_fixture.h"
#include "gods98/config.h"
#include "gods98/images_link_names.h"
#include "gods98/images_unit.h"
#include "game/lego_unit.h"
#include "test/lrr_test.h"
#include "test/synthetic_level_fixture.h"
#include "test/synthetic_initializer_fixture.h"

namespace synthetic_fixture {
bool prepare_catalog(const char* root)
{
    // An activity with one moving null; the only mesh is the existing triangle.
    return fixture_mkdir("Data/Primitive") &&
           write_null_scene("Data/Primitive/motion.lws") &&
           write_triangle("Data/Primitive/triangle.lwo") &&
           write_text("Data/Primitive/Primitive.ae",
               (std::string(root) + " { Activities { Activity_Stand Motion } "
               "Motion { FILE motion LWSFILE TRUE LOOPING TRUE } "
               "CameraNullName Anchor CameraNullFrames 2 CameraFlipDir TRUE "
               "CarryNullName Anchor CarryNullFrames 1 ToolNullName Anchor ToolNullFrames 1 "
               "WheelMesh triangle WheelNullName Anchor WheelRadius 2.5 "
               "PivotMaxZ 0.75 PivotMinZ -0.5 Shape 1,0:0,2 "
               "PowerLevelScene motion Upgrades { Level0001 { Part Anchor,1 } } }").c_str());
}

const char CATALOG[] =
    "Fixture { "
    "UpgradeTypes { Part Primitive\\triangle } "
    "VehicleTypes { VehicleA Primitive VehicleB Primitive } "
    "MiniFigureTypes { Figure Primitive } "
    "RockMonsterTypes { Creature Primitive } "
    "BuildingTypes { Structure Primitive } "
    "ObjectNames { VehicleA Sample_vehicle VehicleB Second_vehicle Figure Sample_figure "
    "Creature Sample_creature Structure Sample_structure Ore Sample_ore } "
    "ObjectTheNames { VehicleA The_vehicle VehicleB The_second Figure The_figure "
    "Creature The_creature Structure The_structure Ore The_ore } }";

// Reuse the same primitive or null scene for all slots of a given format.
// The test inputs do not represent the named objects or their appearance.
std::string misc_resource_block()
{
    std::string config = " MiscObjects { ";
    for (const char* key : {"Boulder", "Crystal", "Ore", "ProcessedOre", "ElectricFence", "LazerHit"})
        config += std::string(key) + " Primitive\\triangle ";
    for (const char* key : {"BoulderExplodeIce", "BoulderExplode", "SmashPath", "Explosion",
                            "ElectricFenceStud", "ShortElectricFenceBeam", "LongElectricFenceBeam",
                            "RechargeSparkle", "MiniTeleportUp", "Pusher", "Freezer", "LaserShot", "PathDust"})
        config += std::string(key) + " Primitive\\motion ";
    for (const char* key : {"Dynamite", "OohScary", "Barrier", "SpiderWeb", "IceCube"})
        config += std::string(key) + " Primitive ";
    config += "} ";
    return config;
}

// This fixture is ephemeral and deliberately uses the same single geometric
// primitive and grayscale images for each required input slot. It does not
// represent or substitute for any shipped game asset or level.
bool initialise_full_game(const char* directory, const char* programName, const char* extraStats)
{
    if (!CHECK(make_transition_fixture(directory), "generate abstract grid fixture") ||
        !CHECK(prepare_catalog("Lego"), "generate primitive model fixture")) return false;
    for (const char* dir : {"Data/Interface", "Data/Interface/Fonts", "Data/Interface/MessagePanel",
                            "Data/Interface/MessageTabs", "Data/Interface/InfoPanel", "Data/Interface/Buttons"}) {
        if (!CHECK(fixture_mkdir(dir), "create scratch image directory")) return false;
    }
    for (const char* name : {"FONT5_HI.bmp", "font5_HI.bmp", "ToolTipFont.bmp", "DeskTopFont.bmp",
                             "MBriefFONT.bmp", "MBriefFONT2.bmp", "RSFont.bmp"}) {
        const std::string path = std::string("Data/Interface/Fonts/") + name;
        if (!CHECK(write_bmp(path.c_str(), 80, 190), "generate grayscale font input")) return false;
    }
    for (const char* path : {"Data/Interface/MessagePanel/MsgPanel.bmp",
                             "Data/Interface/MessageTabs/infoOverFlow.bmp",
                             "Data/Interface/InfoPanel/scrollInfo.bmp",
                             "Data/Interface/Buttons/scrollInfoSld.bmp"}) {
        if (!CHECK(write_bmp(path, 16, 16), "generate solid test image")) return false;
    }
    std::ifstream input("Data/transition.cfg");
    if (!CHECK(input.good(), "open generated configuration")) return false;
    std::string config(std::istreambuf_iterator<char>{input}, {});
    input.close();
    const auto mainPos = config.find("Main {");
    if (!CHECK(mainPos != std::string::npos, "fixture has Main section")) return false;
    config.insert(mainPos + 6, "\n FrontEnd FALSE DontPlayAvis TRUE startlevel GridA NoSound TRUE "
        "LoadScreen L\\load.bmp ShutdownScreen L\\load.bmp ProgressBar L\\load.bmp ProgressWindow R:0,0,16,4\n");
    if (extraStats != nullptr) {
        const auto statsPos = config.find("Stats {");
        if (!CHECK(statsPos != std::string::npos, "fixture has Stats section")) return false;
        config.insert(statsPos + 7, extraStats);
    }
    std::string catalog = CATALOG;
    config.insert(config.rfind('}'), catalog.substr(catalog.find('{') + 1, catalog.rfind('}') - catalog.find('{') - 1));
    std::string resources = misc_resource_block() + "\n Buttons640x480 { ";
    for (const char* key : {"Toggle", "TaggedObjectView", "ZoomIn", "ZoomOut"})
        resources += std::string("Panel_Radar PanelButton_Radar_") + key + ",L\\panel.bmp,NULL,NULL,1,2,8,9,Unknown ";
    for (const char* key : {"Options", "Priorities", "CallToArms"})
        resources += std::string("Panel_TopPanel PanelButton_TopPanel_") + key + ",L\\panel.bmp,NULL,NULL,1,2,8,9,Unknown ";
    resources += "}\n";
    config.insert(config.rfind('}'), resources);
    if (!CHECK(write_text("Data/Lego.cfg", config.c_str()), "write original fixture configuration") ||
        !initialise_headless(programName)) return false;
    block_text_token_pool_init();
    Image_Initialise();
    lego_game_config_callback_registry_init("Lego");
    if (!CHECK(lego_game_config_init(), "real initializer accepts synthetic inputs")) return false;
    return true;
}
}
