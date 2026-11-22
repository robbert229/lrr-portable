// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_tooltip — tooltip formatting, hover delay and cancellation, and
 * clamping the box and laying out icons in rows near a screen edge. */
#include "game/tooltip_link_names.h"
#include "game/tooltip.h"
#include "gods98/font_record.h"
#include "test/lrr_test.h"

#include <cstdarg>
#include <vector>

struct IconDraw {
    Image* image;
    Point2F position;
};

static std::vector<IconDraw> icons;
static Area2F box;
static char printed[64];
static unsigned int boxesDrawn;

extern "C" void __wrap_surface_fill_pixel_spans(Area2F* areas, uint32_t count,
                                               float, float, float, int)
{
    if (count != 0) box = areas[0];
    boxesDrawn += count;
}

extern "C" void __wrap_surface_draw_polyline(Point2F*, Point2F*, uint32_t,
                                             float, float, float, int) {}

extern "C" void __wrap_font_print_f(const TextureSurface*, int, int, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(printed, sizeof(printed), fmt, args);
    va_end(args);
}

extern "C" void __wrap_Image_DisplayScaled(Image* image, const Area2F*, const Point2F* pos,
                                            const Point2F*)
{
    icons.push_back({ image, *pos });
}

int main(void)
{
    Font font = {};
    font.fontHeight = 4;
    for (auto& column : font.posSet) {
        for (Area2F& glyph : column) glyph.width = 2.0f;
    }
    tooltip_initialise(reinterpret_cast<TextureSurface*>(&font), 1, 1, 0.5f,
                       100, 80, 3, 0.5f, 0.25f, 0.75f);
    int type = -1;
    CHECK_EQ(tooltip_get_type("tooltip_priority", &type), 1);
    CHECK_EQ(type, 7);
    CHECK_EQ(tooltip_get_type("unknown", &type), 0);
    CHECK_EQ(type, 7);

    Image wide = {}, tall = {}, nextRow = {};
    wide.width = 8;
    wide.height = 3;
    tall.width = 6;
    tall.height = 5;
    nextRow.width = 5;
    nextRow.height = 2;
    tooltip_set_content(type, "Ore_%d\\nOK", 4);
    tooltip_add_icon(type, &wide);
    tooltip_add_icon(type, &tall);
    tooltip_add_icon(type, nullptr);
    tooltip_add_icon(type, &nextRow);

    tooltip_activate(type);
    tooltip_update(95, 75, 6.0f);
    CHECK_EQ(boxesDrawn, 0u);
    tooltip_update(95, 75, 1.0f); // Missing activation cancels the hover.
    tooltip_activate(type);
    tooltip_update(95, 75, 6.5f);
    tooltip_activate(type);
    tooltip_update(95, 75, 6.0f);
    CHECK_EQ(boxesDrawn, 0u);
    tooltip_activate(type);
    tooltip_update(95, 75, 1.0f);
    CHECK_EQ(boxesDrawn, 1u);
    CHECK_STREQ(printed, "Ore 4\nOK");
    CHECK_EQ(box.x, 82.0f);
    CHECK_EQ(box.y, 55.0f);
    CHECK_EQ(box.width, 18.0f);
    CHECK_EQ(box.height, 19.0f);
    if (CHECK_EQ(icons.size(), 3u)) {
        CHECK_EQ(icons[0].image, &wide);
        CHECK_EQ(icons[0].position.x, 84.0f);
        CHECK_EQ(icons[0].position.y, 65.0f);
        CHECK_EQ(icons[1].image, &tall);
        CHECK_EQ(icons[1].position.x, 92.0f);
        CHECK_EQ(icons[1].position.y, 65.0f);
        CHECK_EQ(icons[2].image, &nextRow);
        CHECK_EQ(icons[2].position.x, 84.0f);
        CHECK_EQ(icons[2].position.y, 70.0f);
    }
    tooltip_update(95, 75, 1.0f);
    CHECK_EQ(boxesDrawn, 1u);
    return lrr_test_result();
}
