// SPDX-License-Identifier: LGPL-3.0-or-later
/* Native-only forwarders for GAME-004's Container calls (container_route.h).
 * Port tail; never part of the matching build. */
#include "container_route.h"

#if defined(LEGORRR_BACKEND_MINIWIN)

#include "container_native.hpp"
#include "record.hpp"
#include "resource.hpp"
#include "string.hpp"
#include "texture.hpp"
#include "view.hpp"
#include "crt/crt.hpp"
#include "entity.h"
/* Last: container.h defines the routing macros undone below. */
#include "container.h"

/* container.h routes these names to the forwarders below; the forwarders
 * themselves call the gods98 functions. */
#undef container_load
#undef scene_render_lock_toggle
#undef entity_visual_resource_load
#undef ddiscene_frame_update_and_callbacks
#undef ddiscene_advance_by_time_target
#undef ddiscene_object_frame_position_get
#undef ddiscene_object_length_get
#undef ddiscene_object_pose_method_dispatch
#undef ddiscene_object_position_set
#undef scene_action_name_set
#undef ledger_stub_00472a40_DDiSceneObjectDestroy
#undef xf_texture_name_format
#undef resource_name_register_or_query_target
#undef typed_child_resource_scalar_get
#undef ledger_stub_004756d0_VideoObjectAspectRatioGet

extern "C" void* container_route_load(void* parent, const char* filename,
                                      const char* typeString, int looseLoad)
{
    if (typeString != NULL && strcasecmp_lite(typeString, "ACT") == 0)
        return container_native_load_act(filename,
                                         engine_feature_word_peek(), NULL, 0u);
    return container_load(parent, filename, typeString, looseLoad);
}

extern "C" void container_route_hide(void* scene_object, int enabled)
{
    if (container_native_owns(scene_object)) {
        container_native_hidden_set(scene_object, enabled);
        return;
    }
    scene_render_lock_toggle(scene_object, enabled);
}

extern "C" void* container_route_clone(void* scene_object)
{
    if (container_native_owns(scene_object))
        return container_native_clone(scene_object);
    return entity_visual_resource_load(scene_object);
}

extern "C" float container_route_animation_time_set(void* scene_object,
                                                    float time)
{
    if (container_native_owns(scene_object))
        return container_native_animation_time_set(scene_object, time);
    return ddiscene_frame_update_and_callbacks(scene_object, time);
}

extern "C" float container_route_animation_forward(void* scene_object,
                                                   float delta)
{
    if (container_native_owns(scene_object))
        return container_native_animation_forward(scene_object, delta);
    return ddiscene_advance_by_time_target(scene_object, delta);
}

extern "C" unsigned int container_route_animation_frames(void* scene_object)
{
    if (container_native_owns(scene_object))
        return container_native_animation_frames(scene_object);
    return ddiscene_object_frame_position_get(scene_object);
}

extern "C" float container_route_animation_time(void* scene_object)
{
    if (container_native_owns(scene_object))
        return container_native_animation_time(scene_object);
    return ddiscene_object_length_get(scene_object);
}

extern "C" int container_route_orientation_set(
    void* object, void* alternate_frame, float parameter_3,
    float parameter_4, float parameter_5, float parameter_6,
    float parameter_7, float parameter_8)
{
    if (alternate_frame == NULL && container_native_owns(object))
        return container_native_orientation_set(object, parameter_3,
            parameter_4, parameter_5, parameter_6, parameter_7, parameter_8);
    return ddiscene_object_pose_method_dispatch(object, alternate_frame,
        parameter_3, parameter_4, parameter_5, parameter_6, parameter_7,
        parameter_8);
}

extern "C" int container_route_position_set(void* object,
                                            void* alternate_frame,
                                            float x, float y, float z)
{
    if (alternate_frame == NULL && container_native_owns(object))
        return container_native_position_set(object, x, y, z);
    return ddiscene_object_position_set(object, alternate_frame, x, y, z);
}

extern "C" unsigned int container_route_activity_set(void* scene_object,
                                                     const char* action_name)
{
    if (container_native_owns(scene_object))
        return container_native_activity_set(scene_object, action_name);
    return scene_action_name_set(scene_object, action_name);
}

extern "C" void container_route_remove(void* scene_object)
{
    if (container_native_owns(scene_object)) {
        container_native_remove(scene_object);
        return;
    }
    ledger_stub_00472a40_DDiSceneObjectDestroy(scene_object);
}

extern "C" const char* container_route_part_name_format(
    void* texture_record, const char* name_base, const int* frame_index)
{
    if (container_native_owns(texture_record))
        return container_native_part_name_format(texture_record, name_base,
                                                 frame_index);
    return xf_texture_name_format(texture_record, name_base, frame_index);
}

extern "C" void* container_route_search_tree(
    void** root, const char* name, int mode,
    unsigned int* match_count_or_index)
{
    if (mode == 0 && container_native_owns(root))
        return container_native_part_find(root, name);
    return resource_name_register_or_query_target(root, name, mode,
                                                  match_count_or_index);
}

extern "C" float container_route_trans_coef(void* scene_object)
{
    if (container_native_owns(scene_object))
        return container_native_trans_coef(scene_object);
    return typed_child_resource_scalar_get(scene_object);
}

extern "C" float container_route_frame_scale(void* video_object)
{
    /* Native Containers and their null frames carry no scale. */
    if (container_native_object_known(video_object)) return 0.0f;
    return ledger_stub_004756d0_VideoObjectAspectRatioGet(video_object);
}

#endif /* LEGORRR_BACKEND_MINIWIN */
