// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_scrollinfo — scrollbar rounding, arrow bounds and drag positions,
 * using synthetic image dimensions to check where the thumb is drawn. */
#include "game/panel_unit.h"
#include "game/scrollinfo_link_names.h"
#include "game/scrollinfo.h"
#include "test/lrr_test.h"

static Image bar, thumb;
static Point2F thumbPosition;

extern "C" Image* __wrap_Image_LoadBMPScaled(const char* name, unsigned int, unsigned int)
{
    return std::strcmp(name, "thumb") == 0 ? &thumb : &bar;
}

extern "C" void __wrap_Image_DisplayScaled(Image* image, const Area2F*,
                                            const Point2F* position, const Point2F*)
{
    if (image == &thumb) thumbPosition = *position;
}

static void check_position(unsigned int expected)
{
    unsigned int range = 0;
    unsigned int position = 0;
    scroll_info_get_range(0, &range, &position);
    CHECK_EQ(range, 4u);
    CHECK_EQ(position, expected);
}

int main(void)
{
    CHECK_EQ(scroll_info_round(1.25f), 1);
    CHECK_EQ(scroll_info_round(1.5f), 2);
    CHECK_EQ(scroll_info_round(-1.5f), -1);
    CHECK_EQ(scroll_info_round(-1.75f), -2);

    const Area2F increase = { 2.0f, 0.0f, 4.0f, 4.0f };
    const Area2F decrease = { 2.0f, 24.0f, 4.0f, 4.0f };
    const Area2F track = { 2.0f, 6.0f, 4.0f, 16.0f };
    constexpr int informationPanel = 7;
    thumb.height = 4;
    scroll_info_initialise("bar", 0, 0, informationPanel,
                           static_cast<const char*>(nullptr), 0u, 0, 0);
    scroll_info_set_xy_offset(0, 2, 4);
    scroll_info_add_structure(0, 0, &increase, nullptr, nullptr);
    scroll_info_add_structure(0, 1, &decrease, nullptr, nullptr);
    scroll_info_add_create_structure(0, &track, 5, 2, "thumb");
    check_position(2);
    scroll_info_set_range(0, 4, 2);
    indexed_pair_set(informationPanel, 20.0f, 30.0f);

    unsigned int active = 99;
    CHECK_EQ(scroll_info_update(&active, 28, 34, 0, 0), 0);
    CHECK_EQ(active, 99u);
    CHECK_EQ(scroll_info_update(&active, 24, 34, 1, 0), 1);
    check_position(2);
    CHECK_EQ(scroll_info_update(&active, 24, 34, 0, 1), 1);
    CHECK_EQ(active, 0u);
    check_position(3);
    scroll_info_draw(0);
    CHECK_EQ(thumbPosition.x, 24.0f);
    CHECK_EQ(thumbPosition.y, 49.0f);

    scroll_info_update(&active, 24, 58, 1, 0);
    scroll_info_update(&active, 24, 58, 0, 1);
    check_position(2);
    scroll_info_set_range(0, 4, 4);
    scroll_info_update(&active, 24, 34, 1, 0);
    scroll_info_update(&active, 24, 34, 0, 1);
    check_position(4);

    CHECK_EQ(scroll_info_update(&active, 24, 40, 0, 0), 1);
    CHECK_EQ(scroll_info_update(&active, 24, 50, 1, 0), 1);
    check_position(3); // Ten of sixteen track pixels is halfway from 2 to 3.
    return lrr_test_result();
}
