// SPDX-License-Identifier: LGPL-3.0-or-later
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>

#include "platform/native/test_fixture.h"
#include "gods98/config.h"
#include "gods98/container_link_names.h"
#include "gods98/container_unit.h"
#include "gods98/lws.h"
#include "gods98/files.h"
#include "gods98/font_api.h"
#include "gods98/images_link_names.h"
#include "gods98/images_unit.h"
#include "gods98/directdraw.h"
#include "game/animation_object_record.h"
#include "game/building_model_record.h"
#include "game/lego_unit.h"
#include "game/lego_level_record.h"
#include "game/legoobject_unit.h"
#include "game/loader.h"
#include "game/upgrade_record.h"
#include "game/vehicle_model_record.h"
#include "test/lrr_test.h"
#include "test/synthetic_level_fixture.h"
#include "game/lego_globs.h"

using namespace synthetic_fixture;

extern "C" int lego_render_quality_config_apply(void);
extern "C" int lego_keep_running(void);

static bool prepare_catalog(const char* root = "Fixture")
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

static constexpr char CATALOG[] =
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

static bool set_config(const char* text)
{
    legoGlobs.config = block_parse(text, std::strlen(text));
    return CHECK(legoGlobs.config != nullptr, "parse synthetic config");
}

static void check_catalog()
{
    if (!CHECK(prepare_catalog(), "generate primitive activity and mesh") ||
        !CHECK(write_text("Data/catalog.cfg", CATALOG), "write catalog config")) return;
    legoGlobs.config = block_open("catalog.cfg");
    if (!CHECK(legoGlobs.config != nullptr, "open catalog config")) return;
    if (!CHECK(lego_upgrade_types_load(), "load upgrade catalog") ||
        !CHECK(lego_entity_vehicle_type_table_load(), "load vehicle catalog") ||
        !CHECK(lego_entity_minifigure_type_table_load(), "load figure catalog") ||
        !CHECK(lego_entity_rockmonster_type_table_load(), "load creature catalog") ||
        !CHECK(lego_entity_building_types_load(), "load building catalog")) return;
    if (!CHECK_EQ(s_catalog_counts[0], 2u)) return;
    for (unsigned i = 1; i < 5; ++i) {
        if (!CHECK_EQ(s_catalog_counts[i], 1u)) return;
    }
    const auto* part = static_cast<Upgrade_PartModel*>(lego_upgrade_name_entry_find("part"));
    if (CHECK(part != nullptr, "case-insensitive upgrade lookup")) {
        CHECK_EQ(part->objID, 0u);
        CHECK(part->cont != nullptr, "part mesh loaded");
    }
    auto* vehicles = static_cast<VehicleModel*>(legoGlobs.vehicleData);
    CHECK_STREQ(legoGlobs.vehicleName[1], "VehicleB");
    CHECK_EQ(vehicles[1].objID, 1u);
    CHECK_EQ(vehicles[0].wheelRadius, 2.5f);
    CHECK(vehicles[0].contWheels[0] != nullptr, "wheel mesh loaded");
    CHECK_EQ(vehicles[0].upgrades.count, 1u);
    if (CHECK(vehicles[0].upgrades.parts != nullptr, "upgrade binding allocated")) {
        CHECK(vehicles[0].upgrades.parts[0].upgradeData == part, "upgrade references loaded catalog part");
        CHECK_EQ(vehicles[0].upgrades.parts[0].level, 1u);
    }
    CHECK(Container_IsCurrentActivity(static_cast<Container*>(vehicles[0].contAct1), "Activity_Stand"),
          "initial activity selected");
    auto* figure = static_cast<AnimationObject*>(legoGlobs.miniFigureData);
    CHECK_EQ(figure->cameraNullFrames, 2u);
    CHECK_STREQ(figure->cameraNullName, "Anchor");
    auto* creature = static_cast<AnimationObject*>(legoGlobs.rockMonsterData);
    CHECK(creature->container != nullptr, "creature activity loaded");
    auto* building = static_cast<BuildingModel*>(legoGlobs.buildingData);
    if (!CHECK_EQ(building->shapeCount, 3u)) return;
    CHECK_EQ(building->weapons.pivotMaxZ, 0.75f);
    CHECK_EQ(building->weapons.pivotMinZ, -0.5f);
    if (CHECK(building->shapePoints != nullptr, "building shape allocated")) {
        CHECK_EQ(building->shapePoints[1].x, 1);
        CHECK_EQ(building->shapePoints[2].y, 2);
    }
    CHECK(building->contPowerLevel != nullptr, "power-level scene loaded");
    lego_object_names_load(static_cast<BlockRec*>(legoGlobs.config));
    lego_object_the_names_load(static_cast<BlockRec*>(legoGlobs.config));
    CHECK_STREQ(legoGlobs.objectNamesKind1[0], "Sample vehicle");
    CHECK_STREQ(legoGlobs.objectTheNamesKind4[0], "The structure");
    CHECK_STREQ(legoGlobs.objectNamesSpecial[1], "Sample ore");
    CHECK_STREQ(legoGlobs.objectNamesKind2[0], "Sample figure");
    CHECK_STREQ(legoGlobs.objectNamesKind3[0], "Sample creature");
    CHECK_STREQ(legoGlobs.objectTheNamesKind1[1], "The second");
    block_close(static_cast<BlockRec*>(legoGlobs.config));
}

static void check_missing()
{
    if (!set_config("Fixture { VehicleTypes { Missing Missing } MiniFigureTypes { Missing Missing } "
                    "RockMonsterTypes { Missing Missing } BuildingTypes { Missing Missing } }")) return;
    CHECK_EQ(lego_upgrade_types_load(), 0);
    CHECK_EQ(lego_entity_vehicle_type_table_load(), 0);
    CHECK_EQ(lego_entity_minifigure_type_table_load(), 0);
    CHECK_EQ(lego_entity_rockmonster_type_table_load(), 0);
    CHECK_EQ(lego_entity_building_types_load(), 0);
    block_close(static_cast<BlockRec*>(legoGlobs.config));
}

// Reuse the same primitive or null scene for all slots of a given format.
// The test inputs do not represent the named objects or their appearance.
static std::string misc_resource_block()
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

static void check_misc_resources()
{
    if (!CHECK(prepare_catalog(), "generate generic resource files")) return;
    const std::string config = "Fixture {" + misc_resource_block() + "}";
    if (!set_config(config.c_str())) return;
    lego_misc_object_resources_load(static_cast<BlockRec*>(legoGlobs.config));
    for (void* record : {legoGlobs.contBoulder, legoGlobs.contCrystal, legoGlobs.contOresTable,
                         legoGlobs.contProcessedOre, legoGlobs.contElectricFence, legoGlobs.contDynamite,
                         legoGlobs.contSpiderWeb, legoGlobs.contSmashPath, legoGlobs.contBoulderExplode,
                         legoGlobs.contMiniTeleportUp, legoGlobs.contLaserShot, legoGlobs.contBoulderExplodeIce,
                         legoGlobs.contOohScary, legoGlobs.contBarrier, legoGlobs.contRechargeSparkle,
                         legoGlobs.contElectricFenceStud, legoGlobs.contPusher, legoGlobs.contFreezer,
                         legoGlobs.contIceCube}) {
        if (CHECK(record != nullptr, "resource slot loaded")) {
            CHECK(Container_IsHidden(static_cast<Container*>(record)), "resource template hidden");
        }
    }
    block_close(static_cast<BlockRec*>(legoGlobs.config));
}

static void check_config_and_loader()
{
    if (!set_config("Fixture { Main { fpclipblocks 7 quality Flat dither TRUE filter TRUE } "
                    "ToolTipInfo { HealthText Example EnergyText Other } "
                    "UpgradeNames { Level3 Sample Level16 Ignored Other Ignored } }")) return;
    CHECK(lego_render_quality_config_apply(), "apply valid graphics settings");
    CHECK_EQ(legoGlobs.quality, 2);
    CHECK_EQ(legoGlobs.FPClipBlocks, 7.0f);
    lego_tooltip_info_strings_load(static_cast<BlockRec*>(legoGlobs.config), "Fixture");
    CHECK_STREQ(legoGlobs.HealthText, "Example");
    CHECK_STREQ(legoGlobs.EnergyText, "Other");
    CHECK(legoGlobs.UpgradeNames[0] == nullptr, "no unspecified upgrade name");
    lego_level_upgrade_name_strings_load(static_cast<BlockRec*>(legoGlobs.config));
    CHECK_STREQ(legoGlobs.UpgradeNames[3], "Sample");
    block_close(static_cast<BlockRec*>(legoGlobs.config));

    if (!set_config("Fixture { Main { fpclipblocks 5 quality Wireframe } }")) return;
    CHECK(lego_render_quality_config_apply(), "apply wireframe settings");
    CHECK_EQ(legoGlobs.quality, 0);
    block_close(static_cast<BlockRec*>(legoGlobs.config));
    if (!set_config("Fixture { Main { fpclipblocks 5 quality Invalid } }")) return;
    CHECK_EQ(lego_render_quality_config_apply(), 0);
    block_close(static_cast<BlockRec*>(legoGlobs.config));

    if (!CHECK(write_bmp("Data/screen.bmp", 16, 16) && write_bmp("Data/font.bmp", 80, 190) &&
               write_text("Data/profile.txt", "Setup=8\n"), "generate loader inputs")) return;
    auto* font = static_cast<TextureSurface*>(font_atlas_record_create("font.bmp"));
    if (!CHECK(font != nullptr, "load grayscale font placeholder")) return;
    Area2F bounds = {0, 0, 16, 4};
    loader_initialise("screen.bmp", "screen.bmp", font, "profile.txt", 1, "screen.bmp", &bounds, "Sample_text");
    loader_display_loading_bar("Setup");
    loader_file_load_callback("sample", 12, nullptr);
    loader_display_loading_bar(nullptr);
    loader_display_shutdown();
    loader_shutdown("output.txt");
    file_set_load_callback(nullptr, nullptr);
    font_atlas_record_release(font);
    std::ifstream profile("Data/output.txt");
    CHECK(profile.good(), "loader profile written");
    const std::string result(std::istreambuf_iterator<char>{profile}, {});
    CHECK_STREQ(result.c_str(), "Setup=12\n");
}

// This fixture is ephemeral and deliberately uses the same single geometric
// primitive and grayscale images for each required input slot. It does not
// represent or substitute for any shipped game asset or level.
static bool check_initializer()
{
    if (!CHECK(make_transition_fixture("startup_initializer_fixture"), "generate abstract grid fixture") ||
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
        !initialise_headless("SyntheticInitializer")) return false;
    block_text_token_pool_init();
    Image_Initialise();
    lego_game_config_callback_registry_init("Lego");
    if (!CHECK(lego_game_config_init(), "real initializer accepts synthetic inputs")) return false;
    auto* grid = legoGlobs.currLevel;
    if (!CHECK(grid != nullptr, "generated grid is installed")) return false;
    CHECK_STREQ(grid->name, "GridA");
    CHECK_EQ(grid->width, W);
    CHECK_EQ(grid->height, H);
    CHECK_EQ(grid->blockSize, 40.0f);
    CHECK_EQ(legoGlobs.gameSpeed, 1.0f);
    CHECK_EQ(s_catalog_counts[0], 2u);
    for (unsigned i = 1; i < 5; ++i) CHECK_EQ(s_catalog_counts[i], 1u);
    unsigned counts[2] = {};
    legoobject_run_through_lists([](Lego_Object* object, void* data) {
        auto* counts = static_cast<unsigned*>(data);
        if (object->type == 6) ++counts[0];
        if (object->type == 7) ++counts[1];
        return 0;
    }, counts, 0);
    CHECK_EQ(counts[0], 1u);
    CHECK_EQ(counts[1], 1u);
    for (unsigned i = 0; i < 3; ++i) CHECK(lego_engine_tick(1.0f), "one engine tick");
    CHECK_EQ(legoGlobs.currLevel, grid);
    if (CHECK(lego_keep_running(), "real transition to the second generated grid") &&
        CHECK(legoGlobs.currLevel != nullptr, "second grid installed")) {
        CHECK_STREQ(legoGlobs.currLevel->name, "GridB");
        CHECK_EQ(legoGlobs.currLevel->width, TransitionW);
        CHECK_EQ(legoGlobs.currLevel->height, TransitionH);
        CHECK_EQ(legoGlobs.currLevel->ore, 0);
        CHECK_EQ(legoGlobs.currLevel->crystals, 0);
        CHECK_EQ(legoGlobs.recordObjectCount, 1u);
        CHECK(lego_engine_tick(1.0f), "second grid ticks");
    }
    lego_engine_shutdown();
    CHECK(legoGlobs.currLevel == nullptr, "shutdown unloads grid");
    // WinMain releases the DirectDraw device after the game shutdown.
    directdraw_shutdown();
    return true;
}

int main(int argc, char** argv)
{
    const std::string scenario = argc == 2 ? argv[1] : "catalog";
    if (scenario == "initializer") { check_initializer(); return lrr_test_result(); }
    if (!CHECK(scenario == "catalog" || scenario == "missing" || scenario == "misc" ||
               scenario == "setup", "known scenario")) return lrr_test_result();
    const std::string directory = "startup_positive_" + scenario;
    if (!CHECK(fixture_enter(directory.c_str()) &&
               fixture_mkdir("Data"), "isolated scratch directory") ||
        !initialise_headless("StartupCatalog")) return lrr_test_result();
    block_text_token_pool_init();
    legoGlobs.gameName = "Fixture";
    legoGlobs.rootCont = Container_Initialise("Fixture");
    if (!CHECK(legoGlobs.rootCont != nullptr, "initialise scene root")) return lrr_test_result();
    lws_initialise(nullptr, nullptr, nullptr, nullptr);
    if (scenario == "missing") check_missing();
    else if (scenario == "misc") check_misc_resources();
    else if (scenario == "setup") check_config_and_loader();
    else check_catalog();
    return lrr_test_result();
}
