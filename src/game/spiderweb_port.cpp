// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-042 SpiderWeb.c: the port tail (ADR 0006 decision 4; finding 620).
 * The original-shape unit is spiderweb.c. These are the port's models of
 * its functions, unannotated, under the port's names: they read the port's
 * EngineState, EntitySlot and the grid buffer s_field_grid_buf
 * (gods98/engine.cpp), not the original's Lego_Level and LegoObject, so
 * native callers (the simulation ticker and the tests) keep calling them. Lego.c (lego_unit.c) calls spiderweb.c. */
/* Matching build: LegoObject.c callers call legoobject_unit.c (#575). */
#include "legoobject_port_aliases.h"
#include "camera.hpp"
#include "engine.hpp"
#include "frameloop.hpp"
#include "mainsim.hpp"
#include "message.hpp"
#include "tickers.hpp"
#include "simtick.hpp"
#include "view.hpp"
#include "record.hpp"
#include "crt/crt.hpp"
#include "sound.hpp"
#include "audio.hpp"
#include "media.hpp"
#include "resource.hpp"
#include "heap.hpp"
#include "scene_resources.hpp"
#include "game/frontend.hpp"
#include "game/entity.hpp"
#include "game/building_scene.hpp"
#include "game/actor_scene.hpp"
#include "game/objective.hpp"
#include "game/effects.hpp"
#include "game/scene_render.hpp"
#include "game/util.hpp"
#include "scene_pool.hpp"
#include "string.hpp"
#include "game/port_loop_ports.hpp"
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
/* GAME-021 lego_unit.c reads this word (C linkage, ADR 0005 step 6). */
extern "C" uint32_t s_overlay_word_03610;
#include "engine_for_grid_cell_records.hpp"
#include <math.h>
#include <float.h>
#include <string.h>
#include <stdint.h>
#include "ai.hpp"
#include "type_config_ports.hpp"
#include "entity_pose_delta.hpp"
#include "entity_motion_ports.hpp"
#include "activity_timeout.hpp"
#include "entity_motion_probe.hpp"
#include "simulation_ui_ports.hpp"
#include "type_motion_scale.hpp"
#include "entity_quad_collision.hpp"
#include "spawn.hpp"
#include "weapon.hpp"
#include "level.hpp"
#include "terrain_grid.hpp"
#include "cmd.hpp"
#include "select.hpp"
#include "gods98/model.hpp"
#include "gods98/block.hpp"
#include "upgrades.hpp"
#include "entity_for_grid_cell_records.hpp"
#include <limits.h>
#include "game.hpp"
#include "gods98/surface.hpp"
#include "gods98/flic.hpp"
#include "tutorial_hud_ports.hpp"
#include "level_for_grid_cell_records.hpp"
#include "game/render.hpp"
#include "simtick_for_grid_cell_records.hpp"

namespace unported_ledger_stubs {
int ledger_stub_00466250_ExpandCellEntitySpawn(int x, int y);

int ledger_stub_00466490_CellSightRayCheckAndFlag(const EntitySlot* observer);
} // namespace unported_ledger_stubs

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static int post_step_cell_sight_callback(EntitySlot* entity, void* payload);

void grid_cell_record_buffer_allocate(EngineState* block)
{
    if (block == NULL) {
        return; /* deviation: the corpus strides the block blindly */
    }
    const int w = (int)block->grid_w;
    const int h = (int)block->grid_h;
    /* The corpus uses 12-byte records on x86 (entity, timer, flags). The
     * portable build widens the entity slot with sizeof(void*), retaining a
     * 16-byte record on LP64 while naturally returning to 12 bytes on x86. */
    const int cell_size = (int)sizeof(void*) + 8;
    void* buf = engine_heap_alloc((long)(w * h * cell_size));
    if (buf != NULL) {
        uint32_t* p = (uint32_t*)buf;
        for (uint32_t n = (uint32_t)(w * h * (cell_size / 4)) & 0x3fffffffu;
             n != 0u; --n) {
            *p++ = 0u;
        }
    }
    s_field_grid_buf = buf;
    s_field_grid_desc = block;
}

void entity_field_grid_buffer_release(void)
{
    if (s_field_grid_buf != NULL) {
        engine_heap_free(s_field_grid_buf);
        s_field_grid_buf = NULL;
    }
}

void grid_cell_record_buffer_replace(EngineState* block)
{
    entity_field_grid_buffer_release();
    grid_cell_record_buffer_allocate(block);
}

namespace unported_ledger_stubs {
/* ExpandCellEntitySpawn (FUN_00466250) creates a type-12 entity only when
 * the cell's expansion bit is clear and the surrounding terrain flags allow
 * its orientation. The scene action setter itself remains renderer-owned;
 * entity creation, cell state, and the cell object's live sidecar are
 * handled here. */
int ledger_stub_00466250_ExpandCellEntitySpawn(int x, int y)
{
    static const float half_pi = 1.57079632679f;
    GameContext* game = entity_pool_callback_context_get();
    LevelState* level = game != NULL ? game->level : NULL;
    uint8_t* cell;
    uint32_t flags;
    uint32_t entity_word;
    float position[3];
    float orientation;
    EntitySlot* entity;

    if (level == NULL) level = level_current_owner_get();
    if (game == NULL || level == NULL || level->ptr_terrain == NULL)
        return 0;
    if ((unsigned)x >= (unsigned)level_width(level) ||
        (unsigned)y >= (unsigned)level_height(level)) return 0;
    map_cell_record_grid_activate(level);
    cell = cell_record_unchecked(level, x, y);
    if (cell == NULL) return 0;
    memcpy(&flags, cell + 0x08, sizeof(flags));
    if ((flags & 0x100u) != 0u || (flags & 8u) == 0u) return 0;

    /* FUN_004663f0 first tests the two horizontal neighbors. If either
     * lacks bit 0x10, both vertical neighbors must carry it and orientation
     * is zero; otherwise the expansion faces at pi/2. */
    if ((cell_flag_word(level, x + 1, y) & 0x10u) == 0u ||
        (cell_flag_word(level, x - 1, y) & 0x10u) == 0u) {
        if ((cell_flag_word(level, x, y + 1) & 0x10u) == 0u ||
            (cell_flag_word(level, x, y - 1) & 0x10u) == 0u) return 0;
        orientation = 0.0f;
    }
    else {
        orientation = half_pi;
    }
    if (!terrain_cell_world_position_get(x, y, position)) return 0;

    entity = entity_create_with_position(game, 0x0c, NULL, 0,
        position[0], position[1], orientation);
    if (entity == NULL) return 0;

    entity_word = (uint32_t)(uintptr_t)entity;
    memcpy(cell, &entity_word, sizeof(entity_word));
    {
        const float expansion_time = 100.0f;
        memcpy(cell + 0x04, &expansion_time, sizeof(expansion_time));
    }
    flags |= 0x100u;
    memcpy(cell + 0x08, &flags, sizeof(flags));
    entity->flags_3e0 |= 0x100u;
    /* The native owner keeps the full pointer in its cell-link sidecar; the
     * retail record word above remains the 32-bit image used by x86 callers. */
    (void)grid_cell_object_link_insert(entity, x, y);
    return 1;
}
} // namespace unported_ledger_stubs

void set_expansion_entity_at_grid_cell(int x, int y, void* entity)
{
    if (s_field_grid_buf != NULL && s_field_grid_desc != NULL) {
        /* Preserve the corpus's repeated index calculation and write order.
         * On x86 these expressions use stride 0x0c, flags at +8, and timer
         * at +4: imul width*y; add x; lea index*3; then scale by four. */
        *(void**)((char*)s_field_grid_buf +
                  ((int)s_field_grid_desc->grid_w * y + x) *
                      ((int)sizeof(void*) + 8)) = entity;
        uint32_t* flags = (uint32_t*)((char*)s_field_grid_buf +
                  ((int)s_field_grid_desc->grid_w * y + x) *
                      ((int)sizeof(void*) + 8) + (int)sizeof(void*) + 4);
        *flags = *flags | 0x100u;
        *(uint32_t*)((char*)s_field_grid_buf +
                  ((int)s_field_grid_desc->grid_w * y + x) *
                      ((int)sizeof(void*) + 8) + (int)sizeof(void*)) =
            0x42c80000u;
    }
}

int can_spawn_expansion_entity_at_grid_cell(int x, int y, float* out)
{
    const EngineState* desc = s_field_grid_desc;
    const int w = (int)desc->grid_w;
    const char* vox = (const char*)desc->grid_cells;
    const long i = (long)y * w + x;
    const long s = i * 9; /* the corpus's lea [eax+eax*8], *8 rides the scale */
    const char* base = vox + s * 8;
    if ((base[8] & 8) == 0) {
        return 0;
    }
    if ((base[0x50] & 0x10) == 0 || (base[-0x40] & 0x10) == 0) {
        const long iu = (long)(y + 1) * w + x;
        const long su = iu * 9;
        const long id = (long)(y - 1) * w + x;
        const long sd = id * 9;
        if ((vox[su * 8 + 8] & 0x10) == 0 || (vox[sd * 8 + 8] & 0x10) == 0) {
            return 0;
        }
        if (out != NULL) {
            *(uint32_t*)out = 0u; /* the corpus's zero immediate dword */
        }
    } else if (out != NULL) {
        *(uint32_t*)out = 0x3fc90fdbu; /* the corpus's immediate dword */
        return 1;
    }
    return 1;
}

namespace unported_ledger_stubs {
int ledger_stub_00466490_CellSightRayCheckAndFlag(
    const EntitySlot* observer)
{
    EntitySlot* cell_entity = NULL;
    LevelState* level;
    float observer_position[2];
    float start[2];
    float end[2];
    float direction_length;
    int cell_x = -1;
    int cell_y = -1;
    uint32_t cell_flags = 0u;
    int hit;
    if (observer == NULL || observer->owner_ctx == NULL ||
        observer->owner_ctx->level == NULL) return 0;
    level = observer->owner_ctx->level;
    if (level->ptr_terrain == NULL) return 0;
    entity_transform_position_get(observer, observer_position);
    if (!terrain_world_xy_to_cell(level->ptr_terrain, observer_position[0],
            observer_position[1], &cell_x, &cell_y) ||
        !entity_field_grid_cell_get(cell_x, cell_y, (void**)&cell_entity,
                                    &cell_flags) ||
        (cell_flags & 0x100u) == 0u || cell_entity == NULL ||
        !terrain_cell_world_center_xy_get(level->ptr_terrain, cell_x, cell_y,
                                          start)) return 0;

    /* Retail normalizes the occupied cell entity's surface vector before
     * using it as the ray endpoint. Prefer the installed renderer accessor;
     * EntitySlot's attached direction is the typed runtime representation
     * when that accessor has no provider. */
    if (!entity_surface_vector_normalize_if_available(cell_entity, end)) {
        end[0] = cell_entity->attached_direction[0];
        end[1] = cell_entity->attached_direction[1];
        direction_length = (float)sqrt((double)(end[0] * end[0] +
                                                 end[1] * end[1]));
        if (direction_length == 0.0f) return 0;
        end[0] /= direction_length;
        end[1] /= direction_length;
    }
    if ((entity_state_flags_get(observer) & 0x100u) != 0u) {
        hit = entity_surface_quad_segment_intersect((EntitySlot*)observer,
            start, end, NULL, 0, &s_entity_quad_collision_hooks);
    }
    else {
        hit = entity_segment_center_proximity_check(observer, start, end, 0);
    }
    if (!hit) return 0;
    ((EntitySlot*)observer)->flags_3e0 |= 0x02000000u;
    cell_entity->flags_3e0 |= 0x02000000u;
    return 1;
}
} // namespace unported_ledger_stubs

void simtick_post_step(EngineContext* c)
{
    (void)c;
    /* FUN_004665c0 calls FUN_004377b0 with this relay. That walker excludes
     * the same 0x80000-filtered records as the existing filtered pool API. */
    entity_pool_find_by_callback_filtered_current(
        post_step_cell_sight_callback, NULL);
    if (s_post_step_hook != NULL)
        s_post_step_hook(c, s_post_step_user);
    ++s_post_n;
}

void grid_cell_invalidate_and_clear_flags(int x, int y)
{
    const EngineState* desc = s_field_grid_desc;
    char* cell;
    if (s_field_grid_buf == NULL || desc == NULL) {
        return; /* deviation: the corpus strides the missing grid blindly */
    }
    cell = (char*)s_field_grid_buf +
           (size_t)((int)desc->grid_w * y + x) *
               ((size_t)sizeof(void*) + 8u);
    /* Corpus shape: the entity is re-read from the slot before each flag
     * window it touches (the aliasing reloads). */
    void* entity = NULL;
    __builtin_memcpy(&entity, cell + 0x00, sizeof entity);
    if (entity != NULL) {
        uint32_t w = 0u;
        __builtin_memcpy(&w, (char*)entity + 0x3e8, sizeof w);
        w |= 0x800000u;
        __builtin_memcpy((char*)entity + 0x3e8, &w, sizeof w);
        entity = NULL;
        __builtin_memcpy(&entity, cell + 0x00, sizeof entity);
        if (entity != NULL) {
            __builtin_memcpy(&w, (char*)entity + 0x3e0, sizeof w);
            w &= 0xfdffffffu;
            __builtin_memcpy((char*)entity + 0x3e0, &w, sizeof w);
        }
    }
    uint32_t flags = 0u;
    __builtin_memcpy(&flags, cell + sizeof(void*) + 4, sizeof flags);
    flags &= 0xfffffeffu;
    __builtin_memcpy(cell + sizeof(void*) + 4, &flags, sizeof flags);
}

static int post_step_cell_sight_callback(EntitySlot* entity, void* payload)
{
    (void)payload;
    (void)unported_ledger_stubs::
        ledger_stub_00466490_CellSightRayCheckAndFlag(entity);
    /* LAB_004665e0's result does not terminate the entity-pool walk. */
    return 0;
}
