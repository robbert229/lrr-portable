// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_panel_unit — named controls, translated hit rectangles, rotation
 * bounds and the radar panel's movement and readiness. */
#include "game/panel_unit.h"
#include "test/lrr_test.h"

static Point2F drawnAt;

extern "C" void __wrap_Image_DisplayScaled(Image*, const Area2F*, const Point2F* pos,
                                           const Point2F*)
{
    drawnAt = *pos;
}

int main(void)
{
    ui_panel_name_tables_init();
    int panelType = -1;
    unsigned int buttonIndex = 99;
    CHECK_EQ(ui_panel_name_table_lookup("panel_RaDaR", &panelType), 1);
    CHECK_EQ(panelType, PANEL_RADAR);
    CHECK_EQ(ui_panel_indexed_name_lookup(PANEL_RADAR, "panelbutton_radar_zoomin",
                                         &buttonIndex), 1);
    CHECK_EQ(buttonIndex, 2u);
    CHECK_EQ(ui_panel_indexed_name_lookup(PANEL_RADAR, "PanelButton_InfoDock_Close",
                                         &buttonIndex), 0);
    CHECK_EQ(buttonIndex, 2u);

    PanelData panel = {};
    PanelButton button = {};
    panel.currPos = { 10.0f, 20.0f };
    button.x1 = 2.0f;
    button.y1 = 4.0f;
    button.x2 = 6.0f;
    button.y2 = 8.0f;
    CHECK_EQ(panel_button_point_inside(&panel, &button, 12, 24), 1);
    CHECK_EQ(panel_button_point_inside(&panel, &button, 15, 27), 1);
    CHECK_EQ(panel_button_point_inside(&panel, &button, 11, 24), 0);
    CHECK_EQ(panel_button_point_inside(&panel, &button, 16, 27), 0);
    CHECK_EQ(panel_button_point_inside(&panel, &button, 15, 28), 0);

    CHECK_EQ(point_distance_within_radius(2, 3, 5, 7, 5), 0);
    CHECK_EQ(point_distance_within_radius(2, 3, 5, 7, 6), 1);
    int x = 22;
    int y = 9;
    float_pair_to_integer_pair_x87(&x, &y, 2, 9, 5);
    CHECK_EQ(x, 6);
    CHECK_EQ(y, 9);

    panelGlobs.panelTable[PANEL_RADAR].flags = PANEL_FLAG_IN;
    panel_category_bounds_set(PANEL_RADAR, -8, 8, 0, 4);
    CHECK_EQ(request_record_ready_flag4_target(PANEL_RADAR), 1);
    panel_state_flags_toggle(PANEL_RADAR);
    CHECK_EQ(request_record_ready_flag4_target(PANEL_RADAR), 0);
    CHECK_EQ(request_record_ready_flag2_target(PANEL_RADAR), 0);
    CHECK(request_record_flag8_is_set_target(PANEL_RADAR), "the panel starts sliding out");
    simtick_manager_tick(PANEL_RADAR, 0.0f);
    CHECK_EQ(drawnAt.x, 0.0f);
    CHECK_EQ(drawnAt.y, 4.0f);
    simtick_manager_tick(PANEL_RADAR, 4.0f);
    CHECK_EQ(drawnAt.x, -4.0f);
    CHECK_EQ(drawnAt.y, 6.0f);
    CHECK_EQ(request_record_ready_flag2_target(PANEL_RADAR), 0);
    for (int i = 0; i < 3; ++i) simtick_manager_tick(PANEL_RADAR, 4.0f);
    CHECK_EQ(drawnAt.x, -8.0f);
    CHECK_EQ(drawnAt.y, 8.0f);
    CHECK_EQ(request_record_ready_flag2_target(PANEL_RADAR), 1);
    CHECK_EQ(request_record_flag8_is_set_target(PANEL_RADAR), 0);
    return lrr_test_result();
}
