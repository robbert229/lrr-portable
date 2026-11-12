// SPDX-License-Identifier: LGPL-3.0-or-later
/* The camera controls (original unit GAME-022, game/camera_control.c) that
 * units rebuilt as C call, and the primary camera they act on.
 *
 * C boundary header (ADR 0005). The C++ declarations (gods98/view.hpp,
 * gods98/view_for_panel_unit.hpp) carry extern "C";
 * src/game/test_c_boundaries.cpp includes them with this header. Shared C++
 * headers do not include this file (finding 506).
 *
 * The declarations keep the port's names, which the port tail
 * (camera_control_port.cpp) defines for native callers. In the matching
 * build the #defines below point them at the unit's original functions
 * (finding 587); a #define adds no declaration, so the includers' symbol
 * numbering does not move (finding 538). camera_control.c declares its own
 * functions under their own names. */
#ifndef LEGORRR_GAME_CAMERA_CONTROL_H
#define LEGORRR_GAME_CAMERA_CONTROL_H

#if defined(_MSC_VER) && _MSC_VER < 1200 && \
    !defined(LEGORRR_GAME_CAMERA_CONTROL_UNIT)
#define camera_pan_axis_update camera_add_tilt
#define camera_tilt_axis_update camera_add_rotation
#define camera_zoom_update camera_add_zoom
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* DAT_005560e4: legoGlobs.cameraMain, defined in gods98/view.cpp. */
extern void* s_zoom_step_context;

/* FUN_00436900 Camera_AddTilt. */
void camera_pan_axis_update(void* camera, float delta);
/* FUN_004369f4 Camera_AddRotation. */
void camera_tilt_axis_update(void* camera, float delta);
/* FUN_00436aa5 Camera_AddZoom. */
void camera_zoom_update(void* camera, float delta);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_CAMERA_CONTROL_H */
