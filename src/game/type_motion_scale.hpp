// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGORRR_GAME_TYPE_MOTION_SCALE_HPP
#define LEGORRR_GAME_TYPE_MOTION_SCALE_HPP

#include "entity.hpp"

typedef struct TypeMotionScaleHooks {
    void* context;
    int (*indexed_entry_get)(void* context, int weapon_type,
                             float* scale_out, int* value_out);
    void (*motion_apply)(void* context, EntitySlot* entity, float amount,
                         int mode, float scale);
    void (*entry_state_store)(void* context, EntitySlot* entity,
                              int weapon_type, int entry_value);
    void (*finish_apply)(void* context, EntitySlot* entity,
                         void* argument, int mode);
    void (*type3_entry_miss)(void* context, EntitySlot* entity);
} TypeMotionScaleHooks;

int type_motion_scale_apply_dispatch(EntitySlot* entity, int weapon_type,
                                     int apply_index, float scale,
                                     void* finish_argument,
                                     const TypeMotionScaleHooks* hooks);

#endif
