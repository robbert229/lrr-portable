// SPDX-License-Identifier: LGPL-3.0-or-later
#include "entity.hpp"
#include "actor_scene.hpp"
#include "gods98/input.hpp"
#include "gods98/media.hpp"
#include "gods98/view.hpp"
#include "platform/platform.h"
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

void entity_motion_inputs_initialize(EntitySlot* entity)
{
    if (entity == NULL) return;
    entity->motion_input_2bc = 0;
    entity->motion_input_2c0 = 0;
    entity->motion_input_2c4 = 0.0f;
    entity->motion_inputs_initialized = 1u;
    entity->activity_generation = 0u;
    entity->activity_advance_valid = 0u;
    entity->activity_advance_generation = 0u;
    entity->activity_advance_activity = -1;
    entity->activity_advance_overflow = 0.0f;
}

static void motion_add32(int32_t* field, int value)
{
    uint32_t bits;
    memcpy(&bits, field, sizeof(bits));
    bits += (uint32_t)value;
    memcpy(field, &bits, sizeof(bits));
}

void entity_motion_input_accumulate(EntitySlot* entity, int forward,
                                    int strafe, float yaw)
{
    EntitySlot* target = entity;
    if (entity == NULL || !entity->motion_inputs_initialized) return;
    if (entity->type == 2 && entity->owner_36c != NULL)
        target = (EntitySlot*)entity->owner_36c;
    if (!target->motion_inputs_initialized || target->stats_record == NULL) {
        entity->motion_inputs_initialized = 0u;
        return;
    }
    /* 44a310 checks and resets separately for each held-key call. */
    if ((target->flags_3e0 & 1u) != 0u)
        entity_action_sequence_reset(target, 0);
    if ((entity_capability_flags_get(target) & 4u) == 0u) strafe = 0;
    motion_add32(&target->motion_input_2bc, strafe);
    motion_add32(&target->motion_input_2c0, forward);
    target->motion_input_2c4 = (float)(target->motion_input_2c4 + yaw);
}

static EntitySlot* motion_controlled_entity(void)
{
    void* controlled = media_min_distance_context();
    GameContext* game = entity_pool_callback_context_get();
    int type, index;
    if (controlled == NULL || game == NULL) return NULL;
    for (type = 1; type <= 19; ++type) {
        int count = entity_count(game, type);
        for (index = 0; index < count; ++index) {
            EntitySlot* entity = (EntitySlot*)entity_at(game, type, index);
            if (entity == controlled && entity->pool_link_408 == (uintptr_t)entity)
                return entity;
        }
    }
    return NULL;
}

void entity_motion_inputs_capture(const InputState* input)
{
    EntitySlot* entity;
    if (input == NULL || media_min_distance_mode() != 0) return;
    entity = motion_controlled_entity();
    if (entity == NULL || (entity->flags_3e4 & 1u) != 0u) return;
    if (input_key_down(input, PLATFORM_KEY_UP))
        entity_motion_input_accumulate(entity, 1, 0, 0.0f);
    if (input_key_down(input, PLATFORM_KEY_DOWN))
        entity_motion_input_accumulate(entity, -1, 0, 0.0f);
    if (input_key_down(input, PLATFORM_KEY_LEFT))
        entity_motion_input_accumulate(entity, 0, 0, -0.05f);
    if (input_key_down(input, PLATFORM_KEY_RIGHT))
        entity_motion_input_accumulate(entity, 0, 0, 0.05f);
    if (input_key_down(input, 'z'))
        entity_motion_input_accumulate(entity, 0, -1, 0.0f);
    if (input_key_down(input, 'x'))
        entity_motion_input_accumulate(entity, 0, 1, 0.0f);
}

static int motion_finite(float value)
{
    return value == value && value >= -FLT_MAX && value <= FLT_MAX;
}

static int motion_unavailable(EntitySlot*, void*, const char*)
{
    return -1;
}

int entity_motion_idle_gate_checked(EntitySlot* entity, float delta)
{
    void* component;
    float movement_delta = 0.0f, scale = 0.0f;
    float first[3], second[3], position[3];
    int i;
    if (entity == NULL || !entity->motion_inputs_initialized ||
        (entity->type != 2 && entity->type != 3))
        return motion_unavailable(entity, NULL, "input lifetime/type");
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    component = actor_scene_motion_component(entity);
#else
    component = entity->attached_scene_object;
#endif
    /* 442b10 performs all four preamble operations even for zero inputs.
     * Its computed scale is not written out on that zero branch. */
    if (component == NULL || scene_root_object_get() == NULL)
        return motion_unavailable(entity, component, "component/root");
    if (!entity_motion_scale_checked(entity, delta, &movement_delta, &scale))
        return motion_unavailable(entity, component, "scale/terrain");
    if (!ddiscene_object_facing_pair_get(component, NULL, first, second))
        return motion_unavailable(entity, component, "orientation");
    if (!ddiscene_object_position_get(component, NULL, position))
        return motion_unavailable(entity, component, "position");
    for (i = 0; i < 3; ++i)
        if (!motion_finite(first[i]) || !motion_finite(second[i]) ||
            !motion_finite(position[i]))
            return motion_unavailable(entity, component, "nonfinite pose");
    if (!motion_finite(entity->motion_input_2c4))
        return motion_unavailable(entity, component, "nonfinite input");
    /* Nonzero translation/collision ownership is not yet complete. Retain
     * those inputs; an unavailable operation must never unlock completion. */
    if (entity->motion_input_2bc != 0 || entity->motion_input_2c0 != 0 ||
        entity->motion_input_2c4 != 0.0f)
        return motion_unavailable(entity, component, "pending nonzero motion");

    entity->motion_input_2bc = 0;
    entity->motion_input_2c0 = 0;
    entity->motion_input_2c4 = 0.0f;
    return 0;
}

int entity_neighbor_step_idle_checked(EntitySlot* entity, int mode)
{
    uint32_t stats;
    int blocked;
    if (entity == NULL || entity->stats_record == NULL) return -1;
    stats = entity_stats_word_get(entity);
    blocked = (entity->flags_3e0 & 1u) != 0u ||
        entity_flags_mask107c33de_or_1c00203_any_test(entity);
    /* Both 433c10 actions are gated by this actual predicate before RNG or
     * route side effects. Other eligible branches remain unavailable here. */
    if (((stats & 0x1000u) == 0u || blocked) &&
        ((mode == 0 && (stats & 0x400u) == 0u) || blocked)) return 0;
    return -1;
}
