// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_carried_facing — legoobject_attached_direction_vector_update
 * (LegoObject.c, 0x00440a10), which moves each carried object onto its
 * carrier's carry null and sets the object's facing to the direction it
 * moved.
 *
 * When the carrier stands still the object doesn't move, and the
 * original's (pos - old) / |pos - old| is 0/0: a NaN facing. A unit routing
 * with a NaN facing skips its turn on the spot, as a vehicle carried by a
 * LargeCat would. The port keeps the previous facing (#961). */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "game/legoobject_record.h"

extern "C" {
void legoobject_attached_direction_vector_update(Lego_Object* obj);
}

/* The test's stand-ins (--wrap, CMakeLists.txt): one container, the
 * carried object's, and the raider's carry null at s_null. Placing the
 * container on the null moves it there. */
static float s_container[3];
static float s_null[3];
static int s_null_token;
static int s_container_token;

extern "C" void* __wrap_renderer_cached_handle_48_get_or_create(void*)
{
    return &s_null_token;
}
extern "C" void __wrap_ddiscene_object_position_get(void*, void*, float* pos)
{
    pos[0] = s_container[0];
    pos[1] = s_container[1];
    pos[2] = s_container[2];
}
extern "C" void __wrap_ddiscene_object_position_set(void*, void* frame, float x, float y, float z)
{
    if (frame == &s_null_token) {
        s_container[0] = s_null[0] + x;
        s_container[1] = s_null[1] + y;
        s_container[2] = s_null[2] + z;
    }
}
extern "C" void __wrap_ddiscene_object_pose_method_dispatch(void*, void*, float, float, float,
                                                            float, float, float)
{
}

static int s_failures = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

static int near(float a, float b) { return fabsf(a - b) < 1e-5f; }

static int facing_is(const Lego_Object* obj, float x, float y, float z)
{
    return near(obj->faceDirection[0], x) && near(obj->faceDirection[1], y) &&
           near(obj->faceDirection[2], z);
}

/* A raider (type 2, on foot) carrying a power crystal (type 6, a plain
 * container object). */
static Lego_Object s_raider;
static Lego_Object s_crystal;

static void setup(float x, float y, float z)
{
    memset(&s_raider, 0, sizeof(s_raider));
    memset(&s_crystal, 0, sizeof(s_crystal));
    s_raider.type = 2;
    s_raider.numCarriedObjects = 1;
    s_raider.carriedObjects[0] = &s_crystal;
    s_raider.flags1 = 0x400;
    s_crystal.type = 6;
    s_crystal.flags3 = 0x20000;
    s_crystal.other = &s_container_token;
    s_crystal.carryingThisObject = &s_raider;
    s_crystal.faceDirection[0] = x;
    s_crystal.faceDirection[1] = y;
    s_crystal.faceDirection[2] = z;
}

static void place_container(float x, float y, float z)
{
    s_container[0] = x;
    s_container[1] = y;
    s_container[2] = z;
}

static void move_null(float x, float y, float z)
{
    s_null[0] = x;
    s_null[1] = y;
    s_null[2] = z;
}

/* The carrier walks: the facing is the direction the object moved, as in
 * the original. */
static void test_carrier_moving(void)
{
    setup(0.0f, 1.0f, 0.0f);
    place_container(10.0f, 20.0f, 0.0f);
    move_null(13.0f, 24.0f, 0.0f);
    legoobject_attached_direction_vector_update(&s_raider);
    CHECK(facing_is(&s_crystal, 0.6f, 0.8f, 0.0f), "moving: faces the way it moved");
    CHECK(near(s_crystal.faceDirectionLength, 5.0f), "moving: the length is the distance moved");
    CHECK(near(s_crystal.field_2a0, 10.0f), "moving: field_2a0 holds the old position");
}

/* The carrier stands still: the object is already on the null, so it
 * doesn't move. The facing stays what it was, not 0/0. */
static void test_carrier_standing_still(void)
{
    setup(0.0f, 1.0f, 0.0f);
    move_null(13.0f, 24.0f, 0.0f);
    place_container(13.0f, 24.0f, 0.0f);
    legoobject_attached_direction_vector_update(&s_raider);
    CHECK(isfinite(s_crystal.faceDirection[0]) && isfinite(s_crystal.faceDirection[1]) &&
              isfinite(s_crystal.faceDirection[2]),
          "still: the facing is not NaN");
    CHECK(facing_is(&s_crystal, 0.0f, 1.0f, 0.0f), "still: keeps the previous facing");
    CHECK(s_crystal.faceDirectionLength == 0.0f, "still: the length is 0");

    /* Many frames standing still change nothing. */
    for (int i = 0; i < 10; i++) legoobject_attached_direction_vector_update(&s_raider);
    CHECK(facing_is(&s_crystal, 0.0f, 1.0f, 0.0f), "still: keeps it over many frames");
}

/* The carrier walks, then stops: the object keeps the direction of its
 * last move, and the next move sets it again. */
static void test_carrier_stops_and_goes(void)
{
    setup(0.0f, 1.0f, 0.0f);
    place_container(0.0f, 0.0f, 0.0f);
    move_null(-2.0f, 0.0f, 0.0f);
    legoobject_attached_direction_vector_update(&s_raider);
    CHECK(facing_is(&s_crystal, -1.0f, 0.0f, 0.0f), "stops: faces its move");
    legoobject_attached_direction_vector_update(&s_raider);
    CHECK(facing_is(&s_crystal, -1.0f, 0.0f, 0.0f), "stops: keeps its last move");
    move_null(-2.0f, 3.0f, 0.0f);
    legoobject_attached_direction_vector_update(&s_raider);
    CHECK(facing_is(&s_crystal, 0.0f, 1.0f, 0.0f), "goes: faces its new move");
}

int main(void)
{
    test_carrier_moving();
    test_carrier_standing_still();
    test_carrier_stops_and_goes();
    if (s_failures != 0) {
        printf("%d check(s) failed\n", s_failures);
        return 1;
    }
    printf("carried_facing: all checks passed\n");
    return 0;
}
