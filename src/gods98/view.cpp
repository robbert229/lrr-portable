// SPDX-License-Identifier: LGPL-3.0-or-later
#include <math.h>
#include <string.h>

#include "view.hpp"

/* Reconstruction-owned globals for corpus constants. */
float g_view_follow_scale = 1.0f;   /* DAT_005560a4 (grid pick follow scale) */
int   g_view_grid_edge   = 0;       /* DAT_005560ac (tile-count edge clamp) */
int   g_view_test_blend  = 0;       /* test hook for the mode-3 EMA target */

/* ---- pure vector helpers (corpus FUN_00401720/1500/1420/16f0/13a0) ------ */

void vec_scale(float* dst, const float* src, float s)
{
    dst[0] = s * src[0];
    dst[1] = s * src[1];
    dst[2] = s * src[2];
}

void vec_add(float* dst, const float* a, const float* b)
{
    dst[0] = a[0] + b[0];
    dst[1] = a[1] + b[1];
    dst[2] = a[2] + b[2];
}

float vec_dist(const float* a, const float* b)
{
    float dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

float vec_dot(const float* a, const float* b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

float vec_len(const float* a)
{
    return sqrtf(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
}

/* ---- camera-handle primitives (record-backed) --------------------------- */

void cam_offset(CameraHandle* c, float x, float y, float z)
{
    if (c == NULL) {
        return;
    }
    c->inuse = 1;
    c->pos[0] += x;
    c->pos[1] += y;
    c->pos[2] += z;
}

void cam_get_pos(CameraHandle* c, float out[3])
{
    if (c != NULL && out != NULL) {
        out[0] = c->pos[0];
        out[1] = c->pos[1];
        out[2] = c->pos[2];
    }
}

void cam_set_pos(CameraHandle* c, float x, float y, float z)
{
    if (c == NULL) {
        return;
    }
    c->inuse = 1;
    c->pos[0] = x;
    c->pos[1] = y;
    c->pos[2] = z;
}

void cam_get_facing(CameraHandle* c, float out[3])
{
    if (c != NULL && out != NULL) {
        out[0] = c->rot[0];
        out[1] = c->rot[1];
        out[2] = c->rot[2];
    }
}

void cam_set_facing(CameraHandle* c, const float* r)
{
    if (c == NULL || r == NULL) {
        return;
    }
    c->inuse = 1;
    c->rot[0] = r[0];
    c->rot[1] = r[1];
    c->rot[2] = r[2];
}

/* ---- world/stub boundaries ---------------------------------------------- */
/*  FUN_0044f6d0 grid pick   -> floor floor of the camera position.
 *  FUN_0044f940 terrain height-> 0 (no level/world data yet).
 *  FUN_00431570/FUN_0044fe70 camera angle -> 0.
 *  FUN_0044a200 object lookup -> 0 (no object layer yet; mode 2 idles).
 *  FUN_0044a070 velocity blend-> returns 0 (targeted EMA deferred).
 *  FUN_0041f940 / FUN_004369f4 / FUN_004798b0 camera steps -> no-ops/1. */

static int grid_pick(EngineContext* ctx, const float* pos, int* gx, int* gy)
{
    /* with a bound level, cells come from the real voxel grid; terrain uses
     * the voxel surface byte (+0x04). */
    if (ctx->level_block != NULL) {
        int w = engine_level_width(ctx);
        int h = engine_level_height(ctx);
        int x = (int)pos[0];
        int y = (int)pos[1];
        if (w > 0 && h > 0 && x >= 0 && y >= 0 && x < w && y < h) {
            *gx = x;
            *gy = y;
            return 1;
        }
        return 0;
    }
    *gx = (int)pos[0];
    *gy = (int)pos[1];
    return 1;
}
static float terrain_height(EngineContext* ctx, float x, float y)
{
    if (ctx->level_block != NULL) {
        return (float)engine_voxel_byte(ctx, (int)x, (int)y, 0x04);
    }
    return 0.0f;
}
static void cam_angle()
{
    /* TODO(view): FUN_00431570/FUN_0044fe70 camera angle. */
}
static int object_lookup(int /*id*/)
{
    return 0; /* TODO(view): FUN_0044a200 object layer. */
}
static int target_blend(int /*id*/, int /*mode*/, float* blend,
                        float* facing)
{
    if (blend != NULL) {
        blend[0] = blend[1] = blend[2] = 0.0f;
    }
    if (facing != NULL) {
        facing[0] = facing[1] = facing[2] = 0.0f;
    }
    return g_view_test_blend; /* 0 = deferred (TODO: FUN_0044a070) */
}

/* ---- the ported tick ---------------------------------------------------- */

static void view_tail(ViewContext* v, float scale)
{
    /* Corpus tail: when the motion timer has expired, velocity snaps to 0;
     * otherwise it decays toward the step size and the camera is shifted by
     * the negative velocity (glide-to-stop). */
    if (v->timer_num <= v->timer_acc) {
        v->velocity[0] = v->velocity[1] = v->velocity[2] = 0.0f;
    } else {
        float decay = ((float)(v->timer_num - v->timer_acc) / (float)v->timer_num)
                    * v->step;
        /* corpus FUN_004798b0 ramp (D3DRM math helper) modeled as 1.0 */
        vec_scale(v->velocity, v->velocity, decay);
        v->timer_acc += (int)scale;
        cam_offset(&v->cam_world, -v->velocity[0], -v->velocity[1],
                   -v->velocity[2]);
    }
}

void view_tick(ViewContext* v, EngineContext* ctx, float dt, float scale)
{
    if (v == NULL) {
        return;
    }
    EngineState* st = ctx != NULL ? ctx->state_block : NULL;
    (void)st;
    cam_offset(&v->cam_world, -v->velocity[0], -v->velocity[1],
               -v->velocity[2]);

    if (v->mode == 1) {
        /* Follow: keep the follow camera at the offset, then clamp it at the
         * world grid edges (corpus: local_8/local_c vs DAT_005560ac). */
        vec_scale(v->offset, v->offset, g_view_follow_scale);
        cam_offset(&v->cam_follow, v->offset[0], v->offset[1], v->offset[2]);
        float pos[3], fac[3];
        cam_get_pos(&v->cam_follow, pos);
        cam_get_facing(&v->cam_follow, fac);
        (void)fac;
        int gx = 0, gy = 0;
        if (!grid_pick(ctx, pos, &gx, &gy)) {
            cam_offset(&v->cam_follow, -v->offset[0], -v->offset[1],
                       -v->offset[2]);
        } else {
            const int edge = g_view_grid_edge;
            if (gx < edge || engine_level_width(ctx) - edge <= gx) {
                cam_offset(&v->cam_follow, -v->offset[0], 0, 0);
            }
            if (gy < edge || engine_level_height(ctx) - edge <= gy) {
                cam_offset(&v->cam_follow, 0, -v->offset[1], 0);
            }
            float h = terrain_height(ctx, pos[0], pos[1]);
            cam_set_pos(&v->cam_facing, pos[0], pos[1], h);
            cam_angle(); /* rotor; sets yaw below in the corpus */
            float yaw = v->follow_span + (-0.0f);
            cam_set_pos(&v->cam_world, 0.0f, 0.0f, -yaw);
        }
        view_tail(v, scale);
        return;
    }

    if (v->mode == 2) {
        /* Scripted / lagged follow: interpolate toward the target with
         * k = |d|/(|d|+A) once an object target exists (object_lookup). */
        int target = object_lookup(v->follow_id);
        if (target == 0) {
            view_tail(v, scale);
            return;
        }
        float goal3[3], facing[3];
        cam_get_pos(&v->cam_world, goal3);
        cam_get_facing(&v->cam_world, facing);
        float dist = vec_len(goal3);
        float a = (float)v->a3;
        float k = a <= 0.0f ? 0.0f : dist / (dist + a); (void)k;
        /* ...full corpus interpolations follow when the object layer lands */
        view_tail(v, scale);
        return;
    }

    if (v->mode == 3) {
        /* Free/cruise: velocity-EMA toward the target + dirty-flag gating. */
        float pos[3], facing[3];
        cam_get_pos(&v->cam_world, pos);
        cam_get_facing(&v->cam_world, facing);
        float blend[3], tface[3];
        /* TODO(view): FUN_0044a070 velocity-EMA; when the target is not
         * dirty (flags & 0x10) the cached target is used closer. */
        if (target_blend(v->follow_id, v->a5, blend, tface) == 0) {
            view_tail(v, scale);
            return;
        }
        float d1 = vec_dist(tface, facing);
        float d2 = vec_dot(tface, facing);
        (void)d1; (void)d2;
        /* --_DAT_0049e378/dt*_e384 modelling using the real dt */
        float frame = dt > 0.0f ? (0.0333333f / dt) * 0.5f : 1.0f;
        vec_scale(tface, tface, frame);
        vec_add(facing, facing, tface);
        cam_set_pos(&v->cam_world, tface[0], tface[1], tface[2]);
        cam_set_facing(&v->cam_world, facing);
        v->flags |= 0x10;
        view_tail(v, scale);
        return;
    }

    view_tail(v, scale);
}

/* The corpus ticks the three view contexts each frame (finding 03 §4). */
void view_views_tick(EngineContext* ctx, float dt, float scale)
{
    if (ctx == NULL) {
        return;
    }
    for (int i = 0; i < 3; ++i) {
        if (ctx->views[i] != NULL) {
            view_tick(ctx->views[i], ctx, dt, scale);
        }
    }
}