// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-036 GAME-036: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/GAME-036.csv. */
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
#include <float.h>
#include <stdint.h>
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
#include "terrain_grid.hpp"
#include "cmd.hpp"
#include "select.hpp"
#include "gods98/engine.hpp"
#include "gods98/message.hpp"
#include "gods98/block.hpp"
#include "upgrades.hpp"
#include "frontend.hpp"
#include "entity_shared.hpp"
#include "util.hpp"
#include "gods98/media.hpp"
#include <time.h>
#include "frontend.h"
#include "objective.hpp"
#include "terrain_feedback.hpp"
#include "savegame.hpp"
#include "gods98/audio.hpp"
#include "gods98/simtick.hpp"
#include "gods98/flic.hpp"
#include "gods98/input.hpp"
#include "priority_config_ports.hpp"
#include "tutorial_hud_ports.hpp"
#include "ui_action_dispatch.hpp"
#include "camera_drag.hpp"
#include "screen_pick.hpp"
#include "pointer_cursor.hpp"
#include "ui_list.hpp"
#include "frontend_shared.hpp"
#include <limits.h>
#include "game.hpp"
#include "scene_render.hpp"
#include "terrain_grid_shared.hpp"
#include "heap_shared.hpp"

/* The palette globals read by FUN_0045e9e0 are adjacent float triplets in
 * the retail data section.  The game's ScreenCoordNormalizeInit divides this
 * block by 255 before the style resolver uses it; retain the original values
 * here and apply that same normalization when copying a selected triplet. */
typedef struct TerrainTilePaletteWords {
    uint32_t word[3];
} TerrainTilePaletteWords;

enum TerrainTilePaletteIndex {
    TERRAIN_TILE_PALETTE_TYPE1_PRE = 7,
    TERRAIN_TILE_PALETTE_TYPE2_PRE = 6,
    TERRAIN_TILE_PALETTE_TYPE3_PRE = 5,
    TERRAIN_TILE_PALETTE_TYPE4_PRE = 4,
    TERRAIN_TILE_PALETTE_TYPE5_PRE = 3,
    TERRAIN_TILE_PALETTE_TYPE6 = 2,
    TERRAIN_TILE_PALETTE_TYPE7 = 1,
    TERRAIN_TILE_PALETTE_TYPE8_SHARED = 20,
    TERRAIN_TILE_PALETTE_TYPE9 = 23,
    TERRAIN_TILE_PALETTE_TYPE10 = 21,
    TERRAIN_TILE_PALETTE_TYPE11 = 22,
    TERRAIN_TILE_PALETTE_DUG_TYPE1 = 12,
    TERRAIN_TILE_PALETTE_DUG_TYPE2 = 11,
    TERRAIN_TILE_PALETTE_DUG_TYPE3 = 10,
    TERRAIN_TILE_PALETTE_DUG_TYPE4 = 9,
    TERRAIN_TILE_PALETTE_DUG_TYPE5 = 8,
    TERRAIN_TILE_PALETTE_CELL_OCCUPIED = 18,
    TERRAIN_TILE_PALETTE_CELL_UNOCCUPIED = 19,
    TERRAIN_TILE_PALETTE_SPECIAL_DUG = 25,
    TERRAIN_TILE_PALETTE_FLAGS_100000 = 24
};

static const float s_snorm_pair_consts[2][7][2] = {
    {{320.0f, 40.0f},  {360.0f, 180.0f}, {360.0f, 180.0f}, {400.0f, 180.0f},
     {280.0f, 180.0f}, {220.0f, 180.0f}, {0.0f, 0.0f}},
    {{420.0f, 180.0f}, {360.0f, 180.0f}, {360.0f, 180.0f}, {400.0f, 180.0f},
     {280.0f, 180.0f}, {220.0f, 180.0f}, {320.0f, 40.0f}}
};

static const TerrainTilePaletteWords s_terrain_tile_palette[] = {
    { { 0x42340000u, 0x00000000u, 0x42960000u } }, /* 0x4a8f34 */
    { { 0x42240000u, 0x42c00000u, 0x437f0000u } }, /* 0x4a8f40 */
    { { 0x437f0000u, 0x42b40000u, 0x00000000u } }, /* 0x4a8f4c */
    { { 0x43240000u, 0x43630000u, 0x43150000u } }, /* 0x4a8f58 */
    { { 0x42c20000u, 0x434b0000u, 0x42ac0000u } }, /* 0x4a8f64 */
    { { 0x00000000u, 0x432f0000u, 0x41400000u } }, /* 0x4a8f70 */
    { { 0x00000000u, 0x42ba0000u, 0x00000000u } }, /* 0x4a8f7c */
    { { 0x00000000u, 0x423c0000u, 0x00000000u } }, /* 0x4a8f88 */
    { { 0x43470000u, 0x42f60000u, 0x437f0000u } }, /* 0x4a8f94 */
    { { 0x432e0000u, 0x42b20000u, 0x43690000u } }, /* 0x4a8fa0 */
    { { 0x43100000u, 0x42700000u, 0x43480000u } }, /* 0x4a8fac */
    { { 0x42e60000u, 0x41f00000u, 0x43280000u } }, /* 0x4a8fb8 */
    { { 0x42ac0000u, 0x40000000u, 0x43080000u } }, /* 0x4a8fc4 */
    { { 0x437f0000u, 0x42f60000u, 0x437b0000u } }, /* 0x4a8ff4 */
    { { 0x43690000u, 0x42b20000u, 0x43620000u } }, /* 0x4a9000 */
    { { 0x43480000u, 0x42700000u, 0x43400000u } }, /* 0x4a900c */
    { { 0x43280000u, 0x41f00000u, 0x431e0000u } }, /* 0x4a9018 */
    { { 0x43080000u, 0x40000000u, 0x42fa0000u } }, /* 0x4a9024 */
    { { 0x437f0000u, 0x437f0000u, 0x00000000u } }, /* 0x4a9030 */
    { { 0x431d0000u, 0x431b0000u, 0x00000000u } }, /* 0x4a903c */
    { { 0x431b0000u, 0x42820000u, 0x41600000u } }, /* 0x4a9048 */
    { { 0x43340000u, 0x437f0000u, 0x00000000u } }, /* 0x4a9054 */
    { { 0x437f0000u, 0x437f0000u, 0x00000000u } }, /* 0x4a9060 */
    { { 0x00000000u, 0x423c0000u, 0x43350000u } }, /* 0x4a906c */
    { { 0x433f0000u, 0x433f0000u, 0x433f0000u } }, /* 0x4a9078 */
    { { 0x42d80000u, 0x00000000u, 0x00000000u } }  /* 0x4a9084 */
};

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static void terrain_tile_palette_copy(void* output, uint32_t palette);
static uint32_t terrain_tile_palette_index(uint32_t material, int walkable, int wave);

// FUNCTION: LEGORR 0x0045d8b0
void screen_coord_normalize_init(void)
{
    const double color_k = (double)((float)(1.0 / 255.0));
    int i;
    int mirror;
    int row;
    for (i = 0; i < 90; ++i) {
        s_snorm_colors[i] = (float)((double)s_snorm_colors[i] * color_k);
    }
    for (i = 0; i < 20; ++i) {
        s_snorm_cells[0][i] = 1.0f;
        s_snorm_cells[1][i] = 1.0f;
    }
    for (mirror = 0; mirror < 2; ++mirror) {
        for (row = 0; row < 7; ++row) {
            s_snorm_pairs[mirror][row][0] = s_snorm_pair_consts[mirror][row][0];
            s_snorm_pairs[mirror][row][1] = s_snorm_pair_consts[mirror][row][1];
        }
    }
    s_snorm_rows = 7u;
    const double w = (double)((float)display_width_get());
    const double h = (double)((float)display_height_get());
    const double kx = (double)((float)(1.0 / 640.0));
    const double ky = (double)((float)(1.0 / 480.0));
    for (mirror = 0; mirror < 2; ++mirror) {
        for (row = 0; row < 7; ++row) {
            s_snorm_pairs[mirror][row][0]
                = (float)((w * kx) * (double)s_snorm_pairs[mirror][row][0]);
            s_snorm_pairs[mirror][row][1]
                = (float)((h * ky) * (double)s_snorm_pairs[mirror][row][1]);
        }
    }
    s_snorm_sentinels[0] = 0xffffffffu;
    s_snorm_sentinels[1] = 0xffffffffu;
}

/* UiRenderRecordCreate (FUN_0045da90): build the 0x3c corpus record —
 * the source, its four copied payload words, the extra argument, and the
 * source's float read at byte offset +0x10 (the corpus threads that read
 * through its float10 widening — a float -> float10 -> float round trip
 * is exact, so the port copies). Deviations: the record fields widen
 * LP64-safe, the payload array and source are guarded, and the corpus's
 * uninitialized record tail stays absent from the modeled record. */
// FUNCTION: LEGORR 0x0045da90
UiRenderRecord* ui_render_record_create(void* source,
                                        const uint32_t* payload,
                                        uint32_t extra)
{
    if (source == NULL || payload == NULL) {
        return NULL; /* deviation: the corpus strides blindly */
    }
    UiRenderRecord* rec = (UiRenderRecord*)engine_heap_alloc(
        (long)sizeof(UiRenderRecord));
    if (rec == NULL) {
        return NULL;
    }
    rec->source = source;
    rec->payload[0] = payload[0];
    rec->payload[1] = payload[1];
    rec->payload[2] = payload[2];
    rec->payload[3] = payload[3];
    rec->extra = extra;
    __builtin_memcpy(&rec->float_at_10,
                     (const unsigned char*)source + 0x10, sizeof rec->float_at_10);
    return rec;
}

// FUNCTION: LEGORR 0x0045daf0
void ui_render_record_free(void* record)
{
    engine_heap_free(record);
}

// FUNCTION: LEGORR 0x0045db00
void textured_scene_object_draw_at_position(const void* view,
                                            const float position_xy[2],
                                            float z, float size)
{
    float saved_matrix[16];
    float position[4];
    float scale;
    int width;
    int saved = 0;
    if (view == NULL || position_xy == NULL ||
        s_textured_scene_draw_hooks.draw == NULL) return;
    if (s_textured_scene_draw_hooks.state_save != NULL)
        saved = s_textured_scene_draw_hooks.state_save(
            s_textured_scene_draw_hooks.user, saved_matrix);
    if (s_textured_scene_draw_hooks.view_transform_set != NULL)
        s_textured_scene_draw_hooks.view_transform_set(
            s_textured_scene_draw_hooks.user, (const char*)view + 4);
    position[0] = position_xy[0];
    position[1] = position_xy[1];
    position[2] = z;
    position[3] = 0.0f;
    view_point_to_screen_geometry_transform(view, position);
    scale = size * 0.7f + 0.3f;
    width = (int)scale;
    s_textured_scene_draw_hooks.draw(s_textured_scene_draw_hooks.user,
        position, width, width * 2, scale * 0.7f, scale * 0.9f, scale);
    if (saved && s_textured_scene_draw_hooks.state_restore != NULL)
        s_textured_scene_draw_hooks.state_restore(
            s_textured_scene_draw_hooks.user, saved_matrix);
}

/* EntityTypeAndStateFlagPredicate (FUN_0045e400) — see entity.hpp. */
// FUNCTION: LEGORR 0x0045e400
int entity_type_and_state_flag_predicate(const EntitySlot* e)
{
    if (e == NULL) {
        return 0; /* deviation: the corpus strides blindly */
    }
    const int type = e->type;
    if (type != 0xe && type != 5 && type != 8 && type != 0xd
        && type != 0x10 && type != 0x11 && type != 0x13
        && type != 9 && type != 0x12 && type != 4 && type != 0xc) {
        return ((~entity_state_flags_get(e)) >> 29) & 1u;
    }
    return 0;
}

/* RectContainsPointHalfOpen (FUN_0045e660): rect is the corpus record's
 * float quad {x, y, w, h} at +0x4…+0x10 — half-open in both axes. */
// FUNCTION: LEGORR 0x0045e660
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) int rect_contains_point_half_open(const float* rect_record,
                                                    int x, int y)
{
    __asm {
        sub esp, 8
        mov eax, dword ptr [esp + 0x10]
        mov ecx, dword ptr [esp + 0x0c]
        xor edx, edx
        mov dword ptr [esp], eax
        mov dword ptr [esp + 4], edx
        fild qword ptr [esp]
        fcom dword ptr [ecx + 4]
        fnstsw ax
        test ah, 1
        jne rect_contains_false_with_stack
        fld dword ptr [ecx + 0x0c]
        fadd dword ptr [ecx + 4]
        fxch st(1)
        fcompp
        fnstsw ax
        test ah, 1
        je rect_contains_false

        mov eax, dword ptr [esp + 0x14]
        mov dword ptr [esp + 4], edx
        mov dword ptr [esp], eax
        fild qword ptr [esp]
        fcom dword ptr [ecx + 8]
        fnstsw ax
        test ah, 1
        jne rect_contains_false_with_stack
        fld dword ptr [ecx + 0x10]
        fadd dword ptr [ecx + 8]
        fxch st(1)
        fcompp
        fnstsw ax
        test ah, 1
        je rect_contains_false
        mov eax, 1
        add esp, 8
        ret

    rect_contains_false_with_stack:
        fstp st(0)
    rect_contains_false:
        xor eax, eax
        add esp, 8
        ret
    }
}
#else
int rect_contains_point_half_open(const float* rect_record, int x, int y)
{
    if (((rect_record[1] <= (float)x)
         && ((float)x < rect_record[3] + rect_record[1]))
        && ((rect_record[2] <= (float)y)
            && ((float)y < rect_record[4] + rect_record[2]))) {
        return 1;
    }
    return 0;
}
#endif

// FUNCTION: LEGORR 0x0045e6d0
int terrain_point_pick_resolve(const TerrainPointPickRegion* region,
                               uint32_t screen_x, uint32_t screen_y,
                               float out_world_xy[2],
                               uint32_t argument_a,
                               uint32_t argument_b)
{
    float world_x;
    float world_y;
    float x = (float)screen_x;
    float y = (float)screen_y;
    if (region == NULL || s_terrain_point_pick_hook == NULL ||
        region->tile_size == 0.0f ||
        x < region->screen_left ||
        x >= region->screen_left + region->screen_width ||
        y < region->screen_top ||
        y >= region->screen_top + region->screen_height) {
        return 0;
    }
    world_y = region->scale *
        ((region->screen_top + region->screen_width * 0.5f) - y) /
        region->tile_size + region->world_origin_y;
    world_x = region->world_origin_x - region->scale *
        ((region->screen_left + region->screen_width * 0.5f) - x) /
        region->tile_size;
    if (!s_terrain_point_pick_hook(region->grid_token, world_x, world_y,
                                   argument_a, argument_b,
                                   s_terrain_point_pick_user)) {
        return 0;
    }
    if (out_world_xy != NULL) {
        out_world_xy[0] = world_x;
        out_world_xy[1] = world_y;
    }
    return 1;
}

// FUNCTION: LEGORR 0x0045e820
int viewport_point_callback_dispatch(const TerrainPointPickRegion* region,
                                     uint32_t screen_x, uint32_t screen_y,
                                     uint32_t* object_handle,
                                     float out_position_xy[2])
{
    float world_xy[2];
    float position[3];
    uint32_t handle = 0u;
    if (object_handle != NULL) *object_handle = 0u;
    if (region == NULL || object_handle == NULL ||
        s_viewport_point_dispatch_hooks.resolve == NULL ||
        !terrain_point_pick_resolve(region, screen_x, screen_y, world_xy,
                                    0u, 0u)) {
        return 0;
    }
    if (!s_viewport_point_dispatch_hooks.resolve(region->grid_token,
            world_xy[0], world_xy[1], region->scale, &handle,
            s_viewport_point_dispatch_hooks.user) || handle == 0u) {
        return 0;
    }
    *object_handle = handle;
    if (out_position_xy != NULL &&
        s_viewport_point_dispatch_hooks.position_get != NULL &&
        s_viewport_point_dispatch_hooks.position_get(handle, position,
            s_viewport_point_dispatch_hooks.user)) {
        out_position_xy[0] = position[0];
        out_position_xy[1] = position[1];
    }
    return 1;
}

/* ViewPointToScreenGeometryTransform (FUN_0045e940) — see view.hpp. The
 * record window is the projection block: center +4/+8, half-size +c/+10,
 * scale pair +14/+18, screen offsets +1c/+20. Deviations: NULL guards on
 * both inputs (the corpus dereferences blindly). The scale ratio is
 * recomputed at each output as in the corpus. */
// FUNCTION: LEGORR 0x0045e940
void view_point_to_screen_geometry_transform(const void* view, float p[4])
{
    if (view == NULL || p == NULL) {
        return; /* deviation: the corpus strides both blindly */
    }
    const char* w = (const char*)view;
    float cx = 0.0f, cy = 0.0f, half_x = 0.0f, half_y = 0.0f;
    float scale_a = 0.0f, scale_b = 0.0f, off_x = 0.0f, off_y = 0.0f;
    __builtin_memcpy(&cx, w + 0x04, sizeof cx);
    __builtin_memcpy(&cy, w + 0x08, sizeof cy);
    __builtin_memcpy(&half_x, w + 0x0c, sizeof half_x);
    __builtin_memcpy(&half_y, w + 0x10, sizeof half_y);
    __builtin_memcpy(&scale_a, w + 0x14, sizeof scale_a);
    __builtin_memcpy(&scale_b, w + 0x18, sizeof scale_b);
    __builtin_memcpy(&off_x, w + 0x1c, sizeof off_x);
    __builtin_memcpy(&off_y, w + 0x20, sizeof off_y);

    p[0] = p[0] - off_x;
    p[1] = p[1] - off_y;
    p[0] = (scale_a / scale_b) * p[0];
    p[1] = (scale_a / scale_b) * p[1];
    p[2] = (scale_a / scale_b) * p[2];
    p[0] = p[0] + half_x * 0.5f + cx;
    p[1] = (half_y * 0.5f + cy) - p[1];
    p[3] = (scale_a / scale_b) * p[3];
}

/* The target reads the current-level material and cell flags, then writes a
 * palette triplet only on the branches where the retail body selects one.
 * Recognized palette words are recorded verbatim from the retail data at
 * 0x4a8f34..0x4a908c. */
// FUNCTION: LEGORR 0x0045e9e0
int terrain_tile_style_resolve(void* output, int x, int y)
{
    GameContext* game = entity_pool_callback_context_get();
    LevelState* level;
    const uint8_t* voxel;
    uint32_t flags = 0u;
    uint32_t palette;
    int material;
    int walkable;
    if (output == NULL) return 0;
    level = game != NULL ? game->level : NULL;
    if (level == NULL) level = (LevelState*)map_cell_record_grid_level_get();
    if (level == NULL) return 0;
    voxel = level_voxel(level, x, y);
    if (voxel == NULL) return 0;
    memcpy(&flags, voxel + 0x08, sizeof(flags));
    material = level_tile_type(level, x, y);
    walkable = level_cell_walkable_flag(level, x, y);
    if (walkable) {
        palette = terrain_tile_palette_index((uint32_t)material, 1,
            level_cell_flag_wave(level, x, y) != 0u);
        if (palette != 0xffffffffu) terrain_tile_palette_copy(output, palette);
        return 1;
    }

    if (level_cell_mask18_clear(level, x, y) &&
        level_cell_flag_4(level, x, y) != 0u) {
        if ((flags & 0x20000u) != 0u) {
            terrain_tile_palette_copy(output, TERRAIN_TILE_PALETTE_SPECIAL_DUG);
            return 1;
        }
        palette = terrain_tile_palette_index((uint32_t)material, 0, 0);
        if (palette != 0xffffffffu) {
            terrain_tile_palette_copy(output, palette);
            return 1;
        }
        return 1;
    }

    if (level_cell_flag_8_bounded(level, x, y) == 0u) return 0;
    if (material == 6) {
        terrain_tile_palette_copy(output, TERRAIN_TILE_PALETTE_TYPE6);
        return 1;
    }
    if (material == 7) {
        terrain_tile_palette_copy(output, TERRAIN_TILE_PALETTE_TYPE7);
        return 1;
    }
    if (material == 9) {
        terrain_tile_palette_copy(output, TERRAIN_TILE_PALETTE_TYPE9);
        return 1;
    }
    if ((flags & 0x100000u) != 0u) {
        terrain_tile_palette_copy(output, TERRAIN_TILE_PALETTE_FLAGS_100000);
        return 1;
    }
    if (level_cell_occupied_at(level, x, y) != 0) {
        if (level_cell_secondary_flag_100(level, x, y) != 0u) {
            terrain_tile_palette_copy(output,
                TERRAIN_TILE_PALETTE_CELL_OCCUPIED);
        }
        else {
            terrain_tile_palette_copy(output,
                TERRAIN_TILE_PALETTE_CELL_UNOCCUPIED);
        }
        return 1;
    }
    terrain_tile_palette_copy(output, 0u);
    return 1;
}
static uint32_t terrain_tile_palette_index(uint32_t material, int walkable,
                                           int wave)
{
    if (walkable) {
        if (wave) {
            if (material >= 1u && material <= 5u)
                return 18u - material;
            /* The wave branch shares the same palette records for terrain
             * types 8, 10, and 11.  The reference switches on these cases
             * after the five wave-specific type colors. */
            if (material == 8u) return TERRAIN_TILE_PALETTE_TYPE8_SHARED;
            if (material == 10u) return TERRAIN_TILE_PALETTE_TYPE10;
            if (material == 11u) return TERRAIN_TILE_PALETTE_TYPE11;
            return 0xffffffffu;
        }
        switch (material) {
        case 1: return TERRAIN_TILE_PALETTE_DUG_TYPE1;
        case 2: return TERRAIN_TILE_PALETTE_DUG_TYPE2;
        case 3: return TERRAIN_TILE_PALETTE_DUG_TYPE3;
        case 4: return TERRAIN_TILE_PALETTE_DUG_TYPE4;
        case 5: return TERRAIN_TILE_PALETTE_DUG_TYPE5;
        case 8: return TERRAIN_TILE_PALETTE_TYPE8_SHARED;
        case 10: return TERRAIN_TILE_PALETTE_TYPE10;
        case 11: return TERRAIN_TILE_PALETTE_TYPE11;
        default: return 0xffffffffu;
        }
    }
    switch (material) {
    case 1: return TERRAIN_TILE_PALETTE_TYPE1_PRE;
    case 2: return TERRAIN_TILE_PALETTE_TYPE2_PRE;
    case 3: return TERRAIN_TILE_PALETTE_TYPE3_PRE;
    case 4: return TERRAIN_TILE_PALETTE_TYPE4_PRE;
    case 5: return TERRAIN_TILE_PALETTE_TYPE5_PRE;
    case 6: return TERRAIN_TILE_PALETTE_TYPE6;
    case 7: return TERRAIN_TILE_PALETTE_TYPE7;
    case 8: return TERRAIN_TILE_PALETTE_TYPE8_SHARED;
    case 9: return TERRAIN_TILE_PALETTE_TYPE9;
    case 10: return TERRAIN_TILE_PALETTE_TYPE10;
    case 11: return TERRAIN_TILE_PALETTE_TYPE11;
    default: return 0xffffffffu;
    }
}

static void terrain_tile_palette_copy(void* output, uint32_t palette)
{
    if (palette < sizeof(s_terrain_tile_palette) /
                  sizeof(s_terrain_tile_palette[0])) {
        float color[3];
        uint32_t component;
        for (component = 0; component < 3u; ++component) {
            memcpy(&color[component],
                   &s_terrain_tile_palette[palette].word[component],
                   sizeof(color[component]));
            color[component] /= 255.0f;
        }
        memcpy((unsigned char*)output + 0x10, color, sizeof(color));
    }
}
