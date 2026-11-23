// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_objinfo — load a synthetic health bar configuration and capture
 * the filled rectangles and border geometry sent to the drawing layer. */
#include "game/legoobject_record.h"
#include "game/objinfo_link_names.h"
#include "game/objinfo.h"
#include "gods98/config.h"
#include "gods98/draw.h"
#include "test/lrr_test.h"

#include <vector>

struct Fill {
    Area2F rect;
    float red;
    float green;
};

struct Border {
    Point2F from;
    Point2F to;
    float red;
};

static bool canShowDamage = true;
static std::vector<Fill> fills;
static std::vector<Border> borders;

extern "C" int __wrap_entity_quad_mesh_render_record_allowed(Lego_Object*)
{
    return canShowDamage;
}

extern "C" void __wrap_surface_fill_pixel_spans(Area2F* rects, uint32_t count,
                                               float r, float g, float, int)
{
    for (uint32_t i = 0; i < count; ++i) fills.push_back({ rects[i], r, g });
}

extern "C" void __wrap_surface_draw_polyline(Point2F* from, Point2F* to,
                                             uint32_t count, float r, float, float, int)
{
    for (uint32_t i = 0; i < count; ++i) borders.push_back({ from[i], to[i], r });
}

int main(void)
{
    struct ConfigEntry {
        char key[28];
        char value[16];
        unsigned int depth;
    } entries[] = {
        { "LegoRR", "{", 0 },
        { "ObjInfo", "{", 1 },
        { "HealthBarPosition", "-2:3", 2 },
        { "HealthBarWidthHeight", "6:2", 2 },
        { "HealthBarBorderSize", "1", 2 },
        { "HealthBarBorderRGB", "255:0:0", 2 },
        { "HealthBarBackgroundRGB", "255:0:0", 2 },
        { "HealthBarRGB", "0:255:0", 2 },
        { "HealthBarVertical", "TRUE", 2 },
    };
    constexpr size_t count = sizeof(entries) / sizeof(entries[0]);
    BlockRec config[count] = {};
    for (size_t i = 0; i < count; ++i) {
        config[i].key = entries[i].key;
        config[i].value = entries[i].value;
        config[i].depth = entries[i].depth;
        if (i != 0) config[i].prev = &config[i - 1];
        if (i + 1 < count) config[i].next = &config[i + 1];
    }

    CHECK_EQ(objinfo_load_health_bar(config, "Missing"), 0);
    CHECK_EQ(objinfo_load_health_bar(config, "LegoRR"), 1);

    Lego_Object raider = {};
    raider.health = 50.0f;
    canShowDamage = false;
    objinfo_draw_health_bar(&raider, 10, 20);
    CHECK(fills.empty() && borders.empty(), "hidden objects have no health overlay");

    canShowDamage = true;
    objinfo_draw_health_bar(&raider, 10, 20);
    if (CHECK_EQ(fills.size(), 2u)) {
        CHECK_EQ(fills[0].rect.x, 8.0f);
        CHECK_EQ(fills[0].rect.y, 23.0f);
        CHECK_EQ(fills[0].rect.width, 8.0f);
        CHECK_EQ(fills[0].rect.height, 2.0f);
        CHECK_NEAR(fills[0].red, 1.0f, 1e-6);
        CHECK_EQ(fills[1].rect.y, 25.0f);
        CHECK_EQ(fills[1].rect.height, 2.0f);
        CHECK_NEAR(fills[1].green, 1.0f, 1e-6);
    }
    if (CHECK_EQ(borders.size(), 4u)) {
        CHECK_EQ(borders[0].from.x, 8.0f);
        CHECK_EQ(borders[0].from.y, 23.0f);
        CHECK_EQ(borders[0].to.x, 8.0f);
        CHECK_EQ(borders[0].to.y, 25.0f);
        CHECK_EQ(borders[0].red, 1.0f);
        CHECK_NEAR(borders[2].red, 0.6f, 1e-6);
    }

    fills.clear();
    raider.health = 150.0f;
    objinfo_draw_health_bar(&raider, 10, 20);
    if (CHECK_EQ(fills.size(), 2u)) {
        CHECK_EQ(fills[0].rect.height, 0.0f);
        CHECK_EQ(fills[1].rect.height, 4.0f);
    }
    return lrr_test_result();
}
