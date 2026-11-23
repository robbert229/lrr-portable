// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_camera_control — range-gated zoom and free movement, and the
 * acceleration cap and accumulated camera translation. */
#include "game/camera_control.h"
#include "test/lrr_test.h"

extern "C" void camera_set_zoom_range(LegoCamera* camera, float minimum, float maximum);
extern "C" void camera_set_zoom(LegoCamera* camera, float zoom);
extern "C" void camera_enable_free_movement(LegoCamera* camera, int enabled);
extern "C" void camera_init_camera_movements(float speed, float dropOff,
                                             float acceleration, int scrollIndent);
extern "C" unsigned int camera_get_mouse_scroll_indent(void);
extern "C" void camera_move(LegoCamera* camera, Camera_Vector3F* direction, float elapsed);
extern "C" void camera_stop_movement(LegoCamera* camera);

int main(void)
{
    LegoCamera camera = {};
    camera_set_zoom_range(&camera, 8.0f, 24.0f);
    camera_set_zoom(&camera, 8.0f);
    CHECK_EQ(camera.zoom, 8.0f);
    camera_zoom_update(&camera, 16.0f);
    CHECK_EQ(camera.zoom, 24.0f);
    camera_zoom_update(&camera, 1.0f);
    CHECK_EQ(camera.zoom, 24.0f);
    camera_enable_free_movement(&camera, 1);
    camera_zoom_update(&camera, 1.0f);
    CHECK_EQ(camera.zoom, 25.0f);
    camera_enable_free_movement(&camera, 0);
    camera_set_zoom(&camera, 7.0f);
    CHECK_EQ(camera.zoom, 25.0f);
    camera_set_zoom(&camera, 12.0f);
    CHECK_EQ(camera.zoom, 12.0f);

    camera_init_camera_movements(4.0f, 0.5f, 2.0f, 3);
    CHECK_EQ(camera_get_mouse_scroll_indent(), 3u);
    Camera_Vector3F right = { 8.0f, 0.0f, 0.0f };
    camera_move(&camera, &right, 1.0f);
    CHECK_EQ(camera.moveSpeed, 2.0f);
    CHECK_NEAR(camera.moveVector.x, 2.0f, 1e-6);
    Camera_Vector3F up = { 0.0f, 2.0f, 0.0f };
    camera_move(&camera, &up, 2.0f);
    CHECK_EQ(camera.moveSpeed, 4.0f);
    CHECK_NEAR(camera.moveVector.x, 2.0f, 1e-6);
    CHECK_NEAR(camera.moveVector.y, 8.0f, 1e-6);
    CHECK_NEAR(camera.moveVector.z, 0.0f, 1e-6);
    camera_stop_movement(&camera);
    CHECK_EQ(camera.moveSpeed, 0.0f);
    return lrr_test_result("camera_control");
}
