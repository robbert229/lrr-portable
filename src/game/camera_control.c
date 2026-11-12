// SPDX-License-Identifier: LGPL-3.0-or-later
/* Original translation unit GAME-022: the game camera (LegoCamera), text
 * 00435b20-00436c20, .rdata 0049e378-0049e39c (finding 505, finding 587).
 *
 * Rebuilt in the original's shape (ADR 0005). The original compiled this
 * object without optimisation: every function keeps its frame pointer,
 * reloads each operand from memory, and calls Maths.h's inline vector
 * functions out of line (the copies at 004013a0-00401750). The functions
 * are packed with int3 fill (finding 505 rule 5). The matching build
 * compiles this file with /Od (CMakeLists.txt, LRR_UNOPTIMISED_UNITS).
 *
 * The port's camera (ViewContext, the engine's camera controller buffer and
 * their hooks) stays in the port tail, camera_control_port.cpp, under the
 * port's names. In the matching build, lego_unit.c and panel_unit.c call
 * this unit under the port's names through the #defines in
 * lego_unit_imports.h and camera_control.h. */
#define LEGORRR_GAME_CAMERA_CONTROL_UNIT
#include <math.h>

#include "camera_control.h"
#include "camera_control_imports.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Camera_Vector3F {
    float x;
    float y;
    float z;
} Camera_Vector3F;

/* The prefix of Map3D (terrain_map.h's TerrainMap). */
typedef struct Camera_Map {
    unsigned int gridWidth;   /* +0x00 */
    unsigned int gridHeight;  /* +0x04 */
} Camera_Map;

/* The prefix of Lego_Level (lego_unit.h). */
typedef struct Camera_Level {
    char* name;               /* +0x00 */
    Camera_Map* map;          /* +0x04 */
} Camera_Level;

/* LegoCamera_Type. */
#define CAMERA_TYPE_TOPDOWN 1
#define CAMERA_TYPE_RADAR   2
#define CAMERA_TYPE_FP      3

/* LegoCamera_Flags. */
#define CAMERA_FLAG_TILTRANGE     0x01
#define CAMERA_FLAG_ROTATIONRANGE 0x02
#define CAMERA_FLAG_ZOOMRANGE     0x04
#define CAMERA_FLAG_FREEMOVEMENT  0x08
#define CAMERA_FLAG_FPSETUP       0x10

/* LegoCamera (0xbc bytes, Mem_Alloc'd by Camera_Create). */
typedef struct LegoCamera {
    int type;                         /* +0x00 LegoCamera_Type */
    struct EntitySlot* trackObj;      /* +0x04 tracked or first-person object */
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

/* Container_Create (00472950). The port types its argument as the parent's
 * scene-object descriptor, which is the parent container (lego_port.cpp's
 * lego_container_create_adapter); scene_pool.hpp gives it C linkage. */
void* ddiscene_object_allocate(void* parent);

LegoCamera* camera_create(void* root, int camType);
void camera_enable_free_movement(LegoCamera* cam, int on);
void camera_free(LegoCamera* cam);
void camera_init_camera_movements(float cameraSpeed, float cameraDropOff,
                                  float cameraAcceleration,
                                  int mouseScrollIndent);
void camera_copy_fp_position_orientation(LegoCamera* cam,
                                         LegoCamera* otherCam);
void camera_track_object(LegoCamera* cam, struct EntitySlot* obj,
                         float trackSpeed, float trackZoom, float tilt,
                         float trackRotationSpeed);
void camera_set_fp_object(LegoCamera* cam, struct EntitySlot* obj,
                          int cameraFrame);
int camera_get_fp_camera_frame(LegoCamera* cam);
void camera_stop_movement(LegoCamera* cam);
unsigned int camera_get_mouse_scroll_indent(void);
void camera_shake(LegoCamera* cam, float intensity, float duration);
void camera_update(LegoCamera* cam, Camera_Level* level, float elapsedAbs,
                   float elapsedInter);
void camera_set_tilt_range(LegoCamera* cam, float minTilt, float maxTilt);
void camera_set_tilt(LegoCamera* cam, float tilt);
void camera_add_tilt(LegoCamera* cam, float angle);
void camera_set_rotation_range(LegoCamera* cam, float minRotation,
                               float maxRotation);
void camera_set_rotation(LegoCamera* cam, float rotation);
void camera_add_rotation(LegoCamera* cam, float angle);
void camera_set_zoom_range(LegoCamera* cam, float minDist, float maxDist);
void camera_set_zoom(LegoCamera* cam, float zoom);
void camera_add_zoom(LegoCamera* cam, float delta);
void camera_add_translation_2d(LegoCamera* cam, float translateX,
                               float translateY);
void camera_get_topdown_position(LegoCamera* cam, Camera_Vector3F* pos);
void camera_set_topdown_position(LegoCamera* cam, float x, float y);
int camera_get_topdown_world_pos(LegoCamera* cam, struct TerrainMap* map,
                                 Camera_Vector3F* worldPos);
float camera_get_rotation(LegoCamera* cam);
void camera_move(LegoCamera* cam, Camera_Vector3F* dir, float elapsed);

/* Camera_InitCameraMovements' four settings (Lego.cfg CameraSpeed,
 * CameraDropOff, CameraAcceleration and MouseScrollIndent). */
typedef struct Camera_Globs {
    float cameraSpeed;                /* 005560a0 */
    float cameraDropOff;              /* 005560a4 */
    float cameraAcceleration;         /* 005560a8 */
    unsigned int mouseScrollIndent;   /* 005560ac */
} Camera_Globs;

/* cameraGlobs: a common symbol in the original (not in the unit's .bss
 * range). */
// GLOBAL: LEGORR 0x005560a0
Camera_Globs cameraGlobs;

// FUNCTION: LEGORR 0x00435b20
LegoCamera* camera_create(void* root, int camType)
{
    LegoCamera* cam;

    if ((cam = (LegoCamera*)engine_memory_allocate(sizeof(LegoCamera))) != 0) {
        cam->type = camType;
        cam->trackObj = 0;
        cam->trackRadarZoomSpeed = cam->trackRadarZoom = 0.0f;
        cam->flags = 0;
        cam->tilt = cam->rotation = 0.0f;
        cam->zoom = 200.0f;
        cam->trackFPCameraFrame = 0;
        cam->moveVector.x = 0.0f;
        cam->moveVector.y = 0.0f;
        cam->moveVector.z = 0.0f;
        cam->moveSpeed = 0.0f;
        cam->shakeIntensity = cam->shakeDuration = cam->shakeTimer = 0.0f;
        cam->shakeVector.x = cam->shakeVector.y = cam->shakeVector.z = 0.0f;

        if ((cam->contCam = ddiscene_object_allocate(0)) != 0) {
            ddiscene_object_position_set(cam->contCam, 0, 0.0f, 0.0f, -cam->zoom);
            ddiscene_object_pose_method_dispatch(cam->contCam, 0, 0.0f, 0.0f, 1.0f,
                                                 0.0f, 1.0f, 0.0f);
            if (camType != CAMERA_TYPE_FP) {
                if ((cam->contRoot = ddiscene_object_allocate(root)) != 0) {
                    if ((cam->contListener = ddiscene_object_allocate(root)) != 0) {
                        if ((cam->contTilt = ddiscene_object_allocate(cam->contRoot)) != 0) {
                            if (camType == CAMERA_TYPE_TOPDOWN) {
                                if ((cam->contEar = ddiscene_object_allocate(cam->contTilt)) != 0) {
                                    ddiscene_object_position_set(cam->contEar, 0, 0.0f, 0.0f, -80.0f);
                                }
                            }
                            ddiscene_scene_object_attach_or_unlink(cam->contCam, cam->contTilt);
                            return cam;
                        }
                        ledger_stub_00472a40_DDiSceneObjectDestroy(cam->contListener);
                    }
                    ledger_stub_00472a40_DDiSceneObjectDestroy(cam->contRoot);
                }
                ledger_stub_00472a40_DDiSceneObjectDestroy(cam->contCam);
            } else {
                ddiscene_scene_object_attach_or_unlink(cam->contCam, root);
                return cam;
            }
        }
        engine_memory_free(cam);
    }
    return 0;
}

// FUNCTION: LEGORR 0x00435d91
void camera_enable_free_movement(LegoCamera* cam, int on)
{
    if (on) cam->flags |= CAMERA_FLAG_FREEMOVEMENT;
    else cam->flags &= ~CAMERA_FLAG_FREEMOVEMENT;
}

// FUNCTION: LEGORR 0x00435dc8
void camera_free(LegoCamera* cam)
{
    ledger_stub_00472a40_DDiSceneObjectDestroy(cam->contCam);
    if (cam->type != CAMERA_TYPE_FP) {
        ledger_stub_00472a40_DDiSceneObjectDestroy(cam->contRoot);
        ledger_stub_00472a40_DDiSceneObjectDestroy(cam->contTilt);
    }
    engine_memory_free(cam);
}

// FUNCTION: LEGORR 0x00435e0e
void camera_init_camera_movements(float cameraSpeed, float cameraDropOff,
                                  float cameraAcceleration,
                                  int mouseScrollIndent)
{
    cameraGlobs.cameraSpeed = cameraSpeed;
    cameraGlobs.cameraDropOff = cameraDropOff;
    cameraGlobs.cameraAcceleration = cameraAcceleration;
    cameraGlobs.mouseScrollIndent = mouseScrollIndent;
}

// FUNCTION: LEGORR 0x00435e35
void camera_copy_fp_position_orientation(LegoCamera* cam,
                                         LegoCamera* otherCam)
{
    Camera_Vector3F pos, dir, up;

    if (cam->type == CAMERA_TYPE_FP) {
        ddiscene_object_position_get(otherCam->contCam, 0, (float*)&pos);
        ddiscene_object_facing_pair_get(otherCam->contCam, 0, (float*)&dir,
                                        (float*)&up);
        ddiscene_object_position_set(cam->contCam, 0, pos.x, pos.y, pos.z);
        ddiscene_object_pose_method_dispatch(cam->contCam, 0, dir.x, dir.y,
                                             dir.z, up.x, up.y, up.z);
    }
}

// FUNCTION: LEGORR 0x00435ebb
void camera_track_object(LegoCamera* cam, struct EntitySlot* obj,
                         float trackSpeed, float trackZoom, float tilt,
                         float trackRotationSpeed)
{
    cam->trackObj = obj;
    cam->trackRadarZoomSpeed = trackSpeed;
    cam->trackRadarZoom = trackZoom;
    camera_set_tilt(cam, tilt);
    cam->trackRadarRotationSpeed = trackRotationSpeed;
}

// FUNCTION: LEGORR 0x00435ef4
void camera_set_fp_object(LegoCamera* cam, struct EntitySlot* obj,
                          int cameraFrame)
{
    cam->trackObj = obj;
    cam->trackFPCameraFrame = cameraFrame;
}

// FUNCTION: LEGORR 0x00435f0b
int camera_get_fp_camera_frame(LegoCamera* cam)
{
    return cam->trackFPCameraFrame;
}

// FUNCTION: LEGORR 0x00435f16
void camera_stop_movement(LegoCamera* cam)
{
    cam->moveSpeed = 0.0f;
}

// FUNCTION: LEGORR 0x00435f28
unsigned int camera_get_mouse_scroll_indent(void)
{
    return cameraGlobs.mouseScrollIndent;
}

// FUNCTION: LEGORR 0x00435f32
void camera_shake(LegoCamera* cam, float intensity, float duration)
{
    cam->shakeIntensity = intensity;
    cam->shakeDuration = duration;
    cam->shakeTimer = 0.0f;
}

// FUNCTION: LEGORR 0x00435f5c
void camera_update(LegoCamera* cam, Camera_Level* level, float elapsedAbs,
                   float elapsedInter)
{
    scene_object_camera_teleport_or_offset(cam->contCam, 1,
                                           -cam->shakeVector.x,
                                           -cam->shakeVector.y,
                                           -cam->shakeVector.z);

    if (cam->type == CAMERA_TYPE_TOPDOWN) {
        Camera_Vector3F pos, dir, up;
        int by, bx;

        vec_scale((float*)&cam->moveVector, (float*)&cam->moveVector,
                  cameraGlobs.cameraDropOff);
        scene_object_camera_teleport_or_offset(cam->contRoot, 2,
                                               cam->moveVector.x,
                                               cam->moveVector.y,
                                               cam->moveVector.z);
        ddiscene_object_position_get(cam->contRoot, 0, (float*)&pos);
        ddiscene_object_facing_pair_get(cam->contRoot, 0, (float*)&dir,
                                        (float*)&up);
        if (terrain_map_world_to_block((struct TerrainMap*)level->map, pos.x,
                                       pos.y, &bx, &by)) {
            float camDist, height;

            if (bx < cameraGlobs.mouseScrollIndent ||
                bx >= level->map->gridWidth - cameraGlobs.mouseScrollIndent) {
                scene_object_camera_teleport_or_offset(cam->contRoot, 2,
                                                       -cam->moveVector.x,
                                                       0.0f, 0.0f);
            }
            if (by < cameraGlobs.mouseScrollIndent ||
                by >= level->map->gridHeight - cameraGlobs.mouseScrollIndent) {
                scene_object_camera_teleport_or_offset(cam->contRoot, 2, 0.0f,
                                                       -cam->moveVector.y,
                                                       0.0f);
            }
            /* Deviation (scoring only): the comma keeps MSVC from folding
             * the call's "add esp, 0xc" into the float argument's push, as
             * the original's code does not (finding 587). */
            ddiscene_object_position_set(cam->contListener, 0, pos.x, pos.y,
                                         (0, terrain_map_world_z_get(
                                             (struct TerrainMap*)level->map,
                                             pos.x, pos.y)));
            ddiscene_object_pose_method_dispatch(cam->contListener, 0, up.x,
                                                 up.y, 0.0f, 0.0f, 0.0f,
                                                 -1.0f);
            height = -terrain_map_world_z_unmodified_get(CAMERA_LEGO_GET_MAP(),
                                                         pos.x, pos.y);
            ddiscene_object_position_set(cam->contCam, cam->contTilt, 0.0f,
                                         0.0f, -(camDist = cam->zoom + height));
        } else {
            scene_object_camera_teleport_or_offset(cam->contRoot, 2,
                                                   -cam->moveVector.x,
                                                   -cam->moveVector.y,
                                                   -cam->moveVector.z);
        }
    } else if (cam->type == CAMERA_TYPE_FP) {
        if (cam->trackObj != 0 && elapsedAbs != 0.0f) {
            Camera_Vector3F destPos, destDir;
            float dot;
            Camera_Vector3F oldPos, oldDir;
            float dist, t;

            ddiscene_object_position_get(cam->contCam, 0, (float*)&oldPos);
            ddiscene_object_facing_pair_get(cam->contCam, 0, (float*)&oldDir,
                                            (float*)&destDir);
            entity_follow_target_pose_get(cam->trackObj,
                                          cam->trackFPCameraFrame,
                                          (float*)&destPos, (float*)&destDir);
            if (!(cam->flags & CAMERA_FLAG_FPSETUP) ||
                (dist = vec_dist((float*)&destPos, (float*)&oldPos)) > 100.0f ||
                (dot = vec_dot((float*)&destDir, (float*)&oldDir)) < 0.0f) {
                oldPos = destPos;
                oldDir = destDir;
            }
            t = 1.0f / elapsedAbs * 4.0f;
            vec_normalize((float*)&oldDir);
            vec_normalize((float*)&destDir);
            vec_scale((float*)&oldDir, (float*)&oldDir, t);
            vec_add((float*)&destDir, (float*)&destDir, (float*)&oldDir);
            vec_scale((float*)&oldPos, (float*)&oldPos, t);
            vec_add((float*)&destPos, (float*)&destPos, (float*)&oldPos);
            vec_scale((float*)&destPos, (float*)&destPos, 1.0f / (1.0f + t));
            if (dist > 0.01) {
                ddiscene_object_position_set(cam->contCam, 0, destPos.x,
                                             destPos.y, destPos.z);
            }
            if (dot < 0.99999) {
                ddiscene_object_pose_method_dispatch(cam->contCam, 0,
                                                     destDir.x, destDir.y,
                                                     destDir.z, 0.0f, 0.0f,
                                                     -1.0f);
            }
            cam->flags |= CAMERA_FLAG_FPSETUP;
        }
    } else if (cam->type == CAMERA_TYPE_RADAR) {
        if (cam->trackObj != 0) {
            void* cont;

            if ((cont = entity_type_component_get(cam->trackObj)) != 0) {
                /* Declared in their frame order: MSVC 5.0 lays out /Od
                 * locals by a hash of their names (finding 587). */
                float totalDist, startZoom;
                Camera_Vector3F camPos2, move, movement;
                float zdiff;
                Camera_Vector3F rootPos, objDirection, camPos;
                float rootDist, factor, distance, maxMove;
                Camera_Vector3F objPos;

                maxMove = cam->trackRadarZoomSpeed * elapsedInter;
                lego_flags1_bit17_set(0);
                ddiscene_object_position_get(cam->contCam, 0, (float*)&camPos);
                ddiscene_object_position_get(cont, 0, (float*)&objPos);
                objPos.z -= cam->trackRadarZoom / 10.0f;
                startZoom = cam->zoom;
                cam->zoom = cam->trackRadarZoom;
                ddiscene_object_position_set(cam->contCam, cam->contTilt, 0.0f,
                                             0.0f, -cam->zoom);
                ddiscene_object_position_get(cam->contRoot, 0,
                                             (float*)&rootPos);
                ddiscene_object_position_set(cam->contRoot, 0, objPos.x,
                                             objPos.y, objPos.z);
                ddiscene_object_position_get(cam->contCam, 0,
                                             (float*)&camPos2);
                vec_sub((float*)&movement, (float*)&camPos2, (float*)&camPos);
                if ((distance = vec_len((float*)&movement)) > maxMove && distance < 120.0f) {
                    factor = distance / maxMove;
                    ddiscene_object_facing_pair_get(cont, 0, (float*)&objDirection,
                                                    0);
                    vec_scale((float*)&move, (float*)&objDirection, factor);
                    vec_add((float*)&objPos, (float*)&objPos, (float*)&move);
                    vec_sub((float*)&movement, (float*)&objPos, (float*)&rootPos);
                    rootDist = vec_len((float*)&movement);
                    zdiff = (float)fabs(cam->trackRadarZoom - startZoom);
                    totalDist = rootDist + zdiff;
                    vec_normalize((float*)&movement);
                    vec_scale((float*)&movement, (float*)&movement,
                              rootDist / totalDist * maxMove);
                    vec_add((float*)&rootPos, (float*)&rootPos, (float*)&movement);
                    ddiscene_object_position_set(cam->contRoot, 0, rootPos.x,
                                                 rootPos.y, rootPos.z);
                    if (startZoom > cam->trackRadarZoom) {
                        cam->zoom = startZoom - zdiff / totalDist * maxMove;
                    } else if (startZoom != cam->trackRadarZoom) {
                        cam->zoom = startZoom + zdiff / totalDist * maxMove;
                    }
                    ddiscene_object_position_set(cam->contCam, cam->contTilt,
                                                 0.0f, 0.0f, -cam->zoom);
                }
                camera_add_rotation(cam, cam->trackRadarRotationSpeed *
                                             elapsedAbs);
            }
        } else {
            lego_flags1_bit17_set(1);
        }
    }

    if (cam->shakeTimer < cam->shakeDuration) {
        float t, r;

        t = (cam->shakeDuration - cam->shakeTimer) / cam->shakeDuration *
            cam->shakeIntensity;
        r = random_float_range(0.0f, 1.0f);
        d3drm_vector_random((float*)&cam->shakeVector);
        vec_scale((float*)&cam->shakeVector, (float*)&cam->shakeVector, t * r);
        cam->shakeTimer += elapsedInter;
        scene_object_camera_teleport_or_offset(cam->contCam, 1,
                                               -cam->shakeVector.x,
                                               -cam->shakeVector.y,
                                               -cam->shakeVector.z);
    } else {
        cam->shakeVector.x = cam->shakeVector.y = cam->shakeVector.z = 0.0f;
    }
}

// FUNCTION: LEGORR 0x00436831
void camera_set_tilt_range(LegoCamera* cam, float minTilt, float maxTilt)
{
    cam->flags |= CAMERA_FLAG_TILTRANGE;
    cam->tiltMin = minTilt;
    cam->tiltMax = maxTilt;
}

// FUNCTION: LEGORR 0x00436860
void camera_set_tilt(LegoCamera* cam, float tilt)
{
    if (!(cam->flags & CAMERA_FLAG_TILTRANGE) ||
        (cam->flags & CAMERA_FLAG_FREEMOVEMENT) ||
        (tilt >= cam->tiltMin && tilt <= cam->tiltMax)) {
        cam->tilt = tilt;
        ddiscene_object_pose_method_dispatch(cam->contTilt, cam->contRoot,
                                             0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
                                             0.0f);
        render_object_rotate_axis(cam->contTilt, 1, 1.0f, 0.0f, 0.0f, -tilt);
    }
}

// FUNCTION: LEGORR 0x00436900
void camera_add_tilt(LegoCamera* cam, float angle)
{
    angle += cam->tilt;
    camera_set_tilt(cam, angle);
}

// FUNCTION: LEGORR 0x00436921
void camera_set_rotation_range(LegoCamera* cam, float minRotation,
                               float maxRotation)
{
    cam->flags |= CAMERA_FLAG_ROTATIONRANGE;
    cam->rotationMin = minRotation;
    cam->rotationMax = maxRotation;
}

// FUNCTION: LEGORR 0x00436953
void camera_set_rotation(LegoCamera* cam, float rotation)
{
    if (!(cam->flags & CAMERA_FLAG_ROTATIONRANGE) ||
        (cam->flags & CAMERA_FLAG_FREEMOVEMENT) ||
        (rotation >= cam->rotationMin && rotation <= cam->rotationMax)) {
        cam->rotation = rotation;
        ddiscene_object_pose_method_dispatch(cam->contRoot, 0, 0.0f, 0.0f,
                                             1.0f, 0.0f, 1.0f, 0.0f);
        render_object_rotate_axis(cam->contRoot, 1, 0.0f, 0.0f, 1.0f,
                                  -rotation);
    }
}

// FUNCTION: LEGORR 0x004369f4
void camera_add_rotation(LegoCamera* cam, float angle)
{
    angle += cam->rotation;
    camera_set_rotation(cam, angle);
}

// FUNCTION: LEGORR 0x00436a18
void camera_set_zoom_range(LegoCamera* cam, float minDist, float maxDist)
{
    cam->flags |= CAMERA_FLAG_ZOOMRANGE;
    cam->zoomMin = minDist;
    cam->zoomMax = maxDist;
}

// FUNCTION: LEGORR 0x00436a4a
void camera_set_zoom(LegoCamera* cam, float zoom)
{
    if (!(cam->flags & CAMERA_FLAG_ZOOMRANGE) ||
        (cam->flags & CAMERA_FLAG_FREEMOVEMENT) ||
        (zoom >= cam->zoomMin && zoom <= cam->zoomMax)) {
        cam->zoom = zoom;
    }
}

// FUNCTION: LEGORR 0x00436aa5
void camera_add_zoom(LegoCamera* cam, float delta)
{
    delta += cam->zoom;
    camera_set_zoom(cam, delta);
}

// FUNCTION: LEGORR 0x00436ac9
void camera_add_translation_2d(LegoCamera* cam, float translateX,
                               float translateY)
{
    scene_object_camera_teleport_or_offset(cam->contRoot, 1, translateX,
                                           translateY, 0.0f);
}

// FUNCTION: LEGORR 0x00436ae9
void camera_get_topdown_position(LegoCamera* cam, Camera_Vector3F* pos)
{
    if (cam->type == CAMERA_TYPE_TOPDOWN) {
        ddiscene_object_position_get(cam->contRoot, 0, (float*)pos);
    }
}

// FUNCTION: LEGORR 0x00436b0b
void camera_set_topdown_position(LegoCamera* cam, float x, float y)
{
    if (cam->type == CAMERA_TYPE_TOPDOWN) {
        ddiscene_object_position_set(cam->contRoot, 0, x, y, 0.0f);
    }
}

// FUNCTION: LEGORR 0x00436b33
int camera_get_topdown_world_pos(LegoCamera* cam, struct TerrainMap* map,
                                 Camera_Vector3F* worldPos)
{
    if (cam->type == CAMERA_TYPE_TOPDOWN) {
        ddiscene_object_position_get(cam->contRoot, 0, (float*)worldPos);
        worldPos->z = terrain_map_world_z_get(map, worldPos->x, worldPos->y);
    }
    return 1;
}

// FUNCTION: LEGORR 0x00436b79
float camera_get_rotation(LegoCamera* cam)
{
    return cam->rotation;
}

// FUNCTION: LEGORR 0x00436b87
void camera_move(LegoCamera* cam, Camera_Vector3F* dir, float elapsed)
{
    vec_normalize((float*)dir);
    cam->moveSpeed += elapsed * cameraGlobs.cameraAcceleration;
    if (cam->moveSpeed > cameraGlobs.cameraSpeed) {
        cam->moveSpeed = cameraGlobs.cameraSpeed;
    }
    vec_scale((float*)dir, (float*)dir, cam->moveSpeed * elapsed);
    vec_add((float*)&cam->moveVector, (float*)&cam->moveVector, (float*)dir);
}

#ifdef __cplusplus
}
#endif
