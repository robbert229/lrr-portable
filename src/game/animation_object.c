// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-004 — original translation unit (text 00406990-004070b0; finding
 * 505). Animation objects: a loaded activity container with the null names
 * from its `.ae` config, a cache of the nulls looked up by name, and three
 * level-of-detail mesh lists.
 *
 * Rebuilt in the original's shape (issue #513, ADR 0005): one file for the
 * unit, its 21 functions in original order and the C-compatible subset, so
 * the matching build compiles it as C. The native port compiles it as C++.
 * Functions keep the port's names, apart from the new
 * animation_object_null_find (0x00406f50). Where a C++ caller that is not
 * converted yet uses a function, it keeps the port's parameter types and
 * casts to the record (finding 549). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "animation_object.h"
#include "entity.h"
#include "gods98/config.h"
#include "gods98/container.h"
#include "gods98/entity.h"
#include "gods98/record.h"
#include "gods98/texture.h"
#include "gods98/utils.h"
#include "gods98/view.h"
#include "crt/vc5names.h"

/* The unit's .rdata (0049e054-0049e058). */
// GLOBAL: LEGORR 0x0049e054
static const float s_animation_object_zero = 0.0f;
// GLOBAL: LEGORR 0x0049e058
static const float s_animation_object_one = 1.0f;

// FUNCTION: LEGORR 0x00406990
int entity_type_config_resource_row_load(void* destination,
                                         unsigned int objectId, void* root,
                                         const char* filename,
                                         void* configName,
                                         unsigned int rowKind)
{
    AnimationObject* object = (AnimationObject*)destination;
    const char* gameName = (const char*)configName;
    char baseDir[1024];
    char path[1024];
    char* parts[100];
    unsigned int count;
    unsigned int i;
    BlockFile* config;
    int result = 0;

    strcpy(baseDir, filename);
    count = sound_variant_split(baseDir, parts,
                                // STRING: LEGORR 0x004a19c8
                                "\\");
    if (count > 1) {
        for (i = 1; i < count; i++) parts[i][-1] = '\\';
    }
    sprintf(path,
            // STRING: LEGORR 0x004a19b8
            "%s\\%s.%s",
            baseDir, parts[count - 1],
            // STRING: LEGORR 0x004a19c4
            "ae");

    if ((config = block_open(path))) {
        if ((object->container = container_load(root, filename,
                                                // STRING: LEGORR 0x004a19b4
                                                "ACT",
                                                1))) {
            if ((object->cameraNullName = block_get_str_copy(config,
                    string_path_join(gameName,
                                     // STRING: LEGORR 0x004a19a4
                                     "CameraNullName",
                                     NULL)))) {
                object->cameraNullFrames = atoi(block_get_strptr(config,
                        string_path_join(gameName,
                                         // STRING: LEGORR 0x004a1990
                                         "CameraNullFrames",
                                         NULL))
                    ? block_get_strptr(config,
                          string_path_join(gameName, "CameraNullFrames", NULL))
                    : "");
                object->cameraFlipDir = block_get_bool_path(config,
                    string_path_join(gameName,
                                     // STRING: LEGORR 0x004a1980
                                     "CameraFlipDir",
                                     NULL));
            } else {
                object->cameraNullFrames = 0;
            }
            object->drillNullName = block_get_str_copy(config,
                string_path_join(gameName,
                                 // STRING: LEGORR 0x004a1970
                                 "DrillNullName",
                                 NULL));
            object->footStepNullName = block_get_str_copy(config,
                string_path_join(gameName,
                                 // STRING: LEGORR 0x004a195c
                                 "FootStepNullName",
                                 NULL));
            object->carryNullName = block_get_str_copy(config,
                string_path_join(gameName,
                                 // STRING: LEGORR 0x004a194c
                                 "CarryNullName",
                                 NULL));
            object->throwNullName = block_get_str_copy(config,
                string_path_join(gameName,
                                 // STRING: LEGORR 0x004a193c
                                 "ThrowNullName",
                                 NULL));
            object->depositNullName = block_get_str_copy(config,
                string_path_join(gameName,
                                 // STRING: LEGORR 0x004a192c
                                 "DepositNullName",
                                 NULL));
            object->objectId = objectId;
            object->polyMedium = mesh_lod_load(config, gameName, filename, 1, 1);
            object->polyHigh = mesh_lod_load(config, gameName, filename, 2, 1);
            object->polyFP = mesh_lod_load(config, gameName, filename, 3,
                                           object->cameraNullFrames);
            object->flags = ANIMATION_OBJECT_FLAG_OWNS_NAMES;
            result = 1;
        }
        block_close(config);
    }
    return result;
}

// FUNCTION: LEGORR 0x00406c00
void renderer_object_update_from_record_field64(AnimationObject* object,
                                                int swap)
{
    resource_list_update_matching_key(object->polyMedium, object->container,
                                      (void*)(size_t)(swap == 0), 0);
}

// FUNCTION: LEGORR 0x00406c30
void renderer_object_update_from_record_field68(AnimationObject* object,
                                                int swap)
{
    resource_list_update_matching_key(object->polyHigh, object->container,
                                      (void*)(size_t)(swap == 0), 0);
}

// FUNCTION: LEGORR 0x00406c60
void renderer_object_update_from_record_field6c(AnimationObject* object,
                                                int swap,
                                                unsigned int camera)
{
    resource_list_update_matching_key(object->polyFP, object->container,
                                      (void*)(size_t)(swap == 0), camera);
}

// FUNCTION: LEGORR 0x00406c90
void animation_object_visibility_set(const void* object, unsigned int hide)
{
    scene_render_lock_toggle(((const AnimationObject*)object)->container,
                             hide);
}

// FUNCTION: LEGORR 0x00406cb0
void renderer_record_clone(AnimationObject* source,
                           AnimationObject* destination)
{
    *destination = *source;
    destination->flags &= ~ANIMATION_OBJECT_FLAG_OWNS_NAMES;
    destination->polyMedium = model_entry_list_flatten(source->polyMedium);
    destination->polyHigh = model_entry_list_flatten(source->polyHigh);
    destination->polyFP = model_entry_list_flatten(source->polyFP);
    destination->container = entity_visual_resource_load(source->container);
}

// FUNCTION: LEGORR 0x00406d10
void animation_object_time_update(AnimationObject* object, float time)
{
    ddiscene_frame_update_and_callbacks(object->container, time);
}

// FUNCTION: LEGORR 0x00406d30
float renderer_object_timing_value_adjust_by_count(void* record, float time,
                                                   unsigned int count)
{
    AnimationObject* object = (AnimationObject*)record;
    float value = ddiscene_advance_by_time_target(object->container, time);
    if (count > 1 && s_animation_object_zero != value) {
        return value - (float)(ddiscene_object_frame_position_get(
                                   object->container) * (count - 1));
    }
    return value;
}

// FUNCTION: LEGORR 0x00406da0
float animation_object_time_get(AnimationObject* object)
{
    return ddiscene_object_length_get(object->container);
}

// FUNCTION: LEGORR 0x00406dc0
void animation_object_transform_set(AnimationObject* object, float x,
                                    float y)
{
    ddiscene_object_pose_method_dispatch(object->container, NULL, x, y,
                                         0.0f, 0.0f, 0.0f, -1.0f);
}

// FUNCTION: LEGORR 0x00406df0
void render_node_position_from_callback(AnimationObject* object, float x,
                                        float y,
                                        AnimationObjectHeightFn heightFn,
                                        void* data)
{
    float z = heightFn(x, y, data);
    ddiscene_object_position_set(object->container, NULL, x, y, z);
}

// FUNCTION: LEGORR 0x00406e30
void* animation_object_handle_get(const void* object)
{
    return ((const AnimationObject*)object)->container;
}

// FUNCTION: LEGORR 0x00406e40
unsigned int activity_runner_update(AnimationObject* object,
                                    const char* activityName, float time)
{
    unsigned int result;
    unsigned int i;

    linked_record_field4_clear(object->polyMedium);
    linked_record_field4_clear(object->polyHigh);
    linked_record_field4_clear(object->polyFP);

    object->drillNull = NULL;
    object->footStepNull = NULL;
    object->carryNull = NULL;
    object->throwNull = NULL;
    object->depositNull = NULL;
    for (i = 0; i < object->cameraNullFrames; i++) {
        object->cameraNulls[i] = NULL;
    }

    result = scene_action_name_set(object->container, activityName);
    ddiscene_frame_update_and_callbacks(object->container, time);
    return result;
}

// FUNCTION: LEGORR 0x00406ec0
void entity_payload_release_subslots(EntitySlot* slot)
{
    AnimationObject* object = (AnimationObject*)slot;

    owned_object_reference_chain_release(object->polyMedium);
    owned_object_reference_chain_release(object->polyHigh);
    owned_object_reference_chain_release(object->polyFP);

    if (object->flags & ANIMATION_OBJECT_FLAG_OWNS_NAMES) {
        if (object->drillNullName) free(object->drillNullName);
        if (object->footStepNullName) free(object->footStepNullName);
        if (object->cameraNullName) free(object->cameraNullName);
        if (object->carryNullName) free(object->carryNullName);
        if (object->throwNullName) free(object->throwNullName);
    }
    ledger_stub_00472a40_DDiSceneObjectDestroy(object->container);
}

// FUNCTION: LEGORR 0x00406f50
void* animation_object_null_find(AnimationObject* object, const char* name,
                                 int frameNo)
{
    const char* partName = xf_texture_name_format(object->container, name,
                                                  &frameNo);
    return resource_name_register_or_query_target(
        (void**)object->container, partName, 0, NULL);
}

// FUNCTION: LEGORR 0x00406f80
void* activity_runner_state_read(AnimationObject* object, int cameraNo)
{
    if (object->cameraNullName) {
        if (object->cameraNulls[cameraNo] == NULL) {
            object->cameraNulls[cameraNo] = animation_object_null_find(
                object, object->cameraNullName, cameraNo);
        }
        return object->cameraNulls[cameraNo];
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00406fb0
void* renderer_cached_handle_40_get_or_create(void* object)
{
    AnimationObject* model = (AnimationObject*)object;
    if (model->drillNull == NULL) {
        if (model->drillNullName) {
            return model->drillNull = animation_object_null_find(
                       model, model->drillNullName, 0);
        }
        return NULL;
    }
    return model->drillNull;
}

// FUNCTION: LEGORR 0x00406fe0
void* renderer_cached_handle_48_get_or_create(void* object)
{
    AnimationObject* model = (AnimationObject*)object;
    if (model->carryNullName) {
        if (model->carryNull == NULL) {
            return model->carryNull = animation_object_null_find(
                       model, model->carryNullName, 0);
        }
        return model->carryNull;
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00407010
void* renderer_cached_handle_50_get_or_create(AnimationObject* object)
{
    if (object->depositNullName) {
        if (object->depositNull == NULL) {
            return object->depositNull = animation_object_null_find(
                       object, object->depositNullName, 0);
        }
        return object->depositNull;
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00407040
int renderer_cached_handle_4c_is_not_one(AnimationObject* object)
{
    if (object->throwNull == NULL && object->throwNullName) {
        object->throwNull = animation_object_null_find(
            object, object->throwNullName, 0);
    }
    if (object->throwNull) {
        if (ledger_stub_004756d0_VideoObjectAspectRatioGet(object->throwNull)
            != s_animation_object_one) {
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00407090
float animation_object_value_get(AnimationObject* object)
{
    return typed_child_resource_scalar_get(object->container);
}
