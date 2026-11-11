// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-026 GAME-026: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/GAME-026.csv. */
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <stdint.h>
#include "terrain_grid.hpp"
#include "../gods98/block.hpp"
#include "level.hpp"
#include "game.hpp"
#include "entity.hpp"
#include "weapon.hpp"
#include "util.hpp"
#include "scene_render.hpp"
#include "../crt/crt.hpp"
#include "../gods98/heap.hpp"
#include "../gods98/resource.hpp"
#include "../gods98/record.hpp"
#include "../gods98/input.hpp"
#include "../gods98/message.hpp"
#include "../gods98/texture.hpp"
#include "../gods98/surface.hpp"
#include "../gods98/view.hpp"
#include "../gods98/engine.hpp"
#include <stdlib.h>
#include "terrain_grid_shared.hpp"
#include "gods98/media.hpp"
#include "effects.hpp"
#include "gods98/sound.hpp"
#include "gods98/simtick.hpp"
#include "gods98/string.hpp"
#include "gods98/camera.hpp"
#include "type_config_ports.hpp"
#include "ai.hpp"
#include "cmd.hpp"
#include "upgrades.hpp"
#include "spawn.hpp"
#include "port_loop_ports.hpp"
#include "frontend.hpp"
#include "objective.hpp"
#include "gods98/flic.hpp"
#include "select.hpp"
#include "tutorial_hud_ports.hpp"
#include "level_shared.hpp"
#include "screen_pick.hpp"
#include "frameloop.hpp"
#include "input_shared.hpp"
#include <ctype.h>
#include "model.hpp"
#include "record_shared.hpp"
#include "game/render.hpp"
#include "simtick_shared.hpp"
#include <stdarg.h>
#include "scene_resources.hpp"
#include "scene_pool.hpp"
#include "view_shared.hpp"
#include "screen_pick_shared.hpp"

typedef struct TerrainIndexedMeshLayerOwner {
    void* reserved[3];
    void* mesh;
} TerrainIndexedMeshLayerOwner;

typedef struct TerrainIndexedMeshLayerObject {
    void* reserved[3];
    TerrainIndexedMeshLayerOwner* owner;
} TerrainIndexedMeshLayerObject;

/* --- surface-type writer pair (2026-10-24, see level.hpp) -------------------
 * The corpus's appearance table (DAT_004a52c0, 12-byte entries) ships the
 * reference data's six color triplets: white, half-gray, brown, light
 * green, blue, yellow. Larger type indexes read past the table into
 * undefined data in the corpus; the port substitutes white. The triplets
 * feed the level-owned SceneRender cell-color cache; alpha is fixed at 1.0
 * in every target call. */
static const float s_surface_appearance[6][3] = {
    { 1.0f, 1.0f, 1.0f },   /* 0 white */
    { 0.5f, 0.5f, 0.5f },   /* 1 half-gray */
    { 1.0f, 0.5f, 0.4f },   /* 2 brown */
    { 0.5f, 1.0f, 0.5f },   /* 3 light green */
    { 0.4f, 0.5f, 1.0f },   /* 4 blue */
    { 0.6f, 0.6f, 0.0f },   /* 5 yellow */
};

static int terrain_grid_cell_valid(const TerrainSurfaceGrid* grid,
                                   uint32_t x, uint32_t y);
static uint32_t terrain_grid_cell_index(const TerrainSurfaceGrid* grid,
                                        uint32_t x, uint32_t y);

static int terrain_surface_grid_cell_orientation_get(
    const TerrainSurfaceGrid* grid, uint32_t x, uint32_t y, uint32_t index);

extern "C" float random_float_range(float minimum, float maximum);

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static void terrain_create_vertices_write(void* context, uint32_t cell, const float vertices[12]);
static int terrain_indexed_mesh_create(TerrainState* terrain, size_t quad_count);
static LevelState* terrain_material_transition_owner_get( TerrainSurfaceGrid* grid);
static int terrain_material_transition_vertex_payload_get( TerrainState* terrain, uint32_t x, uint32_t y, float out[4][9]);
static int terrain_material_transition_vertices_get( LevelState* level, TerrainState* terrain, TerrainSurfaceGrid* grid, uint32_t x, uint32_t y, float out[12]);
static void terrain_indexed_mesh_layer_object_init( TerrainState* terrain, TerrainIndexedMeshLayerObject* object, TerrainIndexedMeshLayerOwner* owner);
static int terrain_surface_record_bounds_get(const TerrainSurfaceGrid* grid, uint32_t* width, uint32_t* height);
static uint32_t terrain_surface_record_width(const TerrainSurfaceGrid* grid);
static void marker_cell(EngineState* st, uint32_t x, uint32_t y);
static int terrain_surface_triangle_height(const float a[3], const float b[3], const float c[3], float x, float z, float* height_out);
static int terrain_ray_triangle_hit(const float origin[3], const float direction[3], const float a[3], const float b[3], const float c[3], float hit[3]);

// FUNCTION: LEGORR 0x0044e0c0
void* terrain_create_retail(void* engine_context, const char* surface_map,
                            float blocksize, float terrain_value)
{
    /* The retail entry receives DAT_005560d8 as its scene/resource owner.
     * Retain that owner with the CPU terrain even when its DDi mesh callbacks
     * are unavailable. */
    MapFile* m = level_map_open(surface_map);
    if (m == NULL) {
        return NULL;
    }
    TerrainState* t = (TerrainState*)calloc(1, sizeof(TerrainState));
    if (t == NULL) {
        level_map_close(m);
        return NULL;
    }
    t->width = m->width;
    t->height = m->height;
    t->dim[0] = (uint32_t)(m->width - 1);
    t->dim[1] = (uint32_t)(m->height - 1);
    t->dim[2] = (uint32_t)m->width;
    t->dim[3] = (uint32_t)m->height;
    t->blocksize = blocksize;
    t->terrain_value = terrain_value;
    t->scene_context = engine_context;
    t->world_off_x = -(float)(m->width - 1) * blocksize * 0.5f;
    t->world_off_y = (float)(m->height - 1) * blocksize * 0.5f;

    size_t n = (size_t)m->width * m->height;
    t->blocks = (uint8_t*)calloc(n, TERRAIN_BLOCK_STRIDE);
    if (t->blocks == NULL) {
        level_map_close(m);
        free(t);
        return NULL;
    }
    t->variant_window = (uint8_t*)calloc(1u, CELL_VARIANT_WINDOW_SIZE);
    if (t->variant_window == NULL) {
        free(t->blocks);
        level_map_close(m);
        free(t);
        return NULL;
    }
    /* The retail TerrainState begins with the dimensions/stride and stores
     * its block table at +0x28; the compatibility lists occupy +0x34 and
     * +0x4034. Keep that view in a native sidecar because host pointers make
     * the compact 32-bit object layout unsuitable for inline tables. */
    memcpy(t->variant_window + 0x00, &t->dim[0], sizeof(uint32_t));
    memcpy(t->variant_window + 0x04, &t->dim[1], sizeof(uint32_t));
    memcpy(t->variant_window + 0x08, &t->dim[2], sizeof(uint32_t));
    memcpy(t->variant_window + 0x28, &t->blocks, sizeof(t->blocks));
    int x, y;
    for (y = 0; y < m->height; ++y) {
        for (x = 0; x < m->width; ++x) {
            uint8_t* rec = t->blocks
                           + ((size_t)y * m->width + x) * TERRAIN_BLOCK_STRIDE;
            rec[0x18] = 0;
            rec[0x19] = 0x60;
            rec[0x1A] = (uint8_t)level_map_cell(m, x, y);
            /* FUN_0044e0c0 draws two shared-stream values, normalizes their
             * vector, and stores it at +0x1c/+0x20 with magnitude 0.3. */
            float nx = random_float_range(0.0f, 1.0f);
            float ny = random_float_range(0.0f, 1.0f);
            float inv = 1.0f / (float)sqrt((double)(nx * nx + ny * ny));
            nx *= inv * 0.3f;
            ny *= inv * 0.3f;
            memcpy(rec + 0x1C, &nx, sizeof(nx));
            memcpy(rec + 0x20, &ny, sizeof(ny));
        }
    }
    /* TerrainGridVertexBuild creates the CPU copy of each retail quad. The
     * render backend can later mirror these records into its native mesh. */
    t->minimum_y = 0.0f;
    if (m->width > 1 && m->height > 1) {
        size_t quad_count = (size_t)(m->width - 1) * (size_t)(m->height - 1);
        if (quad_count > ((size_t)-1) / (12u * sizeof(float))) {
            terrain_destroy(t);
            level_map_close(m);
            return NULL;
        }
        t->cell_vertices = (float*)calloc(quad_count * 12u,
                                          sizeof(float));
        t->cell_detail_flags = (uint32_t*)calloc(quad_count,
                                                 sizeof(uint32_t));
        if (t->cell_vertices == NULL || t->cell_detail_flags == NULL) {
            terrain_destroy(t);
            level_map_close(m);
            return NULL;
        }
        /* TerrainCreate allocates one 4-vertex mesh record per quad, then
         * LwoLayerEnabledSet(scene, index, 1) sets its indexed +0x6c bit.
         * MeshRecordAllocateAndFill has zeroed that word first, so this is
         * the complete initial per-cell detail state. */
        {
            size_t detail_index;
            for (detail_index = 0u; detail_index < quad_count;
                 ++detail_index)
                t->cell_detail_flags[detail_index] = 1u;
        }
        {
            TerrainVertexBuildGrid build;
            memset(&build, 0, sizeof(build));
            build.width = (uint32_t)(m->width - 1);
            build.height = (uint32_t)(m->height - 1);
            build.map_width = (uint32_t)m->width;
            build.map_height = (uint32_t)m->height;
            /* FUN_0044e4d0 uses context +0x14 (TerrainCreate's fourth
             * argument) for height, while +0x10 controls X/Z spacing. */
            build.vertical_scale = terrain_value;
            build.spacing = blocksize;
            build.x_origin = t->world_off_x;
            build.z_origin = t->world_off_y;
            build.minimum_y = 0.0f;
            build.cell_records = t->blocks;
            build.context = t;
            build.vertices_write = terrain_create_vertices_write;
            terrain_grid_vertex_build(&build);
            t->minimum_y = build.minimum_y;
        }
    }
    /* Choose each quad's diagonal by comparing the two opposite-corner
     * height deltas. The lower delta is orientation bit 0 (finding 114).
     * Retail rebuilds both stored triangle vectors for every quad; choosing
     * a new orientation also marks the cell and its three neighboring
     * records dirty through TerrainCellOrientationToggleAndRebuild. */
    for (y = 0; y + 1 < m->height; ++y) {
        for (x = 0; x + 1 < m->width; ++x) {
            uint8_t* top_left = t->blocks +
                ((size_t)y * (size_t)m->width + (size_t)x) *
                    TERRAIN_BLOCK_STRIDE;
            uint8_t* top_right = top_left + TERRAIN_BLOCK_STRIDE;
            uint8_t* bottom_left = top_left +
                (size_t)m->width * TERRAIN_BLOCK_STRIDE;
            uint8_t* bottom_right = bottom_left + TERRAIN_BLOCK_STRIDE;
            int diagonal_ad = (int)top_left[0x1A] -
                              (int)bottom_right[0x1A];
            int diagonal_bc = (int)top_right[0x1A] -
                              (int)bottom_left[0x1A];
            if (diagonal_ad < 0) diagonal_ad = -diagonal_ad;
            if (diagonal_bc < 0) diagonal_bc = -diagonal_bc;
            if (diagonal_ad < diagonal_bc) {
                top_left[0x19] |= 1u;
                terrain_cell_changed_flag_set(t, x, y);
                terrain_cell_changed_flag_set(t, x + 1, y);
                terrain_cell_changed_flag_set(t, x + 1, y + 1);
                terrain_cell_changed_flag_set(t, x, y + 1);
            }
            terrain_create_cell_surface_vectors_rebuild(
                t, (uint32_t)y * t->dim[0] + (uint32_t)x,
                (uint32_t)y * t->dim[2] + (uint32_t)x,
                (top_left[0x19] & 1u) != 0u);
        }
    }
    if (m->width > 1 && m->height > 1) {
        size_t quad_count = (size_t)t->dim[0] * (size_t)t->dim[1];
        if (!terrain_indexed_mesh_create(t, quad_count)) {
            terrain_destroy(t);
            level_map_close(m);
            return NULL;
        }
    }
    level_map_close(m);
    return t;
}

// FUNCTION: LEGORR 0x0044e4d0
void terrain_grid_vertex_build(TerrainVertexBuildGrid* grid)
{
    static const float corners[4][2] = {
        { 0.0f, 0.0f }, { 1.0f, 0.0f },
        { 1.0f, 1.0f }, { 0.0f, 1.0f }
    };
    static const uint8_t sample_x_offset[4] = { 0u, 1u, 1u, 0u };
    static const uint8_t sample_y_offset[4] = { 0u, 0u, 1u, 1u };
    uint32_t x, y;
    if (grid == NULL || grid->cell_records == NULL ||
        grid->vertices_write == NULL || grid->map_width == 0 ||
        grid->map_height == 0 || grid->width >= grid->map_width ||
        grid->height >= grid->map_height) return;
    for (y = 0; y < grid->height; ++y) {
        for (x = 0; x < grid->width; ++x) {
            float vertices[12];
            unsigned int corner;
            for (corner = 0; corner < 4; ++corner) {
                int sample_x = (int)x + (int)sample_x_offset[corner];
                int sample_y = (int)y + (int)sample_y_offset[corner];
                unsigned long sample_index;
                float height;
                sample_index = ((unsigned long)sample_y * grid->map_width +
                                (unsigned long)sample_x) * 0x28ul;
                height = -(float)grid->cell_records[sample_index + 0x1aul] *
                         grid->vertical_scale;
                vertices[corner * 3] = ((float)x + corners[corner][0]) *
                                       grid->spacing + grid->x_origin;
                vertices[corner * 3 + 1] = grid->z_origin -
                    ((float)y + corners[corner][1]) * grid->spacing;
                vertices[corner * 3 + 2] = height;
                if (height < grid->minimum_y) grid->minimum_y = height;
            }
            grid->vertices_write(grid->context, y * grid->width + x, vertices);
        }
    }
}
// FUNCTION: LEGORR 0x0044e670
void nested_pointer_record_destroy(NestedPointerRecord* record)
{
    if (record == NULL) return;
    if (s_nested_handle_release_hook != NULL) {
        s_nested_handle_release_hook(record->first_handle);
        s_nested_handle_release_hook(record->second_handle);
    }
    if (s_nested_memory_release_hook != NULL) {
        s_nested_memory_release_hook(record->child_buffer);
        s_nested_memory_release_hook(record);
    }
}

// FUNCTION: LEGORR 0x0044e6b0
void indexed_flag_value_set_one(void* record, uint32_t index)
{
    if (record == NULL) {
        return;
    }
    const uint32_t one = 1u;
    const size_t off = 0x40b4u + (size_t)(index & 0xffu) * 4u;
    __builtin_memcpy((char*)record + off, &one, sizeof(one));
}

// FUNCTION: LEGORR 0x0044e6d0
void terrain_cell_material_transition_begin(TerrainSurfaceGrid* grid,
                                            uint32_t x, uint32_t y,
                                            int material,
                                            int rotation)
{
    LevelState* level = terrain_material_transition_owner_get(grid);
    TerrainState* terrain = level != NULL
        ? (TerrainState*)level->ptr_terrain : NULL;
    uint32_t index = 0u;
    uint32_t record_width;
    size_t record_index;
    uint8_t prior_material;
    float vertices[12];
    float vertex_payload[4][9];
    int have_vertices = 0;
    if (level == NULL || (grid == NULL && terrain == NULL)) return;
    /* The retail routine receives the terrain renderer object directly. The
     * port may enter through a helper that has only the level, so recover its
     * live surface owner before applying the final material operation. */
    if (grid == NULL) grid = level_surface_grid_get(level);
    if (grid != NULL) {
        if (x >= grid->width || y >= grid->height) return;
        index = y * grid->width + x;
    }
    else if (terrain == NULL || x >= terrain->dim[0] ||
             y >= terrain->dim[1]) return;
    record_width = grid != NULL && grid->record_width != 0u
        ? grid->record_width
        : (uint32_t)level_width(level);
    if (record_width == 0u && grid != NULL) record_width = grid->width;
    record_index = (size_t)y * record_width + x;
    if (terrain != NULL && terrain->blocks != NULL &&
        record_index < (size_t)terrain->dim[2] * terrain->dim[3]) {
        prior_material = terrain->blocks[record_index * 0x28u + 0x18u];
    }
    else if (grid != NULL && grid->cell_records != NULL) {
        prior_material = grid->cell_records[record_index * 0x28u + 0x18u];
    }
    else {
        prior_material = (uint8_t)material;
    }

    if (prior_material != (uint8_t)material &&
        level->material_transition_suppress_flags[prior_material] == 0u) {
        have_vertices = terrain_material_transition_vertices_get(
            level, terrain, grid, x, y, vertices) &&
            terrain_material_transition_vertex_payload_get(
                terrain, x, y, vertex_payload);
    }
    if (have_vertices) {
        uint32_t slot_index;
        for (slot_index = 0u; slot_index < 10u; ++slot_index) {
            LevelTerrainMaterialTransitionSlot* slot =
                &level->material_transition_slots[slot_index];
            uint32_t corner;
            if ((slot->flags & 1u) != 0u) continue;
            slot->timer = 0.05f;
            slot->cell_x = (int32_t)x;
            slot->cell_y = (int32_t)y;
            for (corner = 0u; corner < 4u; ++corner) {
                slot->saved_uv[corner][0] = vertex_payload[corner][6];
                slot->saved_uv[corner][1] = vertex_payload[corner][7];
            }
            slot->reserved = 0u;
            slot->flags |= 1u;
            if (s_material_transition_mesh_hook != NULL) {
                (void)s_material_transition_mesh_hook(
                    s_material_transition_mesh_context, level, slot_index, 0,
                    (int)x, (int)y, vertices, 1.0f);
            }
            break;
        }
    }

    /* The reference invokes the material setter whether or not a transition
     * slot was available. */
    if (grid != NULL)
        terrain_cell_material_set(grid, x, y, (uint8_t)material,
                                  (uint8_t)rotation);
    if (terrain != NULL && terrain->blocks != NULL &&
        record_index < (size_t)terrain->dim[2] * terrain->dim[3])
        terrain->blocks[record_index * 0x28u + 0x18u] = (uint8_t)material;
}
/* Retail GridCellSetPayloadIndex asks the TerrainCreate scene object at
 * +0x24 for the detail entry at width * y + x, then discards the result.
 * Keep this target wrapper separate from the host eligibility predicate
 * below, which reads the same mirrored enable bit. */
// FUNCTION: LEGORR 0x0044e860
void grid_cell_set_payload_index(void* terrain_handle, int x, int y)
{
    TerrainState* terrain = (TerrainState*)terrain_handle;
    TerrainIndexedMeshLayerObject object;
    TerrainIndexedMeshLayerOwner owner;
    unsigned int entry_index;
    if (terrain == NULL) return;
    entry_index = (unsigned int)((int64_t)terrain->dim[2] * y + x);
    terrain_indexed_mesh_layer_object_init(terrain, &object, &owner);
    (void)terrain_detail_entry_get_at_index(&object, entry_index);
}

// FUNCTION: LEGORR 0x0044e880
void terrain_grid_cells_initialize(TerrainSurfaceGrid* grid)
{
    uint32_t x;
    uint32_t y;
    if (grid == NULL) return;
    for (y = 0; y < grid->height; ++y) {
        for (x = 0; x < grid->width; ++x) {
            if (grid->initialize_cell != NULL)
                grid->initialize_cell(grid->initialize_context, x, y);
            else
                terrain_cell_surface_vectors_initialize(grid, x, y);
        }
    }
}

/* SurfaceVertexSpanValidate (FUN_0044e8c0): check every distinct cell in a
 * sampled terrain span. Adjacent corner heights must stay within the edge
 * limit; cells carrying the terrain height-check bit must also stay within
 * the requested total height range. */
// FUNCTION: LEGORR 0x0044E8C0
int terrain_surface_vertex_span_validate(TerrainSurfaceGrid* grid,
                                         const int* cell_xy,
                                         uint32_t cell_count,
                                         float maximum_height_span,
                                         float maximum_edge_delta)
{
    static const uint8_t corner_cell_offsets[4][2] = {
        { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 }
    };
    static const uint8_t edge_corners[6][2] = {
        { 0, 1 }, { 0, 2 }, { 0, 3 }, { 1, 2 }, { 1, 3 }, { 2, 3 }
    };
    uint32_t i;
    int have_flagged_height = 0;
    uint32_t record_width = 0u;
    uint32_t record_height = 0u;
    float minimum_height = 10000.0f;
    float maximum_height = -10000.0f;
    if (grid == NULL || (cell_count != 0 && cell_xy == NULL)) return 0;
    (void)terrain_surface_record_bounds_get(grid, &record_width,
                                            &record_height);
    i = 0u;
    while (i < cell_count) {
        float vertices[12];
        uint32_t corner;
        uint32_t edge;
        int x = cell_xy[i * 2];
        int y = cell_xy[i * 2 + 1];
        /* The corpus advances the pair cursor by two when adjacent entries
         * match, skipping both entries in that duplicate step. */
        if (i + 1u < cell_count &&
            x == cell_xy[(i + 1u) * 2u] &&
            y == cell_xy[(i + 1u) * 2u + 1u]) {
            i += 2u;
            continue;
        }
        if (grid->surface_vectors == NULL || x < 0 || y < 0 ||
            (uint32_t)x >= grid->width || (uint32_t)y >= grid->height)
            return 0;
        terrain_cell_surface_vertices_get(grid, (uint32_t)x, (uint32_t)y,
                                           vertices);
        for (edge = 0; edge < 6; ++edge) {
            float first = vertices[edge_corners[edge][0] * 3];
            float second = vertices[edge_corners[edge][1] * 3];
            float delta = first - second;
            if (delta < 0.0f) delta = -delta;
            if (delta > maximum_edge_delta) return 0;
        }
        if (grid->cell_records != NULL && record_width != 0u) {
            for (corner = 0u; corner < 4u; ++corner) {
                const int record_x = x + corner_cell_offsets[corner][0];
                const int record_y = y + corner_cell_offsets[corner][1];
                size_t record_index;
                float height;
                if (record_x < 0 || record_y < 0 ||
                    (uint32_t)record_x >= record_width ||
                    (uint32_t)record_y >= record_height) continue;
                record_index = (size_t)(uint32_t)record_y *
                    record_width + (uint32_t)record_x;
                if ((grid->cell_records[record_index * 0x28u + 0x19u] &
                     8u) == 0u) continue;
                height = vertices[corner * 3u];
                if (height < minimum_height) minimum_height = height;
                if (height > maximum_height) maximum_height = height;
                have_flagged_height = 1;
            }
        }
        ++i;
    }
    if (have_flagged_height &&
        maximum_height - minimum_height > maximum_height_span) return 0;
    return 1;
}

/* TerrainCellListSurfaceHeightClampMutate (FUN_0044ead0): use unprocessed
 * cell corners to compute one target height, bounded by already-processed
 * cells, then clamp each new corner to the tolerance band around that target. */
// FUNCTION: LEGORR 0x0044ead0
void terrain_cell_surface_height_clamp(TerrainSurfaceGrid* grid,
                                       const int* cell_xy,
                                       uint32_t cell_count,
                                       float maximum_offset)
{
    /* TerrainCellSurfaceVerticesGet returns corners in TerrainGridVertexBuild
     * order: (0,0), (1,0), (1,1), (0,1). The matching record flags must use
     * that same order in both passes. */
    const int corner_offsets[8] = { 0, 0, 1, 0, 1, 1, 0, 1 };
    float staged_vertices[118];
    uint32_t i;
    uint32_t staged_count = 0;
    float half_offset;
    float average = 0.0f;
    float flagged_minimum = 10000.0f;
    float flagged_maximum = -10000.0f;
    uint32_t unprocessed_count = 0;
    int have_flagged_height = 0;
    uint32_t record_width, record_height;
    if (grid == NULL || grid->surface_vectors == NULL ||
        grid->cell_records == NULL || cell_xy == NULL ||
        maximum_offset < 0.0f) return;
    if (!terrain_surface_record_bounds_get(grid, &record_width,
                                           &record_height)) return;
    half_offset = maximum_offset * 0.5f;

    /* The reference stages unique consecutive coordinate pairs, averages
     * corners of cells not already marked with bit 8, and uses processed
     * cells to bound that shared target height. */
    for (i = 0; i < cell_count; ++i) {
        int x = cell_xy[i * 2];
        int y = cell_xy[i * 2 + 1];
        float* vertices;
        uint32_t corner;
        if (i != 0u && x == cell_xy[(i - 1u) * 2u] &&
            y == cell_xy[(i - 1u) * 2u + 1u]) continue;
        if (x < 0 || y < 0 || (uint32_t)x >= grid->width ||
            (uint32_t)y >= grid->height) continue;
        /* The retail stack has room for nine complete 12-float vertex
         * records. Reject an invalid oversized list before writing past it. */
        if (staged_count >= 9u) return;
        vertices = staged_vertices + staged_count * 12u;
        terrain_cell_surface_vertices_get(grid, (uint32_t)x,
                                          (uint32_t)y, vertices);
        for (corner = 0; corner < 4; ++corner) {
            float height = vertices[corner * 3u];
            const int neighbor_x = x + corner_offsets[corner * 2u];
            const int neighbor_y = y + corner_offsets[corner * 2u + 1u];
            size_t neighbor_index;
            uint8_t flags;
            if (neighbor_x < 0 || neighbor_y < 0 ||
                (uint32_t)neighbor_x >= record_width ||
                (uint32_t)neighbor_y >= record_height) continue;
            neighbor_index = (size_t)neighbor_y * record_width +
                             (uint32_t)neighbor_x;
            flags = grid->cell_records[neighbor_index * 0x28u + 0x19u];
            if ((flags & 8u) != 0u) {
                if (height < flagged_minimum) flagged_minimum = height;
                if (height > flagged_maximum) flagged_maximum = height;
                have_flagged_height = 1;
            }
            else {
                average += height;
                unprocessed_count++;
            }
        }
        staged_count++;
    }
    if (unprocessed_count == 0u) return;
    average /= (float)unprocessed_count;
    if (have_flagged_height) {
        if (flagged_maximum < average + half_offset)
            average = flagged_maximum - half_offset;
        if (average - half_offset < flagged_minimum)
            average = flagged_minimum + half_offset;
    }

    staged_count = 0;
    for (i = 0; i < cell_count; ++i) {
        int x = cell_xy[i * 2];
        int y = cell_xy[i * 2 + 1];
        float* vertices;
        uint32_t corner;
        if (i != 0u && x == cell_xy[(i - 1u) * 2u] &&
            y == cell_xy[(i - 1u) * 2u + 1u]) continue;
        if (x < 0 || y < 0 || (uint32_t)x >= grid->width ||
            (uint32_t)y >= grid->height) continue;
        if (staged_count >= 9u) return;
        vertices = staged_vertices + staged_count * 12u;
        for (corner = 0; corner < 4; ++corner) {
            const int neighbor_x = x + corner_offsets[corner * 2u];
            const int neighbor_y = y + corner_offsets[corner * 2u + 1u];
            size_t neighbor_index;
            uint8_t* flags;
            float difference = vertices[corner * 3u] - average;
            if (neighbor_x < 0 || neighbor_y < 0 ||
                (uint32_t)neighbor_x >= record_width ||
                (uint32_t)neighbor_y >= record_height) continue;
            neighbor_index = (size_t)neighbor_y * record_width +
                             (uint32_t)neighbor_x;
            flags = &grid->cell_records[neighbor_index * 0x28u + 0x19u];
            if ((*flags & 8u) != 0u) continue;
            if (difference > half_offset || difference < -half_offset) {
                terrain_cell_surface_voxel_neighbors_mutate(
                    grid, (uint32_t)neighbor_x, (uint32_t)neighbor_y,
                    ((difference / difference) * half_offset + average) -
                        vertices[corner * 3u]);
            }
            *flags |= 8u;
        }
        staged_count++;
    }
}
/* TerrainCellOrientationToggleAndRebuild (FUN_0044edf0): switch the quad's
 * diagonal by cyclically rotating its four stored vertex records. Retail
 * writes the corresponding bit at cell +0x19, rebuilds the two triangle
 * normals, and marks the cell and four shared-corner records dirty. */
// FUNCTION: LEGORR 0x0044edf0
void terrain_cell_surface_orientation_set(TerrainSurfaceGrid* grid,
                                         uint32_t x, uint32_t y,
                                         int flipped)
{
    uint32_t index;
    uint32_t corner;
    uint32_t record_width, record_height;
    int old_flip;
    float rotated[12];
    float* vertices;
    if (!terrain_grid_cell_valid(grid, x, y) || grid->surface_vectors == NULL)
        return;
    index = terrain_grid_cell_index(grid, x, y);
    old_flip = terrain_surface_grid_cell_orientation_get(grid, x, y, index);
    flipped = flipped != 0;
    if (old_flip == flipped) return;

    vertices = grid->surface_vectors + index * 12u;
    for (corner = 0u; corner < 4u; ++corner) {
        uint32_t source = old_flip ? ((corner + 1u) & 3u)
                                   : ((corner + 3u) & 3u);
        memcpy(rotated + corner * 3u, vertices + source * 3u,
               3u * sizeof(float));
    }
    memcpy(vertices, rotated, sizeof(rotated));
    if (grid->cell_records != NULL) {
        uint8_t* flags = grid->cell_records +
            ((size_t)y * terrain_surface_record_width(grid) + x) * 0x28u +
            0x19u;
        *flags = (uint8_t)((*flags & (uint8_t)~1u) | (flipped ? 1u : 0u));
    }
    if (grid->triangulation_flags != NULL) {
        grid->triangulation_flags[index] = (uint8_t)(
            (grid->triangulation_flags[index] & (uint8_t)~1u) |
            (flipped ? 1u : 0u));
    }
    terrain_cell_surface_vectors_rebuild(grid, x, y);
    if (grid->cell_records != NULL) {
        static const int dirty_offsets[4][2] = {
            { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 }
        };
        record_width = terrain_surface_record_width(grid);
        record_height = grid->height;
        if (grid->terrain_window != NULL) {
            uint32_t window_height;
            memcpy(&window_height,
                   (const uint8_t*)grid->terrain_window + 4,
                   sizeof(window_height));
            if (window_height != 0u)
                record_height = window_height;
        }
        for (corner = 0u; corner < 4u; ++corner) {
            int dirty_x = (int)x + dirty_offsets[corner][0];
            int dirty_y = (int)y + dirty_offsets[corner][1];
            if (dirty_x >= 0 && dirty_y >= 0 &&
                (uint32_t)dirty_x < record_width &&
                (uint32_t)dirty_y < record_height) {
                size_t dirty_index = (size_t)dirty_y * record_width +
                                     (uint32_t)dirty_x;
                grid->cell_records[dirty_index * 0x28u + 0x19u] |= 4u;
            }
            if (grid->owner_level != NULL &&
                grid->owner_level->ptr_terrain != NULL) {
                terrain_cell_changed_flag_set(
                    grid->owner_level->ptr_terrain, dirty_x, dirty_y);
            }
        }
    }
    else if (grid->owner_level != NULL &&
             grid->owner_level->ptr_terrain != NULL) {
        static const int dirty_offsets[4][2] = {
            { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 }
        };
        for (corner = 0u; corner < 4u; ++corner) {
            terrain_cell_changed_flag_set(
                grid->owner_level->ptr_terrain,
                (int)x + dirty_offsets[corner][0],
                (int)y + dirty_offsets[corner][1]);
        }
    }
}

// FUNCTION: LEGORR 0x0044eff0
void simtick_marker_sweep(EngineContext* c, float dt, float scale)
{
    (void)scale;
    EngineState* st = c != NULL ? c->state_block : NULL;
    LevelState* level = level_current_owner_get();
    if (level != NULL) {
        /* The retail +0x73d0 dirty word and +0x28 cell records belong to the
         * active LevelState's TerrainState. EngineState::sim_grid is only a
         * test/legacy adapter and is not populated by the production level
         * loader. */
        s_marker_fired = 0;
        terrain_dirty_cell_vectors_refresh(level);
    }
    else if (st != NULL && (st->sim_marker_flags & 2u) != 0u) {
        s_marker_fired = 0;
        for (uint32_t y = 0; y < st->sim_h; ++y) {
            for (uint32_t x = 0; x < st->sim_w; ++x) {
                uint8_t* cell = (uint8_t*)st->sim_grid +
                    (y * st->sim_w + x) * 0x28;
                if ((cell[0x19] & 4u) != 0u) {
                    marker_cell(st, x, y);
                    cell[0x19] = cell[0x19] & ~4u;
                }
            }
        }
        st->sim_marker_flags = (uint8_t)(st->sim_marker_flags & ~2u);
    }
    terrain_material_transition_tick(path_record_surface_grid_get(), dt);
    /* FUN_00450b60's terrain-vertex interpolation pass still needs its
     * cell-list and renderer-owned vertex records. */
}

// FUNCTION: LEGORR 0x0044f090
void terrain_material_transition_tick(TerrainSurfaceGrid* grid, float delta)
{
    LevelState* level = terrain_material_transition_owner_get(grid);
    TerrainState* terrain = level != NULL
        ? (TerrainState*)level->ptr_terrain : NULL;
    uint32_t slot_index;
    if (level == NULL) return;
    if (grid == NULL) grid = level_surface_grid_get(level);
    for (slot_index = 0u; slot_index < 10u; ++slot_index) {
        LevelTerrainMaterialTransitionSlot* slot =
            &level->material_transition_slots[slot_index];
        float vertices[12];
        int have_vertices = 0;
        if ((slot->flags & 1u) == 0u) continue;
        if (slot->cell_x < 0 || slot->cell_y < 0) {
            slot->flags &= ~1u;
            continue;
        }
        have_vertices = terrain_material_transition_vertices_get(
            level, terrain, grid, (uint32_t)slot->cell_x,
            (uint32_t)slot->cell_y, vertices);
        if (!have_vertices) {
            slot->flags &= ~1u;
            continue;
        }
        if (s_material_transition_mesh_hook != NULL) {
            (void)s_material_transition_mesh_hook(
                s_material_transition_mesh_context, level, slot_index, 1,
                slot->cell_x, slot->cell_y, vertices, 1.0f - slot->timer);
        }
        slot->timer += delta * 0.04f;
        if (slot->timer > 1.0f) {
            if (s_material_transition_mesh_hook != NULL) {
                (void)s_material_transition_mesh_hook(
                    s_material_transition_mesh_context, level, slot_index, 2,
                    slot->cell_x, slot->cell_y, vertices,
                    1.0f - slot->timer);
            }
            slot->flags &= ~1u;
        }
    }
}

// FUNCTION: LEGORR 0x0044f1a0
void input_key_mapping_add_bidirectional(InputKeyMappingTable* table,
                                         uint8_t key_a, uint8_t key_b)
{
    uint32_t i;
    if (table == NULL) return;
    for (i = 0; i < table->count[key_a]; ++i) {
        if (table->values[key_a][i] == key_b) {
            return;
        }
    }
    table->values[key_a][table->count[key_a]] = key_b;
    ++table->count[key_a];
    table->values[key_b][table->count[key_b]] = key_a;
    ++table->count[key_b];
}

// FUNCTION: LEGORR 0x0044f220
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void scene_object_resource_handle_set(void* object,
                                                        uint32_t handle)
{
    __asm {
        mov ecx, dword ptr [esp + 4]
        mov eax, dword ptr [esp + 8]
        mov dword ptr [ecx + 2ch], eax
        ret
    }
}
#else
void scene_object_resource_handle_set(void* object, uint32_t handle)
{
    if (object == NULL) {
        return;
    }
    __builtin_memcpy((char*)object + 0x2cu, &handle, sizeof(handle));
}
#endif

// FUNCTION: LEGORR 0x0044f230
void terrain_cell_material_set(TerrainSurfaceGrid* grid, uint32_t x,
                               uint32_t y, uint8_t material,
                               uint8_t rotation)
{
    uint32_t index;
    uint32_t record_width;
    uint32_t handle = material;
    uint8_t flags;
    uint32_t i;
    static const uint8_t corner_order[4] = { 1, 3, 2, 0 };
    if (!terrain_grid_cell_valid(grid, x, y) || grid->cell_records == NULL)
        return;
    index = terrain_grid_cell_index(grid, x, y);
    record_width = grid->record_width != 0u
        ? grid->record_width : grid->width;
    flags = grid->cell_records[((size_t)y * record_width + x) *
                               0x28u + 0x19u];
    if (grid->material_handle_resolve != NULL)
        handle = grid->material_handle_resolve(grid->material_context, material);
    grid->cell_records[((size_t)y * record_width + x) * 0x28u +
                       0x18u] = material;
    if ((flags & 1u) != 0) rotation = (uint8_t)(rotation + 1u);
    if (grid->surface_palette_apply != NULL)
        grid->surface_palette_apply(grid->material_context, index, handle);
    if ((flags & 0x10u) == 0 && grid->texture_corner_values != NULL) {
        uint8_t* output = grid->texture_corner_values + index * 4u;
        for (i = 0; i < 4; ++i)
            output[i] = corner_order[(uint8_t)(i - rotation) & 3u];
    }
}

/* TerrainVoxelNeighborsMutate (FUN_0044f380): move the selected world-space
 * vertex in each adjacent cell's corner table, then rebuild both triangles
 * for the four cells sharing that vertex. */
// FUNCTION: LEGORR 0x0044f380
void terrain_cell_surface_voxel_neighbors_mutate(TerrainSurfaceGrid* grid,
                                                uint32_t x, uint32_t y,
                                                float height_delta)
{
    float corners[8] = { 0.0f, 0.0f, -1.0f, 0.0f,
                         -1.0f, -1.0f, 0.0f, -1.0f };
    float offset[3] = { 0.0f, 0.0f, height_delta };
    uint32_t corner;
    uint32_t cell_y;
    for (corner = 0; corner < 4; ++corner) {
        corners[corner * 2u] += (float)x;
        corners[corner * 2u + 1u] += (float)y;
    }
    for (corner = 0; corner < 4; ++corner) {
        const int cell_x = (int)corners[corner * 2u];
        const int vertex_y = (int)corners[corner * 2u + 1u];
        terrain_cell_surface_vector_offset_add(
            grid, (uint32_t)cell_x, (uint32_t)vertex_y, corner, offset);
    }
    for (cell_y = y - 1u; cell_y <= y; ++cell_y) {
        uint32_t cell_x;
        for (cell_x = x - 1u; cell_x <= x; ++cell_x) {
            terrain_cell_surface_vectors_rebuild(grid, cell_x, cell_y);
        }
    }
}
// FUNCTION: LEGORR 0x0044f490
void terrain_grid_operation_apply_all(TerrainSurfaceGrid* grid,
                                     uint32_t operation, void* context,
                                     TerrainGridOperationHook hook)
{
    uint32_t x;
    uint32_t y;
    if (grid != NULL && hook != NULL) {
        for (y = 0; y < grid->height; ++y) {
            for (x = 0; x < grid->width; ++x)
                hook(context, y * grid->width + x, operation);
        }
    }
}

// FUNCTION: LEGORR 0x0044f4e0
uint8_t terrain_cell_surface_type_set(void* terrain, int x, int y, int type)
{
    TerrainState* t = (TerrainState*)terrain;
    if (t == NULL || t->blocks == NULL) {
        return 0;
    }
    if (x < 0 || y < 0 || x >= t->width || y >= t->height) {
        return 0; /* deviation: the corpus indexes blind */
    }
    uint8_t* blk = t->blocks
        + ((size_t)y * (size_t)t->dim[2] + (size_t)x) *
            TERRAIN_BLOCK_STRIDE;
    const uint8_t prior = blk[0x24];
    blk[0x24] = (uint8_t)type;
    const float* c = ((unsigned)type < 6u)
        ? s_surface_appearance[type] : s_surface_appearance[0];
    /* The corpus looks up the triplet at table + type*0xc and hands it to
     * the appearance leaf (mode 1) — the seam half above. */
    terrain_cell_surface_appearance_apply(t, x, y, 1, c[0], c[1], c[2]);
    return prior;
}
/* TerrainCellSurfaceTypeGet (FUN_0044f540, finding 226): the per-cell
 * surface-type byte in the physical block table — the port's terrain
 * handle is the TerrainState model, so the corpus's record fields
 * +0x8/+0x28 (stride, table) map to the modeled width/table pair. Bounds
 * guard added — the corpus indexes blind (documented deviation). */
// FUNCTION: LEGORR 0x0044f540
int terrain_cell_surface_type_get(const void* terrain, int x, int y)
{
    const TerrainState* t = (const TerrainState*)terrain;
    if (t == NULL || t->blocks == NULL) {
        return 0;
    }
    if (x < 0 || y < 0 || x >= t->width || y >= t->height) {
        return 0;
    }
    return t->blocks[0x24 + ((size_t)y * (size_t)t->width + (size_t)x) * TERRAIN_BLOCK_STRIDE];
}

// FUNCTION: LEGORR 0x0044f570
void terrain_cell_surface_type_reset(void* terrain, int x, int y)
{
    /* corpus: the white appearance (1,1,1) through the leaf in mode 0,
     * then the byte cleared — the leaf's mode-0 half already clears. */
    terrain_cell_surface_appearance_apply(terrain, x, y, 0, 1.0f, 1.0f,
                                          1.0f);
}
// FUNCTION: LEGORR 0x0044f5c0
void terrain_cell_surface_appearance_apply(void* terrain, int x, int y,
                                           int mode, float r, float g,
                                           float b)
{
    TerrainState* t = (TerrainState*)terrain;
    if (t == NULL || t->blocks == NULL) {
        return;
    }
    if (x < 0 || y < 0 || x >= t->width || y >= t->height) {
        return; /* deviation: the corpus indexes blind */
    }
    uint8_t* blk = t->blocks
        + ((size_t)y * (size_t)t->dim[2] + (size_t)x) *
            TERRAIN_BLOCK_STRIDE;
    if (t->owner_level != NULL) {
        scene_render_terrain_cell_surface_color_set(
            t->owner_level, (uint32_t)x, (uint32_t)y,
            mode != 0 ? r : 1.0f,
            mode != 0 ? g : 1.0f,
            mode != 0 ? b : 1.0f);
    }
    if (mode == 0) {
        /* corpus: the mesh half gets (1,1,1) and (1,1,1) alpha, and the
         * byte is cleared. */
        blk[0x24] = 0u;
        (void)r; (void)g; (void)b;
    }
    /* FUN_00475020 writes the cell mesh's RGB and fixed opaque alpha. The
     * SceneRender cache stores the RGB under the same width-minus-one quad
     * stride; TerrainState's +0x24 appearance byte remains separate from the
     * material byte at +0x18. */
    (void)r; (void)g; (void)b;
}

/* MapCellToWorldCenter (FUN_0044f640, finding 225): the descriptor's
 * int fields convert to float at the use site — reproduced exactly,
 * including the unsigned bounds compare. */
// FUNCTION: LEGORR 0x0044f640
int map_cell_to_world_center(const void* map, uint32_t cell_x,
                             uint32_t cell_y, float* world_x, float* world_y)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    const uint32_t* fields = (const uint32_t*)map;
    if (cell_x < fields[0] && cell_y < fields[1]) {
        *world_x = *(const float*)((const char*)map + 0x10) * 0.5f +
            (float)cell_x * *(const float*)((const char*)map + 0x10) +
            *(const float*)((const char*)map + 0x18);
        *world_y = *(const float*)((const char*)map + 0x1c) -
            (*(const float*)((const char*)map + 0x10) * 0.5f +
             (float)cell_y * *(const float*)((const char*)map + 0x10));
        return 1;
    }
    return 0;
#else
    const uint32_t width = rec_dword(map, 0x00);
    const uint32_t height = rec_dword(map, 0x04);
    const float scale = rec_float(map, 0x10);
    const float origin_x = rec_float(map, 0x18);
    const float origin_y = rec_float(map, 0x1C);

    if (cell_x < width && cell_y < height) {
        if (world_x != NULL) {
            *world_x = scale * 0.5f + (float)cell_x * scale + origin_x;
        }
        if (world_y != NULL) {
            *world_y = origin_y - (scale * 0.5f + (float)cell_y * scale);
        }
        return 1;
    }
    return 0;
#endif
}

/* TerrainGridPick (FUN_0044f6d0): forward the same cell outputs and retain
 * the bounds result; the corpus's temporary subcell write is discarded. */
// FUNCTION: LEGORR 0x0044f6d0
int terrain_grid_pick(const void* map, float world_x, float world_y,
                      int* cell_x, int* cell_y)
{
    return world_to_terrain_cell_bounds_check(
        map, world_x, world_y, cell_x, cell_y, (float*)&cell_y);
}
// FUNCTION: LEGORR 0x0044f700
int world_to_terrain_cell_bounds_check(const void* map, float world_x,
                                       float world_y, int* cell_x,
                                       int* cell_y, float* subcell)
{
    int cx;
    int cy;
#if defined(_MSC_VER) && defined(_M_IX86)
    cx = (int)((world_x - *(const float*)((const char*)map + 0x18)) /
               *(const float*)((const char*)map + 0x10));
    cy = (int)((*(const float*)((const char*)map + 0x1c) - world_y) /
               *(const float*)((const char*)map + 0x10));
#else
    if (map == NULL) {
        if (cell_x != NULL) {
            *cell_x = (int)world_x;
        }
        if (cell_y != NULL) {
            *cell_y = (int)world_y;
        }
        return 0;
    }
    cx = (int)((world_x - rec_float(map, 0x18)) / rec_float(map, 0x10));
    cy = (int)((rec_float(map, 0x1c) - world_y) / rec_float(map, 0x10));
#endif
    if (cell_x != NULL) {
        *cell_x = cx;
    }
    if (cell_y != NULL) {
        *cell_y = cy;
    }
#if defined(_MSC_VER) && defined(_M_IX86)
    if (!(-1 < cx && cx < *(const int*)map && -1 < cy &&
          cy < ((const int*)map)[1])) {
#else
    if (!(-1 < cx && cx < (int)rec_dword(map, 0x00) && -1 < cy &&
          cy < (int)rec_dword(map, 0x04))) {
#endif
        return 0;
    }
    if (subcell != NULL) {
        const float bs = *(const float*)((const char*)map + 0x10);
        const float ox = *(const float*)((const char*)map + 0x18);
        const float oy = *(const float*)((const char*)map + 0x1C);
        float px = (world_x - ((float)cx * bs + ox)) / (bs * 0.5f);
        float py = -((world_y - (oy - (float)cy * bs)) / (bs * 0.5f));
        if (1.0f < px) {
            px = 2.0f - px;
        }
        if (1.0f < py) {
            py = 2.0f - py;
        }
        if (px < py) {
            py = px;
        }
        *subcell = py;
    }
    return 1;
}

// FUNCTION: LEGORR 0x0044f810
void grid_coordinate_clamp_and_store(const void* map, float world_x,
                                     float world_y, int* cell_x, int* cell_y)
{
    const int w = (int)rec_dword(map, 0x00);
    const int h = (int)rec_dword(map, 0x04);
    int cx = (int)world_x;
    int cy = (int)world_y;
    if (cx < 0) {
        cx = 0;
    }
    if (w <= cx) {
        cx = w - 1;
    }
    if (cy < 0) {
        cy = 0;
    }
    if (h <= cy) {
        cy = h - 1;
    }
    if (cell_x != NULL) {
        *cell_x = cx;
    }
    if (cell_y != NULL) {
        *cell_y = cy;
    }
}

// FUNCTION: LEGORR 0x0044f870
uint32_t terrain_world_point_to_cell(const void* map, const float* point,
                                     float* cell_out, float* offset_out)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    const void* map_source = map;
    const float* point_source = point;
    int picked;
    picked = terrain_grid_pick(map, point[0], point[1],
                               (int*)&map, (int*)&point);
    if (picked != 0) {
        const int cell_x = (int)(uint32_t)map;
        const int cell_y = (int)(uint32_t)point;
        if (cell_out != NULL) {
            cell_out[0] = (float)cell_x;
            cell_out[1] = (float)cell_y;
        }
        if (offset_out != NULL) {
            float center_x;
            float center_y;
            float inverse_scale;
            map_cell_to_world_center(map_source, (uint32_t)cell_x,
                                     (uint32_t)cell_y, &center_x, &center_y);
            offset_out[0] = point_source[0] - center_x;
            offset_out[1] = point_source[1] - center_y;
            inverse_scale = 1.0f /
                *(const float*)((const char*)map_source + 0x10);
            offset_out[0] = inverse_scale * offset_out[0] - -0.5f;
            offset_out[1] = inverse_scale * offset_out[1] - -0.5f;
        }
    }
#else
    int cx = 0, cy = 0;
    if (point != NULL
        && world_to_terrain_cell_bounds_check(map, point[0], point[1],
                                              &cx, &cy, NULL) != 0) {
        if (cell_out != NULL) {
            cell_out[0] = (float)cx;
            cell_out[1] = (float)cy;
        }
        if (offset_out != NULL) {
            float wx = 0.0f, wy = 0.0f;
            float inverse_scale;
            map_cell_to_world_center(map, (uint32_t)cx, (uint32_t)cy,
                                     &wx, &wy);
            offset_out[0] = point[0] - wx;
            offset_out[1] = point[1] - wy;
            inverse_scale = 1.0f / rec_float(map, 0x10);
            offset_out[0] = inverse_scale * offset_out[0] - -0.5f;
            offset_out[1] = inverse_scale * offset_out[1] - -0.5f;
        }
    }
#endif
    return 0u;
}

/* TerrainSurfaceHeightInterpolate (FUN_0044f940): choose the cell under the
 * world point, use its live four-vertex CPU quad, and interpolate in the
 * target's two triangles (0,1,3) and (1,2,3). TerrainCreate's two stored
 * triangle records use this 1-to-3 diagonal; the cell orientation bit rotates
 * the four vertices before interpolation. Heights are already in world units
 * after TerrainGridVertexBuild, so the target returns the interpolated value
 * without another scale. */
// FUNCTION: LEGORR 0x0044f940
float terrain_surface_height_interpolate(const void* handle, float world_x,
                                         float world_z)
{
    static const uint8_t flipped_order[4] = { 1u, 2u, 3u, 0u };
    const TerrainState* terrain = (const TerrainState*)handle;
    int cell_x, cell_y;
    size_t cell_index;
    uint8_t* record;
    float vertices[4][3];
    float height;
    uint32_t corner;
    int flipped;
    if (terrain == NULL || terrain->blocks == NULL ||
        terrain->cell_vertices == NULL ||
        !terrain_world_xy_to_cell(handle, world_x, world_z,
                                  &cell_x, &cell_y) ||
        cell_x < 0 || cell_y < 0 || cell_x >= terrain->width - 1 ||
        cell_y >= terrain->height - 1) return 0.0f;
    cell_index = (size_t)cell_y * (size_t)(terrain->width - 1) +
                 (size_t)cell_x;
    record = terrain->blocks +
        ((size_t)cell_y * (size_t)terrain->width + (size_t)cell_x) *
            TERRAIN_BLOCK_STRIDE;
    flipped = (record[0x19] & 1u) != 0u;
    for (corner = 0u; corner < 4u; ++corner) {
        const uint32_t source = flipped
            ? flipped_order[corner] : corner;
        memcpy(vertices[corner],
               terrain->cell_vertices + cell_index * 12u + source * 3u,
               3u * sizeof(float));
    }
    if (terrain_surface_triangle_height(vertices[0], vertices[1],
                                        vertices[3], world_x, world_z,
                                        &height) ||
        terrain_surface_triangle_height(vertices[1], vertices[2],
                                        vertices[3], world_x, world_z,
                                        &height)) return height;
    return 0.0f;
}

/* TerrainCellSurfaceTriangleVerticesSelect (FUN_0044fab0). The renderer
 * grid's point coordinates are world-space; use its owning terrain transform
 * to select the cell and its configured block size for the edge test. */
// FUNCTION: LEGORR 0x0044FAB0
void terrain_cell_surface_triangle_vertices_select(
    TerrainSurfaceGrid* grid, float x, float z, uint32_t out_vertices[3])
{
    int mapped_x, mapped_z;
    uint32_t cell_x;
    uint32_t cell_z;
    uint32_t index;
    uint32_t corner_offset;
    const uint32_t* triangle;
    float vertices[12];
    const uint8_t* cell_records;
    uint32_t record_width;
    size_t record_index;
    int flipped;
    uint32_t i;
    float cell_size;
    if (grid == NULL || out_vertices == NULL || grid->owner_level == NULL ||
        grid->owner_level->ptr_terrain == NULL)
        return;
    if (!terrain_world_xy_to_cell(grid->owner_level->ptr_terrain, x, z,
                                  &mapped_x, &mapped_z)) return;
    if (mapped_x < 0 || mapped_z < 0) return;
    cell_x = (uint32_t)mapped_x;
    cell_z = (uint32_t)mapped_z;
    if (!terrain_grid_cell_valid(grid, cell_x, cell_z) ||
        grid->surface_vectors == NULL)
        return;
    cell_size = terrain_blocksize_get(grid->owner_level->ptr_terrain);
    if (cell_size == 0.0f) return;
    index = terrain_grid_cell_index(grid, cell_x, cell_z);
    flipped = terrain_cell_surface_vertices_get(grid, cell_x, cell_z,
                                                vertices);
    if (!flipped) {
        if ((vertices[1] - z) + (x - vertices[0]) < cell_size)
            corner_offset = 0u;
        else
            corner_offset = 3u;
    }
    else if (cell_size <= (vertices[4] - z) + (vertices[3] - x)) {
        corner_offset = 0u;
    }
    else {
        corner_offset = 3u;
    }
    cell_records = grid->cell_records;
    record_width = grid->record_width != 0u
        ? grid->record_width : grid->width;
    if (cell_records == NULL && grid->terrain_window != NULL) {
        uint32_t window_width, window_height, window_stride;
        memcpy(&window_width, grid->terrain_window, sizeof(window_width));
        memcpy(&window_height,
               (const uint8_t*)grid->terrain_window + 4u,
               sizeof(window_height));
        memcpy(&window_stride,
               (const uint8_t*)grid->terrain_window + 8u,
               sizeof(window_stride));
        memcpy(&cell_records,
               (const uint8_t*)grid->terrain_window + 0x28u,
               sizeof(cell_records));
        if (cell_records == NULL || cell_x >= window_width ||
            cell_z >= window_height) return;
        record_width = window_stride;
    }
    if (cell_records == NULL || record_width == 0u || cell_x >= record_width)
        return;
    record_index = (size_t)cell_z * record_width + cell_x;
    triangle = (const uint32_t*)(cell_records + record_index * 0x28u +
                                  (corner_offset == 0u ? 0u : 0x0cu));
    for (i = 0u; i < 3u; ++i) out_vertices[i] = triangle[i];
}

/* TerrainSurfacePointProjectOffset (FUN_0044fb90): choose the cell edge
 * intersected by the supplied horizontal normal, intersect that edge with
 * the point's normal line, then apply the requested block-scaled offset.
 * The reference receives its TerrainState and four-corner data from the
 * active renderer; this path uses TerrainState's retained CPU quad and the
 * two triangle normals stored in its 0x28-byte cell record. */
// FUNCTION: LEGORR 0x0044fb90
int terrain_state_surface_point_project_offset(
    void* terrain_handle, float world_x, float world_z, int orientation,
    float offset, float* out_x, float* out_z)
{
    static const uint8_t flipped_order[4] = { 1u, 2u, 3u, 0u };
    TerrainState* terrain = (TerrainState*)terrain_handle;
    int cell_x, cell_y;
    size_t quad_index, record_index;
    uint8_t* record;
    float vertices[4][3];
    float normal_x, normal_z;
    float edge_point[2], edge_direction[2];
    float point[2], normal_direction[2], intersection[2];
    float length, scaled_offset;
    uint32_t corner;
    int flipped, first_corner, second_corner;
    if (terrain == NULL || terrain->blocks == NULL ||
        terrain->cell_vertices == NULL || out_x == NULL || out_z == NULL ||
        !terrain_world_xy_to_cell(terrain, world_x, world_z,
                                  &cell_x, &cell_y) ||
        cell_x < 0 || cell_y < 0 || cell_x >= terrain->width - 1 ||
        cell_y >= terrain->height - 1) return 0;

    quad_index = (size_t)(uint32_t)cell_y * terrain->dim[0] +
                 (uint32_t)cell_x;
    record_index = (size_t)(uint32_t)cell_y * terrain->dim[2] +
                   (uint32_t)cell_x;
    record = terrain->blocks + record_index * TERRAIN_BLOCK_STRIDE;
    flipped = (record[0x19] & 1u) != 0u;
    for (corner = 0u; corner < 4u; ++corner) {
        const uint32_t source = flipped ? flipped_order[corner] : corner;
        memcpy(vertices[corner], terrain->cell_vertices +
               quad_index * 12u + source * 3u, 3u * sizeof(float));
    }

    /* FUN_0044fb90 selects the stored triangle normal using the same
     * Manhattan-distance tests against the first two sampled corners. */
    if (!flipped) {
        if ((world_x - vertices[0][0]) +
            (vertices[1][2] - world_z) < terrain->blocksize)
            record += 0u;
        else
            record += 0x0cu;
    }
    else {
        if ((vertices[1][0] - world_x) +
            (vertices[1][2] - world_z) < terrain->blocksize)
            record += 0x0cu;
        else
            record += 0u;
    }
    normal_x = *(float*)record;
    normal_z = *(float*)(record + 4u);

    if (orientation == 0) {
        if ((float)fabs((double)normal_x) <=
            (float)fabs((double)normal_z)) {
            if (normal_z >= 0.0f) {
                first_corner = 0;
                second_corner = 1;
            }
            else {
                first_corner = 2;
                second_corner = 3;
            }
        }
        else if (normal_x >= 0.0f) {
            first_corner = 1;
            second_corner = 2;
        }
        else {
            first_corner = 0;
            second_corner = 3;
        }
    }
    else if (flipped) {
        first_corner = 1;
        second_corner = 3;
    }
    else {
        first_corner = 0;
        second_corner = 2;
    }

    edge_point[0] = vertices[first_corner][0];
    edge_point[1] = vertices[first_corner][2];
    edge_direction[0] = vertices[second_corner][0] - edge_point[0];
    edge_direction[1] = vertices[second_corner][2] - edge_point[1];
    point[0] = world_x;
    point[1] = world_z;
    normal_direction[0] = normal_x;
    normal_direction[1] = normal_z;
    if (line_intersection_2d_compute(intersection, edge_point,
            edge_direction, point, normal_direction) == NULL) return 0;

    length = (float)sqrt((double)(normal_x * normal_x +
                                  normal_z * normal_z));
    if (length == 0.0f) return 0;
    scaled_offset = offset * terrain->blocksize;
    *out_x = intersection[0] - (normal_x / length) * scaled_offset;
    *out_z = intersection[1] - (normal_z / length) * scaled_offset;
    return 1;
}

/* 0x44fe70 samples the authored SurfaceMap plane, independently of the
 * live/mutated quad used by 0x44f940. Byte decoding and RoughLevel scaling
 * stay inside the terrain owner; the camera receives a world-space height. */
// FUNCTION: LEGORR 0x0044fe70
float terrain_authored_surface_height_interpolate(const void* handle,
                                                  float world_x,
                                                  float world_y)
{
    const TerrainState* terrain = (const TerrainState*)handle;
    static const int corner_x[4] = { 0, 1, 1, 0 };
    static const int corner_y[4] = { 0, 0, 1, 1 };
    float height[4];
    float fx, fy;
    int x, y, corner;
    if (terrain == NULL || terrain->blocks == NULL ||
        terrain->blocksize == 0.0f ||
        !terrain_world_xy_to_cell(handle, world_x, world_y, &x, &y))
        return 0.0f;
    fx = (world_x - terrain->world_off_x) / terrain->blocksize - (float)x;
    fy = (terrain->world_off_y - world_y) / terrain->blocksize - (float)y;
    for (corner = 0; corner < 4; ++corner) {
        size_t index = ((size_t)(y + corner_y[corner]) *
            (size_t)terrain->width + (size_t)(x + corner_x[corner])) *
            TERRAIN_BLOCK_STRIDE;
        height[corner] = -(float)terrain->blocks[index + 0x1a] *
            terrain->terrain_value;
    }
    /* The reference uses the fixed 00-to-11 diagonal and does not consult
     * the live mesh's orientation flag, vertex offsets or mutation state. */
    if (fy >= fx)
        return height[0] + (height[2] - height[3]) * fx +
            (height[3] - height[0]) * fy;
    return height[0] + (height[1] - height[0]) * fx +
        (height[2] - height[1]) * fy;
}

// FUNCTION: LEGORR 0x00450060
int grid_cell_read_surface_vector(TerrainSurfaceGrid* grid, uint32_t x,
                                  uint32_t y, float out[3])
{
    uint32_t index;
    uint32_t corner;
    float vertices[4][3];
    if (!terrain_grid_cell_valid(grid, x, y) || out == NULL) return 0;
    index = terrain_grid_cell_index(grid, x, y);
    /* Retail selects the geometry vector using the live cell-record bit, not
     * the optional triangulation cache. */
    corner = terrain_surface_grid_cell_orientation_get(grid, x, y, index)
        ? 1u : 0u;
    if (grid->surface_vectors == NULL) {
        if (grid->owner_level == NULL ||
            !scene_render_terrain_cell_vertices_get(grid->owner_level, x, y,
                                                     vertices)) return 0;
        out[0] = vertices[corner][0];
        out[1] = vertices[corner][2];
        out[2] = vertices[corner][1];
        return 1;
    }
    memcpy(out, grid->surface_vectors + (index * 4 + corner) * 3,
           sizeof(float) * 3);
    return 1;
}
// FUNCTION: LEGORR 0x004500d0
int terrain_cell_surface_vertices_get(TerrainSurfaceGrid* grid, uint32_t x,
                                      uint32_t y, float out[12])
{
    uint32_t index;
    int flip;
    static const uint8_t flipped_order[4] = { 1, 2, 3, 0 };
    uint32_t i;
    if (grid == NULL || out == NULL) return 0;
    if (x >= grid->width || y >= grid->height) return (int)grid->width;
    index = terrain_grid_cell_index(grid, x, y);
    flip = terrain_surface_grid_cell_orientation_get(grid, x, y, index);
    if (grid->surface_vectors == NULL) {
        float vertices[4][3];
        uint32_t corner;
        if (grid->owner_level == NULL ||
            !scene_render_terrain_cell_vertices_get(grid->owner_level, x, y,
                                                     vertices)) return 0;
        /* SceneRender stores conventional (X,height,Z) triples; the retail
         * indexed surface read returns (X,Z,height). Its corner-height
         * cache already reflects the cell orientation, so preserve that
         * order instead of rotating it a second time. */
        for (corner = 0u; corner < 4u; ++corner) {
            out[corner * 3u] = vertices[corner][0];
            out[corner * 3u + 1u] = vertices[corner][2];
            out[corner * 3u + 2u] = vertices[corner][1];
        }
        return flip;
    }
    for (i = 0; i < 4; ++i) {
        uint32_t src_corner = flip ? flipped_order[i] : i;
        memcpy(out + i * 3, grid->surface_vectors + (index * 4 + src_corner) * 3,
               sizeof(float) * 3);
    }
    return flip;
}

// FUNCTION: LEGORR 0x00450220
void grid_cell_read_surface_values(void* terrain, int x, int y,
                                   float out_values[12])
{
    TerrainState* state = (TerrainState*)terrain;
    float indexed_output[4][9];
    TerrainIndexedMeshLayerObject object;
    TerrainIndexedMeshLayerOwner owner;
    size_t quad_index;
    uint32_t corner;
    uint32_t component;
    if (state == NULL || out_values == NULL || x < 0 || y < 0 ||
        (uint32_t)x >= state->dim[0] ||
        (uint32_t)y >= state->dim[1]) return;
    quad_index = (size_t)(uint32_t)y * state->dim[0] + (uint32_t)x;
    if (state->indexed_mesh_blocks == NULL ||
        state->indexed_mesh_vertices == NULL) return;
    terrain_indexed_mesh_cell_sync(state, quad_index);
    memset(indexed_output, 0, sizeof(indexed_output));
    terrain_indexed_mesh_layer_object_init(state, &object, &owner);
    (void)lwo_layer_polygon_vertex_data_read(&object, (int)quad_index, 0,
                                            4, indexed_output);
    for (corner = 0u; corner < 4u; ++corner)
        for (component = 0u; component < 3u; ++component)
            out_values[corner * 3u + component] =
                indexed_output[corner][component];
}

/* FUN_004502c0: unsigned pair membership (negative coords wrap). */
// FUNCTION: LEGORR 0x004502c0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) int level_coord_pair_within_bounds_target(
    const uint32_t* dimensions, uint32_t cx, uint32_t cy)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov ecx, dword ptr [esp + 8]
        cmp ecx, dword ptr [eax]
        jb coordinate_x_in_bounds
        xor eax, eax
        ret
    coordinate_x_in_bounds:
        mov edx, dword ptr [esp + 0ch]
        mov ecx, dword ptr [eax + 4]
        cmp edx, ecx
        sbb eax, eax
        neg eax
        ret
    }
}
#else
int level_coord_pair_within_bounds_target(const uint32_t* dimensions,
                                                uint32_t cx, uint32_t cy)
{
    if (cx < dimensions[0]) {
        return cy < dimensions[1];
    }
    return 0;
}
#endif

/* The seven argument slots match FUN_004502e0. The grid and ray-hook pointees
 * are portable owners rather than the retail terrain/view records, so this is
 * an ABI-shaped implementation over semantic adapters, not a raw-record
 * replacement. */
// FUNCTION: LEGORR 0x004502e0
int screen_ray_terrain_cell_pick_target(
    TerrainSurfaceGrid* grid, const CameraPointerRayHooks* ray_hooks,
    float screen_x, float screen_y, int* out_cell_x, int* out_cell_y,
    float out_hit_xyz[3])
{
    float near_point[3];
    float far_point[3];
    float direction[3];
    float plane_point[3] = { 0.0f, 0.0f, -50.0f };
    const float plane_normal[3] = { 0.0f, 0.0f, -1.0f };
    float intersection[3];
    float terrain_height;
    int center_x, center_y;
    uint32_t iteration;
    uint32_t x, y;
    float best_distance = 0.0f;
    int found = 0;
    /* FUN_004502e0 iterates twenty times in FUN_00450560, starting from
     * z=-50, then checks the estimated cell and its two-cell border.
     * The portable view interface exposes the ray and plane intersection but
     * not the reference's world-to-screen vertex transform, so choose the
     * nearest exact terrain-ray hit within that same local neighborhood. */
    if (grid == NULL || ray_hooks == NULL ||
        ray_hooks->screen_point_to_ray == NULL || out_cell_x == NULL ||
        out_cell_y == NULL || out_hit_xyz == NULL ||
        grid->owner_level == NULL || grid->owner_level->ptr_terrain == NULL ||
        grid->width == 0u || grid->height == 0u ||
        !ray_hooks->screen_point_to_ray(ray_hooks->context, screen_x,
                                        screen_y, 0.0f, near_point) ||
        !ray_hooks->screen_point_to_ray(ray_hooks->context, screen_x,
                                        screen_y, 1.0f, far_point)) return 0;
    direction[0] = far_point[0] - near_point[0];
    direction[1] = far_point[1] - near_point[1];
    direction[2] = far_point[2] - near_point[2];
    {
        const float length = (float)sqrt(direction[0] * direction[0] +
            direction[1] * direction[1] + direction[2] * direction[2]);
        if (length == 0.0f) return 0;
        direction[0] /= length;
        direction[1] /= length;
        direction[2] /= length;
    }
    terrain_height = plane_point[2];
    for (iteration = 0u; iteration < 20u; ++iteration) {
        if (ray_hooks->plane_intersect != NULL) {
            plane_point[2] = terrain_height;
            if (!ray_hooks->plane_intersect(ray_hooks->context, near_point,
                    direction, plane_point, plane_normal, intersection))
                return 0;
        }
        else {
            float distance;
            if (direction[2] == 0.0f) return 0;
            distance = (terrain_height - near_point[2]) / direction[2];
            if (distance < 0.0f) return 0;
            intersection[0] = near_point[0] + direction[0] * distance;
            intersection[1] = near_point[1] + direction[1] * distance;
            intersection[2] = near_point[2] + direction[2] * distance;
        }
        terrain_height = terrain_surface_height_interpolate(
            grid->owner_level->ptr_terrain,
            intersection[0], intersection[1]);
    }
    if (!terrain_world_xy_to_cell(grid->owner_level->ptr_terrain,
            intersection[0], intersection[1], &center_x, &center_y)) return 0;
    for (y = 0u; y < 5u; ++y) {
        int candidate_y = center_y + (int)y - 2;
        if (candidate_y < 0 || (uint32_t)candidate_y >= grid->height) continue;
        for (x = 0u; x < 5u; ++x) {
            int candidate_x = center_x + (int)x - 2;
            float candidate[3];
            float dx, dy, dz, distance;
            if (candidate_x < 0 || (uint32_t)candidate_x >= grid->width)
                continue;
            if (!terrain_cell_ray_hit_test(grid, candidate_x, candidate_y,
                                           near_point, direction, candidate))
                continue;
            dx = candidate[0] - near_point[0];
            dy = candidate[1] - near_point[1];
            dz = candidate[2] - near_point[2];
            distance = dx * direction[0] + dy * direction[1] +
                       dz * direction[2];
            if (distance < 0.0f || (found && distance >= best_distance))
                continue;
            best_distance = distance;
            *out_cell_x = candidate_x;
            *out_cell_y = candidate_y;
            out_hit_xyz[0] = candidate[0];
            out_hit_xyz[1] = candidate[1];
            out_hit_xyz[2] = candidate[2];
            found = 1;
        }
    }
    return found;
}

/* RandomReachableGridPointSelect (FUN_00450560): the ray-descending
 * grid-point pick — see view.hpp for the plane geometry. Both float*
 * direction arguments ride the corpus's forwarded-dword convention:
 * the point and ray direction are vectors, and Ghidra typed them as
 * plain dwords because FUN_004798e0 only forwards them. The plane-point
 * z quirk is faithful: FUN_004798e0 answers into `out` while the height
 * store lands on the plane quad's z-slot, so the corpus effectively
 * intersects against successively lower planes. */
// FUNCTION: LEGORR 0x00450560
void random_reachable_grid_point_select(EngineContext* ctx, const float* p,
                                        const float* dir, float* out,
                                        int* gx, int* gy, int rounds)
{
    float plane[3] = { 0.0f, 0.0f, -50.0f };
    const float normal[3] = { 0.0f, 0.0f, -1.0f };
    for (int i = rounds; i != 0; --i) {
        view_ray_intersection_test(out, p, dir, plane, normal);
        plane[2] = terrain_height(ctx, out[0], out[1]);
    }
    (void)engine_context_grid_pick(ctx, out, gx, gy);
}

/* FUN_00450600: clear and queue every in-bounds cell in the inclusive circle.
 * The callback runs after the surface value is cleared but before the
 * coordinate is appended, matching the reference's observable order. */
// FUNCTION: LEGORR 0x00450600
void terrain_surface_clear_circle(TerrainSurfaceGrid* grid, int center_x,
                                  int center_y, int radius,
                                  TerrainGridCircleCellCallback callback)
{
    if (grid == NULL || grid->surface_values == NULL || radius < 0 ||
        radius > 32767 || center_x < INT_MIN + radius ||
        center_x > INT_MAX - radius || center_y < INT_MIN + radius ||
        center_y > INT_MAX - radius)
        return;
    {
        int y_offset;
        const int radius_squared = radius * radius;
        for (y_offset = -radius; y_offset <= radius; ++y_offset) {
            int x_offset;
            for (x_offset = -radius; x_offset <= radius; ++x_offset) {
                const int x = center_x + x_offset;
                const int y = center_y + y_offset;
                if (x_offset * x_offset + y_offset * y_offset >
                    radius_squared || x < 0 || y < 0 ||
                    (uint32_t)x >= grid->width ||
                    (uint32_t)y >= grid->height) continue;

                grid_surface_write_at_xy(grid, x, y, 0);
                if (callback != NULL) callback(x, y);
                if (grid->changed_count < TERRAIN_GRID_CHANGE_CAPACITY) {
                    grid->changed_xy[grid->changed_count][0] =
                        (int16_t)(x_offset + center_x);
                    grid->changed_xy[grid->changed_count][1] =
                        (int16_t)(y_offset + center_y);
                    ++grid->changed_count;
                }
            }
        }
    }
}
// FUNCTION: LEGORR 0x00450700
void grid_surface_write_at_xy(TerrainSurfaceGrid* grid, int x, int y,
                              uint8_t value)
{
    uint32_t index;
    if (grid == NULL || grid->surface_values == NULL || x < 0 || y < 0 ||
        (uint32_t)x >= grid->width || (uint32_t)y >= grid->height)
        return;
    index = (uint32_t)y * grid->width + (uint32_t)x;
    /* FUN_00450700 delegates this exact index to the surface-value setter. */
    grid->surface_values[index] = value;
    /* The retail setter also toggles the same indexed terrain mesh record
     * that TerrainCreate enabled for each quad. */
    if (grid->owner_level != NULL)
        terrain_detail_enable_set(grid->owner_level->ptr_terrain, index,
                                  value != 0u);
}

// FUNCTION: LEGORR 0x00450730
void terrain_cell_change_queue_append(TerrainSurfaceGrid* grid, int x, int y)
{
    if (grid == NULL) return;
    grid_surface_write_at_xy(grid, x, y, 0);
    /* Retail writes the surface value before appending the 16-bit pair. */
    if (grid->changed_count >= TERRAIN_GRID_CHANGE_CAPACITY) return;
    grid->changed_xy[grid->changed_count][0] = (int16_t)x;
    grid->changed_xy[grid->changed_count][1] = (int16_t)y;
    ++grid->changed_count;
}

// FUNCTION: LEGORR 0x00450780
void terrain_cell_change_queue_flush(TerrainSurfaceGrid* grid)
{
    uint32_t i;
    if (grid == NULL) return;
    for (i = 0; i < grid->changed_count; ++i) {
        grid_surface_write_at_xy(grid, grid->changed_xy[i][0],
                                 grid->changed_xy[i][1], 1);
    }
    grid->changed_count = 0;
}
// FUNCTION: LEGORR 0x004507d0
int terrain_cell_edge_vector_query(TerrainSurfaceGrid* grid, uint32_t x,
                                   uint32_t y, float out[3])
{
    uint32_t cell_x = x;
    uint32_t cell_y = y;
    uint32_t corner = 0;
    uint32_t index;
    if (grid == NULL || out == NULL || grid->width == 0 || grid->height == 0 ||
        x > grid->width || y > grid->height) return 0;
    if (x == grid->width) {
        --cell_x;
        corner = 1;
    }
    if (y == grid->height) {
        --cell_y;
        corner = (x == grid->width) ? 2u : 3u;
    }
    if (!terrain_grid_cell_valid(grid, cell_x, cell_y) ||
        grid->surface_vectors == NULL) return 0;
    index = terrain_grid_cell_index(grid, cell_x, cell_y);
    if (terrain_surface_grid_cell_orientation_get(grid, cell_x, cell_y,
                                                  index)) {
        corner = (corner + 1u) & 3u;
    }
    memcpy(out, grid->surface_vectors + (index * 4 + corner) * 3,
           sizeof(float) * 3);
    return 1;
}

/* The terrain context's block-size float is the dword at +0x10. */
// FUNCTION: LEGORR 0x00450890
float terrain_context_scale_get(const void* context)
{
    return *(const float*)((const uint8_t*)context + 0x10);
}

/* CellNeighborDirectionCode (FUN_004508a0): eight neighbors, -1 else. */
// FUNCTION: LEGORR 0x004508a0
int cell_neighbor_direction_code(int x1, int y1, int x2, int y2)
{
    int result = -1;
    if (x1 == x2) {
        if (y2 == y1 - 1) {
            return 0;
        }
        if (y2 == y1 + 1) {
            return 2;
        }
    }
    else {
        const int x1_next = x1 + 1;
        if (y1 == y2) {
            if (x1_next == x2) {
                return 1;
            }
            if (x1 - 1 == x2) {
                return 3;
            }
        }
        else {
            if (x1_next == x2) {
                if (y1 - 1 == y2) {
                    return 4;
                }
                if (x1_next == x2 && y1 + 1 == y2) {
                    return 5;
                }
            }
            if (x1 - 1 == x2) {
                if (y1 + 1 == y2) {
                    return 6;
                }
                if (x1 - 1 == x2 && y1 - 1 == y2) {
                    result = 7;
                }
            }
        }
    }
    return result;
}

/* --- terrain dirty queue (see level.hpp) ---------------------------------
 * The corpus's queue lives inside its terrain context (u16 pair entries,
 * stride 4, capacity (0x7198-0x4a88)/4 = 2500, count at +0x7198). */
// FUNCTION: LEGORR 0x00450960
void terrain_cell_dirty_queue_push_and_clear(void* terrain, int x, int y,
                                             int push)
{
    TerrainState* t = (TerrainState*)terrain;
    if (t == NULL || t->blocks == NULL) {
        return; /* deviation: corpus dereferences its context blindly */
    }
    if (x < 0 || y < 0 || x >= t->width || y >= t->height) {
        return; /* bounds deviation (as across this module) */
    }
    uint8_t* flag = t->blocks
                    + ((size_t)y * (size_t)t->width + (size_t)x)
                      * TERRAIN_BLOCK_STRIDE + 0x19;
    if (push == 0) {
        if ((*flag & 0x10) == 0) {
            return;
        }
        if (t->dirty_queue_count == 0u) {
            return; /* corpus's empty-queue path: flag stays set */
        }
        int i;
        for (i = 0; i < (int)t->dirty_queue_count; ++i) {
            if ((uint32_t)t->dirty_queue_x[i] == (uint32_t)x &&
                (uint32_t)t->dirty_queue_y[i] == (uint32_t)y) {
                break;
            }
        }
        if ((uint32_t)i >= t->dirty_queue_count) {
            return; /* corpus's not-found path: flag stays set */
        }
        /* overwrite the found entry with the last one, then drop it */
        t->dirty_queue_x[i] = t->dirty_queue_x[t->dirty_queue_count - 1u];
        t->dirty_queue_y[i] = t->dirty_queue_y[t->dirty_queue_count - 1u];
        --t->dirty_queue_count;
        *flag &= 0xef;
        return;
    }
    if ((*flag & 0x10) != 0) {
        return; /* already queued (faithful) */
    }
    /* The corpus has no capacity branch, but its queue owns exactly 2500
     * entries. Keep the flag clear if a corrupt/overfull state cannot retain
     * the coordinate; otherwise the cell would become permanently stuck. */
    if (t->dirty_queue_count >= LEVEL_DIRTY_QUEUE_CAP) return;
    t->dirty_queue_x[t->dirty_queue_count] = (uint16_t)x;
    t->dirty_queue_y[t->dirty_queue_count] = (uint16_t)y;
    ++t->dirty_queue_count;
    *flag |= 0x10;
}

// FUNCTION: LEGORR 0x00450a80
void cell_list_min_distance_scale_apply(int enable) /* FUN_00450a80 */
{
    GameContext* game = entity_pool_callback_context_get();
    LevelState* level = game != NULL ? game->level : NULL;
    TerrainState* terrain;
    if (level == NULL) level = level_current_owner_get();
    terrain = level != NULL ? (TerrainState*)level->ptr_terrain : NULL;
    uint32_t dirty_count = terrain != NULL
        ? terrain->dirty_queue_count : 0u;
    if (enable != 0 && (s_cell_list_flags & 4u) == 0) {
        for (uint32_t i = 0u; i < dirty_count; ++i) {
            /* The corpus strides the u16 pair entries unsigned: the
             * index folds the y-entry through the scale word. */
            const uint32_t x = (uint32_t)terrain->dirty_queue_x[i];
            const uint32_t y = (uint32_t)terrain->dirty_queue_y[i];
            scene_mesh_color_set(s_cell_list_object,
                                 (int)(y * (uint32_t)s_cell_list_scale + x),
                                 0.3f, 0.3f, 0.3f);
        }
        s_cell_list_flags |= 4u;
        return;
    }
    if (enable == 0 && (s_cell_list_flags & 4u) != 0) {
        for (uint32_t i = 0u; i < dirty_count; ++i) {
            const uint32_t x = (uint32_t)terrain->dirty_queue_x[i];
            const uint32_t y = (uint32_t)terrain->dirty_queue_y[i];
            scene_mesh_color_set(s_cell_list_object,
                                 (int)(y * (uint32_t)s_cell_list_scale + x),
                                 0.0f, 0.0f, 0.0f);
        }
        s_cell_list_flags &= ~4u;
    }
}

/* TerrainCellSpecialStatePredicate (FUN_0040d3f0) — see level.hpp. */
/* CellVariantEntryMatchCheck (FUN_00451180) — see level.hpp. The
 * coordinate pair names two cells through the same window: the block
 * table stride is the window's own dword at + 0x08. */
// FUNCTION: LEGORR 0x00451180
int cell_variant_entry_match_check(const void* grid, uint32_t x1, uint32_t y1,
                                   uint32_t x2, uint32_t y2)
{
    if (grid == NULL) {
        return 0; /* deviation: the corpus strides the window blindly */
    }
    const unsigned char* w = (const unsigned char*)grid;
    if ((y1 != y2 || x1 != x2) != 0) {
        uint32_t width, height, stride;
        __builtin_memcpy(&width, w + 0x00, sizeof width);
        __builtin_memcpy(&height, w + 0x04, sizeof height);
        __builtin_memcpy(&stride, w + 0x08, sizeof stride);
        if (x1 < width && y1 < height && x2 < width && y2 < height) {
            void* blocks = NULL;
            __builtin_memcpy(&blocks, w + 0x28, sizeof blocks);
            if (blocks == NULL) {
                return 0; /* deviation: the corpus strides the base */
            }
            const unsigned char* base = (const unsigned char*)blocks + 0x18;
            const size_t i1 = ((size_t)y1 * stride + x1) * 0x28;
            const size_t i2 = ((size_t)y2 * stride + x2) * 0x28;
            const uint32_t v1 = base[i1];
            const uint32_t v2 = base[i2];
            if (v1 != 0xffu && v2 != 0xffu) {
                const size_t cnt = w[0x4034u + v1];
                for (size_t i = 0; i < cnt; ++i) {
                    if (w[0x34u + (size_t)v1 * 0x80u + i] == (unsigned char)v2) {
                        return 1;
                    }
                }
                return 0; /* corpus: the list ran out (and count 0) */
            }
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00451230
void terrain_cell_surface_vector_write(TerrainSurfaceGrid* grid,
                                       uint32_t x, uint32_t y,
                                       uint32_t corner,
                                       const float vector[3])
{
    uint32_t index;
    if (!terrain_grid_cell_valid(grid, x, y) || grid->surface_vectors == NULL ||
        vector == NULL || corner > 3) return;
    index = terrain_grid_cell_index(grid, x, y);
    if (terrain_surface_grid_cell_orientation_get(grid, x, y, index))
        corner = (corner + 1u) & 3u;
    memcpy(grid->surface_vectors + (index * 4 + corner) * 3, vector,
           sizeof(float) * 3);
}

/* --- corner normal sampling (see level.hpp) ------------------------------- */
// FUNCTION: LEGORR 0x004512d0
#if defined(_MSC_VER) && defined(_M_IX86)
int terrain_cell_normal_sample(const void* terrain, uint32_t x, uint32_t y,
                               uint32_t corner, float out[3])
{
    const TerrainState* t = (const TerrainState*)terrain;
    const uint8_t* p;
    float* second;
    float length;
    float inverse;
    if (x < t->dim[0] && y < t->dim[1]) {
        p = t->blocks + (y * t->dim[2] + x) * TERRAIN_BLOCK_STRIDE;
        if ((p[0x19] & 1u) != 0u) corner = (corner + 1u) & 3u;
        if (corner == 0u) {
            out[0] = *(const float*)(p + 0x00);
            out[1] = *(const float*)(p + 0x04);
            out[2] = *(const float*)(p + 0x08);
        }
        if (corner == 2u) {
            second = (float*)(p + 0x0c);
            out[0] = second[0];
            out[1] = second[1];
            out[2] = second[2];
        }
        if (corner == 1u || corner == 3u) {
            second = (float*)(p + 0x0c);
            out[0] = second[0] + *(const float*)(p + 0x00);
            out[1] = second[1] + *(const float*)(p + 0x04);
            out[2] = second[2] + *(const float*)(p + 0x08);
            length = (float)sqrt(out[0] * out[0] + out[2] * out[2] +
                                 out[1] * out[1]);
            inverse = 1.0f / length;
            out[0] = inverse * out[0];
            out[1] = inverse * out[1];
            out[2] = inverse * out[2];
        }
        return 1;
    }
    out[2] = 0.0f;
    out[1] = 0.0f;
    out[0] = 0.0f;
    return 0;
}
#else
int terrain_cell_normal_sample(const void* terrain, uint32_t x, uint32_t y,
                               uint32_t corner, float out[3])
{
    const TerrainState* t = (const TerrainState*)terrain;
    uint32_t sel;
    const uint8_t* p;
    const float* fa;
    const float* fb;
    float v[3];
    if (out == NULL) return 0;
    if (t == NULL || t->blocks == NULL ||
        x >= t->dim[0] || y >= t->dim[1]) {
        for (int i = 0; i < 3; ++i) {
            out[i] = 0.0f;
        }
        return 0;
    }
    p = t->blocks + ((size_t)y * (size_t)t->dim[2] + (size_t)x) *
        TERRAIN_BLOCK_STRIDE;
    sel = corner;
    if ((p[0x19] & 1u) != 0u) {
        sel = (sel + 1u) & 3u;
    }
    fa = (const float*)(p + 0x00);
    fb = (const float*)(p + 0x0c);
    if (sel == 0u) {
        v[0] = fa[0]; v[1] = fa[1]; v[2] = fa[2];
    } else if (sel == 2u) {
        v[0] = fb[0]; v[1] = fb[1]; v[2] = fb[2];
    } else if (sel == 1u || sel == 3u) {
        v[0] = fb[0] + fa[0];
        v[1] = fb[1] + fa[1];
        v[2] = fb[2] + fa[2];
        /* FUN_004512d0's x87 expression evaluates x*x + z*z + y*y. */
        const float inv = 1.0f / (float)
            sqrt(v[0] * v[0] + v[2] * v[2] + v[1] * v[1]);
        v[0] *= inv;
        v[1] *= inv;
        v[2] *= inv;
    }
    else {
        /* The retail switch leaves the output untouched for other corner
         * selectors, while still returning success for the in-bounds cell. */
        return 1;
    }
    out[0] = v[0];
    out[1] = v[1];
    out[2] = v[2];
    return 1;
}
#endif

// FUNCTION: LEGORR 0x00451450
void terrain_cell_surface_vector_offset_add(TerrainSurfaceGrid* grid,
                                            uint32_t x, uint32_t y,
                                            uint32_t corner,
                                            const float offset[3])
{
    uint32_t index;
    float* vector;
    if (!terrain_grid_cell_valid(grid, x, y) || grid->surface_vectors == NULL ||
        offset == NULL || corner > 3) return;
    index = terrain_grid_cell_index(grid, x, y);
    if (terrain_surface_grid_cell_orientation_get(grid, x, y, index))
        corner = (corner + 1u) & 3u;
    vector = grid->surface_vectors + (index * 4 + corner) * 3;
    vector[0] += offset[0];
    vector[1] += offset[1];
    vector[2] += offset[2];
}
/* FUN_004514f0: refresh both stored triangle normals from the cell's four
 * surface vectors. */
// FUNCTION: LEGORR 0x004514f0
void terrain_cell_surface_vectors_rebuild(TerrainSurfaceGrid* grid,
                                          uint32_t x, uint32_t y)
{
    const float* vertices;
    float first[3];
    float second[3];
    uint32_t index;
    uint8_t* record;
    if (grid == NULL || grid->cell_records == NULL ||
        grid->surface_vectors == NULL || x >= grid->width ||
        y >= grid->height) return;
    index = terrain_grid_cell_index(grid, x, y);
    vertices = grid->surface_vectors + index * 12u;
    record = grid->cell_records +
        ((size_t)y * terrain_surface_record_width(grid) + x) * 0x28u;
    surface_triangle_normal_calculate(first, vertices, vertices + 3,
                                      vertices + 9);
    surface_triangle_normal_calculate(second, vertices + 3, vertices + 6,
                                      vertices + 9);
    memcpy(record, first, sizeof(first));
    memcpy(record + 0x0c, second, sizeof(second));
}

/* FUN_004515a0: header dims at +0x08/+0x0C (port guards a NULL image). */
// FUNCTION: LEGORR 0x004515a0
void level_map_image_header(const uint8_t* image, uint32_t* width,
                            uint32_t* height)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    *width = *(const uint32_t*)(image + 0x08);
    *height = *(const uint32_t*)(image + 0x0c);
#else
    *width = 0;
    *height = 0;
    if (image != NULL) {
        __builtin_memcpy(width, image + 0x08, sizeof(*width));
        __builtin_memcpy(height, image + 0x0C, sizeof(*height));
    }
#endif
}

/* FUN_004515c0: zero-extended u16 cell at +0x10 + (y*width+x)*2. */
// FUNCTION: LEGORR 0x004515c0
uint16_t level_map_image_cell(const uint8_t* image, int x, int width, int y)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    return *(const uint16_t*)(image + 0x10 + (y * width + x) * 2);
#else
    if (image == NULL) {
        return 0;
    }
    /* MapFileGetCell performs this multiply/add in 32-bit unsigned
     * arithmetic before doubling the cell index. Keep that behavior on
     * 64-bit hosts as well; the target accessor has no coordinate check. */
    const uint32_t index = (uint32_t)y * (uint32_t)width + (uint32_t)x;
    const uint32_t byte_offset = index * 2u;
    uint16_t cell;
    __builtin_memcpy(&cell, image + 0x10 + byte_offset, sizeof(cell));
    return cell;
#endif
}

// FUNCTION: LEGORR 0x004515E0
int terrain_cell_ray_hit_test(TerrainSurfaceGrid* grid, int x,
                              int y, const float origin[3],
                              const float direction[3], float hit[3])
{
    float vertices[12];
    float rendered_vertices[4][3];
    float first_hit[3], second_hit[3];
    int first, second;
    int i;
    if (origin == NULL || direction == NULL || hit == NULL || grid == NULL ||
        x < 0 || y < 0 ||
        !terrain_grid_cell_valid(grid, (uint32_t)x, (uint32_t)y))
        return 0;
    if (grid->surface_vectors != NULL) {
        (void)terrain_cell_surface_vertices_get(grid, (uint32_t)x,
                                                (uint32_t)y, vertices);
    }
    else {
        if (grid->owner_level == NULL ||
            !scene_render_terrain_cell_vertices_get(grid->owner_level,
                (uint32_t)x, (uint32_t)y, rendered_vertices)) return 0;
        memcpy(vertices, rendered_vertices, sizeof(rendered_vertices));
    }
    first = terrain_ray_triangle_hit(origin, direction, vertices,
                                     vertices + 3, vertices + 6, first_hit);
    second = terrain_ray_triangle_hit(origin, direction, vertices + 6,
                                      vertices + 9, vertices, second_hit);
    if (!first && !second) return 0;
    if (first && second) {
        float dx0 = first_hit[0] - origin[0];
        float dy0 = first_hit[1] - origin[1];
        float dz0 = first_hit[2] - origin[2];
        float dx1 = second_hit[0] - origin[0];
        float dy1 = second_hit[1] - origin[1];
        float dz1 = second_hit[2] - origin[2];
        if (dx1*dx1 + dy1*dy1 + dz1*dz1 <
            dx0*dx0 + dy0*dy0 + dz0*dz0) {
            for (i = 0; i < 3; ++i) hit[i] = second_hit[i];
            return 1;
        }
    }
    for (i = 0; i < 3; ++i) hit[i] = first ? first_hit[i] : second_hit[i];
    return 1;
}

/* PrimitiveModelRecordCreate (FUN_00451ab0): six-word record with a
 * heap-owned name; its object slot is empty, the caller kind is copied,
 * flags start at 2, and the list link starts clear. */
// FUNCTION: LEGORR 0x00451ab0
ModelEntryRecord* model_primitive_record_create(ModelEntryRecord* previous,
                                                const char* name, int kind)
{
    ModelEntryRecord* record = (ModelEntryRecord*)engine_heap_alloc(
        (long)sizeof(ModelEntryRecord));
    if (record == NULL || name == NULL) {
        engine_heap_free(record);
        return NULL;
    }
    record->object = NULL;
    record->instance = NULL;
    const size_t length = strlen(name) + 1;
    record->name = (char*)engine_heap_alloc((long)length);
    if (record->name == NULL) {
        engine_heap_free(record);
        return NULL;
    }
    memcpy(record->name, name, length);
    record->kind = kind;
    record->flags = 2;
    record->next = NULL;
    if (previous != NULL) {
        previous->next = record;
    }
    return record;
}

/* LinkedRecordListFlatten (FUN_00451b30): copy six fields from each
 * linked record into a contiguous engine-heap array, reset instance, mark
 * every entry active, mark the first entry as the list head, and terminate
 * the final next link. */
// FUNCTION: LEGORR 0x00451b30
ModelEntryRecord* model_entry_list_flatten(const ModelEntryRecord* head)
{
    size_t count = 0;
    const ModelEntryRecord* node;
    for (node = head; node != NULL; node = node->next) {
        ++count;
    }
    if (count == 0) {
        return NULL;
    }
    if (count > (size_t)LONG_MAX / sizeof(ModelEntryRecord)) {
        return NULL;
    }
    ModelEntryRecord* flat = (ModelEntryRecord*)engine_heap_alloc(
        (long)(count * sizeof(ModelEntryRecord)));
    if (flat == NULL) {
        return NULL;
    }
    node = head;
    for (size_t i = 0; i < count; ++i) {
        flat[i] = *node;
        flat[i].instance = NULL;
        flat[i].flags = (flat[i].flags & ~2u) | 1u;
        flat[i].next = (i + 1 < count) ? &flat[i + 1] : NULL;
        node = node->next;
    }
    flat[0].flags |= 2u;
    return flat;
}

// FUNCTION: LEGORR 0x00451c30
void linked_record_field4_clear(void* record)
{
    if (record == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    char* node = (char*)record;
    while (node != NULL) {
        const uint32_t zero = 0u;
        __builtin_memcpy(node + 4, &zero, sizeof(zero));
        void* next = NULL;
        __builtin_memcpy(&next, node + 0x14, sizeof(next));
        node = (char*)next;
    }
}

static int terrain_grid_cell_valid(const TerrainSurfaceGrid* grid,
                                   uint32_t x, uint32_t y)
{
    return grid != NULL && x < grid->width && y < grid->height;
}

static uint32_t terrain_grid_cell_index(const TerrainSurfaceGrid* grid,
                                        uint32_t x, uint32_t y)
{
    return y * grid->width + x;
}
static uint32_t terrain_surface_record_width(const TerrainSurfaceGrid* grid)
{
    return grid->record_width != 0u ? grid->record_width : grid->width;
}

/* The surface dimensions describe quads; the record window may include the
 * extra row and column at their far edges. */
static int terrain_surface_record_bounds_get(const TerrainSurfaceGrid* grid,
                                             uint32_t* width,
                                             uint32_t* height)
{
    if (grid == NULL || width == NULL || height == NULL) return 0;
    *width = terrain_surface_record_width(grid);
    *height = grid->height;
    if (grid->terrain_window != NULL) {
        uint32_t window_height;
        memcpy(&window_height, (const uint8_t*)grid->terrain_window + 4,
               sizeof(window_height));
        if (window_height != 0u) *height = window_height;
    }
    return *width != 0u && *height != 0u;
}

static int terrain_surface_grid_cell_orientation_get(
    const TerrainSurfaceGrid* grid, uint32_t x, uint32_t y, uint32_t index)
{
    if (grid->cell_records != NULL) {
        const uint32_t stride = terrain_surface_record_width(grid);
        if (x < stride && y < grid->height)
            return (grid->cell_records[((size_t)y * stride + x) * 0x28u +
                                       0x19u] & 1u) != 0u;
    }
    if (grid->terrain_window != NULL) {
        uint32_t width, height, stride;
        const uint8_t* blocks;
        memcpy(&width, grid->terrain_window, sizeof(width));
        memcpy(&height, (const uint8_t*)grid->terrain_window + 4,
               sizeof(height));
        memcpy(&stride, (const uint8_t*)grid->terrain_window + 8,
               sizeof(stride));
        memcpy(&blocks, (const uint8_t*)grid->terrain_window + 0x28,
               sizeof(blocks));
        if (blocks != NULL && x < width && y < height)
            return (blocks[((size_t)y * stride + x) * 0x28u + 0x19u] & 1u)
                != 0u;
    }
    return grid->triangulation_flags != NULL &&
        (grid->triangulation_flags[index] & 1u) != 0u;
}
static int terrain_ray_triangle_hit(const float origin[3],
                                    const float direction[3],
                                    const float a[3], const float b[3],
                                    const float c[3], float hit[3])
{
    float normal[3];
    float point[3];
    float v0[3], v1[3], v2[3];
    float d00, d01, d11, d20, d21, denominator, u, v;
    int i;
    surface_triangle_normal_calculate(normal, a, b, c);
    if (!view_ray_intersection_test(point, origin, direction, a, normal))
        return 0;
    for (i = 0; i < 3; ++i) {
        v0[i] = c[i] - a[i];
        v1[i] = b[i] - a[i];
        v2[i] = point[i] - a[i];
    }
    d00 = v0[0]*v0[0] + v0[1]*v0[1] + v0[2]*v0[2];
    d01 = v0[0]*v1[0] + v0[1]*v1[1] + v0[2]*v1[2];
    d11 = v1[0]*v1[0] + v1[1]*v1[1] + v1[2]*v1[2];
    d20 = v2[0]*v0[0] + v2[1]*v0[1] + v2[2]*v0[2];
    d21 = v2[0]*v1[0] + v2[1]*v1[1] + v2[2]*v1[2];
    denominator = d00 * d11 - d01 * d01;
    if (denominator == 0.0f) return 0;
    u = (d11 * d20 - d01 * d21) / denominator;
    v = (d00 * d21 - d01 * d20) / denominator;
    if (u < 0.0f || v < 0.0f || u + v > 1.0f) return 0;
    for (i = 0; i < 3; ++i) hit[i] = point[i];
    return 1;
}

static void terrain_indexed_mesh_layer_object_init(
    TerrainState* terrain, TerrainIndexedMeshLayerObject* object,
    TerrainIndexedMeshLayerOwner* owner)
{
    memset(object, 0, sizeof(*object));
    memset(owner, 0, sizeof(*owner));
    owner->mesh = &terrain->indexed_mesh;
    object->owner = owner;
}

static int terrain_derived_quad_create(void* context, uint32_t index,
                                       const uint32_t triangles[6])
{
    TerrainState* terrain = (TerrainState*)context;
    (void)triangles;
    if (terrain == NULL || terrain->owner_level == NULL) return -1;
    if (terrain->derived_vertices == NULL || index >=
        terrain->derived_mesh.width * terrain->derived_mesh.height ||
        index > 0x7fffffffu) return -1;
    return (int)index; /* software mesh slot, not a native DDi handle */
}

static int terrain_derived_quad_vertices_emit(void* context, int quad,
                                               const float vertices[4][3])
{
    TerrainState* terrain = (TerrainState*)context;
    if (terrain == NULL || terrain->derived_vertices == NULL || quad < 0 ||
        (uint32_t)quad >= terrain->derived_mesh.width *
                           terrain->derived_mesh.height) return 0;
    memcpy(terrain->derived_vertices + (size_t)quad * 12u,
           vertices, 12u * sizeof(float));
    return 1;
}

static int terrain_indexed_mesh_create(TerrainState* terrain,
                                       size_t quad_count)
{
    size_t index;
    if (terrain == NULL || quad_count == 0u ||
        quad_count > ((size_t)-1) / 0x70u ||
        quad_count > ((size_t)-1) / (32u * sizeof(float)) ||
        quad_count > 0xffffffffu) return 0;
    terrain->indexed_mesh_blocks = (uint8_t*)calloc(quad_count, 0x70u);
    terrain->indexed_mesh_vertices = (float*)calloc(
        quad_count * 32u, sizeof(float));
    terrain->derived_mesh.records = (uint8_t*)calloc(
        (size_t)terrain->dim[2] * terrain->dim[3], 8u);
    terrain->derived_vertices = (float*)calloc(
        (size_t)terrain->dim[2] * terrain->dim[3] * 12u, sizeof(float));
    if (terrain->indexed_mesh_blocks == NULL ||
        terrain->indexed_mesh_vertices == NULL ||
        terrain->derived_mesh.records == NULL ||
        terrain->derived_vertices == NULL) return 0;
    terrain->derived_mesh.width = terrain->dim[2];
    terrain->derived_mesh.height = terrain->dim[3];
    for (index = 0u; index < (size_t)terrain->dim[2] * terrain->dim[3];
         ++index) terrain->derived_mesh.records[index * 8u + 4u] = 1u;
    terrain->derived_mesh.dirty = &terrain->derived_mesh_dirty;
    terrain->derived_mesh.hooks.context = terrain;
    terrain->derived_mesh.hooks.create_quad = terrain_derived_quad_create;
    terrain->derived_mesh.hooks.vertices_emit =
        terrain_derived_quad_vertices_emit;
    terrain->indexed_mesh.block_count = (uint32_t)quad_count;
    terrain->indexed_mesh.reserved = 0u;
    terrain->indexed_mesh.blocks = terrain->indexed_mesh_blocks;
    terrain->detail_mesh_object.mesh_record = &terrain->indexed_mesh;
    terrain->detail_mesh_token =
        (uint32_t)(uintptr_t)&terrain->detail_mesh_object;
    for (index = 0u; index < quad_count; ++index) {
        uint8_t* block = terrain->indexed_mesh_blocks + index * 0x70u;
        uint32_t vertex_count = 4u;
        uint32_t detail_enabled = 1u;
        void* vertices = terrain->indexed_mesh_vertices + index * 32u;
        memcpy(block + 0x04u, &vertex_count, sizeof(vertex_count));
        memcpy(block + 0x0cu, &vertices, sizeof(vertices));
        memcpy(block + 0x6cu, &detail_enabled, sizeof(detail_enabled));
        terrain_indexed_mesh_cell_sync(terrain, index);
    }
    return 1;
}

static void terrain_create_vertices_write(void* context, uint32_t cell,
                                          const float vertices[12])
{
    TerrainState* terrain = (TerrainState*)context;
    float* output;
    uint32_t corner;
    if (terrain == NULL || terrain->cell_vertices == NULL) return;
    output = terrain->cell_vertices + (size_t)cell * 12u;
    for (corner = 0u; corner < 4u; ++corner) {
        /* FUN_0044e4d0 emits retail records as X,Z,height. TerrainState's
         * SceneRender-facing cache stores X,height,Z. */
        output[corner * 3u] = vertices[corner * 3u];
        output[corner * 3u + 1u] = vertices[corner * 3u + 2u];
        output[corner * 3u + 2u] = vertices[corner * 3u + 1u];
        if (vertices[corner * 3u + 2u] < terrain->minimum_y)
            terrain->minimum_y = vertices[corner * 3u + 2u];
    }
}

static int terrain_surface_triangle_height(const float a[3], const float b[3],
                                           const float c[3], float x, float z,
                                           float* height_out)
{
    const float denominator = (b[2] - c[2]) * (a[0] - c[0]) +
                              (c[0] - b[0]) * (a[2] - c[2]);
    float wa, wb, wc;
    if (height_out == NULL || denominator == 0.0f) return 0;
    wa = ((b[2] - c[2]) * (x - c[0]) +
          (c[0] - b[0]) * (z - c[2])) / denominator;
    wb = ((c[2] - a[2]) * (x - c[0]) +
          (a[0] - c[0]) * (z - c[2])) / denominator;
    wc = 1.0f - wa - wb;
    if (wa < -0.0001f || wb < -0.0001f || wc < -0.0001f) return 0;
    *height_out = wa * a[1] + wb * b[1] + wc * c[1];
    return 1;
}

static LevelState* terrain_material_transition_owner_get(
    TerrainSurfaceGrid* grid)
{
    if (grid != NULL && grid->owner_level != NULL)
        return grid->owner_level;
    return level_current_owner_get();
}

/* The retail transition path reads the four current mesh vertices. Use the
 * indexed mesh first and SceneRender's live quad cache when no TerrainState
 * geometry is available. TerrainSurfaceGrid::surface_vectors stores the
 * separate corner vectors and is not vertex-position data. */
static int terrain_material_transition_vertices_get(
    LevelState* level, TerrainState* terrain, TerrainSurfaceGrid* grid,
    uint32_t x, uint32_t y, float out[12])
{
    uint32_t corner;
    if (terrain_cell_surface_values_available(terrain, (int)x, (int)y)) {
        grid_cell_read_surface_values(terrain, (int)x, (int)y, out);
        return 1;
    }
    if (grid == NULL || x >= grid->width || y >= grid->height) return 0;
    if (level != NULL) {
        float scene_vertices[4][3];
        if (!scene_render_terrain_cell_vertices_get(
                level, x, y, scene_vertices)) return 0;
        for (corner = 0u; corner < 4u; ++corner) {
            out[corner * 3u] = scene_vertices[corner][0];
            out[corner * 3u + 1u] = scene_vertices[corner][2];
            out[corner * 3u + 2u] = scene_vertices[corner][1];
        }
        return 1;
    }
    return 0;
}

/* FUN_0044e6d0 copies words 6 and 7 from each nine-word vertex output slot
 * into its 0x38-byte transition record. Keep the full indexed-mesh payload
 * here so the saved words remain the source UV pair; the twelve-word
 * GridCellReadSurfaceValues result contains positions only. */
static int terrain_material_transition_vertex_payload_get(
    TerrainState* terrain, uint32_t x, uint32_t y, float out[4][9])
{
    TerrainIndexedMeshLayerObject object;
    TerrainIndexedMeshLayerOwner owner;
    size_t quad_index;
    if (terrain == NULL || out == NULL ||
        x >= terrain->dim[0] || y >= terrain->dim[1] ||
        terrain->indexed_mesh_blocks == NULL ||
        terrain->indexed_mesh_vertices == NULL) return 0;
    quad_index = (size_t)y * terrain->dim[0] + x;
    terrain_indexed_mesh_cell_sync(terrain, quad_index);
    memset(out, 0, sizeof(float) * 4u * 9u);
    terrain_indexed_mesh_layer_object_init(terrain, &object, &owner);
    (void)lwo_layer_polygon_vertex_data_read(&object, (int)quad_index, 0,
                                            4, out);
    return 1;
}

static void marker_cell(EngineState* st, uint32_t x, uint32_t y)
{
    TerrainSurfaceGrid* grid = path_record_surface_grid_get();
    if (st != NULL && grid != NULL && grid->width == st->sim_w &&
        grid->height == st->sim_h) {
        terrain_cell_surface_vectors_initialize(grid, x, y);
    }
    ++s_marker_fired;
}
