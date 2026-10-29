// SPDX-License-Identifier: LGPL-3.0-or-later
/* Initial perspective scene renderer for the native game path.
 *
 * The terrain SurfaceMap is the authored heightfield; the erosion/surface
 * voxel layers select the visible material. This renderer turns those data
 * into depth-tested terrain faces and renders the placed Pilot entities as
 * simple faceted figures. It is deliberately isolated from the simulation:
 * callers provide the current level/entities and a locked RGBA framebuffer.
 */
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "entity.hpp"
#include "level.hpp"
#include "render.hpp"
#include "select.hpp"
#include "texture.hpp"

typedef struct SceneVec3 { float x, y, z; } SceneVec3;
typedef struct ScenePoint { float x, y, z; } ScenePoint;
typedef struct SceneColor { uint8_t r, g, b; } SceneColor;
typedef struct SceneCamera {
    SceneVec3 eye, right, up, forward;
    float focal, cx, cy;
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

static SceneColor terrain_color(const uint8_t* voxel)
{
    SceneColor c;
    switch (voxel != NULL ? voxel[0x02] : 0) {
    case 1:  c = scene_color(134, 83, 48); break;
    case 6:  c = scene_color(46, 156, 172); break;
    case 11: c = scene_color(202, 138, 37); break;
    default: c = scene_color(79, 67, 59); break;
    }
    unsigned surface = voxel != NULL ? voxel[0x04] : 0;
    return color_scale(c, 0.82f + (float)(surface & 7u) * 0.035f);
}

static int scene_project(const SceneCamera* c, SceneVec3 p, ScenePoint* out)
{
    SceneVec3 rel = scene_sub(p, c->eye);
    float z = scene_dot(rel, c->forward);
    if (z < 8.0f) return 0;
    out->x = c->cx + scene_dot(rel, c->right) * c->focal / z;
    out->y = c->cy - scene_dot(rel, c->up) * c->focal / z;
    out->z = z;
    return 1;
}

static void scene_triangle_sampled(SceneCamera* c, SceneVec3 a, SceneVec3 b,
                                  SceneVec3 d, SceneColor color,
                                  const TextureSurface* texture,
                                  float u0, float v0, float u1, float v1,
                                  float u2, float v2)
{
    ScenePoint p[3];
    if (!scene_project(c, a, &p[0]) || !scene_project(c, b, &p[1])
        || !scene_project(c, d, &p[2])) return;
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
                               float u, float v)
{
    SceneColor white = {255, 255, 255};
    scene_triangle_sampled(c, a, b, d, white, texture,
                           u, v, u + 0.35f, v, u + 0.35f, v + 0.35f);
    scene_triangle_sampled(c, a, d, e, white, texture,
                           u, v, u + 0.35f, v + 0.35f, u, v + 0.35f);
}

static SceneVec3 terrain_point(int x, int y, float height, int width, int rows,
                               float block, float height_scale)
{
    return scene_vec3((x - width * 0.5f) * block,
                      -height * height_scale,
                      (y - rows * 0.5f) * block);
}

static int scene_camera_configure(SceneCamera* c, const GameContext* ctx,
                                 const LevelState* lvl,
                                 int w, int h)
{
    int gw = level_width(lvl), gh = level_height(lvl);
    float block = terrain_blocksize_get(lvl->ptr_terrain);
    if (block <= 0.0f) block = 40.0f;
    float hs = block * 0.10f;
    float world_w = (float)gw * block, world_d = (float)gh * block;
    if (s_scene_focus_level != lvl) {
        s_scene_focus_level = lvl;
        s_scene_focus = scene_vec3(0.0f, -8.0f * hs, 0.0f);
        float fx = 0.0f, fz = 0.0f;
        int figures = ctx != NULL
            ? entity_count((GameContext*)ctx, ENTITY_TYPE_MINIFIG) : 0;
        if (figures > 0) {
            for (int i = 0; i < figures; ++i) {
                const EntitySlot* e = entity_at((GameContext*)ctx,
                                                ENTITY_TYPE_MINIFIG, i);
                if (e == NULL) continue;
                fx += e->x;
                fz += e->y;
            }
            fx /= (float)figures;
            fz /= (float)figures;
            int gx = (int)fx, gy = (int)fz;
            s_scene_focus.x = (fx - (float)gw * 0.5f) * block;
            s_scene_focus.z = (fz - (float)gh * 0.5f) * block;
            s_scene_focus.y = -(float)terrain_surface_height_get(
                lvl->ptr_terrain, gx, gy) * hs - block * 0.15f;
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
    c->focal = (float)h * 1.02f;
    c->cx = (float)w * 0.5f;
    c->cy = (float)h * 0.53f;
    c->width = w;
    c->height = h;
    return block > 0.0f && gw > 0 && gh > 0;
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
            SceneVec3 a = terrain_point(x, y, z, w, h, block, hs);
            SceneVec3 b = terrain_point(x + 1, y, z, w, h, block, hs);
            SceneVec3 d = terrain_point(x + 1, y + 1, z, w, h, block, hs);
            SceneVec3 e = terrain_point(x, y + 1, z, w, h, block, hs);
            SceneColor top = terrain_color(vox != NULL
                ? vox + ((size_t)y * (size_t)w + (size_t)x) * LEVEL_VOXEL_STRIDE
                : NULL);
            if (texture != NULL) {
                scene_quad_textured(c, a, b, d, e, texture,
                                    (float)x * 0.35f, (float)y * 0.35f);
            } else {
                scene_quad(c, a, b, d, e, color_scale(top, 1.0f));
            }

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
                                        lo_b, lo_a, texture, u, v);
                } else {
                    scene_quad(c, edge_a[side], edge_b[side], lo_b, lo_a,
                               color_scale(top, light));
                }
            }
        }
    }
}

static SceneVec3 figure_point(SceneVec3 origin, float x, float y, float z,
                              float yaw)
{
    float cs = scene_cos(yaw), sn = scene_sin(yaw);
    return scene_vec3(origin.x + x * cs + z * sn, origin.y + y,
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
    float yaw = e->rot * 0.01745329252f;
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

void level_render_scene(const LevelState* lvl, const GameContext* ctx,
                        int w, int h, void* pixels, int pitch)
{
    if (lvl == NULL || ctx == NULL || pixels == NULL || w <= 0 || h <= 0
        || lvl->ptr_terrain == NULL) {
        if (lvl != NULL && ctx != NULL) {
            level_render_map(lvl, ctx, -1, -1, w, h, pixels, pitch);
        }
        return;
    }
    if (ctx->reward_active) {
        level_render_map(lvl, ctx, -1, -1, w, h, pixels, pitch);
        return;
    }
    size_t count = (size_t)w * (size_t)h;
    if (count > s_scene_depth_capacity) {
        float* next = (float*)realloc(s_scene_depth, count * sizeof(float));
        if (next == NULL) {
            level_render_map(lvl, ctx, -1, -1, w, h, pixels, pitch);
            return;
        }
        s_scene_depth = next;
        s_scene_depth_capacity = count;
    }
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

    int gw = level_width(lvl), gh = level_height(lvl);
    float block = terrain_blocksize_get(lvl->ptr_terrain);
    if (block <= 0.0f) block = 40.0f;
    float hs = block * 0.10f;
    SceneCamera camera;
    scene_camera_configure(&camera, ctx, lvl, w, h);
    camera.pixels = pixels;
    camera.pitch = pitch; camera.depth = s_scene_depth;

    if (s_scene_floor_texture == NULL) {
        s_scene_floor_texture = texture_get_or_load(
            NULL, "World\\WorldTextures\\LavaSplit\\LAVA00.BMP");
    }
    render_terrain(&camera, lvl, block, hs, s_scene_floor_texture);
    for (int type = ENTITY_TYPE_VEHICLE; type <= ENTITY_TYPE_BUILDING; ++type) {
        for (int i = 0; i < entity_count((GameContext*)ctx, type); ++i) {
            const EntitySlot* e = entity_at((GameContext*)ctx, type, i);
            if (e == NULL) continue;
            int gx = (int)e->x, gy = (int)e->y;
            float ground = -terrain_surface_height_get(lvl->ptr_terrain,gx,gy)*hs;
            SceneVec3 origin = {(e->x-gw*0.5f)*block, ground,
                                (e->y-gh*0.5f)*block};
            if (type == ENTITY_TYPE_MINIFIG) {
                render_raider(&camera, e, block, origin,
                              selection_contains_entity(ctx, e));
            } else if (type == ENTITY_TYPE_BUILDING) {
                /* A blocky Tool Store silhouette until the .ae scene loader
                 * supplies its authored object assembly. */
                figure_box(&camera,origin,e->rot*0.01745329252f,0,block*0.42f,
                           0,block*0.80f,block*0.84f,block*0.72f,
                           scene_color(91,83,72));
                figure_box(&camera,origin,e->rot*0.01745329252f,0,block*0.90f,
                           0,block*0.58f,block*0.14f,block*0.52f,
                           scene_color(181,145,75));
            } else {
                figure_box(&camera,origin,e->rot*0.01745329252f,0,block*0.25f,
                           0,block*0.48f,block*0.45f,block*0.38f,
                           scene_color(140,138,129));
            }
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
    if (!scene_camera_configure(&c, ctx, lvl, w, h)) return 0;
    SceneVec3 ray = scene_normalize(scene_add(
        c.forward,
        scene_add(scene_scale(c.right, ((float)mouse_x - c.cx) / c.focal),
                  scene_scale(c.up, (c.cy - (float)mouse_y) / c.focal))));
    int gw = level_width(lvl), gh = level_height(lvl);
    float block = terrain_blocksize_get(lvl->ptr_terrain);
    float ground = s_scene_focus.y + block * 0.15f;
    if (scene_abs(ray.y) < 0.0001f) return 0;
    float t = (ground - c.eye.y) / ray.y;
    if (t <= 0.0f) return 0;
    SceneVec3 hit = scene_add(c.eye, scene_scale(ray, t));
    *cell_x = hit.x / block + (float)gw * 0.5f;
    *cell_y = hit.z / block + (float)gh * 0.5f;
    return *cell_x >= 0.0f && *cell_y >= 0.0f
        && *cell_x < (float)gw && *cell_y < (float)gh;
}

EntitySlot* level_scene_pick_entity(GameContext* ctx, const LevelState* lvl,
                                    int w, int h, int mouse_x, int mouse_y,
                                    float radius_pixels)
{
    if (ctx == NULL || lvl == NULL || lvl->ptr_terrain == NULL) return NULL;
    SceneCamera c;
    if (!scene_camera_configure(&c, ctx, lvl, w, h)) return NULL;
    int gw = level_width(lvl), gh = level_height(lvl);
    float block = terrain_blocksize_get(lvl->ptr_terrain);
    float hs = block * 0.10f;
    float best = radius_pixels * radius_pixels;
    EntitySlot* picked = NULL;
    for (int i = 0; i < entity_count(ctx, ENTITY_TYPE_MINIFIG); ++i) {
            const EntitySlot* e = entity_at(ctx, ENTITY_TYPE_MINIFIG, i);
            if (e == NULL) continue;
            int gx = (int)e->x, gy = (int)e->y;
            SceneVec3 p = {(e->x - (float)gw * 0.5f) * block,
                -terrain_surface_height_get(lvl->ptr_terrain, gx, gy) * hs
                    + block * 0.48f,
                (e->y - (float)gh * 0.5f) * block};
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
