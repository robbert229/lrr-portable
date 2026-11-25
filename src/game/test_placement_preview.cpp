// SPDX-License-Identifier: LGPL-3.0-or-later
/* Existing placement admission and commit on a generated asymmetric
 * footprint and grid, with no authored building artwork. */
#include <array>
#include "gods98/container_link_names.h"
#include "gods98/container_unit.h"
#include "gods98/directdraw.h"
#include "game/aitask.h"
#include "game/aitask_globs.h"
#include "game/aitask_record.h"
#include "game/construction_link_names.h"
#include "game/construction.h"
#include "game/lego_unit.h"
#include "game/legoobject_unit.h"
#include "game/selectplace_link_names.h"
#include "game/selectplace.h"
#include "game/terrain_map.h"
#include "test/lrr_test.h"
#include "test/synthetic_initializer_fixture.h"
#include "game/lego_globs.h"

extern "C" int legoobject_building_placement_preview_update(int, int, int, int, float*, int, int, int, void*);
extern "C" int terrain_cell_variant_flag_propagate_current(const int*);

namespace {
constexpr char Stats[] = " Structure { Levels 1 CollRadius 4 CostOre 2 CostCrystal 1 StoreObjects TRUE } ";
constexpr unsigned Floor = 0x8u, Clear = 0x200000u, Path = 0x20000000u;
constexpr std::array<SelectPlace_Point2I, 3> Shape{{{0,0},{1,0},{0,2}}};

Lego_Block& Cell(int x, int y)
{
    return legoGlobs.currLevel->blocks[y * legoGlobs.currLevel->width + x];
}

unsigned Requests(unsigned handle, int kind)
{
    unsigned result = 0;
    for (AITask* task = aiTaskGlobs.globalHead; task; task = task->next)
        if (task->type == 7 && task->zone == static_cast<int>(handle) && task->field_20 == kind && !(task->flags & 0x20)) ++result;
    return result;
}

void Scenario()
{
    auto* map = legoGlobs.currLevel->map;
    auto* preview = static_cast<SelectPlace*>(legoGlobs.field_2f4);
    if (!CHECK(preview && preview->contMesh, "initialised preview mesh")) return;
    SelectPlace_Point2I origin{1, 2};
    int pos[] = {origin.x, origin.y};
    float mouse[2];
    terrain_map_block_to_world(map, origin.x, origin.y, &mouse[0], &mouse[1]);
    mouse[1] += 4;

    // The fixture's three footprint cells are clear, but none has a path
    // neighbour. The real admission routine refuses the placement.
    for (const auto& pt : Shape) {
        auto& cell = Cell(origin.x + pt.x, origin.y + pt.y);
        cell.flags1 = (cell.flags1 | Floor | Clear) & ~Path;
        cell.flags2 = 0;
        cell.f_02 = 0;
    }
    CHECK_EQ(selectplace_check_and_update(preview, &origin, Shape.data(), Shape.size(), 0, map, 0), nullptr);
    CHECK_EQ(legoobject_building_placement_preview_update(4, 0, 1, 0, mouse, origin.x, origin.y, 1, preview), 1);
    CHECK_EQ(construction_zone_find_by_handle_or_at_block(pos, nullptr), nullptr);

    // Give one footprint cell a real connected path neighbour.
    Cell(2, 3).flags1 |= Floor | Clear;
    int pathPos[] = {2, 3};
    CHECK_EQ(terrain_cell_variant_flag_propagate_current(pathPos), 1);
    CHECK(Cell(2, 3).flags1 & Path, "path admitted");
    auto* placed = selectplace_check_and_update(preview, &origin, Shape.data(), Shape.size(), 0, map, 0);
    if (!CHECK(placed != nullptr, "path-adjacent footprint admitted")) return;
    for (unsigned i = 0; i < Shape.size(); ++i) {
        CHECK_EQ(placed[i].x, origin.x + Shape[i].x);
        CHECK_EQ(placed[i].y, origin.y + Shape[i].y);
        CHECK_EQ(Container_Mesh_IsGroupHidden(preview->contMesh, i), 0);
    }
    CHECK_EQ(Container_Mesh_IsGroupHidden(preview->contMesh, Shape.size()), 1);
    TerrainMapVertex vertices[20] = {};
    Container_Mesh_GetVertices(preview->contMesh, 0, 0, 20, vertices);
    TerrainMapVector3 corners[4] = {};
    terrain_map_block_vertices_raw_get(map, origin.x, origin.y, corners);
    for (unsigned i = 0; i < 4; ++i) {
        CHECK_NEAR(vertices[i].position.x, corners[i].x, 1e-4);
        CHECK_NEAR(vertices[i].position.y, corners[i].y, 1e-4);
        CHECK_NEAR(vertices[i].position.z, corners[i].z - preview->tileDepth, 1e-4);
    }

    // A temporarily uneven footprint is rejected; restoring the vertex
    // re-enables the same shape without changing its path adjacency.
    terrain_map_vertex_move(map, 1, 2, 100);
    CHECK_EQ(selectplace_check_and_update(preview, &origin, Shape.data(), Shape.size(), 0, map, 0), nullptr);
    terrain_map_vertex_move(map, 1, 2, -100);
    CHECK(selectplace_check_and_update(preview, &origin, Shape.data(), Shape.size(), 0, map, 0) != nullptr,
          "level footprint admitted again");
    const auto savedMaterial = Cell(2, 2).f_02;
    Cell(2, 2).f_02 = 6;
    CHECK_EQ(selectplace_check_and_update(preview, &origin, Shape.data(), Shape.size(), 0, map, 0), nullptr);
    Cell(2, 2).f_02 = savedMaterial;
    // Three quarter turns encounter the abstract wall or grid edge;
    // the preview tries alternatives and keeps the one valid rotation.
    terrain_map_block_to_world(map, origin.x, origin.y, &mouse[0], &mouse[1]);
    mouse[0] += 4;
    CHECK_EQ(legoobject_building_placement_preview_update(4, 0, 0, 1, mouse, origin.x, origin.y, 1, preview), 1);
    CHECK_EQ(legoGlobs.field_388, 0);
    CHECK_EQ(construction_zone_find_by_handle_or_at_block(pos, nullptr), nullptr);
    CHECK_EQ(legoobject_building_placement_preview_update(4, 0, 0, 0, mouse, origin.x, origin.y, 0, preview), 1);
    CHECK_EQ(Container_IsHidden(preview->contMesh), 1);
    CHECK_EQ(legoobject_building_placement_preview_update(4, 0, 1, 0, mouse, 3, 2, 1, preview), 1);
    int wall[] = {3, 2};
    CHECK_EQ(construction_zone_find_by_handle_or_at_block(wall, nullptr), nullptr);

    // Commit the actual preview, which reserves the three cells and
    // requests the configured material quantities, but does not complete
    // any building or supply the resources automatically.
    CHECK_EQ(legoobject_building_placement_preview_update(4, 0, 1, 0, mouse, origin.x, origin.y, 1, preview), 0);
    auto* zone = construction_zone_find_by_handle_or_at_block(pos, nullptr);
    if (!CHECK(zone != nullptr, "construction zone created by commit")) return;
    const unsigned handle = zone->handle;
    CHECK_EQ(zone->shapeCount, 3u);
    CHECK_EQ(zone->direction, 0u);
    CHECK_EQ(zone->requestCount, 3u);
    CHECK_EQ(zone->placedCount, 0u);
    CHECK_EQ(Requests(handle, 6), 1u);
    CHECK_EQ(Requests(handle, 7), 2u);
    for (unsigned i = 0; i < Shape.size(); ++i) {
        CHECK_EQ(zone->shapePoints[i].x, origin.x + Shape[i].x);
        CHECK_EQ(zone->shapePoints[i].y, origin.y + Shape[i].y);
        CHECK_EQ(Cell(zone->shapePoints[i].x, zone->shapePoints[i].y).construct, zone);
    }
    CHECK_EQ(Container_IsHidden(preview->contMesh), 1);
    construction_zone_cancel_building(pos);
    CHECK_EQ(construction_zone_find_by_handle_or_at_block(pos, nullptr), nullptr);
    CHECK_EQ(Requests(handle, 6), 0u);
    CHECK_EQ(Requests(handle, 7), 0u);
    for (const auto& pt : Shape) CHECK_EQ(Cell(origin.x + pt.x, origin.y + pt.y).construct, nullptr);
}
}

int main()
{
    if (!synthetic_fixture::initialise_full_game("placement_preview", "PlacementPreview", Stats))
        return lrr_test_result();
    Scenario();
    lego_engine_shutdown();
    CHECK_EQ(legoGlobs.currLevel, nullptr);
    directdraw_shutdown();
    return lrr_test_result("placement_preview");
}
