// SPDX-License-Identifier: LGPL-3.0-or-later
#include <cstdlib>
#include <string>

#include "gods98/config.h"
#include "gods98/directdraw.h"
#include "gods98/font_api.h"
#include "game/interface_unit.h"
#include "game/lego_unit.h"
#include "game/lego_level_record.h"
#include "game/legoobject_globs.h"
#include "game/legoobject_record.h"
#include "game/legoobject_unit.h"
#include "game/panel_unit.h"
#include "game/reward.h"
#include "game/terrain_map.h"
#include "platform/native/test_fixture.h"
#include "test/lrr_test.h"
#include "test/synthetic_level_fixture.h"
#include "game/lego_globs.h"

extern "C" int lego_keep_running(void);
extern "C" void save_slot_set_target(int slot);

using namespace synthetic_fixture;

struct Census {
    unsigned total = 0;
    unsigned ore = 0;
    unsigned crystals = 0;
    int crystalX = -1, crystalY = -1;
};

static int count_object(Lego_Object* object, void* data)
{
    auto& count = *static_cast<Census*>(data);
    ++count.total;
    if (object->type == 7) ++count.ore;
    if (object->type == 6) {
        ++count.crystals;
        float x = 0, y = 0;
        legoobject_transform_position_get(object, &x, &y);
        CHECK(terrain_map_world_to_block(legoGlobs.currLevel->map, x, y, &count.crystalX, &count.crystalY),
              "crystal lies inside current grid");
    }
    return 0;
}

static void check_empty()
{
    CHECK(legoGlobs.currLevel == nullptr, "no current level");
    // Without a current level the pool should contain no live objects.
    unsigned count = 0;
    legoobject_run_through_lists([](Lego_Object*, void* data) {
        ++*static_cast<unsigned*>(data);
        return 0;
    }, &count, 0);
    CHECK_EQ(count, 0u);
    for (void* object : legoGlobs.recordObjects) CHECK(object == nullptr, "object registry entry cleared");
}

static bool check_grid(bool second)
{
    auto* level = legoGlobs.currLevel;
    if (!CHECK(level != nullptr, "current grid")) return false;
    CHECK_STREQ(level->name, second ? "GridB" : "GridA");
    CHECK_STREQ(level->fullName, second ? "Grid B" : "Grid A");
    if (!CHECK_EQ(level->width, second ? TransitionW : W) ||
        !CHECK_EQ(level->height, second ? TransitionH : H)) return false;
    CHECK_EQ(level->blockSize, second ? 32.0f : 40.0f);
    CHECK_EQ(level->noDrain, second ? 0 : 1);
    CHECK_NEAR(level->oxygenRate, second ? 0.005f : 0.002f, 0.000001f);
    CHECK_EQ(level->upgradeTimes[1], second ? 50.0f : 150.0f);
    CHECK_EQ(level->upgradeTimes[2], second ? 125.0f : 100.0f);
    CHECK_EQ(level->upgradeTimes[4], second ? 50.0f : 200.0f);
    CHECK_EQ(level->trainTime, second ? 25.0f : 75.0f);
    CHECK_EQ(level->emergeTimeOut, second ? 1500.0f : 50.0f);
    CHECK_EQ(level->buildingTolerance, second ? 4.0f : 2.0f);
    CHECK_EQ(level->buildingMaxVariation, second ? 6.0f : 3.0f);
    CHECK_EQ((s_panel_highlight_flags & 0x2000u) != 0, !second);
    CHECK_EQ((s_panel_highlight_flags & 0x200000u) != 0, !second);
    CHECK_EQ(level->crystals, 0);
    CHECK_EQ(level->ore, 0);
    CHECK_EQ(level->oxygenLevel, 100.0f);
    CHECK_EQ(legoGlobs.gameSpeed, 1.0f);
    // Vertical barrier in A, horizontal barrier in B.
    CHECK_EQ((level->blocks[2 * level->width + 3].flags1 & 8u) != 0, second);
    CHECK_EQ((level->blocks[3 * level->width + 2].flags1 & 8u) != 0, !second);
    CHECK(!(level->blocks[(second ? 4 : 3) * level->width + (second ? 2 : 4)].flags1 & 8u),
          "mutated barrier cell restored on load");
    Census count;
    legoobject_run_through_lists(count_object, &count, 0);
    CHECK_EQ(count.total, second ? 1u : 2u);
    CHECK_EQ(legoGlobs.recordObjectCount, count.total);
    CHECK_EQ(count.ore, second ? 0u : 1u);
    CHECK_EQ(count.crystals, 1u);
    CHECK_EQ(count.crystalX, second ? 6 : 5);
    CHECK_EQ(count.crystalY, second ? 5 : 4);
    return true;
}

static void dirty_grid(bool second)
{
    auto* level = legoGlobs.currLevel;
    for (unsigned frame = 0; frame < 3; ++frame) lego_engine_tick(1.0f);
    CHECK_EQ(legoGlobs.currLevel, level);
    // Mutate a loose barrier cell and live accounting; neither should persist.
    const unsigned x = second ? 2 : 4;
    const unsigned y = second ? 4 : 3;
    CHECK_EQ(lego_terrain_cell_mutate(level, x, y, 0), 1);
    CHECK(level->blocks[y * level->width + x].flags1 & 8u, "barrier opened");
    level->ore = 7;
    level->crystals = 9;
    lego_game_speed_set(2.0f);
    CHECK_EQ(legoGlobs.gameSpeed, 2.0f);
}

static bool unload()
{
    const char* next = lego_level_free();
    CHECK_STREQ(next, "GridB");
    std::free(const_cast<char*>(next));
    check_empty();
    CHECK_EQ(legoGlobs.gameSpeed, 1.0f);
    return lrr_test_failures() == 0;
}

static bool setup(const std::string& directory)
{
    if (!CHECK(make_transition_fixture(directory.c_str()), "write independent grid fixtures") ||
        !initialise_headless("DistinctLevelLifecycle") || !boot_game("transition.cfg")) return false;
    save_slot_set_target(-1); // No saved mission or external save data.
    rewardGlobs.config = static_cast<BlockFile*>(legoGlobs.config);
    rewardGlobs.gameName = legoGlobs.gameName;
    if (!CHECK(reward_initialise(), "initialise reward records")) return false;
    legoGlobs.fontStandard = font_atlas_record_create("L\\font.bmp");
    legoGlobs.fontToolTip = font_atlas_record_create("L\\font.bmp");
    if (!CHECK(legoGlobs.fontStandard && legoGlobs.fontToolTip, "create owned fonts")) return false;
    interface_initialise(0, 0, static_cast<TextureSurface*>(legoGlobs.fontToolTip));
    const unsigned types[] = {0, 1, 2};
    const float areas[12] = {};
    const int tips[3] = {};
    panel_texture_slots_allocate(PANEL_TOP, 3, types, areas, nullptr, nullptr, nullptr, tips);
    return true;
}

int main(int argc, char** argv)
{
    const std::string scenario = argc == 2 ? argv[1] : "transition";
    if (!CHECK(scenario == "transition" || scenario == "missing-ptl" ||
               scenario == "missing-surface", "known scenario")) return lrr_test_result();
    if (!setup("distinct_level_" + scenario)) return lrr_test_result();
    check_empty();
    if (!CHECK(lego_level_load("GridA"), "load grid A") || !check_grid(false)) return lrr_test_result();
    dirty_grid(false);
    if (scenario == "transition") {
        if (!CHECK(lego_keep_running(), "follow GridA nextlevel to GridB") || !check_grid(true))
            return lrr_test_result();
        const short initialRandomField = legoGlobs.currLevel->blocks[0].f_20;
        dirty_grid(true);
        if (!CHECK(lego_keep_running(), "restart GridB through nextlevel") || !check_grid(true))
            return lrr_test_result();
        CHECK_EQ(legoGlobs.currLevel->blocks[0].f_20, initialRandomField);
    } else {
        const bool ptl = scenario == "missing-ptl";
        const char* file = ptl ? "Data/L2/empty.ptl" : "Data/L2/surf.map";
        fixture_remove_all(file);
        CHECK(!fixture_exists(file), "required input removed");
        if (ptl) {
            CHECK_EQ(lego_keep_running(), 0); // Failed real transition to B.
        } else {
            if (!unload()) return lrr_test_result();
            CHECK_EQ(lego_level_load("GridB"), 0);
        }
        check_empty();
        if (!CHECK(ptl ? write_text(file, "Lego* {\n}\n") : write_transition_surface(),
                   "restore required input")) return lrr_test_result();
        if (!CHECK(lego_level_load("GridB"), "recover by loading grid B") || !check_grid(true))
            return lrr_test_result();
    }
    for (unsigned frame = 0; frame < 3; ++frame) lego_engine_tick(1.0f);
    CHECK(objectGlobs.listCount > 0, "object pool allocated before shutdown");
    lego_engine_shutdown();
    for (void* chunk : objectGlobs.listSet) CHECK_EQ(chunk, nullptr);
    CHECK_EQ(objectGlobs.freeList, nullptr);
    // The census walks the pool metadata, so don't dereference freed chunks
    // if a regression leaves the count nonzero after shutdown.
    if (CHECK_EQ(objectGlobs.listCount, 0u)) check_empty();
    directdraw_shutdown();
    return lrr_test_result();
}
