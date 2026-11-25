// SPDX-License-Identifier: LGPL-3.0-or-later
/* Generated terrain geometry and configured mission progression. */
#include <cmath>
#include <string>
#include "gods98/config.h"
#include "game/lego_unit.h"
#include "game/legoobject_unit.h"
#include "game/objective_unit.h"
#include "game/panel_unit.h"
#include "game/terrain_map.h"
#include "game/terrain_cell_objects.h"
#include "test/lrr_test.h"
#include "test/synthetic_level_fixture.h"
#include "game/lego_globs.h"

extern "C" {
void objective_load_original(BlockRec*, const char*, const char*, Lego_Level*, unsigned, unsigned);
void objective_tick_target(void*, Lego_Level*, float, float);
int objective_is_ended_original();
int level_objective_result_get();
}

namespace {
using namespace synthetic_fixture;

void geometry()
{
    auto* map = legoGlobs.currLevel->map;
    constexpr int bx = 2, by = 2;
    auto& block = map->blocks[by * map->blockWidth + bx];
    float x, y;
    terrain_map_block_to_world(map, bx, by, &x, &y);
    const float s = map->blockSize;
    const float z = terrain_map_world_z_get(map, x, y);
    // Raise the eastern edge of a quad; both neighbouring triangles share
    // the same planar slope. The modified mesh, not the height file, is used.
    terrain_map_vertex_move(map, bx + 1, by, 8);
    terrain_map_vertex_move(map, bx + 1, by + 1, 8);
    CHECK(map->flags & TERRAIN_MAP_FLAG_MODIFIED, "normal update pending");
    terrain_map_update(map, 0);
    CHECK_EQ(map->flags & TERRAIN_MAP_FLAG_MODIFIED, 0u);
    for (int rotated : {0, 1, 0}) {
        terrain_map_block_rotated_set(map, bx, by, rotated);
        terrain_map_update(map, 0);
        CHECK_EQ(bool(block.flags & TERRAIN_MAP_BLOCK_ROTATED), bool(rotated));
        for (float dx : {-0.25f, 0.25f}) {
            for (float dy : {-0.25f, 0.25f}) {
                TerrainMapVector3 origin{x + dx*s, y + dy*s, z - 100};
                TerrainMapVector3 direction{0, 0, 1}, hit{};
                CHECK_EQ(terrain_map_block_ray_intersection(map, bx, by, &origin, &direction, &hit), 1);
                CHECK_NEAR(hit.z, z + (dx + 0.5f)*8, 1e-3);
                CHECK_NEAR(terrain_map_world_z_get(map, origin.x, origin.y), hit.z, 1e-3);
                TerrainMapPoint2I picked{};
                CHECK_EQ(terrain_map_ray_intersection(map, &origin, &direction, &hit, &picked, 4), 1);
                CHECK_EQ(picked.x, bx);
                CHECK_EQ(picked.y, by);
                CHECK_NEAR(hit.z, z + (dx + 0.5f)*8, 1e-3);
            }
        }
        TerrainMapVector3 outside{x + s, y, z - 100}, direction{0, 0, 1}, hit{};
        CHECK_EQ(terrain_map_block_ray_intersection(map, bx, by, &outside, &direction, &hit), 0);
        float ex, ey;
        CHECK_EQ(terrain_map_world_edge_point_get(map, x - s/4, y + s/8, 0, 0, &ex, &ey), 1);
        CHECK_NEAR(std::fabs(ex-x), s/2, 1e-3);
        CHECK_NEAR(ey, y+s/8, 1e-3);
        CHECK_EQ(terrain_map_world_edge_point_get(map, x - s/4, y + s/8, 1, 0, &ex, &ey), 1);
        CHECK_NEAR(std::fabs(ex-x), std::fabs(ey-y), 1e-3);
    }
    const TerrainMapPoint2I shape[] = {{bx, by}};
    CHECK_EQ(terrain_map_shape_tolerance_check(map, shape, 1, 0, 4), 0);
    CHECK_EQ(terrain_map_shape_tolerance_check(map, shape, 1, 0, 9), 1);
    terrain_map_shape_flatten(map, shape, 1, 0);
    terrain_map_update(map, 0);
    CHECK_EQ(terrain_map_shape_tolerance_check(map, shape, 1, 0.01f, 0.01f), 1);
    CHECK(block.flags & TERRAIN_MAP_BLOCK_FLATTENED, "flattened footprint");
    CHECK_NEAR(terrain_map_world_z_get(map, x, y), z+4, 1e-3);

    terrain_map_visible_clear(map);
    terrain_map_visible_radius_add(map, 0, 0, 2, nullptr);
    CHECK_EQ(map->visibleCount, 6u);
    CHECK_EQ(terrain_map_block_hidden_get(map, 0, 2), 0);
    terrain_map_visible_clear(map);
    CHECK_EQ(map->visibleCount, 0u);
    CHECK_EQ(terrain_map_block_hidden_get(map, 0, 2), 1);

    const unsigned initialWobbles = map->uvWobbleCount;
    terrain_map_block_uv_wobble_set(map, bx, by, 1);
    terrain_map_block_uv_wobble_set(map, bx, by, 1);
    CHECK_EQ(map->uvWobbleCount, initialWobbles + 1);
    terrain_map_wobble_emissive_set(map, 1);
    CHECK(map->flags & TERRAIN_MAP_FLAG_WOBBLE_EMISSIVE, "animated surface enabled");
    terrain_map_update(map, 2);
    terrain_map_wobble_emissive_set(map, 0);
    terrain_map_block_uv_wobble_set(map, bx, by, 0);
    CHECK_EQ(map->uvWobbleCount, initialWobbles);
    CHECK_EQ(block.flags & TERRAIN_MAP_BLOCK_UVWOBBLE, 0);
    CHECK_EQ(map->flags & TERRAIN_MAP_FLAG_WOBBLE_EMISSIVE, 0u);

    const unsigned old = block.texture;
    const unsigned next = (old+1)%64;
    terrain_map_transitions_update(map, 100);
    terrain_map_block_texture_fade_set(map, bx, by, next, 1);
    CHECK_EQ(block.texture, next);
    auto& transition = map->transitions[0];
    CHECK(transition.flags & 1, "old texture retained during fade");
    const int group = transition.groupID;
    terrain_map_transitions_update(map, 10);
    CHECK(transition.flags & 1, "partial fade remains live");
    CHECK_NEAR(transition.timer, 0.45f, 1e-5);
    terrain_map_transitions_update(map, 20);
    CHECK_EQ(transition.flags & 1, 0u);
    terrain_map_block_texture_fade_set(map, bx, by, old, 2);
    CHECK_EQ(transition.groupID, group);
    CHECK(transition.flags & 1, "transition mesh group reused");
    terrain_map_transitions_update(map, 100);
    CHECK_EQ(transition.flags & 1, 0u);
    terrain_map_texture_no_fade_set(map, old);
    terrain_map_block_texture_fade_set(map, bx, by, next, 0);
    CHECK_EQ(block.texture, next);
    CHECK_EQ(transition.flags & 1, 0u);
}

void fence_lifecycle()
{
    ElectricFenceObject first = {}, second = {};
    // Isolated placement requires a neighbouring node. The fixture has
    // no buildings, so seed the first node at the existing creation seam.
    CHECK_EQ(electric_fence_can_place_at_block(1, 2), 0);
    auto* node = electric_fence_create(&first, 1, 2);
    if (!CHECK(node != nullptr, "seed node")) return;
    CHECK_EQ(electric_fence_can_place_at_block(1, 2), 0);
    CHECK_EQ(electric_fence_can_place_at_block(1, 3), 1);
    CHECK_EQ(electric_fence_can_place_at_block(1, 4), 1);
    CHECK_EQ(electric_fence_can_place_at_block(0, 2), 0);
    auto* other = electric_fence_create(&second, 1, 3);
    if (!CHECK(other != nullptr, "second node")) return;
    CHECK_EQ(electric_fence_can_place_at_block(1, 4), 1);
    CHECK_EQ(electric_fence_can_place_at_block(1, 3), 0);
    node->timer = 5;
    float elapsed = 2;
    electric_fence_node_update_callback(node, &elapsed);
    CHECK_EQ(node->timer, 3.0f);
    elapsed = 3;
    electric_fence_node_update_callback(node, &elapsed);
    CHECK_EQ(node->timer, 0.0f);
    electric_fence_node_update_callback(node, &elapsed);
    CHECK_EQ(node->timer, 0.0f);
    electric_fence_remove_node(other, 1, 3);
    CHECK_EQ(electric_fence_stud_fits(legoGlobs.currLevel, 1, 3), 0);
    electric_fence_remove_node(node, 1, 2);
    CHECK_EQ(electric_fence_can_place_at_block(1, 3), 0);
    CHECK_EQ(electric_fence_block_is_fence(1, 2), 0);
}

void missions(const std::string& scenario)
{
    const bool fail = scenario == "failure";
    auto* level = legoGlobs.currLevel;
    const std::string config = std::string("Lego* { Grid { "
        "DontShowObjectiveAdvisor TRUE DontShowObjectiveAcheiveAdvisor TRUE "
        "DontShowObjectiveFailedAdvisor TRUE TimerObjective 2:") +
        (fail ? "HitTimeFailObjective" : "HitTimeSuccessObjective") +
        " OreObjective 3 ConstructionObjective " + (scenario == "object" ? "Ore" : "ProcessedOre") + " } }";
    auto* parsed = block_parse(config.c_str(), config.size());
    if (!CHECK(parsed != nullptr, "parse synthetic mission")) return;
    objective_load_original(parsed, "Lego", "Grid", level, 640, 480);
    block_close(parsed);
    CHECK_EQ(level->objective.timer, 50.0f);
    CHECK_EQ(level->objective.ore, 3u);
    CHECK_EQ(level->objective.constructionType, 7);
    CHECK_EQ(level->objective.constructionID, scenario == "object" ? 0 : 1);
    Lego_Object* original = nullptr;
    if (scenario == "object") {
        legoobject_run_through_lists_skip_upgrade_parts([](Lego_Object* obj, void* data) {
            if (obj->type != 7) return 0;
            *static_cast<Lego_Object**>(data) = obj;
            return 1;
        }, &original);
        if (!CHECK(original != nullptr, "initial abstract resource")) return;
        legoobject_remove(original);
    }
    // The original objective unit reads the stored-ore balance from
    // this legacy field; stage that balance directly at its input boundary.
    level->crystals = 0;
    objective_tick_target(nullptr, level, 10, 0);
    CHECK_EQ(level->objective.timer, 40.0f);
    CHECK_EQ(objective_is_ended_original(), 0);
    // No matching object remains in the live list.
    level->crystals = 3;
    objective_tick_target(nullptr, level, 15, 0);
    CHECK_EQ(level->objective.timer, 25.0f);
    CHECK_EQ(objective_is_ended_original(), 0);
    if (scenario == "object") {
        // A newly created matching object satisfies the configured condition
        // before the timer expires, but only while the quota is met.
        float x, y;
        terrain_map_block_to_world(level->map, 1, 2, &x, &y);
        auto* matching = legoobject_create_in_world(legoGlobs.contOresTable, 7, 0, 0, x, y, 0);
        if (!CHECK(matching != nullptr, "create generated matching object")) return;
        level->crystals = 2;
        objective_tick_target(nullptr, level, 1, 0);
        CHECK_EQ(objective_is_ended_original(), 0);
        level->crystals = 3;
        objective_tick_target(nullptr, level, 1, 0);
        CHECK_EQ(level->objective.timer, 23.0f);
        CHECK_EQ(objective_is_ended_original(), 1);
        CHECK_EQ(level->status, 1);
        CHECK_EQ(level_objective_result_get(), 1);
        return;
    }
    objective_tick_target(nullptr, level, 24, 0);
    CHECK_EQ(level->objective.timer, 1.0f);
    CHECK_EQ(objective_is_ended_original(), 0);
    objective_tick_target(nullptr, level, 1, 0);
    CHECK_EQ(level->objective.timer, 0.0f);
    CHECK_EQ(objective_is_ended_original(), 1);
    CHECK_EQ(level->status, fail ? 2 : 1);
    CHECK_EQ(level_objective_result_get(), !fail);
}
}

int main(int argc, char** argv)
{
    const std::string scenario = argc == 2 ? argv[1] : "geometry";
    if (!CHECK(scenario == "geometry" || scenario == "fence" || scenario == "success" ||
               scenario == "failure" || scenario == "object", "known scenario")) return lrr_test_result();
    if (!CHECK(make_fixture(("terrain_objectives_" + scenario).c_str()), "generated grid") ||
        !initialise_headless("TerrainObjectives") || !boot_game() ||
        !CHECK(lego_level_load("Level01"), "load generated level")) return lrr_test_result();
    if (scenario == "geometry") geometry();
    else if (scenario == "fence") fence_lifecycle();
    else missions(scenario);
    if (lrr_test::failures) return lrr_test_result();
    const unsigned types[] = {0, 1, 2};
    const float areas[12] = {};
    const int tips[3] = {};
    panel_texture_slots_allocate(PANEL_TOP, 3, types, areas, nullptr, nullptr, nullptr, tips);
    lego_level_free();
    CHECK_EQ(legoGlobs.currLevel, nullptr);
    return lrr_test_result();
}
