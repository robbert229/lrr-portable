// SPDX-License-Identifier: LGPL-3.0-or-later
/* Scene-specific render setup entrypoints. */
#ifndef LEGORRR_SCENE_RENDER_HPP
#define LEGORRR_SCENE_RENDER_HPP

#include "../gods98/resource.hpp"
#include <stdint.h>

struct LevelState;

int scene_lights_build_from_config(const SceneLightBuildOps* ops);
/* Update the live native terrain quad from the four vertices emitted by the
 * level's TerrainCellMutationProgress path. */
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
int scene_render_terrain_cell_height_center_get(const struct LevelState* level,
                                               uint32_t x, uint32_t y,
                                               float* height_out);
int scene_render_terrain_cell_vertices_get(const struct LevelState* level,
                                           uint32_t x, uint32_t y,
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
