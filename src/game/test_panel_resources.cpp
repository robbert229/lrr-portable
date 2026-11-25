// SPDX-License-Identifier: LGPL-3.0-or-later
/* Standalone panel resource and geometry contracts using solid bitmaps.
 * No game state or authored interface appearance is loaded. */
#include <cstdlib>
#include <vector>
#include "gods98/directdraw.h"
#include "gods98/images_link_names.h"
#include "gods98/images_unit.h"
#include "game/panel_unit.h"
#include "platform/native/test_fixture.h"
#include "test/lrr_test.h"
#include "test/synthetic_level_fixture.h"

namespace {
struct Draw {
    Image* image;
    Point2F at, size;
    bool scaled;
};
std::vector<Draw> draws;
}

extern "C" void __wrap_Image_DisplayScaled(Image* image, const Area2F*, const Point2F* pos,
                                           const Point2F* size)
{
    draws.push_back({image, *pos, size ? *size : Point2F{}, size != nullptr});
}

namespace {
using namespace synthetic_fixture;

bool dimensions(Image* image, unsigned w, unsigned h)
{
    if (!CHECK(image != nullptr, "loaded solid image")) return false;
    CHECK_EQ(image->width, w);
    CHECK_EQ(image->height, h);
    CHECK(image->surface != nullptr, "owned surface");
    return true;
}

void check_draw(Image* image, float x, float y)
{
    if (!CHECK_EQ(draws.size(), 1u)) return;
    CHECK_EQ(draws[0].image, image);
    CHECK_NEAR(draws[0].at.x, x, 1e-5);
    CHECK_NEAR(draws[0].at.y, y, 1e-5);
}

void rotation_config()
{
    const char config[] =
        "Sample { PanelRotationControl { CenterPositionX 19 CenterPositionY 27 Radius 11 "
        "LeftImage tile.bmp,3.5,7 RightImage strip.bmp,9,2 "
        "UpImage tile.bmp,4,8 DownImage strip.bmp,5,12 } }";
    auto* parsed = block_parse(config, sizeof(config)-1);
    if (!CHECK(parsed != nullptr, "parse independent resource config")) return;
    frontend_panel_rotation_control_config_load(parsed, "Sample");
    block_close(parsed);
    CHECK_EQ(panelGlobs.rotationCenterX, 19);
    CHECK_EQ(panelGlobs.rotationCenterY, 27);
    CHECK_EQ(panelGlobs.rotationRadius, 11);
    const Point2F positions[] = {{4,8},{5,12},{3.5f,7},{9,2}};
    for (unsigned i = 0; i < 4; ++i) {
        dimensions(panelGlobs.rotationImages[i], i%2 ? 9 : 13, i%2 ? 4 : 7);
        CHECK_EQ(panelGlobs.rotationImagePositions[i].x, positions[i].x);
        CHECK_EQ(panelGlobs.rotationImagePositions[i].y, positions[i].y);
        if (panelGlobs.rotationImages[i]) Image_Remove(panelGlobs.rotationImages[i]);
        panelGlobs.rotationImages[i] = nullptr;
    }
    // Omitting the optional images and positions restores the defaults.
    const char defaults[] = "Sample { PanelRotationControl { Radius 4 } }";
    parsed = block_parse(defaults, sizeof(defaults)-1);
    if (!CHECK(parsed != nullptr, "parse defaults")) return;
    frontend_panel_rotation_control_config_load(parsed, "Sample");
    block_close(parsed);
    CHECK_EQ(panelGlobs.rotationCenterX, 1);
    CHECK_EQ(panelGlobs.rotationCenterY, 1);
    CHECK_EQ(panelGlobs.rotationRadius, 4);
    for (auto* image : panelGlobs.rotationImages) CHECK_EQ(image, nullptr);
}

void panel_lifecycle()
{
    constexpr int category = PANEL_RADAR;
    auto& panel = panelGlobs.panelTable[category];
    legacy_overlay_image_record_load("tile.bmp", category, PANEL_FLAG_OUT);
    auto* background = static_cast<Image*>(panel.image);
    if (!dimensions(background, 13, 7)) return;
    CHECK_EQ(panel.flags, unsigned(PANEL_FLAG_OPEN | PANEL_FLAG_OUT));
    CHECK_EQ(panel.isFlic, 0);
    legacy_overlay_image_record_load("absent.bmp", PANEL_INFO_DOCK, PANEL_FLAG_IN);
    CHECK_EQ(panelGlobs.panelTable[PANEL_INFO_DOCK].image, nullptr);
    CHECK_EQ(panelGlobs.panelTable[PANEL_INFO_DOCK].flags & PANEL_FLAG_OPEN, 0u);
    panel_category_bounds_set(category, -11, 17, 5, 9);
    CHECK_EQ(panel.currPos.x, -11.0f);
    CHECK_EQ(panel.currPos.y, 17.0f);
    CHECK_EQ(legacy_panel_slot_coordinate_hit_test(category, -8, 19), 0);
    CHECK_EQ(legacy_panel_slot_coordinate_hit_test(category, 5, 19), 1);
    panel_state_flags_toggle(category);
    CHECK_EQ(request_record_ready_flag4_target(category), 0);
    draws.clear();
    simtick_manager_tick(category, 4);
    check_draw(background, -3, 13);
    CHECK(request_record_flag8_is_set_target(category), "still sliding");
    panel_state_flags_toggle(category); // reverse while in motion
    for (unsigned i = 0; i < 6; ++i) simtick_manager_tick(category, 4);
    CHECK_EQ(request_record_ready_flag2_target(category), 1);
    CHECK_EQ(panel.currPos.x, -11.0f);
    CHECK_EQ(panel.currPos.y, 17.0f);

    const unsigned types[] = {1, 0};
    const float areas[] = {2,3,7,5, 9,4,3,6};
    const int tips[] = {15, 29};
    char tile[] = "tile.bmp", strip[] = "strip.bmp", missing[] = "absent.bmp";
    char* normal[] = {tile, missing};
    char* highlight[] = {strip, nullptr};
    char* pressed[] = {tile, nullptr};
    panel_texture_slots_allocate(category, 2, types, areas, normal, highlight, pressed, tips);
    auto& active = panel.buttonList[1];
    auto& optional = panel.buttonList[0];
    CHECK_EQ(panel.buttonCount, 2u);
    CHECK_EQ(active.toolTip, 15);
    CHECK_EQ(optional.toolTip, 29);
    CHECK_EQ(optional.normalImage, nullptr);
    CHECK_EQ(optional.highlightImage, nullptr);
    CHECK_EQ(optional.pressedImage, nullptr);
    if (!dimensions(active.normalImage, 13, 7) ||
        !dimensions(active.highlightImage, 9, 4) ||
        !dimensions(active.pressedImage, 13, 7)) return;
    float bounds[4]{};
    ui_entry_bounds_get(category, 1, bounds);
    for (unsigned i = 0; i < 4; ++i) CHECK_EQ(bounds[i], areas[i]);
    indexed_pair_set(category, 7, 11);
    draws.clear();
    panel_texture_slots_blit_to(category);
    check_draw(active.normalImage, 9, 14);
    active.flags = PANEL_BUTTON_FLAG_HIGHLIGHT;
    draws.clear();
    panel_texture_slots_blit_to(category);
    check_draw(active.highlightImage, 9, 14);
    panel_item_flag8_set_if_enabled(category, 1, 1);
    CHECK_EQ(active.flags, unsigned(PANEL_BUTTON_FLAG_HIGHLIGHT));
    panel_item_flag32_set(category, 1, 1);
    panel_item_flag8_set_if_enabled(category, 1, 1);
    draws.clear();
    panel_texture_slots_blit_to(category);
    check_draw(active.pressedImage, 9, 14);
    panel_item_flag16_set(category, 1, 1);
    draws.clear();
    panel_texture_slots_blit_to(category);
    CHECK(draws.empty(), "hidden item emits no draw");
    panel_item_flag16_set(category, 1, 0);
    panel_item_flag8_set_if_enabled(category, 1, 0);
    CHECK_EQ(active.flags & PANEL_BUTTON_FLAG_TOGGLED, 0u);
    active.clickCount = 4;
    active.flags |= PANEL_BUTTON_FLAG_BANNER | PANEL_BUTTON_FLAG_TOGGLED;
    legacy_panel_category_interaction_state_reset();
    CHECK_EQ(panel.currPos.x, 5.0f);
    CHECK_EQ(panel.currPos.y, 9.0f);
    CHECK_EQ(request_record_ready_flag4_target(category), 1);
    CHECK_EQ(active.clickCount, 0);
    CHECK_EQ(active.flags & (PANEL_BUTTON_FLAG_BANNER | PANEL_BUTTON_FLAG_TOGGLED), 0u);
    CHECK(active.flags & PANEL_BUTTON_FLAG_TOGGLE, "reset retains toggle capability");
    std::free(panel.buttonList);
    panel.buttonList = nullptr;
    panel.buttonCount = 0;
}

void meter_resources()
{
    constexpr int category = PANEL_RADAR;
    three_surface_resources_load("tile.bmp", "strip.bmp", "absent.bmp");
    dimensions(panelGlobs.crystalSmallImage, 13, 7);
    dimensions(panelGlobs.crystalUsedImage, 9, 4);
    CHECK_EQ(panelGlobs.crystalNoSmallImage, nullptr);
    panel_resource_layout_store("strip.bmp", 2, 3, 40, "absent.bmp", 4, 5);
    if (!dimensions(panelGlobs.airMeterJuiceImage, 9, 4)) return;
    CHECK_EQ(panelGlobs.airMeterNoAirImage, nullptr);
    CHECK_EQ(panelGlobs.airMeterNoAirOffset.x, 4.0f);
    CHECK_EQ(panelGlobs.airMeterNoAirOffset.y, 5.0f);
    indexed_pair_set(category, 10, 20);
    draws.clear();
    advisor_progress_bar_fill_draw(category, 0);
    CHECK(draws.empty(), "empty meter not drawn");
    advisor_progress_bar_fill_draw(category, 25);
    check_draw(panelGlobs.airMeterJuiceImage, 12, 23);
    if (draws.size() == 1) {
        CHECK(draws[0].scaled, "scaled fill");
        CHECK_NEAR(draws[0].size.x, 10.0f, 1e-5);
        CHECK_EQ(draws[0].size.y, 4.0f);
    }
    draws.clear();
    advisor_progress_bar_fill_draw(category, 125);
    check_draw(panelGlobs.airMeterJuiceImage, 12, 23);
    if (draws.size() == 1) CHECK_NEAR(draws[0].size.x, 40.0f, 1e-5);
    panel_resource_handle_and_dimensions_store("tile.bmp", 17, 31, 20);
    if (!dimensions(panelGlobs.sideBarMeterImage, 13, 7)) return;
    draws.clear();
    panel_side_bar_draw();
    CHECK(draws.empty(), "empty meter not drawn");
    ui_overlay_animation_advance(1, 2);
    const float expected = 1.0f - 0.95f*0.95f;
    CHECK_NEAR(panelGlobs.sideBarMeterLevel, expected, 1e-6);
    panel_side_bar_draw();
    check_draw(panelGlobs.sideBarMeterImage, 17, 31-20*expected);
    if (draws.size() == 1) {
        CHECK_EQ(draws[0].size.x, 13.0f);
        CHECK_NEAR(draws[0].size.y, 20*expected, 1e-5);
    }
    ui_overlay_animation_advance(0, 3);
    CHECK_EQ(panelGlobs.sideBarMeterLevel, 0.0f);
    draws.clear();
    panel_side_bar_draw();
    CHECK(draws.empty(), "depleted meter not drawn");
}
}

int main()
{
    if (!CHECK(fixture_enter("panel_resource_fixture") && fixture_mkdir("Data") &&
               write_bmp("Data/tile.bmp", 13, 7) && write_bmp("Data/strip.bmp", 9, 4),
               "generate anonymous solid images") ||
        !initialise_headless("PanelResources")) return lrr_test_result();
    Image_Initialise();
    ui_panel_name_tables_init();
    rotation_config();
    panel_lifecycle();
    meter_resources();
    Image_Shutdown();
    panelGlobs = {};
    directdraw_shutdown();
    return lrr_test_result("panel_resources");
}
