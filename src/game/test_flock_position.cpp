// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_flock_position — LegoObject.c's flock update
 * (LegoObject_Flocks_Update, 0044bc30) places the flock on its object's
 * container only while the flock has no position yet
 * (SteeringGroup.hasPosition). The original read that flag through
 * 004897d0, a one-line getter the linker folded with AnimClone_IsLws; on
 * 64-bit that getter's field sits where a SteeringGroup keeps hasTarget,
 * so the port reads hasPosition itself. The test drives 0044bc30 on flocks
 * whose two flags differ.
 *
 * The test links with --wrap for the Stats.c and Container.c getters that
 * 0044bc30 calls, so the unit reaches the stubs below instead of an
 * object's stats and a real container. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/legoobject_record.h"
#include "game/steering.h"

extern "C" void legoobject_motion_to_scene_node_sync(Lego_Object* obj, float elapsed);

/* The object's flock stats: a flock (0x200000) that stays shown
 * (0x400000), never attacks, and does not move (speed 0). */
extern "C" unsigned int __wrap_entity_stats_word_get(const Lego_Object*)
{
    return 0x200000 | 0x400000;
}
extern "C" float __wrap_entity_stats_float_at120(const Lego_Object*) { return 0.0f; }
extern "C" float __wrap_entity_stats_float_at124(const Lego_Object*) { return 0.0f; }
extern "C" float __wrap_entity_stats_float_at128(const Lego_Object*) { return 0.0f; }
extern "C" float __wrap_entity_stats_float_at12c(const Lego_Object*) { return 0.0f; }
extern "C" float __wrap_entity_stats_float_at130(const Lego_Object*) { return 0.0f; }
extern "C" float __wrap_entity_stats_float_at134(const Lego_Object*) { return 0.0f; }
extern "C" float __wrap_entity_stats_float_at138(const Lego_Object*) { return 1.0e9f; }

/* The object's container sits at (1, 2, 3). */
extern "C" void __wrap_ddiscene_object_position_get(void*, void*, float* pos)
{
    pos[0] = 1.0f;
    pos[1] = 2.0f;
    pos[2] = 3.0f;
}

static int s_failures = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

static int at(const SteeringVector3& v, float x, float y, float z)
{
    return fabsf(v.x - x) < 1e-6f && fabsf(v.y - y) < 1e-6f && fabsf(v.z - z) < 1e-6f;
}

/* One update of a flock built from `position` and `target`; returns the
 * leader's position afterwards. */
static SteeringVector3 update_flock(const SteeringVector3* position,
                                    const SteeringVector3* target, int* hasPosition)
{
    static Lego_Object obj;
    SteeringGroup* flock;
    SteeringVector3 result;

    memset(&obj, 0, sizeof(obj));
    flock = callback_state_record_create(position, target, 0.0f, 0.0f, 0.0f, NULL);
    obj.field_32c = flock;
    legoobject_motion_to_scene_node_sync(&obj, 0.0f);
    result = flock->leader->position;
    *hasPosition = flock->hasPosition;
    callback_state_list_dispose(flock);
    return result;
}

int main(void)
{
    const SteeringVector3 there = { 5.0f, 6.0f, 7.0f };
    SteeringVector3 pos;
    int hasPosition;

    /* No position but a target: the flock is placed on the container,
     * (x, -z, y). On 64-bit the folded getter read hasTarget (1) here and
     * left the flock where it was. */
    pos = update_flock(NULL, &there, &hasPosition);
    CHECK(at(pos, 1.0f, -3.0f, 2.0f), "a flock without a position is placed on its container");
    CHECK(hasPosition == 1, "placing the flock gives it a position");

    /* A position but no target: the flock stays where it is. */
    pos = update_flock(&there, NULL, &hasPosition);
    CHECK(at(pos, 5.0f, 6.0f, 7.0f), "a flock with a position is not moved to its container");

    /* Neither, as LegoObject_Flocks_Initialise creates them. */
    pos = update_flock(NULL, NULL, &hasPosition);
    CHECK(at(pos, 1.0f, -3.0f, 2.0f), "a new flock is placed on its container");

    if (s_failures == 0) printf("flock_position: ok\n");
    return s_failures != 0;
}
