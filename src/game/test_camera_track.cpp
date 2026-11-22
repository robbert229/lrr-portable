// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_camera_track — camera_control.c's radar (tracking) camera,
 * camera_update (0x00435f5c), moves its root towards the tracked object by
 * at most maxMove = trackRadarZoomSpeed * elapsedInter a frame.
 *
 * When the camera is already on its target and the game is paused (the
 * objective briefing at level start), elapsedInter is 0, so maxMove is 0
 * and float rounding leaves the camera a hair off its target. The move's
 * factor = distance / maxMove is then infinite and the root's position
 * went NaN; miniwin's SetPosition keeps a NaN matrix NaN, so the root
 * stayed NaN for the level. The move is also 0 / 0 when rootDist and zdiff
 * are both 0. The port skips the move in both cases.
 *
 * The test links with --wrap for the scene calls camera_update makes, so
 * the unit moves the fake frames below: a root, a tilt under it and the
 * camera under that. Each holds only a translation, which is all the radar
 * camera's move reads. */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "game/camera_control.h"

extern "C" void camera_update(LegoCamera* cam, void* level, float elapsedAbs,
                              float elapsedInter);

struct Frame {
    Frame* parent;
    float pos[3];
};

static Frame s_root, s_tilt, s_cam;
/* The tracked object's container, a frame of its own, facing +y. */
static Frame s_target;
static float s_target_facing[3] = { 0.0f, 1.0f, 0.0f };

/* A frame's world position: its translation and those of its parents. */
static void world(const Frame* f, float* out)
{
    out[0] = out[1] = out[2] = 0.0f;
    for (; f != NULL; f = f->parent) {
        out[0] += f->pos[0];
        out[1] += f->pos[1];
        out[2] += f->pos[2];
    }
}

extern "C" void __wrap_ddiscene_object_position_get(void* object, void*, float* pos)
{
    world((const Frame*)object, pos);
}
extern "C" void __wrap_ddiscene_object_position_set(void* object, void*, float x, float y, float z)
{
    Frame* f = (Frame*)object;
    f->pos[0] = x;
    f->pos[1] = y;
    f->pos[2] = z;
}
extern "C" void __wrap_ddiscene_object_facing_pair_get(void*, void*, float* first, float* second)
{
    if (first != NULL) memcpy(first, s_target_facing, sizeof(s_target_facing));
    if (second != NULL) second[0] = second[1] = second[2] = 0.0f;
}
extern "C" void* __wrap_legoobject_type_component_get(const void*) { return &s_target; }
extern "C" void __wrap_lego_flags1_bit17_set(int) {}
extern "C" void __wrap_scene_object_camera_teleport_or_offset(void*, unsigned int, float, float, float) {}
extern "C" void __wrap_ddiscene_object_pose_method_dispatch(void*, void*, float, float, float, float, float, float) {}
extern "C" void __wrap_render_object_rotate_axis(void*, unsigned int, float, float, float, float) {}

static int s_failures = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

static int finite3(const float* v) { return isfinite(v[0]) && isfinite(v[1]) && isfinite(v[2]); }

/* A radar camera tracking s_target at (10, 20, 5), zoom 40, whose root
 * stands at rootPos. The camera's aim point is the target lowered by
 * zoom / 10, so (10, 20, 1). */
static LegoCamera make_camera(const float* rootPos, float speed)
{
    LegoCamera cam;

    memset(&cam, 0, sizeof(cam));
    memset(&s_root, 0, sizeof(s_root));
    memset(&s_tilt, 0, sizeof(s_tilt));
    memset(&s_cam, 0, sizeof(s_cam));
    memset(&s_target, 0, sizeof(s_target));
    s_tilt.parent = &s_root;
    s_cam.parent = &s_tilt;
    s_target.pos[0] = 10.0f;
    s_target.pos[1] = 20.0f;
    s_target.pos[2] = 5.0f;
    memcpy(s_root.pos, rootPos, sizeof(s_root.pos));
    s_cam.pos[2] = -40.0f;

    cam.type = 2; /* CAMERA_TYPE_RADAR */
    cam.trackObj = (struct Lego_Object*)&s_target;
    cam.trackRadarZoomSpeed = speed;
    cam.trackRadarZoom = 40.0f;
    cam.zoom = 40.0f;
    cam.contRoot = &s_root;
    cam.contCam = &s_cam;
    cam.contTilt = &s_tilt;
    return cam;
}

static int at(const float* v, float x, float y, float z)
{
    return fabsf(v[0] - x) < 1e-3f && fabsf(v[1] - y) < 1e-3f && fabsf(v[2] - z) < 1e-3f;
}

int main(void)
{
    float pos[3];

    /* Paused (maxMove 0), the camera a hair off its target: the move is
     * skipped and the camera stays on the target. */
    {
        const float start[3] = { 10.001f, 20.0f, 1.0f };
        LegoCamera cam = make_camera(start, 2.0f);

        camera_update(&cam, NULL, 0.0f, 0.0f);
        world(&s_root, pos);
        CHECK(finite3(pos), "a paused camera near its target keeps a finite root");
        CHECK(at(pos, 10.0f, 20.0f, 1.0f), "a paused camera near its target stands on the target");
        CHECK(isfinite(cam.zoom) && cam.zoom == 40.0f, "a paused camera keeps its zoom");
        world(&s_cam, pos);
        CHECK(finite3(pos), "a paused camera near its target keeps a finite camera position");
    }

    /* The target faces diagonally, so no component of aim point + facing *
     * factor is 0 * infinity: they are all infinite, and so is totalDist,
     * which a "totalDist > 0" test alone would let through. */
    {
        const float start[3] = { 10.001f, 20.0f, 1.0f };
        LegoCamera cam = make_camera(start, 2.0f);

        s_target_facing[0] = 0.6f;
        s_target_facing[1] = 0.6f;
        s_target_facing[2] = 0.5f;
        camera_update(&cam, NULL, 0.0f, 0.0f);
        s_target_facing[0] = 0.0f;
        s_target_facing[1] = 1.0f;
        s_target_facing[2] = 0.0f;
        world(&s_root, pos);
        CHECK(finite3(pos), "a paused camera keeps a finite root whichever way its target faces");
        CHECK(at(pos, 10.0f, 20.0f, 1.0f), "a paused camera stands on the target whichever way it faces");
    }

    /* The same, repeated: the root does not drift into NaN over frames. */
    {
        const float start[3] = { 10.0f, 20.0f, 1.0f };
        LegoCamera cam = make_camera(start, 2.0f);
        int i;

        for (i = 0; i < 10; ++i) camera_update(&cam, NULL, 0.0f, 0.0f);
        world(&s_root, pos);
        CHECK(at(pos, 10.0f, 20.0f, 1.0f), "a camera on its target stays there over paused frames");
    }

    /* totalDist 0 with maxMove 1: the aim point plus its facing times
     * distance / maxMove (5) is the root, so rootDist and zdiff are both
     * 0 and rootDist / totalDist is 0 / 0. */
    {
        const float start[3] = { 10.0f, 25.0f, 1.0f };
        LegoCamera cam = make_camera(start, 1.0f);

        camera_update(&cam, NULL, 1.0f, 1.0f);
        world(&s_root, pos);
        CHECK(finite3(pos), "a camera whose move is 0 / 0 keeps a finite root");
        CHECK(isfinite(cam.zoom), "a camera whose move is 0 / 0 keeps a finite zoom");
        CHECK(at(pos, 10.0f, 20.0f, 1.0f), "a camera whose move is 0 / 0 stands on the target");
    }

    /* An ordinary move is unchanged: 5 units from the target at 2 units a
     * frame, the root moves 2 units along (-5, 2.5, 0). */
    {
        const float start[3] = { 15.0f, 20.0f, 1.0f };
        LegoCamera cam = make_camera(start, 2.0f);

        camera_update(&cam, NULL, 1.0f, 1.0f);
        world(&s_root, pos);
        CHECK(at(pos, 13.2111f, 20.8944f, 1.0f), "a camera away from its target moves by maxMove");
    }

    /* Further than 120 units away, the camera is set on the target. */
    {
        const float start[3] = { 500.0f, 20.0f, 1.0f };
        LegoCamera cam = make_camera(start, 2.0f);

        camera_update(&cam, NULL, 1.0f, 1.0f);
        world(&s_root, pos);
        CHECK(at(pos, 10.0f, 20.0f, 1.0f), "a camera far from its target is set on it");
    }

    if (s_failures == 0) printf("camera_track: ok\n");
    return s_failures != 0;
}
