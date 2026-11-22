// SPDX-License-Identifier: LGPL-3.0-or-later
/* mesh_unit.c — parse a texture sequence's base and fixed-width suffix,
 * then produce successive numbered texture names. */
#include "gods98/mesh_api.h"
#include "test/lrr_test.h"

int main()
{
    char base[32] = {}, next[32] = {};
    unsigned long frame = 0, digits = 0;
    CHECK_EQ(mesh_get_texture_seq_info("water_009", base, &frame, &digits), 1);
    CHECK_STREQ(base, "water_");
    CHECK_EQ(frame, 9ul);
    CHECK_EQ(digits, 3ul);

    mesh_get_next_in_sequence(base, next, &frame, digits);
    CHECK_STREQ(next, "water_009");
    CHECK_EQ(frame, 10ul);
    mesh_get_next_in_sequence(base, next, &frame, digits);
    CHECK_STREQ(next, "water_010");
    CHECK_EQ(frame, 11ul);

    CHECK_EQ(mesh_get_texture_seq_info("water", base, &frame, &digits), 0);
    CHECK_EQ(mesh_get_texture_seq_info("012", base, &frame, &digits), 0);
    CHECK_EQ(mesh_get_texture_seq_info("a1b000", base, &frame, &digits), 1);
    CHECK_STREQ(base, "a1b");
    CHECK_EQ(frame, 0ul);
    CHECK_EQ(digits, 3ul);
    mesh_get_next_in_sequence(base, next, &frame, digits);
    CHECK_STREQ(next, "a1b000");
    CHECK_EQ(frame, 1ul);
    return lrr_test_result();
}
