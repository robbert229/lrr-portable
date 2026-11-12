// SPDX-License-Identifier: LGPL-3.0-or-later
/* The camera controls (original unit GAME-022, game/camera_control.cpp)
 * that units rebuilt as C call, and the primary camera they act on.
 *
 * C boundary header (ADR 0005). The C++ declarations (gods98/view.hpp,
 * gods98/view_for_panel_unit.hpp) carry extern "C";
 * src/game/test_c_boundaries.cpp includes them with this header. Shared C++
 * headers do not include this file (finding 506). */
#ifndef LEGORRR_GAME_CAMERA_CONTROL_H
#define LEGORRR_GAME_CAMERA_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

/* DAT_005560e4: legoGlobs.cameraMain, defined in gods98/view.cpp. */
extern void* s_zoom_step_context;

/* FUN_00436900. */
void camera_pan_axis_update(void* camera, float delta);
/* FUN_004369f4 CameraTiltAxisUpdate. */
void camera_tilt_axis_update(void* camera, float delta);
/* FUN_00436aa5. */
void camera_zoom_update(void* camera, float delta);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_CAMERA_CONTROL_H */
