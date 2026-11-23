// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_detail — flat and skewed detail mesh transforms and the packed
 * texture selector's high-nibble column and low-nibble row. */
#include "game/detail_link_names.h"
#include "game/detail.h"
#include "test/lrr_test.h"

static void check_matrix(float x2, float y2, float xScale, float shear)
{
    float result[16];
    for (float& entry : result)
        entry = -1.0f;
    detail_matrix_setup(result, 4.0f, 8.0f, 2.0f, x2, y2);
    const float expected[] = { xScale, shear, 0.0f, 0.0f,
                               0.0f, 0.25f, 0.0f, 0.0f,
                               0.0f, 0.0f, 1.0f, 0.0f,
                               0.0f, 0.0f, 0.0f, 1.0f };
    for (unsigned int i = 0; i < 16u; ++i)
        CHECK_EQ(result[i], expected[i]);
}

int main(void)
{
    check_matrix(4.0f, 0.0f, 1.0f, 0.0f);
    check_matrix(8.0f, -4.0f, 2.0f, -1.0f);

    int markers[6] = {};
    void* textures[] = { &markers[0], &markers[1], &markers[2],
                         &markers[3], &markers[4], &markers[5] };
    Detail_TextureSet set = { 3u, 2u, textures };
    CHECK_EQ(detail_get_texture(nullptr, 0x00), nullptr);
    CHECK_EQ(detail_get_texture(&set, 0x00), &markers[0]);
    CHECK_EQ(detail_get_texture(&set, 0x20), &markers[2]);
    CHECK_EQ(detail_get_texture(&set, 0x01), &markers[3]);
    CHECK_EQ(detail_get_texture(&set, 0x21), &markers[5]);
    return lrr_test_result("detail");
}
