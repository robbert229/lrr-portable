// SPDX-License-Identifier: LGPL-3.0-or-later
/* Matching build: Lego.c callers call lego_unit.c (#587). */
#include "lego_port_aliases.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <limits.h>

#include "game.hpp"
#include "gods98/block.hpp"
#include "gods98/engine.hpp"
#include "gods98/media.hpp"
#include "effects.hpp"
#include "gods98/heap.hpp"
#include "gods98/message.hpp"
#include "gods98/record.hpp"
#include "gods98/resource.hpp"
#include "gods98/sound.hpp"
#include "gods98/simtick.hpp"
#include "gods98/string.hpp"
#include "gods98/surface.hpp"
#include "crt/crt.hpp"
#include "gods98/view.hpp"
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
#include "util.hpp"
#include "gods98/flic.hpp"
#include "terrain_grid.hpp"
#include "select.hpp"
#include "scene_render.hpp"
#include "tutorial_hud_ports.hpp"
/* GAME-026's Map3D_IsInsideDimensions (FUN_004502c0) is defined in the C
 * unit terrain_map.c. This first declaration gives it C linkage; the frozen
 * level headers below redeclare it and inherit that linkage (finding 556). */
extern "C" int level_coord_pair_within_bounds_target(
    const uint32_t* dimensions, uint32_t cx, uint32_t cy);
#include "level_shared.hpp"
/* GAME-021's lego_unit.c reads the block-update lists (issue #574): C
 * linkage, which the frozen header below and the definitions inherit. */
extern "C" int32_t s_cell_refresh_track[][2];
extern "C" int s_cell_refresh_track_count;
extern "C" int32_t s_cell_previous_track[][2];
extern "C" int s_cell_previous_track_count;
extern "C" int32_t s_cell_visited_track[][2];
extern "C" int s_cell_visited_track_count;
/* GAME-021's lego_unit.c reads the surface type descriptions (issue #587):
 * C linkage, which the frozen header below and the definitions inherit. */
extern "C" const char* s_surface_type_names[];
extern "C" uint32_t s_surface_type_values[];
#include "level_for_lego.hpp"
/* Defined in GAME-021's lego_unit.c (issue #544): C linkage, which the
 * frozen header below inherits. */
extern "C" float engine_float_field260_get_current_target(void);
extern "C" uint32_t global_state_field84_get_current_target(void);
extern "C" uint32_t s_neighbor_pick_rotation;
extern "C" int level_cell_flag8_check_target(const LevelState* unused, int cx, int cy);
#include "level_from_lego.hpp"
#include "level_for_files.hpp"
#include "level_for_legoobject.hpp"
/* C linkage for LegoObject.c state that legoobject_unit.c also reads
 * (defined in legoobject.cpp; the frozen header below redeclares it). */
extern "C" uint32_t s_grid_count_first[GRID_COUNT_TABLE_DWORDS];
extern "C" uint32_t s_grid_count_second[GRID_COUNT_TABLE_DWORDS];
extern "C" int s_terrain_sfx_grid[TERRAIN_SFX_GRID_CELLS];
#include "level_from_legoobject.hpp"
#include "level_for_aitask.hpp"

/* Cell mutation's cheat branch calls the existing entity-side implementation
 * of SmallSpiderCellSwarmSpawn. Its runtime prerequisites are checked by that
 * owner (active entity context, surface grid, and configured entity type). */

/* ------------------------------------------------------------------ *
 *  Corpus .rdata constants (identities from finding 07/22 and the
 *  decompile corpus; numeric values are best-effort re-derivations — the
 *  shipped data section is under copyright and not reproduced here).
 * ------------------------------------------------------------------ */

#define LVL_SEC_TO_TICK   25.0f       /* _DAT_0049e2b4: 25 game ticks/second */
#define LVL_RGB_SCALE     0.0039215686f /* _DAT_0049e328: 1/255            */

/* The reference overlay owner is the indexed-surface state at DAT_00552180,
 * DAT_00552184/88, and DAT_00552198. This reconstruction does not yet create
 * or retain that owner from the level/render startup path, so keep the reader
 * explicit instead of pretending that TerrainState owns the render quads.
 * Once the backend binds the indexed surface, the normal helpers below read
 * and update the actual quad records supplied by that owner. */
void* s_terrain_quad_surface;
uint32_t s_terrain_quad_width;
uint32_t s_terrain_quad_height;
TerrainQuadVertexReadHook s_terrain_quad_vertex_read;
void* s_terrain_quad_vertex_context;
TerrainCellOverlayHeightFillHook
    s_terrain_cell_overlay_height_fill;
void* s_terrain_cell_overlay_height_fill_context;
LevelTerrainMaterialTransitionMeshHook
    s_material_transition_mesh_hook;
void* s_material_transition_mesh_context;
TerrainSurfaceGrid* level_surface_grid_get(const LevelState* level);
LevelState* s_lava_erosion_level_context;
EngineState s_level_field_grid_state;
LevelState* s_level_field_grid_owner;
static int s_debug_scene_transform_enabled;
static int s_debug_camera_pose_mode;
static LevelDebugRenderOps s_level_debug_render_ops;
void terrain_cell_refresh_dispatch_level(LevelState* level,
                                                const int cell_xy[2],
                                                int mode);
void map_cell_tracking_activity_state_update(GameContext* game,
                                                     LevelState* level);

CellTransientEffectPort* s_cell_transient_effect_ports;

CellTransientEffectPort* cell_transient_effect_port_find(
    const LevelState* level, const void* record)
{
    CellTransientEffectPort* port;
    for (port = s_cell_transient_effect_ports; port != NULL;
         port = port->next) {
        if (port->level == level && port->record == record) return port;
    }
    return NULL;
}

void* cell_transient_effect_get(LevelState* level, void* record)
{
    CellTransientEffectPort* port =
        cell_transient_effect_port_find(level, record);
    if (port != NULL) return port->emitter;
    return (void*)(uintptr_t)*(uint32_t*)((uint8_t*)record + 0x1c);
}

static void cell_transient_effect_ports_release(LevelState* level)
{
    CellTransientEffectPort** link = &s_cell_transient_effect_ports;
    while (*link != NULL) {
        CellTransientEffectPort* port = *link;
        if (port->level == level) {
            *link = port->next;
            free(port);
        }
        else {
            link = &port->next;
        }
    }
}

static void level_field_grid_buffer_release(LevelState* level)
{
    if (level == NULL || s_level_field_grid_owner != level) return;
    entity_field_grid_buffer_release();
    s_level_field_grid_owner = NULL;
    s_level_field_grid_state.grid_w = 0u;
    s_level_field_grid_state.grid_h = 0u;
}

void terrain_quad_vertex_source_set(void* surface, uint32_t width,
                                    uint32_t height,
                                    TerrainQuadVertexReadHook hook,
                                    void* context)
{
    s_terrain_quad_surface = surface;
    s_terrain_quad_width = width;
    s_terrain_quad_height = height;
    s_terrain_quad_vertex_read = hook;
    s_terrain_quad_vertex_context = context;
}

void terrain_cell_overlay_height_fill_hook_set(
    TerrainCellOverlayHeightFillHook hook, void* context)
{
    s_terrain_cell_overlay_height_fill = hook;
    s_terrain_cell_overlay_height_fill_context = context;
}

/* TerrainDirtyCellQuadNormalEmitPass has the same renderer quad table that
 * FUN_00463590 reads from DAT_00552198. Preserve its per-cell bit-0 veto at
 * the call boundary, where the port has the corresponding record pointer. */
static int terrain_cell_surface_normal_get_for_pass(
    uint32_t x, uint32_t y, const uint8_t* records,
    uint32_t width, uint32_t height, float out[3])
{
    size_t index;
    if (records == NULL || x >= width || y >= height) return 0;
    index = (size_t)y * (size_t)width + (size_t)x;
    if ((records[index * 8u + 4u] & 1u) != 0u) return 0;
    return terrain_cell_surface_normal_get(x, y, out);
}

void terrain_dirty_quad_normal_emit_pass(TerrainDirtyQuadNormalPass* pass)
{
    static const int neighbor_offsets[4][4][2] = {
        { { 0, 0 }, { -1, 0 }, { 0, 1 }, { -1, 1 } },
        { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } },
        { { 0, 0 }, { 0, -1 }, { 1, -1 }, { 1, 0 } },
        { { 0, 0 }, { -1, -1 }, { 0, -1 }, { -1, 0 } }
    };
    uint32_t y;
    if (pass == NULL || pass->records == NULL || pass->dirty == NULL ||
        pass->emit_normals == NULL || *pass->dirty == 0u) return;
    *pass->dirty = 0u;
    for (y = 0; y < pass->height; ++y) {
        uint32_t x;
        for (x = 0; x < pass->width; ++x) {
            uint32_t index = y * pass->width + x;
            uint8_t* record = pass->records + index * 8u;
            float normals[4][3];
            uint32_t corner;
            int component;
            if ((record[4] & 2u) == 0u) continue;
            for (corner = 0; corner < 4u; ++corner) {
                unsigned int table_corner = (record[4] & 4u) != 0u
                    ? ((corner - 1u) & 3u) : corner;
                unsigned int neighbor;
                float length;
                for (component = 0; component < 3; ++component)
                    normals[corner][component] = 0.0f;
                for (neighbor = 0; neighbor < 4u; ++neighbor) {
                    const int nx = (int)x +
                        neighbor_offsets[table_corner][neighbor][0];
                    const int ny = (int)y +
                        neighbor_offsets[table_corner][neighbor][1];
                    float normal[3];
                    if (nx >= 0 && ny >= 0 &&
                        terrain_cell_surface_normal_get_for_pass(
                            (uint32_t)nx, (uint32_t)ny, pass->records,
                            pass->width, pass->height, normal)) {
                        normals[corner][0] += normal[0];
                        normals[corner][1] += normal[1];
                        normals[corner][2] += normal[2];
                    }
                }
                length = (float)sqrt((double)(
                    normals[corner][0] * normals[corner][0] +
                    normals[corner][1] * normals[corner][1] +
                    normals[corner][2] * normals[corner][2]));
                length = 1.0f / length;
                normals[corner][0] *= length;
                normals[corner][1] *= length;
                normals[corner][2] *= length;
            }
            pass->emit_normals(pass->context, index, normals);
            record[4] = (uint8_t)(record[4] & ~2u);
        }
    }
}

void terrain_cell_quad_geometry_emit(TerrainCellQuadEmitGrid* grid,
                                     uint32_t x, uint32_t y,
                                     const float vertices[4][3])
{
    static const uint32_t triangle_indices[6] = {
        0u, 1u, 3u, 1u, 2u, 3u
    };
    uint32_t index;
    uint8_t* record;
    uint8_t flags;
    int quad;
    int flipped;
    float ordered[4][3];
    uint32_t i;
    if (grid == NULL || vertices == NULL || grid->records == NULL ||
        x >= grid->width || y >= grid->height) return;
    index = y * grid->width + x;
    record = grid->records + index * 8u;
    flags = record[4];
    quad = (int)*(uint32_t*)record;
    if ((flags & 1u) != 0u) {
        if (grid->hooks.create_quad == NULL) return;
        quad = grid->hooks.create_quad(grid->hooks.context, index,
                                       triangle_indices);
        *(uint32_t*)record = (uint32_t)quad;
        if (quad != -1) {
            if (grid->hooks.quad_position_apply != NULL)
                grid->hooks.quad_position_apply(grid->hooks.context, quad,
                    grid->position_parameter);
            if (grid->hooks.quad_post_create_update != NULL)
                grid->hooks.quad_post_create_update(grid->hooks.context,
                                                     quad, 1);
            if (grid->hooks.visibility_set != NULL)
                grid->hooks.visibility_set(grid->hooks.context, quad, 1);
            flags = (uint8_t)(flags & ~1u);
        }
    }
    if (grid->dirty != NULL) *grid->dirty = 1u;
    flags = (uint8_t)(flags | 2u);
    flipped = !((vertices[2][2] <= vertices[0][2]
                 ? vertices[2][2] : vertices[0][2]) <= vertices[1][2] &&
                (vertices[2][2] <= vertices[0][2]
                 ? vertices[2][2] : vertices[0][2]) <= vertices[3][2]);
    {
        static const uint8_t clear_order[4] = { 3u, 2u, 1u, 0u };
        static const uint8_t set_order[4] = { 0u, 3u, 2u, 1u };
        const uint8_t* order = flipped ? set_order : clear_order;
        for (i = 0u; i < 4u; ++i)
            memcpy(ordered[i], vertices[order[i]], 3u * sizeof(float));
        flags = (uint8_t)((flags & ~4u) | (flipped ? 4u : 0u));
    }
    record[4] = flags;
    if (quad != -1 && grid->hooks.vertices_emit != NULL)
        grid->hooks.vertices_emit(grid->hooks.context, quad, ordered);
}

static const char* K_OLISTFILE      = "olistfile";

/* ------------------------------------------------------------------ *
 *  PRNG — VC5 LIBC.LIB rand.obj supplies the target's shared _srand/_rand
 *  entries and initial seed word (DAT_004afcc8 = 1). Portable builds retain
 *  the equivalent local LCG.
 * ------------------------------------------------------------------ */

// LIBRARY: LEGORR 0x0048e160 SYMBOL
// _srand
#if !defined(_MSC_VER) || (_MSC_VER >= 1200) || !defined(_M_IX86)
/* LIBC.LIB static data at 0x004afcc8: not annotated (finding 540). */
static uint32_t s_rand_state = 1;

void level_random_seed_set(uint32_t seed)
{
    s_rand_state = seed;
}

// LIBRARY: LEGORR 0x0048e170 SYMBOL
// _rand
int level_rand_u16(void)
{
    s_rand_state = s_rand_state * 214013u + 2531011u;
    return (((int32_t)s_rand_state) >> 16) & 0x7fff;
}
#endif

int level_random_draw15(void)
{
    return (int)level_rand_u16();
}

/* ------------------------------------------------------------------ *
 *  `.map` file container (finding 07): raw image, direct-mapped.
 * ------------------------------------------------------------------ */

MapFile* level_map_open(const char* name)
{
    int slot;
    int32_t size = 0;
    uint8_t* data;
    MapFile* m;
    if (name == NULL || name[0] == '\0') {
        return NULL;
    }
    slot = map_open2(name, &size);
    if (slot < 0) {
        return NULL;
    }
    data = level_map_slot_get((uint32_t)slot);
    if (data == NULL || size < 0x10) {
        level_map_slot_release((uint32_t)slot);
        return NULL;
    }
    m = (MapFile*)calloc(1, sizeof(MapFile));
    if (m == NULL) {
        level_map_slot_release((uint32_t)slot);
        return NULL;
    }
    m->data = data;
    m->size = size;
    m->slot = (uint32_t)slot;
    __builtin_memcpy(&m->width, data + 0x08, sizeof(uint32_t));
    __builtin_memcpy(&m->height, data + 0x0c, sizeof(uint32_t));
    return m;
}

void level_map_close(MapFile* m)
{
    if (m != NULL) {
        level_map_slot_release(m->slot);
        free(m);
    }
}

uint16_t level_map_cell(const MapFile* m, int x, int y)
{
    if (m == NULL || x < 0 || y < 0 || x >= m->width || y >= m->height) {
        return 0;
    }
    uint16_t v;
    __builtin_memcpy(&v, m->data + 0x10 + ((size_t)y * m->width + x) * 2, 2);
    return v;
}

/* ------------------------------------------------------------------ *
 *  TerrainCreate @ 0x44e0c0 — scoped port (finding 07 §"Voxel record").
 *
 *  Builds the terrain struct (grid dimensions + world scale), the parallel
 *  physical block table (per-cell stride 0x28), and a port-owned indexed
 *  mesh record window whose vertex reads use the same 0x82470 copier as the
 *  retail DDi mesh path. Shader and COM surface ownership remain in the
 *  renderer boundary.
 * ------------------------------------------------------------------ */

int terrain_roof_owner_initialize(LevelState* level)
{
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    TerrainState* terrain = level != NULL ? (TerrainState*)level->ptr_terrain : NULL;
    void* root = scene_root_object_get();
    if (terrain == NULL || terrain->owner_level != level || root == NULL ||
        terrain->roof_scene != NULL ||
        indexed_surface_scene_owner_get() != NULL) return 0;
    if (!indexed_surface_resource_allocate(root, terrain->width,
                                           terrain->height)) return 0;
    void* scene = indexed_surface_scene_owner_get();
    if (scene == NULL) { graphics_overlay_resources_release(); return 0; }
    free(terrain->derived_mesh.records);
    terrain->derived_mesh.records = indexed_surface_geometry_records_get();
    terrain->roof_scene = scene;
    terrain->roof_root = root;
    graphics_overlay_visibility_set(1u);
    return 1;
#else
    (void)level;
    return 0;
#endif
}

int terrain_roof_visibility_get(const LevelState* level)
{
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    typedef long (__stdcall *ParentGet)(void*, void**);
    typedef unsigned long (__stdcall *Release)(void*);
    const TerrainState* t = level != NULL ? (const TerrainState*)level->ptr_terrain : NULL;
    void* channels[3];
    void* parent = NULL;
    if (t == NULL || t->roof_scene == NULL ||
        t->roof_scene != indexed_surface_scene_owner_get() ||
        t->roof_root != scene_root_object_get()) return -1;
    memcpy(channels, t->roof_scene, sizeof(channels));
    if (((ParentGet)(*(void***)channels[1])[0x5c / 4])(
            channels[1], &parent) != 0 || parent == NULL) return -1;
    int result = parent == channels[0] ? 1 :
                 (parent == channels[2] ? 0 : -1);
    ((Release)(*(void***)parent)[2])(parent);
    return result;
#else
    (void)level;
    return -1;
#endif
}

int terrain_cell_roof_vertices_get(const LevelState* level, uint32_t x,
                                    uint32_t y, float out[12])
{
    static const uint8_t clear_order[4] = { 3u, 2u, 1u, 0u };
    static const uint8_t set_order[4] = { 0u, 3u, 2u, 1u };
    const TerrainState* t = level != NULL ? (const TerrainState*)level->ptr_terrain : NULL;
    const uint8_t* order;
    size_t index;
    uint32_t corner;
    if (t == NULL || out == NULL || t->derived_vertices == NULL ||
        t->derived_mesh.records == NULL || x >= t->dim[0] || y >= t->dim[1]) return 0;
    index = (size_t)y * t->derived_mesh.width + x;
    if ((t->derived_mesh.records[index * 8u + 4u] & 1u) != 0u) return 0;
    order = (t->derived_mesh.records[index * 8u + 4u] & 4u) != 0u
        ? set_order : clear_order;
    for (corner = 0u; corner < 4u; ++corner) {
        const float* src = t->derived_vertices + index * 12u + corner * 3u;
        out[order[corner] * 3u] = src[0];
        out[order[corner] * 3u + 1u] = src[2];
        out[order[corner] * 3u + 2u] = src[1];
    }
    return 1 + ((t->derived_mesh.records[index * 8u + 4u] & 4u) != 0u);
}

typedef struct TerrainVertexReadRequest {
    int block_index;
    int first_vertex;
    int count;
    void* output;
} TerrainVertexReadRequest;

void terrain_indexed_mesh_cell_sync(TerrainState* terrain,
                                           size_t quad_index)
{
    static const uint8_t flipped_order[4] = { 3u, 0u, 1u, 2u };
    size_t quad_count;
    const uint8_t* order = NULL;
    uint8_t* cell;
    float* destination;
    uint32_t corner;
    if (terrain == NULL || terrain->cell_vertices == NULL ||
        terrain->indexed_mesh_vertices == NULL || terrain->blocks == NULL ||
        terrain->dim[0] == 0u || terrain->dim[1] == 0u) return;
    quad_count = (size_t)terrain->dim[0] * (size_t)terrain->dim[1];
    if (quad_index >= quad_count) return;
    cell = terrain->blocks +
        ((quad_index / (size_t)terrain->dim[0]) * terrain->dim[2] +
         quad_index % (size_t)terrain->dim[0]) * TERRAIN_BLOCK_STRIDE;
    if ((cell[0x19] & 1u) != 0u) order = flipped_order;
    destination = terrain->indexed_mesh_vertices + quad_index * 32u;
    for (corner = 0u; corner < 4u; ++corner) {
        const uint32_t source_corner = order != NULL
            ? order[corner] : corner;
        const float* source = terrain->cell_vertices +
            quad_index * 12u + source_corner * 3u;
        float* vertex = destination + corner * 8u;
        /* The retail indexed reader copies position, normal, and UV words
         * from 0x20-byte vertices. TerrainCreate owns positions here; the
         * other five words have no producer in the portable terrain path. */
        vertex[0] = source[0];
        vertex[1] = source[2];
        vertex[2] = source[1];
    }
}

static void* terrain_indexed_mesh_get(void* context, void* object)
{
    TerrainIndexedMeshObject* mesh_object =
        (TerrainIndexedMeshObject*)object;
    (void)context;
    if (mesh_object == NULL) return NULL;
    return mesh_object->mesh_record;
}

static uint32_t terrain_indexed_mesh_entry_get(void* context, void* mesh,
                                               unsigned int index)
{
    TerrainState::TerrainIndexedMeshRecord* record =
        (TerrainState::TerrainIndexedMeshRecord*)mesh;
    uint32_t flags;
    uint8_t* block;
    (void)context;
    if (record == NULL || record->blocks == NULL ||
        index >= record->block_count) return 0u;
    block = (uint8_t*)record->blocks + (size_t)index * 0x70u;
    memcpy(&flags, block + 0x6cu, sizeof(flags));
    return flags & 1u;
}

static const TerrainDetailLookupOps s_terrain_detail_lookup_ops = {
    NULL,
    terrain_indexed_mesh_get,
    terrain_indexed_mesh_entry_get,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

static uint32_t terrain_indexed_mesh_vertex_data_read(
    void* context, void* mesh, const void* request_pointer)
{
    const TerrainVertexReadRequest* request =
        (const TerrainVertexReadRequest*)request_pointer;
    (void)context;
    if (mesh == NULL || request == NULL) return 0u;
    indexed_geometry_data_copy_to_output(mesh, request->block_index,
        request->first_vertex, request->count, request->output);
    return 0u;
}

static const LwoLayerDispatchOps s_terrain_indexed_mesh_dispatch_ops = {
    NULL,
    terrain_indexed_mesh_get,
    NULL,
    NULL,
    NULL,
    terrain_indexed_mesh_vertex_data_read,
    NULL,
    NULL,
    NULL
};

void terrain_create_cell_surface_vectors_rebuild(TerrainState* terrain,
                                                         uint32_t quad_index,
                                                         uint32_t record_index,
                                                         int flipped)
{
    float* vertices;
    float ordered_vertices[12];
    float first[3];
    float second[3];
    uint8_t* record;
    static const uint8_t forward[4] = { 3u, 0u, 1u, 2u };
    static const uint8_t identity[4] = { 0u, 1u, 2u, 3u };
    const uint8_t* order = flipped != 0 ? forward : identity;
    uint32_t corner;
    if (terrain == NULL || terrain->blocks == NULL ||
        terrain->cell_vertices == NULL ||
        quad_index >= terrain->dim[0] * terrain->dim[1] ||
        record_index >= terrain->dim[2] * terrain->dim[3]) return;
    vertices = terrain->cell_vertices + (size_t)quad_index * 12u;
    record = terrain->blocks + (size_t)record_index * TERRAIN_BLOCK_STRIDE;
    for (corner = 0u; corner < 4u; ++corner) {
        const float* source = vertices + order[corner] * 3u;
        ordered_vertices[corner * 3u] = source[0];
        ordered_vertices[corner * 3u + 1u] = source[2];
        ordered_vertices[corner * 3u + 2u] = source[1];
    }
    surface_triangle_normal_calculate(first, ordered_vertices,
                                      ordered_vertices + 3,
                                      ordered_vertices + 9);
    surface_triangle_normal_calculate(second, ordered_vertices + 3,
                                      ordered_vertices + 6,
                                      ordered_vertices + 9);
    memcpy(record, first, sizeof(first));
    memcpy(record + 0x0cu, second, sizeof(second));
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void* terrain_create(const char* surface_map, float blocksize)
{
    return terrain_create_retail(NULL, surface_map, blocksize, 0.0f);
}
#endif /* !LEGORRR_BACKEND_WINAPI */

void terrain_destroy(void* handle)
{
    TerrainState* t = (TerrainState*)handle;
    if (t != NULL) {
        if (t->owner_level != NULL &&
            t->owner_level->ptr_tile_mesh_grid != NULL) {
            TerrainTileMeshGrid* tile_grid =
                t->owner_level->ptr_tile_mesh_grid;
            if (tile_grid->surface_text_map == t->variant_window) {
                tile_grid->surface_text_map = NULL;
                tile_grid->surface_text_map_insert = NULL;
            }
            if (tile_grid->surface_grid != NULL &&
                tile_grid->surface_grid->terrain_window ==
                    t->variant_window) {
                terrain_surface_grid_terrain_window_set(
                    tile_grid->surface_grid, NULL);
            }
        }
        {
            TerrainSurfaceGrid* surface = path_record_surface_grid_get();
            if (surface != NULL &&
                surface->terrain_window == t->variant_window) {
                terrain_surface_grid_terrain_window_set(surface, NULL);
            }
        }
        if (t->roof_scene != NULL) {
            if (indexed_surface_scene_owner_get() == t->roof_scene)
                graphics_overlay_resources_release();
            t->derived_mesh.records = NULL; /* released global owner table */
            t->roof_scene = t->roof_root = NULL;
        }
        t->owner_level = NULL;
        if (t->detail_surface != NULL) {
            free(t->detail_surface->surface_values);
            free(t->detail_surface);
        }
        free(t->derived_mesh.records);
        free(t->derived_vertices);
        free(t->indexed_mesh_vertices);
        free(t->indexed_mesh_blocks);
        free(t->cell_vertices);
        free(t->cell_detail_flags);
        free(t->variant_window);
        free(t->blocks);
        free(t);
    }
}

int terrain_cell_base_vertices_get(const void* handle, uint32_t x,
                                   uint32_t y, float vertices[12])
{
    const TerrainState* terrain = (const TerrainState*)handle;
    if (terrain == NULL || vertices == NULL ||
        terrain->cell_vertices == NULL || x >= terrain->dim[0] ||
        y >= terrain->dim[1]) return 0;
    memcpy(vertices, terrain->cell_vertices +
        ((size_t)y * terrain->dim[0] + x) * 12u, 12u * sizeof(float));
    return 1;
}

/* The retail geometry buffer stores four vertex records per quad in
 * row-major order. GridCellReadSurfaceValues copies the first three words of
 * each record verbatim; these are the mesh's X, Z, height words. The
 * +0x19 orientation bit reflects the cyclic vertex rotation already
 * performed on the retail geometry object. */
int terrain_cell_surface_values_available(const void* handle, int x, int y)
{
    const TerrainState* terrain = (const TerrainState*)handle;
    if (terrain == NULL || x < 0 || y < 0 ||
        (uint32_t)x >= terrain->dim[0] ||
        (uint32_t)y >= terrain->dim[1]) return 0;
    if (terrain->cell_vertices != NULL) return 1;
    if (terrain->owner_level != NULL) {
        float vertices[4][3];
        return scene_render_terrain_cell_vertices_get(
            terrain->owner_level, (uint32_t)x, (uint32_t)y, vertices);
    }
    return 0;
}

/* TerrainCreate initializes each indexed record's +0x6c bit, and
 * GridSurfaceWriteAtXY updates that bit. This host predicate is used by
 * entity eligibility and does not alter GridCellSetPayloadIndex's void ABI. */
int grid_cell_detail_enabled_get(void* terrain_handle, int x, int y)
{
    const TerrainState* terrain = (const TerrainState*)terrain_handle;
    size_t cell_index;
    uint32_t flags;
    if (terrain == NULL || x < 0 || y < 0 ||
        (uint32_t)x >= terrain->dim[0] || (uint32_t)y >= terrain->dim[1])
        return 0;
    if (terrain->cell_detail_flags == NULL) return 0;
    cell_index = (size_t)(uint32_t)y * (size_t)terrain->dim[0]
        + (uint32_t)x;
    if (terrain->indexed_mesh_blocks != NULL) {
        memcpy(&flags, terrain->indexed_mesh_blocks + cell_index * 0x70u +
               0x6cu, sizeof(flags));
        return (flags & 1u) != 0u;
    }
    return (terrain->cell_detail_flags[cell_index] & 1u) != 0u;
}

void terrain_detail_enable_set(void* terrain_handle, uint32_t index,
                               int enabled)
{
    TerrainState* terrain = (TerrainState*)terrain_handle;
    size_t quad_count;
    if (terrain == NULL || terrain->cell_detail_flags == NULL ||
        terrain->dim[0] == 0u || terrain->dim[1] == 0u)
        return;
    quad_count = (size_t)terrain->dim[0] * (size_t)terrain->dim[1];
    if ((size_t)index >= quad_count) return;
    if (enabled != 0)
        terrain->cell_detail_flags[index] |= 1u;
    else
        terrain->cell_detail_flags[index] &= ~1u;
    if (terrain->indexed_mesh_blocks != NULL) {
        uint8_t* block = terrain->indexed_mesh_blocks +
            (size_t)index * 0x70u;
        uint32_t flags;
        memcpy(&flags, block + 0x6cu, sizeof(flags));
        if (enabled != 0) flags |= 1u;
        else flags &= ~1u;
        memcpy(block + 0x6cu, &flags, sizeof(flags));
    }
}

TerrainSurfaceGrid* terrain_camera_detail_grid_get(LevelState* level)
{
    TerrainState* terrain = level != NULL
        ? (TerrainState*)level->ptr_terrain : NULL;
    TerrainSurfaceGrid* grid;
    size_t count;
    if (terrain == NULL || terrain->owner_level != level ||
        terrain->indexed_mesh_blocks == NULL ||
        terrain->cell_detail_flags == NULL || terrain->dim[0] == 0u ||
        terrain->dim[1] == 0u) {
        fprintf(stderr, "camera-diag: grid prerequisites level=%p terrain=%p owner=%p mesh=%p flags=%p dims=%u,%u\n",
            level, terrain, terrain ? terrain->owner_level : NULL,
            terrain ? terrain->indexed_mesh_blocks : NULL,
            terrain ? terrain->cell_detail_flags : NULL,
            terrain ? terrain->dim[0] : 0u, terrain ? terrain->dim[1] : 0u);
        fflush(stderr);
        return NULL;
    }
    if (terrain->detail_surface != NULL) return terrain->detail_surface;
    count = (size_t)terrain->dim[0] * terrain->dim[1];
    grid = (TerrainSurfaceGrid*)calloc(1u, sizeof(TerrainSurfaceGrid));
    if (grid == NULL) { fprintf(stderr, "camera-diag: grid allocation failed\n"); fflush(stderr); return NULL; }
    grid->surface_values = (uint8_t*)malloc(count);
    if (grid->surface_values == NULL) { fprintf(stderr, "camera-diag: grid values allocation failed count=%lu\n", (unsigned long)count); fflush(stderr); free(grid); return NULL; }
    memset(grid->surface_values, 1, count);
    grid->width = terrain->dim[0];
    grid->height = terrain->dim[1];
    grid->record_width = terrain->dim[2];
    grid->cell_records = terrain->blocks;
    grid->terrain_window = terrain->variant_window;
    grid->owner_level = level;
    terrain->detail_surface = grid;
    return grid;
}

/* LAB_00429110 is the transient-effect callback, not palette helper429160.
 * The native callback is synchronous between clearing and queueing a cell. */
static LevelState* s_camera_detail_callback_level;
static int s_camera_detail_callback_ready;
static void terrain_camera_detail_cell_callback(int x, int y)
{
    uint8_t* cell = level_voxel(s_camera_detail_callback_level, x, y);
    uint32_t flags;
    void* effect;
    if (cell == NULL) return;
    memcpy(&flags, cell + 8, sizeof(flags));
    if ((flags & 0x4000u) == 0u) return;
    effect = cell_transient_effect_get(s_camera_detail_callback_level, cell);
    if (effect == NULL) { fprintf(stderr, "camera-diag: effect missing cell=%d,%d flags=%08lx\n", x, y, (unsigned long)flags); fflush(stderr); s_camera_detail_callback_ready = 0; return; }
    transient_effect_enable_set(effect, 0);
}

int terrain_camera_detail_collect(LevelState* level, const float position[3],
                                  const float forward[3], int primary)
{
    TerrainSurfaceGrid* grid = terrain_camera_detail_grid_get(level);
    const TerrainState* terrain = level != NULL
        ? (const TerrainState*)level->ptr_terrain : NULL;
    float world_x, world_y;
    int center_x, center_y, dx, dy;
    const int radius = primary ? 8 : 2;
    uint32_t required = 0u;
    LevelState* prior_callback_level;
    int ready;
    if (grid == NULL || position == NULL ||
        (primary && forward == NULL)) { fprintf(stderr, "camera-diag: collect arguments grid=%p position=%p forward=%p primary=%d\n", grid, position, forward, primary); fflush(stderr); return 0; }
    /* Native42921d reads TerrainCreate+10 spacing, then exact49e2d4=3.
     * It uses the follow position plus the actual eye's world forward. */
    world_x = position[0]; world_y = position[1];
    if (primary) {
        float lead = terrain->blocksize * 3.0f;
        world_x += forward[0] * lead;
        world_y += forward[1] * lead;
    }
    /* 44f6d0 writes cell coordinates even when outside physical bounds;
     * the radius scan performs the per-cell admission. */
    (void)terrain_world_xy_to_cell(terrain, world_x, world_y,
                                    &center_x, &center_y);
    if (center_x < INT_MIN + radius || center_x > INT_MAX - radius ||
        center_y < INT_MIN + radius || center_y > INT_MAX - radius) {
        fprintf(stderr, "camera-diag: center bounds x=%d y=%d radius=%d\n", center_x, center_y, radius); fflush(stderr);
        return 0;
    }
    for (dy = -radius; dy <= radius; ++dy)
        for (dx = -radius; dx <= radius; ++dx)
            if (dx * dx + dy * dy <= radius * radius &&
                center_x + dx >= 0 && center_y + dy >= 0 &&
                (uint32_t)(center_x + dx) < grid->width &&
                (uint32_t)(center_y + dy) < grid->height) ++required;
    /* Never clear a cell whose restoration cannot be retained. The real
     * radius8 plus radius2 passes fit the native queue's512 entries. */
    if (grid->changed_count > TERRAIN_GRID_CHANGE_CAPACITY ||
        required > TERRAIN_GRID_CHANGE_CAPACITY - grid->changed_count) {
        fprintf(stderr, "camera-diag: queue capacity changed=%lu required=%lu\n", (unsigned long)grid->changed_count, (unsigned long)required); fflush(stderr);
        return 0;
    }
    prior_callback_level = s_camera_detail_callback_level;
    s_camera_detail_callback_level = level;
    s_camera_detail_callback_ready = 1;
    terrain_surface_clear_circle(grid, center_x, center_y, radius,
        primary ? terrain_camera_detail_cell_callback : NULL);
    ready = s_camera_detail_callback_ready;
    s_camera_detail_callback_level = prior_callback_level;
    return ready;
}

void terrain_camera_detail_restore(LevelState* level)
{
    TerrainState* terrain = level != NULL
        ? (TerrainState*)level->ptr_terrain : NULL;
    if (terrain != NULL && terrain->owner_level == level &&
        terrain->detail_surface != NULL)
        terrain_cell_change_queue_flush(terrain->detail_surface);
}

int terrain_surface_height_get(const void* handle, int x, int y)
{
    const TerrainState* t = (const TerrainState*)handle;
    if (t == NULL || t->blocks == NULL || x < 0 || y < 0
        || x >= t->width || y >= t->height) {
        return 0;
    }
    return t->blocks[((size_t)y * (size_t)t->width + (size_t)x)
                      * TERRAIN_BLOCK_STRIDE + 0x1a];
}

/* Presentation fallback uses TerrainCreate's actual fourth argument, including
 * zero and negative scales; block spacing is an independent quantity. */
float terrain_height_scale_get(const void* handle)
{
    const TerrainState* t = (const TerrainState*)handle;
    return t != NULL ? t->terrain_value : 0.0f;
}

float terrain_blocksize_get(const void* handle)
{
    const TerrainState* t = (const TerrainState*)handle;
    return t != NULL ? t->blocksize : 0.0f;
}

/* MapCellToWorldCenter (FUN_0044f640): use the terrain's retained grid
 * transform and return the center of a cell in world x/y coordinates. */
int terrain_cell_world_center_xy_get(const void* handle, int x, int y,
                                     float position_xy[2])
{
    const TerrainState* t = (const TerrainState*)handle;
    if (t == NULL || position_xy == NULL || x < 0 || y < 0 ||
        x >= t->width || y >= t->height) return 0;
    return terrain_cell_world_center_xy_transform(handle, x, y, position_xy);
}

int terrain_cell_world_center_xy_transform(const void* handle, int x, int y,
                                           float position_xy[2])
{
    const TerrainState* t = (const TerrainState*)handle;
    if (t == NULL || position_xy == NULL) return 0;
    position_xy[0] = t->blocksize * 0.5f + (float)x * t->blocksize
        + t->world_off_x;
    position_xy[1] = t->world_off_y -
        (t->blocksize * 0.5f + (float)y * t->blocksize);
    return 1;
}

/* WorldToTerrainCellBoundsCheck (FUN_0044f700): invert the vertex-origin
 * transform, convert through the target CRT, then check physical quads.
 * The half-block offset belongs only to MapCellToWorldCenter. */
int terrain_world_xy_to_cell(const void* handle, float x, float y,
                             int* cell_x, int* cell_y)
{
    const TerrainState* t = (const TerrainState*)handle;
    int mapped_x, mapped_y;
    if (t == NULL || t->blocksize == 0.0f) return 0;
    mapped_x = crt_float_to_long(
        (x - t->world_off_x) / t->blocksize);
    mapped_y = crt_float_to_long(
        (t->world_off_y - y) / t->blocksize);
    if (cell_x != NULL) *cell_x = mapped_x;
    if (cell_y != NULL) *cell_y = mapped_y;
    /* 0x44f700 bounds physical quads (dim[0/1] = width/height - 1),
     * after writing the truncated results, even when outside the grid. */
    return mapped_x >= 0 && mapped_y >= 0 &&
        (uint32_t)mapped_x < t->dim[0] &&
        (uint32_t)mapped_y < t->dim[1];
}

/* TerrainCellChangedFlagSet (FUN_0044efb0): the physical cell dirty bit is
 * available even though TerrainState does not yet own the renderer's global
 * dirty word or mesh handles. */
void terrain_cell_changed_flag_set(void* terrain, int x, int y)
{
    TerrainState* t = (TerrainState*)terrain;
    if (t == NULL || t->blocks == NULL || x < 0 || y < 0 ||
        x >= t->width || y >= t->height) return;
    t->blocks[((size_t)y * (size_t)t->width + (size_t)x) *
              TERRAIN_BLOCK_STRIDE + 0x19] |= 4u;
    t->dirty_flags |= 2u;
}

/* First half of TerrainDirtyCellUpdate (FUN_0044eff0). The ticker owns the
 * later dirty-list interpolation (FUN_00450b60) and transition dispatch
 * (FUN_0044f090); this level-owned pass uses the actual TerrainState records
 * instead of the detached EngineState::sim_grid mirror. */
void terrain_dirty_cell_vectors_refresh(LevelState* level)
{
    TerrainState* terrain;
    TerrainSurfaceGrid* surface;
    uint32_t y;
    if (level == NULL) level = level_current_owner_get();
    terrain = level != NULL ? (TerrainState*)level->ptr_terrain : NULL;
    if (terrain == NULL || terrain->blocks == NULL ||
        (terrain->dirty_flags & 2u) == 0u) return;
    surface = level_surface_grid_get(level);
    /* FUN_00450f30 writes renderer-owned surface vectors. Keep the dirty
     * records pending if that owner has not been installed yet. */
    if (surface == NULL) return;
    for (y = 0u; y < (uint32_t)terrain->height; ++y) {
        uint32_t x;
        for (x = 0u; x < (uint32_t)terrain->width; ++x) {
            uint8_t* record = terrain->blocks +
                ((size_t)y * (size_t)terrain->width + x) *
                    TERRAIN_BLOCK_STRIDE;
            if ((record[0x19] & 4u) == 0u) continue;
            terrain_cell_surface_vectors_initialize(surface, x, y);
            record[0x19] &= (uint8_t)~4u;
        }
    }
    terrain->dirty_flags &= ~2u;
}

/* ------------------------------------------------------------------ *
 *  Layer loaders — the ten grid importers (finding 07).
 * ------------------------------------------------------------------ */

uint8_t* voxel_at(LevelState* s, int x, int y)
{
    int w = level_int(s, LEVEL_OFF_WIDTH);
    return (uint8_t*)level_load_ptr(s, LEVEL_OFF_VOXELS)
           + ((size_t)y * w + x) * LEVEL_VOXEL_STRIDE;
}

unsigned int level_predug_coordinate_count_get(void)
{
    return s_predug_coordinate_count;
}
int level_predug_coordinate_get(unsigned int index, int cell_xy_out[2])
{
    if (cell_xy_out == NULL || index >= s_predug_coordinate_count) return 0;
    cell_xy_out[0] = s_predug_coordinates[index][0];
    cell_xy_out[1] = s_predug_coordinates[index][1];
    return 1;
}
void level_predug_coordinate_pool_clear(void)
{
    memset(s_predug_coordinates, 0, sizeof(s_predug_coordinates));
    s_predug_coordinate_count = 0u;
}

TerrainActivityBuildRecordModel s_terrain_activity_build_records[256];

void* terrain_activity_build_record_get(LevelState* level, int x, int y)
{
    for (int i = 0; level != NULL && i < 256; ++i) {
        TerrainActivityBuildRecordModel* record =
            &s_terrain_activity_build_records[i];
        if (record->level == level && record->x == x && record->y == y) {
            return record;
        }
    }
    return NULL;
}

/* FUNCTION: LEGORR 0x004314c0 */
void terrain_activity_build_record_release(void* value)
{
    for (int i = 0; value != NULL && i < 256; ++i) {
        if (value == &s_terrain_activity_build_records[i]) {
            s_terrain_activity_build_records[i].level = NULL;
            s_terrain_activity_build_records[i].enabled = 0;
            return;
        }
    }
}

/* FUNCTION: LEGORR 0x00431530 */
void terrain_activity_build_records_release_all(LevelState* level)
{
    for (int i = 0; level != NULL && i < 256; ++i) {
        if (s_terrain_activity_build_records[i].level == level) {
            terrain_activity_build_record_release(
                &s_terrain_activity_build_records[i]);
        }
    }
}

/* Keep the retail per-cell entry available to the path-record registration
 * and teardown routines. The map-load sweep remains on the private helper
 * above so it can preserve the original float coordinate call shape. */
void level_cell_init_pass(LevelState* level, int x, int y)
{
    map_cell_record_grid_activate(level);
    cell_init_pass(level, (float)x, (uint32_t)y);
}

/* ------------------------------------------------------------------ *
 *  MapLoadPipeline @ 0x42b510.
 * ------------------------------------------------------------------ */

/* ------------------------------------------------------------------ *
 *  Level load @ 0x429890 — config read + pipeline + post-load init.
 *
 *  Config paths are rooted at the file's first record (upstream block
 *  parser keeps the `Lego` root, so a level key reads as
 *  "Lego*::<levelname>::<key>", matching the corpus's BlockQueryDefault
 *  path-building "%s%s%s").
 * ------------------------------------------------------------------ */

/* Build the config path "<root>::<level>::<key>", rooted at the file's first
 * record name (`util_root_name`) — the block parser keeps the top-level
 * `Lego {…}` record as the root, and the corpus's LevelLoad paths are
 * relative to it (finding 12, FUN_004792e0). */

/* The shipped Lego.cfg nests every level group under the root's `Levels`
 * container (`Lego* { … Levels { … Level01 { … } } }`), so the flat
 * `root::level::key` path misses. Resolve through `Levels` first, then fall
 * back to the direct shape (the test fixtures and any flatter config).
 * Returns the resolving path (into out) or NULL. */
const char* cfg_resolve_path(BlockFile* cfg, const char* level,
                                    const char* key, char* out, size_t out_sz)
{
    return util_level_cfg_path(cfg, level, key, out, out_sz);
}

int cfg_typed(BlockFile* cfg, const char* level, const char* key, int dflt)
{
    char path[512];
    if (cfg_resolve_path(cfg, level, key, path, sizeof(path)) == NULL) {
        return dflt;
    }
    /* FUN_004790d0 delegates to ConfigBooleanGet @ 0x477710: missing or
     * unrecognized values answer 2 instead of parsing a numeric string. */
    return block_get_bool_path(cfg, path);
}

/* Retail applies NoGather after the level intro. Keep the config decision
 * shared by the frameloop and next-level caller, which own that post-intro
 * point in the portable runtime. */
void level_no_gather_config_setup(BlockFile* cfg, const char* level_name)
{
    int mode = cfg_typed(cfg, level_name, "NoGather", 0) == 1 ? 1 : 0;
    level_no_gather_object_setup(mode);
}

void level_destroy(LevelState* s)
{
    GameContext* game;
    int was_current;
    uint32_t transition_index;
    if (s == NULL) {
        return;
    }
    deferred_olist_entity_level_end(s);
    game = entity_pool_callback_context_get();
    was_current = game != NULL && game->level == s;
    objective_active_level_clear(s);
    if (was_current) {
        /* PendingLevelName calls AdvisorDisplayStateResourcesRelease before
         * freeing the active level. This owns tutorial script and advisor
         * token resources, whose pointers are process-wide in the port. */
        (void)tutorial_display_resources_release();
        game->level = NULL;
    }
    unported_ledger_stubs::dependency_grid_level_release(s);
    terrain_cell_quad_run_stage_level_release(s);
    cell_transient_effect_ports_release(s);
    map_cell_record_grid_release(s);
    level_field_grid_buffer_release(s);
    /* PendingLevelName releases the level's TerrainActivityBuildRecords
     * before freeing the level. These records are kept in a process-wide
     * sidecar, so clear the entries tagged with this owner here as well. */
    terrain_activity_build_records_release_all(s);
    for (transition_index = 0u; transition_index < 10u;
         ++transition_index) {
        LevelTerrainMaterialTransitionSlot* slot =
            &s->material_transition_slots[transition_index];
        if ((slot->flags & 1u) != 0u &&
            s_material_transition_mesh_hook != NULL) {
            (void)s_material_transition_mesh_hook(
                s_material_transition_mesh_context, s, transition_index, 2,
                slot->cell_x, slot->cell_y, NULL, 0.0f);
        }
        slot->flags &= ~1u;
    }
    scene_render_terrain_level_release(s);
    camera_spawn_level_block_bind(NULL);
    lava_erosion_level_context_release(s);
    /* Level teardown's corpus path teardown includes ReleaseAllGroupRecords
     * (0x4099d0), which owns the global descriptor and payload chain. */
    path_record_release_all();
    path_record_cell_links_release(s, NULL);
    path_record_surface_grid_bind(NULL);
    /* PendingLevelName releases the 55 coordinate chains before the level.
     * These heads are typed sidecars, not the raw retail +0x114 window. */
    for (int coordinate_list = 0; coordinate_list < 0x37;
         ++coordinate_list) {
        void* node = s->pool_heads[coordinate_list];
        while (node != NULL) {
            void* next = *(void**)((char*)node + 8);
            engine_heap_free(node);
            node = next;
        }
        s->pool_heads[coordinate_list] = NULL;
    }
    if (s->ptr_tile_mesh_grid != NULL) {
        if (s->ptr_tile_mesh_grid->surface_grid != NULL &&
            s->ptr_tile_mesh_grid->surface_grid->owner_level == s) {
            s->ptr_tile_mesh_grid->surface_grid->owner_level = NULL;
            terrain_surface_grid_terrain_window_set(
                s->ptr_tile_mesh_grid->surface_grid, NULL);
        }
        terrain_texture_set_release(s->ptr_tile_mesh_grid);
        free(s->ptr_tile_mesh_grid);
        s->ptr_tile_mesh_grid = NULL;
    }
    free(s->ptr_voxels);
    free(s->ptr_layerl);
    free(s->ptr_full_name);
    terrain_destroy(s->ptr_terrain);
    free(s->ptr_next);
    /* ptr_name is borrowed from the caller (the level *name* passed to
     * level_load); never freed here. */
    __builtin_memset(s, 0, sizeof(*s));
    free(s);
    /* PendingLevelName resets the shared interface state after releasing
     * the active level. Discarding an uninstalled load must not reset it. */
    if (was_current) lego::level_state_counters_clear();
}

int level_width(const LevelState* s) { return s ? level_int(s, LEVEL_OFF_WIDTH) : 0; }
int level_height(const LevelState* s) { return s ? level_int(s, LEVEL_OFF_HEIGHT) : 0; }

/* TerrainDescriptorFindExact (FUN_004090c0): when a pair key is supplied,
 * both words must match; otherwise the optional single key is compared
 * against the node's single-key field. The global list head is threaded in
 * as `list`. */
TerrainDescriptorNode* terrain_descriptor_find_exact(
    TerrainDescriptorNode* list, const uint32_t* pair_key,
    const uint32_t* single_key)
{
    for (TerrainDescriptorNode* node = list; node != NULL; node = node->next) {
        if (pair_key != NULL) {
            if (node->key_pair_first == pair_key[0]
                && node->key_pair_second == pair_key[1]) {
                return node;
            }
        }
        else if (single_key != NULL && node->key_single == *single_key) {
            return node;
        }
    }
    return NULL;
}

/* TerrainCellPredicateCheck (FUN_004090a0): the public path passes a pair
 * key and a NULL single-key alternative, then reports whether an exact
 * descriptor exists. */
int terrain_cell_predicate_check(TerrainDescriptorNode* list,
                                 const uint32_t* pair_key)
{
    return terrain_descriptor_find_exact(list, pair_key, NULL) != NULL;
}

const char* level_next_name(const LevelState* s)
{
    return s != NULL ? (const char*)s->ptr_next : NULL;
}

const char* level_full_name(const LevelState* s)
{
    return s != NULL ? (const char*)s->ptr_full_name : NULL;
}

uint8_t* level_voxel(const LevelState* s, int x, int y)
{
    if (s == NULL || x < 0 || y < 0 || x >= level_width(s) || y >= level_height(s)) {
        return NULL;
    }
    return voxel_at((LevelState*)s, x, y);
}

/* Level grid coordinates use the CRT's x87 float-to-long rounding. */
int level_cell_of(float v)
{
    return crt_float_to_long(v);
}

int level_walkable(const LevelState* s, int cx, int cy)
{
    if (s == NULL) {
        return 0;
    }
    /* Corpus `param < w - 2U` unsigned compare: negatives fail too. */
    int w = level_width(s), h = level_height(s);
    if ((unsigned)cx >= (unsigned)w - 2u || (unsigned)cy >= (unsigned)h - 2u) {
        return 0;
    }
    const uint8_t* rec = level_voxel(s, cx, cy);
    if (rec == NULL) {
        return 0;
    }
    uint32_t flags;
    __builtin_memcpy(&flags, rec + 0x08, sizeof(flags));
    int type = (int)(signed char)rec[2];
    return (flags & TILE_FLAG_WALKABLE) != 0 && (flags & TILE_FLAG_BLOCKING) == 0 &&
           type != 1 && type != 7 && type != 11;
}

int entity_move_destination_cell_usable_current(const struct EntitySlot* e,
                                                uint32_t cx, uint32_t cy)
{
    return entity_move_destination_cell_usable(e, level_current_owner_get(),
                                               (int)cx, (int)cy);
}

int terrain_cell_allows_placement_current(const struct EntitySlot* e,
                                          uint32_t cx, uint32_t cy)
{
    return terrain_cell_allows_placement(e, level_current_owner_get(),
                                         (int)cx, (int)cy);
}

void level_tile_mark(LevelState* s, int cx, int cy, int set)
{
    uint8_t* rec = level_voxel(s, cx, cy);
    if (rec == NULL) {
        return;
    }
    uint32_t* flags = (uint32_t*)(rec + 0x08);
    if (set != 0) {
        *flags |= TILE_FLAG_OCCUPIED;
    } else {
        *flags &= ~TILE_FLAG_OCCUPIED;
    }
}

void level_tile_mark_current(const int cell_xy[2], int set)
{
    level_tile_mark_current_target(cell_xy, set);
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
int level_tile_occupied(const LevelState* s, int cx, int cy)
{
    const uint8_t* rec = level_voxel(s, cx, cy);
    if (rec == NULL) {
        return 1; /* no handle / no cell: cannot rule a reservation out */
    }
    uint32_t flags;
    __builtin_memcpy(&flags, rec + 0x08, sizeof(flags));
    return (flags & TILE_FLAG_OCCUPIED) != 0;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

int level_tile_type(const LevelState* s, int cx, int cy)
{
    const uint8_t* rec = level_voxel(s, cx, cy);
    return rec != NULL ? (int)(signed char)rec[2] : -1;
}

/* --- terrain-cell flag primitives ----------------------------------------
 * Core readers over the voxel record. The corpus family computes the record
 * address with no bounds check at all. Bounded variants below preserve
 * their own dim-minus-one checks; unchecked target leaves use the raw
 * row-major helper where they do not have that gate. */

uint32_t cell_flag_word(const LevelState* s, int cx, int cy)
{
    const uint8_t* rec = level_voxel(s, cx, cy);
    uint32_t flags = 0;
    if (rec != NULL) {
        __builtin_memcpy(&flags, rec + 0x08, sizeof(flags));
    }
    return flags;
}

static uint32_t cell_secondary_word(const LevelState* s, int cx, int cy)
{
    const uint8_t* rec = level_voxel(s, cx, cy);
    uint32_t word = 0;
    if (rec != NULL) {
        __builtin_memcpy(&word, rec + 0x0C, sizeof(word));
    }
    return word;
}

/* The target's coordinate-pair leaf readers do not apply grid bounds. Keep
 * their 32-bit row-major address calculation distinct from level_voxel(),
 * whose checked access is useful to higher-level portable code. */
uint8_t* cell_record_unchecked(LevelState* s, int cx, int cy)
{
    uint8_t* voxels;
    uint32_t index;
    uint32_t byte_offset;
    if (s == NULL) return NULL;
    voxels = (uint8_t*)level_load_ptr(s, LEVEL_OFF_VOXELS);
    if (voxels == NULL) return NULL;
    index = (uint32_t)cy * (uint32_t)level_int(s, LEVEL_OFF_WIDTH) +
            (uint32_t)cx;
    byte_offset = index * (uint32_t)LEVEL_VOXEL_STRIDE;
    return voxels + byte_offset;
}

/* 0x434570: the +0x14 float belongs to the level voxel record. */
int terrain_cell_drill_work_add(LevelState* level, int x, int y,
                                float divisor, float elapsed)
{
    uint8_t* record;
    float work;
    if (divisor == 0.0f || divisor != divisor || level == NULL) return 0;
    record = cell_record_unchecked(level, x, y);
    if (record == NULL) return 0;
    memcpy(&work, record + 0x14, sizeof(work));
    work += elapsed / divisor;
    memcpy(record + 0x14, &work, sizeof(work));
    return work > 1.0f ? 1 : 0;
}

const uint8_t* cell_record_unchecked_const(const LevelState* s,
                                                   int cx, int cy)
{
    return cell_record_unchecked((LevelState*)s, cx, cy);
}

uint32_t cell_flag_word_unchecked(const LevelState* s, int cx, int cy)
{
    const uint8_t* rec = cell_record_unchecked_const(s, cx, cy);
    uint32_t flags = 0u;
    if (rec != NULL) __builtin_memcpy(&flags, rec + 0x08, sizeof(flags));
    return flags;
}

/* FUN_00432bd0: traversable bit set, blocked gate (bit 3) clear, inside
 * the unsigned dim-1 gate. The cast keeps the corpus's unsigned wrap: a
 * zero dimension lets every coordinate through. */
int level_cell_walkable_flag(const LevelState* s, int cx, int cy)
{
    const int w = level_int(s, LEVEL_OFF_WIDTH);
    const int h = level_int(s, LEVEL_OFF_HEIGHT);
    if ((unsigned)cx < ((unsigned)w - 1u) &&
        (unsigned)cy < ((unsigned)h - 1u)) {
        const uint32_t flags = cell_flag_word(s, cx, cy);
        return (flags & 0x10u) != 0 && (flags & 0x8u) == 0;
    }
    return 0;
}

/* FUN_00432c20: wave-spawn flag bit (finding 10's wave flag). */
uint32_t level_cell_flag_wave(const LevelState* s, int cx, int cy)
{
    return cell_flag_word(s, cx, cy) & 0x40000u;
}

/* FUN_00432c50: speed class 0 under the terrain block bit, else the low
 * terrain-type bits + 1. */
int level_cell_speed_class(const LevelState* s, int cx, int cy)
{
    /* FUN_00432c50 addresses the voxel row directly. In particular, it does
     * not use the bounded LevelCellFlag helpers: callers establish their own
     * cell gate before asking for the speed class. */
    const uint32_t flags = cell_flag_word_unchecked(s, cx, cy);
    if ((flags & 0x200000u) != 0) {
        return 0;
    }
    return (int)(flags & 3u) + 1;
}

/* Explicit-level adapter; unlike the mapped target, return the masked byte
 * so callers that need the individual flag bits can inspect them. */
uint32_t level_cell_mask_c0(const LevelState* s, int cx, int cy)
{
    const uint8_t* rec = level_voxel(s, cx, cy);
    return rec != NULL ? (uint32_t)rec[8] & 0xc0u : 0u;
}

/* FUN_00432e90: predug edge-wall bit (finding 07, load-time 3×3 OR). */
uint32_t level_cell_flag_edge_wall(const LevelState* s, int cx, int cy)
{
    return cell_flag_word(s, cx, cy) & 0x4000000u;
}

/* Resolve the portable owner for targets which read DAT_005560d4. Their
 * row-major record calculations remain in each mapped target body. */
LevelState* terrain_current_level_resolve(void)
{
    LevelState* level = (LevelState*)map_cell_record_grid_level_get();
    return level != NULL ? level : level_current_owner_get();
}

/* FUN_00432ec0 / FUN_00432f00: state byte equals 1 / 6. */
int level_cell_state_is_1(const LevelState* s, int cx, int cy)
{
    return level_tile_type(s, cx, cy) == 1;
}

int level_cell_state_is_6(const LevelState* s, int cx, int cy)
{
    return level_tile_type(s, cx, cy) == 6;
}

/* FUN_00432f40: terrain bits 0x18 clear; outside the gate counts as
 * clear, exactly as the corpus returns true for the early-out. */
int level_cell_mask18_clear(const LevelState* s, int cx, int cy)
{
    const int w = level_int(s, LEVEL_OFF_WIDTH);
    const int h = level_int(s, LEVEL_OFF_HEIGHT);
    if ((unsigned)cx < ((unsigned)w - 1u) &&
        (unsigned)cy < ((unsigned)h - 1u)) {
        return (cell_flag_word(s, cx, cy) & 0x18u) == 0;
    }
    return 1;
}

/* Explicit-level adapter for code which already owns a level instance. */
uint32_t level_cell_flag_4(const LevelState* s, int cx, int cy)
{
    const int w = level_int(s, LEVEL_OFF_WIDTH);
    const int h = level_int(s, LEVEL_OFF_HEIGHT);
    if ((unsigned)cx < ((unsigned)w - 1u) &&
        (unsigned)cy < ((unsigned)h - 1u)) {
        const uint8_t* voxels =
            (const uint8_t*)level_load_ptr(s, LEVEL_OFF_VOXELS);
        const uint32_t index = (uint32_t)cy * (uint32_t)w + (uint32_t)cx;
        const uint32_t flags =
            *(const uint32_t*)(voxels + index * LEVEL_VOXEL_STRIDE + 8u);
        return flags & 4u;
    }
    return 0u;
}

/* Explicit-level adapters for the two scalar mask leaves. */
uint32_t level_cell_flag_2000(const LevelState* s, int cx, int cy)
{
    return cell_flag_word(s, cx, cy) & 0x2000u;
}

uint32_t level_cell_flag_80(const LevelState* s, int cx, int cy)
{
    return cell_flag_word(s, cx, cy) & 0x80u;
}

uint32_t level_cell_flag_8000(const LevelState* s, int cx, int cy)
{
    return cell_flag_word(s, cx, cy) & 0x8000u;
}

/* FUN_00433060: secondary flag word (record+12) bit 0. */
uint32_t level_cell_secondary_bit0(const LevelState* s, int cx, int cy)
{
    return cell_secondary_word(s, cx, cy) & 1u;
}

/* Port-facing coordinate helper retains explicit LevelState ownership. */
int level_cell_occupied_at(const LevelState* s, int cx, int cy)
{
    GameContext* game = entity_pool_callback_context_get();
    if (game != NULL && game->level == s) {
        const int cell_xy[2] = { cx, cy };
        return level_cell_occupied_at_current(cell_xy) != 0u;
    }
    return (cell_flag_word(s, cx, cy) & 0x20000000u) != 0;
}

/* FUN_004330e0: terrain/wave pair mask 0x180000. */
int level_cell_mask_180000(const LevelState* s, int cx, int cy)
{
    return (cell_flag_word(s, cx, cy) & 0x180000u) != 0;
}

/* FUN_00433120: raise/clear flag bit 0x10000000 (bounds-guarded). */
void level_cell_flag_10000000_set(LevelState* s, int cx, int cy, int set)
{
    if (s == NULL || cx < 0 || cy < 0 ||
        cx >= level_int(s, LEVEL_OFF_WIDTH) ||
        cy >= level_int(s, LEVEL_OFF_HEIGHT)) {
        return;
    }
    const int w = level_int(s, LEVEL_OFF_WIDTH);
    uint8_t* rec = (uint8_t*)level_load_ptr(s, LEVEL_OFF_VOXELS)
                 + ((size_t)cy * (size_t)w + (size_t)cx) * LEVEL_VOXEL_STRIDE;
    uint32_t* flags = (uint32_t*)(rec + 0x08);
    if (set != 0) {
        *flags |= 0x10000000u;
    } else {
        *flags &= ~0x10000000u;
    }
}

/* Raise/clear flag bit 0x800000 (the cell-command terrain half; see
 * level.hpp). Bounds-guarded. */
void level_cell_flag_800000_set(LevelState* s, int cx, int cy, int set)
{
    uint8_t* rec = level_voxel(s, cx, cy);
    if (rec == NULL) {
        return;
    }
    uint32_t* flags = (uint32_t*)(rec + 0x08);
    if (set != 0) {
        *flags |= 0x800000u;
    } else {
        *flags &= ~0x800000u;
    }
}

/* --- second terrain-cell leaf cluster ------------------------------------
 * Same voxel-record model and bounds deviation. The corpus's tracking /
 * refresh companions landed later with the tracked set-family below —
 * the queues keep module statics and the per-cell FUN_0042f6f0 refresh
 * stays a TODO(corpus) seam. */

/* Explicit-level adapter. */
uint32_t level_cell_secondary_flag_100(const LevelState* s, int cx, int cy)
{
    return cell_secondary_word(s, cx, cy) & 0x100u;
}

/* Explicit-level adapter. */
uint32_t level_cell_visited_flag(const LevelState* s, int cx, int cy)
{
    return cell_secondary_word(s, cx, cy) & 2u;
}

/* Explicit-level secondary bit 0 clear helper. */
void level_cell_secondary_bit0_clear(LevelState* s, int cx, int cy)
{
    uint8_t* rec = level_voxel(s, cx, cy);
    if (rec == NULL) {
        return;
    }
    uint32_t* word = (uint32_t*)(rec + 0x0C);
    *word &= ~1u;
}

/* Explicit-level secondary bit 0 set helper. */
void level_cell_secondary_bit0_set(LevelState* s, int cx, int cy)
{
    uint8_t* rec = level_voxel(s, cx, cy);
    if (rec == NULL) {
        return;
    }
    uint32_t* word = (uint32_t*)(rec + 0x0C);
    *word |= 1u;
}

/* FUN_004324d0 / FUN_00432520: secondary bit 3 pair. */
void level_cell_secondary_bit8_set(LevelState* s, int cx, int cy, int set)
{
    uint8_t* rec = level_voxel(s, cx, cy);
    if (rec == NULL) {
        return;
    }
    uint32_t* word = (uint32_t*)(rec + 0x0C);
    if (set != 0) {
        *word |= 8u;
    } else {
        *word &= ~8u;
    }
}

uint32_t level_cell_secondary_flag_8(const LevelState* s, int cx, int cy)
{
    return cell_secondary_word(s, cx, cy) & 8u;
}

/* FUN_00432aa0: mode 0 = masked 0x400 read; mode nonzero = OR test of
 * the 0x400 flag and the secondary BYTE's bit 4 (the byte read is the
 * low byte of the secondary word, so the word mask is equivalent). */
uint32_t level_cell_occupied_or_flag(const LevelState* s, int cx, int cy,
                                     int mode)
{
    const uint32_t flags = cell_flag_word(s, cx, cy);
    if (mode == 0) {
        return flags & 0x400u;
    }
    if ((flags & 0x400u) == 0 && (cell_secondary_word(s, cx, cy) & 4u) == 0) {
        return 0;
    }
    return 1;
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
uint32_t level_cell_flag_800_bounded(const LevelState* s, int cx, int cy)
{
    const int w = level_int(s, LEVEL_OFF_WIDTH);
    const int h = level_int(s, LEVEL_OFF_HEIGHT);
    if ((unsigned)cx < ((unsigned)w - 1u) &&
        (unsigned)cy < ((unsigned)h - 1u)) {
        const uint8_t* voxels = (const uint8_t*)level_load_ptr(s, LEVEL_OFF_VOXELS);
        const size_t index = (size_t)cy * (size_t)w + (size_t)cx;
        uint32_t flags;
        __builtin_memcpy(&flags, voxels + index * LEVEL_VOXEL_STRIDE + 8,
                         sizeof(flags));
        return flags & 0x800u;
    }
    return 0u;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

uint32_t level_cell_flag_8_bounded(const LevelState* s, int cx, int cy)
{
    const int w = level_int(s, LEVEL_OFF_WIDTH);
    const int h = level_int(s, LEVEL_OFF_HEIGHT);
    if ((unsigned)cx < ((unsigned)w - 1u) &&
        (unsigned)cy < ((unsigned)h - 1u)) {
        const uint8_t* voxels = (const uint8_t*)level_load_ptr(s, LEVEL_OFF_VOXELS);
        const size_t index = (size_t)cy * (size_t)w + (size_t)cx;
        uint32_t flags;
        __builtin_memcpy(&flags, voxels + index * LEVEL_VOXEL_STRIDE + 8,
                         sizeof(flags));
        return flags & 8u;
    }
    return 0u;
}

uint32_t level_cell_flag_20_bounded(const LevelState* s, int cx, int cy)
{
    const int w = level_int(s, LEVEL_OFF_WIDTH);
    const int h = level_int(s, LEVEL_OFF_HEIGHT);
    if ((unsigned)cx < ((unsigned)w - 1u) &&
        (unsigned)cy < ((unsigned)h - 1u)) {
        const uint8_t* voxels = (const uint8_t*)level_load_ptr(s, LEVEL_OFF_VOXELS);
        const size_t index = (size_t)cy * (size_t)w + (size_t)cx;
        uint32_t flags;
        __builtin_memcpy(&flags, voxels + index * LEVEL_VOXEL_STRIDE + 8,
                         sizeof(flags));
        return flags & 0x20u;
    }
    return 0u;
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
/* FUN_00432b90: unchecked signed state byte in {8, 10, 11}. */
int level_cell_type_is_8_or_10_or_11(const LevelState* s, int cx, int cy)
{
    const uint8_t* rec = cell_record_unchecked_const(s, cx, cy);
    if (rec == NULL) {
        return 0;
    }
    const int type = (int)(signed char)rec[2];
    return type == 8 || type == 10 || type == 11;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

/* FUN_00432dd0: entity-reservation bit (the level_tile_mark flag). */
uint32_t level_cell_flag_reserved(const LevelState* s, int cx, int cy)
{
    const uint8_t* rec = cell_record_unchecked_const(s, cx, cy);
    uint32_t flags;
    if (rec == NULL) return 0u;
    __builtin_memcpy(&flags, rec + 0x08, sizeof(flags));
    return flags & TILE_FLAG_OCCUPIED;
}

/* FUN_00432710: raise/clear flag 0x80000 (bounds-guarded). */
void level_cell_flag_80000_set(LevelState* s, int cx, int cy, int set)
{
    uint8_t* rec = cell_record_unchecked(s, cx, cy);
    uint32_t flags;
    if (rec == NULL) return;
    __builtin_memcpy(&flags, rec + 0x08, sizeof(flags));
    if (set != 0) {
        flags |= 0x80000u;
    } else {
        flags &= ~0x80000u;
    }
    __builtin_memcpy(rec + 0x08, &flags, sizeof(flags));
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
/* FUN_004332c0: record byte +3, unsigned. */
int level_cell_record_byte3(const LevelState* s, int cx, int cy)
{
    const uint8_t* rec = level_voxel(s, cx, cy);
    return rec != NULL ? (int)rec[3] : 0;
}

/* Level-owned form used by map loading and simulation helpers. */
void level_cell_work_flag_set(LevelState* s, int cx, int cy)
{
    const int w = level_int(s, LEVEL_OFF_WIDTH);
    const int h = level_int(s, LEVEL_OFF_HEIGHT);
    if ((unsigned)cx < ((unsigned)w - 1u) &&
        (unsigned)cy < ((unsigned)h - 1u)) {
        uint8_t* rec = level_voxel(s, cx, cy);
        if (rec != NULL) {
            uint32_t* flags = (uint32_t*)(rec + 0x08);
            *flags |= 4u;
        }
    }
}
#endif /* !LEGORRR_BACKEND_WINAPI */

/* TerrainCellWorkFlagClear (FUN_00430320): target-shaped implementation
 * below obtains the current level from GameContext. This explicit-level
 * worker is also used by the erosion path. The descriptor veto walks the
 * live path/descriptor list rooted at DAT_004b8a24; these records carry the
 * pair at +0x0c/+0x10 and next at +0x24. */
void terrain_cell_work_flag_clear(LevelState* s, const int coord[2],
                                  uint32_t selector)
{
    GameContext* current = entity_pool_callback_context_get();
    if (current != NULL && current->level == s) {
        /* Route live-level mutations through the retail current-level entry
         * point; the explicit-level path below remains for off-screen level
         * maintenance that is not the active DAT_005560d4 equivalent. */
        terrain_cell_work_flag_clear_current(coord, selector);
        return;
    }
    if (s == NULL || coord == NULL) {
        return;
    }
    /* FUN_00430320 calculates the cell record directly from the supplied
     * pair; the caller owns coordinate validation. */
    uint8_t* rec = cell_record_unchecked(s, coord[0], coord[1]);
    if (rec == NULL) {
        return;
    }
    uint32_t* flags = (uint32_t*)(rec + 0x08);
    if ((*flags & 0x400u) != 0u || (rec[0x0c] & 4u) != 0u ||
        (*flags & 0x8000u) != 0u) {
        return;
    }

    if (terrain_cell_predicate_check_current(
            (const uint32_t*)coord) != 0) return;

    *flags &= 0xffdfffffu;
    *flags &= 0xdfffffffu;
    *flags &= 0xbfffffffu;
    map_cell_tracking_activity_state_update(
        entity_pool_callback_context_get(), s);
    coordinate_callback_dispatch(coord, 0);
    flic_opcode27_fallback_noop();
    {
        int repeat;
        for (repeat = 0; repeat < 4; ++repeat) {
            coordinate_pair_event_queue(
                coord, (void*)(uintptr_t)selector);
        }
    }
    *flags |= 3u;
}

int level_coord_pair_within_bounds(const LevelState* s, int cx, int cy)
{
    if (s == NULL) return 0;
    return level_coord_pair_within_bounds_target(
        &s->word[LEVEL_OFF_WIDTH / 4], (uint32_t)cx, (uint32_t)cy);
}

/* Explicit-level adapter for staged callers. The retail target above is
 * selected when the requested level is the active one. */
int level_cell_flag8_check(const LevelState* s, int cx, int cy)
{
    if (s == NULL) return 0;
    if (s == g_active_level_retail)
        return level_cell_flag8_check_target(s, cx, cy);
    if (level_coord_pair_within_bounds(s, cx, cy))
        return (cell_flag_word(s, cx, cy) & 8u) != 0;
    return 0;
}

/* FUN_00433330: gated state-byte read through an out param. */
int level_cell_type_get_at(const LevelState* s, uint32_t cx, uint32_t cy,
                           uint32_t* out_type)
{
    const int w = level_int(s, LEVEL_OFF_WIDTH);
    const int h = level_int(s, LEVEL_OFF_HEIGHT);
    if (cx < ((unsigned)w - 1u) && cy < ((unsigned)h - 1u)) {
        const uint8_t* rec = level_voxel(s, (int)cx, (int)cy);
        if (rec != NULL) {
            *out_type = (uint32_t)rec[0x02];
        }
        return 1;
    }
    return 0;
}

/* FUN_0042c5b0: byte classification over the five-entry table. */
int level_cell_byte_state_classify(const LevelState* s, int cx, int cy,
                                   uint32_t* out_a, uint32_t* out_b,
                                   uint32_t* out_c, uint32_t* out_d)
{
    static const uint32_t kTable[5] = { 1, 3, 5, 10, 25 };
    *out_a = 0;
    *out_b = 0;
    *out_c = 0;
    *out_d = 0;

    const uint8_t* rec = level_voxel(s, cx, cy);
    const uint32_t v = rec != NULL ? (uint32_t)rec[0x05] : 0u;
    if (v == 0) {
        return 0;
    }
    const uint32_t idx = v - 1u;
    const uint32_t slot = idx >> 2;
    const uint32_t value = slot < 5 ? kTable[slot] : 0u; /* stack-read guard */
    switch (idx & 3u) {
        case 0u:  *out_a = value; break;
        case 1u:  *out_c = value; break;
        case 2u:  *out_b = value; break;
        default:  *out_d = value; break;
    }
    return 1;
}

/* ActiveEngineFieldB0Get (FUN_00429850, findings 295/392): the active
 * engine-state dword at level-block +0xb0. */
uint32_t active_engine_field_b0_get(const LevelState* s)
{
    return s != NULL ? (uint32_t)level_int(s, LEVEL_OFF_ENGINE_B0) : 0u;
}

/* EngineFloatField210GetByIndex (FUN_00429860, findings 295/392): the
 * indexed float at +0x210 + 4*index. */
float engine_float_field210_get_by_index(const LevelState* s, int index)
{
    if (s == NULL) {
        return 0.0f;
    }
    return level_float(s, LEVEL_OFF_ENGINE_FLOATS + index * 4);
}

float engine_float_field260_get(const LevelState* s)
{
    if (s != NULL && s == g_active_level_retail)
        return engine_float_field260_get_current_target();
    return s != NULL ? level_float(s, LEVEL_OFF_ENGINE_F260) : 0.0f;
}

void air_supply_amount_update_current(const struct EntitySlot* e, float delta)
{
    air_supply_amount_update(level_current_owner_get(), e, delta);
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
uint32_t global_state_field84_get(const LevelState* s)
{
    if (s != NULL && s == g_active_level_retail)
        return global_state_field84_get_current_target();
    return s != NULL ? (uint32_t)level_int(s, LEVEL_OFF_STATE84) : 0u;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

uint32_t upgrade_resource_check_current(int selector)
{
    return upgrade_resource_check(level_current_owner_get(), selector);
}

/* --- tracked/refreshing cell-flag set family ------------------------------
 * Retail's 0x432100 producer writes 100 coordinate pairs at 0x0055684c;
 * 0x432200 snapshots them to 0x00556b6c and clears the secondary 0x100 bit,
 * then 0x4321a0 consumes that snapshot after the entity walk. The separate
 * visited list at 0x00556528 is also 100 coordinate pairs. */

// GLOBAL: LEGORR 0x0055684c
int32_t s_cell_refresh_track[LEVEL_CELL_REFRESH_TRACK_CAP][2];
// GLOBAL: LEGORR 0x00556e8c
int     s_cell_refresh_track_count;
// GLOBAL: LEGORR 0x00556b6c
int32_t s_cell_previous_track[LEVEL_CELL_REFRESH_TRACK_CAP][2];
// GLOBAL: LEGORR 0x00556e90
int     s_cell_previous_track_count;
// GLOBAL: LEGORR 0x00556528
int32_t s_cell_visited_track[LEVEL_CELL_VISITED_TRACK_CAP][2];
// GLOBAL: LEGORR 0x00556848
int     s_cell_visited_track_count;
static TerrainNeighborPresenceHook s_terrain_neighbor_presence_hook;
TerrainNeighborRenderUpdateHook s_terrain_neighbor_render_hook;
void* s_terrain_neighbor_hook_context;
TerrainNeighborActionHook s_terrain_neighbor_build_action;
TerrainNeighborActionHook s_terrain_neighbor_clear_action;
void* s_terrain_neighbor_action_context;
static TerrainCellVariantCompleteHook s_terrain_cell_variant_complete_hook;
static void* s_terrain_cell_variant_complete_context;
static TerrainCellStateProjectionHook s_terrain_cell_state_projection_hook;
static void* s_terrain_cell_state_projection_context;
DockDefinitionCellGetHook s_dock_definition_cell_get_hook;
void* s_dock_definition_cell_get_context;
static SpatialObjectQueryHook s_spatial_object_query_hook;
static void* s_spatial_object_query_context;
static TerrainCellRecordCountIncrementHook s_terrain_cell_record_count_hook;
static void* s_terrain_cell_record_count_context;
GridCellEligibilityHooks s_grid_cell_eligibility_hooks;

void grid_cell_eligibility_hooks_set(const GridCellEligibilityHooks* hooks)
{
    if (hooks == NULL) memset(&s_grid_cell_eligibility_hooks, 0,
                              sizeof(s_grid_cell_eligibility_hooks));
    else s_grid_cell_eligibility_hooks = *hooks;
}

void spatial_object_query_hook_install(SpatialObjectQueryHook hook,
                                       void* context)
{
    s_spatial_object_query_hook = hook;
    s_spatial_object_query_context = context;
}

void* spatial_object_query_execute(const SpatialObjectQueryRequest* request)
{
    if (request == NULL || s_spatial_object_query_hook == NULL) return NULL;
    return s_spatial_object_query_hook(s_spatial_object_query_context,
                                      request);
}

void terrain_cell_record_count_increment_hook_install(
    TerrainCellRecordCountIncrementHook hook, void* context)
{
    s_terrain_cell_record_count_hook = hook;
    s_terrain_cell_record_count_context = context;
}

void terrain_cell_record_count_increment(int a, int b, int c)
{
    if (s_terrain_cell_record_count_hook != NULL) {
        s_terrain_cell_record_count_hook(a, b, c,
                                         s_terrain_cell_record_count_context);
    }
}

int terrain_cell_surface_update_dispatch(
    const uint8_t* cell, int x, int y, void* param_a, void* param_b,
    const TerrainCellSurfaceUpdateHooks* hooks)
{
    float coordinates[12];
    uint32_t flags;
    uint32_t packed_surface;
    void* resource;
    if (cell == NULL || hooks == NULL || hooks->resource_get == NULL ||
        hooks->geometry_get == NULL || hooks->dispatch == NULL) return 0;
    memcpy(&flags, cell + 8, sizeof(flags));
    if ((flags & 0x18u) == 0u) return 0;
    packed_surface = cell[1];
    resource = hooks->resource_get(hooks->context,
                                   packed_surface & 0x0fu,
                                   (packed_surface >> 4) & 0x0fu);
    if (resource == NULL) return 0;
    if (!hooks->geometry_get(hooks->context, x, y, coordinates)) return 0;
    hooks->dispatch(hooks->context, resource, coordinates, packed_surface,
                    cell[3], param_a, param_b);
    return 1;
}

void terrain_cell_mutation_progress_current(int x, int y)
{
    if (x >= 0 && y >= 0) {
        GameContext* game = entity_pool_callback_context_get();
        LevelState* level = game != NULL ? game->level : NULL;
        if (level == NULL)
            level = (LevelState*)map_cell_record_grid_level_get();
        if (level != NULL)
            terrain_cell_mutation_progress(level, (uint32_t)x,
                                           (uint32_t)y);
    }
}

int terrain_stamp_impact_handle(uint32_t x, uint32_t y, int direction,
                                int mode,
                                const TerrainStampImpactHooks* hooks)
{
    static const int neighbor_x[4] = { 1, 0, -1, 0 };
    static const int neighbor_y[4] = { 0, 1, 0, -1 };
    static const int effect_x[4] = { 0, 1, 0, -1 };
    static const int effect_y[4] = { 1, 0, -1, 0 };
    int cell_x, cell_y;
    int neighbors = 0;
    int i;
    float base_height, effect_height;
    if (direction < 0 || direction >= 4) return 0;
    if (hooks == NULL || hooks->cell_flags_get == NULL ||
        hooks->impact_cell_resolve == NULL || hooks->height_get == NULL ||
        hooks->effect_create == NULL)
        return terrain_stamp_impact_handle_current(x, y, direction, mode);
    for (i = 0; i < 4; ++i) {
        if (hooks->cell_flags_get(hooks->context,
                (int)x + neighbor_x[i], (int)y + neighbor_y[i]) & 8u)
            ++neighbors;
    }
    if (!hooks->impact_cell_resolve(hooks->context, x, y, mode,
                                    &cell_x, &cell_y)) return 0;
    base_height = hooks->height_get(hooks->context, cell_x, cell_y, 0, 0);
    if (hooks->impact_event != NULL)
        hooks->impact_event(hooks->context, cell_x, cell_y, 0, 4,
                            base_height);
    effect_height = hooks->height_get(hooks->context, cell_x, cell_y,
                                      (float)effect_x[direction],
                                      (float)effect_y[direction]);
    if (!hooks->effect_create(hooks->context, neighbors > 1, x, y,
                              cell_x, cell_y, effect_height))
        return 0;
    if (hooks->cell_flags_or != NULL)
        hooks->cell_flags_or(hooks->context, x, y, 0x800u);
    return 1;
}

// GLOBAL: LEGORR 0x005562d8
const char* s_surface_type_names[SURFACE_TYPE_SLOTS];
// GLOBAL: LEGORR 0x00556320
uint32_t s_surface_type_values[SURFACE_TYPE_SLOTS];

void surface_type_descriptions_load(SurfaceTypeDescriptionEntry entries[18],
                                    const SurfaceTypeDescriptionHooks* hooks)
{
    unsigned int i;
    if (entries == NULL || hooks == NULL || hooks->value_get == NULL ||
        hooks->parse == NULL) return;
    for (i = 0; i < 18u; ++i) {
        const char* value = hooks->value_get(hooks->context,
                                             entries[i].config_key);
        uint32_t parsed = 0;
        uint32_t flags = 0;
        char buffer[1024];
        size_t length = 0;
        if (value == NULL) continue;
        while (length < sizeof(buffer) - 1u && value[length] != '\0') {
            buffer[length] = value[length];
            ++length;
        }
        buffer[length] = '\0';
        (void)hooks->parse(hooks->context, buffer, &parsed, &flags);
        entries[i].parsed_value = parsed;
        entries[i].flags = flags;
        s_surface_type_values[i] = parsed;
    }
}

void tile_model_entity_registration_release(
    TileModelEntityRegistration* registration,
    const TileModelEntityRegistrationHooks* hooks)
{
    unsigned int i;
    if (registration == NULL) return;
    if (hooks != NULL && hooks->model_release != NULL) {
        for (i = 0; i < 2u; ++i) {
            if (registration->models[i] != NULL)
                hooks->model_release(hooks->context,
                                     registration->models[i]);
        }
    }
    free(registration);
}

typedef struct TerrainProjectedCellCandidate {
    int x;
    int y;
    float distance;
    float metric;
} TerrainProjectedCellCandidate;

uint32_t terrain_projected_cell_state_refresh(
    const TerrainProjectedCellRefreshState* state,
    const TerrainProjectedCellRefreshHooks* hooks)
{
    TerrainProjectedCellCandidate* deferred;
    uint32_t deferred_count = 0, immediate_count = 0, accepted = 0;
    int x_min, x_max, y_min, y_max;
    int x, y;
    size_t capacity;
    if (state == NULL || hooks == NULL || hooks->cell_flags_get == NULL ||
        hooks->candidate_measure == NULL || state->map_width == 0u ||
        state->map_height == 0u || state->radius < 0) return 0;
    x_min = state->center_x - state->radius;
    y_min = state->center_y - state->radius;
    x_max = state->center_x + state->radius;
    y_max = state->center_y + state->radius;
    if (x_min < 0) x_min = 0;
    if (y_min < 0) y_min = 0;
    if (x_max >= (int)state->map_width) x_max = (int)state->map_width - 1;
    if (y_max >= (int)state->map_height) y_max = (int)state->map_height - 1;
    if (x_min >= x_max || y_min >= y_max) return 0;
    capacity = (size_t)(x_max - x_min) * (size_t)(y_max - y_min);
    deferred = (TerrainProjectedCellCandidate*)malloc(
        capacity * sizeof(*deferred));
    if (deferred == NULL) return 0;
    for (y = y_min; y < y_max; ++y) {
        for (x = x_min; x < x_max; ++x) {
            float distance, facing, metric;
            uint32_t flags = hooks->cell_flags_get(hooks->context, x, y);
            if ((flags & 0x18u) == 0u) continue;
            if (!hooks->candidate_measure(hooks->context, x, y,
                                          &distance, &facing, &metric) ||
                distance >= state->distance_limit ||
                facing <= state->facing_limit) continue;
            if (metric <= 0.0f) {
                ++immediate_count;
                continue;
            }
            deferred[deferred_count].x = x;
            deferred[deferred_count].y = y;
            deferred[deferred_count].distance = distance;
            deferred[deferred_count].metric = metric;
            ++deferred_count;
        }
    }
    for (x = 0; x < (int)deferred_count; ++x) {
        TerrainProjectedCellCandidate* candidate = &deferred[x];
        uint32_t flags = hooks->cell_flags_get(hooks->context,
                                               candidate->x, candidate->y);
        int forced = accepted >= state->deferred_limit ||
            candidate->distance >= state->distance_limit * 0.6666667f;
        int overlay = 0;
        if (!forced && state->overlay_enabled != 0u &&
            hooks->cell_overlay_update != NULL)
            overlay = hooks->cell_overlay_update(hooks->context,
                                                 candidate->x, candidate->y);
        if (!forced && overlay) {
            if (hooks->cell_flags_set != NULL)
                hooks->cell_flags_set(hooks->context, candidate->x,
                                      candidate->y, flags | 0x100u);
            ++accepted;
        }
        else {
            if (hooks->cell_flags_set != NULL)
                hooks->cell_flags_set(hooks->context, candidate->x,
                                      candidate->y, flags & ~0x100u);
            if (hooks->cell_refresh != NULL)
                hooks->cell_refresh(hooks->context, candidate->x,
                                    candidate->y);
            if ((flags & 0x4000u) != 0u && hooks->cell_sound != NULL)
                hooks->cell_sound(hooks->context, candidate->x,
                                  candidate->y);
        }
        if (hooks->frame_tick != NULL) hooks->frame_tick(hooks->context);
    }
    while (immediate_count-- != 0u) {
        if (hooks->frame_tick != NULL) hooks->frame_tick(hooks->context);
    }
    free(deferred);
    return accepted;
}

void dock_definition_cell_get_hook_install(DockDefinitionCellGetHook hook,
                                          void* context)
{
    s_dock_definition_cell_get_hook = hook;
    s_dock_definition_cell_get_context = context;
}

TerrainCellLinkedEntryRemoveHook s_terrain_cell_linked_entry_remove_hook;
void* s_terrain_cell_linked_entry_remove_context;

void terrain_neighbor_flag_hooks_install(
    TerrainNeighborPresenceHook presence,
    TerrainNeighborRenderUpdateHook render_update, void* context)
{
    s_terrain_neighbor_presence_hook = presence;
    s_terrain_neighbor_render_hook = render_update;
    s_terrain_neighbor_hook_context = context;
}

void terrain_neighbor_refresh_hooks_install(
    TerrainNeighborActionHook build_action,
    TerrainNeighborActionHook clear_action, void* context)
{
    s_terrain_neighbor_build_action = build_action;
    s_terrain_neighbor_clear_action = clear_action;
    s_terrain_neighbor_action_context = context;
}

void terrain_cell_variant_complete_hook_install(
    TerrainCellVariantCompleteHook hook, void* context)
{
    s_terrain_cell_variant_complete_hook = hook;
    s_terrain_cell_variant_complete_context = context;
}

void terrain_cell_state_projection_hook_install(
    TerrainCellStateProjectionHook hook, void* context)
{
    s_terrain_cell_state_projection_hook = hook;
    s_terrain_cell_state_projection_context = context;
}

void terrain_cell_linked_entry_remove_hook_install(
    TerrainCellLinkedEntryRemoveHook hook, void* context)
{
    s_terrain_cell_linked_entry_remove_hook = hook;
    s_terrain_cell_linked_entry_remove_context = context;
}

static void cell_refresh_track_push(int cx, int cy)
{
    if (s_cell_refresh_track_count < LEVEL_CELL_REFRESH_TRACK_CAP) {
        /* corpus writes unbounded (deviation: silent bound). */
        s_cell_refresh_track[s_cell_refresh_track_count][0] = cx;
        s_cell_refresh_track[s_cell_refresh_track_count][1] = cy;
        ++s_cell_refresh_track_count;
    }
}

static void cell_visited_track_push(int cx, int cy)
{
    if (s_cell_visited_track_count < LEVEL_CELL_VISITED_TRACK_CAP) {
        s_cell_visited_track[s_cell_visited_track_count][0] = cx;
        s_cell_visited_track[s_cell_visited_track_count][1] = cy;
        ++s_cell_visited_track_count;
    }
}

static void cell_refresh_seam(LevelState* s, int cx, int cy)
{
    if (s == NULL) return;
    /* The level owns no scene manager for CellInitPass's activity, effects,
     * and mesh updates. Keep the live terrain's physical cell dirty so the
     * terrain renderer can consume the refresh when its owner is attached. */
    terrain_cell_changed_flag_set(s->ptr_terrain, cx, cy);
}

/* FUN_00439990 clears the level's +0x98 field only on its non-0x10 state
 * path. When the level comes from the map registry rather than the threaded
 * GameContext, reproduce that clear against the active level explicitly. */
void map_cell_tracking_activity_state_update(GameContext* game,
                                                     LevelState* level)
{
    GameContext* matching_game = game != NULL && game->level == level
        ? game : NULL;
    entity_activity_scene_state_update(matching_game);
    if (matching_game == NULL &&
        (entity_state_flags_peek() & 0x10u) == 0u) {
        global_object_field98_clear(level);
    }
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
/* Explicit-level adapter for the current-level retail entry below. */
void map_cell_tracking_flags_clear_and_refresh(LevelState* s,
                                               const int cell_xy[2])
{
    GameContext* game;
    uint8_t* rec;
    uint32_t* flags;
    if (s == NULL || cell_xy == NULL) {
        return;
    }
    rec = level_voxel(s, cell_xy[0], cell_xy[1]);
    if (rec == NULL) {
        return;
    }
    flags = (uint32_t*)(rec + 0x08);
    *flags &= 0xdfffffffu;
    *flags &= 0xbfffffffu;
    game = entity_pool_callback_context_get();
    map_cell_tracking_activity_state_update(game, s);
    /* FUN_004302b0 calls FUN_00402a40, the coordinate-pair callback walk.
     * FUN_00402b30 is a different callback registry and must not be used for
     * this refresh. */
    coordinate_pair_callback_dispatch(cell_xy);
    flic_opcode27_fallback_noop();
}
#endif /* !LEGORRR_BACKEND_WINAPI */

/* TerrainCellStateUpdate (FUN_00432c90): the retail entry takes only the
 * coordinate pair and reads the active level global. Keep the explicit-level
 * worker for portable callers; the mapped entry below supplies the live
 * GameContext owner. */
void terrain_cell_state_effect_spawn(LevelState* level,
                                            const int cell_xy[2])
{
    float position[3];
    float center[2];
    float rotation[3];
    TerrainState* terrain = level != NULL
        ? (TerrainState*)level->ptr_terrain : NULL;
    if (terrain == NULL || cell_xy == NULL ||
        !terrain_cell_world_center_xy_get(terrain, cell_xy[0], cell_xy[1],
                                          center))
        return;
    position[0] = center[0];
    position[1] = center[1];
    position[2] = terrain_surface_height_interpolate(
        terrain, position[0], position[1]);
    if (s_terrain_cell_state_projection_hook != NULL) {
        s_terrain_cell_state_projection_hook(
            cell_xy, position, s_terrain_cell_state_projection_context);
    }
    /* FUN_00432c90 selects effect slot 3 with a D3DRM random orientation
     * after resolving the cell center and interpolated terrain height. */
    d3drm_vector_random(rotation);
    (void)lava_effect_select_and_place(3, position, rotation);
}

int terrain_cell_state_update(LevelState* s, const int cell_xy[2])
{
    uint8_t* rec;
    uint32_t* flags;
    uint32_t value;
    if (s == NULL || cell_xy == NULL) {
        return 0;
    }
    rec = level_voxel(s, cell_xy[0], cell_xy[1]);
    if (rec == NULL) {
        return 0;
    }
    flags = (uint32_t*)(rec + 0x08);
    value = *flags;
    if ((value & 0x200000u) != 0u) {
        return 1;
    }
    {
        uint32_t state = value & 3u;
        if (state != 0u) {
            *flags = (*flags & ~3u) | ((state - 1u) & 3u);
        }
        else {
            *flags |= 0x200000u;
            if (s_terrain_cell_linked_entry_remove_hook != NULL) {
                s_terrain_cell_linked_entry_remove_hook(
                    cell_xy, s_terrain_cell_linked_entry_remove_context);
            }
            selection_mode5_cancel_at_point((const uint32_t*)cell_xy);
            coordinate_callback_dispatch_by_mode(cell_xy, 0);
            terrain_cell_state_effect_spawn(s, cell_xy);
        }
        cell_init_pass(s, (float)cell_xy[0], (uint32_t)cell_xy[1]);
        return state == 0u;
    }
}

/* TerrainCellVariantFlagPropagate (FUN_00432600) takes one coordinate pair
 * and reads the active level global. */
int terrain_cell_variant_flag_propagate(LevelState* s, const int cell_xy[2])
{
    static const int offsets[5][2] = {
        {0, 0}, {0, -1}, {1, 0}, {0, 1}, {-1, 0}
    };
    uint8_t* rec;
    uint32_t* flags;
    uint32_t old_flags;
    int i;
    if (s == NULL || cell_xy == NULL) {
        return 0;
    }
    rec = level_voxel(s, cell_xy[0], cell_xy[1]);
    if (rec == NULL) {
        return 0;
    }
    flags = (uint32_t*)(rec + 0x08);
    old_flags = *flags;
    if ((old_flags & 0x200000u) == 0u || rec[2] == 6) {
        return 0;
    }
    if ((old_flags & 0x80000000u) == 0u) {
        *flags |= 0x20000000u;
    } else {
        lava_erosion_pending_cell_add(cell_xy, 0);
        rec[7] = 0;
    }
    *flags &= 0xbfffffffu;
    for (i = 0; i < 5; ++i) {
        cell_init_pass(s,
            (float)(cell_xy[0] + offsets[i][0]),
            (uint32_t)(cell_xy[1] + offsets[i][1]));
    }
    map_cell_tracking_activity_state_update(
        entity_pool_callback_context_get(), s);
    return 1;
}

void terrain_cell_flag100_set_and_track(LevelState* s, int cx, int cy)
{
    uint8_t* rec = level_voxel(s, cx, cy);
    if (rec == NULL) {
        return;
    }
    uint32_t* word = (uint32_t*)(rec + 0x0C);
    *word |= 0x100u;
    cell_refresh_track_push(cx, cy);
    cell_init_pass(s, (float)cx, (uint32_t)cy);
}

void terrain_cell_visited_flag_set_and_track(LevelState* s, int cx, int cy)
{
    uint8_t* rec = level_voxel(s, cx, cy);
    if (rec == NULL) {
        return;
    }
    uint32_t* word = (uint32_t*)(rec + 0x0C);
    *word |= 2u;
    cell_visited_track_push(cx, cy);
}

void terrain_cell_visited_flags_clear_all(LevelState* s)
{
    int i;
    for (i = 0; i < s_cell_visited_track_count; ++i) {
        uint8_t* rec = level_voxel(s, s_cell_visited_track[i][0],
                                  s_cell_visited_track[i][1]);
        if (rec == NULL) {
            continue;
        }
        uint32_t* word = (uint32_t*)(rec + 0x0C);
        *word &= ~2u;
    }
    s_cell_visited_track_count = 0;
}

void terrain_cell_primary_flags_mask_clear_and_refresh(LevelState* s,
                                                       int cx, int cy)
{
    uint8_t* rec = level_voxel(s, cx, cy);
    if (rec == NULL) {
        return;
    }
    uint32_t* primary = (uint32_t*)(rec + 0x08);
    /* Corpus 0x432385-0x43238d loads the dword into esi, masks it, then
     * stores it back. Keep that separate RMW sequence under /Od. */
    uint32_t primary_value = *primary;
    primary_value &= 0xdfef7bffu;
    *primary = primary_value;
    uint32_t* secondary = (uint32_t*)(rec + 0x0C);
    /* Same load/and/store shape for the secondary word at 0x4323a7-0x4323ac. */
    uint32_t secondary_value = *secondary;
    secondary_value &= 0xfffffffbu;
    *secondary = secondary_value;
    cell_init_pass(s, (float)cx, (uint32_t)cy);
}

void terrain_cell_flags_set_and_refresh(LevelState* s, int cx, int cy)
{
    uint8_t* rec = level_voxel(s, cx, cy);
    if (rec == NULL) {
        return;
    }
    uint32_t* primary = (uint32_t*)(rec + 0x08);
    /* Corpus 0x432415-0x43241d uses load/or/store, then repeats the
     * operation for the secondary word. */
    uint32_t primary_value = *primary;
    primary_value |= 0x108000u;
    *primary = primary_value;
    uint32_t* secondary = (uint32_t*)(rec + 0x0C);
    uint32_t secondary_value = *secondary;
    secondary_value |= 4u;
    *secondary = secondary_value;
    cell_init_pass(s, (float)cx, (uint32_t)cy);
}

void terrain_cell_flags_set_mask100400(LevelState* s, int x, int y)
{
    uint8_t* rec = level_voxel(s, x, y);
    if (rec == NULL) {
        return;
    }
    uint32_t* primary = (uint32_t*)(rec + 0x08);
    *primary |= 0x100400u;
    cell_init_pass(s, (float)x, (uint32_t)y);
}

void terrain_cell_flags_set_mask20108000(LevelState* s, int x, int y)
{
    uint8_t* rec = level_voxel(s, x, y);
    if (rec == NULL) {
        return;
    }
    uint32_t* primary = (uint32_t*)(rec + 0x08);
    *primary |= 0x20108000u;
    cell_init_pass(s, (float)x, (uint32_t)y);
}

void terrain_cell_flag40000000_clear_or_set(LevelState* s, int cx, int cy,
                                            int set)
{
    uint8_t* rec = level_voxel(s, cx, cy);
    if (rec == NULL) {
        return;
    }
    uint32_t* primary = (uint32_t*)(rec + 0x08);
    if (set == 0) {
        *primary &= 0xbfffffffu;
    } else {
        *primary |= 0x40000000u;
    }
    cell_init_pass(s, (float)cx, (uint32_t)cy);
}

/* LevelDebugInputCellMutatorDispatch (FUN_004288e0), wired stateful branches.
 * The engine calls this dispatcher only while DAT_00556ed0 bit 0x10 is set.
 * Unsupported branches that need the engine's hovered-object pointer or
 * renderer-owned DDi object remain at that owner boundary. */
void level_debug_render_ops_set(const LevelDebugRenderOps* ops)
{
    if (ops == NULL) memset(&s_level_debug_render_ops, 0,
                            sizeof(s_level_debug_render_ops));
    else s_level_debug_render_ops = *ops;
}

void level_debug_input_cell_mutator_dispatch(
    EngineContext* engine, GameContext* ctx, const InputState* input,
    int cell_x, int cell_y)
{
    EntitySlot* entity;
    if (engine == NULL || (engine->debug & 0x10u) == 0u || input == NULL)
        return;

    /* DAT_0076ac4f: selected record's +0x148 bit 0x400 toggle. */
    if (input_key_edge(input, 0xcf) != 0 && selection_has_any(ctx) != 0) {
        entity = selection_first(ctx);
        if (entity != NULL) entity_record_flag400_toggle_and_refresh(entity);
    }

    /* DAT_0076abae is slot 0x2e; the retail body performs the same nested
     * type-8 clear/detach call twice on the first selected entity. */
    if (input_key_edge(input, 0x2e) != 0 && selection_has_any(ctx) != 0) {
        entity = selection_first(ctx);
        if (entity != NULL) {
            entity_clear_nested_state_8_fields(entity);
            entity_clear_nested_state_8_fields(entity);
        }
    }

    /* DAT_0076abb1 (slot 0x31) resets the first selected entity's action
     * sequence with preserve_mode 0. */
    if (input_key_edge(input, 0x31) != 0 && selection_has_any(ctx) != 0) {
        entity = selection_first(ctx);
        if (entity != NULL) entity_action_sequence_reset(entity, 0);
    }

    /* DAT_0076abd8: set the engine 0x01000000 state bit and reset the
     * separate SystemState word only when entering this mode. */
    if (input_key_edge(input, 0x58) != 0) {
        if ((engine->state & 0x01000000u) == 0u) {
            uint32_t zero = 0u;
            engine->state |= 0x01000000u;
            (void)system_state_flags_set(&zero);
        } else {
            engine->state &= ~0x01000000u;
        }
    }

    /* DAT_0076abd7 calls GlobalModeZeroInputSet(0) on every held frame. */
    if (input_key_down(input, 0x57) != 0) {
        unported_ledger_stubs::dependency_grid_mode_zero_input_set(
            ctx != NULL ? ctx->level : NULL, 0);
    }

    /* DAT_0076abc3 flips DAT_004de57c, then forwards that value to the
     * existing scene-transform state owner. */
    if (input_key_edge(input, 0x43) != 0) {
        s_debug_scene_transform_enabled =
            s_debug_scene_transform_enabled == 0 ? 1 : 0;
        scene_transform_enable_set(s_debug_scene_transform_enabled);
    }

    /* DAT_0076abc4: toggle the renderer's debug pose, then reset the paired
     * transform/effect motion snapshot. The callback pair is the owner seam
     * for DAT_005560f0 and DAT_005560e4+0x2c. */
    if (input_key_edge(input, 0x44) != 0) {
        float position_x, position_y, position_z, pose_z;
        s_debug_camera_pose_mode = s_debug_camera_pose_mode == 0 ? 1 : 0;
        if (s_debug_camera_pose_mode == 0) {
            position_x = 200.0f;
            position_y = 140.0f;
            position_z = -130.0f;
            pose_z = 0.75f;
        } else {
            position_x = 250.0f;
            position_y = 190.0f;
            position_z = 20.0f;
            pose_z = 0.0f;
        }
        if (s_level_debug_render_ops.object_position_set != NULL) {
            (void)s_level_debug_render_ops.object_position_set(
                s_level_debug_render_ops.context, position_x, position_y,
                position_z);
        }
        if (s_level_debug_render_ops.object_pose_method_dispatch != NULL) {
            (void)s_level_debug_render_ops.object_pose_method_dispatch(
                s_level_debug_render_ops.context, -1.0f, -0.8f, pose_z,
                0.0f, 1.0f, 0.0f);
        }
        simtick_scene_position_reset_and_effect_motion_cancel();
    }

    if (ctx == NULL || ctx->level == NULL) return;

    /* DAT_0076ab91 is held-key TerrainNeighborFlagSet(level, 0, x, y). */
    if (input_key_down(input, 0x11) != 0) {
        terrain_neighbor_flag_set(ctx->level, 0u, (uint32_t)cell_x,
                                  (uint32_t)cell_y);
    }

    /* DAT_0076ab8e issues one type-0x14 entity command batch. */
    if (input_key_edge(input, 0x0e) != 0) {
        entity_command14_batch_issue(5);
    }

    /* DAT_0076abd1 is a held-key path to TerrainCellInitialize. */
    if (input_key_down(input, 0x51) != 0) {
        level_cell_init_pass(ctx->level, cell_x, cell_y);
    }

    /* DAT_0076abd3: edge-triggered TerrainCellMutation(mode 0). */
    if (input_key_edge(input, 0x53) != 0) {
        (void)terrain_cell_mutate(ctx->level, (uint32_t)cell_x,
                                  (uint32_t)cell_y, 0);
    }

    /* DAT_0076aba4: toggle the cell-object entry selected by the retail
     * presence query. The create/delete helpers own their full pool and
     * entity teardown effects. */
    if (input_key_edge(input, 0x24) != 0) {
        if (grid_cell_record_present_check(cell_x, cell_y) == 0)
            (void)create_cell_object_if_available(cell_x, cell_y);
        else
            (void)delete_cell_object(cell_x, cell_y);
    }

    /* DAT_0076aba3: expand a cell into its configured type-12 entity. */
    if (input_key_edge(input, 0x23) != 0) {
        (void)unported_ledger_stubs::
            ledger_stub_00466250_ExpandCellEntitySpawn(cell_x, cell_y);
    }
}

/* --- neighbor-refresh / circle / edge-flag cluster (see level.hpp) ------- */

static TerrainEdgeMidpointUpdateFn s_terrain_edge_midpoint_update_hook;
static void* s_terrain_edge_midpoint_update_user;

void terrain_edge_midpoint_update_hook_set(TerrainEdgeMidpointUpdateFn hook,
                                           void* user)
{
    s_terrain_edge_midpoint_update_hook = hook;
    s_terrain_edge_midpoint_update_user = user;
}

/* FUNCTION: LEGORR 0x0046e610. Update the LevelState-owned staged-record
 * cell flags from their two edge midpoints; the optional hook performs only
 * the associated native mesh/scene updates. */
void terrain_edge_flag_midpoint_update(LevelState* level)
{
    terrain_world_spatial_edge_midpoints_update(level);
    if (s_terrain_edge_midpoint_update_hook != NULL) {
        s_terrain_edge_midpoint_update_hook(level,
                                            s_terrain_edge_midpoint_update_user);
    }
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void terrain_cell_work_flag_circle_set(LevelState* s, int center_x,
                                       int center_y, int radius)
{
    const int w = level_int(s, LEVEL_OFF_WIDTH);
    const int h = level_int(s, LEVEL_OFF_HEIGHT);
    const int r2 = radius * radius; /* corpus wrapping int math */
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            if (dx * dx + dy * dy > r2) {
                continue;
            }
            const int cx = center_x + dx;
            const int cy = center_y + dy;
            /* The corpus's signed candidate bounds, then the setter's
             * own in-gate test — both faithful. */
            if (-1 < cx && cx < w && -1 < cy && cy < h) {
                level_cell_work_flag_set(s, cx, cy);
            }
        }
    }
}
#endif /* !LEGORRR_BACKEND_WINAPI */

/* TerrainNeighborWorkFlagScan (FUN_00431580): the work-flag breadth-first
 * sweep. A double-buffered float-pair queue (two pages of (w+h)*2 entries
 * carved from one (w+h)*0x20 engine-heap block — FUN_0048dbd0/FUN_0048db80)
 * starts at the seed coordinate; each round drains the current page and, for
 * every neighbour (row = y±1, col = x±1, indexed row*w + col in the stride-
 * 0x48 cell records at DAT+0xb8, port: level_voxel), processes the cells
 * carrying work flag bit 0x20000 by calling TerrainCellMutation in mode 1,
 * then clears the bit and queues each neighbour at its own coordinate unless
 * it is the coordinate currently being scanned. Rounds alternate pages until
 * the new page arrives empty. The source indexes out-of-bounds cells and
 * queue capacity blindly; level_voxel safely rejects invalid cells, while
 * the queue retains the source's (w+h)-derived capacity assumption. */
void terrain_neighbor_work_flag_scan(LevelState* s, unsigned int seed_x,
                                     unsigned int seed_y)
{
    const int w = level_width(s);
    const int h = level_height(s);
    const int span = w + h;
    if (span <= 0) {
        return; /* deviation: the corpus seeds a zero-size (1-byte) alloc
                 * and spills into the block header; the port declines */
    }
    float* buf = (float*)engine_heap_alloc((long)span * 0x20);
    if (buf == NULL) {
        return; /* corpus: the entire scan is alloc-guarded */
    }
    float* page[2];
    unsigned int count[2];
    page[0] = buf;
    page[1] = buf + span * 4; /* +16(w+h) bytes = 4(w+h) floats */
    count[0] = 1u;
    count[1] = 0u;
    page[0][0] = (float)seed_x; /* corpus fildll's the dword's bits */
    page[0][1] = (float)seed_y;
    int cur = 0;
    do {
        if (count[cur] != 0u) {
            const float* pf = page[cur];
            for (unsigned int idx = 0; idx < count[cur]; ++idx, pf += 2) {
                const int col0 = level_cell_of(pf[0]); /* FUN_0048e060 pair */
                const int row0 = level_cell_of(pf[1]);
                if ((int64_t)row0 - 1 <= (int64_t)row0 + 1) { /* corpus's wrap guard */
                    for (int row = row0 - 1; row <= row0 + 1; ++row) {
                        for (int col = col0 - 1; col <= col0 + 1; ++col) {
                            uint8_t* rec = level_voxel(s, col, row);
                            uint32_t word = 0;
                            if (rec != NULL) {
                                __builtin_memcpy(&word, rec + 0x08,
                                                 sizeof(word));
                            }
                            if ((word & 0x20000u) != 0u) {
                                terrain_cell_mutate(s, (uint32_t)col,
                                                    (uint32_t)row, 1);
                                /* Mutation can publish deferred building
                                 * footprint flags. Retail reloads the live
                                 * record before clearing only the work bit. */
                                rec = level_voxel(s, col, row);
                                if (rec != NULL) {
                                    __builtin_memcpy(&word, rec + 0x08,
                                                     sizeof(word));
                                    word &= 0xfffdffffu;
                                    __builtin_memcpy(rec + 0x08, &word,
                                                     sizeof(word));
                                }
                                if (col != col0 || row != row0) {
                                    unsigned int n = count[1 - cur];
                                    float* dst = page[1 - cur];
                                    dst[2 * n] = (float)col;
                                    dst[2 * n + 1] = (float)row;
                                    count[1 - cur] = n + 1u;
                                }
                            }
                        }
                    }
                }
            }
        }
        cur = (cur == 0) ? 1 : 0;
        count[1 - cur] = 0u; /* the drained page's count */
    } while (count[cur] != 0u);
    engine_heap_free(buf);
}

typedef char cell_coordinate_timestamp_stride_check[
    sizeof(CellCoordinateTimestamp) == 8u ? 1 : -1];

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
/* terrain_cell_dirty_flag: faithful physical-byte read (see level.hpp). */
int terrain_cell_dirty_flag(const void* terrain, int x, int y)
{
    const TerrainState* t = (const TerrainState*)terrain;
    if (t == NULL || t->blocks == NULL || x < 0 || y < 0
        || x >= t->width || y >= t->height) {
        return 0;
    }
    return (t->blocks
            + ((size_t)y * (size_t)t->width + (size_t)x)
              * TERRAIN_BLOCK_STRIDE + 0x19)[0] & 0x10;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

/* --- the cell-list min-distance scale apply (see level.hpp) ---------------
 * The corpus's cell-list context (the level block's +0x10 pointer) has no
 * ported owner, so its scale word, scene-object slot, and applied-flag bit
 * land as zero-init own storage behind install/peek hooks. Dirty coordinates
 * and their count are retained by the terrain instance, matching +0x4a88
 * and +0x7198 instead of sharing one queue across levels. */
void*    s_cell_list_object;
int      s_cell_list_scale;
uint32_t s_cell_list_flags; /* corpus word +0x73d0: bit 2 = applied */

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void cell_list_apply_state_install(void* scene_object, int scale)
{
    s_cell_list_object = scene_object;
    s_cell_list_scale = scale;
}

void* cell_list_object_peek(void)
{
    return s_cell_list_object;
}

uint32_t cell_list_apply_flags_peek(void)
{
    return s_cell_list_flags;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

/* Flat dword index of a cell's selector word, or -1 when the coordinate
 * cannot address the region (bounds deviation). */
static int64_t grid_rec_block_index(int x, int y)
{
    const int64_t idx = (int64_t)y + (int64_t)x * GRID_REC_COLS;
    const int64_t off = 32 + idx * GRID_REC_STRIDE;
    if (idx < 0 || off < 0 || off >= GRID_REC_REGION_DWORDS) {
        return -1;
    }
    return off;
}

/* Flat dword index of a block field word, or -1. */
static int64_t grid_rec_field_index(int x, int y, int field)
{
    const int64_t off = grid_rec_block_index(x, y);
    if (off < 0 || field < 0) {
        return -1;
    }
    const int64_t word = off + 1 + field;
    if (word >= GRID_REC_REGION_DWORDS) {
        return -1;
    }
    return word;
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void terrain_cell_record_get(int x, int y, int field,
                             uint32_t* value_a, uint32_t* value_b)
{
    GameContext* game = entity_pool_callback_context_get();
    LevelState* level = game != NULL ? game->level : NULL;
    const unported_ledger_stubs::DependencyGridTuple* native_records = NULL;
    uint32_t native_count = 0u;
    int64_t block;
    int64_t table_index;
    int64_t pointer_index;
    if (level == NULL)
        level = (LevelState*)map_cell_record_grid_level_get();
    if (x < 0 || x >= (int)DEPENDENCY_GRID_ROWS || y < 0 ||
        y >= (int)DEPENDENCY_GRID_COLUMNS) return;
    if (unported_ledger_stubs::dependency_grid_cell_record_get_native(
            level, x, y, field,
            &native_records, &native_count)) {
        /* Preserve the retail 32-bit image word for callers that inspect
         * the output; native dereferences use the sidecar API directly. */
        if (value_a != NULL)
            *value_a = (uint32_t)(uintptr_t)native_records;
        if (value_b != NULL) *value_b = native_count;
        return;
    }
    block = grid_rec_block_index(x, y);
    if (block < 0) return;
    table_index = (block - 32) +
        (s_grid_rec[(size_t)block] != 0u ? field : 0);
    pointer_index = table_index + 16;
    if (table_index < 0 || pointer_index >= GRID_REC_REGION_DWORDS ||
        (s_grid_rec[(size_t)block] != 0u && field < 0)) return;
    if (value_a != NULL && value_b != NULL) {
        terrain_cell_record_get_target(x, y, field, value_a, value_b);
        return;
    }
    if (value_a != NULL) *value_a = s_grid_rec[(size_t)pointer_index];
    if (value_b != NULL) *value_b = s_grid_rec[(size_t)table_index];
}
#endif /* !LEGORRR_BACKEND_WINAPI */

uint32_t cell_flag4_is_set(int x, int y, int field)
{
    const int64_t block = grid_rec_block_index(x, y);
    const uint32_t dependency_flag =
        unported_ledger_stubs::dependency_grid_flag4_get(x, y, field);
    if (dependency_flag != 0xffffffffu) return dependency_flag;
    if (block < 0) {
        return 0;
    }
    const int64_t word = (s_grid_rec[(size_t)block] != 0u)
        ? grid_rec_field_index(x, y, field)
        : grid_rec_field_index(x, y, 0);
    if (word < 0) {
        return 0;
    }
    return cell_flag4_is_set_target(x, y, field);
}

void cell_flag100_set(int x, int y, int field)
{
    if (unported_ledger_stubs::dependency_grid_flag100_set(x, y, field))
        return;
    const int64_t word = grid_rec_field_index(x, y, field);
    if (word >= 0) cell_flag100_set_target(x, y, field);
}

uint32_t cell_flag100_is_set(int x, int y, int field)
{
    const uint32_t dependency_flag =
        unported_ledger_stubs::dependency_grid_flag100_get(x, y, field);
    if (dependency_flag != 0xffffffffu) return dependency_flag;
    const int64_t word = grid_rec_field_index(x, y, field);
    if (word < 0) {
        return 0;
    }
    return cell_flag100_is_set_target(x, y, field);
}

/* --- neighbor coordinate classification (see level.hpp) ------------------- */
static uint32_t terrain_neighbor_coordinate_check_explicit(
    const LevelState* s, int x, int y, uint8_t* kind, int mode)
{
    /* Corpus neighbor deltas: (0,-1), (+1,0), (0,+1), (-1,0). */
    static const int dx[4] = { 0, 1, 0, -1 };
    static const int dy[4] = { -1, 0, 1, 0 };
    /* Mask -> (kind, code) tables for mode 0 / mode != 0. */
    static const uint8_t kind_tbl[16] = {
        0, 2, 3, 3, 0, 0, 0, 0, 1, 2, 1, 3, 1, 2, 1, 0
    };
    static const uint8_t code_tbl[16] = {
        0x65, 0x65, 0x65, 99, 0x65, 0x62, 99, 100,
        0x65, 99, 0x62, 100, 99, 100, 100, 0x60
    };
    static const uint8_t alt_code_tbl[16] = {
        0x75, 0x75, 0x75, 0x73, 0x75, 0x72, 0x73, 0x74,
        0x75, 0x73, 0x72, 0x74, 0x73, 0x74, 0x74, 0x71
    };

    unsigned mask = 0;
    for (unsigned i = 0; i < 4; ++i) {
        const uint32_t w = cell_flag_word(s, x + dx[i], y + dy[i]);
        if ((w & 0x20000000u) != 0u || (w & 0x400u) != 0u) {
            mask |= 1u << i;
        }
    }
    const uint8_t k = kind_tbl[mask];
    if (kind != NULL) {
        *kind = k;
    }
    return (mode != 0) ? alt_code_tbl[mask] : code_tbl[mask];
}

uint32_t terrain_neighbor_coordinate_check(const LevelState* s, int x, int y,
                                           uint8_t* kind, int mode)
{
    if (s != NULL && s == g_active_level_retail && kind != NULL) {
        return (uint32_t)terrain_neighbor_coordinate_check_current_target(
            x, y, kind, mode);
    }
    return terrain_neighbor_coordinate_check_explicit(s, x, y, kind, mode);
}

int64_t grid_count_index(int row, int column, int slot)
{
    const int64_t cell = (int64_t)row * 15 + column;
    const int64_t idx = cell * 16 + slot;
    if (cell < 0 || slot < 0 || idx >= GRID_COUNT_TABLE_DWORDS) {
        return -1; /* bounds deviation (corpus indexes blindly) */
    }
    return idx;
}

/* FUNCTION: LEGORR 0x00455300
 * Resolve a configured object type to its grid-record table coordinates and
 * return the selected first-table count. */
uint32_t entity_type_grid_record_count_get(const char* type_name, int slot)
{
    int type = 0;
    int row = 0;
    if (!entity_type_row_for_name(type_name, &type, &row)) return 0u;
    return grid_record_count_get(type, row, slot);
}

int32_t grid_count_table_slot_peek(int row, int column, int slot, int table)
{
    const int64_t idx = grid_count_index(row, column, slot);
    if (idx < 0) {
        return -1;
    }
    return (int)(table != 0 ? s_grid_count_second[(size_t)idx]
                            : s_grid_count_first[(size_t)idx]);
}

static TerrainCellRefreshEntityCallback s_terrain_cell_refresh_callback;
static void* s_terrain_cell_refresh_callback_context;

int terrain_cell_refresh_callback_adapter(EntitySlot* entity,
                                                  void* payload)
{
    TerrainCellRefreshCallbackPayload* data =
        (TerrainCellRefreshCallbackPayload*)payload;
    if (data == NULL || s_terrain_cell_refresh_callback == NULL) return 0;
    return s_terrain_cell_refresh_callback(entity, data->cell_xy, data->mode,
        s_terrain_cell_refresh_callback_context);
}

void terrain_cell_refresh_entity_callback_install(
    TerrainCellRefreshEntityCallback callback, void* context)
{
    s_terrain_cell_refresh_callback = callback;
    s_terrain_cell_refresh_callback_context = context;
}

void terrain_cell_refresh_dispatch_level(LevelState* lvl,
                                                const int cell_xy[2],
                                                int mode)
{
    TerrainCellRefreshCallbackPayload payload;
    if (lvl == NULL || cell_xy == NULL) return;
    payload.cell_xy = cell_xy;
    payload.mode = mode;
    (void)entity_pool_callback_search(
        terrain_cell_refresh_callback_adapter, &payload);
    if (lvl->ptr_terrain != NULL) {
        terrain_cell_surface_type_reset(lvl->ptr_terrain, cell_xy[0],
                                        cell_xy[1]);
    }
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void terrain_cell_refresh_dispatch(const struct GameContext* ctx, int x,
                                   int y)
{
    LevelState* lvl = (ctx != NULL) ? ctx->level : NULL;
    const int cell_xy[2] = { x, y };
    /* Explicit-context callers correspond to the observed mode-zero call
     * sites. Keep the same callback-before-surface-reset ordering as the
     * retail entrypoint. */
    terrain_cell_refresh_dispatch_level(lvl, cell_xy, 0);
}
#endif /* !LEGORRR_BACKEND_WINAPI */

CoordinateEntityDispatchHook s_coordinate_direct_callback;
CoordinateEntityDispatchHook s_coordinate_mode_zero_callback;
CoordinateEntityDispatchHook s_coordinate_mode_nonzero_callback;
static void* s_coordinate_dispatch_context;

void coordinate_entity_dispatch_hooks_install(
    CoordinateEntityDispatchHook direct_callback,
    CoordinateEntityDispatchHook mode_zero_callback,
    CoordinateEntityDispatchHook mode_nonzero_callback, void* context)
{
    s_coordinate_direct_callback = direct_callback;
    s_coordinate_mode_zero_callback = mode_zero_callback;
    s_coordinate_mode_nonzero_callback = mode_nonzero_callback;
    s_coordinate_dispatch_context = context;
}

int coordinate_entity_dispatch_adapter(EntitySlot* entity,
                                              void* payload)
{
    CoordinateEntityDispatchPayload* const data =
        (CoordinateEntityDispatchPayload*)payload;
    if (data == NULL || data->callback == NULL) {
        return 0;
    }
    return data->callback(entity, data->coordinate_pair, data->extra,
                          s_coordinate_dispatch_context);
}

void coordinate_callback_dispatch(const void* coord_pair, int value)
{
    CoordinateEntityDispatchPayload payload;
    payload.coordinate_pair = coord_pair;
    payload.extra = value;
    payload.callback = s_coordinate_direct_callback;
    entity_pool_callback_search(coordinate_entity_dispatch_adapter, &payload);
}

void global_object_field8c_increment_for_level(LevelState* s)
{
    if (s != NULL) level_set_int(s, 0x8C, level_int(s, 0x8C) + 1);
}

void global_object_field8c_subtract_for_level(LevelState* s, int amount)
{
    if (s != NULL) level_set_int(s, 0x8C, level_int(s, 0x8C) - amount);
}

void global_object_field98_add_for_level(LevelState* s, int amount)
{
    if (s != NULL) level_set_int(s, 0x98, level_int(s, 0x98) + amount);
}

void global_object_field98_clear(LevelState* s)
{
    if (s != NULL) {
        level_set_int(s, 0x98, 0);
    }
}

void level_state_field9c_add_for_level(LevelState* s, int amount)
{
    if (s != NULL) level_set_int(s, 0x9C, level_int(s, 0x9C) + amount);
}

void global_object_field94_increment_for_level(LevelState* s)
{
    if (s != NULL) level_set_int(s, 0x94, level_int(s, 0x94) + 1);
}

void global_object_fielda8_increment_for_level(LevelState* s)
{
    if (s != NULL) level_set_int(s, 0xA8, level_int(s, 0xA8) + 1);
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
uint32_t level_field8c_get(const LevelState* s)
{
    return s != NULL ? (uint32_t)level_int(s, 0x8C) : 0u;
}

uint32_t level_field98_get(const LevelState* s)
{
    return s != NULL ? (uint32_t)level_int(s, 0x98) : 0u;
}

uint32_t level_field9c_get(const LevelState* s)
{
    return s != NULL ? (uint32_t)level_int(s, 0x9C) : 0u;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

uint32_t level_field94_get(const LevelState* s)
{
    return s != NULL ? (uint32_t)level_int(s, 0x94) : 0u;
}

uint32_t level_fielda8_get(const LevelState* s)
{
    return s != NULL ? (uint32_t)level_int(s, 0xA8) : 0u;
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void terrain_sfx_grid_cell_set(int cell, int handle)
{
    if (cell >= 0 && cell < TERRAIN_SFX_GRID_CELLS) {
        s_terrain_sfx_grid[cell] = handle;
    }
}

int terrain_sfx_grid_cell_peek(int cell)
{
    if (cell < 0 || cell >= TERRAIN_SFX_GRID_CELLS) {
        return 0;
    }
    return s_terrain_sfx_grid[cell];
}
#endif /* !LEGORRR_BACKEND_WINAPI */

TerrainChoiceWindow s_choice_windows[4];
uint32_t s_choice_scalar[2];  /* DAT_004c7e98/e9c role */
uint32_t s_choice_active;     /* DAT_004c7ea0 role */
uint32_t s_choice_flags;      /* DAT_004c7ea8 role */
uintptr_t s_choice_record;    /* DAT_004c7ea4 role */

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void terrain_choice_window_install(int word, uint32_t value)
{
    if (word >= 0)
        terrain_choice_window_array_install(word % 4, word / 4, value);
}

uint32_t terrain_choice_window_peek(int word)
{
    int kind, index;
    if (word < 0) return 0u;
    kind = word % 4;
    index = word / 4;
    if (index >= (int)s_choice_windows[kind].count) return 0u;
    return s_choice_windows[kind].values[index];
}

void terrain_choice_window_array_install(int kind, int index,
                                         uint32_t value)
{
    TerrainChoiceWindow* window;
    uint32_t count;
    uint32_t* values;
    if (kind < 0 || kind >= 4 || index < 0) return;
    window = &s_choice_windows[kind];
    count = (uint32_t)index + 1u;
    if (count > window->count) {
        if ((size_t)count > 0xffffffffu / sizeof(*values)) return;
        values = (uint32_t*)realloc(window->values,
                                    (size_t)count * sizeof(*values));
        if (values == NULL) return;
        memset(values + window->count, 0,
               (size_t)(count - window->count) * sizeof(*values));
        window->values = values;
        window->count = count;
    }
    window->values[index] = value;
}

void terrain_choice_scalar_install(int scalar, uint32_t value)
{
    if (scalar >= 0 && scalar < 2) {
        s_choice_scalar[scalar] = value;
    }
}

uint32_t terrain_choice_active_peek(void) { return s_choice_active; }
#endif /* !LEGORRR_BACKEND_WINAPI */
uint32_t terrain_choice_flags_peek(void) { return s_choice_flags; }
void terrain_choice_flags_set(uint32_t flags) { s_choice_flags = flags; }
#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
uintptr_t terrain_choice_record_peek(void) { return s_choice_record; }

/* CellCardinalNeighborApply (FUN_00409cd0) — see level.hpp. The corpus's
 * offset pairs, in its loop order: north, east, south, west. */
void cell_cardinal_neighbor_apply(const struct GameContext* ctx, int x,
                                  int y)
{
    static const int k_offsets[8] = { 0, -1, 1, 0, 0, 1, -1, 0 };
    for (int i = 0; i < 4; ++i) {
        terrain_cell_refresh_dispatch(ctx, x + k_offsets[2 * i],
                                      y + k_offsets[2 * i + 1]);
    }
}
#endif /* !LEGORRR_BACKEND_WINAPI */

/* TerrainCreate's 0x34/0x4034 reciprocal variant lists are hosted in a
 * separate allocation on native builds. This accessor supplies the same
 * window to the surface-vector pass and its 0x451180 predicate. */
const void* terrain_cell_variant_window_get(const void* terrain)
{
    const TerrainState* state = (const TerrainState*)terrain;
    return state != NULL ? state->variant_window : NULL;
}

/* --- the path record window band (2026-10-25) ----------------------------- */

/* PathRecordAllocateAndLink (FUN_00409350): allocates the corpus's 0xd4
 * word window (53 dwords), zero-fills it the corpus way, stamps the
 * serial counter store at +0x08, copies the source coordinate pair to
 * +0x0c/+0x10, and links the previous most-recent record (corpus dword
 * slot 9). Deviations: the window is widened past the corpus stride so
 * the link holds a port pointer (port-layout convention), the corpus's
 * belt-and-braces zero stores are spelled explicitly after the fill, and
 * the allocator's failure word is guarded. */

#define PATH_RECORD_WORDS (PATH_RECORD_WINDOW / 4u)

typedef struct CellPathRecordPort {
    LevelState* level;
    int x;
    int y;
    void* record;
    struct CellPathRecordPort* next;
} CellPathRecordPort;

PathRecordPayloadPort* s_path_record_payload_ports;
static CellPathRecordPort* s_cell_path_record_ports;
PathRecordEntityPort* s_path_record_entity_ports;
static TerrainSurfaceGrid* s_path_record_surface_grid;

static PathRecordPayloadPort* path_record_payload_port_find(
    const void* record)
{
    PathRecordPayloadPort* port;
    for (port = s_path_record_payload_ports; port != NULL; port = port->next) {
        if (port->record == record) return port;
    }
    return NULL;
}

static CellPathRecordPort* cell_path_record_port_find(
    const LevelState* level, int x, int y)
{
    CellPathRecordPort* port;
    for (port = s_cell_path_record_ports; port != NULL; port = port->next) {
        if (port->level == level && port->x == x && port->y == y)
            return port;
    }
    return NULL;
}

static PathRecordEntityPort* path_record_entity_port_find(
    const void* record)
{
    PathRecordEntityPort* port;
    for (port = s_path_record_entity_ports; port != NULL; port = port->next) {
        if (port->record == record) return port;
    }
    return NULL;
}

int path_record_payload_set(void* record, void* payload, uint32_t count)
{
#if defined(_WIN32)
    if (record == NULL) return 0;
    *(void**)((char*)record + 0x14) = payload;
    *(uint32_t*)((char*)record + 0x18) = count;
    return 1;
#else
    PathRecordPayloadPort* port;
    uintptr_t payload_address;
    if (record == NULL) return 0;
    port = path_record_payload_port_find(record);
    if (payload != NULL && port == NULL) {
        port = (PathRecordPayloadPort*)engine_heap_alloc(
            (long)sizeof(PathRecordPayloadPort));
        if (port == NULL) return 0;
        port->record = record;
        port->payload = NULL;
        port->count = 0u;
        port->next = s_path_record_payload_ports;
        s_path_record_payload_ports = port;
    }
    if (port != NULL) {
        port->payload = payload;
        port->count = count;
    }
    payload_address = (uintptr_t)payload;
    *(uint32_t*)((char*)record + 0x14) =
        (uint32_t)(payload_address & 0xffffffffu);
    *(uint32_t*)((char*)record + 0x18) = count;
    return 1;
#endif
}

void* path_record_payload_get(const void* record)
{
    PathRecordPayloadPort* port = path_record_payload_port_find(record);
    if (port != NULL) return port->payload;
#if defined(_WIN32)
    if (record != NULL) return *(void* const*)((const char*)record + 0x14);
#endif
    return NULL;
}

uint32_t path_record_payload_count_get(const void* record)
{
    PathRecordPayloadPort* port = path_record_payload_port_find(record);
    if (port != NULL) return port->count;
    return record != NULL ? *(const uint32_t*)((const char*)record + 0x18)
                          : 0u;
}

void path_record_surface_grid_bind(TerrainSurfaceGrid* grid)
{
    if (s_path_record_surface_grid != NULL &&
        s_path_record_surface_grid != grid) {
        LevelState* prior_owner = s_path_record_surface_grid->owner_level;
        s_path_record_surface_grid->owner_level = NULL;
        if (prior_owner != NULL)
            terrain_surface_grid_terrain_window_set(
                s_path_record_surface_grid, NULL);
        if (prior_owner != NULL && prior_owner->ptr_tile_mesh_grid != NULL &&
            prior_owner->ptr_tile_mesh_grid->surface_grid ==
                s_path_record_surface_grid) {
            prior_owner->ptr_tile_mesh_grid->surface_grid = NULL;
        }
    }
    s_path_record_surface_grid = grid;
    if (grid != NULL) {
        GameContext* game = entity_pool_callback_context_get();
        LevelState* current_level = game != NULL ? game->level : NULL;
        if (current_level == NULL)
            current_level = (LevelState*)map_cell_record_grid_level_get();
        if (current_level != NULL) {
            if (grid->owner_level == NULL)
                grid->owner_level = current_level;
            if (grid->owner_level == current_level)
                grid->record_width = (uint32_t)level_width(current_level);
            if (grid->owner_level == current_level &&
                current_level->ptr_tile_mesh_grid != NULL)
                current_level->ptr_tile_mesh_grid->surface_grid = grid;
            if (grid->owner_level == current_level &&
                current_level->ptr_terrain != NULL)
                terrain_surface_grid_terrain_window_set(
                    grid, terrain_cell_variant_window_get(
                        current_level->ptr_terrain));
        }
        else if (grid->owner_level != NULL) {
            grid->record_width = (uint32_t)level_width(grid->owner_level);
            if (grid->owner_level->ptr_terrain != NULL)
                terrain_surface_grid_terrain_window_set(
                    grid, terrain_cell_variant_window_get(
                        grid->owner_level->ptr_terrain));
        }
    }
}

TerrainSurfaceGrid* path_record_surface_grid_get(void)
{
    GameContext* game = entity_pool_callback_context_get();
    LevelState* current_level = game != NULL ? game->level : NULL;
    if (current_level == NULL)
        current_level = (LevelState*)map_cell_record_grid_level_get();
    if (current_level != NULL &&
        current_level->ptr_tile_mesh_grid != NULL &&
        current_level->ptr_tile_mesh_grid->surface_grid != NULL) {
        return current_level->ptr_tile_mesh_grid->surface_grid;
    }
    return s_path_record_surface_grid;
}

TerrainSurfaceGrid* level_surface_grid_get(const LevelState* level)
{
    TerrainSurfaceGrid* surface;
    TerrainState* terrain = level != NULL
        ? (TerrainState*)level->ptr_terrain : NULL;
    if (level != NULL && level->ptr_tile_mesh_grid != NULL &&
        level->ptr_tile_mesh_grid->surface_grid != NULL) {
        surface = level->ptr_tile_mesh_grid->surface_grid;
        if (terrain != NULL) {
            terrain_surface_grid_terrain_window_set(
                surface, terrain_cell_variant_window_get(terrain));
            /* FUN_0044f230's +0x28 table is TerrainCreate's 0x28-byte
             * physical block image. Bind that live image when no explicit
             * surface record owner was supplied. */
            if (surface->cell_records == NULL)
                surface->cell_records = terrain->blocks;
            if (surface->record_width == 0u)
                surface->record_width = terrain->dim[2];
        }
        return surface;
    }
    surface = path_record_surface_grid_get();
    if (surface != NULL && surface->owner_level != NULL &&
        surface->owner_level != level) return NULL;
    if (surface != NULL && terrain != NULL) {
        terrain_surface_grid_terrain_window_set(
            surface, terrain_cell_variant_window_get(terrain));
        if (surface->cell_records == NULL)
            surface->cell_records = terrain->blocks;
        if (surface->record_width == 0u)
            surface->record_width = terrain->dim[2];
    }
    return surface;
}

void level_terrain_material_transition_mesh_hook_set(
    LevelTerrainMaterialTransitionMeshHook hook, void* context)
{
    s_material_transition_mesh_hook = hook;
    s_material_transition_mesh_context = context;
}

int level_cell_path_record_set(LevelState* level, int x, int y,
                               void* record)
{
#if defined(_WIN32)
    uint8_t* cell;
    if (level == NULL) return 0;
    cell = level_voxel(level, x, y);
    if (cell == NULL) return 0;
    *(void**)(cell + 0x10) = record;
    return 1;
#else
    uint8_t* cell;
    CellPathRecordPort* port;
    CellPathRecordPort* previous;
    uintptr_t record_address;
    if (level == NULL) return 0;
    cell = level_voxel(level, x, y);
    if (cell == NULL) return 0;
    port = cell_path_record_port_find(level, x, y);
    if (record == NULL) {
        if (port != NULL) {
            previous = NULL;
            for (CellPathRecordPort* current = s_cell_path_record_ports;
                 current != NULL; current = current->next) {
                if (current == port) {
                    if (previous != NULL) previous->next = current->next;
                    else s_cell_path_record_ports = current->next;
                    engine_heap_free(current);
                    break;
                }
                previous = current;
            }
        }
    }
    else {
        if (port == NULL) {
            port = (CellPathRecordPort*)engine_heap_alloc(
                (long)sizeof(CellPathRecordPort));
            if (port == NULL) return 0;
            port->level = level;
            port->x = x;
            port->y = y;
            port->next = s_cell_path_record_ports;
            s_cell_path_record_ports = port;
        }
        port->record = record;
    }
    record_address = (uintptr_t)record;
    *(uint32_t*)(cell + 0x10) =
        (uint32_t)(record_address & 0xffffffffu);
    return 1;
#endif
}

/* The two terrain path-record operations share this exact voxel transition:
 * registration sets the active bit and record link, while teardown clears
 * them; an adjacent duplicate point controls the paired-point bit. */
int level_cell_path_record_state_apply(LevelState* level, int x, int y,
                                       void* record, int is_registering,
                                       int duplicate_point)
{
    uint8_t* cell = level_voxel(level, x, y);
    uint32_t* flags;
    if (cell == NULL) return 0;
    flags = (uint32_t*)(cell + 0x08);
    if (is_registering) {
        if (!level_cell_path_record_set(level, x, y, record)) return 0;
        if (duplicate_point) *flags |= 0x8000u;
        *flags |= 0x100000u;
    }
    else {
        if (!level_cell_path_record_set(level, x, y, NULL)) return 0;
        if (duplicate_point) *flags &= ~0x8000u;
        *flags &= ~0x100000u;
    }
    level_cell_init_pass(level, x, y);
    return 1;
}

void* level_cell_path_record_get(const LevelState* level, int x, int y)
{
    CellPathRecordPort* port = cell_path_record_port_find(level, x, y);
    if (port != NULL) return port->record;
#if defined(_WIN32)
    {
        const uint8_t* cell = level_voxel(level, x, y);
        return cell != NULL ? *(void* const*)(cell + 0x10) : NULL;
    }
#else
    return NULL;
#endif
}

int path_record_entity_append(void* record, struct EntitySlot* entity)
{
#if defined(_WIN32)
    uint32_t count;
    if (record == NULL || entity == NULL) return 0;
    count = *(uint32_t*)((char*)record + 0xcc);
    if (count >= 40u) return 0;
    *(struct EntitySlot**)((char*)record + 0x2c + count * 4u) = entity;
    *(uint32_t*)((char*)record + 0xcc) = count + 1u;
    /* CountMatchingCoordinatesInList (0x408e10) walks this same pointer
     * array using the independent progress/count word at +0x20. Retail's
     * OperationGroupAppendObject advances both words for every appended
     * entity. */
    *(uint32_t*)((char*)record + 0x20) =
        *(uint32_t*)((char*)record + 0x20) + 1u;
    return 1;
#else
    PathRecordEntityPort* port;
    uintptr_t entity_address;
    if (record == NULL || entity == NULL) return 0;
    port = path_record_entity_port_find(record);
    if (port == NULL) {
        port = (PathRecordEntityPort*)engine_heap_alloc(
            (long)sizeof(PathRecordEntityPort));
        if (port == NULL) return 0;
        port->record = record;
        port->count = 0u;
        port->next = s_path_record_entity_ports;
        s_path_record_entity_ports = port;
    }
    if (port->count >= 40u) return 0;
    port->entities[port->count] = entity;
    entity_address = (uintptr_t)entity;
    *(uint32_t*)((char*)record + 0x2c + port->count * 4u) =
        (uint32_t)(entity_address & 0xffffffffu);
    ++port->count;
    *(uint32_t*)((char*)record + 0xcc) = port->count;
    *(uint32_t*)((char*)record + 0x20) = port->count;
    return 1;
#endif
}

uint32_t path_record_entity_count_get(const void* record)
{
    PathRecordEntityPort* port = path_record_entity_port_find(record);
    if (port != NULL) return port->count;
    return record != NULL ? *(const uint32_t*)((const char*)record + 0xcc)
                          : 0u;
}

struct EntitySlot* path_record_entity_get(const void* record, uint32_t index)
{
    PathRecordEntityPort* port = path_record_entity_port_find(record);
    if (port != NULL)
        return index < port->count ? port->entities[index] : NULL;
#if defined(_WIN32)
    if (record != NULL && index < path_record_entity_count_get(record))
        return *(struct EntitySlot* const*)((const char*)record + 0x2c +
                                            index * 4u);
#endif
    return NULL;
}

void path_record_cell_links_release(LevelState* level, const void* record)
{
    CellPathRecordPort* previous = NULL;
    CellPathRecordPort* port = s_cell_path_record_ports;
    while (port != NULL) {
        CellPathRecordPort* next = port->next;
        if ((level == NULL || port->level == level) &&
            (record == NULL || port->record == record)) {
            if (port->level != NULL) {
                uint8_t* cell = level_voxel(port->level, port->x, port->y);
                if (cell != NULL) *(uint32_t*)(cell + 0x10) = 0u;
            }
            if (previous != NULL) previous->next = next;
            else s_cell_path_record_ports = next;
            engine_heap_free(port);
        }
        else {
            previous = port;
        }
        port = next;
    }
}

void path_record_release(void* record)
{
    path_record_remove_and_free(record);
}

/* FUNCTION: LEGORR 0x00409300
 * The original first checks whether the coordinate pair already has a
 * path descriptor, then creates and links a 0xd4-byte path record and
 * sets bit 1 in its final dword. This portable entry point models the
 * creation branch; callers that need duplicate suppression perform that
 * check before calling it. */
void* tutorial_path_descriptor_create(const uint32_t* coord_pair)
{
    char* record;
    if (coord_pair == NULL) return NULL;
    record = (char*)path_record_allocate_and_link(coord_pair);
    if (record == NULL) return NULL;
    *(uint32_t*)(record + 0xd0) |= 2u;
    return record;
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
uint32_t path_record_count_peek(void) { return s_path_record_count; }
#endif /* !LEGORRR_BACKEND_WINAPI */
void* path_record_last_peek(void) { return s_path_record_last; }

/* Test hook: stages a count-table slot (the corpus's table writers are
 * unported, so the faithful read shape is exercised through this hook —
 * the install-hook precedent). */
void grid_count_table_slot_poke(int row, int column, int slot, int table,
                                uint32_t value)
{
    const int64_t idx = grid_count_index(row, column, slot);
    if (idx < 0) {
        return; /* the bounds deviation rides through */
    }
    if (table != 0) {
        s_grid_count_second[(size_t)idx] = value;
    } else {
        s_grid_count_first[(size_t)idx] = value;
    }
}

/* Explicit-level adapter retained for callers that already own a context.
 * The mapped retail entry point below uses the current global level. */
void terrain_cell_queued_work_transition(const struct GameContext* ctx,
                                         int x, int y)
{
    const LevelState* lvl = (ctx != NULL) ? ctx->level : NULL;
    if (lvl == NULL) {
        return;
    }
    const uint32_t flags = cell_flag_word(lvl, x, y);
    if ((flags & 0x10u) == 0u || (flags & 0xe0u) != 0u) {
        return;
    }
    uint8_t* rec = level_voxel(lvl, x, y);
    if (rec == NULL) {
        return; /* deviation: the corpus strides OOB cells blindly */
    }
    uint32_t* word = (uint32_t*)(rec + 0x08);
    *word = *word | 0x20u;

    cell_init_pass((LevelState*)lvl, (float)x, (uint32_t)y);
    /* FUN_004311d0 receives param3 == 0 here; it initializes local matrix
     * data and returns without creating or changing the build record. */
    const int pair[2] = { x, y };
    coordinate_callback_dispatch(pair, 0);      /* 0x402b30 seam */
    message_dispatch(0x1c, 0, 0, pair);         /* 0x419b80 */
}

typedef char lava_erosion_pending_cell_stride_check[
    sizeof(LavaErosionPendingCell) == 8u ? 1 : -1];
int s_erosion_triggered_this_tick;
unsigned int s_lava_erosion_mutation_threshold = 4u;
LavaErosionEdgeApplyHook s_lava_erosion_edge_apply;
LavaErosionCellActionHook s_lava_erosion_work_clear;
LavaErosionCellActionHook s_lava_erosion_cell_change;
LavaErosionSoundHook s_lava_erosion_sound;
LavaErosionEventHook s_lava_erosion_event;
void* s_lava_erosion_hook_context;

static void lava_erosion_runtime_tables_clear(void)
{
    memset(s_erosion_marks, 0, sizeof(s_erosion_marks));
    memset(s_lava_erosion_pending_cells, 0,
           sizeof(s_lava_erosion_pending_cells));
    memset(s_lava_erosion_stage_coordinates, 0,
           sizeof(s_lava_erosion_stage_coordinates));
    memset(s_lava_erosion_stage_timers, 0,
           sizeof(s_lava_erosion_stage_timers));
    memset(s_ts_active, 0, sizeof(s_ts_active));
    memset(s_ts_coordinates, 0, sizeof(s_ts_coordinates));
    memset(s_ts_stamps, 0, sizeof(s_ts_stamps));
    s_lava_erosion_pending_count = 0u;
    s_erosion_trigger_accumulator = 0.0f;
    s_ts_now = 0;
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
float erosion_decay_scale_peek(void) { return s_erosion_decay_scale; }
float erosion_delay_scale_peek(void) { return s_erosion_delay_scale; }
int erosion_timestamp_tick_peek(void) { return s_ts_now; }

int erosion_scratch_peek(int which, int index)
{
    if (which == 0) {
        return (index >= 0 && index < EROSION_MARK_COUNT)
                   ? (int)s_erosion_marks[index] : -1;
    }
    return (index >= 0 && index < EROSION_TICKET_COUNT)
               ? (int)s_ts_active[index] : -1;
}

int lava_erosion_neighbor_check_level(const LevelState* s, int seed_x,
                                     int seed_y, int out_xy[2])
{
    const uint8_t* seed = level_voxel(s, seed_x, seed_y);
    if (seed == NULL || seed[2] != 6) {
        return 0; /* deviation: guarded seed (corpus strides blindly) */
    }
    /* Corpus neighbour deltas; the first x-dword is uninitialized stack
     * residue, pinned to 0 (the cardinal ring). */
    static const int dx[4] = { 0, 1, 0, -1 };
    static const int dy[4] = { -1, 0, 1, 0 };
    for (int i = 0; i != 4; ++i) {
        const int nx = seed_x + dx[i];
        const int ny = seed_y + dy[i];
        if (out_xy != NULL) {
            out_xy[0] = nx;
            out_xy[1] = ny;
        }
        const uint8_t* rec = level_voxel(s, nx, ny);
        if (rec == NULL) {
            continue; /* deviation: out-of-bounds neighbour (corpus strides) */
        }
        uint32_t flags = 0;
        __builtin_memcpy(&flags, rec + 8, sizeof flags);
        if ((flags & 0x80000000u) == 0 && (flags & 8u) != 0 &&
            rec[6] != 0) {
            const int candidate_xy[2] = { nx, ny };
            if (cell_coordinate_timestamp_exists(candidate_xy) == 0u) {
                return 1;
            }
        }
    }
    return 0;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

/* LavaErosionStageRegister (FUN_0040ea10): claim the first inactive stage
 * slot, seed the cell byte and countdown, then append it to the ordered
 * pending-candidate table. */
int lava_erosion_stage_register(const int xy[2], int stage)
{
    int slot;
    uint8_t* cell;
    if (xy == NULL || s_lava_erosion_level_context == NULL) return 0;
    cell = level_voxel(s_lava_erosion_level_context, xy[0], xy[1]);
    if (cell == NULL) return 0;
    if (!lava_erosion_stage_slot_find_free(&slot)) return 0;
    cell[7] = (uint8_t)stage;
    s_lava_erosion_stage_coordinates[slot].x = xy[0];
    s_lava_erosion_stage_coordinates[slot].y = xy[1];
    s_lava_erosion_stage_timers[slot] = lava_erosion_delay_from_cell(xy) /
                                        (float)(stage + 1);
    s_erosion_marks[slot] = 1u;
    lava_erosion_pending_cell_add(xy, 1);
    return 1;
}

unsigned int lava_erosion_pending_cell_count_get(void)
{
    return s_lava_erosion_pending_count;
}

int lava_erosion_pending_cell_get(unsigned int index, int xy_out[2])
{
    if (xy_out == NULL || index >= s_lava_erosion_pending_count) return 0;
    xy_out[0] = s_lava_erosion_pending_cells[index].x;
    xy_out[1] = s_lava_erosion_pending_cells[index].y;
    return 1;
}

int lava_erosion_stage_get(unsigned int slot, int xy_out[2],
                           float* timer_out, uint8_t* cell_stage_out)
{
    const uint8_t* cell;
    if (slot >= EROSION_MARK_COUNT || s_erosion_marks[slot] == 0u) return 0;
    if (xy_out != NULL) {
        xy_out[0] = s_lava_erosion_stage_coordinates[slot].x;
        xy_out[1] = s_lava_erosion_stage_coordinates[slot].y;
    }
    if (timer_out != NULL) *timer_out = s_lava_erosion_stage_timers[slot];
    cell = level_voxel(s_lava_erosion_level_context,
        s_lava_erosion_stage_coordinates[slot].x,
        s_lava_erosion_stage_coordinates[slot].y);
    if (cell_stage_out != NULL) *cell_stage_out = cell != NULL ? cell[7] : 0u;
    return 1;
}

void lava_erosion_level_context_set(LevelState* s)
{
    if (s_lava_erosion_level_context != s) {
        lava_erosion_runtime_tables_clear();
        if (s == NULL) {
            s_erosion_decay_scale = 0.0f;
            s_erosion_delay_scale = 0.0f;
            s_lava_erosion_mutation_threshold = 4u;
        }
    }
    s_lava_erosion_level_context = s;
}

void lava_erosion_level_context_release(LevelState* s)
{
    if (s_lava_erosion_level_context == s) {
        lava_erosion_level_context_set(NULL);
    }
}

/* LavaErosionNeighborEligible (FUN_0040f2b0): bit 3 is the base candidate
 * gate; reserved cells and terrain types 6, 9, and 7 are excluded. The
 * reference checks an optional surface owner before honoring the edge-wall
 * flag. The portable level record does not expose that owner yet, so a raised
 * edge-wall flag is conservatively treated as a rejection. */
int lava_erosion_neighbor_eligible(const LevelState* s, int x, int y)
{
    const int type = level_tile_type(s, x, y);
    if (s == NULL || level_cell_flag_8_bounded(s, x, y) == 0u ||
        level_cell_flag_reserved(s, x, y) != 0u || type == 6 || type == 9 ||
        type == 7) {
        return 0;
    }
    return level_int(s, LEVEL_OFF_SAFECAVERNS) == 0 ||
           level_cell_flag_edge_wall(s, x, y) == 0u;
}

/* The engine adapter keeps a trigger result for its portable test counter;
 * the mapped retail entry above is void and leaves EAX unused. */
int lava_erosion_pending_cell_tick_result(float scale)
{
    lava_erosion_pending_cell_tick(scale);
    return s_erosion_triggered_this_tick;
}

void lava_erosion_hooks_set(LavaErosionEdgeApplyHook edge_apply,
                            LavaErosionCellActionHook work_clear,
                            LavaErosionCellActionHook cell_change,
                            LavaErosionSoundHook sound,
                            LavaErosionEventHook event,
                            void* context)
{
    s_lava_erosion_edge_apply = edge_apply;
    s_lava_erosion_work_clear = work_clear;
    s_lava_erosion_cell_change = cell_change;
    s_lava_erosion_sound = sound;
    s_lava_erosion_event = event;
    s_lava_erosion_hook_context = context;
}

void lava_erosion_mutation_threshold_set(unsigned int threshold)
{
    s_lava_erosion_mutation_threshold = threshold;
}

/* --- the cell route + neighbour pick band (2026-10-25) -----------------------
 * TerrainCellRouteAndOccupancyCheck (FUN_00431da0) answers whether an
 * entity may move from cell (from_x, from_y) to (to_x, to_y). The corpus
 * indexes a cardinal delta table with a dword walker, so a neighbour hit
 * in slot 0 reads the same as "no cardinal neighbour" (pinned quirk).
 * The mapped x86 entry uses the retail six-argument ABI and reads cell
 * records directly from the active level's +0xb8 voxel pointer. Its world
 * position check uses the port's entity-position adapter, whose explicit
 * level argument and return contract do not reproduce the renderer-owned
 * scene-position call exactly. The separate explicit-level helper retains
 * bounds and NULL guards for callers that operate on arbitrary levels.
 *
 * FirstFreeNeighborCellPick (FUN_00431c70) walks the four cardinal
 * neighbours in a rotation that advances by the corpus's DAT_004de588
 * pick counter (own storage here; peek hook), calling the route check
 * with source and destination pinned to the neighbour cell. In mode 1 a
 * neighbour that is in bounds, open (flag 0x10) and unblocked (flag 8
 * clear) is skipped; otherwise it is tried. The mapped x86 entry uses the
 * retail stack delta table and reloads the active-level global after each
 * route check. The portable fallback uses the guarded sidecar APIs. */
const int s_cardinal_deltas[4][2] = {{0, -1}, {1, 0}, {0, 1},
                                            {-1, 0}};

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
uint32_t neighbor_pick_rotation_peek(void) { return s_neighbor_pick_rotation; }
#endif /* !LEGORRR_BACKEND_WINAPI */

static int route_cell_byte(const LevelState* s, int x, int y, int off)
{
    const uint8_t* rec = level_voxel(s, x, y);
    return rec != NULL ? (int)(signed char)rec[off] : 0;
}

uint32_t route_cell_flags(const LevelState* s, int x, int y)
{
    const uint8_t* rec = level_voxel(s, x, y);
    uint32_t flags = 0u;
    if (rec != NULL) {
        __builtin_memcpy(&flags, rec + 0x08, sizeof flags);
    }
    return flags;
}

int terrain_cell_route_and_occupancy_check(const struct EntitySlot* entity,
                                           const LevelState* s,
                                           int from_x, int from_y,
                                           int to_x, int to_y, int strict)
{
    if (entity != NULL
        && (entity_stats_word_get(entity) & 0x20000u) != 0u
        && entity->owner_36c == NULL) {
        return 0;
    }

    /* Cardinal delta walk; 4 marks "no hit" (the corpus's walker dword
     * ends past the table). Slot 0 hits as delta_index 0 — the corpus's
     * "param_1 == 0" gate quirk. */
    int delta_index = 4;
    for (int i = 0; i != 4; ++i) {
        if (s_cardinal_deltas[i][0] + to_x == from_x
            && to_y + s_cardinal_deltas[i][1] == from_y) {
            delta_index = i;
            break;
        }
    }

    uint32_t block_relief = 0u; /* corpus +14c word & 0x200 */
    uint32_t special = 0u;      /* stats & 1 */
    uint32_t idle_gate = 0u;    /* stats & 0x1000000 */
    uint32_t walk_gate = 1u;    /* stats & 0x2000000 (initial pass) */
    uint32_t state_mask = 0u;   /* state & 0x200000 */
    if (entity != NULL) {
        if (entity->type == 0xb) {
            return 0;
        }
        block_relief = entity_capability_flags_get(entity) & 0x200u;
        const uint32_t stats = entity_stats_word_get(entity);
        special = stats & 1u;
        idle_gate = stats & 0x1000000u;
        walk_gate = stats & 0x2000000u;
        state_mask = entity_state_flags_get(entity) & 0x200000u;
        if (strict == 0) {
            /* Retail resolves the entity's world position through the active
             * terrain grid picker before applying this source-cell fast path. */
            int entity_cell_x = -1;
            int entity_cell_y = -1;
            if (s != NULL && s->ptr_terrain != NULL) {
                entity_position_to_terrain_cell(entity, s, &entity_cell_x,
                                                &entity_cell_y);
                if (entity_cell_x == from_x && entity_cell_y == from_y)
                    return 1;
            }
        }
    }

    const int type = route_cell_byte(s, from_x, from_y, 0x02);
    if (type == 9) {
        if (idle_gate == 0u) {
            return 0;
        }
    } else if (walk_gate == 0u) {
        return 0;
    }
    if (type == 6 && state_mask == 0u
        && (route_cell_flags(s, from_x, from_y) & 0x200u) == 0u) {
        return 0;
    }

    const uint32_t flags = route_cell_flags(s, from_x, from_y);
    if ((flags & 0x400u) != 0u) {
        return 0;
    }
    if (strict != 0 && (flags & 0x40u) != 0u) {
        return 0;
    }
    if (block_relief == 0u && (route_cell_byte(s, from_x, from_y, 0x0c) & 4) != 0) {
        return 0;
    }

    int answer = 1;
    if (special == 0u) {
        if ((flags & 8u) == 0u) {
            return 0;
        }
    } else {
        if ((flags & 0x10u) != 0u) {
            answer = 3; /* the corpus promotes the answer on an open cell */
        }
        if (route_cell_byte(s, from_x, from_y, 0x00) == 0) {
            return 0; /* *local_28 == 0: the cell's state byte reads unset */
        }
        if ((flags & 0x10u) != 0u) {
            const int source_type = route_cell_byte(s, from_x, from_y, 0x02);
            const int above = route_cell_byte(s, from_x, from_y - 1, 0x02);
            if (source_type == 6 || above == 6 ||
                source_type == 7 || above == 7) {
                return 0; /* source/above wall types block the open-cell route */
            }
        }
        if (delta_index == 0) {
            if ((flags & 0x2000u) != 0u ||
                (route_cell_flags(s, to_x, to_y) & 0x2000u) != 0u) {
                answer = 2;
            }
        }
    }

    if (to_x != from_x && to_y != from_y) {
        /* diagonal corner pair along the destination row */
        if ((route_cell_flags(s, from_x, to_y) & 8u) != 0u
            && (route_cell_flags(s, to_x, from_y) & 8u) != 0u) {
            return 2;
        }
    }
    return answer;
}

int first_free_neighbor_cell_pick(const struct EntitySlot* entity,
                                  const LevelState* s, const int* cell_in,
                                  int* cell_out, int mode)
{
    ++s_neighbor_pick_rotation;
    const int x = cell_in[0], y = cell_in[1];
    for (int i = 0; i != 4; ++i) {
        const uint32_t k
            = (s_neighbor_pick_rotation + (uint32_t)i) & 3u;
        const int nx = s_cardinal_deltas[k][0] + x;
        const int ny = s_cardinal_deltas[k][1] + y;
        if (mode != 0
            && (unsigned)nx < (unsigned)level_width(s) - 1u
            && (unsigned)ny < (unsigned)level_height(s) - 1u) {
            const uint32_t flags = route_cell_flags(s, nx, ny);
            if ((flags & 0x10u) != 0u && (flags & 8u) == 0u) {
                continue; /* corpus bVar2: already-open cell is skipped */
            }
        }
        if (terrain_cell_route_and_occupancy_check(entity, s, nx, ny, nx,
                                                   ny, 0) != 0) {
            cell_out[0] = nx;
            cell_out[1] = ny;
            return 1;
        }
    }
    return 0;
}

namespace unported_ledger_stubs {

/* Adapter only: SurfaceTypeDescriptionsConfigLoad (FUN_004350a0) takes the
 * config and section owners, then writes the original global tables. The
 * caller-supplied entries/hooks seam does not reproduce those owners. */
void ledger_stub_004350a0_SurfaceTypeDescriptionsConfigLoad(
    SurfaceTypeDescriptionEntry entries[18],
    const SurfaceTypeDescriptionHooks* hooks)
{
    surface_type_descriptions_load(entries, hooks);
}

} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

// TaskCountAtGridCoordinateQuery
int ledger_stub_0043b360_TaskCountAtGridCoordinateQuery(
    uint32_t x, uint32_t y, EntitySlot** out_entity)
{
    return entity_task_at_grid_coordinate_find(x, y, out_entity);
}

} // namespace unported_ledger_stubs

/* CountMatchingCoordinatesInList (FUN_00408e10) walks the member pointer
 * array at +0x2c for the independent +0x20 comparison count. +0xcc is the
 * pointer append count used by entity teardown; append keeps the two equal,
 * while the direct helper follows +0x20 as retail does. */
uint32_t path_record_count_matching_coordinates(const void* record,
                                                int type, int row)
{
    uint32_t count, index, matches = 0u;
    if (record == NULL) return 0u;
    memcpy(&count, (const char*)record + 0x20, sizeof(count));
    for (index = 0u; index < count; ++index) {
        const EntitySlot* entity = path_record_entity_get(record, index);
        if (entity != NULL && entity->type == type && entity->etype == row)
            ++matches;
    }
    return matches;
}

namespace unported_ledger_stubs {

int ledger_stub_00408d70_TerrainDescriptorCoordinateCountLimitCheck(
    uint32_t descriptor_key, int type, int row)
{
    return terrain_descriptor_coordinate_count_limit_check(
        descriptor_key, type, row);
}

} // namespace unported_ledger_stubs

uint32_t terrain_kind_nine_neighbor_emit_for_footprint_cells(
    const int cell_xy[2], const int* footprint_xy, uint32_t footprint_count)
{
    static const int neighbor_xy[4][2] = {
        { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 }
    };
    GameContext* context = entity_pool_callback_context_get();
    LevelState* level = context != NULL ? context->level : NULL;
    uint8_t* descriptor = (uint8_t*)path_record_last_peek();
    uint32_t emitted = 0u;
    float scale;
    float edge;
    float command_offsets[4][2];
    uint32_t segment;

    if (level == NULL) level = level_current_owner_get();
    if (cell_xy == NULL || footprint_xy == NULL || footprint_count == 0u ||
        level == NULL || level->ptr_terrain == NULL) return 0u;

    /* TerrainDescriptorFindExact(cell_xy, 0): records are head-linked at
     * +0x24 and key the requested cell at +0x0c/+0x10. */
    while (descriptor != NULL) {
        int descriptor_x, descriptor_y;
        memcpy(&descriptor_x, descriptor + 0x0c, sizeof(descriptor_x));
        memcpy(&descriptor_y, descriptor + 0x10, sizeof(descriptor_y));
        if (descriptor_x == cell_xy[0] && descriptor_y == cell_xy[1]) break;
        descriptor = (uint8_t*)*(void**)(descriptor + 0x24);
    }
    if (descriptor == NULL) return 0u;

    /* The original obtains the terrain handle from the current level and
     * uses its +0x10 block size to place the four edge command anchors. */
    scale = terrain_blocksize_get(level->ptr_terrain);
    if (scale == 0.0f) return 0u;
    edge = 8.8f / scale;
    command_offsets[0][0] = 0.5f;
    command_offsets[0][1] = 1.0f - edge;
    command_offsets[1][0] = 1.0f - edge;
    command_offsets[1][1] = 0.5f;
    command_offsets[2][0] = 0.5f;
    command_offsets[2][1] = edge;
    command_offsets[3][0] = edge;
    command_offsets[3][1] = 0.5f;

    for (segment = 0u; segment < footprint_count; ++segment) {
        const int* point = footprint_xy + segment * 2u;
        int is_duplicate = segment + 1u < footprint_count &&
            point[0] == point[2] && point[1] == point[3];
        int direction;
        if (is_duplicate) {
            /* Duplicate consecutive coordinates delimit the same interior
             * pair in the corpus loop; it advances over both entries. */
            ++segment;
            continue;
        }

        for (direction = 0; direction < 4; ++direction) {
            int neighbor_x = point[0] + neighbor_xy[direction][0];
            int neighbor_y = point[1] + neighbor_xy[direction][1];
            uint32_t candidate;
            int inside = 0;
            for (candidate = 0u; candidate < footprint_count;) {
                const int* candidate_xy = footprint_xy + candidate * 2u;
                if (candidate + 1u < footprint_count &&
                    candidate_xy[0] == candidate_xy[2] &&
                    candidate_xy[1] == candidate_xy[3]) {
                    /* The corpus's paired-coordinate walk advances past a
                     * duplicate pair without considering either entry. */
                    candidate += 2u;
                    continue;
                }
                if (candidate_xy[0] == neighbor_x &&
                    candidate_xy[1] == neighbor_y) {
                    inside = 1;
                    break;
                }
                ++candidate;
            }
            if (!inside) {
                uint32_t descriptor_key;
                memcpy(&descriptor_key, descriptor + 0x08,
                       sizeof(descriptor_key));
                cmd_command_kind_record_issue(
                    9u, 0u, 0u, point, descriptor_key,
                    command_offsets[direction], 1);
                ++(*(uint32_t*)(descriptor + 0x1c));
                ++emitted;
            }
        }
    }
    return emitted;
}

namespace unported_ledger_stubs {

uint32_t ledger_stub_00408ec0_KindNineNeighborEmitForFootprintCells(
    const int cell_xy[2], const int* footprint_xy, uint32_t footprint_count)
{
    return terrain_kind_nine_neighbor_emit_for_footprint_cells(
        cell_xy, footprint_xy, footprint_count);
}

void ledger_stub_004093b0_PassableAdjacentCellSearchExpand(
    const int cell_xy[2])
{
    placement_cell_search_expand(cell_xy);
}

int nearby_placement_entity_search_current(const int cell_xy[2],
                                           int resource_amount)
{
    return ledger_stub_00409470_NeighborCellEntitySearchRecurse(
        cell_xy, resource_amount);
}

static int entity_path_footprint_record_get(const EntitySlot* entity,
                                            const TypeConfigTable** table_out,
                                            const uint32_t** entries_out,
                                            uint32_t* count_out,
                                            uint32_t* token_out)
{
    const TypeConfigTable* table;
    const uint8_t* row_data;
    const uint32_t* entries;
    uint32_t row_kind, row, shape_token, count;
    if (table_out != NULL) *table_out = NULL;
    if (entries_out != NULL) *entries_out = NULL;
    if (count_out != NULL) *count_out = 0u;
    if (token_out != NULL) *token_out = 0u;
    /* Retail indexes DAT_005561e0 directly with the dword at entity +4.
     * EntitySlot::etype is that row index; resolving type_name here changes
     * behavior for renamed/aliased objects and needlessly depends on the
     * name table being loaded. */
    if (entity == NULL || entity->etype < 0) return 0;
    row_kind = 2u; /* DAT_005561e0 is the BuildingTypes table. */
    table = entity_type_config_table_get(row_kind);
    row = (uint32_t)entity->etype;
    if (table == NULL || table->rows == NULL || table->row_stride < 0x8cu ||
        row >= table->count) return 0;
    row_data = (const uint8_t*)table->rows + (size_t)row * table->row_stride;
    memcpy(&shape_token, row_data + 0x84u, sizeof(shape_token));
    memcpy(&count, row_data + 0x88u, sizeof(count));
    entries = type_config_shape_table_resolve(table, shape_token);
    if (entries == NULL || count == 0u || count > 10u) return 0;
    if (table_out != NULL) *table_out = table;
    if (entries_out != NULL) *entries_out = entries;
    if (count_out != NULL) *count_out = count;
    if (token_out != NULL) *token_out = shape_token;
    return 1;
}

static int entity_path_footprint_cell_get(const TypeConfigTable* table,
                                          const uint32_t* entries,
                                          uint32_t shape_token,
                                          uint32_t index, int cell_xy[2])
{
    const char* x_text;
    const char* y_text;
    uint32_t x_token, y_token;
    if (table == NULL || entries == NULL || cell_xy == NULL) return 0;
    /* The type parser reserves pair zero as the entity's anchor; subsequent
     * pairs are string tokens in the BuildingTypes Shape record. */
    if (index == 0u) {
        cell_xy[0] = 0;
        cell_xy[1] = 0;
        return 1;
    }
    x_token = entries[index * 2u];
    y_token = entries[index * 2u + 1u];
    x_text = type_config_shape_string_resolve(table, shape_token, x_token);
    y_text = type_config_shape_string_resolve(table, shape_token, y_token);
    if (x_text == NULL || y_text == NULL) return 0;
    cell_xy[0] = atoi(x_text);
    cell_xy[1] = atoi(y_text);
    return 1;
}

void entity_cell_path_effects_apply_level(EntitySlot* entity,
                                                  LevelState* level)
{
    const TypeConfigTable* table;
    const uint32_t* footprint;
    uint32_t count;
    uint32_t shape_token;
    int source_cell[2];
    int direction;
    float direction_xy[2];
    void* component;
    uint32_t i;
    if (entity == NULL || level == NULL ||
        !entity_path_footprint_record_get(entity, &table, &footprint,
                                          &count, &shape_token)) {
        entity_placement_commands_build_once(entity);
        return;
    }
    entity_position_to_terrain_cell(entity, level, &source_cell[0],
                                    &source_cell[1]);
    /* Retail 0x4422a0 reads the type component via 0x4a200, then asks its
     * renderer surface for the normalized orientation vector. The host
     * component handle has no production writer yet; visual-resource cache
     * entries and velocity are not proven substitutes. */
    component = entity->type_component_handle;
    if (!entity_surface_vector_normalize_if_available(component,
                                                       direction_xy)) {
        /* The scene surface-vector provider is not installed in this build.
         * Preserve the decompiled final command setup without mutating cells
         * using a guessed direction. */
        entity_placement_commands_build_once(entity);
        return;
    }
    if (fabs((double)direction_xy[0]) <=
        fabs((double)direction_xy[1])) {
        direction = direction_xy[1] <= 0.0f ? 2 : 0;
    }
    else {
        direction = direction_xy[0] <= 0.0f ? 3 : 1;
    }
    for (i = 0; i < count; ++i) {
        int offset_xy[2];
        int dx, dy;
        int cell_xy[2];
        int repeated = 0;
        if (!entity_path_footprint_cell_get(table, footprint, shape_token,
                                            i, offset_xy)) continue;
        dx = offset_xy[0];
        dy = offset_xy[1];
        if (direction == 1) { int old_dx = dx; dx = -dy; dy = old_dx; }
        else if (direction == 2) { dx = -dx; dy = -dy; }
        else if (direction == 3) { int old_dx = dx; dx = dy; dy = -old_dx; }
        cell_xy[0] = source_cell[0] + dx;
        cell_xy[1] = source_cell[1] + dy;
        if (i + 1u < count) {
            int next_offset[2];
            int next_dx, next_dy;
            int next_x, next_y;
            if (!entity_path_footprint_cell_get(table, footprint, shape_token,
                                                i + 1u, next_offset)) {
                next_offset[0] = 0;
                next_offset[1] = 0;
            }
            next_dx = next_offset[0];
            next_dy = next_offset[1];
            if (direction == 1) {
                int old_dx = next_dx; next_dx = -next_dy; next_dy = old_dx;
            }
            else if (direction == 2) {
                next_dx = -next_dx; next_dy = -next_dy;
            }
            else if (direction == 3) {
                int old_dx = next_dx; next_dx = next_dy; next_dy = -old_dx;
            }
            next_x = source_cell[0] + next_dx;
            next_y = source_cell[1] + next_dy;
            repeated = cell_xy[0] == next_x && cell_xy[1] == next_y;
        }

        terrain_cell_primary_flags_mask_clear_and_refresh(
            level, cell_xy[0], cell_xy[1]);
        level_cell_secondary_bit0_clear(level, cell_xy[0], cell_xy[1]);
        if ((entity->flags_3ec & 8u) == 0u) {
            float position[3];
            float rotation[3];
            if (terrain_cell_world_position_get(cell_xy[0], cell_xy[1],
                                                position)) {
                d3drm_vector_random(rotation);
                (void)lava_effect_select_and_place(3, position, rotation);
            }
            terrain_cell_work_flag_clear(level, cell_xy, 0x3bu);
        }
        terrain_cell_and_neighbors_update(cell_xy[0], cell_xy[1]);
        if (repeated) ++i;
    }
    entity_placement_commands_build_once(entity);
}

/* Explicit-level compatibility entry point retained for port callers. The
 * type record, rotation, terrain operations, and effect slots are resolved
 * internally; the former hook bundle is ignored. */
void entity_cell_path_effects_apply(
    EntitySlot* entity, LevelState* level,
    const EntityCellPathEffectHooks* hooks)
{
    (void)hooks;
    if (level == NULL && entity != NULL) {
        GameContext* context = entity->owner_ctx;
        if (context != NULL) level = context->level;
    }
    entity_cell_path_effects_apply_level(entity, level);
}

// EntityPlacementCommandsBuildOnce
void ledger_stub_00409f20_EntityPlacementCommandsBuildOnce(EntitySlot* entity)
{
    entity_placement_commands_build_once(entity);
}

// LavaErosionNeighborMutate
void ledger_stub_0040f330_LavaErosionNeighborMutate(
    const int xy[2], unsigned int direction, int propagate)
{
    lava_erosion_neighbor_mutate(xy, direction, propagate);
}

// ErosionEightNeighborPropagate
void ledger_stub_0040f5f0_ErosionEightNeighborPropagate(
    const int xy[2], unsigned int direction)
{
    lava_erosion_eight_neighbor_propagate(xy, direction);
}

// LavaErosionCellEligibleAndMutate
int ledger_stub_0040f190_LavaErosionCellEligibleAndMutate(
    const int xy[2], int hint)
{
    return lava_erosion_cell_eligible_and_mutate(xy, hint);
}

} // namespace unported_ledger_stubs
