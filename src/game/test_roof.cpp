// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_roof — quad vertex order when the diagonal changes, and the
 * plane and vertex normals written for a sloping block. */
#include "game/roof.h"
#include "test/lrr_test.h"

struct Vertex {
    Roof_Vector3F position;
    Roof_Vector3F normal;
    float u, v;
    unsigned int colour;
};
static_assert(sizeof(Vertex) == 0x24, "the mesh vertex layout");
static Vertex vertices[4];

extern "C" unsigned long __wrap_container_mesh_set_vertices(void*, unsigned long,
                                                            unsigned long first,
                                                            unsigned long count,
                                                            const void* input)
{
    std::memcpy(vertices + first, input, count * sizeof(Vertex));
    return 1;
}

extern "C" unsigned long __wrap_lwo_layer_polygon_vertex_data_read(void*, unsigned long,
                                                                   unsigned long first,
                                                                   unsigned long count,
                                                                   void* output)
{
    std::memcpy(output, vertices + first, count * sizeof(Vertex));
    return 1;
}

int main(void)
{
    RoofBlock grid[2] = { { 0, 0 }, { 1, ROOF_FLAG_NOGROUP } };
    roofGlobs.width = 2;
    roofGlobs.height = 1;
    roofGlobs.grid = grid;
    const Roof_Vector3F nw = { 0.0f, 0.0f, 0.0f };
    const Roof_Vector3F ne = { 2.0f, 0.0f, 0.0f };
    const Roof_Vector3F se = { 2.0f, 2.0f, 2.0f };
    const Roof_Vector3F sw = { 0.0f, 2.0f, 2.0f };
    roof_set_block_roof_vertices(0, 0, &nw, &ne, &se, &sw);
    CHECK_EQ(grid[0].flags & ROOF_FLAG_ROTATED, 0);
    CHECK_EQ(vertices[0].position.y, 2.0f);
    CHECK_EQ(vertices[3].position.y, 0.0f);
    CHECK_EQ(vertices[3].position.x, 0.0f);
    CHECK_EQ(roofGlobs.needUpdate, 1);

    Roof_Vector3F normal = {};
    CHECK_EQ(roof_get_block_plane_normal(1, 0, &normal), 0);
    CHECK_EQ(roof_get_block_plane_normal(0, 1, &normal), 0);
    CHECK_EQ(roof_get_block_plane_normal(0, 0, &normal), 1);
    CHECK_NEAR(normal.x, 0.0f, 1e-6);
    CHECK_NEAR(normal.y, 0.70710678f, 1e-6);
    CHECK_NEAR(normal.z, -0.70710678f, 1e-6);

    roof_update();
    CHECK_EQ(roofGlobs.needUpdate, 0);
    CHECK_EQ(grid[0].flags & ROOF_FLAG_UPDATE, 0);
    for (const Vertex& vertex : vertices) {
        CHECK_NEAR(vertex.normal.x, normal.x, 1e-6);
        CHECK_NEAR(vertex.normal.y, normal.y, 1e-6);
        CHECK_NEAR(vertex.normal.z, normal.z, 1e-6);
    }

    Roof_Vector3F lowNE = ne;
    lowNE.z = -2.0f;
    roof_set_block_roof_vertices(0, 0, &nw, &lowNE, &se, &sw);
    CHECK(grid[0].flags & ROOF_FLAG_ROTATED, "a lower NE corner changes the diagonal");
    CHECK_EQ(vertices[0].position.y, 0.0f);
    CHECK_EQ(vertices[3].position.x, 2.0f);
    CHECK_EQ(vertices[3].position.z, -2.0f);
    roofGlobs.grid = nullptr;
    return lrr_test_result();
}
