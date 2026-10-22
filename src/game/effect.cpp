// SPDX-License-Identifier: LGPL-3.0-or-later
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "effect.hpp"
#include "level.hpp"

/* Corpus constants pinned in the exported evidence (finding 121):
 *   FUN_00406760's control scaling — local_18 = {1.0, -1.3} normalized to
 *   length dist*0.2, local_20 = {0.3, 1.0} to length dist/3.
 *   FUN_004065f0 — the cubic point evaluation (decompiled).
 *   FUN_00406820 — the 50-point polyline fill.
 *   FUN_004067b0 — the cumulative segment-length table.
 *   FUN_004068c0 — the arc-length sampler. */
#define ARC_POINTS EFFECT_ARC_POINTS

/* Corpus FUN_004065f0: a cubic point on the arc. `p0` = start (0,0),
 * `c1`/`c2` = the two control vectors, `p3` = end (dist, dz), t in
 * [0,1]. Transcribed from the shipped decompile. */
static void arc_cubic(float p0[2], float c1[2], float c2[2], float p3[2],
                      float t, float out[2])
{
    float fVar1 = c1[0] * 3.0f + p0[0];
    float fVar5 = t * t;
    float fVar4 = (fVar1 - p0[0]) * 3.0f;
    float fVar3 = ((p3[0] - c2[0] * 3.0f) - fVar1) * 3.0f - fVar4;
    out[0] = fVar4 * t + fVar3 * fVar5 +
             (((p3[0] - p0[0]) - fVar4) - fVar3) * fVar5 * t + p0[0];

    fVar1 = c1[1] * 3.0f + p0[1];
    fVar4 = (fVar1 - p0[1]) * 3.0f;
    fVar3 = ((p3[1] - c2[1] * 3.0f) - fVar1) * 3.0f - fVar4;
    out[1] = fVar4 * t + fVar3 * fVar5 +
             (((p3[1] - p0[1]) - fVar4) - fVar3) * fVar5 * t + p0[1];
}

/* Corpus FUN_00406760: normalize {x,y} to length `scale`. */
static void arc_scalevec(float v[2], float scale)
{
    float len = sqrtf(v[0] * v[0] + v[1] * v[1]);
    if (len <= 0.0f) {
        v[0] = v[1] = 0.0f;
        return;
    }
    float p2 = len / scale;
    v[0] /= p2;
    v[1] /= p2;
}

float effect_arc_build(float dist, float dz, float pts[ARC_POINTS][2],
                       float lens[ARC_POINTS], int* n)
{
    if (n == NULL) {
        return 0.0f;
    }
    /* Degenerate shot (no horizontal separation). */
    if (!(dist > 0.0f)) {
        *n = 0;
        if (pts != NULL) {
            pts[0][0] = 0.0f;
            pts[0][1] = 0.0f;
        }
        return 0.0f;
    }

    /* Control vectors per FUN_0046fc70's caller constants. */
    float c1[2] = { 1.0f, -1.3f };
    float c2[2] = { 0.3f, 1.0f };
    arc_scalevec(c1, dist * 0.2f);
    arc_scalevec(c2, dist * 0.33333334f);
    float p0[2] = { 0.0f, 0.0f };
    float p3[2] = { dist, dz };

    *n = ARC_POINTS;
    for (int u = 0; u < ARC_POINTS; ++u) {
        float t = (u == 0) ? 0.0f
                           : (float)u * (1.0f / (float)(ARC_POINTS - 1));
        if (u == 0) {
            pts[0][0] = p0[0];
            pts[0][1] = p0[1];
        } else {
            arc_cubic(p0, c1, c2, p3, t, pts[u]);
        }
    }

    /* Cumulative segment lengths (FUN_004067b0 + FUN_00406730). */
    lens[0] = 0.0f;
    for (int u = 1; u < ARC_POINTS; ++u) {
        float dx = pts[u][0] - pts[u - 1][0];
        float dy = pts[u][1] - pts[u - 1][1];
        lens[u] = lens[u - 1] + sqrtf(dx * dx + dy * dy);
    }
    return lens[ARC_POINTS - 1];
}

void effect_arc_sample(const float pts[ARC_POINTS][2],
                       const float lens[ARC_POINTS], int n,
                       float d, float* hx, float* hz)
{
    float h = 0.0f, z = 0.0f;
    if (n < 2) {
        if (n == 1) {
            h = pts[0][0];
            z = pts[0][1];
        }
        if (hx != NULL) {
            *hx = h;
        }
        if (hz != NULL) {
            *hz = z;
        }
        return;
    }
    float total = lens[n - 1];
    if (d <= 0.0f) {
        h = pts[0][0];
        z = pts[0][1];
    } else if (d >= total) {
        h = pts[n - 1][0];
        z = pts[n - 1][1];
    } else {
        for (int u = 1; u < n; ++u) {
            if (d < lens[u]) {
                float seg = lens[u] - lens[u - 1];
                float frac = (seg > 0.0f) ? (d - lens[u - 1]) / seg : 0.0f;
                h = pts[u - 1][0] + (pts[u][0] - pts[u - 1][0]) * frac;
                z = pts[u - 1][1] + (pts[u][1] - pts[u - 1][1]) * frac;
                break;
            }
        }
    }
    if (hx != NULL) {
        *hx = h;
    }
    if (hz != NULL) {
        *hz = z;
    }
}

/* --- lane ----------------------------------------------------------------- */

static int free_slot(GameContext* ctx)
{
    if (ctx == NULL) {
        return -1;
    }
    for (int i = 0; i < EFFECT_LANE_CAP; ++i) {
        if (!ctx->effect_lane[i].active) {
            return i;
        }
    }
    return -1;
}

int effect_lane_dispatch(GameContext* ctx, EntitySlot* owner,
                         EntitySlot* target, int kind)
{
    if (ctx == NULL || owner == NULL) {
        return -1;
    }
    int slot = free_slot(ctx);
    if (slot < 0) {
        return -1;
    }
    EffectSlot* rec = &ctx->effect_lane[slot];
    memset(rec, 0, sizeof(*rec));
    rec->active = 1;
    rec->kind = kind;
    rec->owner = owner;
    rec->target = target;
    rec->sx = owner->x;
    rec->sy = owner->y;
    rec->sz = 0.0f;
    rec->px = rec->sx;
    rec->py = rec->sy;
    rec->pz = rec->sz;
    rec->npts = 0;

    if (target != NULL) {
        rec->tx = target->x;
        rec->ty = target->y;
        rec->tz = 0.0f;
    } else {
        rec->tx = rec->sx + 1.0f;
        rec->ty = rec->sy;
        rec->tz = 0.0f;
    }

    if (kind == EFFECT_STRAIGHT) {
        /* Corpus FUN_0046faf0: straight shots carry a unit direction and
         * the speed constant 0x40a00000 = 5.0. */
        float dx = rec->tx - rec->sx;
        float dy = rec->ty - rec->sy;
        float len = sqrtf(dx * dx + dy * dy);
        if (len > 0.0f) {
            rec->dirx = dx / len;
            rec->diry = dy / len;
        } else {
            rec->dirx = 1.0f;
            rec->diry = 0.0f;
        }
        rec->speed = 5.0f;
        rec->total = len;
    } else {
        /* Arc: build the corpus ballistic polyline. Height delta zero in
         * the port's 2D lane (the corpus's z term is the third sample
         * output; the port plane drops it). Speed 4.0 per FUN_0046fc70's
         * 0x40800000 record. */
        float dx = rec->tx - rec->sx;
        float dy = rec->ty - rec->sy;
        float dist = sqrtf(dx * dx + dy * dy);
        rec->total = effect_arc_build(dist, 0.0f, rec->arc, rec->arc_len,
                                      &rec->npts);
        if (dist > 0.0f) {
            rec->dirx = dx / dist;
            rec->diry = dy / dist;
        } else {
            rec->dirx = 1.0f;
            rec->diry = 0.0f;
        }
        rec->speed = 4.0f;
        /* The corpus firer also marks the entity's arc-in-flight bit
         * (+0x3e0 0x400000); the callback completion clears it. */
        owner->flags_3e0 |= 0x00400000u;
    }

    owner->arc_slot = slot;
    ctx->effect_dispatches++;
    return slot;
}

/* Straight shots: move along the unit direction. Return 1 on arrival. */
static int step_straight(GameContext* /*ctx*/, EffectSlot* rec, float dt)
{
    float step = rec->speed * dt;
    float dx = rec->tx - rec->px;
    float dy = rec->ty - rec->py;
    float remain = sqrtf(dx * dx + dy * dy);
    if (remain <= step) {
        rec->px = rec->tx;
        rec->py = rec->ty;
        return 1;
    }
    rec->px += rec->dirx * step;
    rec->py += rec->diry * step;
    return 0;
}

/* Arc shots: progress along the corpus polyline by arc length, sample it,
 * and place the world position from the start + horizontal dir * sample.
 * Return 1 when the whole arc has been walked. */
static int step_arc(GameContext* /*ctx*/, EffectSlot* rec, float dt)
{
    rec->progress += rec->speed * dt;
    if (rec->progress >= rec->total) {
        rec->px = rec->tx;
        rec->py = rec->ty;
        return 1;
    }
    float hx, hz;
    effect_arc_sample(rec->arc, rec->arc_len, rec->npts, rec->progress,
                      &hx, &hz);
    rec->px = rec->sx + rec->dirx * hx;
    rec->py = rec->sy + rec->diry * hx;
    rec->pz = hz;
    return 0;
}

/* Corpus completion of an arriving arc (callback 0x43cade): the target is
 * woken (+0x3e8 bit 0x800000), the owner's arc flag (+0x3e0 0x400000) is
 * cleared, +0x340 = -1.0. */
static void arrive(GameContext* ctx, EffectSlot* rec)
{
    if (rec->target != NULL) {
        rec->target->flags_3e8 |= 0x00800000u;
    }
    if (rec->owner != NULL) {
        rec->owner->flags_3e0 &= ~0x00400000u;
        rec->owner->field_340 = -1.0f;
        rec->owner->arc_slot = -1;
    }
    (void)ctx;
}

int effect_lane_tick(GameContext* ctx, float dt)
{
    int arrivals = 0;
    if (ctx == NULL) {
        return 0;
    }
    for (int i = 0; i < EFFECT_LANE_CAP; ++i) {
        EffectSlot* rec = &ctx->effect_lane[i];
        if (!rec->active) {
            continue;
        }
        int done = (rec->kind == EFFECT_STRAIGHT) ? step_straight(ctx, rec, dt)
                                                 : step_arc(ctx, rec, dt);
        if (done) {
            arrive(ctx, rec);
            rec->active = 0;
            ++arrivals;
        }
    }
    return arrivals;
}

int effect_lane_active(const GameContext* ctx)
{
    if (ctx == NULL) {
        return 0;
    }
    int n = 0;
    for (int i = 0; i < EFFECT_LANE_CAP; ++i) {
        if (ctx->effect_lane[i].active) {
            ++n;
        }
    }
    return n;
}

int effect_lane_dispatches(const GameContext* ctx)
{
    return (ctx != NULL) ? ctx->effect_dispatches : 0;
}

/* Draw the lane's live records as small screen markers (the screen-space
 * effect lane draw path). Coordinates are world units like the entity
 * markers — converted through the bound level grid exactly as
 * level_render_map converts entity positions (finding 121). */
static void draw_marker(const LevelState* lvl, void* pixels, int pitch,
                        int w, int h, float wx, float wy,
                        int cr, int cg, int cb)
{
    if (pixels == NULL || lvl == NULL) {
        return;
    }
    int gw = level_width(lvl);
    int gh = level_height(lvl);
    if (gw <= 0 || gh <= 0) {
        return;
    }
    int px = (int)(wx / (float)gw * (float)w);
    int py = (int)(wy / (float)gh * (float)h);
    if (px < 0 || px >= w || py < 0 || py >= h) {
        return;
    }
    uint8_t* p = (uint8_t*)pixels + (size_t)py * pitch + (size_t)px * 4;
    p[0] = (uint8_t)cr;
    p[1] = (uint8_t)cg;
    p[2] = (uint8_t)cb;
}

void effect_lane_draw(const GameContext* ctx, int w, int h, void* pixels,
                      int pitch)
{
    if (ctx == NULL || pixels == NULL || w <= 0 || h <= 0) {
        return;
    }
    const LevelState* lvl = ctx->level;
    for (int i = 0; i < EFFECT_LANE_CAP; ++i) {
        const EffectSlot* rec = &ctx->effect_lane[i];
        if (!rec->active) {
            continue;
        }
        if (rec->kind == EFFECT_ARC) {
            draw_marker(lvl, pixels, pitch, w, h, rec->px, rec->py,
                        120, 255, 160);
        } else {
            draw_marker(lvl, pixels, pitch, w, h, rec->px, rec->py,
                        255, 220, 120);
        }
    }
}

/* --- screen shake ---------------------------------------------------------- */
/* Definitions of the ai.hpp-declared raisers: extern "C" so their symbols
 * match the ah.hpp declarations (the library compiles C++). */

extern "C" {

void ai_shake_raise(GameContext* ctx, EntitySlot* e)
{
    if (ctx == NULL || e == NULL) {
        return;
    }
    /* FUN_004475c0: +0x3e4 |= 0x40000000 on impact. */
    e->flags_3e4 |= 0x40000000u;
    /* The corpus's global amplitude is left to whichever shake branch runs
     * (reset to 1.0 by the decay when it crosses zero); the port mirrors
     * the stated start value on first raise. */
    if (ctx->shake_amp <= 0.0f) {
        ctx->shake_amp = 1.0f;
    }
}

/* FUN_004420d0 / FUN_0046fc70: the arc fire dispatch — a ballistic
 * projectile from `shooter` toward `target`, raising the +0x3e0 arc flag
 * on the shooter and recording the +0x2e4 target handle the callback arc
 * branch reads. */
int ai_fire_arc(GameContext* ctx, EntitySlot* shooter, EntitySlot* target)
{
    if (ctx == NULL || shooter == NULL || target == NULL) {
        return -1;
    }
    int slot = effect_lane_dispatch(ctx, shooter, target, EFFECT_ARC);
    if (slot < 0) {
        return -1;
    }
    shooter->arc_target = (void*)target; /* +0x2e4 */
    return slot;
}

} /* extern "C" */