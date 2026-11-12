// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_animation_object — the GAME-004 unit (game/animation_object.c, issue
 * #513): the record layout, the cached null lookups that need no container,
 * the container accessor, and a load whose `.ae` file is missing.
 *
 * The functions that reach a container go through gods98 code that needs a
 * scene, so they are covered by the matching build, not here. */
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "game/animation_object.h"

static int s_failures = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

static void test_layout(void)
{
    /* The original's offsets hold where pointers are 32-bit. */
    if (sizeof(void*) == 4) {
        CHECK(sizeof(AnimationObject) == 0x74, "record is 0x74 bytes");
        CHECK(offsetof(AnimationObject, container) == 0x04, "container +0x04");
        CHECK(offsetof(AnimationObject, cameraNullName) == 0x20,
              "camera null name +0x20");
        CHECK(offsetof(AnimationObject, drillNull) == 0x40, "drill null +0x40");
        CHECK(offsetof(AnimationObject, cameraNulls) == 0x54,
              "camera nulls +0x54");
        CHECK(offsetof(AnimationObject, polyMedium) == 0x64,
              "medium meshes +0x64");
        CHECK(offsetof(AnimationObject, flags) == 0x70, "flags +0x70");
    }
}

static void test_cached_nulls(void)
{
    AnimationObject object;
    int marker[6];
    memset(&object, 0, sizeof(object));

    /* No names: nothing to look up. */
    CHECK(activity_runner_state_read(&object, 1) == NULL,
          "no camera null name gives NULL");
    CHECK(renderer_cached_handle_40_get_or_create(&object) == NULL,
          "no drill null name gives NULL");
    CHECK(renderer_cached_handle_48_get_or_create(&object) == NULL,
          "no carry null name gives NULL");
    CHECK(renderer_cached_handle_50_get_or_create(&object) == NULL,
          "no deposit null name gives NULL");
    CHECK(renderer_cached_handle_4c_is_not_one(&object) == 0,
          "no throw null reads as scale 1");

    /* A cached null is returned without a lookup. */
    object.cameraNullName = (char*)"Camera";
    object.cameraNulls[1] = &marker[0];
    object.drillNull = &marker[1];
    object.carryNullName = (char*)"Carry";
    object.carryNull = &marker[2];
    object.depositNullName = (char*)"Deposit";
    object.depositNull = &marker[3];
    CHECK(activity_runner_state_read(&object, 1) == &marker[0],
          "cached camera null");
    CHECK(renderer_cached_handle_40_get_or_create(&object) == &marker[1],
          "cached drill null, even without a name");
    CHECK(renderer_cached_handle_48_get_or_create(&object) == &marker[2],
          "cached carry null");
    CHECK(renderer_cached_handle_50_get_or_create(&object) == &marker[3],
          "cached deposit null");

    /* The carry and deposit lookups test the name first. */
    object.carryNullName = NULL;
    object.depositNullName = NULL;
    CHECK(renderer_cached_handle_48_get_or_create(&object) == NULL,
          "carry null needs its name");
    CHECK(renderer_cached_handle_50_get_or_create(&object) == NULL,
          "deposit null needs its name");

    object.container = &marker[4];
    CHECK(animation_object_handle_get(&object) == &marker[4],
          "handle is the container");
}

static void test_load_missing(void)
{
    AnimationObject object;
    memset(&object, 0, sizeof(object));
    CHECK(entity_type_config_resource_row_load(&object, 7, NULL,
                                               "NoSuchDir\\NoSuchObject",
                                               (void*)"Game", 0) == 0,
          "a missing .ae file fails the load");
    CHECK(object.container == NULL && object.flags == 0,
          "a failed load leaves the record alone");
}

int main(void)
{
    test_layout();
    test_cached_nulls();
    test_load_missing();
    if (s_failures != 0) {
        printf("%d failure(s)\n", s_failures);
        return 1;
    }
    printf("animation_object: OK\n");
    return 0;
}
