// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-038 Roof.c's port tail (finding 619): the port's model of the roof
 * hide stack, the overlay visibility cache and the plane-normal reader,
 * unannotated (ADR 0006 decision 4). The original-shape unit is roof.c.
 *
 * Every build keeps these definitions: the native port calls them. In the
 * matching build lego_unit_imports.h points Lego.c at roof.c's names
 * instead, so nothing there calls these. Relocated from the verbatim
 * terrain_overlay_quads.cpp (body hashes:
 * docs/coverage/relocations/GAME-038.csv). The port's models of
 * Roof_Initialise, Roof_SetTexture and Roof_Shutdown are in
 * gods98/resource.cpp. */
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
#include "record_for_terrain_overlay_quads.hpp"
#include <stdio.h>
#include <stdint.h>
#include "game.hpp"
#include "gods98/block.hpp"
#include "gods98/engine.hpp"
#include "gods98/media.hpp"
#include "effects.hpp"
#include "gods98/message.hpp"
#include "gods98/simtick.hpp"
#include "gods98/surface.hpp"
#include "gods98/camera.hpp"
#include "terrain_grid.hpp"
#include "entity.hpp"
#include "type_config_ports.hpp"
#include "ai.hpp"
#include "cmd.hpp"
#include "upgrades.hpp"
#include "spawn.hpp"
#include "port_loop_ports.hpp"
#include "frontend.hpp"
#include "level.hpp"
#include "objective.hpp"
#include "gods98/flic.hpp"
#include "select.hpp"
#include "scene_render.hpp"
#include "tutorial_hud_ports.hpp"
#include "level_for_terrain_overlay_quads.hpp"

/* The corpus's indexed-surface grid is owned by the overlay resource
 * loader, which is still unported. This sidecar exposes a small install
 * seam and preserves the hide-stack order without fabricating that loader. */
#define INDEXED_SURFACE_ENTRY_LIMIT 256

static const void* s_indexed_surface_hidden[INDEXED_SURFACE_ENTRY_LIMIT];

void graphics_overlay_visibility_set(uint32_t flag)
{
    if (s_overlay_cache != flag) {
        /* The corpus commits the cache before testing the requested flag
         * (0x4634c0, then 0x4634c5) and dispatches separate 1/0 paths. */
        s_overlay_cache = flag;
        if (flag != 0u) {
            scene_visibility_hook_call((void*)(uintptr_t)s_overlay_word, 1);
        } else {
            scene_visibility_hook_call((void*)(uintptr_t)s_overlay_word, 0);
        }
    }
}

/* IndexedSurfaceEntryHideAndRecord (Roof_HideBlock, 0x004634f0): compute y*width+x,
 * hide the selected surface and save its handle for a later restore. */
void indexed_surface_entry_hide_and_record(int x, int y)
{
    if (s_indexed_surface_width == 0u || s_indexed_surface_entries == NULL
        || x < 0 || y < 0) {
        return;
    }
    const uint32_t index = (uint32_t)y * s_indexed_surface_width
        + (uint32_t)x;
    if (index >= s_indexed_surface_entry_count
        || s_indexed_surface_hidden_count >= INDEXED_SURFACE_ENTRY_LIMIT) {
        return;
    }
    const void* handle = s_indexed_surface_entries[index];
    scene_visibility_hook_call((void*)handle, 0);
    s_indexed_surface_hidden[s_indexed_surface_hidden_count++] = handle;
}

/* IndexedSurfaceEntriesRestore (Roof_ShowHiddenBlocks, 0x00463540): show each handle saved by
 * the hide pass and reset the temporary list. */
void indexed_surface_entries_restore(void)
{
    for (uint32_t i = 0; i < s_indexed_surface_hidden_count; ++i) {
        scene_visibility_hook_call((void*)s_indexed_surface_hidden[i], 1);
    }
    s_indexed_surface_hidden_count = 0u;
}

int terrain_cell_surface_normal_get(uint32_t x, uint32_t y, float out[3])
{
    float vertices[4][3];
    float first[3];
    float second[3];
    float nx, ny, nz, inverse_length;
    size_t index;
    if (out == NULL) return 0;
    /* The retail helper also gates on bit 0 of its renderer-owned 8-byte quad
     * record. This standalone API has no record pointer; the dirty-normal
     * pass enforces that gate at its call site. Do not substitute the
     * surface orientation bit here. */
    if (s_terrain_quad_surface != NULL &&
        s_terrain_quad_vertex_read != NULL &&
        x < s_terrain_quad_width && y < s_terrain_quad_height) {
        index = (size_t)y * (size_t)s_terrain_quad_width + (size_t)x;
        if (!s_terrain_quad_vertex_read(s_terrain_quad_vertex_context,
                s_terrain_quad_surface, index, vertices)) return 0;
    }
    else {
        GameContext* game = entity_pool_callback_context_get();
        LevelState* level = game != NULL ? game->level : NULL;
        if (level == NULL) level = level_current_owner_get();
        if (level == NULL || level->ptr_terrain == NULL ||
            !scene_render_terrain_cell_vertices_get(level, x, y,
                                                     vertices)) return 0;
    }
    /* The polygon reader returns the four points in the order consumed by
     * FUN_00463590: triangles (0,1,2) and (2,3,0). */
    surface_triangle_normal_calculate(first, vertices[0], vertices[1],
                                      vertices[2]);
    surface_triangle_normal_calculate(second, vertices[2], vertices[3],
                                      vertices[0]);
    nx = first[0] + second[0];
    ny = first[1] + second[1];
    nz = first[2] + second[2];
    inverse_length = 1.0f / (float)sqrt((double)(nx * nx + ny * ny + nz * nz));
    /* The game's output frame swaps the second and third normal components. */
    out[0] = nx * inverse_length;
    out[1] = nz * inverse_length;
    out[2] = ny * inverse_length;
    return 1;
}
