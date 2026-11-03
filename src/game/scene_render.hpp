// SPDX-License-Identifier: LGPL-3.0-or-later
/* Scene-specific render setup entrypoints. */
#ifndef LEGORRR_SCENE_RENDER_HPP
#define LEGORRR_SCENE_RENDER_HPP

#include "../gods98/resource.hpp"
#include <stdint.h>

struct LevelState;

int scene_lights_build_from_config(const SceneLightBuildOps* ops);
/* Scene height contract: Y is signed/scaled geometry height, initially
 * -SurfaceMap byte * TerrainCreate's retained height multiplier. Both initial
 * seeding and TerrainCellMutationProgress submit canonical corners 00,10,11,01.
 * No consumer may reapply a sign or height scale to cached Y. */
void scene_render_terrain_cell_vertices_update(const struct LevelState* level,
                                              uint32_t x, uint32_t y,
                                              const float vertices[12]);
/* Per-cell RGB override emitted by TerrainCellSurfaceAppearanceApply. The
 * retail path always supplies alpha 1.0, so the native cache stores opaque
 * color only. */
void scene_render_terrain_cell_surface_color_set(
    const struct LevelState* level, uint32_t x, uint32_t y,
    float r, float g, float b);
/* Apply the retail first-component displacement to a shared map vertex. */
void scene_render_terrain_vertex_offset(const struct LevelState* level,
                                        int vertex_x, int vertex_y,
                                        float world_delta);
void scene_render_terrain_cell_orientation_apply(
    const struct LevelState* level, uint32_t x, uint32_t y, int flipped);
/* Returns geometry Y on the selected diagonal at the quad center. */
int scene_render_terrain_cell_height_center_get(const struct LevelState* level,
                                               uint32_t x, uint32_t y,
                                               float* height_out);
/* Returns presentation XYZ, rotating complete triples when flipped so that
 * triangles (0,1,2)/(0,2,3) match rendering. Cache storage stays canonical. */
int scene_render_terrain_cell_vertices_get(const struct LevelState* level,
                                           uint32_t x, uint32_t y,
                                           float vertices[4][3]);
/* Same geometry Y and presentation X/Z, in canonical 00,10,11,01 order.
 * Mutation fallback applies world-Y deltas to these stable physical corners. */
int scene_render_terrain_cell_vertices_canonical_get(
    const struct LevelState* level, uint32_t x, uint32_t y,
    float vertices[4][3]);
/* CPU copy of the per-cell height quad filled by TerrainCellMutation. The
 * native renderer owns the row-major record; the DDi object mutation remains
 * an engine-side operation. */
void scene_render_terrain_cell_overlay_height_fill(void* context,
                                                   struct LevelState* level,
                                                   uint32_t x, uint32_t y);
int scene_render_terrain_cell_overlay_height_get(
    const struct LevelState* level, uint32_t x, uint32_t y,
    float heights[4]);
int scene_render_terrain_cell_overlay_slot_get(
    const struct LevelState* level, uint32_t x, uint32_t y,
    float* height_out);
void scene_render_terrain_cell_overlay_slot_reset(
    const struct LevelState* level, uint32_t x, uint32_t y);
void scene_render_terrain_level_release(const struct LevelState* level);

#endif /* LEGORRR_SCENE_RENDER_HPP */
