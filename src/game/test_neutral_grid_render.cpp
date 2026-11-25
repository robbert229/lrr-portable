// SPDX-License-Identifier: LGPL-3.0-or-later
/* Generic grid colour classification and rendered geometry on the existing
 * abstract fixture. No imported image or original level is rendered. */
#include <cmath>
#include <cstdint>
#include <cstring>
#include <set>
#include <miniwin/ddraw.h>
#include "gods98/directdraw.h"
#include "gods98/draw.h"
#include "game/lego_unit.h"
#include "game/legoobject_unit.h"
#include "game/objective_unit.h"
#include "game/radarmap_link_names.h"
#include "game/radarmap.h"
#include "game/terrain_map.h"
#include "test/lrr_test.h"
#include "test/synthetic_initializer_fixture.h"
#include "game/lego_globs.h"

namespace {
Lego_Block& cell(unsigned x, unsigned y)
{
    auto* l = legoGlobs.currLevel;
    return l->blocks[y * l->width + x];
}
using PackedColour = uint32_t;

PackedColour colour(unsigned surface, uint32_t flags1, uint32_t flags2 = 0)
{
    auto& block = cell(2, 2);
    const auto saved = block;
    block.f_02 = static_cast<unsigned char>(surface);
    block.flags1 = flags1;
    block.flags2 = flags2;
    DrawRect draw{};
    CHECK_EQ(radarmap_get_block_colour(&draw, 2, 2), 1);
    block = saved;
    for (float channel : {draw.r, draw.g, draw.b})
        CHECK(std::isfinite(channel) && channel >= 0 && channel <= 1, "normalised colour");
    return (PackedColour(std::lround(draw.r * 255)) << 16) |
           (PackedColour(std::lround(draw.g * 255)) << 8) |
            PackedColour(std::lround(draw.b * 255));
}

void classify()
{
    constexpr uint32_t Wall = 4, Open = 8, Ground = 16, Reinforced = 0x40000;
    std::set<PackedColour> ground, wall, reinforced;
    for (unsigned surface = 1; surface <= 5; ++surface) {
        ground.insert(colour(surface, Ground));
        wall.insert(colour(surface, Wall));
        reinforced.insert(colour(surface, Ground | Reinforced));
    }
    CHECK_EQ(ground.size(), 5u);
    CHECK_EQ(wall.size(), 5u);
    CHECK_EQ(reinforced.size(), 5u);
    for (PackedColour c : ground) {
        CHECK_EQ(wall.count(c), 0u);
        CHECK_EQ(reinforced.count(c), 0u);
    }
    const PackedColour water = colour(7, Wall);
    const PackedColour liquid = colour(6, Wall);
    const PackedColour deep = colour(9, Wall);
    CHECK_EQ(colour(7, Open), water);
    CHECK_EQ(colour(6, Open), liquid);
    CHECK_EQ(colour(9, Open), deep);
    CHECK(water != liquid && liquid != deep && deep != water, "distinct terrain classifications");
    const PackedColour a = colour(8, Ground);
    const PackedColour b = colour(10, Ground);
    const PackedColour c = colour(11, Ground);
    CHECK_EQ(colour(8, Wall), a);
    CHECK_EQ(colour(10, Wall), b);
    CHECK_EQ(colour(11, Wall), c);
    CHECK(a != b && b != c && c != a, "distinct resource classifications");
    CHECK(colour(1, Wall | 0x20000) != colour(1, Wall), "concealed wall differs");
    const PackedColour normalFloor = colour(0, Open);
    const PackedColour workFloor = colour(0, Open | 0x100000);
    const PackedColour unpoweredPath = colour(0, Open | 0x20000000);
    const PackedColour poweredPath = colour(0, Open | 0x20000000, 0x100);
    CHECK_EQ((std::set<PackedColour>{normalFloor, workFloor, unpoweredPath, poweredPath}.size()), 4u);
    DrawRect ignored{};
    const auto saved = cell(2, 2);
    cell(2, 2).flags1 = 0;
    CHECK_EQ(radarmap_get_block_colour(&ignored, 2, 2), 0);
    cell(2, 2) = saved;
}

bool pixel_region_changed(int x, int y, int width, int height)
{
    auto* back = directDrawGlobs.bSurf;
    if (!CHECK(back != nullptr, "back surface available")) return false;
    DDSURFACEDESC2 desc{};
    desc.dwSize = sizeof(desc);
    if (!CHECK_EQ(back->Lock(nullptr, &desc, DDLOCK_WAIT, nullptr), DD_OK)) return false;
    const unsigned bytes = desc.ddpfPixelFormat.dwRGBBitCount / 8;
    const auto rgb = desc.ddpfPixelFormat.dwRBitMask | desc.ddpfPixelFormat.dwGBitMask |
                     desc.ddpfPixelFormat.dwBBitMask;
    bool changed = false;
    if (CHECK(desc.lpSurface && bytes >= 2 && bytes <= 4 && rgb != 0 &&
              desc.lPitch >= static_cast<LONG>((x + width) * bytes) &&
              y + height <= static_cast<int>(desc.dwHeight),
              "supported software pixel layout")) {
        const auto* pixels = static_cast<const uint8_t*>(desc.lpSurface);
        for (int row = y; row < y + height; ++row)
            for (int col = x; col < x + width; ++col) {
                uint32_t value = 0;
                std::memcpy(&value, pixels + row * desc.lPitch + col * bytes, bytes);
                changed |= (value & rgb) != 0;
            }
    }
    CHECK_EQ(back->Unlock(static_cast<LPRECT>(nullptr)), DD_OK);
    return changed;
}

void draw()
{
    // The software renderer's pixel-format state is established by its
    // first BeginFrame, not simply by DirectDraw device creation.
    objective_stop_showing_objective();
    if (!CHECK(lego_engine_tick(1), "initial rendered frame")) return;
    auto* level = legoGlobs.currLevel;
    Point2F centre{};
    terrain_map_block_to_world(level->map, 2, 3, &centre.x, &centre.y);
    Area2F bounds{24, 32, 80, 80};
    auto* map = radarmap_create(level->map, &bounds, 20.0f);
    if (!CHECK(map != nullptr, "neutral grid projection allocated")) return;
    auto* figure = legoobject_create_in_world(legoGlobs.miniFigureData, 2, 0, 0,
                                               centre.x, centre.y, 0);
    // Use an existing neutral model and no game artwork.
    if (!CHECK(figure != nullptr, "neutral object created")) { radarmap_free(map); return; }
    Area2F clip{7, 9, 240, 170};
    viewport_subrectangle_set(&clip);
    directdraw_clear(nullptr, 0);
    const auto saved = cell(2, 3);
    cell(2, 3).flags1 |= 0x20;
    const auto savedView = legoGlobs.viewMode;
    legoGlobs.viewMode = 0;
    radarmap_draw(map, &centre);
    CHECK_NEAR(map->viewLeft, centre.x - 2 * level->blockSize, 0.001f);
    CHECK_NEAR(map->viewTop, centre.y - 2 * level->blockSize, 0.001f);
    CHECK_NEAR(map->viewRight, centre.x + 2 * level->blockSize, 0.001f);
    CHECK_NEAR(map->viewBottom, centre.y + 2 * level->blockSize, 0.001f);
    CHECK(map->drawRectCount >= 1u, "visible object rectangles submitted");
    const unsigned visibleBefore = map->drawRectCount;
    auto* outside = legoobject_create_in_world(legoGlobs.miniFigureData, 2, 0, 0,
                                                 centre.x + level->blockSize * 3,
                                                 centre.y + level->blockSize, 0);
    CHECK(outside != nullptr, "second object outside clipped area");
    radarmap_draw(map, &centre);
    CHECK_EQ(map->drawRectCount, visibleBefore);
    CHECK(pixel_region_changed(24, 32, 80, 80), "geometry rendered inside projected region");
    CHECK(!pixel_region_changed(0, 0, 16, 16), "pixels outside the projected region untouched");
    Area2F restored{};
    viewport_rectangle_get(&restored);
    CHECK_EQ(restored.x, clip.x);
    CHECK_EQ(restored.y, clip.y);
    CHECK_EQ(restored.width, clip.width);
    CHECK_EQ(restored.height, clip.height);

    // Click round trip and hit testing at the neutral model's centre.
    int bx = -1, by = -1;
    Point2F world{};
    CHECK_EQ(radarmap_screen_to_block_pos(map, 64, 72, &world, &bx, &by), 1);
    CHECK_EQ(bx, 2);
    CHECK_EQ(by, 3);
    Lego_Object* hit = nullptr;
    CHECK_EQ(radarmap_try_select_object(map, 64, 72, &hit, &world), 1);
    CHECK_EQ(hit, figure);
    CHECK_EQ(radarmap_try_select_object(map, 104, 72, &hit, nullptr), 0);
    CHECK_EQ(radarmap_try_select_object(map, 70, 72, &hit, nullptr), 0);
    cell(2, 3).flags2 |= 4u;
    Point2F selectedPosition{};
    CHECK_EQ(radarmap_try_select_object(map, 70, 72, &hit, &selectedPosition), 1);
    CHECK(hit != nullptr, "expanded occupied-cell search found an object");
    CHECK(std::fabs(selectedPosition.x - (centre.x + 12)) <= level->blockSize &&
          std::fabs(selectedPosition.y - centre.y) <= level->blockSize, "hit within expanded radius");
    cell(2, 3).flags2 = saved.flags2;
    radarmap_draw_survey_dot_circle(map, &centre, level->blockSize, 0.5f);
    viewport_rectangle_get(&restored);
    CHECK_EQ(restored.x, clip.x);
    CHECK_EQ(restored.y, clip.y);

    legoGlobs.viewMode = savedView;
    if (savedView == 1) radarmap_draw(map, &centre); // camera projection overlay with actual viewport
    viewport_rectangle_get(&restored);
    CHECK_EQ(restored.x, clip.x);
    CHECK_EQ(restored.y, clip.y);
    cell(2, 3) = saved;
    radarmap_free(map);
    viewport_subrectangle_set(nullptr);
}
}

int main()
{
    const char stats[] = " Figure { Levels 1 CrossLand TRUE CollRadius 3 NumOfToolsCanCarry 1 } ";
    if (!synthetic_fixture::initialise_full_game("neutral_grid_render", "NeutralGridRender", stats))
        return lrr_test_result();
    classify();
    draw();
    lego_engine_shutdown();
    CHECK_EQ(legoGlobs.currLevel, nullptr);
    directdraw_shutdown();
    return lrr_test_result("neutral_grid_render");
}
