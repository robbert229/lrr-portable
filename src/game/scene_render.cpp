// SPDX-License-Identifier: LGPL-3.0-or-later
/* Software perspective scene renderer.
 *
 * The terrain SurfaceMap is the authored heightfield; the erosion/surface
 * voxel layers select the visible material. This renderer turns those data
 * into depth-tested terrain faces and renders the placed Pilot entities as
 * simple faceted figures. It is deliberately isolated from the simulation:
 * callers provide the current level/entities and a locked RGBA framebuffer.
 * The Win32 camera is the engine/view owner's last displayed projection.
 */
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "entity.hpp"
#include "ai.hpp"
#include "crt/crt.hpp"
#include "level.hpp"
#include "render.hpp"
#include "scene_render.hpp"
#include "tutorial_hud_ports.hpp"
#include "engine.hpp"
#include "gods98/heap.hpp"
#include "gods98/record.hpp"
#include "gods98/surface.hpp"
#include "gods98/view.hpp"
#include "terrain_grid.hpp"
#include "port_loop_ports.hpp"
#include "select.hpp"
#include "texture.hpp"

typedef struct SceneVec3 { float x, y, z; } SceneVec3;
typedef struct ScenePoint { float x, y, z; } ScenePoint;
typedef struct SceneColor { uint8_t r, g, b; } SceneColor;
typedef struct SceneCamera {
    SceneVec3 eye, right, up, forward;
    float focal_x, focal_y, cx, cy;
    int width, height;
    void* pixels;
    int pitch;
    float* depth;
} SceneCamera;

static float* s_scene_depth;
static size_t s_scene_depth_capacity;
static TextureSurface* s_scene_floor_texture;
static const LevelState* s_scene_focus_level;
static SceneVec3 s_scene_focus;
static const LevelState* s_scene_vertex_level;
static uint32_t s_scene_vertex_width;
static uint32_t s_scene_vertex_height;
/* Canonical map corners (00,10,11,01), signed/scaled geometry Y.
 * Orientation selects a diagonal; it never moves heights between X/Z slots. */
static float* s_scene_vertex_heights;
static uint8_t* s_scene_vertex_valid;
static uint8_t* s_scene_vertex_flipped;
static float* s_scene_surface_colors;
static uint8_t* s_scene_surface_color_valid;
static const LevelState* s_scene_surface_display_level;
static float* s_scene_surface_display_colors;
static uint8_t* s_scene_surface_display_valid;
static uint8_t* s_scene_surface_display_markers;
static size_t s_scene_surface_display_capacity;
static uint32_t s_scene_surface_display_width, s_scene_surface_display_height;
static float* s_scene_overlay_heights;
static uint8_t* s_scene_overlay_valid;
static float* s_scene_overlay_slot_heights;
static uint8_t* s_scene_overlay_slot_active;

static int scene_render_vertex_cache_ensure(const LevelState* level)
{
    uint32_t width, height;
    size_t count;
    float* heights;
    uint8_t* valid;
    uint8_t* flipped;
    float* surface_colors;
    uint8_t* surface_color_valid;
    float* overlay_heights;
    uint8_t* overlay_valid;
    float* overlay_slot_heights;
    uint8_t* overlay_slot_active;
    if (level == NULL) return 0;
    width = (uint32_t)level_width(level);
    height = (uint32_t)level_height(level);
    if (s_scene_vertex_level == level && s_scene_vertex_heights != NULL &&
        s_scene_vertex_width == width && s_scene_vertex_height == height)
        return 1;
    free(s_scene_vertex_heights);
    free(s_scene_vertex_valid);
    free(s_scene_vertex_flipped);
    free(s_scene_surface_colors);
    free(s_scene_surface_color_valid);
    free(s_scene_overlay_heights);
    free(s_scene_overlay_valid);
    free(s_scene_overlay_slot_heights);
    free(s_scene_overlay_slot_active);
    s_scene_vertex_heights = NULL;
    s_scene_vertex_valid = NULL;
    s_scene_vertex_flipped = NULL;
    s_scene_surface_colors = NULL;
    s_scene_surface_color_valid = NULL;
    s_scene_overlay_heights = NULL;
    s_scene_overlay_valid = NULL;
    s_scene_overlay_slot_heights = NULL;
    s_scene_overlay_slot_active = NULL;
    s_scene_vertex_level = NULL;
    s_scene_vertex_width = 0u;
    s_scene_vertex_height = 0u;
    if (width == 0u || height == 0u ||
        (size_t)width > ((size_t)-1) / (size_t)height) return 0;
    count = (size_t)width * (size_t)height;
    if (count > ((size_t)-1) / (4u * sizeof(float))) return 0;
    heights = (float*)malloc(count * 4u * sizeof(float));
    valid = (uint8_t*)calloc(count, sizeof(uint8_t));
    flipped = (uint8_t*)calloc(count, sizeof(uint8_t));
    surface_colors = (float*)malloc(count * 3u * sizeof(float));
    surface_color_valid = (uint8_t*)calloc(count, sizeof(uint8_t));
    overlay_heights = (float*)calloc(count * 4u, sizeof(float));
    overlay_valid = (uint8_t*)calloc(count, sizeof(uint8_t));
    overlay_slot_heights = (float*)calloc(count, sizeof(float));
    overlay_slot_active = (uint8_t*)calloc(count, sizeof(uint8_t));
    if (heights == NULL || valid == NULL || flipped == NULL ||
        surface_colors == NULL || surface_color_valid == NULL ||
        overlay_heights == NULL ||
        overlay_valid == NULL || overlay_slot_heights == NULL ||
        overlay_slot_active == NULL) {
        free(heights);
        free(valid);
        free(flipped);
        free(surface_colors);
        free(surface_color_valid);
        free(overlay_heights);
        free(overlay_valid);
        free(overlay_slot_heights);
        free(overlay_slot_active);
        return 0;
    }
    s_scene_vertex_level = level;
    s_scene_vertex_width = width;
    s_scene_vertex_height = height;
    s_scene_vertex_heights = heights;
    s_scene_vertex_valid = valid;
    s_scene_vertex_flipped = flipped;
    s_scene_surface_colors = surface_colors;
    s_scene_surface_color_valid = surface_color_valid;
    s_scene_overlay_heights = overlay_heights;
    s_scene_overlay_valid = overlay_valid;
    s_scene_overlay_slot_heights = overlay_slot_heights;
    s_scene_overlay_slot_active = overlay_slot_active;
    return 1;
}

int scene_render_terrain_quad_allocate(const LevelState* level,
                                       uint32_t index, uint32_t quad_width)
{
    if (!scene_render_vertex_cache_ensure(level) || quad_width == 0u ||
        quad_width + 1u != s_scene_vertex_width ||
        index / quad_width + 1u >= s_scene_vertex_height ||
        index > 0x7fffffffu) return -1;
    return (int)index;
}

void scene_render_terrain_cell_orientation_apply(const LevelState* level,
                                                uint32_t x, uint32_t y,
                                                int flipped)
{
    size_t index;
    if (!scene_render_vertex_cache_ensure(level) ||
        x >= s_scene_vertex_width || y >= s_scene_vertex_height) return;
    /* This is the scene's map-width cache, not the DDi quad-width list. */
    index = (size_t)y * s_scene_vertex_width + x;
    s_scene_vertex_flipped[index] = flipped != 0 ? 1u : 0u;
}

void scene_render_terrain_cell_vertices_update(const LevelState* level,
                                              uint32_t x, uint32_t y,
                                              const float vertices[12])
{
    size_t index;
    uint32_t corner;
    if (vertices == NULL || !scene_render_vertex_cache_ensure(level) ||
        x >= s_scene_vertex_width || y >= s_scene_vertex_height) return;
    index = (size_t)y * s_scene_vertex_width + x;
    for (corner = 0u; corner < 4u; ++corner)
        s_scene_vertex_heights[index * 4u + corner] =
            vertices[corner * 3u + 1u];
    s_scene_vertex_valid[index] = 1u;
}

void scene_render_terrain_surface_display_discard(void)
{
    s_scene_surface_display_level = NULL;
}

int scene_render_terrain_surface_display_begin(const LevelState* level)
{
    size_t count;
    float* colors;
    uint8_t* valid;
    uint8_t* markers;
    size_t i;
    scene_render_terrain_surface_display_discard();
    if (!scene_render_vertex_cache_ensure(level)) return 0;
    count = (size_t)s_scene_vertex_width * s_scene_vertex_height;
    if (count > ((size_t)-1) / (3u * sizeof(float))) return 0;
    if (count > s_scene_surface_display_capacity) {
        colors = (float*)malloc(count * 3u * sizeof(float));
        valid = (uint8_t*)malloc(count);
        markers = (uint8_t*)malloc(count);
        if (colors == NULL || valid == NULL || markers == NULL) {
            free(colors);
            free(valid);
            free(markers);
            return 0;
        }
        free(s_scene_surface_display_colors);
        free(s_scene_surface_display_valid);
        free(s_scene_surface_display_markers);
        s_scene_surface_display_markers = markers;
        s_scene_surface_display_colors = colors;
        s_scene_surface_display_valid = valid;
        s_scene_surface_display_capacity = count;
    }
    memcpy(s_scene_surface_display_colors, s_scene_surface_colors,
           count * 3u * sizeof(float));
    memcpy(s_scene_surface_display_valid, s_scene_surface_color_valid, count);
    /* The fallback shader also reads callback92's marker. Capture that
     * input without freezing unrelated material or geometry updates. */
    for (i = 0u; i < count; ++i) {
        const uint8_t* voxel = level_voxel(level,
            (int)(i % s_scene_vertex_width),
            (int)(i / s_scene_vertex_width));
        s_scene_surface_display_markers[i] = voxel != NULL ? voxel[4] : 0u;
    }
    s_scene_surface_display_width = s_scene_vertex_width;
    s_scene_surface_display_height = s_scene_vertex_height;
    s_scene_surface_display_level = level;
    return 1;
}

void scene_render_terrain_cell_surface_color_set(const LevelState* level,
                                                uint32_t x, uint32_t y,
                                                float r, float g, float b)
{
    size_t index;
    if (!scene_render_vertex_cache_ensure(level) ||
        x >= s_scene_vertex_width || y >= s_scene_vertex_height) return;
    /* The target's renderer list uses the quad-width stride (map width - 1)
     * while the surrounding LevelState records use map width. */
    index = (size_t)y * (size_t)(s_scene_vertex_width - 1u) + x;
    if (index >= (size_t)s_scene_vertex_width * s_scene_vertex_height) return;
    s_scene_surface_colors[index * 3u] = r;
    s_scene_surface_colors[index * 3u + 1u] = g;
    s_scene_surface_colors[index * 3u + 2u] = b;
    s_scene_surface_color_valid[index] = 1u;
}

void scene_render_terrain_vertex_offset(const LevelState* level,
                                        int vertex_x, int vertex_y,
                                        float world_delta)
{
    static const int dx[4] = { 0, -1, -1, 0 };
    static const int dy[4] = { 0, 0, -1, -1 };
    unsigned int corner;
    if (!scene_render_vertex_cache_ensure(level)) return;
    for (corner = 0u; corner < 4u; ++corner) {
        int x = vertex_x + dx[corner];
        int y = vertex_y + dy[corner];
        size_t index;
        if (x < 0 || y < 0 || (uint32_t)x + 1u >= s_scene_vertex_width ||
            (uint32_t)y + 1u >= s_scene_vertex_height) continue;
        index = (size_t)y * s_scene_vertex_width + (uint32_t)x;
        if (s_scene_vertex_valid[index] != 0u)
            s_scene_vertex_heights[index * 4u + corner] += world_delta;
    }
}

void scene_render_terrain_level_release(const LevelState* level)
{
    if (level == NULL || level == s_scene_focus_level) {
        s_scene_focus_level = NULL;
        memset(&s_scene_focus, 0, sizeof(s_scene_focus));
    }
    if (level != NULL && level != s_scene_vertex_level) return;
    scene_render_terrain_surface_display_discard();
    free(s_scene_surface_display_colors);
    free(s_scene_surface_display_valid);
    free(s_scene_surface_display_markers);
    s_scene_surface_display_markers = NULL;
    s_scene_surface_display_colors = NULL;
    s_scene_surface_display_valid = NULL;
    s_scene_surface_display_capacity = 0u;
    free(s_scene_vertex_heights);
    free(s_scene_vertex_valid);
    free(s_scene_vertex_flipped);
    free(s_scene_surface_colors);
    free(s_scene_surface_color_valid);
    free(s_scene_overlay_heights);
    free(s_scene_overlay_valid);
    free(s_scene_overlay_slot_heights);
    free(s_scene_overlay_slot_active);
    s_scene_vertex_heights = NULL;
    s_scene_vertex_valid = NULL;
    s_scene_vertex_flipped = NULL;
    s_scene_surface_colors = NULL;
    s_scene_surface_color_valid = NULL;
    s_scene_overlay_heights = NULL;
    s_scene_overlay_valid = NULL;
    s_scene_overlay_slot_heights = NULL;
    s_scene_overlay_slot_active = NULL;
    s_scene_vertex_level = NULL;
    s_scene_vertex_width = 0u;
    s_scene_vertex_height = 0u;
}

static int scene_render_terrain_cell_vertex_heights(
    const LevelState* level, uint32_t x, uint32_t y, float heights[4])
{
    size_t index;
    if (heights == NULL || !scene_render_vertex_cache_ensure(level) ||
        x >= s_scene_vertex_width || y >= s_scene_vertex_height) return 0;
    index = (size_t)y * s_scene_vertex_width + x;
    if (s_scene_vertex_valid[index] == 0u) return 0;
    memcpy(heights, s_scene_vertex_heights + index * 4u,
           4u * sizeof(float));
    return 1;
}

/* Public quad order matches the renderer's two 0-to-2 triangles. */
int scene_render_terrain_cell_height_center_get(const LevelState* level,
                                               uint32_t x, uint32_t y,
                                               float* height_out)
{
    float vertices[4][3];
    if (height_out == NULL ||
        !scene_render_terrain_cell_vertices_get(level, x, y, vertices))
        return 0;
    *height_out = (vertices[0][1] + vertices[2][1]) * 0.5f;
    return 1;
}

int scene_render_terrain_cell_vertices_get(const LevelState* level,
                                           uint32_t x, uint32_t y,
                                           float vertices[4][3])
{
    static const uint8_t forward[4] = { 3u, 0u, 1u, 2u };
    float canonical[4][3];
    uint32_t corner;
    int flipped;
    if (vertices == NULL ||
        !scene_render_terrain_cell_vertices_canonical_get(
            level, x, y, canonical)) return 0;
    flipped = s_scene_vertex_flipped != NULL &&
        s_scene_vertex_level == level &&
        s_scene_vertex_flipped[(size_t)y * s_scene_vertex_width + x] != 0u;
    for (corner = 0u; corner < 4u; ++corner)
        memcpy(vertices[corner], canonical[!flipped ? forward[corner] : corner],
               3u * sizeof(float));
    return 1;
}

int scene_render_terrain_cell_vertices_canonical_get(
    const LevelState* level, uint32_t x, uint32_t y, float vertices[4][3])
{
    static const int corner_x[4] = { 0, 1, 1, 0 };
    static const int corner_y[4] = { 0, 0, 1, 1 };
    float heights[4];
    int width, height;
    float block;
    float height_scale;
    float base[12];
    uint32_t corner;
    if (vertices == NULL || level == NULL || level->ptr_terrain == NULL ||
        x >= (uint32_t)level_width(level) ||
        y >= (uint32_t)level_height(level)) return 0;
    width = level_width(level);
    height = level_height(level);
    block = terrain_blocksize_get(level->ptr_terrain);
    if (block <= 0.0f) block = 40.0f;
    height_scale = terrain_height_scale_get(level->ptr_terrain);
    if (terrain_cell_base_vertices_get(level->ptr_terrain, x, y, base)) {
        for (corner = 0u; corner < 4u; ++corner)
            heights[corner] = base[corner * 3u + 1u];
    }
    else if (!scene_render_terrain_cell_vertex_heights(level, x, y, heights)) {
        for (corner = 0u; corner < 4u; ++corner) {
            int sample_x = (int)x + corner_x[corner];
            int sample_y = (int)y + corner_y[corner];
            /* The presentation shell extends the last physical row/column;
             * it has no extra authored height byte beyond that boundary. */
            if (sample_x >= width) sample_x = width - 1;
            if (sample_y >= height) sample_y = height - 1;
            heights[corner] = -(float)terrain_surface_height_get(
                level->ptr_terrain, sample_x, sample_y) * height_scale;
        }
    }
    for (corner = 0u; corner < 4u; ++corner) {
        const float gx = (float)((int)x + corner_x[corner]);
        const float gy = (float)((int)y + corner_y[corner]);
        vertices[corner][0] = (gx - (float)width * 0.5f) * block;
        vertices[corner][1] = heights[corner];
        vertices[corner][2] = (gy - (float)height * 0.5f) * block;
    }
    return 1;
}

void scene_render_terrain_cell_overlay_height_fill(void* context,
                                                   LevelState* level,
                                                   uint32_t x, uint32_t y)
{
    float vertices[4][3];
    size_t index;
    uint32_t corner;
    (void)context;
    if (!scene_render_vertex_cache_ensure(level) ||
        x >= s_scene_vertex_width || y >= s_scene_vertex_height ||
        !scene_render_terrain_cell_vertices_get(level, x, y, vertices))
        return;
    index = (size_t)y * s_scene_vertex_width + x;
    /* Keep the four live geometry-space heights with their row-major quad.
     * The staged DDi record selects an edge pair and owns its own +8 scalar;
     * this cache retains source values without guessing that selector. */
    for (corner = 0u; corner < 4u; ++corner)
        s_scene_overlay_heights[index * 4u + corner] = vertices[corner][1];
    s_scene_overlay_valid[index] = 1u;
    {
        float height;
        uint32_t active = 0u;
        if (terrain_cell_quad_run_slot_state_get(
                level, (int)x, (int)y, &height, &active)) {
            s_scene_overlay_slot_heights[index] = height;
            s_scene_overlay_slot_active[index] = (uint8_t)active;
        }
        else {
            s_scene_overlay_slot_active[index] = 0u;
        }
    }
}

int scene_render_terrain_cell_overlay_height_get(
    const LevelState* level, uint32_t x, uint32_t y, float heights[4])
{
    size_t index;
    if (heights == NULL || !scene_render_vertex_cache_ensure(level) ||
        x >= s_scene_vertex_width || y >= s_scene_vertex_height)
        return 0;
    index = (size_t)y * s_scene_vertex_width + x;
    if (s_scene_overlay_valid[index] == 0u) return 0;
    memcpy(heights, s_scene_overlay_heights + index * 4u,
           4u * sizeof(float));
    return 1;
}

int scene_render_terrain_cell_overlay_slot_get(
    const LevelState* level, uint32_t x, uint32_t y, float* height_out)
{
    size_t index;
    if (!scene_render_vertex_cache_ensure(level) ||
        x >= s_scene_vertex_width || y >= s_scene_vertex_height)
        return 0;
    index = (size_t)y * s_scene_vertex_width + x;
    if (s_scene_overlay_slot_active[index] == 0u) return 0;
    if (height_out != NULL) *height_out = s_scene_overlay_slot_heights[index];
    return 1;
}

void scene_render_terrain_cell_overlay_slot_reset(const LevelState* level,
                                                  uint32_t x, uint32_t y)
{
    size_t index;
    if (!scene_render_vertex_cache_ensure(level) ||
        x >= s_scene_vertex_width || y >= s_scene_vertex_height) return;
    index = (size_t)y * s_scene_vertex_width + x;
    s_scene_overlay_slot_active[index] = 0u;
}

/* MSVC 5.0 accepts aggregate initialization for declarations, but not the
 * C++11 braced expressions used as return values and function arguments. */
static SceneVec3 scene_vec3(float x, float y, float z)
{
    SceneVec3 v;
    v.x = x; v.y = y; v.z = z;
    return v;
}

static SceneColor scene_color(uint8_t r, uint8_t g, uint8_t b)
{
    SceneColor c;
    c.r = r; c.g = g; c.b = b;
    return c;
}

typedef struct SceneRingTarget {
    void* pixels;
    int pitch;
    int width;
    int height;
} SceneRingTarget;

static void scene_ring_pixel(SceneRingTarget* target, int x, int y,
                             const float rgb[3])
{
    uint8_t* pixel;
    int channel;
    if (target == NULL || target->pixels == NULL || rgb == NULL ||
        x < 0 || y < 0 || x >= target->width || y >= target->height) return;
    pixel = (uint8_t*)target->pixels + (size_t)y * (size_t)target->pitch +
        (size_t)x * 4u;
    for (channel = 0; channel < 3; ++channel) {
        float value = rgb[channel] * 255.0f;
        if (value < 0.0f) value = 0.0f;
        if (value > 255.0f) value = 255.0f;
        pixel[channel] = (uint8_t)value;
    }
    pixel[3] = 255u;
}

static void scene_ring_line(SceneRingTarget* target, int x0, int y0,
                            int x1, int y1, const float rgb[3])
{
    int dx = x1 >= x0 ? x1 - x0 : x0 - x1;
    int dy = y1 >= y0 ? y1 - y0 : y0 - y1;
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int error = dx - dy;
    for (;;) {
        scene_ring_pixel(target, x0, y0, rgb);
        if (x0 == x1 && y0 == y1) break;
        {
            int twice = error * 2;
            if (twice > -dy) { error -= dy; x0 += sx; }
            if (twice < dx) { error += dx; y0 += sy; }
        }
    }
}

static void scene_ring_rect_draw(const float rect[4], uint32_t color,
                                 void* context)
{
    SceneRingTarget* target = (SceneRingTarget*)context;
    int left, top, right, bottom, x, y;
    uint8_t red = (uint8_t)(color >> 16);
    uint8_t green = (uint8_t)(color >> 8);
    uint8_t blue = (uint8_t)color;
    if (target == NULL || rect == NULL) return;
    left = (int)rect[0]; top = (int)rect[1];
    right = (int)(rect[0] + rect[2]);
    bottom = (int)(rect[1] + rect[3]);
    for (y = top; y < bottom; ++y) {
        for (x = left; x < right; ++x) {
            float rgb[3];
            uint8_t* pixel;
            if (x < 0 || y < 0 || x >= target->width || y >= target->height)
                continue;
            pixel = (uint8_t*)target->pixels +
                (size_t)y * (size_t)target->pitch + (size_t)x * 4u;
            rgb[0] = (float)red / 255.0f;
            rgb[1] = (float)green / 255.0f;
            rgb[2] = (float)blue / 255.0f;
            scene_ring_pixel(target, x, y, rgb);
            pixel[3] = 255u;
        }
    }
}

static void scene_ring_strip_draw(const float first_xy[20],
                                  const float second_xy[20],
                                  uint32_t point_count,
                                  const float rgb[3], int mode,
                                  void* context)
{
    SceneRingTarget* target = (SceneRingTarget*)context;
    uint32_t i;
    (void)mode;
    if (first_xy == NULL || second_xy == NULL || rgb == NULL ||
        point_count > 10u) return;
    for (i = 0u; i < point_count; ++i) {
        int ax = (int)first_xy[i * 2u];
        int ay = (int)first_xy[i * 2u + 1u];
        int bx = (int)second_xy[i * 2u];
        int by = (int)second_xy[i * 2u + 1u];
        scene_ring_line(target, ax, ay, bx, by, rgb);
        if (i != 0u) {
            scene_ring_line(target, (int)first_xy[(i - 1u) * 2u],
                (int)first_xy[(i - 1u) * 2u + 1u], ax, ay, rgb);
            scene_ring_line(target, (int)second_xy[(i - 1u) * 2u],
                (int)second_xy[(i - 1u) * 2u + 1u], bx, by, rgb);
        }
    }
}

static float scene_min(float a, float b) { return a < b ? a : b; }
static float scene_max(float a, float b) { return a > b ? a : b; }
static float scene_sqrt(float v) { return (float)sqrt((double)v); }
static float scene_abs(float v) { return (float)fabs((double)v); }
static float scene_floor(float v) { return (float)floor((double)v); }
static float scene_ceil(float v) { return (float)ceil((double)v); }
static float scene_cos(float v) { return (float)cos((double)v); }
static float scene_sin(float v) { return (float)sin((double)v); }

static SceneVec3 scene_add(SceneVec3 a, SceneVec3 b)
{ return scene_vec3(a.x + b.x, a.y + b.y, a.z + b.z); }
static SceneVec3 scene_sub(SceneVec3 a, SceneVec3 b)
{ return scene_vec3(a.x - b.x, a.y - b.y, a.z - b.z); }
static SceneVec3 scene_scale(SceneVec3 a, float s)
{ return scene_vec3(a.x * s, a.y * s, a.z * s); }
static float scene_dot(SceneVec3 a, SceneVec3 b)
{ return a.x * b.x + a.y * b.y + a.z * b.z; }
static SceneVec3 scene_cross(SceneVec3 a, SceneVec3 b)
{ return scene_vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
                    a.x * b.y - a.y * b.x); }
static SceneVec3 scene_normalize(SceneVec3 a)
{
    float n = scene_sqrt(scene_dot(a, a));
    return n > 1e-5f ? scene_scale(a, 1.0f / n) : scene_vec3(0, 0, 1);
}

static SceneColor color_scale(SceneColor c, float scale)
{
    if (scale < 0.0f) scale = 0.0f;
    if (scale > 1.25f) scale = 1.25f;
    int r = (int)(c.r * scale), g = (int)(c.g * scale), b = (int)(c.b * scale);
    return scene_color((uint8_t)(r > 255 ? 255 : r),
                       (uint8_t)(g > 255 ? 255 : g),
                       (uint8_t)(b > 255 ? 255 : b));
}

static SceneColor terrain_color(const uint8_t* voxel,
    const LevelState* level, uint32_t x, uint32_t y)
{
    SceneColor c;
    switch (voxel != NULL ? voxel[0x02] : 0) {
    case 1:  c = scene_color(134, 83, 48); break;
    case 6:  c = scene_color(46, 156, 172); break;
    case 11: c = scene_color(202, 138, 37); break;
    default: c = scene_color(79, 67, 59); break;
    }
    unsigned surface = voxel != NULL ? voxel[0x04] : 0;
    if (s_scene_surface_display_level == level &&
        s_scene_surface_display_width == s_scene_vertex_width &&
        s_scene_surface_display_height == s_scene_vertex_height &&
        x < s_scene_surface_display_width && y < s_scene_surface_display_height)
        surface = s_scene_surface_display_markers[
            (size_t)y * s_scene_surface_display_width + x];
    return color_scale(c, 0.82f + (float)(surface & 7u) * 0.035f);
}

static SceneColor terrain_surface_color_override_get(
    const LevelState* level, uint32_t x, uint32_t y, SceneColor fallback)
{
    size_t index;
    float r, g, b;
    const float* colors;
    const uint8_t* valid;
    if (!scene_render_vertex_cache_ensure(level) ||
        x >= s_scene_vertex_width || y >= s_scene_vertex_height) return fallback;
    index = (size_t)y * (size_t)(s_scene_vertex_width - 1u) + x;
    if (index >= (size_t)s_scene_vertex_width * s_scene_vertex_height)
        return fallback;
    colors = s_scene_surface_colors;
    valid = s_scene_surface_color_valid;
    if (s_scene_surface_display_level == level &&
        s_scene_surface_display_width == s_scene_vertex_width &&
        s_scene_surface_display_height == s_scene_vertex_height) {
        colors = s_scene_surface_display_colors;
        valid = s_scene_surface_display_valid;
    }
    if (valid[index] == 0u) return fallback;
    r = colors[index * 3u];
    g = colors[index * 3u + 1u];
    b = colors[index * 3u + 2u];
    if (r < 0.0f) r = 0.0f;
    if (g < 0.0f) g = 0.0f;
    if (b < 0.0f) b = 0.0f;
    if (r > 1.0f) r = 1.0f;
    if (g > 1.0f) g = 1.0f;
    if (b > 1.0f) b = 1.0f;
    return scene_color((uint8_t)(r * 255.0f),
                       (uint8_t)(g * 255.0f),
                       (uint8_t)(b * 255.0f));
}

static int scene_project(const SceneCamera* c, SceneVec3 p, ScenePoint* out)
{
    SceneVec3 rel = scene_sub(p, c->eye);
    float z = scene_dot(rel, c->forward);
    if (z < 8.0f) return 0;
    out->x = c->cx + scene_dot(rel, c->right) * c->focal_x / z;
    out->y = c->cy - scene_dot(rel, c->up) * c->focal_y / z;
    out->z = z;
    return 1;
}

/* Drawing and picking reject the same whole triangle at the near plane. */
static int scene_triangle_project(const SceneCamera* c, SceneVec3 a,
                                  SceneVec3 b, SceneVec3 d, ScenePoint p[3])
{
    return scene_project(c, a, &p[0]) && scene_project(c, b, &p[1])
        && scene_project(c, d, &p[2]);
}

static void scene_triangle_sampled(SceneCamera* c, SceneVec3 a, SceneVec3 b,
                                  SceneVec3 d, SceneColor color,
                                  const TextureSurface* texture,
                                  float u0, float v0, float u1, float v1,
                                  float u2, float v2)
{
    ScenePoint p[3];
    if (!scene_triangle_project(c, a, b, d, p)) return;
    float area = (p[1].x - p[0].x) * (p[2].y - p[0].y)
               - (p[1].y - p[0].y) * (p[2].x - p[0].x);
    if (scene_abs(area) < 0.001f) return;
    int x0 = (int)scene_floor(scene_min(p[0].x, scene_min(p[1].x, p[2].x)));
    int x1 = (int)scene_ceil(scene_max(p[0].x, scene_max(p[1].x, p[2].x)));
    int y0 = (int)scene_floor(scene_min(p[0].y, scene_min(p[1].y, p[2].y)));
    int y1 = (int)scene_ceil(scene_max(p[0].y, scene_max(p[1].y, p[2].y)));
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= c->width) x1 = c->width - 1;
    if (y1 >= c->height) y1 = c->height - 1;
    uint8_t* base = (uint8_t*)c->pixels;
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            float px = (float)x + 0.5f, py = (float)y + 0.5f;
            float w0 = ((p[1].x - px) * (p[2].y - py)
                      - (p[1].y - py) * (p[2].x - px)) / area;
            float w1 = ((p[2].x - px) * (p[0].y - py)
                      - (p[2].y - py) * (p[0].x - px)) / area;
            float w2 = 1.0f - w0 - w1;
            if (w0 < -0.001f || w1 < -0.001f || w2 < -0.001f) continue;
            float z = w0 * p[0].z + w1 * p[1].z + w2 * p[2].z;
            size_t index = (size_t)y * (size_t)c->width + (size_t)x;
            if (z >= c->depth[index]) continue;
            c->depth[index] = z;
            uint8_t* q = base + (size_t)y * (size_t)c->pitch + (size_t)x * 4;
            if (texture != NULL && texture->pixels != NULL
                && texture->width > 0 && texture->height > 0) {
                float u = w0 * u0 + w1 * u1 + w2 * u2;
                float v = w0 * v0 + w1 * v1 + w2 * v2;
                int tx = (int)scene_floor(u * texture->width);
                int ty = (int)scene_floor(v * texture->height);
                tx %= texture->width; if (tx < 0) tx += texture->width;
                ty %= texture->height; if (ty < 0) ty += texture->height;
                uint32_t sample = texture->pixels[(size_t)ty
                    * (size_t)texture->width + (size_t)tx];
                q[0] = (uint8_t)(((sample >> 16) & 255u) * color.r / 255u);
                q[1] = (uint8_t)(((sample >> 8) & 255u) * color.g / 255u);
                q[2] = (uint8_t)((sample & 255u) * color.b / 255u);
                q[3] = 255;
            } else {
                q[0] = color.r; q[1] = color.g; q[2] = color.b; q[3] = 255;
            }
        }
    }
}

static void scene_triangle(SceneCamera* c, SceneVec3 a, SceneVec3 b,
                          SceneVec3 d, SceneColor color)
{
    scene_triangle_sampled(c, a, b, d, color, NULL,
                          0, 0, 0, 0, 0, 0);
}

static void scene_quad(SceneCamera* c, SceneVec3 a, SceneVec3 b,
                       SceneVec3 d, SceneVec3 e, SceneColor color)
{
    scene_triangle(c, a, b, d, color);
    scene_triangle(c, a, d, e, color);
}

static void scene_quad_textured(SceneCamera* c, SceneVec3 a, SceneVec3 b,
                               SceneVec3 d, SceneVec3 e,
                               const TextureSurface* texture,
                               float u, float v, SceneColor tint)
{
    scene_triangle_sampled(c, a, b, d, tint, texture,
                           u, v, u + 0.35f, v, u + 0.35f, v + 0.35f);
    scene_triangle_sampled(c, a, d, e, tint, texture,
                           u, v, u + 0.35f, v + 0.35f, u, v + 0.35f);
}

/* Barycentric interpolation in the exact top triangles submitted below.
 * Geometry Y is already signed/scaled; no byte-height conversion occurs here. */
static int scene_triangle_height(const float a[3], const float b[3],
                                 const float d[3], float x, float z,
                                 float* height)
{
    float denominator = (b[2] - d[2]) * (a[0] - d[0]) +
                        (d[0] - b[0]) * (a[2] - d[2]);
    float wa, wb, wd;
    if (denominator == 0.0f) return 0;
    wa = ((b[2] - d[2]) * (x - d[0]) +
          (d[0] - b[0]) * (z - d[2])) / denominator;
    wb = ((d[2] - a[2]) * (x - d[0]) +
          (a[0] - d[0]) * (z - d[2])) / denominator;
    wd = 1.0f - wa - wb;
    if (wa < -0.0001f || wb < -0.0001f || wd < -0.0001f) return 0;
    *height = wa * a[1] + wb * b[1] + wd * d[1];
    return 1;
}

static int scene_terrain_height_get(const LevelState* level, float grid_x,
                                   float grid_y, float* height)
{
    float block = terrain_blocksize_get(level->ptr_terrain);
    int x = (int)scene_floor(grid_x), y = (int)scene_floor(grid_y);
    float px, pz;
    static const int offsets[9][2] = {
        { 0, 0 }, { -1, -1 }, { 0, -1 }, { 1, -1 },
        { -1, 0 }, { 1, 0 }, { -1, 1 }, { 0, 1 }, { 1, 1 }
    };
    int candidate;
    if (block <= 0.0f) block = 40.0f;
    px = (grid_x - (float)level_width(level) * 0.5f) * block;
    pz = (grid_y - (float)level_height(level) * 0.5f) * block;
    /* Try the expected cell first, then adjoining displaced quads. */
    for (candidate = 0; candidate < 9; ++candidate) {
        float vertices[4][3];
        int cx = x + offsets[candidate][0];
        int cy = y + offsets[candidate][1];
        if (cx < 0 || cy < 0 ||
            !scene_render_terrain_cell_vertices_get(
                level, (uint32_t)cx, (uint32_t)cy, vertices)) continue;
        if (scene_triangle_height(vertices[0], vertices[1], vertices[2],
                                   px, pz, height) ||
            scene_triangle_height(vertices[0], vertices[2], vertices[3],
                                   px, pz, height)) return 1;
    }
    return 0;
}

/* Entity/scene poses are world coordinates. The software scene uses a
 * centered, positive-down grid plane; drawing, focus, rings and picking all
 * cross that boundary here rather than treating world values as cell indices. */
static int scene_entity_origin_get(const EntitySlot* entity,
                                    const LevelState* level,
                                    float block, SceneVec3* origin)
{
    float grid[2];
    if (!entity_position_grid_coordinates_get(entity, level, grid)) return 0;
    origin->x = (grid[0] - (float)level_width(level) * 0.5f) * block;
    origin->z = (grid[1] - (float)level_height(level) * 0.5f) * block;
    return scene_terrain_height_get(level, grid[0], grid[1], &origin->y);
}

static int scene_camera_configure(SceneCamera* c, const GameContext* ctx,
                                 const LevelState* lvl,
                                 int w, int h, int publish)
{
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    ViewContext* view = engine_current_primary_view_get();
    ViewCameraProjection projection;
    float center[2];
    float block = terrain_blocksize_get(lvl->ptr_terrain);
    (void)ctx;
    if (publish && !view_primary_projection_publish(view, lvl, w, h)) return 0;
    if (!view_primary_projection_get(view, lvl, w, h, &projection) ||
        block <= 0.0f || !terrain_cell_world_center_xy_get(
            lvl->ptr_terrain, 0, 0, center)) return 0;
    memset(c, 0, sizeof(*c));
    /* Same world->grid->presentation map as scene_entity_origin_get.
     * Directions receive the axis permutation only; geometry Y keeps its
     * signed/scaled value. The eye/basis come from the actual RM frames. */
    c->eye = scene_vec3(projection.pose.eye[0] - center[0] + block * 0.5f -
        (float)level_width(lvl) * block * 0.5f, projection.pose.eye[2],
        center[1] - projection.pose.eye[1] + block * 0.5f -
        (float)level_height(lvl) * block * 0.5f);
    c->forward = scene_normalize(scene_vec3(projection.pose.forward[0],
        projection.pose.forward[2], -projection.pose.forward[1]));
    c->up = scene_normalize(scene_vec3(projection.pose.up[0],
        projection.pose.up[2], -projection.pose.up[1]));
    /* RM's left-handed frame has right = up cross forward. */
    c->right = scene_normalize(scene_cross(c->up, c->forward));
    c->up = scene_normalize(scene_cross(c->forward, c->right));
    c->focal_x = projection.focal_x;
    c->focal_y = projection.focal_y;
    c->cx = projection.center_x;
    c->cy = projection.center_y;
    c->width = w;
    c->height = h;
    return 1;
#else
    /* The unrelated native renderer retains its existing camera path. */
    (void)publish;
    int gw = level_width(lvl), gh = level_height(lvl);
    float block = terrain_blocksize_get(lvl->ptr_terrain);
    if (block <= 0.0f) block = 40.0f;
    float hs = terrain_height_scale_get(lvl->ptr_terrain);
    float world_w = (float)gw * block, world_d = (float)gh * block;
    if (s_scene_focus_level != lvl) {
        s_scene_focus_level = lvl;
        s_scene_focus = scene_vec3(0.0f, -8.0f * hs, 0.0f);
        SceneVec3 sum = scene_vec3(0.0f, 0.0f, 0.0f);
        int visible = 0;
        int figures = ctx != NULL
            ? entity_count((GameContext*)ctx, ENTITY_TYPE_MINIFIG) : 0;
        for (int i = 0; i < figures; ++i) {
            const EntitySlot* e = entity_at((GameContext*)ctx,
                                            ENTITY_TYPE_MINIFIG, i);
            SceneVec3 origin;
            if (!scene_entity_origin_get(e, lvl, block, &origin)) continue;
            sum = scene_add(sum, origin);
            ++visible;
        }
        if (visible > 0) {
            s_scene_focus = scene_scale(sum, 1.0f / (float)visible);
            s_scene_focus.y -= block * 0.15f;
        }
    }
    SceneVec3 target = s_scene_focus;
    SceneVec3 eye = {target.x + world_w * 0.14f,
                     target.y + world_d * 0.20f,
                     target.z - world_d * 0.28f};
    memset(c, 0, sizeof(*c));
    c->eye = eye;
    c->forward = scene_normalize(scene_sub(target, eye));
    c->right = scene_normalize(scene_cross(c->forward, scene_vec3(0,1,0)));
    c->up = scene_normalize(scene_cross(c->right, c->forward));
    c->focal_x = c->focal_y = (float)h * 1.02f;
    c->cx = (float)w * 0.5f;
    c->cy = (float)h * 0.53f;
    c->width = w;
    c->height = h;
    return block > 0.0f && gw > 0 && gh > 0;
#endif
}

int scene_render_displayed_world_point_project(const LevelState* level,
    const GameContext* game, int width, int height,
    unsigned int displayed_generation, const float world[3], float screen[2])
{
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    ViewCameraProjection displayed;
    SceneCamera camera;
    ScenePoint projected;
    SceneVec3 point;
    float center[2];
    float block;
    if (level == NULL || level->ptr_terrain == NULL || game == NULL ||
        world == NULL || screen == NULL || displayed_generation == 0u ||
        !view_primary_projection_get(engine_current_primary_view_get(),
            level, width, height, &displayed) ||
        displayed.generation != displayed_generation ||
        !scene_camera_configure(&camera, game, level, width, height, 0)) return 0;
    block = terrain_blocksize_get(level->ptr_terrain);
    if (block <= 0.0f || !terrain_cell_world_center_xy_get(
            level->ptr_terrain, 0, 0, center)) return 0;
    /* Identical world->presentation mapping to camera v2's eye conversion.
     * Vertical value already is signed/scaled live terrain height. */
    point = scene_vec3(world[0] - center[0] + block * 0.5f -
        (float)level_width(level) * block * 0.5f, world[2],
        center[1] - world[1] + block * 0.5f -
        (float)level_height(level) * block * 0.5f);
    if (!scene_project(&camera, point, &projected)) return 0;
    screen[0] = projected.x;
    screen[1] = projected.y;
    return 1;
#else
    (void)level; (void)game; (void)width; (void)height;
    (void)displayed_generation; (void)world; (void)screen;
    return 0;
#endif
}

static void render_terrain(SceneCamera* c, const LevelState* lvl,
                           float block, float hs,
                           const TextureSurface* texture)
{
    const uint8_t* vox = (const uint8_t*)level_load_ptr(lvl, LEVEL_OFF_VOXELS);
    int w = level_width(lvl), h = level_height(lvl);
    /* Give the map's exposed boundary a raised cave wall instead of leaving
     * the player looking out into empty space. The 3D render targets a camera
     * view of this authored terrain; this sentinel is only a visual shell. */
    const float outside = -30.0f;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float z = (float)terrain_surface_height_get(lvl->ptr_terrain, x, y);
            float vertices[4][3];
            SceneVec3 a, b, d, e;
            if (!scene_render_terrain_cell_vertices_canonical_get(
                    lvl, (uint32_t)x, (uint32_t)y, vertices)) continue;
            a = scene_vec3(vertices[0][0], vertices[0][1], vertices[0][2]);
            b = scene_vec3(vertices[1][0], vertices[1][1], vertices[1][2]);
            d = scene_vec3(vertices[2][0], vertices[2][1], vertices[2][2]);
            e = scene_vec3(vertices[3][0], vertices[3][1], vertices[3][2]);
            SceneColor top = terrain_color(vox != NULL
                ? vox + ((size_t)y * (size_t)w + (size_t)x) * LEVEL_VOXEL_STRIDE
                : NULL, lvl, (uint32_t)x, (uint32_t)y);
            top = terrain_surface_color_override_get(lvl, (uint32_t)x,
                                                      (uint32_t)y, top);
            /* Rotate complete XYZ corners to select the other diagonal.
             * Wall edges below retain canonical map adjacency. */
            int flipped = s_scene_vertex_flipped != NULL &&
                s_scene_vertex_level == lvl &&
                s_scene_vertex_flipped[(size_t)y * s_scene_vertex_width + x];
            flipped = !flipped; /* native base 0,1,3 / 1,2,3 */
            if (texture != NULL) {
                float u = (float)x * 0.35f, v = (float)y * 0.35f;
                if (flipped) {
                    scene_triangle_sampled(c, e, a, b, top, texture,
                        u, v + 0.35f, u, v, u + 0.35f, v);
                    scene_triangle_sampled(c, e, b, d, top, texture,
                        u, v + 0.35f, u + 0.35f, v,
                        u + 0.35f, v + 0.35f);
                }
                else scene_quad_textured(c, a, b, d, e, texture, u, v, top);
            }
            else if (flipped) scene_quad(c, e, a, b, d, top);
            else scene_quad(c, a, b, d, e, top);

            const int dx[4] = {0, 1, 0, -1};
            const int dy[4] = {-1, 0, 1, 0};
            SceneVec3 edge_a[4], edge_b[4];
            edge_a[0] = a; edge_a[1] = b;
            edge_a[2] = d; edge_a[3] = e;
            edge_b[0] = b; edge_b[1] = d;
            edge_b[2] = e; edge_b[3] = a;
            for (int side = 0; side < 4; ++side) {
                int nx = x + dx[side], ny = y + dy[side];
                float nh = (nx < 0 || ny < 0 || nx >= w || ny >= h)
                         ? outside
                         : (float)terrain_surface_height_get(lvl->ptr_terrain,
                                                             nx, ny);
                if (scene_abs(nh - z) < 0.5f) continue;
                SceneVec3 lo_a = edge_a[side], lo_b = edge_b[side];
                lo_a.y = lo_b.y = -nh * hs;
                float light = side == 0 ? 0.62f : (side == 1 ? 0.74f : 0.55f);
                if (texture != NULL) {
                    float u = (float)(side % 2 == 0 ? x : y) * 0.35f;
                    float v = (float)(side % 2 == 0 ? y : x) * 0.35f;
                    scene_quad_textured(c, edge_a[side], edge_b[side],
                                        lo_b, lo_a, texture, u, v,
                                        color_scale(top, light));
                } else {
                    scene_quad(c, edge_a[side], edge_b[side], lo_b, lo_a,
                               color_scale(top, light));
                }
            }
        }
    }
}

/* Roof output has its own native parent/visibility path. It never replaces
 * ground heights. Admission follows the actual visual-frame parent, whose
 * alternate-root state was observed in retail; no wall/depth override. */
static void scene_roof(SceneCamera* c, const LevelState* level)
{
    static const uint8_t clear_order[4] = { 3u, 2u, 1u, 0u };
    static const uint8_t set_order[4] = { 0u, 3u, 2u, 1u };
    float center[2];
    float block;
    int x, y;
    if (terrain_roof_visibility_get(level) != 1 ||
        !terrain_cell_world_center_xy_get(level->ptr_terrain, 0, 0, center))
        return;
    block = terrain_blocksize_get(level->ptr_terrain);
    for (y = 0; y + 1 < level_height(level); ++y) {
        for (x = 0; x + 1 < level_width(level); ++x) {
            float canonical[12];
            SceneVec3 vertices[4];
            int code = terrain_cell_roof_vertices_get(level,
                (uint32_t)x, (uint32_t)y, canonical);
            const uint8_t* order = code == 2 ? set_order : clear_order;
            uint32_t corner;
            if (code == 0) continue;
            for (corner = 0u; corner < 4u; ++corner) {
                const float* source = canonical + order[corner] * 3u;
                vertices[corner] = scene_vec3(
                    source[0] - center[0] + block * 0.5f -
                        (float)level_width(level) * block * 0.5f,
                    source[1], center[1] - source[2] + block * 0.5f -
                        (float)level_height(level) * block * 0.5f);
            }
            scene_triangle(c, vertices[0], vertices[1], vertices[3],
                            scene_color(78, 65, 48));
            scene_triangle(c, vertices[1], vertices[2], vertices[3],
                            scene_color(78, 65, 48));
        }
    }
}

/* The RM camera/terrain boundary retains native world height: up is -Z,
 * represented by software Y. Local primitive heights stay positive-up; apply
 * the same basis to drawing, picking and projected feedback. */
static float scene_figure_height_offset(float local_up)
{
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    return -local_up;
#else
    return local_up;
#endif
}

static SceneVec3 figure_point(SceneVec3 origin, float x, float y, float z,
                              float yaw)
{
    float cs = scene_cos(yaw), sn = scene_sin(yaw);
    return scene_vec3(origin.x + x * cs + z * sn, origin.y + scene_figure_height_offset(y),
                      origin.z - x * sn + z * cs);
}

static void figure_box(SceneCamera* c, SceneVec3 origin, float yaw,
                       float x, float y, float z, float sx, float sy, float sz,
                       SceneColor color)
{
    float hx = sx * 0.5f, hy = sy * 0.5f, hz = sz * 0.5f;
    SceneVec3 v[8];
    v[0] = figure_point(origin,x-hx,y-hy,z-hz,yaw);
    v[1] = figure_point(origin,x+hx,y-hy,z-hz,yaw);
    v[2] = figure_point(origin,x+hx,y+hy,z-hz,yaw);
    v[3] = figure_point(origin,x-hx,y+hy,z-hz,yaw);
    v[4] = figure_point(origin,x-hx,y-hy,z+hz,yaw);
    v[5] = figure_point(origin,x+hx,y-hy,z+hz,yaw);
    v[6] = figure_point(origin,x+hx,y+hy,z+hz,yaw);
    v[7] = figure_point(origin,x-hx,y+hy,z+hz,yaw);
    static const uint8_t faces[6][4] = {
        {0,1,2,3}, {5,4,7,6}, {4,0,3,7},
        {1,5,6,2}, {3,2,6,7}, {4,5,1,0}
    };
    static const float shade[6] = {0.72f, 1.0f, 0.58f, 0.80f, 1.12f, 0.52f};
    for (int f = 0; f < 6; ++f) {
        const uint8_t* q = faces[f];
        SceneColor face = color_scale(color, shade[f]);
        scene_quad(c, v[q[0]], v[q[1]], v[q[2]], v[q[3]], face);
    }
}

static void render_raider(SceneCamera* c, const EntitySlot* e,
                          float block, SceneVec3 origin, int selected)
{
    if (e == NULL) return;
    float yaw = e->rot; /* OList and entity pose headings are radians. */
    float u = block * 0.22f;
    /* Boots and legs. */
    figure_box(c,origin,yaw,-u,block*0.18f,0,u*0.82f,block*0.32f,u,
               scene_color(89,75,61));
    figure_box(c,origin,yaw, u,block*0.18f,0,u*0.82f,block*0.32f,u,
               scene_color(104,88,70));
    figure_box(c,origin,yaw,0,block*0.43f,0,block*0.49f,block*0.26f,
               block*0.30f,scene_color(202,83,38));
    /* Arms, hi-vis torso, yellow head, and green hard hat. */
    figure_box(c,origin,yaw,-block*0.34f,block*0.54f,0,block*0.17f,
               block*0.38f,block*0.20f,scene_color(17,105,91));
    figure_box(c,origin,yaw, block*0.34f,block*0.54f,0,block*0.17f,
               block*0.38f,block*0.20f,scene_color(17,105,91));
    figure_box(c,origin,yaw,0,block*0.78f,0,block*0.36f,block*0.28f,
               block*0.34f,scene_color(222,185,27));
    figure_box(c,origin,yaw,0,block*0.94f,0,block*0.40f,block*0.10f,
               block*0.38f,scene_color(86,171,29));
    figure_box(c,origin,yaw,0,block*0.99f,0,block*0.25f,block*0.11f,
               block*0.26f,scene_color(107,190,35));
    /* The face plate and dark eye/mouth marks sit on the front of the head. */
    figure_box(c,origin,yaw,-block*0.07f,block*0.80f,-block*0.177f,
               block*0.035f,block*0.035f,block*0.018f,scene_color(33,30,22));
    figure_box(c,origin,yaw, block*0.07f,block*0.80f,-block*0.177f,
               block*0.035f,block*0.035f,block*0.018f,scene_color(33,30,22));
    figure_box(c,origin,yaw,0,block*0.73f,-block*0.178f,block*0.12f,
               block*0.025f,block*0.016f,scene_color(48,35,19));
    if (selected) {
        SceneColor marker = {235, 208, 48};
        float span = block * 0.40f, rail = block * 0.045f;
        figure_box(c,origin,yaw,0,block*0.09f,-span,span*2,rail,rail,marker);
        figure_box(c,origin,yaw,0,block*0.09f, span,span*2,rail,rail,marker);
        figure_box(c,origin,yaw,-span,block*0.09f,0,rail,rail,span*2,marker);
        figure_box(c,origin,yaw, span,block*0.09f,0,rail,rail,span*2,marker);
        /* A raised pip stays readable on uneven terrain even when the
         * ground-level selection brackets overlap a cliff face. */
        figure_box(c,origin,yaw,0,block*1.22f,0,block*0.20f,
                   block*0.12f,block*0.20f,marker);
    }
}

static void scene_displayed_projection_invalidate(void)
{
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    view_primary_projection_invalidate(engine_current_primary_view_get());
#endif
}

static void scene_render_map_replacement(const LevelState* lvl,
    const GameContext* ctx, int w, int h, void* pixels, int pitch)
{
    if (lvl != NULL && ctx != NULL && pixels != NULL && w > 0 && h > 0 &&
        level_width(lvl) > 0 && level_height(lvl) > 0)
        scene_displayed_projection_invalidate();
    level_render_map(lvl, ctx, -1, -1, w, h, pixels, pitch);
}

void level_render_scene(const LevelState* lvl, const GameContext* ctx,
                        int w, int h, void* pixels, int pitch)
{
    if (lvl == NULL || ctx == NULL || pixels == NULL || w <= 0 || h <= 0
        || lvl->ptr_terrain == NULL) {
        if (lvl != NULL && ctx != NULL) {
            scene_render_map_replacement(lvl, ctx, w, h, pixels, pitch);
        }
        return;
    }
    if (ctx->reward_active) {
        scene_render_map_replacement(lvl, ctx, w, h, pixels, pitch);
        return;
    }
    size_t count = (size_t)w * (size_t)h;
    if (count > s_scene_depth_capacity) {
        float* next = (float*)realloc(s_scene_depth, count * sizeof(float));
        if (next == NULL) {
            scene_render_map_replacement(lvl, ctx, w, h, pixels, pitch);
            return;
        }
        s_scene_depth = next;
        s_scene_depth_capacity = count;
    }
    /* This call replaces the framebuffer. Simulation-only updates and a
     * failed surface lock never enter here and keep the displayed snapshot. */
    scene_displayed_projection_invalidate();
    uint8_t* out = (uint8_t*)pixels;
    for (int y = 0; y < h; ++y) {
        uint8_t* row = out + (size_t)y * (size_t)pitch;
        for (int x = 0; x < w; ++x) {
            float t = h > 1 ? (float)y / (float)(h - 1) : 0.0f;
            row[x*4+0] = (uint8_t)(8 + t * 17);
            row[x*4+1] = (uint8_t)(11 + t * 14);
            row[x*4+2] = (uint8_t)(18 + t * 12);
            row[x*4+3] = 255;
        }
    }
    for (size_t i = 0; i < count; ++i) s_scene_depth[i] = 1.0e30f;

    float block = terrain_blocksize_get(lvl->ptr_terrain);
    if (block <= 0.0f) block = 40.0f;
    float hs = terrain_height_scale_get(lvl->ptr_terrain);
    SceneCamera camera;
    if (!scene_camera_configure(&camera, ctx, lvl, w, h, 1)) {
        scene_displayed_projection_invalidate();
        return;
    }
    camera.pixels = pixels;
    camera.pitch = pitch; camera.depth = s_scene_depth;

    if (s_scene_floor_texture == NULL) {
        s_scene_floor_texture = texture_get_or_load(
            NULL, "World\\WorldTextures\\LavaSplit\\LAVA00.BMP");
    }
    render_terrain(&camera, lvl, block, hs, s_scene_floor_texture);
    scene_roof(&camera, lvl);
    for (int type = ENTITY_TYPE_VEHICLE; type <= ENTITY_TYPE_BUILDING; ++type) {
        for (int i = 0; i < entity_count((GameContext*)ctx, type); ++i) {
            const EntitySlot* e = entity_at((GameContext*)ctx, type, i);
            if (e == NULL) continue;
            SceneVec3 origin;
            if (!scene_entity_origin_get(e, lvl, block, &origin)) continue;
            if (type == ENTITY_TYPE_MINIFIG) {
                render_raider(&camera, e, block, origin,
                              selection_contains_entity_target(ctx, e, NULL));
            } else if (type == ENTITY_TYPE_BUILDING) {
                /* A blocky Tool Store silhouette until the .ae scene loader
                 * supplies its authored object assembly. */
                figure_box(&camera,origin,e->rot,0,block*0.42f,
                           0,block*0.80f,block*0.84f,block*0.72f,
                           scene_color(91,83,72));
                figure_box(&camera,origin,e->rot,0,block*0.90f,
                           0,block*0.58f,block*0.14f,block*0.52f,
                           scene_color(181,145,75));
            } else {
                figure_box(&camera,origin,e->rot,0,block*0.25f,
                           0,block*0.48f,block*0.45f,block*0.38f,
                           scene_color(140,138,129));
            }
        }
    }

    /* TickEntityScreenEffects walks the 20-slot flagged-entity registry and
     * sends each eligible entity through RenderEntityScreenRing. The native
     * renderer owns projection/framebuffer access here, so submit that same
     * registry from the active scene pass instead of trying to draw while
     * EngineTick holds no locked pixels. */
    {
        LegacyHudOps ring_ops;
        SceneRingTarget ring_target;
        int slot_index;
        memset(&ring_ops, 0, sizeof(ring_ops));
        ring_target.pixels = pixels;
        ring_target.pitch = pitch;
        ring_target.width = w;
        ring_target.height = h;
        ring_ops.context = &ring_target;
        ring_ops.rect_draw = scene_ring_rect_draw;
        ring_ops.ring_strip_draw = scene_ring_strip_draw;
        for (slot_index = 0; slot_index < 20; ++slot_index) {
            void* registered_entity = NULL;
            EntitySlot* entity;
            SceneVec3 marker;
            ScenePoint projected;
            ai_object_registry_slot_peek(0, slot_index, &registered_entity,
                                         NULL);
            entity = (EntitySlot*)registered_entity;
            if (entity == NULL) continue;
            if (current_engine_mode_is_zero() ||
                (entity->type == ENTITY_TYPE_MINIFIG &&
                 global_task_refresh_flag_get() != 0u) ||
                (entity->flags_3e0 & 0x00600000u) != 0u) continue;
            if (!scene_entity_origin_get(entity, lvl, block, &marker))
                continue;
            marker.y += scene_figure_height_offset(block * 0.8f);
            if (!scene_project(&camera, marker, &projected)) continue;
            legacy_entity_screen_ring_render_with_ops(
                entity, projected.x, projected.y, &ring_ops);
        }
    }
}

int level_scene_mouse_cell(const GameContext* ctx, const LevelState* lvl,
                           int w, int h,
                           int mouse_x, int mouse_y,
                           float* cell_x, float* cell_y)
{
    if (lvl == NULL || lvl->ptr_terrain == NULL || w <= 0 || h <= 0
        || cell_x == NULL || cell_y == NULL) return 0;
    SceneCamera c;
    if (!scene_camera_configure(&c, ctx, lvl, w, h, 0)) return 0;
    SceneVec3 ray = scene_normalize(scene_add(
        c.forward,
        scene_add(scene_scale(c.right, ((float)mouse_x - c.cx) / c.focal_x),
                  scene_scale(c.up, (c.cy - (float)mouse_y) / c.focal_y))));
    int gw = level_width(lvl), gh = level_height(lvl);
    float block = terrain_blocksize_get(lvl->ptr_terrain);
    float nearest = 1.0e30f;
    SceneVec3 closest = scene_vec3(0.0f, 0.0f, 0.0f);
    int x, y, triangle;
    if (block <= 0.0f) block = 40.0f;
    for (y = 0; y < gh; ++y) {
        for (x = 0; x < gw; ++x) {
            float vertices[4][3];
            if (!scene_render_terrain_cell_vertices_get(
                    lvl, (uint32_t)x, (uint32_t)y, vertices)) continue;
            for (triangle = 0; triangle < 2; ++triangle) {
                const float* va = vertices[0];
                const float* vb = vertices[triangle == 0 ? 1 : 2];
                const float* vd = vertices[triangle == 0 ? 2 : 3];
                SceneVec3 a = scene_vec3(va[0], va[1], va[2]);
                SceneVec3 b = scene_vec3(vb[0], vb[1], vb[2]);
                SceneVec3 d = scene_vec3(vd[0], vd[1], vd[2]);
                SceneVec3 normal = scene_cross(scene_sub(b, a), scene_sub(d, a));
                float denominator = scene_dot(normal, ray);
                float t, height;
                SceneVec3 hit;
                ScenePoint projected[3];
                if (!scene_triangle_project(&c, a, b, d, projected)) continue;
                if (scene_abs(denominator) < 0.0001f) continue;
                t = scene_dot(normal, scene_sub(a, c.eye)) / denominator;
                if (t <= 0.0f || t >= nearest) continue;
                hit = scene_add(c.eye, scene_scale(ray, t));
                if (!scene_triangle_height(va, vb, vd, hit.x, hit.z, &height))
                    continue;
                nearest = t;
                closest = hit;
            }
        }
    }
    if (nearest == 1.0e30f) return 0;
    *cell_x = closest.x / block + (float)gw * 0.5f;
    *cell_y = closest.z / block + (float)gh * 0.5f;
    return *cell_x >= 0.0f && *cell_y >= 0.0f
        && *cell_x < (float)gw && *cell_y < (float)gh;
}

EntitySlot* level_scene_pick_entity(GameContext* ctx, const LevelState* lvl,
                                    int w, int h, int mouse_x, int mouse_y,
                                    float radius_pixels)
{
    if (ctx == NULL || lvl == NULL || lvl->ptr_terrain == NULL) return NULL;
    SceneCamera c;
    if (!scene_camera_configure(&c, ctx, lvl, w, h, 0)) return NULL;
    float block = terrain_blocksize_get(lvl->ptr_terrain);
    float best = radius_pixels * radius_pixels;
    EntitySlot* picked = NULL;
    for (int i = 0; i < entity_count(ctx, ENTITY_TYPE_MINIFIG); ++i) {
            const EntitySlot* e = entity_at(ctx, ENTITY_TYPE_MINIFIG, i);
            if (e == NULL) continue;
            SceneVec3 p;
            if (!scene_entity_origin_get(e, lvl, block, &p)) continue;
            p.y += scene_figure_height_offset(block * 0.48f);
            ScenePoint screen;
            if (!scene_project(&c, p, &screen)) continue;
            float dx = screen.x - (float)mouse_x;
            float dy = screen.y - (float)mouse_y;
            float distance = dx * dx + dy * dy;
            if (distance < best) {
                best = distance;
                picked = (EntitySlot*)e;
            }
    }
    return picked;
}

// FUNCTION: LEGORR 0x00433770
int scene_lights_build_from_config(const SceneLightBuildOps* ops)
{
    SceneLightDefinition definition;
    void* top_spot;
    void* track_spot;
    void* ambient;
    void* first_person;
    void* rotated_first_person = NULL;
    void* red_auxiliary;
    void* root;
    void* default_owner;
    void* default_light;
    float pose[6];
    float position[3];
    float rgb[3];
    float scale[3];

    if (ops == NULL || ops->rgb_read == NULL || ops->owner_get == NULL ||
        ops->light_create == NULL || ops->light_parameter_set == NULL ||
        ops->light_position_set == NULL || ops->light_pose_set == NULL ||
        ops->light_enable == NULL || ops->scene_root_get == NULL ||
        ops->scene_object_allocate == NULL ||
        ops->scene_object_transform_init == NULL) {
        return 0;
    }

    /* TopSpotRGB: config lookup and light creation are required. */
    definition.config_name = "topspotrgb";
    definition.owner_slot = SCENE_LIGHT_OWNER_TOP_SPOT;
    definition.owner = ops->owner_get(definition.owner_slot, ops->user);
    definition.light_kind = 2;
    if (!ops->rgb_read(definition.config_name, definition.rgb, ops->user))
        return 0;
    top_spot = ops->light_create(&definition, ops->user);
    if (top_spot == NULL) return 0;
    (void)ops->light_parameter_set(top_spot, 0x40u, 0.85f, ops->user);
    (void)ops->light_parameter_set(top_spot, 0x3cu, 0.4f, ops->user);
    position[0] = 200.0f; position[1] = 140.0f; position[2] = -130.0f;
    (void)ops->light_position_set(top_spot, definition.owner, position,
                                  ops->user);
    pose[0] = -1.0f; pose[1] = -0.8f; pose[2] = 0.75f;
    pose[3] = 0.0f; pose[4] = 1.0f; pose[5] = 0.0f;
    (void)ops->light_pose_set(top_spot, definition.owner, pose, ops->user);

    /* TrackSpotRGB: required config/light, then position and pose. */
    definition.config_name = "trackspotrgb";
    definition.owner_slot = SCENE_LIGHT_OWNER_TRACK_SPOT;
    definition.owner = ops->owner_get(definition.owner_slot, ops->user);
    definition.light_kind = 2;
    if (!ops->rgb_read(definition.config_name, definition.rgb, ops->user))
        return 0;
    track_spot = ops->light_create(&definition, ops->user);
    if (track_spot == NULL) return 0;
    position[0] = 0.0f; position[1] = 0.0f; position[2] = -150.0f;
    (void)ops->light_position_set(track_spot, NULL, position, ops->user);
    pose[0] = 0.0f; pose[1] = 0.0f; pose[2] = 1.0f;
    pose[3] = 0.0f; pose[4] = 1.0f; pose[5] = 0.0f;
    (void)ops->light_pose_set(track_spot, NULL, pose, ops->user);

    /* AmbientRGB and FPLightRGB are required, with no post-create setters. */
    definition.config_name = "ambientrgb";
    definition.owner_slot = SCENE_LIGHT_OWNER_AMBIENT;
    definition.owner = ops->owner_get(definition.owner_slot, ops->user);
    definition.light_kind = 0;
    if (!ops->rgb_read(definition.config_name, definition.rgb, ops->user))
        return 0;
    ambient = ops->light_create(&definition, ops->user);
    if (ambient == NULL) return 0;

    definition.config_name = "fplightrgb";
    definition.owner_slot = SCENE_LIGHT_OWNER_FIRST_PERSON;
    definition.owner = ops->owner_get(definition.owner_slot, ops->user);
    definition.light_kind = 1;
    if (!ops->rgb_read(definition.config_name, definition.rgb, ops->user))
        return 0;
    first_person = ops->light_create(&definition, ops->user);
    if (first_person == NULL) return 0;

    /* FPRotLightRGB is the only optional config light. */
    definition.config_name = "fprotlightrgb";
    definition.light_kind = 3;
    if (ops->rgb_read(definition.config_name, definition.rgb, ops->user))
        rotated_first_person = ops->light_create(&definition, ops->user);

    /* The auxiliary red type-3 light is attempted regardless of the
     * optional rotated light and is not a required-success gate in retail. */
    definition.config_name = NULL;
    definition.rgb[0] = 1.0f;
    definition.rgb[1] = 0.0f;
    definition.rgb[2] = 0.0f;
    red_auxiliary = ops->light_create(&definition, ops->user);

    /* Retail creates a scene object under the current root, then places a
     * default white type-2 light on that new object. */
    root = ops->scene_root_get(ops->user);
    default_owner = ops->scene_object_allocate(root, ops->user);
    definition.config_name = NULL;
    definition.owner_slot = SCENE_LIGHT_OWNER_DEFAULT_ROOT;
    definition.owner = default_owner;
    definition.light_kind = 2;
    definition.rgb[0] = 1.0f;
    definition.rgb[1] = 1.0f;
    definition.rgb[2] = 1.0f;
    default_light = ops->light_create(&definition, ops->user);
    if (default_light == NULL) return 0;
    scale[0] = 0.8f; scale[1] = 0.8f; scale[2] = 0.8f;
    (void)ops->scene_object_transform_init(default_light, root, scale,
                                           ops->user);
    pose[0] = 0.0f; pose[1] = 0.0f; pose[2] = 1.0f;
    pose[3] = 0.0f; pose[4] = 1.0f; pose[5] = 0.0f;
    (void)ops->light_pose_set(default_light, NULL, pose, ops->user);
    (void)ops->light_parameter_set(default_light, 0x40u, 0.85f, ops->user);
    (void)ops->light_parameter_set(default_light, 0x3cu, 0.0f, ops->user);

    /* Retail enables in this order, then assigns the three far ranges. */
    (void)ops->light_enable(first_person, 1, ops->user);
    if (rotated_first_person != NULL)
        (void)ops->light_enable(rotated_first_person, 1, ops->user);
    (void)ops->light_enable(top_spot, 1, ops->user);
    (void)ops->light_enable(track_spot, 1, ops->user);
    (void)ops->light_enable(ambient, 1, ops->user);
    (void)ops->light_enable(default_light, 1, ops->user);
    (void)ops->light_enable(red_auxiliary, 1, ops->user);
    (void)ops->light_parameter_set(top_spot, 0x38u, 2000.0f, ops->user);
    (void)ops->light_parameter_set(track_spot, 0x38u, 2000.0f, ops->user);
    (void)ops->light_parameter_set(default_light, 0x38u, 2000.0f, ops->user);
    return 1;
}

namespace unported_ledger_stubs {

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

// SceneQuadGridEmit
// FUNCTION: LEGORR 0x00463f00
SceneQuadGridRecord* scene_quad_grid_emit(
    void* scene, uint32_t parameter, const SceneQuadGridEmitHooks* hooks)
{
    static const uint32_t base_indices[6] = { 0u, 1u, 3u, 1u, 2u, 3u };
    uint32_t indices[30];
    uint16_t index_words[60];
    SceneQuadGridRecord* record;
    uint32_t face;
    if (hooks == NULL || hooks->mesh_create == NULL ||
        hooks->face_vertices_emit == NULL ||
        hooks->face_texture_bind_by_mode == NULL)
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
        /* FUN_00463f00 appends a 20-vertex record for each face group before
         * filling that group's five quads. The retail index pointer is a
         * dword table; the in-memory pool adapter consumes the low word of
         * each dword through its two-word source stride. */
        for (index = 0u; index < 30u; ++index) {
            index_words[index * 2u] = (uint16_t)indices[index];
            index_words[index * 2u + 1u] = 0u;
        }
        (void)mesh_record_allocate_or_append(record->mesh, 20u, 10u, 3u,
                                             index_words);
        for (quad = 0; quad < 5u; ++quad) {
            float vertices[4][9];
            scene_quad_grid_vertices_build(quad, vertices);
            (void)hooks->face_vertices_emit(hooks->context, record->mesh,
                                            face, quad, vertices);
        }
        hooks->face_texture_bind_by_mode(hooks->context, record->mesh,
                                         face, 3u);
    }
    scene_render_lock_toggle(record->mesh, 1);
    return record;
}

SceneQuadGridRecord* ledger_stub_00463f00_SceneQuadGridEmit(
    void* scene, unsigned int parameter, const SceneQuadGridEmitHooks* hooks)
{
    return unported_ledger_stubs::scene_quad_grid_emit(scene, parameter,
                                                        hooks);
}


} // namespace unported_ledger_stubs
