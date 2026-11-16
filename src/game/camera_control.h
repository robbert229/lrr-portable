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

#if !defined(LEGORRR_GAME_CAMERA_CONTROL_UNIT)
#define camera_pan_axis_update camera_add_tilt
#define camera_tilt_axis_update camera_add_rotation
#define camera_zoom_update camera_add_zoom
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct Lego_Object;

typedef struct Camera_Vector3F {
    float x;
    float y;
    float z;
} Camera_Vector3F;

/* LegoCamera (0xbc bytes on Win32, Mem_Alloc'd by Camera_Create): legoGlobs'
 * cameraMain, cameraTrack and cameraFP. The +0x... comments are the Win32
 * offsets; the pointer fields hold real pointers (ADR 0007 decision 4). */
typedef struct LegoCamera {
    int type;                         /* +0x00 LegoCamera_Type */
    struct Lego_Object* trackObj;      /* +0x04 tracked or first-person object */
    float trackRadarZoomSpeed;        /* +0x08 */
    float trackRadarZoom;             /* +0x0c */
    float trackRadarRotationSpeed;    /* +0x10 */
    int trackFPCameraFrame;           /* +0x14 */
    Camera_Vector3F moveVector;       /* +0x18 */
    void* contCam;                    /* +0x24 the camera container */
    void* contRoot;                   /* +0x28 position and rotation (z) */
    void* contListener;               /* +0x2c the sound listener */
    void* contTilt;                   /* +0x30 tilt (x), child of contRoot */
    void* contEar;                    /* +0x34 top-down only */
    unsigned char unused_38[0x40];    /* +0x38 */
    float tilt;                       /* +0x78 */
    float tiltMin;                    /* +0x7c */
    float tiltMax;                    /* +0x80 */
    float rotation;                   /* +0x84 */
    float rotationMin;                /* +0x88 */
    float rotationMax;                /* +0x8c */
    float zoom;                       /* +0x90 */
    float zoomMin;                    /* +0x94 */
    float zoomMax;                    /* +0x98 */
    float moveSpeed;                  /* +0x9c */
    float shakeIntensity;             /* +0xa0 */
    float shakeDuration;              /* +0xa4 */
    float shakeTimer;                 /* +0xa8 */
    Camera_Vector3F shakeVector;      /* +0xac */
    unsigned char flags;              /* +0xb8 LegoCamera_Flags */
} LegoCamera;

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
