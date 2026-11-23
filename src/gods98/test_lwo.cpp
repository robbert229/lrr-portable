// SPDX-License-Identifier: LGPL-3.0-or-later
/* lwo.c — read a synthetic LWOB triangle, resolve its surface indices and
 * reorder surface records to match the names table. */
#include <cstdint>
#include <fstream>

#include "gods98/lwo.h"
#include "platform/native/test_fixture.h"
#include "test/lrr_test.h"

#include "gods98/files.h"

int main()
{
    const uint8_t object[] = {
        'F', 'O', 'R', 'M', 0, 0, 0, 134, 'L', 'W', 'O', 'B',
        'P', 'N', 'T', 'S', 0, 0, 0, 36,
        0, 0, 0, 0,        0, 0, 0, 0,        0, 0, 0, 0,
        0x3f, 0x80, 0, 0,  0, 0, 0, 0,        0, 0, 0, 0,
        0, 0, 0, 0,        0xc0, 0, 0, 0,     0x3f, 0, 0, 0,
        'P', 'O', 'L', 'S', 0, 0, 0, 10,
        0, 3, 0, 0, 0, 2, 0, 1, 0, 2, // Vertices 0, 2, 1, surface 2.
        'S', 'R', 'F', 'S', 0, 0, 0, 12,
        'S', 't', 'o', 'n', 'e', 0, 'M', 'e', 't', 'a', 'l', 0,
        'S', 'U', 'R', 'F', 0, 0, 0, 16,
        'M', 'e', 't', 'a', 'l', 0, 'C', 'O', 'L', 'R', 0, 4, 10, 20, 30, 0,
        'S', 'U', 'R', 'F', 0, 0, 0, 16,
        'S', 't', 'o', 'n', 'e', 0, 'C', 'O', 'L', 'R', 0, 4, 90, 80, 70, 0,
    };
    static_assert(sizeof(object) == 134 + 8);

    if (!CHECK(fixture_enter("lwo_fixture") && fixture_mkdir("Data"), "create LWO fixture"))
        return lrr_test_result();
    {
        std::ofstream file("Data/triangle.lwo", std::ios::binary);
        file.write(reinterpret_cast<const char*>(object), sizeof(object));
        file.close();
        if (!CHECK(file.good(), "write triangle")) return lrr_test_result();
    }
    file_initialise("lwo_fixture", 0, "");

    CHECK_EQ(lwo_texture_projection_mode_parse("PLANAR"), 0);
    CHECK_EQ(lwo_texture_projection_mode_parse("cylindrical image map"), 1);
    CHECK_EQ(lwo_texture_projection_mode_parse("Spherical"), 2);

    APPOBJ* mesh = nullptr;
    if (CHECK_EQ(lwo_app_obj_load("triangle", &mesh, 0), 1) &&
        CHECK(mesh != nullptr, "triangle has an object record")) {
        CHECK_EQ(mesh->aoSize.lwVertCount, 3u);
        CHECK_EQ(mesh->aoSize.lwPolyCount, 1u);
        CHECK_EQ(mesh->aoSize.lwSurfaceCount, 2u);
        CHECK_EQ(mesh->aoFileUV, nullptr);
        if (CHECK(mesh->aoVerts != nullptr, "vertex positions are loaded")) {
            CHECK_NEAR(mesh->aoVerts[3], 1.0f, 1e-6);
            CHECK_NEAR(mesh->aoVerts[7], -2.0f, 1e-6);
            CHECK_NEAR(mesh->aoVerts[8], 0.5f, 1e-6);
        }
        if (CHECK(mesh->aoPoly != nullptr, "polygon is loaded")) {
            const auto& polygon = mesh->aoPoly[0];
            CHECK_EQ(polygon.plyCount, 3u);
            CHECK_EQ(polygon.srfIndex, 1u);
            if (CHECK(polygon.plyData != nullptr, "the polygon has vertex indices")) {
                CHECK_EQ(polygon.plyData[0], 0u);
                CHECK_EQ(polygon.plyData[1], 2u);
                CHECK_EQ(polygon.plyData[2], 1u);
            }
        }
        if (CHECK(mesh->aoSurface != nullptr, "surfaces are loaded")) {
            CHECK_STREQ(mesh->aoSurface[0].srfName, "Stone");
            CHECK_EQ(mesh->aoSurface[0].srfCol[0], 90u);
            CHECK_EQ(mesh->aoSurface[0].srfCol[1], 80u);
            CHECK_EQ(mesh->aoSurface[0].srfCol[2], 70u);
            CHECK_STREQ(mesh->aoSurface[1].srfName, "Metal");
            CHECK_EQ(mesh->aoSurface[1].srfCol[0], 10u);
            CHECK_EQ(mesh->aoSurface[1].srfCol[1], 20u);
            CHECK_EQ(mesh->aoSurface[1].srfCol[2], 30u);
        }
        CHECK_EQ(lwo_app_obj_free(mesh), 1);
    }
    return lrr_test_result();
}
