// SPDX-License-Identifier: LGPL-3.0-or-later
#include "game/water_link_names.h"
#include "game/water.h"
#include "test/lrr_test.h"

static float surface;
static unsigned wet, dry;
extern "C" void __wrap_scene_render_lock_toggle(void*, int) {}
extern "C" void __wrap_render_object_set_packed_rgba_from_float_channels(void*, float, float, float, float) {}
extern "C" unsigned long __wrap_lwo_layer_polygon_vertex_data_read(void*, unsigned long,
    unsigned long, unsigned long count, void* output)
{
    std::memset(output, 0, count * sizeof(TerrainMapVertex));
    return count;
}
extern "C" unsigned long __wrap_container_mesh_set_vertices(void*, unsigned long,
    unsigned long, unsigned long count, const void* input)
{
    auto* vertices = static_cast<const TerrainMapVertex*>(input);
    for (unsigned long i = 0; i < count; ++i) CHECK_EQ(vertices[i].position.z, vertices[0].position.z);
    surface = vertices[0].position.z;
    return count;
}
extern "C" int __wrap_terrain_map_block_vertices_get(TerrainMap*, unsigned, unsigned, TerrainMapVector3* v)
{
    v[0] = {1, -1, -2};
    v[1] = {2, -1, -2};
    v[2] = {2, -2, -2};
    v[3] = {1, -2, -2};
    return 1;
}
extern "C" void __wrap_lego_terrain_cell_edge_flag_set_and_update(Lego_Level*, int, int, int on)
{
    if (on) ++wet; else ++dry;
}

int main()
{
    TerrainMap map = {};
    map.gridWidth = map.gridHeight = 50;
    map.blockSize = 10;
    Lego_Level level = {};
    level.map = &map;
    level.digDepth = 10;
    water_create_pool(1, 1, 2);
    auto& pool = waterGlobs.poolList[0];
    pool.startLevel = pool.level = -5;
    pool.flags = WATER_FLAG_DRAINED;
    pool.entryCount = 1;
    pool.entries[0] = {0, 0, 0, 0, 0, 0};
    water_destroy_wall_complete(&level, 1, 0);
    CHECK_EQ(pool.entries[0].active, 1);
    water_update(&level, 5);
    CHECK(pool.level > -5 && pool.level < -2, "water rises before reaching outlet");
    CHECK_EQ(surface, pool.level);
    CHECK_EQ(wet, 1u);
    water_update(&level, 20);
    CHECK_EQ(pool.level, -2.0f);
    CHECK(pool.flags & WATER_FLAG_FILLED, "pool filled to outlet");
    water_seal_block(1, 0);
    water_update(&level, 20);
    CHECK_EQ(pool.level, -3.0f);
    CHECK_EQ(surface, -3.0f);
    water_update(&level, 50);
    CHECK_EQ(pool.level, -5.0f);
    CHECK(pool.flags & WATER_FLAG_DRAINED, "pool drains to original level");
    CHECK_EQ(wet, 4u);
    CHECK_EQ(dry, 4u);
    return lrr_test_result();
}
