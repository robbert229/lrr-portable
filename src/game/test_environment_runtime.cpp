// SPDX-License-Identifier: LGPL-3.0-or-later
/* Projection on a generated terrain mesh and transient mesh ownership. */
#include <array>
#include <cmath>
#include <string>
#include "gods98/directdraw.h"
#include "gods98/config.h"
#include "gods98/images.h"
#include "gods98/main_globs.h"
#include "gods98/container_link_names.h"
#include "gods98/container_unit.h"
#include "gods98/viewports_link_names.h"
#include "gods98/viewports.h"
#include "game/camera_control.h"
#include "game/lego_unit.h"
#include "game/legoobject_unit.h"
#include "game/objective_unit.h"
#include "game/terrain_map.h"
#include "test/lrr_test.h"
#include "test/synthetic_initializer_fixture.h"
#include "game/lego_globs.h"

struct Vector3F { float x, y, z; };
extern "C" {
void objective_load_original(BlockRec*, const char*, const char*, Lego_Level*, unsigned, unsigned);
void objective_tick_target(void*, Lego_Level*, float, float);
int objective_is_ended_original();
int level_objective_result_get();
void camera_set_topdown_position(LegoCamera*, float, float);
void camera_set_tilt(LegoCamera*, float);
void camera_set_zoom(LegoCamera*, float);
void camera_set_zoom_range(LegoCamera*, float, float);
void camera_set_tilt_range(LegoCamera*, float, float);
}

namespace {
TerrainMap* Map() { return legoGlobs.currLevel->map; }
Viewport* View() { return static_cast<Viewport*>(legoGlobs.viewMain); }

TerrainMapBlock& Block(unsigned x, unsigned y)
{
    return Map()->blocks[y * Map()->blockWidth + x];
}

void Camera(float x, float y, float tilt, float zoom)
{
    camera_set_topdown_position(legoGlobs.cameraMain, x, y);
    camera_set_tilt_range(legoGlobs.cameraMain, 0, 1);
    camera_set_zoom_range(legoGlobs.cameraMain, 100, 400);
    camera_set_tilt(legoGlobs.cameraMain, tilt);
    camera_set_zoom(legoGlobs.cameraMain, zoom);
    viewport_set_back_clip(View(), 1000);
    CHECK(lego_engine_tick(1), "camera/render tick");
}

Point2F ProjectAndPick(int bx, int by, float x, float y, float z)
{
    Vector3F world{x, y, z};
    Point2F screen{};
    viewport_world_to_screen(View(), &screen, &world);
    if (!CHECK(std::isfinite(screen.x) && std::isfinite(screen.y),
               "finite projection")) return screen;
    const int px = int(std::round(screen.x)), py = int(std::round(screen.y));
    if (!CHECK(px >= 0 && py >= 0 && px < int(mainGlobs.appWidth) &&
               py < int(mainGlobs.appHeight), "projected point on screen")) return screen;
    unsigned pickedX = 99, pickedY = 99;
    TerrainMapVector3 hit{};
    if (CHECK_EQ(terrain_map_screen_pick(Map(), View(), px, py, &pickedX, &pickedY, &hit), 1)) {
        CHECK_EQ(pickedX, unsigned(bx));
        CHECK_EQ(pickedY, unsigned(by));
        // Pixel quantization introduces a small world-space error.
        CHECK_NEAR(hit.x, x, 2.0f);
        CHECK_NEAR(hit.y, y, 2.0f);
        CHECK_NEAR(hit.z, terrain_map_world_z_get(Map(), hit.x, hit.y), 0.01f);
        CHECK_NEAR(hit.z, z, 1.0f);
        Vector3F picked{hit.x, hit.y, hit.z};
        Point2F reprojected{};
        viewport_world_to_screen(View(), &reprojected, &picked);
        CHECK_NEAR(reprojected.x, float(px), 0.1f);
        CHECK_NEAR(reprojected.y, float(py), 0.1f);
    }
    return screen;
}

void Projection()
{
    auto* map = Map();
    float cx, cy;
    terrain_map_block_to_world(map, 2, 4, &cx, &cy);
    Camera(cx, cy, 0.3f, 180);
    objective_stop_showing_objective();
    const auto flat = ProjectAndPick(2, 4, cx, cy, terrain_map_world_z_get(map, cx, cy));

    // Make a nonplanar quad by moving just one corner. The chosen
    // diagonal changes which plane contains a point in its interior.
    const float s = map->blockSize;
    const float z = terrain_map_world_z_get(map, cx, cy);
    terrain_map_vertex_move(map, 3, 5, 12);
    for (int rotated : {0, 1}) {
        terrain_map_block_rotated_set(map, 2, 4, rotated);
        terrain_map_update(map, 0);
        const float x = cx + s/8, y = cy - s/8;
        const float expected = z + (rotated ? 7.5f : 3.0f);
        CHECK_NEAR(terrain_map_world_z_get(map, x, y), expected, 0.001f);
        TerrainMapVector3 normal{};
        terrain_map_world_normal_get(map, x, y, &normal);
        CHECK_NEAR(std::sqrt(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z), 1.0f, 1e-4);
        ProjectAndPick(2, 4, x, y, expected);
        const float oppositeX = cx - s/8, oppositeY = cy + s/8;
        const float oppositeZ = z + (rotated ? 4.5f : 0.0f);
        CHECK_NEAR(terrain_map_world_z_get(map, oppositeX, oppositeY), oppositeZ, 0.001f);
        ProjectAndPick(2, 4, oppositeX, oppositeY, oppositeZ);
    }
    Camera(cx - s/2, cy + s/4, 0.45f, 220);
    const auto moved = ProjectAndPick(2, 4, cx, cy, terrain_map_world_z_get(map, cx, cy));
    CHECK(std::fabs(moved.x-flat.x) + std::fabs(moved.y-flat.y) > 3, "camera change moves projected cell");
    float nx, ny;
    terrain_map_block_to_world(map, 1, 4, &nx, &ny);
    ProjectAndPick(1, 4, nx, ny, terrain_map_world_z_get(map, nx, ny));
}

unsigned ActiveTransitions()
{
    unsigned active = 0;
    for (const auto& transition : Map()->transitions)
        if (transition.flags & 1) ++active;
    return active;
}

void Transitions()
{
    auto* map = Map();
    auto* mesh = static_cast<Container*>(map->transMesh);
    terrain_map_transitions_update(map, 100);
    CHECK_EQ(ActiveTransitions(), 0u);
    std::array<int, TERRAIN_MAP_MAX_TRANSITIONS> groups{};
    // Fill each owned transition slot, then change another cell while
    // full. Its base texture still changes, without overwriting an owner.
    for (unsigned i = 0; i <= groups.size(); ++i) {
        const unsigned x = 1 + i%5, y = 1 + i/5;
        auto& block = Block(x, y);
        const unsigned old = block.texture;
        map->noFade[old] = 0;
        const unsigned next = (old+1)%64;
        TerrainMapVertex original[4]{};
        Container_Mesh_GetVertices(static_cast<Container*>(map->mesh),
                                   y*map->gridWidth+x, 0, 4, original);
        terrain_map_block_texture_fade_set(map, x, y, next, i%4);
        CHECK_EQ(block.texture, next);
        if (i < groups.size()) {
            const auto& slot = map->transitions[i];
            CHECK_EQ(slot.x, x);
            CHECK_EQ(slot.y, y);
            CHECK(slot.flags & 1, "slot owned");
            groups[i] = slot.groupID;
            for (unsigned j = 0; j < i; ++j)
                CHECK(groups[i] != groups[j], "separate group per live transition");
            for (unsigned j = 0; j < 4; ++j) {
                CHECK_NEAR(slot.uvs[j].x, original[j].tu, 1e-5);
                CHECK_NEAR(slot.uvs[j].y, original[j].tv, 1e-5);
            }
        }
    }
    CHECK_EQ(ActiveTransitions(), unsigned(groups.size()));
    // An active overlay follows changes to the base geometry while
    // retaining its captured UVs. Inspect both real mesh groups.
    terrain_map_vertex_move(map, 2, 2, 6);
    terrain_map_transitions_update(map, 10);
    TerrainMapVertex overlay[4]{}, base[4]{};
    Container_Mesh_GetVertices(mesh, groups[0], 0, 4, overlay);
    Container_Mesh_GetVertices(static_cast<Container*>(map->mesh), map->gridWidth + 1, 0, 4, base);
    for (unsigned i = 0; i < 4; ++i) {
        CHECK_NEAR(overlay[i].position.x, base[i].position.x, 1e-5);
        CHECK_NEAR(overlay[i].position.y, base[i].position.y, 1e-5);
        CHECK_NEAR(overlay[i].position.z, base[i].position.z, 1e-5);
        CHECK_NEAR(overlay[i].tu, map->transitions[0].uvs[i].x, 1e-5);
        CHECK_NEAR(overlay[i].tv, map->transitions[0].uvs[i].y, 1e-5);
    }
    for (unsigned i = 0; i < groups.size(); ++i) {
        const auto& slot = map->transitions[i];
        CHECK_EQ(slot.x, 1+i%5);
        CHECK_EQ(slot.y, 1+i/5);
        CHECK_EQ(slot.groupID, groups[i]);
        CHECK_NEAR(slot.timer, 0.45f, 1e-5);
        CHECK_EQ(Container_Mesh_IsGroupHidden(mesh, groups[i]), 0);
    }
    terrain_map_transitions_update(map, 20);
    CHECK_EQ(ActiveTransitions(), 0u);
    for (int group : groups) CHECK_EQ(Container_Mesh_IsGroupHidden(mesh, group), 1);
    const unsigned x = 2, y = 4, old = Block(x, y).texture;
    map->noFade[old] = 0;
    terrain_map_block_texture_fade_set(map, x, y, (old+1)%64, 3);
    CHECK_EQ(ActiveTransitions(), 1u);
    CHECK_EQ(map->transitions[0].groupID, groups[0]);
    CHECK_EQ(Container_Mesh_IsGroupHidden(mesh, groups[0]), 0);
    terrain_map_block_texture_fade_set(map, x, y, Block(x, y).texture, 3);
    CHECK_EQ(ActiveTransitions(), 1u);
    // The active transition is intentionally left live for full shutdown.
}

void Mission()
{
    auto* level = legoGlobs.currLevel;
    objective_stop_showing_objective();
    const char config[] =
        "Lego { Grid { DontShowObjectiveAdvisor TRUE DontShowObjectiveAcheiveAdvisor TRUE "
        "DontShowObjectiveFailedAdvisor TRUE TimerObjective 3:HitTimeFailObjective "
        "OreObjective 2 BlockObjective 2,4 } }";
    auto* parsed = block_parse(config, sizeof(config)-1);
    if (!CHECK(parsed != nullptr, "parse generated mission conditions")) return;
    objective_load_original(parsed, "Lego", "Grid", level, 640, 480);
    block_close(parsed);
    float x, y;
    terrain_map_block_to_world(Map(), 2, 3, &x, &y);
    auto* object = legoobject_create_in_world(legoGlobs.miniFigureData, 2, 0, 0, x, y, 0);
    if (!CHECK(object != nullptr, "create neutral object outside target")) return;

    // The original objective unit reads the ore quota from the legacy
    // crystals field. Stage its input with the actual balance operations.
    level->crystals = 0;
    lego_level_resource_balance_add_and_sound(0);
    lego_level_resource_balance_add_and_sound(0);
    CHECK_EQ(level->crystals, 2);
    objective_tick_target(nullptr, level, 10, 0);
    CHECK_EQ(level->objective.timer, 65.0f);
    CHECK_EQ(objective_is_ended_original(), 0);

    terrain_map_block_to_world(Map(), 2, 4, &x, &y);
    legoobject_world_position_set(object, x, y, 0, 0);
    lego_upgrade_resource_debit(0, 1);
    CHECK_EQ(level->crystals, 1);
    objective_tick_target(nullptr, level, 5, 0);
    CHECK_EQ(level->objective.timer, 60.0f);
    CHECK_EQ(objective_is_ended_original(), 0);

    lego_level_resource_balance_add_and_sound(0);
    CHECK_EQ(level->crystals, 2);
    objective_tick_target(nullptr, level, 1, 0);
    CHECK_EQ(level->objective.timer, 59.0f);
    CHECK_EQ(objective_is_ended_original(), 1);
    CHECK_EQ(level->status, 1);
    CHECK_EQ(level_objective_result_get(), 1);
}
}

int main(int argc, char** argv)
{
    const std::string scenario = argc == 2 ? argv[1] : "projection";
    if (!CHECK(scenario == "projection" || scenario == "transitions" || scenario == "mission", "known scenario") ||
        !synthetic_fixture::initialise_full_game(("environment_runtime_"+scenario).c_str(),
                                                "EnvironmentRuntime",
                                                " Figure { Levels 1 CollRadius 7 CollHeight 4 PickSphere 9 CrossLand TRUE RouteSpeed 8 CanClearRubble TRUE NumOfToolsCanCarry 2 } ")) return lrr_test_result();
    if (scenario == "projection") Projection();
    else if (scenario == "transitions") Transitions();
    else Mission();
    lego_engine_shutdown();
    CHECK_EQ(legoGlobs.currLevel, nullptr);
    directdraw_shutdown();
    return lrr_test_result("environment_runtime");
}
