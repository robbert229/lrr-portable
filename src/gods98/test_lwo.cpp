// SPDX-License-Identifier: LGPL-3.0-or-later
/* lwo.c — read a synthetic LWOB triangle, resolve its surface indices and
 * reorder surface records to match the names table. The second material
 * has independent color and transparency texture layers. */
#include <cstdint>
#include <fstream>

#include "gods98/lwo.h"
#include "platform/native/test_fixture.h"
#include "test/lrr_test.h"

#include "gods98/files.h"

int main()
{
    const uint8_t object[] = {
        'F', 'O', 'R', 'M', 0, 0, 1, 96, 'L', 'W', 'O', 'B',
        'P', 'N', 'T', 'S', 0, 0, 0, 36,
        0, 0, 0, 0,        0, 0, 0, 0,        0, 0, 0, 0,
        0x3f, 0x80, 0, 0,  0, 0, 0, 0,        0, 0, 0, 0,
        0, 0, 0, 0,        0xc0, 0, 0, 0,     0x3f, 0, 0, 0,
        'P', 'O', 'L', 'S', 0, 0, 0, 10,
        0, 3, 0, 0, 0, 2, 0, 1, 0, 2, // Vertices 0, 2, 1, surface 2.
        'S', 'R', 'F', 'S', 0, 0, 0, 12,
        'S', 't', 'o', 'n', 'e', 0, 'M', 'e', 't', 'a', 'l', 0,
        'S', 'U', 'R', 'F', 0, 0, 0, 234,
        'M', 'e', 't', 'a', 'l', 0, 'C', 'O', 'L', 'R', 0, 4, 10, 20, 30, 0,
        'F', 'L', 'A', 'G', 0, 2, 1, 4, // Double-sided and smoothing.
        'C', 'T', 'E', 'X', 0, 22,
        'C', 'y', 'l', 'i', 'n', 'd', 'r', 'i', 'c', 'a', 'l', ' ',
        'I', 'm', 'a', 'g', 'e', ' ', 'M', 'a', 'p', 0,
        'T', 'F', 'L', 'G', 0, 2, 0, 0x21, // X axis and pixel blending.
        'T', 'S', 'I', 'Z', 0, 12,
        0x3f, 0, 0, 0, 0x40, 0, 0, 0, 0x40, 0x80, 0, 0, // 0.5, 2, 4.
        'T', 'C', 'T', 'R', 0, 12,
        0xbf, 0x80, 0, 0, 0x3f, 0x80, 0, 0, 0x3e, 0x80, 0, 0, // -1, 1, 0.25.
        'T', 'I', 'M', 'G', 0, 24,
        's', 'a', 'm', 'p', 'l', 'e', '0', '1', '.', 'b', 'm', 'p', ' ',
        '(', 's', 'e', 'q', 'u', 'e', 'n', 'c', 'e', ')', 0,
        // The material scalar values are unsigned 8.8 fixed point.
        'L', 'U', 'M', 'I', 0, 2, 0, 64,
        'D', 'I', 'F', 'F', 0, 2, 0, 192,
        'T', 'R', 'A', 'N', 0, 2, 0, 32,
        'R', 'E', 'F', 'L', 0, 2, 0, 128,
        'S', 'P', 'E', 'C', 0, 2, 0, 96,
        'G', 'L', 'O', 'S', 0, 2, 0, 224,
        'T', 'T', 'E', 'X', 0, 20,
        'S', 'p', 'h', 'e', 'r', 'i', 'c', 'a', 'l', ' ',
        'I', 'm', 'a', 'g', 'e', ' ', 'M', 'a', 'p', 0,
        'T', 'F', 'L', 'G', 0, 2, 0, 4, // Z axis, independent of the base texture.
        'T', 'I', 'M', 'G', 0, 12,
        'm', 'a', 's', 'k', '0', '1', '.', 'b', 'm', 'p', 0, 0,
        'T', 'R', 'A', 'N', 0, 2, 0, 128,
        'S', 'U', 'R', 'F', 0, 0, 0, 16,
        'S', 't', 'o', 'n', 'e', 0, 'C', 'O', 'L', 'R', 0, 4, 90, 80, 70, 0,
    };
    static_assert(sizeof(object) == 352 + 8);

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

            const auto& plain = mesh->aoSurface[0];
            CHECK_EQ(plain.srfPath, nullptr);
            CHECK_EQ(plain.srfNextTex, nullptr);
            CHECK_EQ(plain.srfFlags, 0u);
            const auto& material = mesh->aoSurface[1];
            CHECK_EQ(material.srfFlags, 0x104u);
            CHECK_EQ(material.srfTexMode, 1u);
            CHECK_EQ(material.srfTexFlags, 0xa1u); // Axis, blending, sequence.
            CHECK_STREQ(material.srfPath, "sample01.bmp");
            CHECK_EQ(material.srfTSIZ[0], 0.5f);
            CHECK_EQ(material.srfTSIZ[1], 2.0f);
            CHECK_EQ(material.srfTSIZ[2], 4.0f);
            CHECK_EQ(material.srfTCTR[0], -1.0f);
            CHECK_EQ(material.srfTCTR[1], 1.0f);
            CHECK_EQ(material.srfTCTR[2], 0.25f);
            CHECK_EQ(material.srfLuminous, 0.25f);
            CHECK_EQ(material.srfDiffuse, 0.75f);
            CHECK_EQ(material.srfTransparent, 0.125f);
            CHECK_EQ(material.srfReflect, 0.5f);
            CHECK_EQ(material.srfSpecular, 0.375f);
            CHECK_EQ(material.srfGlossy, 0.875f);
            if (CHECK(material.srfNextTex != nullptr, "transparency layer is owned by material")) {
                const auto& layer = *material.srfNextTex;
                CHECK_EQ(layer.srfTexMode, 2u);
                CHECK_EQ(layer.srfTexFlags, 4u);
                CHECK_STREQ(layer.srfPath, "mask01.bmp");
                CHECK_EQ(layer.srfTransparent, 0.5f);
                CHECK_EQ(layer.srfNextTex, nullptr);
            }
        }
        CHECK_EQ(lwo_app_obj_free(mesh), 1);
    }
    return lrr_test_result();
}
