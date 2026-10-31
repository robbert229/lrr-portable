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
 *   3. camera key slots and bounded camera-controller updates (slots are
 *      identified; the controller record is not represented by ViewContext)
 *   4. the three edge outputs o1/o2/o3 that EngineTick's later tickers read
 *
 * The camera-slot constants below are offsets into the current-key table,
 * derived from MainSimStep's DAT_0076ab80-relative global addresses. Other
 * bindings remain reconstruction-owned. Selecting objects / menus / the
 * rename touch (FUN_004525c0 + FUN_00437880) stay stub boundaries until the
 * entity layer lands.
 */
#ifndef LEGORRR_MAINSIM_HPP
#define LEGORRR_MAINSIM_HPP

#include "engine.hpp"

/* Key bindings (canonical codes, reconstruction-owned). */
#define MNSIM_K_FRAME_RATE 'f' /* Frame Rate Monitor toggle   */
#define MNSIM_K_SAYFB_RENDER 'g' /* render-pass 0x2 toggle      */
#define MNSIM_K_MEMORY 'm'    /* Memory Monitor toggle         */
#define MNSIM_K_SOUNDFX 's'   /* Sound Effects toggle          */
#define MNSIM_K_MUSIC 'x'     /* Music toggle                  */
#define MNSIM_K_AIRCHEAT 'a'  /* air-supply cheat (held)       */
#define MNSIM_K_QUIT 'q'      /* quit request key (ESC is the platform one) */
/* MainSimStep reads these original byte slots from DAT_0076ab80. */
#define MNSIM_K_CAMERA_PAN_POS  0xd0 /* DAT_0076ac50: KEY_CURSORDOWN */
#define MNSIM_K_CAMERA_PAN_NEG  0xc8 /* DAT_0076ac48: KEY_CURSORUP */
#define MNSIM_K_CAMERA_TILT_POS 0xcb /* DAT_0076ac4b: KEY_CURSORLEFT */
#define MNSIM_K_CAMERA_TILT_NEG 0xcd /* DAT_0076ac4d: KEY_CURSORRIGHT */
#define MNSIM_K_CAMERA_ZOOM_POS 0x0c /* DAT_0076ab8c: KEY_MINUS */
#define MNSIM_K_CAMERA_ZOOM_NEG 0x0d /* DAT_0076ab8d: KEY_EQUALS */

/* Main simulation / input-app step (corpus MainSimStep). Returns 0 when the
 * frame must quit. `scale`/`dt` are the frame deltas; o1..o3 receive the
 * edge outputs. */
int main_sim_step(EngineContext* ctx, float scale, float dt,
                  uint32_t* o1, uint32_t* o2, uint32_t* o3);

/* HUD text boundary (corpus FUN_00485230 printf-style HUD). */
void main_hud_text(EngineContext* ctx, const char* fmt);

#endif /* LEGORRR_MAINSIM_HPP */
