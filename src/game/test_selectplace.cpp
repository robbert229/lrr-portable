// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_selectplace — rotate an asymmetric building footprint by quarter
 * turns, preserve a repeated entrance point, and translate every cell. */
#include "game/selectplace_link_names.h"
#include "game/selectplace.h"
#include "test/lrr_test.h"

int main(void)
{
    const SelectPlace_Point2I origin = { 10, 20 };
    const SelectPlace_Point2I shape[] = { { 0, 0 }, { 2, -1 }, { -1, -2 }, { -1, -2 } };
    const SelectPlace_Point2I expected[][4] = {
        { { 10, 20 }, { 12, 19 }, { 9, 18 }, { 9, 18 } },
        { { 10, 20 }, { 11, 22 }, { 12, 19 }, { 12, 19 } },
        { { 10, 20 }, { 8, 21 }, { 11, 22 }, { 11, 22 } },
        { { 10, 20 }, { 9, 18 }, { 8, 21 }, { 8, 21 } },
    };
    for (int rotation = -1; rotation <= 4; ++rotation) {
        const SelectPlace_Point2I* result = selectplace_transform_shape(&origin, shape, 4, rotation);
        const SelectPlace_Point2I* rotated = expected[(rotation + 4) % 4];
        for (unsigned int i = 0; i < 4; ++i) {
            CHECK_EQ(result[i].x, rotated[i].x);
            CHECK_EQ(result[i].y, rotated[i].y);
        }
    }
    CHECK_EQ(shape[1].x, 2);
    CHECK_EQ(shape[1].y, -1);
    return lrr_test_result();
}
