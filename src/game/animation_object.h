// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-004 (original translation unit GAME-004, finding 505): animation
 * objects. An object is a 0x74-byte record that owns one loaded activity
 * container (`.ae` file), the names of its null frames from the `.ae`
 * config, a cache of the nulls it has looked up, and three level-of-detail
 * mesh lists.
 *
 * C-compatible header (ADR 0005). animation_object.c is compiled as C in the
 * matching build and as C++ in the native port. Shared C++ headers do not
 * include this file: every include and declaration a header adds shifts
 * MSVC's symbol numbering (finding 506). src/game/test_c_boundaries.cpp
 * includes it beside the C++ headers that still declare some of these
 * functions.
 *
 * The record is in animation_object_record.h.
 *
 * Seven functions keep the port's parameter types because C++ callers that
 * are not converted yet call them through the port's declarations:
 * entity_type_config_resource_row_load,
 * renderer_object_timing_value_adjust_by_count,
 * entity_payload_release_subslots, animation_object_visibility_set,
 * animation_object_handle_get and the renderer_cached_handle_40/48 lookups
 * (finding 549). */
#ifndef LEGORRR_GAME_ANIMATION_OBJECT_H
#define LEGORRR_GAME_ANIMATION_OBJECT_H

#include "gods98/model.h"
#include "animation_object_record.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The port's entity record (game/game.hpp); the unit only passes it. */
typedef struct Lego_Object EntitySlot;

/* The height callback of render_node_position_from_callback. */
typedef float (*AnimationObjectHeightFn)(float x, float y, void* data);

/* FUN_00406990: load the object (`destination`) with its container and
 * the `.ae` settings for the game name `configName` from `filename` (a path
 * without extension) under `root`. The object and the name are typed
 * `void*`. */
int entity_type_config_resource_row_load(void* destination,
                                         unsigned int objectId, void* root,
                                         const char* filename,
                                         void* configName);
/* FUN_00406c00, FUN_00406c30, FUN_00406c60: swap the medium, high and
 * first-person meshes onto the container (or back when `swap` is 0). */
void renderer_object_update_from_record_field64(AnimationObject* object,
                                                int swap);
void renderer_object_update_from_record_field68(AnimationObject* object,
                                                int swap);
void renderer_object_update_from_record_field6c(AnimationObject* object,
                                                int swap,
                                                unsigned int camera);
/* FUN_00406c90: hide or show the container. */
void animation_object_visibility_set(const void* object, unsigned int hide);
/* FUN_00406cb0: copy `source` into `destination` with its own mesh lists
 * and container. The copy does not own the null names. */
void renderer_record_clone(AnimationObject* source,
                           AnimationObject* destination);
/* FUN_00406d10: set the activity time. */
void animation_object_time_update(AnimationObject* object, float time);
/* FUN_00406d30: the activity's transition time for `count` repeats. */
float renderer_object_timing_value_adjust_by_count(void* record, float time,
                                                   unsigned int count);
/* FUN_00406da0: the activity's length. */
float animation_object_time_get(AnimationObject* object);
/* FUN_00406dc0: orient the container along (x, y). */
void animation_object_transform_set(AnimationObject* object, float x,
                                    float y);
/* FUN_00406df0: place the container at (x, y) and the height `heightFn`
 * gives for it. */
void render_node_position_from_callback(AnimationObject* object, float x,
                                        float y,
                                        AnimationObjectHeightFn heightFn,
                                        void* data);
/* FUN_00406e30: the container. */
void* animation_object_handle_get(const void* object);
/* FUN_00406e40: start an activity at `time`; clears the null cache. */
unsigned int activity_runner_update(AnimationObject* object,
                                    const char* activityName, float time);
/* FUN_00406ec0: release the meshes, the owned names and the container.
 * The port's contract (game/entity.hpp) types the object as an EntitySlot. */
void entity_payload_release_subslots(EntitySlot* slot);
/* FUN_00406f50: the `frameNo`th null called `name` in the container. */
void* animation_object_null_find(AnimationObject* object, const char* name,
                                 int frameNo);
/* FUN_00406f80: the camera null for `cameraNo`, cached. */
void* activity_runner_state_read(AnimationObject* object, int cameraNo);
/* FUN_00406fb0, FUN_00406fe0, FUN_00407010: the drill, carry and deposit
 * nulls, cached. */
void* renderer_cached_handle_40_get_or_create(void* object);
void* renderer_cached_handle_48_get_or_create(void* object);
void* renderer_cached_handle_50_get_or_create(AnimationObject* object);
/* FUN_00407040: whether the throw null's scale is not 1. */
int renderer_cached_handle_4c_is_not_one(AnimationObject* object);
/* FUN_00407090: the activity's transition time. */
float animation_object_value_get(AnimationObject* object);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_ANIMATION_OBJECT_H */
