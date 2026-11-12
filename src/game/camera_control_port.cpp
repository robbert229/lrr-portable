// SPDX-License-Identifier: LGPL-3.0-or-later
/* The port's camera (port tail, ADR 0006 decision 4; finding 587).
 *
 * GAME-022, the original's camera unit, is rebuilt in original shape in
 * camera_control.c. These are the port's models of its functions, under the
 * port's names, on the port's ViewContext, the engine's camera controller
 * buffer and their hooks: native callers and tests use them. Their
 * annotations moved to camera_control.c; each keeps a "Port model of"
 * comment with the original's address. Relocated by tools/relocate_unit.py
 * (body hashes: docs/coverage/relocations/GAME-022.csv). */
#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "view.hpp"
#include "record.hpp"
#include "scene_resources.hpp"
#include "scene_pool.hpp"
#include "surface.hpp"
#include "sound.hpp"
#include "texture.hpp"
#include "heap.hpp"
#include "string.hpp"
#include "resource.hpp"
#include "crt/crt.hpp"
#include "model.hpp"
#include "../game/ai.hpp"
#include "../game/entity.hpp"
#include "../game/level.hpp"
#include "view_shared.hpp"
#include "game/util.hpp"
#include <limits.h>
#include "record_shared.hpp"
#include "gods98/view_for_camera_control.hpp"

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static void scene_object_chain_nodes_release(SceneObjectChainRecord* record);
static int object_lookup(EngineContext* ctx, int id);
static int target_blend(EngineCameraTargetRecord* record, int target_id, float* blend, float* facing);
static void view_tail(ViewContext* v, float scale);

// SceneObjectChainRecordCreate (FUN_00435b20).
/* Port model of 0x00435b20 (camera_control.c). */
SceneObjectChainRecord* scene_object_chain_record_create(void* parent,
                                                         int type)
{
    static const float direction[3] = { 0.0f, 0.0f, 1.0f };
    static const float up[3] = { 0.0f, 1.0f, 0.0f };
    SceneObjectChainRecord* record;
    SceneObjectChainHooks* hooks = &s_scene_object_chain_hooks;
    if (hooks->node_create == NULL) return NULL;
    record = (SceneObjectChainRecord*)engine_heap_alloc(
        (long)sizeof(SceneObjectChainRecord));
    if (record == NULL) return NULL;
    memset(record, 0, sizeof(*record));
    record->type = type;
    record->layer_depth = 200.0f;
    record->node[0] = hooks->node_create(hooks->context, NULL);
    if (record->node[0] == NULL) goto fail;
    if (hooks->node_position_set != NULL) {
        hooks->node_position_set(hooks->context, record->node[0],
                                 0.0f, 0.0f, -record->layer_depth);
    }
    if (hooks->node_orientation_set != NULL) {
        hooks->node_orientation_set(hooks->context, record->node[0],
                                    direction, up);
    }
    if (type == 3) {
        if (hooks->node_attach != NULL)
            hooks->node_attach(hooks->context, record->node[0], parent);
        return record;
    }
    record->node[1] = hooks->node_create(hooks->context, parent);
    if (record->node[1] == NULL) goto fail;
    record->node[2] = hooks->node_create(hooks->context, parent);
    if (record->node[2] == NULL) goto fail;
    record->node[3] = hooks->node_create(hooks->context, record->node[1]);
    if (record->node[3] == NULL) goto fail;
    if (type == 1) {
        record->node[4] = hooks->node_create(hooks->context, record->node[3]);
        if (record->node[4] != NULL && hooks->node_position_set != NULL) {
            hooks->node_position_set(hooks->context, record->node[4],
                                     0.0f, 0.0f, -80.0f);
        }
    }
    if (hooks->node_attach != NULL) {
        hooks->node_attach(hooks->context, record->node[0], record->node[3]);
    }
    return record;
fail:
    scene_object_chain_nodes_release(record);
    engine_heap_free(record);
    return NULL;
}

/* Port model of 0x00435d91 (camera_control.c). */
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void entity_b8_bit3_set_or_clear(void* record, int set)
{
    __asm {
        push ebp
        mov ebp, esp
        cmp dword ptr [ebp + 0ch], 0
        je entity_b8_bit3_clear
        mov eax, dword ptr [ebp + 8]
        mov cl, byte ptr [eax + 0b8h]
        or cl, 8
        mov edx, dword ptr [ebp + 8]
        mov byte ptr [edx + 0b8h], cl
        jmp entity_b8_bit3_done
    entity_b8_bit3_clear:
        mov eax, dword ptr [ebp + 8]
        mov cl, byte ptr [eax + 0b8h]
        and cl, 0f7h
        mov edx, dword ptr [ebp + 8]
        mov byte ptr [edx + 0b8h], cl
    entity_b8_bit3_done:
        pop ebp
        ret
    }
}
#else
void entity_b8_bit3_set_or_clear(void* record, int set)
{
    if (record == NULL) {
        return;
    }
    unsigned char* byte = (unsigned char*)record + 0xb8;
    if (set != 0) {
        *byte = (unsigned char)(*byte | 8);
    } else {
        *byte = (unsigned char)(*byte & ~8u);
    }
}
#endif

// CameraViewContextClose (FUN_00435dc8) releases the world camera first,
// conditionally releases follow and third handles, then frees the context.
/* Port model of 0x00435dc8 (camera_control.c). */
void camera_view_context_close(void* view)
{
    int mode;
    if (view == NULL) return;
    mode = *(int*)view;
    if (s_view_context_release_hook != NULL) {
        s_view_context_release_hook(s_view_context_close_context, view, 9);
        if (mode != 3) {
            s_view_context_release_hook(s_view_context_close_context,
                                        view, 10);
            s_view_context_release_hook(s_view_context_close_context,
                                        view, 12);
        }
    }
    if (s_view_context_free_hook != NULL) {
        s_view_context_free_hook(s_view_context_close_context, view);
    }
}

/* Port model of 0x00435e0e (camera_control.c). */
void camera_view_context_set(float cap, float follow_scale,
                             float pan_scale, int edge)
{
    g_view_pan_cap = cap;
    g_view_follow_scale = follow_scale;
    g_view_pan_scale = pan_scale;
    g_view_grid_edge = edge;
}
// ModelTransformCopyToSceneNode (FUN_00435e35) copies a scene node's pose
// onto the model scene object when the model record kind is 3.
/* Port model of 0x00435e35 (camera_control.c). */
void model_transform_copy_to_scene_node(void* model_record,
                                        void* source_scene_node)
{
    unsigned char* record = (unsigned char*)model_record;
    unsigned char* source = (unsigned char*)source_scene_node;
    uint32_t kind;
    void* model_scene_object;
    void* source_scene_object;
    float position[3];
    float first[3];
    float second[3];
    if (record == NULL || source == NULL) return;
    memcpy(&kind, record, sizeof(kind));
    if (kind != 3u) return;
    memcpy(&model_scene_object, record + 0x24, sizeof(model_scene_object));
    memcpy(&source_scene_object, source + 0x24,
           sizeof(source_scene_object));
    if (model_scene_object == NULL || source_scene_object == NULL) return;
    if (!ddiscene_object_position_get(source_scene_object, NULL, position))
        return;
    if (!ddiscene_object_facing_pair_get(source_scene_object, NULL, first,
                                         second)) return;
    (void)ddiscene_object_position_set(model_scene_object, NULL,
                                       position[0], position[1], position[2]);
    (void)ddiscene_object_pose_method_dispatch(
        model_scene_object, NULL, first[0], first[1], first[2],
        second[0], second[1], second[2]);
}

/* ScreenEffectRecordSet (FUN_00435ebb): copy the four effect parameters
 * around the camera tilt operation in the original record. */
/* Port model of 0x00435ebb (camera_control.c). */
void screen_effect_record_set(void* camera_record,
                              uint32_t field4, uint32_t field8,
                              uint32_t fieldc, float tilt,
                              uint32_t field10)
{
    if (camera_record == NULL) return;
    memcpy((char*)camera_record + 4, &field4, sizeof(field4));
    memcpy((char*)camera_record + 8, &field8, sizeof(field8));
    memcpy((char*)camera_record + 0x0c, &fieldc, sizeof(fieldc));
    camera_tilt_set(camera_record, tilt);
    memcpy((char*)camera_record + 0x10, &field10, sizeof(field10));
}

/* --- entity record-field leaves (see record.hpp) --------------------------- */
/* Port model of 0x00435ef4 (camera_control.c). */
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void record_fields_4_and_14_set(
    void* record, uint32_t value_a, uint32_t value_b)
{
    __asm {
        push ebp
        mov ebp, esp
        mov eax, dword ptr [ebp + 8]
        mov ecx, dword ptr [ebp + 0ch]
        mov dword ptr [eax + 4], ecx
        mov edx, dword ptr [ebp + 8]
        mov eax, dword ptr [ebp + 10h]
        mov dword ptr [edx + 14h], eax
        pop ebp
        ret
    }
}
#else
void record_fields_4_and_14_set(void* record, uint32_t value_a,
                                uint32_t value_b)
{
    if (record == NULL) {
        return;
    }
    __builtin_memcpy((char*)record + 0x04, &value_a, sizeof(value_a));
    __builtin_memcpy((char*)record + 0x14, &value_b, sizeof(value_b));
}
#endif
/* RecordField14Get (FUN_00435f0b). */
/* Port model of 0x00435f0b (camera_control.c). */
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) uint32_t record_field14_get(const void* record)
{
    __asm {
        push ebp
        mov ebp, esp
        mov eax, dword ptr [ebp + 8]
        mov eax, dword ptr [eax + 14h]
        pop ebp
        ret
    }
}
#else
uint32_t record_field14_get(const void* record)
{
    return rec_dword(record, 0x14);
}
#endif

/* CursorScreenRayContextReset (FUN_00435f16) — see view.hpp. */
/* Port model of 0x00435f16 (camera_control.c). */
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void cursor_screen_ray_context_reset(ViewContext* v)
{
    __asm {
        push ebp
        mov ebp, esp
        mov eax, dword ptr [ebp + 8]
        mov dword ptr [eax + 0x9c], 0
        pop ebp
        ret
    }
}
#else
void cursor_screen_ray_context_reset(ViewContext* v)
{
    if (v == NULL) {
        return; /* deviation: the corpus stores blindly */
    }
    v->pan_accum = 0.0f; /* the corpus's view-record +0x9c slot */
}
#endif
/* TerrainGridBorderInsetGet (FUN_00435f28): DAT_005560ac's getter. */
/* Port model of 0x00435f28 (camera_control.c). */
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) int view_grid_edge_get(void)
{
    __asm {
        push ebp
        mov ebp, esp
        mov eax, dword ptr [g_view_grid_edge]
        pop ebp
        ret
    }
}
#else
int view_grid_edge_get(void)
{
    return g_view_grid_edge;
}
#endif

/* Port model of 0x00435f32 (camera_control.c). */
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void entity_fields_a0_a4_store_a8_clear(
    void* record, uint32_t value_a, uint32_t value_b)
{
    __asm {
        push ebp
        mov ebp, esp
        mov eax, dword ptr [ebp + 8]
        mov ecx, dword ptr [ebp + 0ch]
        mov dword ptr [eax + 0a0h], ecx
        mov edx, dword ptr [ebp + 8]
        mov eax, dword ptr [ebp + 10h]
        mov dword ptr [edx + 0a4h], eax
        mov ecx, dword ptr [ebp + 8]
        mov dword ptr [ecx + 0a8h], 0
        pop ebp
        ret
    }
}
#else
void entity_fields_a0_a4_store_a8_clear(void* record, uint32_t value_a,
                                        uint32_t value_b)
{
    if (record == NULL) {
        return;
    }
    const uint32_t zero = 0u;
    __builtin_memcpy((char*)record + 0xa0, &value_a, sizeof(value_a));
    __builtin_memcpy((char*)record + 0xa4, &value_b, sizeof(value_b));
    __builtin_memcpy((char*)record + 0xa8, &zero, sizeof(zero));
}
#endif

/* Port model of 0x00435f5c (camera_control.c). */
void view_tick(ViewContext* v, EngineContext* ctx, float dt, float scale)
{
    if (v == NULL || dt <= 0.0f) {
        return;
    }
    EngineState* st = ctx != NULL ? ctx->state_block : NULL;
    (void)st;
    cam_offset(&v->cam_world, -v->velocity[0], -v->velocity[1],
               -v->velocity[2]);

    if (v->mode == 1) {
        /* Follow: keep the follow camera at the offset, then clamp it at the
         * world grid edges (corpus: local_8/local_c vs DAT_005560ac). */
        vec_scale(v->offset, v->offset, g_view_follow_scale);
        cam_offset(&v->cam_follow, v->offset[0], v->offset[1], v->offset[2]);
        float pos[3], fac[3];
        cam_get_pos(&v->cam_follow, pos);
        cam_get_facing(&v->cam_follow, fac);
        (void)fac;
        int gx = 0, gy = 0;
        if (!engine_context_grid_pick(ctx, pos, &gx, &gy)) {
            cam_offset(&v->cam_follow, -v->offset[0], -v->offset[1],
                       -v->offset[2]);
        } else {
            const int edge = g_view_grid_edge;
            if (gx < edge || engine_level_width(ctx) - 1 - edge <= gx) {
                cam_offset(&v->cam_follow, -v->offset[0], 0, 0);
            }
            if (gy < edge || engine_level_height(ctx) - 1 - edge <= gy) {
                cam_offset(&v->cam_follow, 0, -v->offset[1], 0);
            }
            float h = terrain_height(ctx, pos[0], pos[1]);
            cam_set_pos(&v->cam_facing, pos[0], pos[1], h);
            cam_set_facing(&v->cam_facing, fac);
            float distance = v->follow_span;
            if (v->controller != NULL)
                memcpy(&distance, (char*)v->controller + 0x90,
                       sizeof(distance));
            float base_height = terrain_authored_surface_height_interpolate(
                ctx->level_terrain, pos[0], pos[1]);
            float z = distance - base_height;
            cam_set_pos(&v->cam_world, 0.0f, 0.0f, -z);
        }
        view_tail(v, scale);
        return;
    }

    if (v->mode == 2) {
        /* Scripted / lagged follow: interpolate toward the target with
         * k = |d|/(|d|+A) once an object target exists (object_lookup). */
        if (v->follow_id == 0) {
            if (ctx != NULL) {
                ctx->state &= ~0x20000u;
            }
            view_tail(v, scale);
            return;
        }
        int target = object_lookup(ctx, v->follow_id);
        if (target == 0) {
            view_tail(v, scale);
            return;
        }
        if (ctx != NULL) {
            ctx->state |= 0x20000u;
        }
        float goal3[3], facing[3];
        cam_get_pos(&v->cam_world, goal3);
        cam_get_facing(&v->cam_world, facing);
        float dist = vec_len(goal3);
        float a = (float)v->a3;
        float k = a <= 0.0f ? 0.0f : dist / (dist + a); (void)k;
        /* ...full corpus interpolations follow when the object layer lands */
        view_tail(v, scale);
        return;
    }

    if (v->mode == 3) {
        /* Free/cruise: ease the camera position and facing toward the
         * velocity target.  The factor is 4 / dt, so smaller frame deltas
         * keep the camera closer to the target while larger deltas retain
         * more of the current camera state. */
        float pos[3], facing[3];
        float target_pos[3], target_facing[3];
        float mixed_pos[3], mixed_facing[3];
        float distance, facing_dot, factor, inverse_factor;
        int target_dirty;
        cam_get_pos(&v->cam_world, pos);
        cam_get_facing(&v->cam_world, facing);
        memcpy(target_pos, pos, sizeof(target_pos));
        memcpy(target_facing, facing, sizeof(target_facing));
        if ((v->target_record == NULL &&
             s_view_target_blend_hook == NULL) || dt == 0.0f ||
            target_blend((EngineCameraTargetRecord*)v->target_record,
                         v->target_record != NULL ? v->a5 : v->follow_id,
                         target_pos,
                         target_facing) == 0) {
            view_tail(v, scale);
            return;
        }

        target_dirty = (v->flags & 0x10u) != 0;
        distance = vec_dist(target_pos, pos);
        facing_dot = vec_dot(target_facing, facing);
        if (!target_dirty || distance > 100.0f || facing_dot < 0.0f) {
            memcpy(pos, target_pos, sizeof(pos));
            memcpy(facing, target_facing, sizeof(facing));
        }

        /* The reference normalizes both facing vectors after applying its
         * dirty-target fallback, then blends the current vector toward the
         * target vector. */
        vec_normalize(facing);
        vec_normalize(target_facing);
        factor = 4.0f / dt;
        inverse_factor = 1.0f / (factor + 1.0f);
        vec_scale(mixed_facing, facing, factor);
        vec_add(mixed_facing, mixed_facing, target_facing);
        vec_scale(mixed_facing, mixed_facing, inverse_factor);
        vec_scale(mixed_pos, pos, factor);
        vec_add(mixed_pos, mixed_pos, target_pos);
        vec_scale(mixed_pos, mixed_pos, inverse_factor);

        if (distance > 0.01f)
            cam_set_pos(&v->cam_world, mixed_pos[0], mixed_pos[1],
                        mixed_pos[2]);
        if (facing_dot < 0.99999f)
            cam_set_facing(&v->cam_world, mixed_facing);
        v->flags |= 0x10;
        view_tail(v, scale);
        return;
    }

    view_tail(v, scale);
}

/* Port model of 0x00436831 (camera_control.c). */
void camera_tilt_bounds_set(void* camera, float lo, float hi)
{
    if (camera == NULL) {
        return; /* deviation: the corpus writes blindly */
    }
    uint8_t* flags = (uint8_t*)((char*)camera + CAM_FLAGS_OFF);
    *flags = (uint8_t)(*flags | 1u);
    __builtin_memcpy((char*)camera + 0x7c, &lo, sizeof(lo));
    __builtin_memcpy((char*)camera + 0x80, &hi, sizeof(hi));
}

/* CameraSetTilt (FUN_00436860): honor the optional bound unless override bit
 * 8 is set, store +0x78, reset the scene object's pose, then apply the
 * negative X-axis rotation. The DDiSceneObjectPoseMethodDispatch leaf is an
 * injectable renderer seam because its owning scene interface varies by
 * backend. */
/* Port model of 0x00436860 (camera_control.c). */
void camera_tilt_set(void* camera, float value)
{
    uint8_t flags;
    float lo, hi;
    void* scene_object;
    void* pose_context;
    if (camera == NULL) {
        return;
    }
    flags = *(uint8_t*)((char*)camera + CAM_FLAGS_OFF);
    __builtin_memcpy(&lo, (char*)camera + 0x7c, sizeof(lo));
    __builtin_memcpy(&hi, (char*)camera + 0x80, sizeof(hi));
    if ((flags & 1u) != 0u && (flags & 8u) == 0u &&
        (value < lo || value > hi)) {
        return;
    }
    __builtin_memcpy((char*)camera + 0x78, &value, sizeof(value));
    scene_object = *(void**)((char*)camera + 0x30);
    pose_context = *(void**)((char*)camera + 0x28);
    if (scene_object == NULL) {
        return;
    }
    if (s_camera_pose_dispatch_hook != NULL) {
        s_camera_pose_dispatch_hook(scene_object, pose_context,
                                    0.0f, 0.0f, 1.0f,
                                    0.0f, 1.0f, 0.0f,
                                    s_camera_pose_dispatch_context);
    }
    render_object_rotate_axis(scene_object, 1u, 1.0f, 0.0f, 0.0f, -value);
}

/* CameraPanAxisUpdate (FUN_00436900): add the delta to the controller's
 * current +0x78 angle and route through CameraSetTilt for the same bounds and
 * scene-transform update. */
/* Port model of 0x00436900 (camera_control.c). */
void camera_pan_axis_update(void* camera, float delta)
{
    camera_tilt_set(camera,
                    delta + *(float*)((char*)camera + 0x78));
}

/* Port model of 0x00436921 (camera_control.c). */
void camera_scalar_bounds_set(void* camera, float lo, float hi)
{
    if (camera == NULL) {
        return; /* deviation: the corpus writes blindly */
    }
    uint8_t* flags = (uint8_t*)((char*)camera + CAM_FLAGS_OFF);
    *flags = (uint8_t)(*flags | 2u);
    __builtin_memcpy((char*)camera + 0x88, &lo, sizeof(lo));
    __builtin_memcpy((char*)camera + 0x8c, &hi, sizeof(hi));
}

/* CameraSetBoundedRotationValue (FUN_00436953): same optional-range and
 * override gates as the tilt setter, with a reset pose followed by a
 * negative Z-axis rotation. */
/* Port model of 0x00436953 (camera_control.c). */
void camera_rotation_set(void* camera, float value)
{
    uint8_t flags;
    float lo, hi;
    void* scene_object;
    if (camera == NULL) {
        return;
    }
    flags = *(uint8_t*)((char*)camera + CAM_FLAGS_OFF);
    __builtin_memcpy(&lo, (char*)camera + 0x88, sizeof(lo));
    __builtin_memcpy(&hi, (char*)camera + 0x8c, sizeof(hi));
    if ((flags & 2u) != 0u && (flags & 8u) == 0u &&
        (value < lo || value > hi)) {
        return;
    }
    __builtin_memcpy((char*)camera + 0x84, &value, sizeof(value));
    scene_object = *(void**)((char*)camera + 0x28);
    if (scene_object == NULL) {
        return;
    }
    if (s_camera_pose_dispatch_hook != NULL) {
        s_camera_pose_dispatch_hook(scene_object, NULL,
                                    0.0f, 0.0f, 1.0f,
                                    0.0f, 1.0f, 0.0f,
                                    s_camera_pose_dispatch_context);
    }
    render_object_rotate_axis(scene_object, 1u, 0.0f, 0.0f, 1.0f, -value);
}

/* CameraTiltAxisUpdate (FUN_004369f4): add the delta to the controller's
 * +0x84 angle and route it through camera_rotation_set (0x00436953), as
 * camera_pan_axis_update does for +0x78. Panel.c (GAME-032, finding 554) is
 * its caller. */
/* Port model of 0x004369f4 (camera_control.c). */
extern "C" void camera_tilt_axis_update(void* camera, float delta)
{
    camera_rotation_set(camera, delta + *(float*)((char*)camera + 0x84));
}

/* Port model of 0x00436a18 (camera_control.c). */
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void camera_distance_bounds_set(void* camera,
                                                 float lo, float hi)
{
    __asm {
        push ebp
        mov ebp, esp
        mov eax, dword ptr [ebp + 8]
        mov cl, byte ptr [eax + 0b8h]
        or cl, 4
        mov edx, dword ptr [ebp + 8]
        mov byte ptr [edx + 0b8h], cl
        mov eax, dword ptr [ebp + 8]
        mov ecx, dword ptr [ebp + 0ch]
        mov dword ptr [eax + 94h], ecx
        mov edx, dword ptr [ebp + 8]
        mov eax, dword ptr [ebp + 10h]
        mov dword ptr [edx + 98h], eax
        pop ebp
        ret
    }
}
#else
void camera_distance_bounds_set(void* camera, float lo, float hi)
{
    if (camera == NULL) {
        return; /* deviation: the corpus writes blindly */
    }
    uint8_t* flags = (uint8_t*)((char*)camera + CAM_FLAGS_OFF);
    *flags = (uint8_t)(*flags | 4u);
    __builtin_memcpy((char*)camera + 0x94, &lo, sizeof(lo));
    __builtin_memcpy((char*)camera + 0x98, &hi, sizeof(hi));
}
#endif

/* Port model of 0x00436a4a (camera_control.c). */
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void camera_distance_set(void* camera, float value)
{
    __asm {
        push ebp
        mov ebp, esp
        mov eax, dword ptr [ebp + 8]
        xor ecx, ecx
        mov cl, byte ptr [eax + 0b8h]
        and ecx, 4
        test ecx, ecx
        je short camera_distance_set_write
        mov edx, dword ptr [ebp + 8]
        xor eax, eax
        mov al, byte ptr [edx + 0b8h]
        and eax, 8
        test eax, eax
        jne short camera_distance_set_write
        mov ecx, dword ptr [ebp + 8]
        fld dword ptr [ebp + 0ch]
        fcomp dword ptr [ecx + 94h]
        fnstsw ax
        test ah, 1
        jne short camera_distance_set_done
        mov edx, dword ptr [ebp + 8]
        fld dword ptr [ebp + 0ch]
        fcomp dword ptr [edx + 98h]
        fnstsw ax
        test ah, 41h
        je short camera_distance_set_done
    camera_distance_set_write:
        mov eax, dword ptr [ebp + 8]
        mov ecx, dword ptr [ebp + 0ch]
        mov dword ptr [eax + 90h], ecx
    camera_distance_set_done:
        pop ebp
        ret
    }
}
#else
void camera_distance_set(void* camera, float value)
{
    if (camera == NULL) {
        return; /* deviation: the corpus writes blindly */
    }
    const uint8_t f = *(const uint8_t*)((const char*)camera + CAM_FLAGS_OFF);
    float lo = 0.0f, hi = 0.0f;
    __builtin_memcpy(&lo, (const char*)camera + 0x94, sizeof(lo));
    __builtin_memcpy(&hi, (const char*)camera + 0x98, sizeof(hi));
    if ((f & 4u) == 0u || (f & 8u) != 0u || (lo <= value && value <= hi)) {
        __builtin_memcpy((char*)camera + 0x90, &value, sizeof(value));
    }
}
#endif

/* Port model of 0x00436aa5 (camera_control.c). */
void camera_zoom_update(void* camera, float delta)
{
    if (camera == NULL) {
        return; /* deviation: the corpus reads blindly */
    }
    float cur = 0.0f;
    __builtin_memcpy(&cur, (const char*)camera + 0x90, sizeof(cur));
    camera_distance_set(camera, delta + cur);
}

/* RendererNodeRelativePositionSet (FUN_00436ac9) — see view.hpp. */
/* Port model of 0x00436ac9 (camera_control.c). */
void renderer_node_relative_position_set(const void* node, float x, float y)
{
#if defined(_WIN64) || defined(__x86_64__) || defined(__aarch64__)
    /* Native 64-bit: the handle is the module's widened pointer slot (see
     * view.hpp); a uint32_t read would truncate the pointer. */
    void* handle = NULL;
#else
    uint32_t handle = 0u;
#endif
    if (node != NULL) {
        __builtin_memcpy(&handle, (const char*)node + 0x28, sizeof handle);
    }
    cam_offset((CameraHandle*)handle, x, y, 0.0f);
}
/* SelectionRecordPointGet (FUN_00436ae9) — see view.hpp. */
/* Port model of 0x00436ae9 (camera_control.c). */
void selection_record_point_get(ViewContext* v, float out[3])
{
    if (v != NULL && out != NULL && v->mode == 1) {
        /* corpus: the view record's dword [10] camera (cam_follow) */
        cam_get_pos(&v->cam_follow, out);
    }
}
/* Port model of 0x00436b0b (camera_control.c). */
void selection_record_point_set(ViewContext* v, float x, float y)
{
    if (v != NULL && v->mode == 1) {
        /* corpus: the view record's dword [10] camera (cam_follow), z=0 */
        cam_set_pos(&v->cam_follow, x, y, 0.0f);
    }
}

/* Port model of 0x00436b33 (camera_control.c). */
int camera_get_position_with_terrain_height(ViewContext* v,
                                            EngineContext* ctx,
                                            float out[3])
{
    if (v != NULL && ctx != NULL && out != NULL && v->mode == 1) {
        cam_get_pos(&v->cam_follow, out);
        /* corpus: the terrain leaf takes the world handle; the port routes
         * it through the engine context's bound level block (deviation). */
        out[2] = terrain_height(ctx, out[0], out[1]);
    }
    return 1; /* corpus: constant success, untouched output for mode != 1 */
}

/* CameraRotationGet (FUN_00436b79): the float at +0x84 (opaque-record
 * reader; see header). */
/* Port model of 0x00436b79 (camera_control.c). */
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) float camera_rotation_get(const void* camera)
{
    __asm {
        push ebp
        mov ebp, esp
        mov eax, dword ptr [ebp + 8]
        fld dword ptr [eax + 84h]
        pop ebp
        ret
    }
}
#else
float camera_rotation_get(const void* camera)
{
    return *(const float*)((const char*)camera + 0x84);
}
#endif

/* Port model of 0x00436b87 (camera_control.c). */
void camera_pointer_pan_update(ViewContext* v, float vec[3], float amount)
{
    if (v == NULL || vec == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    vec_normalize(vec);
    v->pan_accum = amount * g_view_pan_scale + v->pan_accum;
    if (g_view_pan_cap < v->pan_accum) {
        v->pan_accum = g_view_pan_cap;
    }
    vec_scale(vec, vec, v->pan_accum * amount);
    vec_add(v->offset, v->offset, vec);
}
static void scene_object_chain_nodes_release(SceneObjectChainRecord* record)
{
    int i;
    if (record == NULL || s_scene_object_chain_hooks.node_release == NULL)
        return;
    for (i = 2; i >= 0; --i) {
        if (record->node[i] != NULL) {
            s_scene_object_chain_hooks.node_release(
                s_scene_object_chain_hooks.context, record->node[i]);
            record->node[i] = NULL;
        }
    }
}

static int object_lookup(EngineContext* ctx, int id)
{
    if (ctx != NULL && id > 0 && id <= (int)ctx->camera_target_count &&
        ctx->camera_targets[id - 1].type != 0u) return 1;
    return s_view_object_lookup_hook != NULL
        ? s_view_object_lookup_hook(id, s_view_follow_target_context) : 0;
}
static int target_blend(EngineCameraTargetRecord* record, int target_id,
                        float* blend,
                        float* facing)
{
    if (record != NULL && record->type >= 1u && record->type <= 3u &&
        record->source[record->type - 1u] != NULL &&
        blend != NULL && facing != NULL) {
        entity_follow_target_pose_get(
            (EntitySlot*)record->source[record->type - 1u], target_id,
            blend, facing);
        return 1;
    }
    if (s_view_target_blend_hook != NULL)
        return s_view_target_blend_hook(target_id,
                                        record != NULL ? (int)record->type : 0,
                                        blend, facing,
                                        s_view_follow_target_context);
    if (blend != NULL) {
        blend[0] = blend[1] = blend[2] = 0.0f;
    }
    if (facing != NULL) {
        facing[0] = facing[1] = facing[2] = 0.0f;
    }
    return g_view_test_blend;
}

static void view_tail(ViewContext* v, float scale)
{
    /* Corpus tail: when the motion timer has expired, velocity snaps to 0;
     * otherwise it decays toward the step size and the camera is shifted by
     * the negative velocity (glide-to-stop). */
    if (v->timer_num <= v->timer_acc) {
        v->velocity[0] = v->velocity[1] = v->velocity[2] = 0.0f;
    } else {
        float decay = ((float)(v->timer_num - v->timer_acc) / (float)v->timer_num)
                    * v->step;
        vec_scale(v->velocity, v->velocity, decay);
        v->timer_acc += (int)scale;
        cam_offset(&v->cam_world, -v->velocity[0], -v->velocity[1],
                   -v->velocity[2]);
    }
}


/* Original-ABI adapters (ADR 0005 step 6) for GAME-021's lego_unit.c. */

/* Camera_GetTopdownPosition (FUN_00436b33): the original takes the map;
 * the port's camera_get_position_with_terrain_height takes the engine
 * context for the terrain height. Without one it leaves the output as the
 * original does for another camera mode. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (Camera_GetTopdownPosition 00436b33 -> camera_get_position_with_terrain_height)
extern "C" int camera_topdown_position_get_adapter(void* camera, void* map, float out[3])
{
    (void)map;
    return camera_get_position_with_terrain_height((ViewContext*)camera, NULL, out);
}

/* Original-ABI adapter (ADR 0005 step 6) for GAME-021's lego_unit.c: the
 * original FUN_00435ebb (Camera_TrackObject) takes the object pointer and
 * its float parameters as floats; the port's entry above takes them as
 * 32-bit words. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (screen_effect_record_set 00435ebb)
extern "C" void screen_effect_record_set_original(void* camera,
                                                  EntitySlot* object,
                                                  float speed, float distance,
                                                  float tilt, float rotation)
{
    uint32_t speed_word;
    uint32_t distance_word;
    uint32_t rotation_word;
    memcpy(&speed_word, &speed, sizeof(speed_word));
    memcpy(&distance_word, &distance, sizeof(distance_word));
    memcpy(&rotation_word, &rotation, sizeof(rotation_word));
    screen_effect_record_set(camera, (uint32_t)(uintptr_t)object, speed_word,
                             distance_word, tilt, rotation_word);
}

/* Original-ABI adapter (ADR 0005 step 6) for GAME-021's lego_unit.c: the
 * original Camera_Shake takes two floats; the port's entry takes their
 * bits as words. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (entity_fields_a0_a4_store_a8_clear, 0x00435f32)
extern "C" void entity_fields_a0_a4_store_a8_clear_original(void* camera,
                                                            float a, float b)
{
    uint32_t bits_a;
    uint32_t bits_b;
    memcpy(&bits_a, &a, sizeof(bits_a));
    memcpy(&bits_b, &b, sizeof(bits_b));
    entity_fields_a0_a4_store_a8_clear(camera, bits_a, bits_b);
}
