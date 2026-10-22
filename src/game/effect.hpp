// SPDX-License-Identifier: LGPL-3.0-or-later
/* Game layer — screen-space effect lane (finding 120).
 *
 * Port of the corpus's entity effect bank (DAT_0050391c) and the callback
 * effect branches that drive it (finding 10's "Documented TODO branches":
 * projectile arcs and screen shake — now closed):
 *
 *   - Projectile arcs: the entity-tick arc branch (callback 0x43ca82) tests
 *     +0x3e0 bit 0x400000, resolves the work object (FUN_0044a200), steps
 *     the projectile scene through FUN_004755e0 + FUN_00475120, and when
 *     the scene's leftover time passes the last frame (fcomps 0.0 with
 *     test $0x41, causing a jump when value <= 0) it completes: +0x3e8 |=
 *     0x800000, +0x3e0 &= ~0x400000, +0x340 = -1.0, and the scene object is
 *     disposed (FUN_00472a40). The ballistic *shape* comes from the arc
 *     polyline builder FUN_00406820 (count {0x32} points via a cubic
 *     FUN_004065f0 with the caller's control points), the cumulative
 *     segment-length table FUN_004067b0, and the arc-length sampler
 *     FUN_004068c0 (walk cumulative lengths, lerp between points). The
 *     firer (FUN_004420d0 / FUN_0046fc70) pre-normalizes two control
 *     vectors: the corpus constants are {1.0, -1.3} scaled to `dist*0.2`
 *     and {0.3, 1.0} scaled to `dist/3` (FUN_00406760), ending at the
 *     target point — that is the "corpus-shaped ballistic curve".
 *   - Screen shake: the branch at 0x43ce88 consumes +0x3e4 bit 0x40000000
 *     (raised by the impact/wake handler FUN_004475c0): while a work
 *     object exists it draws it at a random offset proportional to the
 *     global amplitude DAT_004a48b0 (stated 1.0 in the image, decayed by
 *     the 0x49e3fc constant = 0.16 per second), and once the amplitude
 *     passes below zero it resets DAT_004a48b0 to 1.0 and clears the bit.
 *
 * The port models the lane as a fixed array of EffectSlot records (see
 * game.hpp) that ai_tick advances per-frame; the shake state rides in
 * GameContext (shake_amp + the current viewport offset) so the software
 * renderer can visibly shake the frame without touching the render
 * path's own bookkeeping.
 */
#ifndef LEGORRR_GAME_EFFECT_HPP
#define LEGORRR_GAME_EFFECT_HPP

#include "game.hpp"

#ifdef __cplusplus
extern "C" {
#endif

/* --- arc geometry (corpus FUN_00406820 / 004067b0 / 004068c0) ------------ */

/* Build the corpus ballistic arc for a shot of horizontal distance `dist`
 * and height delta `dz` (the firer's target minus start): a cubic-bezier
 * polyline through the corpus control vectors {1.0,-1.3}·(dist*0.2) and
 * {0.3,1.0}·(dist/3) between start (0,0) and end (dist,dz), plus the
 * cumulative segment-length table. Returns the total arc length. Points
 * and lengths are written into `pts`/`lens` (each EFFECT_ARC_POINTS
 * capacity); `n` receives the point count (0x32 when the distance is
 * nonzero, 0 when degenerate). */
float effect_arc_build(float dist, float dz,
                       float pts[EFFECT_ARC_POINTS][2],
                       float lens[EFFECT_ARC_POINTS], int* n);

/* Sample a built arc at arc-length distance `d`: walk the cumulative
 * length table (FUN_004068c0 semantics) and lerp between the bracketing
 * polyline points in (horizontal, height) space. Degenerate arcs clamp to
 * the single point. */
void effect_arc_sample(const float pts[EFFECT_ARC_POINTS][2],
                       const float lens[EFFECT_ARC_POINTS], int n,
                       float d, float* hx, float* hz);

/* --- lane ops ------------------------------------------------------------- */

/* Dispatch a new effect into the lane: `kind` EFFECT_STRAIGHT (a linear
 * shot — the corpus lazer/pusher records) or EFFECT_ARC (a ballistic arc
 * — the freezer record). The record's start is the owner's position and
 * its target is the `target` slot (may be NULL for straight shots, which
 * then travel along the corpus direction derived from the target cell).
 * On success returns the slot index and increments ctx->effect_dispatches;
 * returns -1 when the lane is full. The corpus allocator is FUN_0046ff70's
 * scan for the first free record (threaded through the +0x2a4 owner mark)
 * and the fire dispatch FUN_004420d0. */
int effect_lane_dispatch(GameContext* ctx, EntitySlot* owner,
                         EntitySlot* target, int kind);

/* Advance every live lane record by dt (the callback arc branch + the
 * effect-bank ticker FUN_0046f550's per-record FUN_0046f920): straight
 * shots move along their unit direction at speed*dt; arcs sample their
 * polyline at progress += speed*dt. On arrival the corpus completes the
 * arc — target wake bit 0x800000 and owner +0x3e0 bit 0x400000 cleared,
 * +0x340 = -1.0 — and the record is freed. Returns the number of arrivals
 * this tick. */
int effect_lane_tick(GameContext* ctx, float dt);

/* Draw live lane records onto the software framebuffer (the callback's
 * screen-space effect lane draw path): each record's current position. The
 * draw reads only the lane — it never modifies the map render path's own
 * state. */
void effect_lane_draw(const GameContext* ctx, int w, int h, void* pixels,
                      int pitch);

/* Number of live records (tests + timing). */
int effect_lane_active(const GameContext* ctx);

/* The cumulative dispatch counter (test helper: lane dispatch counting). */
int effect_lane_dispatches(const GameContext* ctx);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_EFFECT_HPP */