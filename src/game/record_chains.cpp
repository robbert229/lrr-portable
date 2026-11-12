// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-044 GAME-044: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/GAME-044.csv. */
/* Matching build: LegoObject.c callers call legoobject_unit.c (#575). */
#include "legoobject_port_aliases.h"
#include <math.h>
#include <float.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "gods98/texture.hpp"
#include "crt/crt.hpp"
#include "heap.hpp"
#include "string.hpp"
#include "crt/crt_for_record_chains.hpp"
#include <stdint.h>
#include "ai.hpp"
#include "entity.hpp"
#include "building_scene.hpp"
#include "actor_scene.hpp"
#include "type_config_ports.hpp"
#include "entity_pose_delta.hpp"
#include "entity_motion_ports.hpp"
#include "activity_timeout.hpp"
#include "entity_motion_probe.hpp"
#include "port_loop_ports.hpp"
#include "simulation_ui_ports.hpp"
#include "type_motion_scale.hpp"
#include "entity_quad_collision.hpp"
#include "spawn.hpp"
#include "weapon.hpp"
#include "effects.hpp"
#include "level.hpp"
#include "terrain_grid.hpp"
#include "cmd.hpp"
#include "select.hpp"
#include "gods98/model.hpp"
#include "gods98/view.hpp"
#include "gods98/engine.hpp"
#include "gods98/resource.hpp"
#include "gods98/sound.hpp"
#include "gods98/record.hpp"
#include "gods98/message.hpp"
#include "gods98/block.hpp"
#include "upgrades.hpp"
#include "frontend.hpp"
#include "entity_for_record_chains.hpp"
#include "util.hpp"
#include "gods98/media.hpp"
#include "camera.hpp"
#include <stdarg.h>
#include "scene_resources.hpp"
#include "scene_pool.hpp"
#include "surface.hpp"
#include "view_for_record_chains.hpp"
#include "gods98/camera_for_record_chains.hpp"

struct SceneStateDispatchContext {
    uint32_t ordinal;
    uint32_t mode;
    uint32_t flags;
    SceneObjectStateCallback callback;
    void* user;
};

/* LinkedRecordChainFree / GlobalLinkedRecordChainClear — see crt.hpp. */
// GLOBAL: LEGORR 0x004aa450
void* s_linked_list_aux = NULL; /* DAT_004aa450 role: unowned, zero-init */
/* The global-list push (FUN_0046a470) — the corpus's head word has no
 * ported owner, so the head lands here; the link field
 * at record +0x28 is a port-side pointer-sized slot (the corpus's 32-bit
 * dword, widened with the host — deviation documented). */
/* Datacmp note: the corpus words at 0x4aa450/0x4aa458 are file-backed
 * zeros while the port's zero-init statics land in the exe's zero-fill
 * tail; reccmp-datacmp reads only file bytes per side and flags the pair
 * (uninitialized) — both sides are genuinely zero (image-verified). */
// GLOBAL: LEGORR 0x004aa458
void* s_linked_list_head = NULL;  /* DAT_004aa458 role */

/* The original's .bss word at 0x503188, directly after Stats.c's statsGlobs
 * (finding 581): Stats_FindToolFromTaskType's loop ends at its address. */
// GLOBAL: LEGORR 0x00503188
static uint32_t s_entity_flag_mask_all_pass;

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static void record_list_push(void** head, void* record);
static int entity_flag_mask_match_callback(EntitySlot* entity, void* payload);
static int scene_state_dispatch_callback(EntitySlot* entity, void* payload);
static void move_pipeline_hook(float delta);
static void move_post_hook(void);

// FUNCTION: LEGORR 0x0046a370
void linked_record_chain_free(void* record)
{
    if (record == NULL) {
        return; /* the corpus's gate */
    }
    void* next = NULL;
    __builtin_memcpy(&next, (char*)record + 0x1c, sizeof(next));
    linked_record_chain_free(next);
    engine_heap_free(record);
}

// FUNCTION: LEGORR 0x0046a390
void global_linked_record_chain_clear(void)
{
    void* head = s_linked_list_head; /* the corpus reads the head first */
    s_linked_list_aux = NULL;
    linked_record_chain_free(head);
    s_linked_list_head = NULL;
}

/* SelectedGlobalPointTransformAndReturnX (FUN_0046a420) — see view.hpp. */
// FUNCTION: LEGORR 0x0046a420
float selected_global_point_transform_and_return_x(EngineContext* ctx)
{
    if (ctx == NULL) {
        return 0.0f; /* deviation: the corpus strides the globals blindly */
    }
    ViewContext* v = (ctx->context_mode == 1) ? ctx->views[0] : ctx->views[2];
    float frame[3] = { 0.0f, 0.0f, 0.0f };
    (void)camera_get_position_with_terrain_height(v, ctx, frame);
    return frame[0];
}

// FUNCTION: LEGORR 0x0046a470
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void linked_list_push_at_global_head(void* record)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov ecx, dword ptr [s_linked_list_head]
        mov dword ptr [eax + 1ch], ecx
        mov dword ptr [s_linked_list_head], eax
        ret
    }
}
#else
void linked_list_push_at_global_head(void* record)
{
    record_list_push(&s_linked_list_head, record);
}
#endif

/* FlagMaskCallbacksAllPass (FUN_0046a4c0): for each set bit 0..30, visit
 * filtered entities and fail if LAB_0046a490 finds a matching type with a
 * nonzero +0x390 word. */
// FUNCTION: LEGORR 0x0046A4C0
int entity_flag_mask_callbacks_all_pass(uint32_t mask)
{
    for (uint32_t index = 0; index < 31u; ++index) {
        uint32_t bit = 1u << index;
        s_entity_flag_mask_all_pass = 1u;
        if ((mask & bit) != 0u) {
            entity_pool_find_by_callback_filtered_current(
                entity_flag_mask_match_callback, (void*)(uintptr_t)bit);
            if (s_entity_flag_mask_all_pass == 0u) return 0;
        }
    }
    return 1;
}
/* SceneObjectStateDispatch (FUN_0046a510): compute the selector ordinal,
 * optionally allocate/link its 0x20-byte record, then visit filtered entities
 * with the state tuple. The per-caller LAB_0046a3c0 effect and four global
 * words are represented by explicit install/callback hooks. Native records
 * reserve four extra bytes so the widened pointer at +0x1c fits. */
// FUNCTION: LEGORR 0x0046A510
void scene_object_state_dispatch(GameContext* ctx, uint32_t selector,
                                 uint32_t mode, uint32_t flags,
                                 SceneObjectStateCallback callback,
                                 void* user)
{
    SceneStateDispatchContext context;
    context.ordinal = first_set_flag_ordinal(selector);
    context.mode = mode;
    context.flags = flags;
    context.callback = callback;
    context.user = user;
    if ((mode & 2u) != 0u) {
        unsigned char* record = (unsigned char*)engine_heap_alloc(0x24);
        if (record != NULL) {
            uint32_t zero = 0u;
            memset(record, 0, 0x24);
            linked_list_push_at_global_head(record);
            memcpy(record + 0x10, &zero, sizeof(zero));
            if ((flags & 2u) != 0u) {
                memcpy(record, &s_scene_dispatch_point_low, sizeof(uint32_t));
                memcpy(record + 4, &s_scene_dispatch_point_high, sizeof(uint32_t));
                memcpy(record + 0x0c, &s_scene_dispatch_word_c, sizeof(uint32_t));
            }
            if ((flags & 1u) != 0u) {
                memcpy(record + 0x14, &s_scene_dispatch_word14, sizeof(uint32_t));
            }
            memcpy(record + 0x18, &flags, sizeof(flags));
        }
    }
    if (ctx != NULL && callback != NULL) {
        entity_pool_find_by_callback_filtered_current(
            scene_state_dispatch_callback, &context);
    }
}

/* LinkedRecordConditionalFloatAdvance (FUN_0046a700): walk the global
 * +0x1c chain and advance +8 by (+0xc * delta) only when +0x10 is nonzero
 * and +0x18 equals 2. The native chain pointer is widened in the port; the
 * observed record offsets and branch conditions are unchanged. */
// FUNCTION: LEGORR 0x0046A700
void linked_list_conditional_float_advance(float delta)
{
    void* record = s_linked_list_head;
    while (record != NULL) {
        uint32_t active = 0u;
        uint32_t mode = 0u;
        float value = 0.0f;
        float rate = 0.0f;
        void* next = NULL;
        memcpy(&active, (const char*)record + 0x10, sizeof(active));
        memcpy(&mode, (const char*)record + 0x18, sizeof(mode));
        if (active != 0u && mode == 2u) {
            memcpy(&value, (const char*)record + 8, sizeof(value));
            memcpy(&rate, (const char*)record + 0x0c, sizeof(rate));
            value = rate * delta + value;
            memcpy((char*)record + 8, &value, sizeof(value));
        }
        memcpy(&next, (const char*)record + 0x1c, sizeof(next));
        record = next;
    }
}

/* CameraMoveCommandRoute preserves the corpus's move-list call followed by
 * the conditional filtered entity walk. The callback body remains an
 * injectable, unresolved runtime input. */
// FUNCTION: LEGORR 0x0046a730
void camera_move_pipeline(EngineContext* ctx, float delta)
{
    /* The reference routine operates on global state and does not consume an
     * EngineContext. Keep the context parameter only for the port's caller
     * interface; a missing context must not suppress the global list step. */
    (void)ctx;
    move_pipeline_hook(delta);
    if (g_move_hook_enabled()) {
        move_post_hook();
    }
}

/* FirstSetFlagOrdinal (FUN_0046a760): return the one-based index of the
 * lowest selected bit in the 13-bit state selector, or zero when empty. */
// FUNCTION: LEGORR 0x0046A760
uint32_t first_set_flag_ordinal(uint32_t mask)
{
    for (uint32_t bit = 0u; bit < 13u; ++bit) {
        if ((mask & (1u << bit)) != 0u) {
            return bit + 1u;
        }
    }
    return 0u;
}
static void record_list_push(void** head, void* record)
{
    if (record == NULL) {
        return; /* deviation: the corpus strides the record blindly */
    }
    *(void**)((char*)record + 0x1c) = *head;
    *head = record;
}

static int entity_flag_mask_match_callback(EntitySlot* entity, void* payload)
{
    const uint32_t mask = (uint32_t)(uintptr_t)payload;
    const uint32_t type = first_set_flag_ordinal(mask);
    if (entity != NULL && (uint32_t)entity->type == type &&
        entity->flags_390 != 0u) {
        s_entity_flag_mask_all_pass = 0u;
    }
    return 0; /* retail visits every filtered entity for this mask bit */
}

static int scene_state_dispatch_callback(EntitySlot* entity, void* payload)
{
    SceneStateDispatchContext* context = (SceneStateDispatchContext*)payload;
    if (context->callback == NULL) {
        return 0;
    }
    return context->callback(entity, context->ordinal, context->mode,
                             context->flags, context->user);
}

/* The linked-record float advance has its original global list owner in
 * crt.cpp; the entity callback after it remains a separate seam. */
static void move_pipeline_hook(float delta)
{
    linked_list_conditional_float_advance(delta);
}

/* LAB_0046a5c0 is dispatched through EntityPoolFindByCallbackFiltered.
 * Keep the observed filtered traversal even when its unknown callback has
 * not been supplied. */
static int move_post_callback_dispatch(EntitySlot* entity, void* user)
{
    if (s_move_post_callback != NULL) {
        return s_move_post_callback(entity, s_move_post_user);
    }
    (void)entity;
    (void)user;
    return 0;
}

static void move_post_hook(void)
{
    GameContext* game = entity_pool_callback_context_get();
    if (game != NULL) {
        entity_pool_find_by_callback_filtered(
            game, move_post_callback_dispatch, NULL);
    }
}

/* Original-ABI adapter (ADR 0005 step 6) for GAME-021's lego_unit.c: the
 * original FUN_0046a510 takes (selector, mode, flags) and reaches the
 * per-object effect (LAB_0046a3c0) itself; the port's entry above adds the
 * game context and the callback. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (scene_object_state_dispatch)
extern "C" void scene_object_state_dispatch_original(uint32_t selector,
                                                     uint32_t mode,
                                                     uint32_t flags)
{
    scene_object_state_dispatch(entity_pool_callback_context_get(), selector,
                                mode, flags, NULL, NULL);
}
