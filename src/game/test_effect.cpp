// SPDX-License-Identifier: LGPL-3.0-or-later
// Screen-space effect lane (finding 120): projectile arcs, screen shake,
// and the lane's dispatch + draw path. The arc table below is the corpus
// geometry: trapezoid control scaling for a 12.0-shot / 3.0-height arc
// transcribed from FUN_00406760/004065f0/00406820 (see finding 120).
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/ai.hpp"
#include "game/effect.hpp"
#include "game/entity.hpp"
#include "game/game.hpp"
#include "game/level.hpp"

static int s_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);      \
            ++s_failures;                                                      \
        }                                                                      \
    } while (0)

static int approx(float a, float b, float eps)
{
    return fabsf(a - b) <= eps;
}

static EntitySlot* place(GameContext* ctx, int type, const char* name,
                         float x, float y)
{
    int idx = entity_create(ctx, type, name, x, y, 0, 0);
    CHECK(idx >= 0, "entity_create");
    return (EntitySlot*)entity_at(ctx, type, idx);
}

/* ---- corpus arc table (finding 120; script-generated from the FUN_0040
 * ---- 65f0 transcription for dist=12, dz=3) ------------------------------ */
static void check_arc_geometry(void)
{
    /* Control vectors: {1,-1.3} normalized to length dist*0.2,
     * {0.3,1} to length dist/3. */
    float c1[2], c2[2];
    {
        float l1 = sqrtf(1.0f * 1.0f + 1.3f * 1.3f);
        float l2 = sqrtf(0.3f * 0.3f + 1.0f * 1.0f);
        c1[0] = 1.0f / l1 * (12.0f * 0.2f);
        c1[1] = -1.3f / l1 * (12.0f * 0.2f);
        c2[0] = 0.3f / l2 * (12.0f * 0.33333334f);
        c2[1] = 1.0f / l2 * (12.0f * 0.33333334f);
    }
    float pts[EFFECT_ARC_POINTS][2];
    float lens[EFFECT_ARC_POINTS];
    int n = 0;
    float total = effect_arc_build(12.0f, 3.0f, pts, lens, &n);
    CHECK(n == EFFECT_ARC_POINTS, "arc polyline has 50 points");
    CHECK(approx(total, 18.613055f, 1e-3f), "arc total length matches corpus");
    CHECK(approx(pts[0][0], 0.0f, 1e-4f) && approx(pts[0][1], 0.0f, 1e-4f),
          "arc starts at (0,0)");
    CHECK(approx(pts[EFFECT_ARC_POINTS - 1][0], 12.0f, 1e-3f) &&
              approx(pts[EFFECT_ARC_POINTS - 1][1], 3.0f, 1e-3f),
          "arc ends at (dist,dz)");
    /* Cumulative lengths are monotone increasing. */
    int mono = 1;
    for (int i = 1; i < n; ++i) {
        if (lens[i] < lens[i - 1]) {
            mono = 0;
        }
    }
    CHECK(mono, "arc segment lengths are cumulative and nondecreasing");

    /* Sampling at half-total sits inside the curve, between start and end.
     * The corpus transcription (finding 120's arc_table.py) evaluates the
     * half-arc-length point at (7.334105, -4.755673) for dist=12, dz=3. */
    float hx = 0, hz = 0;
    effect_arc_sample(pts, lens, n, total * 0.5f, &hx, &hz);
    CHECK(approx(hx, 7.334105f, 1e-3f), "half-arc horizontal matches corpus");
    CHECK(approx(hz, -4.755673f, 1e-3f), "half-arc height matches corpus");
    /* Quarter arc length: corpus (3.114870, -3.446357). */
    effect_arc_sample(pts, lens, n, total * 0.25f, &hx, &hz);
    CHECK(approx(hx, 3.114870f, 1e-3f) && approx(hz, -3.446357f, 1e-3f),
          "quarter-arc sample matches corpus");

    /* Degenerate shots produce no polyline. */
    float pts2[EFFECT_ARC_POINTS][2];
    float lens2[EFFECT_ARC_POINTS];
    int n2 = 99;
    float t2 = effect_arc_build(0.0f, 0.0f, pts2, lens2, &n2);
    CHECK(n2 == 0 && approx(t2, 0.0f, 1e-6f), "degenerate shot builds no arc");
    (void)c1;
    (void)c2;
}

/* ---- lane dispatch counting + straight-shot travel ----------------------- */
static void check_lane_dispatch(void)
{
    GameContext ctx;
    game_context_init(&ctx);
    EntitySlot* a = place(&ctx, 1, "a", 0, 0);
    EntitySlot* b = place(&ctx, 1, "b", 10, 0);

    int s0 = effect_lane_dispatches(&ctx);
    int slot = effect_lane_dispatch(&ctx, a, b, EFFECT_STRAIGHT);
    CHECK(slot >= 0, "straight dispatch takes a lane slot");
    CHECK(effect_lane_active(&ctx) == 1, "one live lane record");
    CHECK(effect_lane_dispatches(&ctx) == s0 + 1, "dispatch counter incremented");
    CHECK((a->flags_3e0 & 0x00800000u) == 0,
          "straight shot does not raise the arc flag");

    /* Straight shots move 5 cells/s toward the target. */
    effect_lane_tick(&ctx, 1.0f);
    const EffectSlot* rec = &ctx.effect_lane[slot];
    CHECK(approx(rec->px, 5.0f, 0.001f), "straight shot travels 5 cells in 1s");
    CHECK(effect_lane_active(&ctx) == 1, "still live mid-flight");

    /* Full lane rejects dispatches. */
    EntitySlot* c = place(&ctx, 1, "c", 0, 1);
    EntitySlot* d = place(&ctx, 1, "d", 20, 1);
    int dispatched = 1;
    int full = -1;
    for (int i = 1; i < EFFECT_LANE_CAP; ++i) {
        int s = effect_lane_dispatch(&ctx, c, d, EFFECT_ARC);
        if (s >= 0) {
            ++dispatched;
        } else {
            full = s;
            break;
        }
    }
    CHECK(dispatched == EFFECT_LANE_CAP,
          "lane fills exactly EFFECT_LANE_CAP slots");
    CHECK(full == -1, "overflow dispatch rejected (-1)");
}

/* ---- projectile arcs: corpus ballistic travel + arrival ------------------ */
static void check_arc_travel(void)
{
    GameContext ctx;
    game_context_init(&ctx);
    EntitySlot* shoot = place(&ctx, 1, "shooter", 1, 1);
    EntitySlot* target = place(&ctx, 4, "target", 9, 1);

    int slot = ai_fire_arc(&ctx, shoot, target);
    CHECK(slot >= 0, "arc dispatch succeeds");
    CHECK((shoot->flags_3e0 & 0x00400000u) != 0, "arc flag raised on shooter");
    CHECK(shoot->arc_target == (void*)target, "+0x2e4 target handle stored");

    /* The arc travels toward the target along the ballistic curve. */
    effect_lane_tick(&ctx, 1.0f);
    const EffectSlot* rec = &ctx.effect_lane[slot];
    CHECK(rec->px > 1.0f && rec->py >= 1.0f - 0.01f,
          "arc progressed after 1s");
    CHECK((shoot->flags_3e0 & 0x00400000u) != 0,
          "arc flag stays up mid-flight");

    /* Run until arrival: far past the arc's total time. */
    for (int t = 0; t < 60 && effect_lane_active(&ctx) > 0; ++t) {
        effect_lane_tick(&ctx, 1.0f);
    }
    CHECK(effect_lane_active(&ctx) == 0, "arc freed after arrival");
    CHECK(approx(rec->px, 9.0f, 0.01f) && approx(rec->py, 1.0f, 0.01f),
          "projectile lands on the target");
    CHECK((shoot->flags_3e0 & 0x00400000u) == 0,
          "arc flag cleared on arrival");
    CHECK((target->flags_3e8 & 0x00800000u) != 0,
          "target wake bit 0x800000 raised on arrival");
    CHECK(approx(shoot->field_340, -1.0f, 1e-4f), "+0x340 = -1.0 on arrival");
}

/* ---- screen shake: flag raise/clear + amplitude decay -------------------- */
static void check_shake(void)
{
    GameContext ctx;
    game_context_init(&ctx);
    EntitySlot* e = place(&ctx, 1, "shaker", 0, 0);

    CHECK(approx(ctx.shake_amp, 1.0f, 1e-4f), "DAT_004a48b0 starts at 1.0");
    ai_shake_raise(&ctx, e);
    CHECK((e->flags_3e4 & 0x40000000u) != 0, "shake bit raised");
    ai_tick(&ctx, 0.5f);
    CHECK(approx(ctx.shake_amp, 1.0f - 0.5f * 0.16f, 1e-3f),
          "amplitude decays by 0.16/s (dt=0.5)");
    CHECK((e->flags_3e4 & 0x40000000u) != 0, "shake bit stays while amp > 0");
    CHECK(ctx.shake_frame != 0, "frame reports a shaking entity");

    /* After (~1.0/0.16) seconds the amplitude crosses zero, resetting to
     * 1.0 and clearing the bit. */
    for (int t = 0; t < 16 && (e->flags_3e4 & 0x40000000u) != 0; ++t) {
        ai_tick(&ctx, 1.0f);
    }
    CHECK((e->flags_3e4 & 0x40000000u) == 0, "shake bit cleared at decay end");
    CHECK(approx(ctx.shake_amp, 1.0f, 1e-4f), "amplitude reset to 1.0");

    /* A quiet frame after clearing produces no viewport offset. */
    ai_tick(&ctx, 1.0f);
    CHECK(ctx.shake_frame == 0, "no shake offset on a quiet frame");
}

/* ---- lane draw must not corrupt the render path -------------------------- */
static void check_lane_draw(void)
{
    GameContext ctx;
    game_context_init(&ctx);
    uint8_t buf[128 * 128 * 4];
    memset(buf, 0x11, sizeof(buf));

    /* A live arc record with a bound level: draw writes lane pixels only. */
    static uint8_t voxels[16 * 16 * 0x48];
    LevelState map;
    memset(&map, 0, sizeof(map));
    memset(voxels, 0, sizeof(voxels));
    level_set_int(&map, LEVEL_OFF_WIDTH, 16);
    level_set_int(&map, LEVEL_OFF_HEIGHT, 16);
    level_store_ptr(&map, LEVEL_OFF_VOXELS, voxels);
    ctx.level = &map;

    EntitySlot* a = place(&ctx, 1, "a", 0, 0);
    EntitySlot* b = place(&ctx, 1, "b", 8, 0);
    int slot = effect_lane_dispatch(&ctx, a, b, EFFECT_ARC);
    CHECK(slot >= 0, "draw probe dispatch");
    ctx.effect_lane[slot].px = 4.0f;
    ctx.effect_lane[slot].py = 0.0f;

    effect_lane_draw(&ctx, 128, 128, buf, 128 * 4);
    int px = (int)(4.0f / 16.0f * 128.0f);
    int py = (int)(0.0f / 16.0f * 128.0f);
    uint8_t* p = buf + ((size_t)py * 128 + (size_t)px) * 4;
    CHECK(p[0] == 120 && p[1] == 255 && p[2] == 160,
          "arc marker drawn at the sampled position");
    effect_lane_tick(&ctx, 60.0f); /* drain */
    effect_lane_draw(&ctx, 128, 128, buf, 128 * 4);
    memset(buf, 0x11, sizeof(buf));
    effect_lane_draw(&ctx, 128, 128, buf, 128 * 4);
    int changed = 0;
    for (size_t i = 0; i < sizeof(buf); ++i) {
        if (buf[i] != 0x11) {
            ++changed;
        }
    }
    CHECK(changed == 0, "drained lane draws nothing (render path untouched)");
}

int main(void)
{
    check_arc_geometry();
    check_lane_dispatch();
    check_arc_travel();
    check_shake();
    check_lane_draw();

    if (s_failures != 0) {
        fprintf(stderr, "effect_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("effect_test: OK\n");
    return 0;
}