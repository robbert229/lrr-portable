// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-040 SelectPlace.c's port tail (finding 633): the port's models of
 * the unit's functions and the state the port's other files read.
 * Unannotated: the original-shape unit is selectplace.c (ADR 0005). In the
 * matching build the unit defines the port names its callers use
 * (selectplace_link_names.h), so this file compiles its own out there.
 * Relocated by tools/relocate_unit.py (ADR 0006); body hashes:
 * docs/coverage/relocations/GAME-040.csv. */
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
#include "record_for_placement_footprint.hpp"
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include "scene_resources.hpp"
#include "scene_pool.hpp"
#include "surface.hpp"
#include "model.hpp"
#include "../game/ai.hpp"
#include "../game/entity.hpp"
#include "../game/level.hpp"
#include "view_for_placement_footprint.hpp"
#include "../game/port_loop_ports.hpp"

/* SelectPlace_Create (00463f00) opens this object (finding 611): its
 * pool entries 0.0f, 1.0f and -1.0f (0049e718-0049e720) precede
 * 004641c0's. Moved here from info_track_overlay.cpp. */
static void scene_quad_grid_vertices_build(uint32_t quad, float vertices[4][9]);

/* The port's model of SelectPlace_Create (0x00463f00; selectplace.c). */
SceneQuadGridRecord* scene_quad_grid_emit(
    void* scene, uint32_t parameter, const SceneQuadGridEmitHooks* hooks)
{
    static const uint32_t base_indices[6] = { 0u, 1u, 3u, 1u, 2u, 3u };
    uint32_t indices[30];
    uint16_t index_words[60];
    SceneQuadGridRecord* record;
    uint32_t face;
    if (hooks == NULL || hooks->mesh_create == NULL ||
        hooks->mesh_release == NULL ||
        hooks->face_vertices_emit == NULL ||
        hooks->face_texture_bind_by_mode == NULL ||
        hooks->scene_render_lock_toggle == NULL)
        return NULL;
    record = (SceneQuadGridRecord*)engine_heap_alloc(
        (long)sizeof(*record));
    if (record == NULL) return NULL;
    record->mesh = NULL;
    record->parameter = parameter;
    record->mesh = hooks->mesh_create(hooks->context, scene, 3);
    if (record->mesh == NULL) {
        engine_heap_free(record);
        return NULL;
    }
    for (face = 0; face < 5u; ++face) {
        uint32_t i;
        for (i = 0; i < 6u; ++i)
            indices[face * 6u + i] = base_indices[i] + face * 4u;
    }
    for (face = 0; face < 10u; ++face) {
        uint32_t quad;
        uint32_t index;
        int group;
        /* FUN_00463f00 appends a 20-vertex record for each face group before
         * filling that group's five quads. The retail index pointer is a
         * dword table; the in-memory pool adapter consumes the low word of
         * each dword through its two-word source stride. */
        for (index = 0u; index < 30u; ++index) {
            index_words[index * 2u] = (uint16_t)indices[index];
            index_words[index * 2u + 1u] = 0u;
        }
        if (hooks->mesh_record_allocate_or_append != NULL) {
            group = hooks->mesh_record_allocate_or_append(
                hooks->context, record->mesh, 20u, 10u, 3u, indices);
        }
        else {
            group = mesh_record_allocate_or_append(
                record->mesh, 20u, 10u, 3u, index_words);
        }
        /* A fresh mesh must append in face order; every later write uses
         * that group index. Reject stale or incomplete adapter storage. */
        if (group != (int)face) {
            scene_quad_grid_release(record, hooks);
            return NULL;
        }
        for (quad = 0; quad < 5u; ++quad) {
            float vertices[4][9];
            scene_quad_grid_vertices_build(quad, vertices);
            if (!hooks->face_vertices_emit(hooks->context, record->mesh,
                                           face, quad, vertices)) {
                scene_quad_grid_release(record, hooks);
                return NULL;
            }
        }
        if (!hooks->face_texture_bind_by_mode(hooks->context, record->mesh,
                                              face, 3u)) {
            scene_quad_grid_release(record, hooks);
            return NULL;
        }
    }
    if (!hooks->scene_render_lock_toggle(hooks->context, record->mesh, 1)) {
        scene_quad_grid_release(record, hooks);
        return NULL;
    }
    return record;
}

static void scene_quad_grid_vertices_build(uint32_t quad,
                                           float vertices[4][9])
{
    static const float position_components[4][2] = {
        { 0.0f, 0.0f }, { 1.0f, 0.0f },
        { 1.0f, 1.0f }, { 0.0f, 1.0f }
    };
    static const float face_normals[5][3] = {
        { 0.0f, 0.0f, -1.0f },
        { 0.0f, 1.0f, 0.0f },
        { 1.0f, 0.0f, 0.0f },
        { 0.0f, -1.0f, 0.0f },
        { -1.0f, 0.0f, 0.0f }
    };
    uint32_t vertex;
    uint32_t color_word = 0xffffffffu;

    /* The output record is three indeterminate words, normal xyz, the two
     * unit-square components in y,x order, then the opaque word. Clear only
     * the three untouched words on the portable path. */
    memset(vertices, 0, 4u * 9u * sizeof(float));
    for (vertex = 0u; vertex < 4u; ++vertex) {
        vertices[vertex][3] = face_normals[quad][0];
        vertices[vertex][4] = face_normals[quad][1];
        vertices[vertex][5] = face_normals[quad][2];
        vertices[vertex][6] = position_components[vertex][0];
        vertices[vertex][7] = position_components[vertex][1];
        memcpy(&vertices[vertex][8], &color_word, sizeof(color_word));
    }
}

/* Original-ABI adapter (ADR 0005 step 6) for GAME-021's lego_unit.c: SelectPlace_Create (00463f00) takes the root and a size; the port's entry takes a hook table. */
/* In the matching build selectplace.c serves this name (finding 633). */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (scene_quad_grid_emit)
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
extern "C" void* select_place_create_adapter(void* root, float size)
{
    uint32_t bits;
    memcpy(&bits, &size, sizeof(bits));
    return scene_quad_grid_emit(root, bits, NULL);
}
#endif

/* The port's model of selectPlaceShape (005013c8); selectplace.c owns the
 * original's. */
int s_rot_pairs[ROT_PAIR_SLOTS * 2];

/* The port's model of SelectPlace_TransformShape (0x00464110); selectplace.c
 * serves this name in the matching build (finding 633). */
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
void* rotated_pairs_scratch_fill(const int* offset_pair, const int* source,
                                 int count, int rotation)
{
    if (count > ROT_PAIR_SLOTS) {
        count = ROT_PAIR_SLOTS; /* deviation: the corpus fills blindly */
    }
    for (int n = 0; n < count; ++n) {
        const int t = rotation + 0x8000; /* the corpus's sign fold */
        const int u = t >> 31;
        const int sel = (((t ^ u) - u & 3) ^ u) - u;
        int x = source[2 * n];
        int y = source[2 * n + 1];
        switch (sel) {
        case 1: { const int h = x; x = -y; y = -h; } break;
        case 2:  x = -x; y = -y; break;
        case 3: { const int h = x; x = y; y = -h; } break;
        default: break; /* identity (negative folds land here) */
        }
        s_rot_pairs[2 * n]     = x + offset_pair[0];
        s_rot_pairs[2 * n + 1] = y + offset_pair[1];
    }
    return s_rot_pairs;
}
#endif

#if defined(_MSC_VER) && _MSC_VER < 1200
/* selectplace.c serves surface_visibility_set (finding 633). */
#elif defined(_MSC_VER) && defined(_M_IX86)
static __declspec(naked) void surface_visibility_set_target(
    const void* record, uint32_t flag)
{
    __asm {
        mov ecx, dword ptr [esp + 4]
        mov eax, dword ptr [esp + 8]
        push eax
        mov edx, dword ptr [ecx]
        push edx
        call scene_render_lock_toggle
        add esp, 8
        ret
    }
}
#else
static void surface_visibility_set_target(const void* record, uint32_t flag)
{
    scene_render_lock_toggle(rec_ptr(record, 0x00), (int)flag);
}
#endif

/* SurfaceVisibilitySet: owned software meshes use their real visibility;
 * other records retain the target's record +0 handle/scene operation. The
 * port's model of SelectPlace_Hide (0x00464720); selectplace.c serves this
 * name in the matching build (finding 633). */
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
void surface_visibility_set(const void* record, uint32_t flag)
{
    SurfaceVisibilityAdapter adapter = s_surface_visibility_adapter;
    void* context = s_surface_visibility_context;
    if (adapter != NULL && adapter(context, record, flag) != 0) return;
    surface_visibility_set_target(record, flag);
}
#endif
