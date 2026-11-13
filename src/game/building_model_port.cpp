// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-006 Building.c port tail (finding 592): the port's models of the
 * unit, unannotated. The original-shape unit is building_model.c; these
 * models keep the port's contracts (EntitySlot scene-slot sidecars, the
 * resolver and release hooks, NULL guards) for the native port, which calls
 * them by these names.
 *
 * In the matching build (MSVC 5.0) building_model.c defines most of these
 * names itself (building_model_link_names.h), so their definitions here are
 * compiled out there (LRR_BUILDING_MODEL_PORT_NAMES). Only the slot-table
 * rebuild (0x004083a0), whose callers are all port code with another
 * contract, stays in every build.
 * Relocated from the verbatim building_model.cpp (body hashes:
 * docs/coverage/relocations/GAME-006.csv). */
#include "record.hpp"
#include "heap.hpp"
#include "sound.hpp"
#include "string.hpp"
#include "view.hpp"
#include "texture.hpp"
#include "resource.hpp"
#include "crt/crt.hpp"
#include "game/util.hpp"
#include <math.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include "record_for_building_model.hpp"
#include <float.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include "crt/crt_for_building_model.hpp"
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
#include "gods98/engine.hpp"
#include "gods98/message.hpp"
#include "gods98/block.hpp"
#include "upgrades.hpp"
#include "frontend.hpp"
#include "gods98/media.hpp"
#include "entity_for_building_model.hpp"
#include "gods98/entity.hpp"
#include "entity_config_for_building_model.hpp"
#include <stdarg.h>
#include "scene_resources.hpp"
#include "scene_pool.hpp"
#include "surface.hpp"
#include "view_for_building_model.hpp"
#include "gods98/record_for_building_model.hpp"
#include "crt/crt_for_building_model.hpp"
#include "entity_for_building_model.hpp"
#include "entity_config_for_building_model.hpp"
#include "gods98/view_for_building_model.hpp"

/* The port's +0.0f carrier for constant_zero_float_get. Building.c's
 * .rdata 0x0049e084 is building_model.c's literal now. */
static const float  s_zero_float     = 0.0f;

/* The port's definitions of the names the unit takes in the matching build
 * (building_model_link_names.h): every build but MSVC 5.0's. */
#if defined(_MSC_VER) && _MSC_VER < 1200
#define LRR_BUILDING_MODEL_PORT_NAMES 0
#else
#define LRR_BUILDING_MODEL_PORT_NAMES 1
#endif

#if LRR_BUILDING_MODEL_PORT_NAMES /* the unit serves these names in the matching build */
/* Defined below (forward declarations added by tools/relocate_unit.py). */
static void pair_handle_set_pose(void* h, float x, float y);

/* Port model of 0x00407d60 (Building.c, building_model.c). */
int entity_building_type_config_record_parse(void* destination,
                                             uint32_t row_index,
                                             void* scene_context,
                                             const char* asset_path,
                                             void* engine_config)
{
    if (destination == NULL) return 0;
    memset(destination, 0, 0x14cu);
    return entity_type_config_resource_row_load(
        destination, row_index, scene_context, asset_path, engine_config, 2u);
}

/* SceneObjectScalarAdjustByDirection (FUN_004082e0) — see record.hpp. The
 * corpus callers are the config-record parse loop and the simulation flush;
 * the fold itself is a pure record-slot scale (the corpus's x87 double fold
 * lands as the same double arithmetic here). */
/* Port model of 0x004082e0 (Building.c, building_model.c). */
void scene_object_scalar_adjust_by_direction(void* record, int direction)
{
    if (record == NULL) {
        return; /* deviation: the corpus strides the record blindly */
    }
    float scalar = 0.0f;
    __builtin_memcpy(&scalar, (const char*)record + 0x80, sizeof scalar);
    float result;
    if (direction == 0) {
        result = (float)((scalar - 0.1) * 10.0 * 0.11111111);
    } else {
        result = (float)(scalar - (1.0 - scalar) * -0.1);
    }
    if (result < 0.0f) {
        result = 0.0f;
    }
    __builtin_memcpy((char*)record + 0x80, &result, sizeof result);
    uint32_t flags = 0u;
    __builtin_memcpy(&flags, (const char*)record + 0x148, sizeof flags);
    flags |= 2u;
    __builtin_memcpy((char*)record + 0x148, &flags, sizeof flags);
}
/* TaskField84GetAndField88Out (FUN_00408360): after the pinned NULL-task
 * guard, mirror `test eax,eax; je`, the optional +0x88 load/store, then the
 * +0x84 answer load. The corpus itself does not guard the task pointer. */
/* Port model of 0x00408360 (Building.c, building_model.c). */
uint32_t task_field84_get_and_field88_out(const void* task, uint32_t* out88)
{
    if (task == NULL) {
        return 0u;
    }
    if (out88 != NULL) {
        *out88 = *(const uint32_t*)((const char*)task + 0x88);
    }
    return *(const uint32_t*)((const char*)task + 0x84);
}

/* SceneAssignEntityOwner (FUN_00408380) — owner store on the +4 object
 * through the ported owner set. */
/* Port model of 0x00408380 (Building.c, building_model.c). */
void scene_assign_entity_owner(const void* scene, uint32_t owner)
{
    scene_object_owner_set(*(void* const*)((const char*)scene + 0x04), owner);
}

#endif /* LRR_BUILDING_MODEL_PORT_NAMES */

/* SceneEntitySlotTableRebuild (FUN_004083a0): clear the owner windows, walk
 * its +0x9c scene-node chain, prepare each node's renderer, select its
 * animation, then register up to two +0x1c handles followed by +0x24 and
 * +0x20 handles for payload nodes. Host pointers live in native-width
 * sidecars; renderer preparation, state, animation and resource lookup cross
 * the DDiScene seam. */
/* Port model of 0x004083a0 (Building.c, building_model.c). */
void entity_scene_slot_table_rebuild(EntitySlot* e, int mode)
{
    EntitySceneSlotNode* node;
    uint32_t visited = 0u;
    uint32_t cached = 0u;
    if (e == NULL) return;

    memset(e->scene_slot_cache, 0, sizeof(e->scene_slot_cache));
    memset(e->scene_slot_owner_1c_handles, 0,
           sizeof(e->scene_slot_owner_1c_handles));
    memset(e->scene_slot_owner_24_handles, 0,
           sizeof(e->scene_slot_owner_24_handles));
    memset(e->scene_slot_owner_20_handles, 0,
           sizeof(e->scene_slot_owner_20_handles));
    memset(e->scene_slot_owner_1c_counts, 0,
           sizeof(e->scene_slot_owner_1c_counts));
    memset(e->scene_slot_payloads, 0, sizeof(e->scene_slot_payloads));
    e->scene_slot_registration_count = 0u;
    e->scene_slot_cache_count = 0u;
    ++e->scene_slot_generation;

    node = e->scene_slot_nodes;
    while (node != NULL && visited < e->scene_slot_node_count) {
        void* registered[4] = { NULL, NULL, NULL, NULL };
        uint32_t registered_count = 0u;
        uint32_t slot;
        ++visited;
        memset(node->slot_handles, 0, sizeof(node->slot_handles));
        node->slot_handle_count = 0u;
        node->animation_mode = mode;
        if (s_entity_scene_slot_runtime_ops.renderer_position_prepare != NULL)
            s_entity_scene_slot_runtime_ops.renderer_position_prepare(
                s_entity_scene_slot_runtime_ops.context, e, node);
        if (s_entity_scene_slot_runtime_ops.renderer_state_apply != NULL)
            s_entity_scene_slot_runtime_ops.renderer_state_apply(
                s_entity_scene_slot_runtime_ops.context, node);
        if (s_entity_scene_slot_runtime_ops.node_animation_select != NULL)
            s_entity_scene_slot_runtime_ops.node_animation_select(
                s_entity_scene_slot_runtime_ops.context, node, mode);

        if (node->payload != NULL) {
            uint32_t payload_index = e->scene_slot_registration_count;
            void* owner_1c[2] = { NULL, NULL };
            void* owner_24 = NULL;
            void* owner_20 = NULL;
            uint32_t owner_1c_count = 0u;
            if (s_entity_scene_slot_runtime_ops.resource_slot_register != NULL) {
                /* FUN_004083a0 stops the repeated +0x1c lookup at its first
                 * null handle, then always attempts +0x24 and +0x20 once.
                 * The native +0xd8 count includes that terminating lookup. */
                for (slot = 0u; slot < 2u; ++slot) {
                    void* handle =
                        s_entity_scene_slot_runtime_ops.resource_slot_register(
                            s_entity_scene_slot_runtime_ops.context, node,
                            e->scene_slot_owner_keys[0], slot);
                    ++owner_1c_count;
                    if (handle == NULL) break;
                    owner_1c[slot] = handle;
                    registered[registered_count++] = handle;
                }
                owner_24 =
                    s_entity_scene_slot_runtime_ops.resource_slot_register(
                        s_entity_scene_slot_runtime_ops.context, node,
                        e->scene_slot_owner_keys[2], 0u);
                registered[registered_count++] = owner_24;
                owner_20 =
                    s_entity_scene_slot_runtime_ops.resource_slot_register(
                        s_entity_scene_slot_runtime_ops.context, node,
                        e->scene_slot_owner_keys[1], 0u);
                registered[registered_count++] = owner_20;
            }
            if (payload_index < 12u)
                e->scene_slot_payloads[payload_index] = node->payload;
            if (payload_index < 3u) {
                e->scene_slot_owner_1c_counts[payload_index] =
                    owner_1c_count;
                e->scene_slot_owner_1c_handles[payload_index][0] =
                    owner_1c[0];
                e->scene_slot_owner_1c_handles[payload_index][1] =
                    owner_1c[1];
                e->scene_slot_owner_24_handles[payload_index] = owner_24;
                e->scene_slot_owner_20_handles[payload_index] = owner_20;
            }
            node->slot_handle_count = registered_count;
            memcpy(node->slot_handles, registered, sizeof(registered));
            for (slot = 0u; slot < registered_count && cached < 12u; ++slot)
                e->scene_slot_cache[cached++] = registered[slot];
            /* Retail publishes +0x144 only after all slot lookups finish. */
            e->scene_slot_registration_count = payload_index + 1u;
        }
        node = node->next;
    }
    e->scene_slot_cache_count = cached;
}

#if LRR_BUILDING_MODEL_PORT_NAMES /* the unit serves these names in the matching build */
/* SceneActivityChangeSecondary (FUN_004085f0): resolve the requested indexed
 * child from +0x0c and cache it in the +0x40 handle table. */
/* Port model of 0x004085f0 (Building.c, building_model.c). */
void* scene_activity_change_secondary(void* activity, int index)
{
    void** cached;
    void* result = NULL;
    if (activity == NULL) return NULL;
    if (*(void**)((char*)activity + 0x0c) != NULL) {
        cached = (void**)((char*)activity + 0x40) + index;
        if (*cached == NULL) {
            *cached = scene_object_subobject_resolve(
                activity, *(void**)((char*)activity + 0x0c), index);
        }
        result = *cached;
    }
    return result;
}

/* Port model of 0x00408620 (Building.c, building_model.c). */
void scene_object_record_clone_retain_handles(
    const SceneObjectRecord* source, SceneObjectRecord* destination)
{
    unsigned int i;
    if (source == NULL || destination == NULL) return;
    for (i = 0; i < 0x53u; ++i) destination->words[i] = source->words[i];
    destination->words[0x52] = (void*)((uintptr_t)destination->words[0x52]
                                      & ~(uintptr_t)1u);
    if (s_scene_object_record_retain_hook != NULL) {
        if (source->words[1] != NULL)
            destination->words[1] = s_scene_object_record_retain_hook(
                s_scene_object_record_retain_context, source->words[1]);
        if (source->words[0x1f] != NULL)
            destination->words[0x1f] = s_scene_object_record_retain_hook(
                s_scene_object_record_retain_context, source->words[0x1f]);
    }
}

/* SceneObjectPairApplyMode (FUN_00408670) — the corpus blindly loads the +4
 * object and +0x7c partner. Copy native-width port pointer slots inline to
 * avoid rec_ptr calls; the port retains its widened-slot layout (the corpus
 * tests a dword at +0x7c). NULL record behavior therefore follows the corpus
 * and is no longer guarded. */
/* Port model of 0x00408670 (Building.c, building_model.c). */
void scene_object_pair_apply_mode(const void* record, uint32_t flag)
{
    void* object;
    __builtin_memcpy(&object, (const char*)record + 0x04, sizeof(object));
    scene_render_lock_toggle(object, (int)flag);
    void* partner;
    __builtin_memcpy(&partner, (const char*)record + 0x7c, sizeof(partner));
    if (partner != NULL) {
        scene_render_lock_toggle(partner, (int)flag);
    }
}

/* AnimationObjectVisibilityGet (FUN_004086a0) — the +4 object's flag bit 3
 * through the ported getter. */
/* Port model of 0x004086a0 (Building.c, building_model.c). */
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) uint32_t animation_object_visibility_get(const void* anim)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov ecx, dword ptr [eax + 4]
        push ecx
        call scene_object_flag_bit3_get_target
        add esp, 4
        ret
    }
}
#else
uint32_t animation_object_visibility_get(const void* anim)
{
    return scene_object_flag_bit3_get(rec_ptr(anim, 0x04));
}
#endif
/* SceneObjectPairSetRenderParameters (FUN_004086c0): the corpus blindly
 * loads the +4 object and +0x7c partner; inline native-width copies match
 * those loads without rec_ptr calls. The pose dispatch's argument block
 * (x, y, 0 / 0, 0, -1) reads as position + constant facing; the partner is
 * used only when the +0x7c slot is non-NULL. */
/* Port model of 0x004086c0 (Building.c, building_model.c). */
void scene_object_pair_set_render_parameters(void* pair, float x, float y)
{
    void* object;
    __builtin_memcpy(&object, (char*)pair + 0x04, sizeof(object));
    pair_handle_set_pose(object, x, y);
    void* partner;
    __builtin_memcpy(&partner, (char*)pair + 0x7c, sizeof(partner));
    if (partner != NULL) {
        pair_handle_set_pose(partner, x, y);
    }
}

/* SceneObjectPairSetCallbackScalar (FUN_00408710): run the scalar probe on
 * the (x, y) pair with its third argument — the corpus consumes the probe's
 * undefined float return as the camera z; the port pins it to zero (the
 * sole call-site probe, the ported height adapter, answers void). */
/* Port model of 0x00408710 (Building.c, building_model.c). */
void scene_object_pair_set_callback_scalar(void* pair, float x, float y,
                                           SceneScalarProbe probe, void* arg)
{
    if (probe != NULL) {
        probe(x, y, arg);
    }
    cam_set_pos((CameraHandle*)rec_ptr(pair, 0x04), x, y, 0.0f);
    void* partner = rec_ptr(pair, 0x7c);
    if (partner != NULL) {
        cam_set_pos((CameraHandle*)partner, x, y, 0.0f);
    }
}

/* SceneObjectIndexedHandleGetOrCreate (FUN_00408770): resolve the indexed
 * child from +8 and cache it in the corresponding +0x28 slot. The installed
 * resolver models the retail resource-tree lookup/apply sequence. */
/* Port model of 0x00408770 (Building.c, building_model.c). */
void* scene_object_indexed_handle_get_or_create(void* record, int index)
{
    void** source;
    void** cached;
    if (record == NULL) return NULL;
    source = (void**)((char*)record + 8);
    if (*source == NULL) return NULL;
    cached = (void**)((char*)record + 0x28) + index;
    if (*cached == NULL)
        *cached = scene_object_subobject_resolve(record, *source, index);
    return *cached;
}

/* SceneObjectPrimaryHandleGetOrCreate (FUN_004087b0). */
/* Port model of 0x004087b0 (Building.c, building_model.c). */
void* scene_object_primary_handle_get_or_create(void* record)
{
    void** source;
    void** cached;
    if (record == NULL) return NULL;
    source = (void**)((char*)record + 0x10);
    if (*source == NULL) return NULL;
    cached = (void**)((char*)record + 0x68);
    if (*cached == NULL) {
        *cached = scene_object_subobject_resolve(record, *source, 0);
        return *cached;
    }
    return *cached;
}

/* SceneObjectSecondaryHandleGetOrCreate (FUN_004087e0). */
/* Port model of 0x004087e0 (Building.c, building_model.c). */
void* scene_object_secondary_handle_get_or_create(void* record)
{
    void** source;
    void** cached;
    if (record == NULL) return NULL;
    source = (void**)((char*)record + 0x14);
    if (*source == NULL) return NULL;
    cached = (void**)((char*)record + 0x6c);
    if (*cached == NULL) {
        *cached = scene_object_subobject_resolve(record, *source, 0);
        return *cached;
    }
    return *cached;
}

/* SceneObjectIndexedSecondaryHandleGetOrCreate (FUN_00408810). */
/* Port model of 0x00408810 (Building.c, building_model.c). */
void* scene_object_indexed_secondary_handle_get_or_create(void* record,
                                                           int index)
{
    void** source;
    void** cached;
    if (record == NULL) return NULL;
    source = (void**)((char*)record + 0x18);
    if (*source == NULL) return NULL;
    cached = (void**)((char*)record + 0x50) + index;
    if (*cached == NULL)
        *cached = scene_object_subobject_resolve(record, *source, index);
    return *cached;
}

/* Port model of 0x00408850 (Building.c, building_model.c). */
uint32_t scene_object_flags_get(const void* scene_object)
{
    uint32_t flags;
    __builtin_memcpy(&flags, (const char*)scene_object + 0x70,
                     sizeof(flags));
    return flags;
}

// AnimationLoopTimeAdjust (FUN_00408860) shares the counted scene-time
// adjustment, then refreshes and clears the optional loop-time latch.
/* Port model of 0x00408860 (Building.c, building_model.c). */
float animation_loop_time_adjust(void* record, float argument,
                                 uint32_t count)
{
    uint32_t* flags;
    void* loop_frame;
    float value;
    if (record == NULL) return 0.0f;
    value = renderer_object_timing_value_adjust_by_count(record, argument,
                                                          count);
    flags = (uint32_t*)((char*)record + 0x148);
    if ((*flags & 2u) != 0u) {
        loop_frame = *(void**)((char*)record + 0x7c);
        if (loop_frame != NULL && s_scene_timing_count_get_hook != NULL &&
            s_animation_object_time_update_hook != NULL) {
            float scale = *(float*)((char*)record + 0x80);
            int duration = s_scene_timing_count_get_hook(
                loop_frame, s_scene_timing_hooks_context);
            s_animation_object_time_update_hook(
                loop_frame, (float)duration * scale,
                s_animation_object_hooks_context);
        }
        *flags &= ~2u;
    }
    return value;
}

/* Port model of 0x00408930 (Building.c, building_model.c). */
double constant_zero_float_get(void)
{
    return (double)s_zero_float; /* corpus: fld DWORD ds:0049e084 / ret */
}

/* EntityPayloadTeardown (FUN_00408940): set state to -1, then release the
 * optional secondary and primary scene handles through the owner hook. */
/* Port model of 0x00408940 (Building.c, building_model.c). */
void entity_payload_teardown(EntitySlot* e)
{
    if (e == NULL) {
        return;
    }
    entity_payload_state_set(e, -1);
    if (e->payload_aux_handle_7c != NULL
        && s_payload_scene_release_hook != NULL) {
        s_payload_scene_release_hook(e->payload_aux_handle_7c);
    }
    e->payload_aux_handle_7c = NULL;
    entity_payload_release_subslots(e);
}

/* TaskFlagBitQuery (FUN_00408970, findings 366/465). */
/* Port model of 0x00408970 (Building.c, building_model.c). */
uint32_t task_flag_bit_query(const void* task, uint32_t bit, int suppress)
{
    const uint32_t mask = 1u << (bit & 0x1fu);
    uint32_t flags;
    if (task == NULL) {
        return 0u;
    }
    __builtin_memcpy(&flags, (const char*)task + 0x98, sizeof(flags));
    if (suppress != 0 && (flags & mask) != 0) {
        return 0;
    }
    return flags & mask;
}

/* EntityPayloadStateSet (FUN_004089a0): store the payload state and rebuild
 * its scene slot cache. */
/* Port model of 0x004089a0 (Building.c, building_model.c). */
void entity_payload_state_set(EntitySlot* e, int state)
{
    if (e == NULL) {
        return;
    }
    e->payload_state_8c = state;
    entity_scene_slot_table_rebuild(e, 0);
}

/* The pair band's shared pose apply: position (x, y, 0) plus the corpus's
 * constant (0, 0, -1) orientation, through the ported primitives. */
static void pair_handle_set_pose(void* h, float x, float y)
{
    cam_set_pos((CameraHandle*)h, x, y, 0.0f);
    const float up[3] = { 0.0f, 0.0f, -1.0f };
    cam_set_facing((CameraHandle*)h, up);
}
#endif /* LRR_BUILDING_MODEL_PORT_NAMES */
