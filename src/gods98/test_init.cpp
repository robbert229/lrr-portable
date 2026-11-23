// SPDX-License-Identifier: LGPL-3.0-or-later
/* init.c — filter a synthetic display-mode table with wildcard fields
 * and resolve exact display-mode descriptions without opening a window. */
#include "gods98/init.h"
#include "test/lrr_test.h"

int main()
{
    const DirectDraw_Mode modes[] = {
        {640, 480, 16, "640x480x16", DIRECTDRAW_FLAG_VALID},
        {640, 480, 32, "640x480x32", DIRECTDRAW_FLAG_VALID},
        {800, 600, 32, "800x600x32", DIRECTDRAW_FLAG_VALID},
        {1024, 768, 32, "1024x768x32", DIRECTDRAW_FLAG_VALID},
        {800, 480, 24, "800x480x24", DIRECTDRAW_FLAG_VALID},
        {1024, 600, 16, "1024x600x16", DIRECTDRAW_FLAG_VALID},
    };
    initGlobs.modeCount = sizeof(modes) / sizeof(modes[0]);
    std::memcpy(initGlobs.modes, modes, sizeof(modes));
    initGlobs.validModeCount = 0;
    for (unsigned int i = 0; i < initGlobs.modeCount; ++i) CHECK_EQ(init_is_valid_mode(i), 1);

    init_add_valid_mode(640, 0, 16);
    CHECK_EQ(init_is_valid_mode(0), 1);
    CHECK_EQ(init_is_valid_mode(1), 0);
    CHECK_EQ(init_is_valid_mode(2), 0);

    init_add_valid_mode(0, 600, 32);
    init_add_valid_mode(800, 480, 0);
    CHECK_EQ(initGlobs.validModeCount, 3u);
    const int accepted[] = {1, 0, 1, 0, 1, 0};
    for (unsigned int i = 0; i < initGlobs.modeCount; ++i)
        CHECK_EQ(init_is_valid_mode(i), accepted[i]);

    init_add_valid_mode(0, 0, 0);
    for (unsigned int i = 0; i < initGlobs.modeCount; ++i) CHECK_EQ(init_is_valid_mode(i), 1);

    unsigned int selected = 99;
    CHECK_EQ(init_get_mode("800x600x32", &selected), 1);
    CHECK_EQ(selected, 2u);
    CHECK_EQ(init_get_mode("800x600", &selected), 0);
    CHECK_EQ(selected, 2u);
    return lrr_test_result();
}
