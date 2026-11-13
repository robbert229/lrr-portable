// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-049 Water.c's port tail (finding 614): the port's model of the
 * level's water pools, which it keeps as staged cell runs on the level
 * (TerrainWorldSpatialRecords), unannotated (ADR 0006 decision 4). The
 * original-shape unit is water.c.
 *
 * In the matching build (MSVC 5.0) water.c defines the port's names of
 * Water_Initialise and Water_DestroyWallComplete itself
 * (water_link_names.h), so their definitions here are compiled out there
 * (LRR_WATER_PORT_NAMES). The rest stays in every build: the native port
 * calls it. Relocated from the verbatim cell_flood_passes.cpp (body hashes:
 * docs/coverage/relocations/GAME-049.csv). The row-span buffer writers
 * (0046ead0, 0046eb30) had no native caller and are gone with view.cpp's
 * s_map_row_span_records (finding 614). */
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
#include "terrain_grid_for_cell_flood_passes.hpp"
#include "simtick.hpp"
#include "sound.hpp"
#include "game/ai.hpp"
#include "game/cmd.hpp"
#include "game/select.hpp"
#include "game/effects.hpp"
#include "game/render.hpp"
#include "simtick_for_cell_flood_passes.hpp"
#include <ctype.h>
#include <stdarg.h>
#include "scene_resources.hpp"
#include "scene_pool.hpp"
#include "string.hpp"
#include "model.hpp"
#include "view_for_cell_flood_passes.hpp"

typedef struct TerrainWorldSpatialStageContext {
    TerrainWorldSpatialRecords* owner;
    int failed;
} TerrainWorldSpatialStageContext;

static void terrain_world_spatial_candidates_select(
    TerrainWorldSpatialRecord* record, LevelState* level);

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static int terrain_world_spatial_records_merge( TerrainWorldSpatialRecords* owner);
static void terrain_world_spatial_records_sort( TerrainWorldSpatialRecords* owner);
static void terrain_world_spatial_run_flush(void* context, uint32_t row, uint32_t first_cell, uint32_t end_cell);

#if !(defined(_MSC_VER) && _MSC_VER < 1200)
#define LRR_WATER_PORT_NAMES 1
#endif

#ifdef LRR_WATER_PORT_NAMES
void terrain_cell_quad_run_stage_level(void* scene, LevelState* level)
{
    TerrainWorldSpatialRecords* owner;
    TerrainWorldSpatialStageContext context;
    uint32_t record_index;
    if (level == NULL) return;
    (void)scene;
    terrain_cell_quad_run_stage_level_release(level);
    owner = (TerrainWorldSpatialRecords*)calloc(1u, sizeof(*owner));
    if (owner == NULL) return;
    owner->owner = level;
    world_spatial_records_install(level, owner);
    context.owner = owner;
    context.failed = 0;
    /* This is the production CellQuadRunStage producer called at the end of
     * MapLoadPipeline after the voxel and terrain maps are initialized. */
    terrain_cell_quad_run_stage(level, &context, NULL,
                                terrain_world_spatial_run_flush, NULL);
    if (context.failed) {
        terrain_cell_quad_run_stage_level_release(level);
        return;
    }
    if (!terrain_world_spatial_records_merge(owner)) {
        terrain_cell_quad_run_stage_level_release(level);
        return;
    }
    terrain_world_spatial_records_sort(owner);
    staged_cell_candidate_select(NULL, level);
    /* Prime SceneRender's row-major CPU quad entries for the staged cells.
     * Native mesh creation/visibility remains with the DirectDraw owner. */
    for (record_index = 0u; record_index < owner->count; ++record_index) {
        TerrainCellQuadRunRecord* run = &owner->records[record_index].run;
        uint32_t item;
        for (item = 0u; item < run->item_count; ++item) {
            const float* xy = run->cell_xy + item * 2u;
            int cell_x = crt_float_to_long(xy[0]);
            int cell_y = crt_float_to_long(xy[1]);
            if (cell_x >= 0 && cell_y >= 0)
                scene_render_terrain_cell_overlay_height_fill(
                    NULL, level, (uint32_t)cell_x, (uint32_t)cell_y);
        }
    }
}
#endif /* LRR_WATER_PORT_NAMES */

/* StagedCellCandidateSelect (FUN_0046dd10): filter each staged item to its
 * first candidate whose cell is in bounds and is neither type 7 nor type 1. */
void staged_cell_candidate_select(void* unused, LevelState* level)
{
    uint32_t record_index;
    int width;
    int height;
    const uint8_t* voxels;
    StagedCellCandidateSelectionRecord* const records =
        s_staged_selection_records;
    const uint32_t record_count = s_staged_selection_record_count;
    (void)unused;
    if (level != NULL && level->ptr_terrain_quad_runs != NULL) {
        TerrainWorldSpatialRecords* owner =
            (TerrainWorldSpatialRecords*)level->ptr_terrain_quad_runs;
        if (owner->owner == level) {
            for (record_index = 0u; record_index < owner->count;
                 ++record_index)
                terrain_world_spatial_candidates_select(
                    &owner->records[record_index], level);
            return;
        }
    }
    if (level == NULL) return;
    width = level_width(level);
    height = level_height(level);
    voxels = (const uint8_t*)level->ptr_voxels;
    for (record_index = 0; record_index < record_count;
         ++record_index) {
        StagedCellCandidateSelectionRecord* record = &records[record_index];
        uint32_t item;
        record->selected_count = 0u;
        for (item = 0; item < record->item_count; ++item) {
            uint32_t alternative;
            const float* candidates = record->candidate_xy + item * 8u;
            for (alternative = 0; alternative < 4u; ++alternative) {
                int x = crt_float_to_long(candidates[alternative * 2u]);
                int y = crt_float_to_long(candidates[alternative * 2u + 1u]);
                uint32_t index;
                uint8_t cell_type;
                if (x < 0 || x >= width || y < 0 || y >= height) continue;
                index = ((uint32_t)y * (uint32_t)width + (uint32_t)x) *
                    LEVEL_VOXEL_STRIDE;
                cell_type = voxels[index + 2u];
                if (cell_type == 7u || cell_type == 1u) continue;
                record->selected_items[record->selected_count] = item;
                record->selected_alternatives[record->selected_count] =
                    alternative;
                ++record->selected_count;
                break;
            }
        }
    }
}

#ifdef LRR_WATER_PORT_NAMES
/* CellSurfaceQuadHeightFill (FUN_0046e220): find the staged per-level item,
 * activate its CPU slot, reset its offsets, and sample the selected edge.
 * The original scene object's visibility/lock operation remains renderer
 * owned; the record and selector now have a LevelState owner. */
void terrain_cell_surface_quad_height_fill_current(LevelState* level,
                                                    int x, int y)
{
    TerrainWorldSpatialRecord* record;
    TerrainSurfaceGrid* surface;
    uint32_t item = 0u;
    uint32_t orientation;
    uint32_t corner_a, corner_b;
    float vertices[12];
    float first_height, second_height;
    static const uint8_t corner_pairs[8] = {
        0u, 1u, 1u, 2u, 2u, 3u, 3u, 0u
    };
    if (level == NULL)
        level = (LevelState*)map_cell_record_grid_level_get();
    record = terrain_world_spatial_record_find_at(level, x, y, &item);
    surface = path_record_surface_grid_get();
    if (record == NULL || item >= record->run.selected_count ||
        item >= record->run.slot_capacity) return;
    record->run.flags = (record->run.flags & ~4u) | 1u;
    record->run.slot_active[item] = 1u;
    record->run.slot_offsets[item * 2u] = 0.0f;
    record->run.slot_offsets[item * 2u + 1u] = 0.0f;
    if (surface != NULL && surface->owner_level != NULL &&
        surface->owner_level != level) surface = NULL;
    if (surface != NULL && surface->surface_vectors != NULL && x >= 0 &&
        y >= 0 && (uint32_t)x < surface->width &&
        (uint32_t)y < surface->height) {
        (void)terrain_cell_surface_vertices_get(surface, (uint32_t)x,
                                                (uint32_t)y, vertices);
    }
    else {
        float world_vertices[4][3];
        uint32_t corner;
        if (!scene_render_terrain_cell_vertices_get(
                level, (uint32_t)x, (uint32_t)y, world_vertices)) return;
        /* SceneRender exposes conventional (X,height,Z), while the raw
         * terrain surface payload consumed by this retail helper is
         * (X,Z,height). */
        for (corner = 0u; corner < 4u; ++corner)
            vertices[corner * 3u + 2u] = world_vertices[corner][1];
    }
    orientation = record->run.slot_orientations[item];
    if (orientation > 3u) return;
    corner_a = corner_pairs[orientation * 2u];
    corner_b = corner_pairs[orientation * 2u + 1u];
    first_height = vertices[corner_a * 3u + 2u];
    second_height = vertices[corner_b * 3u + 2u];
    record->run.slot_heights[item] = second_height > first_height
        ? second_height : first_height;
    /* Publish the active scalar into the scene-side CPU overlay cache. */
    scene_render_terrain_cell_overlay_height_fill(
        NULL, level, (uint32_t)x, (uint32_t)y);
}
#endif /* LRR_WATER_PORT_NAMES */

void grid_surface_entry_reset(void* surface, int key)
{
    if (surface == NULL || s_grid_surface_lookup == NULL) return;
    int index = key;
    void* entry = s_grid_surface_lookup(surface, key, &index,
                                        s_grid_surface_user);
    if (entry == NULL || index < 0 || index > 4096) return;
    uint32_t flags = 0;
    uint32_t handle = 0u;
    __builtin_memcpy(&flags, (char*)entry + 0x424u, sizeof(flags));
    flags = (flags & ~2u) | 1u;
    __builtin_memcpy((char*)entry + 0x424u, &flags, sizeof(flags));
    __builtin_memcpy(&handle, (char*)entry + 0x420u, sizeof(handle));
    if (s_grid_surface_release != NULL) {
        s_grid_surface_release(handle, s_grid_surface_user);
    }
    uint32_t zero = 0;
    __builtin_memcpy((char*)entry + 0x338u + (size_t)index * 0x18u,
                     &zero, sizeof(zero));
}

void simtick_fx_bank_tick(EngineContext* c, float dt)
{
    uint32_t active_count = 0u;
    float scale = c != NULL && c->time_scale != 0.0f ? c->time_scale : 1.0f;
    for (uint32_t i = 0; i < s_fx_count; ++i) {
        SimtickFxEntry* entry = &s_fx_entries[i];
        float maximum = entry->initial;
        int any_active = 0;
        int fade_out = (entry->flags & 2u) == 0u;
        uint32_t count = entry->count;
        if (count > 256u) count = 256u;
        for (uint32_t j = 0; entry->subentries != NULL && j < count; ++j) {
            SimtickFxSubEntry* sub = &entry->subentries[j];
            int advance_secondary = sub->enabled == 0.0f
                || sub->value < entry->current;
            if (!advance_secondary) {
                if (maximum < sub->value) maximum = sub->value;
                fade_out = 0;
                ++active_count;
                any_active = 1;
                if (sub->value == entry->current) advance_secondary = 1;
            }
            sub->fade_primary += dt * 1.5f;
            if (advance_secondary) sub->fade_secondary += dt * 0.5f;
            if (sub->fade_primary > 240.0f) sub->fade_primary = 240.0f;
            if (sub->fade_secondary > 240.0f) sub->fade_secondary = 240.0f;
        }
        if ((entry->flags & 4u) != 0u) any_active = 0;
        if (any_active || fade_out) {
            const float rate = (entry->current - entry->initial) / scale;
            if (any_active) {
                entry->flags &= ~2u;
                entry->current += active_count * dt * 0.3f / (rate + 1.0f);
                if (entry->current >= maximum) {
                    entry->current = maximum;
                    entry->flags |= 4u;
                }
            } else {
                entry->flags &= ~4u;
                entry->current -= dt * 0.05f;
                if (entry->current <= maximum) {
                    entry->current = maximum;
                    entry->flags |= 2u;
                }
            }
            for (uint32_t j = 0; j < entry->resource_count; ++j) {
                if (s_fx_update != NULL) {
                    s_fx_update(entry->resource, j, entry->current, s_fx_user);
                }
            }
        }
    }
    if (c != NULL) {
        terrain_edge_flag_midpoint_update((LevelState*)c->level_block);
    }
}

/* StagedCellCandidateCollect (FUN_0046e9d0): walk the installed staged-cell
 * records for every cell in [first_cell,end_cell), join matching records
 * from the previous row, append pairs from the first matching record to each
 * distinct later match, and return the first matching record. This matches
 * the retail accumulator: after its first match, later matches preserve that
 * first record as the merge destination. */
void* staged_cell_candidate_collect(uint32_t row, uint32_t first_cell,
                                    uint32_t end_cell)
{
    uint32_t cell;
    uint32_t record_index;
    void* first_record = NULL;
    StagedCellCandidateLinks* const links = s_staged_candidate_links;
    const StagedCellCandidateRecord* const records = s_staged_candidate_records;
    const uint32_t record_count = s_staged_candidate_record_count;
    if (row == 0u || records == NULL || links == NULL ||
        links->previous_records == NULL || links->current_records == NULL)
        return NULL;
    for (cell = first_cell; cell < end_cell; ++cell) {
        for (record_index = 0; record_index < record_count; ++record_index) {
            const StagedCellCandidateRecord* record = &records[record_index];
            uint32_t pair;
            if (record->record == NULL || record->cell_pairs == NULL)
                continue;
            for (pair = 0; pair < record->pair_count; ++pair) {
                const float* xy = record->cell_pairs + pair * 2u;
                if (xy[0] == (float)cell && xy[1] == (float)(row - 1u)) {
                    if (first_record == NULL) {
                        first_record = record->record;
                    }
                    else if (first_record != record->record) {
                        if (links->count >= links->capacity)
                            return first_record;
                        links->previous_records[links->count] = first_record;
                        links->current_records[links->count] = record->record;
                        ++links->count;
                    }
                }
            }
        }
    }
    return first_record;
}

static void terrain_world_spatial_candidates_select(
    TerrainWorldSpatialRecord* record, LevelState* level)
{
    uint32_t item;
    int width = level_width(level);
    int height = level_height(level);
    const uint8_t* voxels = (const uint8_t*)level->ptr_voxels;
    if (record == NULL || voxels == NULL) return;
    record->run.selected_count = 0u;
    for (item = 0u; item < record->run.item_count; ++item) {
        uint32_t alternative;
        const float* xy = record->candidate_xy + item * 8u;
        for (alternative = 0u; alternative < 4u; ++alternative) {
            int x = crt_float_to_long(xy[alternative * 2u]);
            int y = crt_float_to_long(xy[alternative * 2u + 1u]);
            uint8_t type;
            if (x < 0 || y < 0 || x >= width || y >= height) continue;
            type = voxels[((size_t)(uint32_t)y * (size_t)width +
                           (uint32_t)x) * LEVEL_VOXEL_STRIDE + 2u];
            if (type == 7u || type == 1u) continue;
            {
                uint32_t slot = record->run.selected_count;
                record->run.slot_source_items[slot] = item;
                record->run.slot_orientations[slot] = alternative;
                record->run.slot_active[slot] = 0u;
                record->run.slot_offsets[slot * 2u] = 0.0f;
                record->run.slot_offsets[slot * 2u + 1u] = 0.0f;
                record->run.slot_heights[slot] = 0.0f;
                ++record->run.selected_count;
            }
            break;
        }
    }
}
static int terrain_world_spatial_merge_append(
    TerrainWorldSpatialRecords* owner, uint32_t destination, uint32_t source)
{
    TerrainWorldSpatialMerge* merges;
    uint32_t capacity;
    if (owner->merge_count >= owner->merge_capacity) {
        capacity = owner->merge_capacity == 0u
            ? 8u : owner->merge_capacity * 2u;
        if (capacity < owner->merge_capacity ||
            (size_t)capacity > ((size_t)-1) / sizeof(*merges)) return 0;
        merges = (TerrainWorldSpatialMerge*)realloc(
            owner->merges, (size_t)capacity * sizeof(*merges));
        if (merges == NULL) return 0;
        owner->merges = merges;
        owner->merge_capacity = capacity;
    }
    owner->merges[owner->merge_count].destination = destination;
    owner->merges[owner->merge_count].source = source;
    ++owner->merge_count;
    return 1;
}

static int terrain_world_spatial_records_reserve(
    TerrainWorldSpatialRecords* owner, uint32_t minimum)
{
    TerrainWorldSpatialRecord* records;
    uint32_t capacity;
    if (owner->capacity >= minimum) return 1;
    capacity = owner->capacity == 0u ? 8u : owner->capacity;
    while (capacity < minimum) {
        if (capacity > 0x7fffffffu) return 0;
        capacity *= 2u;
    }
    records = (TerrainWorldSpatialRecord*)realloc(
        owner->records, (size_t)capacity * sizeof(*records));
    if (records == NULL) return 0;
    memset(records + owner->capacity, 0,
           (size_t)(capacity - owner->capacity) * sizeof(*records));
    owner->records = records;
    owner->capacity = capacity;
    return 1;
}

static int terrain_world_spatial_record_reserve_items(
    TerrainWorldSpatialRecord* record, uint32_t minimum)
{
    TerrainCellQuadRunRecord* run = &record->run;
    uint32_t capacity = run->slot_capacity;
    float* cells;
    float* candidates;
    float* offsets;
    float* heights;
    uint8_t* active;
    uint32_t* source_items;
    uint32_t* orientations;
    if (capacity >= minimum) return 1;
    capacity = capacity == 0u ? 8u : capacity;
    while (capacity < minimum) {
        if (capacity > 0x7fffffffu) return 0;
        capacity *= 2u;
    }
    cells = (float*)realloc((void*)run->cell_xy,
                            (size_t)capacity * 2u * sizeof(float));
    if (cells == NULL) return 0;
    run->cell_xy = cells;
    candidates = (float*)realloc(record->candidate_xy,
        (size_t)capacity * 8u * sizeof(float));
    if (candidates == NULL) return 0;
    record->candidate_xy = candidates;
    offsets = (float*)realloc(run->slot_offsets,
                              (size_t)capacity * 2u * sizeof(float));
    if (offsets == NULL) return 0;
    run->slot_offsets = offsets;
    heights = (float*)realloc(run->slot_heights,
                              (size_t)capacity * sizeof(float));
    if (heights == NULL) return 0;
    run->slot_heights = heights;
    active = (uint8_t*)realloc(run->slot_active,
                               (size_t)capacity * sizeof(uint8_t));
    if (active == NULL) return 0;
    run->slot_active = active;
    source_items = (uint32_t*)realloc(run->slot_source_items,
                                      (size_t)capacity * sizeof(uint32_t));
    if (source_items == NULL) return 0;
    run->slot_source_items = source_items;
    orientations = (uint32_t*)realloc(
        (void*)run->slot_orientations,
        (size_t)capacity * sizeof(uint32_t));
    if (orientations == NULL) return 0;
    run->slot_orientations = orientations;
    memset(cells + (size_t)run->slot_capacity * 2u, 0,
           (size_t)(capacity - run->slot_capacity) * 2u * sizeof(float));
    memset(candidates + (size_t)run->slot_capacity * 8u, 0,
           (size_t)(capacity - run->slot_capacity) * 8u * sizeof(float));
    memset(offsets + (size_t)run->slot_capacity * 2u, 0,
           (size_t)(capacity - run->slot_capacity) * 2u * sizeof(float));
    memset(heights + run->slot_capacity, 0,
           (size_t)(capacity - run->slot_capacity) * sizeof(float));
    memset(active + run->slot_capacity, 0,
           (size_t)(capacity - run->slot_capacity) * sizeof(uint8_t));
    memset(source_items + run->slot_capacity, 0,
           (size_t)(capacity - run->slot_capacity) * sizeof(uint32_t));
    memset(orientations + run->slot_capacity, 0,
           (size_t)(capacity - run->slot_capacity) * sizeof(uint32_t));
    run->slot_capacity = capacity;
    return 1;
}

static int terrain_world_spatial_record_append(
    TerrainWorldSpatialRecord* record, uint32_t x0, uint32_t x1,
    uint32_t y)
{
    uint32_t x;
    TerrainCellQuadRunRecord* run = &record->run;
    for (x = x0; x < x1; ++x) {
        uint32_t item = run->item_count;
        float* cells;
        float* candidates;
        static const int candidate_dx[4] = { 0, 1, 0, -1 };
        static const int candidate_dy[4] = { -1, 0, 1, 0 };
        uint32_t alternative;
        if (!terrain_world_spatial_record_reserve_items(record, item + 1u))
            return 0;
        cells = (float*)run->cell_xy;
        candidates = record->candidate_xy;
        cells[item * 2u] = (float)x;
        cells[item * 2u + 1u] = (float)y;
        for (alternative = 0u; alternative < 4u; ++alternative) {
            candidates[item * 8u + alternative * 2u] =
                (float)((int)x + candidate_dx[alternative]);
            candidates[item * 8u + alternative * 2u + 1u] =
                (float)((int)y + candidate_dy[alternative]);
        }
        ++run->item_count;
    }
    return 1;
}

static void terrain_world_spatial_run_flush(void* context, uint32_t row,
                                           uint32_t first_cell,
                                           uint32_t end_cell)
{
    TerrainWorldSpatialStageContext* stage =
        (TerrainWorldSpatialStageContext*)context;
    TerrainWorldSpatialRecord* selected = NULL;
    uint32_t selected_index = 0u;
    uint32_t x, record_index;
    if (stage == NULL || stage->owner == NULL || stage->failed) return;
    /* StagedCellCandidateCollect searches the prior row for matching x
     * coordinates and returns the last record found. */
    if (row != 0u) {
        for (x = first_cell; x < end_cell; ++x) {
            for (record_index = 0u;
                 record_index < stage->owner->count; ++record_index) {
                TerrainWorldSpatialRecord* record =
                    &stage->owner->records[record_index];
                uint32_t item;
                for (item = 0u; item < record->run.item_count; ++item) {
                    const float* xy = record->run.cell_xy + item * 2u;
                    if (crt_float_to_long(xy[0]) == (int)x &&
                        crt_float_to_long(xy[1]) == (int)(row - 1u)) {
                        if (selected == NULL) {
                            selected = record;
                            selected_index = record_index;
                        }
                        else if (selected != record &&
                            !terrain_world_spatial_merge_append(
                                stage->owner, selected_index, record_index)) {
                            stage->failed = 1;
                            return;
                        }
                    }
                }
            }
        }
    }
    if (selected == NULL) {
        if (!terrain_world_spatial_records_reserve(stage->owner,
                                                   stage->owner->count + 1u)) {
            stage->failed = 1;
            return;
        }
        selected_index = stage->owner->count++;
        selected = &stage->owner->records[selected_index];
        memset(selected, 0, sizeof(*selected));
        selected->run.mesh = -1;
    }
    if (!terrain_world_spatial_record_append(selected, first_cell, end_cell,
                                              row))
        stage->failed = 1;
}

static int terrain_world_spatial_records_merge(
    TerrainWorldSpatialRecords* owner)
{
    uint32_t merge_index;
    uint32_t read_index;
    uint32_t write_index = 0u;
    for (merge_index = 0u; merge_index < owner->merge_count; ++merge_index) {
        TerrainWorldSpatialMerge* merge = &owner->merges[merge_index];
        TerrainWorldSpatialRecord* destination;
        TerrainWorldSpatialRecord* source;
        uint32_t item;
        if (merge->destination >= owner->count ||
            merge->source >= owner->count ||
            merge->destination == merge->source) continue;
        destination = &owner->records[merge->destination];
        source = &owner->records[merge->source];
        for (item = 0u; item < source->run.item_count; ++item) {
            uint32_t target_item = destination->run.item_count;
            if (!terrain_world_spatial_record_reserve_items(
                    destination, target_item + 1u)) return 0;
            ((float*)destination->run.cell_xy)[target_item * 2u] =
                source->run.cell_xy[item * 2u];
            ((float*)destination->run.cell_xy)[target_item * 2u + 1u] =
                source->run.cell_xy[item * 2u + 1u];
            memcpy(destination->candidate_xy + target_item * 8u,
                   source->candidate_xy + item * 8u,
                   8u * sizeof(float));
            destination->run.slot_active[target_item] =
                source->run.slot_active[item];
            destination->run.slot_source_items[target_item] =
                source->run.slot_source_items[item];
            destination->run.slot_offsets[target_item * 2u] =
                source->run.slot_offsets[item * 2u];
            destination->run.slot_offsets[target_item * 2u + 1u] =
                source->run.slot_offsets[item * 2u + 1u];
            destination->run.slot_heights[target_item] =
                source->run.slot_heights[item];
            ((uint32_t*)destination->run.slot_orientations)[target_item] =
                source->run.slot_orientations[item];
            ++destination->run.item_count;
        }
        source->run.item_count = 0u;
    }
    for (read_index = 0u; read_index < owner->count; ++read_index) {
        if (owner->records[read_index].run.item_count == 0u) {
            terrain_world_spatial_record_release(&owner->records[read_index]);
            continue;
        }
        if (write_index != read_index) {
            owner->records[write_index] = owner->records[read_index];
            memset(&owner->records[read_index], 0,
                   sizeof(owner->records[read_index]));
        }
        ++write_index;
    }
    owner->count = write_index;
    owner->merge_count = 0u;
    return 1;
}

/* Comparator at LAB_0046e9a0 reads the dword at record + 0x320, the
 * accumulated run item count. Its return value orders larger counts first. */
static void terrain_world_spatial_records_sort(
    TerrainWorldSpatialRecords* owner)
{
    uint32_t index;
    for (index = 1u; index < owner->count; ++index) {
        TerrainWorldSpatialRecord value = owner->records[index];
        uint32_t position = index;
        while (position > 0u &&
               owner->records[position - 1u].run.item_count <
                   value.run.item_count) {
            owner->records[position] = owner->records[position - 1u];
            --position;
        }
        owner->records[position] = value;
    }
}
