// SPDX-License-Identifier: LGPL-3.0-or-later
/* A loaded synthetic level: opening a corridor connects previously
 * disconnected caves, and dig-complete events discover generated resources. */
#include <cstdlib>
#include <set>
#include "game/aitask_globs.h"
#include "game/aitask_record.h"
#include "game/lego_unit.h"
#include "game/legoobject_unit.h"
#include "game/terrain_map.h"
#include "test/lrr_test.h"
#include "test/synthetic_level_fixture.h"
#include "game/lego_globs.h"
#include "crt/vc5names.h"

namespace {
using namespace synthetic_fixture;

Lego_Block& Cell(Lego_Level* level, unsigned x, unsigned y)
{
    return level->blocks[y * level->width + x];
}

unsigned ResourceCode(unsigned x, unsigned y)
{
    // Three crystals in the right wall and one ore in a second wall.
    if (x == 4 && y == 3) return 5;
    if (x == 4 && y == 5) return 2;
    return 0;
}

void CheckRoute(Lego_Level* level, bool reachable)
{
    unsigned *xs = nullptr, *ys = nullptr, count = 0;
    CHECK_EQ(legoobject_grid_route_search_adapter(nullptr, 2, 3, 5, 3,
        &xs, &ys, &count), reachable);
    if (reachable && CHECK(xs && ys && count >= 2, "route exists")) {
        CHECK_EQ(count, 4u);
        CHECK_EQ(xs[0], 2u);
        CHECK_EQ(ys[0], 3u);
        CHECK_EQ(xs[count - 1], 5u);
        CHECK_EQ(ys[count - 1], 3u);
        for (unsigned i = 0; i < count; i++) {
            if (CHECK(xs[i] < level->width && ys[i] < level->height, "route in bounds"))
                CHECK(Cell(level, xs[i], ys[i]).flags1 & 8, "route only crosses floor");
            if (i)
                CHECK_EQ(std::abs(int(xs[i]) - int(xs[i - 1])) +
                         std::abs(int(ys[i]) - int(ys[i - 1])), 1);
        }
    } else if (!reachable) {
        CHECK_EQ(xs, nullptr);
        CHECK_EQ(ys, nullptr);
        CHECK_EQ(count, 0u);
    }
    free(xs);
    free(ys);
}

struct Census {
    unsigned ore = 0, crystals = 0;
    unsigned atOreWall = 0, atCrystalWall = 0;
    Lego_Object* discoveredOre = nullptr;
};

int CountResources(Lego_Object* obj, void* data)
{
    auto& census = *static_cast<Census*>(data);
    if (obj->type != 6 && obj->type != 7) return 0;
    if (obj->type == 6) census.crystals++;
    else census.ore++;
    int x = -1, y = -1;
    CHECK(legoobject_position_to_terrain_cell(obj, &x, &y), "resource on map");
    if (x == 4 && y == 5 && obj->type == 7) {
        census.atOreWall++;
        census.discoveredOre = obj;
    }
    if (x == 4 && y == 3 && obj->type == 6) census.atCrystalWall++;
    return 0;
}

Census Resources()
{
    Census result;
    legoobject_run_through_lists_skip_upgrade_parts(CountResources, &result);
    return result;
}

void Scenario()
{
    if (!boot_game()) return;
    crt_srand(12345);
    if (!CHECK(lego_level_load("Level01"), "load generated caves")) return;
    auto* level = legoGlobs.currLevel;
    if (!CHECK(level != nullptr, "current level")) return;
    // The shared boot config has no CryOreMap; exercise its dedicated loader.
    if (!CHECK(lego_load_path_ai(level, "L\\resources.map", 0), "load generated resource layer")) return;
    CHECK_EQ(Cell(level, 4, 5).f_05, 2);
    CHECK_EQ(Cell(level, 4, 3).f_05, 5);
    CHECK_EQ(Cell(level, 4, 3).flags1 & 8u, 0u);
    CHECK_EQ(Cell(level, 4, 5).flags1 & 8u, 0u);
    const int initialOre = level->ore;
    const int initialCrystals = level->f_08c;
    CheckRoute(level, false);
    const Census before = Resources();
    CHECK_EQ(before.ore, 1u);
    CHECK_EQ(before.crystals, 1u);
    CHECK_EQ(lego_terrain_cell_mutate(level, 4, 3, 0), 1);
    CheckRoute(level, true);
    for (unsigned i = 0; i < 8; ++i) lego_engine_tick(1.0f);
    const Census first = Resources();
    CHECK_EQ(first.ore, before.ore);
    CHECK_EQ(first.crystals, before.crystals + 3);
    CHECK_EQ(first.atCrystalWall, 3u);
    CHECK_EQ(Cell(level, 4, 3).f_2c, 3);
    CHECK_EQ(Cell(level, 4, 5).f_30, 0);
    CHECK_EQ(lego_terrain_cell_mutate(level, 4, 5, 0), 1);
    for (unsigned i = 0; i < 8; ++i) lego_engine_tick(1.0f);
    const Census after = Resources();
    CHECK_EQ(after.ore, before.ore + 1);
    CHECK_EQ(after.crystals, before.crystals + 3);
    CHECK_EQ(after.atOreWall, 1u);
    CHECK_EQ(after.atCrystalWall, 3u);
    CHECK_EQ(Cell(level, 4, 5).f_30, 1);
    CHECK_EQ(Cell(level, 4, 3).f_2c, 3);
    // Discovery creates loose resources, not deposits in the stockpile.
    CHECK_EQ(level->ore, initialOre);
    CHECK_EQ(level->f_08c, initialCrystals);
    CHECK_EQ(lego_terrain_cell_mutate(level, 4, 3, 0), 0);
    CHECK_EQ(lego_terrain_cell_mutate(level, 4, 5, 0), 0);
    for (unsigned i = 0; i < 8; ++i) lego_engine_tick(1.0f);
    const Census later = Resources();
    CHECK_EQ(later.ore, after.ore);
    CHECK_EQ(later.crystals, after.crystals);
    CheckRoute(level, true);

    // Translated collect events create real pending tasks. Each new
    // resource must have exactly one task, at the appropriate priority.
    std::set<Lego_Object*> crystalTargets;
    unsigned oreTasks = 0;
    for (AITask* task = aiTaskGlobs.globalHead; task; task = task->next) {
        if (task->type != 3 || !task->object) continue;
        int x = -1, y = -1;
        CHECK(legoobject_position_to_terrain_cell(task->object, &x, &y), "task target on map");
        if (x == 4 && y == 3 && task->object->type == 6) {
            CHECK(crystalTargets.insert(task->object).second, "one task per crystal");
            CHECK_EQ(task->priority, 0);
        }
        if (task->object == later.discoveredOre) {
            oreTasks++;
            CHECK_EQ(task->priority, 1);
        }
    }
    CHECK_EQ(crystalTargets.size(), 3u);
    CHECK_EQ(oreTasks, 1u);
    if (CHECK(later.discoveredOre != nullptr, "discovered ore exists")) {
        auto* removed = later.discoveredOre;
        legoobject_remove(removed);
        for (unsigned i = 0; i < 8; ++i) lego_engine_tick(1.0f);
        CHECK_EQ(Resources().ore, before.ore);
        CHECK_EQ(Resources().crystals, after.crystals);
        unsigned remainingCrystalTasks = 0;
        for (AITask* task = aiTaskGlobs.globalHead; task; task = task->next) {
            CHECK(task->object != removed, "removed resource has no pending task");
            if (task->type == 3 && crystalTargets.count(task->object))
                remainingCrystalTasks++;
        }
        CHECK_EQ(remainingCrystalTasks, 3u);
    }
}
} // namespace

int main()
{
    if (CHECK(make_fixture("level_navigation_scenario_fixture"), "write synthetic fixture") &&
        CHECK(write_map("Data/L/resources.map", W, H, ResourceCode), "write resource layer") &&
        CHECK(write_text("Data/L/empty.ptl",
            "Lego* { DigComplete GenerateFromCryOre GenerateCrystalComplete CollectCrystal GenerateOreComplete CollectOre }\n"),
            "write test event translations") &&
        initialise_headless("LevelNavigationScenario"))
        Scenario();
    return lrr_test_result("level_navigation_scenario");
}
