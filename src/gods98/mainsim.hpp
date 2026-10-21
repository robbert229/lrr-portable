// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — MainSimStep (corpus @ 0x4250c0, finding 04): the frame's
 * input + app-state step and, in the original, its quit gate. Ported
 * structurally — the observed clusters are:
 *
 *   1. quit gate (QuitPoll FUN_004589c0 + KeepRunning FUN_00435940 +
 *      IsInGame checks; focus-loss pause menu when DAT_00505808 == 0)
 *   2. debug/cheat toggles (gated debug word 0x10): Frame Rate Monitor
 *      (state 0x40), render pass (0x2/0x1000/0x2000), Memory Monitor
 *      (state 0x80), Sound Effects (state 0x10), Music, air cheat key
 *   3. camera keys (FUN_00436900/004369f4/00436aa5 on the view contexts
 *      with dt*k) — routed to the view module as a per-key step
 *   4. the three edge outputs o1/o2/o3 that EngineTick's later tickers read
 *
 * Key-slot mapping: the corpus reads scalars like DAT_0076ab9d (per-key
 * bytes at fixed offsets); the findings do not pin which scan codes those
 * are. Reconstruction binds named keys to canonical codes (below) and
 * documents the divergence. Selecting objects / menus / the rename touch
 * (FUN_004525c0 + FUN_00437880) stay stub boundaries until the entity
 * layer lands.
 */
#ifndef LEGORRR_MAINSIM_HPP
#define LEGORRR_MAINSIM_HPP

#include "engine.hpp"

/* Named key bindings (canonical codes, reconstruction-owned). */
#define MNSIM_K_FRAME_RATE 'f' /* Frame Rate Monitor toggle   */
#define MNSIM_K_SAYFB_RENDER 'g' /* render-pass 0x2 toggle      */
#define MNSIM_K_MEMORY 'm'    /* Memory Monitor toggle         */
#define MNSIM_K_SOUNDFX 's'   /* Sound Effects toggle          */
#define MNSIM_K_MUSIC 'x'     /* Music toggle                  */
#define MNSIM_K_AIRCHEAT 'a'  /* air-supply cheat (held)       */
#define MNSIM_K_QUIT 'q'      /* quit request key (ESC is the platform one) */

/* Main simulation / input-app step (corpus MainSimStep). Returns 0 when the
 * frame must quit. `scale`/`dt` are the frame deltas; o1..o3 receive the
 * edge outputs. */
int main_sim_step(EngineContext* ctx, float scale, float dt,
                  uint32_t* o1, uint32_t* o2, uint32_t* o3);

/* Per-key camera step boundary (corpus FUN_00436900/004369f4/00436aa5 on
 * FUN_004560e4..ec). Subs requantized through the view module. */
void main_view_key_step(EngineContext* ctx, int view_idx, uint32_t mode,
                        float amount);

/* HUD text boundary (corpus FUN_00485230 printf-style HUD). */
void main_hud_text(EngineContext* ctx, const char* fmt);

#endif /* LEGORRR_MAINSIM_HPP */