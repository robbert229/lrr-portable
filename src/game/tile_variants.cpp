// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-011 GAME-011: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/GAME-011.csv. */
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
#include <stdint.h>
#include <limits.h>
#include "game.hpp"
#include "gods98/block.hpp"
#include "gods98/engine.hpp"
#include "gods98/media.hpp"
#include "effects.hpp"
#include "gods98/message.hpp"
#include "gods98/simtick.hpp"
#include "gods98/camera.hpp"
#include "terrain_grid.hpp"
#include "type_config_ports.hpp"
#include "cmd.hpp"
#include "upgrades.hpp"
#include "spawn.hpp"
#include "port_loop_ports.hpp"
#include "frontend.hpp"
#include "objective.hpp"
#include "util.hpp"
#include "gods98/flic.hpp"
#include "select.hpp"
#include "scene_render.hpp"
#include "tutorial_hud_ports.hpp"
#include "weapon.hpp"
#include "../gods98/input.hpp"
#include <errno.h>
#include <locale.h>
#include "record_shared.hpp"
#include "gods98/view_for_tile_variants.hpp"
#include "util_for_tile_variants.hpp"

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static uint32_t cells_dword(const void* cells, size_t off);
void scene_rotation_flags_apply(void* object, float projection_scale, const float first[3], const float second[3], const float third[3], float scale, float uniform_scale, float child_value, uint8_t flags);
static float scene_frame_vector_length(const float vector[3]);

/* LoadTileVariants (FUN_0040b310): allocate a 12-byte width/height/handle
 * owner, then call TextureSurfaceCreate(mode 1, null output, mode data 0)
 * with `%s%0.1x%0.1x.bmp` for every row-major cell. The result is the
 * two-interface TextureRelease record (or its portable host-record form). */
// FUNCTION: LEGORR 0x0040B310
TerrainCellResourceGrid* terrain_cell_resource_grid_load(
    const char* bitmap_prefix, uint32_t width, uint32_t height)
{
    TerrainCellResourceGrid* grid;
    size_t count;
    uint32_t y;
    if (bitmap_prefix == NULL ||
        (width != 0u && (size_t)height > ((size_t)-1) / (size_t)width))
        return NULL;
    grid = (TerrainCellResourceGrid*)engine_heap_alloc(
        (long)sizeof(TerrainCellResourceGrid));
    if (grid == NULL) return NULL;
    memset(grid, 0, sizeof(*grid));
    count = (size_t)width * (size_t)height;
    if (count > ((size_t)-1) / sizeof(void*)) {
        engine_heap_free(grid);
        return NULL;
    }
    if (count * sizeof(void*) > (size_t)0x7fffffffu) {
        engine_heap_free(grid);
        return NULL;
    }
    grid->handles = (void**)engine_heap_alloc(
        (long)(count * sizeof(void*)));
    if (grid->handles == NULL) {
        engine_heap_free(grid);
        return NULL;
    }
    memset(grid->handles, 0, count * sizeof(void*));
    grid->width = width;
    grid->height = height;
    for (y = 0u; y < height; ++y) {
        uint32_t x;
        for (x = 0u; x < width; ++x) {
            char path[1024];
            size_t prefix_length = strlen(bitmap_prefix);
            int length;
            if (prefix_length + 21u > sizeof(path)) continue;
            length = sprintf(path + prefix_length, "%x%x.bmp",
                             (unsigned int)x, (unsigned int)y);
            if (length < 0) continue;
            memcpy(path, bitmap_prefix, prefix_length);
            grid->handles[(size_t)y * width + x] =
                texture_surface_create(path, 1u, NULL, 0u);
        }
    }
    return grid;
}

/* TerrainCellResourceGridDispose (FUN_0040b410): release each cell's
 * resource handle, then release the handle table and grid owner. */
// FUNCTION: LEGORR 0x0040B410
void terrain_cell_resource_grid_dispose(TerrainCellResourceGrid* grid)
{
    size_t i, count;
    if (grid == NULL) return;
    if (grid->height != 0u
        && (size_t)grid->width > ((size_t)-1) / (size_t)grid->height) {
        engine_heap_free(grid->handles);
        engine_heap_free(grid);
        return;
    }
    count = (size_t)grid->width * (size_t)grid->height;
    if (grid->handles != NULL) {
        for (i = 0; i < count; ++i) {
            if (grid->handles[i] != NULL) {
                texture_release_record(
                    (TextureReleaseRecord*)grid->handles[i]);
            }
        }
    }
    engine_heap_free(grid->handles);
    engine_heap_free(grid);
}

// FUNCTION: LEGORR 0x0040b470
float projected_quad_screen_area_calculate(
    const ViewProjectionAdapter* adapter, const float* quad_xyz,
    const float bounds_xy[2], int disable_visibility_clip)
{
    float clip[4][4];
    float screen[4][3];
    float first_area;
    float second_area;
    float visibility = 1.0f;
    int i;
    if (adapter == NULL || adapter->transform_point == NULL ||
        quad_xyz == NULL || bounds_xy == NULL) return 0.0f;
    for (i = 0; i < 4; ++i) {
        adapter->transform_point(adapter->context, clip[i], quad_xyz + i * 3);
        screen[i][0] = clip[i][0] / clip[i][3];
        screen[i][1] = clip[i][1] / clip[i][3];
        /* The target writes only projected x/y into these 3-word points;
         * SurfaceTriangleAreaCalculate consumes them as z=0 screen points. */
        screen[i][2] = 0.0f;
        if (!disable_visibility_clip &&
            (screen[i][0] < 0.0f || screen[i][1] < 0.0f ||
             screen[i][0] > bounds_xy[0] || screen[i][1] > bounds_xy[1] ||
             clip[i][2] < 0.0f || clip[i][2] > clip[i][3])) {
            visibility -= 0.25f;
        }
    }
    if (visibility <= 0.0f) return 0.0f;
    first_area = surface_triangle_area_calculate(
        screen[2], screen[1], screen[0], 0);
    second_area = surface_triangle_area_calculate(
        screen[0], screen[3], screen[2], 0);
    return (second_area + first_area) * visibility;
}

/* TileModelEntityRegister (FUN_0040b5f0): allocate two model slots plus two
 * 100-word interface arrays, load the no-texture models, scale and configure
 * them, then copy/reverse the second model's UVs. */
// FUNCTION: LEGORR 0x0040B5F0
TileModelEntityRegistration* tile_model_entity_register(
    void* manager, const char* first_entry, const char* second_entry,
    float scale_percent, int parameter,
    const TileModelEntityRegistrationHooks* hooks)
{
    TileModelEntityRegistration* registration;
    const char* names[2];
    unsigned int model_index;
    if (hooks == NULL || hooks->model_load == NULL ||
        hooks->model_release == NULL || first_entry == NULL)
        return NULL;
    names[0] = first_entry;
    names[1] = second_entry != NULL ? second_entry : first_entry;
    registration = (TileModelEntityRegistration*)calloc(
        1, sizeof(TileModelEntityRegistration));
    if (registration == NULL) return NULL;
    registration->scale_integer = (int32_t)scale_percent;
    registration->registration_flags = 0u;
    registration->parameter = parameter;
    for (model_index = 0; model_index < 2u; ++model_index) {
        registration->models[model_index] = hooks->model_load(
            hooks->context, manager, (int)model_index, names[model_index], 1);
        if (registration->models[model_index] == NULL && model_index == 1u)
            registration->models[model_index] = hooks->model_load(
                hooks->context, manager, 1, first_entry, 1);
        if (registration->models[model_index] == NULL) {
            tile_model_entity_registration_release(registration, hooks);
            return NULL;
        }
    }
    for (model_index = 0; model_index < 2u; ++model_index) {
        void* model = registration->models[model_index];
        float* uv_records = NULL;
        int count = 0;
        unsigned int i;
        float scale = scale_percent * 0.01f;
        if (hooks->model_scale != NULL)
            hooks->model_scale(hooks->context, model, scale, scale, scale);
        if (hooks->model_mode_set != NULL)
            hooks->model_mode_set(hooks->context, model, 0, 1);
        if (hooks->model_mode_set != NULL)
            hooks->model_mode_set(hooks->context, model, 1, 1);
        if (hooks->mesh_uv_get != NULL)
            count = hooks->mesh_uv_get(hooks->context, model, &uv_records);
        if (count < 0) count = 0;
        if (model_index != 0u && uv_records != NULL) {
            for (i = 0; i < (unsigned int)count; ++i) {
                uv_records[i * 9u + 7u] = 1.0f - uv_records[i * 9u + 7u];
                uv_records[i * 9u + 8u] = 1.0f - uv_records[i * 9u + 8u];
            }
        }
        if (hooks->mesh_uv_set != NULL)
            hooks->mesh_uv_set(hooks->context, model, 0, count, uv_records);
        if (hooks->uv_records_release != NULL && uv_records != NULL)
            hooks->uv_records_release(hooks->context, uv_records);
    }
    return registration;
}

namespace unported_ledger_stubs {
// FUNCTION: LEGORR 0x0040b7d0
void cell_visual_channel_table_dispose(CellVisualChannelTable* table)
{
    int index;
    if (table == NULL) return;
    cell_visual_channel_table_resources_clear(table);
    for (index = 0; index < 2; index++) {
        if (table->roots[index] != NULL) {
            if (s_cell_visual_release_hook != NULL)
                s_cell_visual_release_hook(s_cell_visual_release_context,
                                           table->roots[index]);
            else
                ddiscene_object_destroy(table->roots[index]);
        }
    }
    engine_heap_free(table);
}
// FUNCTION: LEGORR 0x0040b810
void cell_visual_channel_table_resources_clear(CellVisualChannelTable* table)
{
    uint32_t channel;
    uint32_t index;
    if (table == NULL) return;
    table->resource_count = 0;
    for (channel = 0; channel < 2u; channel++) {
        for (index = 0; index < 100u; index++) {
            void* resource = table->resources[channel][index];
            if (resource != NULL) {
                if (s_cell_visual_release_hook != NULL)
                    s_cell_visual_release_hook(s_cell_visual_release_context,
                                               resource);
                else
                    ddiscene_object_destroy(resource);
            }
            table->resources[channel][index] = NULL;
        }
    }
}
} // namespace unported_ledger_stubs

/* PackedGridCellValueGet (FUN_0040b850, findings 373/451): low nibble
 * * width + high nibble indexes the cell dwords; the corpus's own NULL
 * grid guard answers 0, reproduced. */
// FUNCTION: LEGORR 0x0040b850
uint32_t packed_grid_cell_value_get(const void* grid, uint32_t packed_cell)
{
    void* cells = rec_ptr(grid, 0x08);
    if (cells == NULL) {
        return 0u;
    }
    const int32_t width = (int32_t)rec_dword(grid, 0x00);
    const int32_t index = (int32_t)(packed_cell & 0xfu) * width
        + (int32_t)((packed_cell & 0xffu) >> 4);
    return cells_dword(cells, (size_t)index * 4u);
}

/* CellVisualChannelRelay (FUN_0040b880): resolve the two root resources into
 * the current channel slot, then apply the selected and opposite corner
 * frames. The record follows the recovered dword layout on x86 while its
 * pointer-bearing banks widen naturally in the portable build. */
// FUNCTION: LEGORR 0x0040b880
void scene_cell_visual_channels_apply(SceneCellVisualRelayRecord* record,
                                      const float corner_points[12],
                                      uint32_t lookup_selector,
                                      uint32_t corner_index, float scale,
                                      float uniform_scale)
{
    static const uint8_t corner_triplets[4][3] = {
        { 3, 0, 2 }, { 0, 1, 3 }, { 1, 2, 0 }, { 2, 3, 1 }
    };
    void* scene_objects[2];
    float child_value = 0.0f;
    uint32_t child_value_bits;
    uint32_t index;
    uint32_t first_corner;
    uint32_t opposite_corner;
    if (record == NULL || corner_points == NULL) return;
    index = record->channel_cursor;
    if (index >= 100u) return;
    scene_objects[0] = entity_visual_resource_load(record->resource_roots[0]);
    scene_objects[1] = entity_visual_resource_load(record->resource_roots[1]);
    record->channel_resources[0][index] = scene_objects[0];
    record->channel_resources[1][index] = scene_objects[1];
    if (record->lookup_table != NULL &&
        record->lookup_table->entries != NULL) {
        uint32_t lookup_index =
            (lookup_selector & 0x0fu) * record->lookup_table->stride +
            ((lookup_selector & 0xffu) >> 4);
        child_value_bits = record->lookup_table->entries[lookup_index];
        memcpy(&child_value, &child_value_bits, sizeof(child_value));
    }
    first_corner = corner_index & 3u;
    opposite_corner = (first_corner + 2u) & 3u;
    scene_rotation_flags_apply(scene_objects[0], record->projection_scale,
        corner_points + corner_triplets[first_corner][0] * 3u,
        corner_points + corner_triplets[first_corner][1] * 3u,
        corner_points + corner_triplets[first_corner][2] * 3u,
        scale, uniform_scale, child_value, 0u);
    scene_rotation_flags_apply(scene_objects[1], record->projection_scale,
        corner_points + corner_triplets[opposite_corner][0] * 3u,
        corner_points + corner_triplets[opposite_corner][1] * 3u,
        corner_points + corner_triplets[opposite_corner][2] * 3u,
        scale, uniform_scale, child_value, 0u);
    record->channel_cursor = index + 1u;
}

/* SceneRotationFlagsApply (FUN_0040ba00): argument 2 is passed through to
 * SceneFrameFromTripletApply as its projection scale; the remaining scalar
 * and bit arguments retain their original stack order. */
// FUNCTION: LEGORR 0x0040BA00
void scene_rotation_flags_apply(void* object, float projection_scale,
                                const float first[3],
                                const float second[3], const float third[3],
                                float scale, float uniform_scale,
                                float child_value, uint8_t flags)
{
    unsigned int i;
    SceneRotationApplyHooks* hooks = &s_scene_rotation_apply_hooks;
    if (object == NULL || first == NULL || second == NULL || third == NULL)
        return;
    if (hooks->frame_transform_apply != NULL) {
        hooks->frame_transform_apply(hooks->context, object, projection_scale,
                                     first, second, third);
    } else {
        /* The renderer's projection-matrix operation is not exposed by the
         * portable scene interface; preserve the available frame update. */
        scene_frame_from_triplet_apply(object, projection_scale,
                                       first, second, third);
    }
    if (hooks->apply != NULL)
        hooks->apply(hooks->context, object, 0x473b40u, 0, 0, 0, 0, 0);
    i = 0u;
    while (hooks->child_count != NULL &&
           i < hooks->child_count(hooks->context, object)) {
        if (hooks->apply != NULL)
            hooks->apply(hooks->context, object, 0x474da0u, i,
                         child_value, 0, 0, 0);
        ++i;
    }
    if (hooks->apply == NULL) return;
    hooks->apply(hooks->context, object, 0x4755e0u, 1,
                 -0.02f, -0.02f, 0, 0);
    hooks->apply(hooks->context, object, 0x4755b0u, 1,
                 1.015f, 1.015f, scale, 0);
    hooks->apply(hooks->context, object, 0x4750b0u, 0,
                 uniform_scale, uniform_scale, uniform_scale, 1.0f);
    if (flags & 1u) hooks->apply(hooks->context, object, 0x475580u, 1,
                                 1, 0, 0, 1.57079637f);
    if (flags & 2u) hooks->apply(hooks->context, object, 0x475580u, 1,
                                 1, 0, 0, -1.57079637f);
    if (flags & 4u) hooks->apply(hooks->context, object, 0x475580u, 1,
                                 0, 1, 0, 1.57079637f);
    if (flags & 8u) hooks->apply(hooks->context, object, 0x475580u, 1,
                                 0, 1, 0, -1.57079637f);
    if (flags & 0x10u) hooks->apply(hooks->context, object, 0x475580u, 1,
                                   0, 0, 1, 1.57079637f);
    if (flags & 0x20u) hooks->apply(hooks->context, object, 0x475580u, 1,
                                   0, 0, 1, -1.57079637f);
    if (flags & 0x80u) hooks->apply(hooks->context, object, 0x4755e0u, 2,
                                   0, -10, 0, 0);
    if (flags & 0x40u) hooks->apply(hooks->context, object, 0x4755e0u, 2,
                                   0, 10, 0, 0);
}
// FUNCTION: LEGORR 0x0040bb90
void scene_frame_from_triplet_apply(void* object, float projection_scale,
                                   const float first[3],
                                   const float second[3],
                                   const float third[3])
{
    float forward[3];
    float normal[3];
    float transformed_second[3];
    float transformed_third[3];
    float projection[16];
    float forward_length, normal_length;
    float projection_height;
    int i;
    if (object == NULL || first == NULL || second == NULL || third == NULL)
        return;
    for (i = 0; i < 3; ++i) forward[i] = second[i] - first[i];
    normal[0] = (third[2] - second[2]) * forward[1] -
                (third[1] - second[1]) * forward[2];
    normal[1] = (third[0] - second[0]) * forward[2] -
                (third[2] - second[2]) * forward[0];
    normal[2] = (third[1] - second[1]) * forward[0] -
                (third[0] - second[0]) * forward[1];
    forward_length = scene_frame_vector_length(forward);
    normal_length = scene_frame_vector_length(normal);
    projection_height = forward[1];
    for (i = 0; i < 3; ++i) {
        forward[i] /= forward_length;
        normal[i] = -normal[i] / normal_length;
    }
    if (s_scene_frame_apply_hooks.orientation_set != NULL)
        s_scene_frame_apply_hooks.orientation_set(
            s_scene_frame_apply_hooks.context, object, 0, normal, forward);
    if (s_scene_frame_apply_hooks.position_set != NULL)
        s_scene_frame_apply_hooks.position_set(
            s_scene_frame_apply_hooks.context, object, 0, first);
    memcpy(transformed_second, second, sizeof(transformed_second));
    memcpy(transformed_third, third, sizeof(transformed_third));
    if (s_scene_frame_apply_hooks.point_transform != NULL) {
        s_scene_frame_apply_hooks.point_transform(
            s_scene_frame_apply_hooks.context, object, transformed_second,
            second);
        s_scene_frame_apply_hooks.point_transform(
            s_scene_frame_apply_hooks.context, object, transformed_third,
            third);
    }
    perspective_projection_matrix_set(
        projection, projection_scale, projection_scale, projection_height,
        transformed_third[0], transformed_third[2]);
    if (s_scene_frame_apply_hooks.projection_set != NULL)
        s_scene_frame_apply_hooks.projection_set(
            s_scene_frame_apply_hooks.context, object, 1, projection);
}

/* BuildPerspectiveProjectionMatrix (FUN_0040bd60): zero the matrix, raise
 * m[10]/m[15], then the three quotients — the corpus's m[1] multiplies
 * p5/p4 by p4/p1, not p5/p1. */
// FUNCTION: LEGORR 0x0040bd60
void perspective_projection_matrix_set(float* out, float p1, float p2,
                                       float p3, float p4, float p5)
{
    out[3] = 0.0f;
    out[2] = 0.0f;
    out[7] = 0.0f;
    out[6] = 0.0f;
    out[4] = 0.0f;
    out[11] = 0.0f;
    out[9] = 0.0f;
    out[8] = 0.0f;
    out[14] = 0.0f;
    out[13] = 0.0f;
    out[12] = 0.0f;
    out[15] = 1.0f;
    out[10] = 1.0f;
    out[0] = p4 / p1;
    out[5] = p3 / p2;
    out[1] = (p5 / p4) * (p4 / p1);
}
static float scene_frame_vector_length(const float vector[3])
{
    return (float)sqrt((double)(vector[0] * vector[0] +
                                 vector[1] * vector[1] +
                                 vector[2] * vector[2]));
}

/* Contents of a native cell array: memcpy read (the corpus's cell
 * dwords can sit unaligned after a stride change). */
static uint32_t cells_dword(const void* cells, size_t off)
{
    uint32_t v = 0;
    if (cells != NULL) {
        __builtin_memcpy(&v, (const char*)cells + off, sizeof(v));
    }
    return v;
}
