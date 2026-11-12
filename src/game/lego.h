// SPDX-License-Identifier: LGPL-3.0-or-later
/* Lego.c (original translation unit GAME-021) state and entry points that
 * units rebuilt as C use. Lego.c is not converted: these are the port's
 * declarations, with C linkage.
 *
 * C boundary header (ADR 0005). The C++ code keeps using lego's C++ headers
 * (level.hpp, gods98/media.hpp, gods98/engine.hpp, port_loop_ports.hpp),
 * whose matching declarations carry extern "C";
 * src/game/test_c_boundaries.cpp includes them with this header. Shared C++
 * headers do not include this file (finding 506). */
#ifndef LEGORRR_GAME_LEGO_H
#define LEGORRR_GAME_LEGO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct LevelState;
struct EngineContext;
struct TextureSurface;

/* legoGlobs.flags1 (DAT_00556ecc). The port keeps the word in its
 * EngineContext, so the unit reads it through this pointer. */
uint32_t* engine_shared_state_word(void);

/* DAT_00556ed0: the debug and panel-highlight flag word. */
extern uint32_t s_panel_highlight_flags;

/* The two fonts Panel.c draws with (DAT_00556124, DAT_00556128), and the
 * radar zoom (DAT_005563f4). The port has no owner of the original
 * legoGlobs fields yet, so these port globals stand in for them. */
extern struct TextureSurface* s_lego_text_window_font;
extern struct TextureSurface* s_lego_crystal_count_font;
extern float s_lego_radar_zoom;

/* FUN_00425c30 Lego_SetPaused-style transition control. */
void ledger_stub_00425c30_TransitionControl(int enabled, uint32_t parameter);
/* FUN_00428800: the interface click sound (0 accepted, 1 refused). */
void media_action_feedback_sound_dispatch(int event);
/* FUN_00431560: the current level. The original reads its objective result
 * at +0x27c. */
struct LevelState* level_current_owner_get(void);
/* FUN_00435aa0: the call-to-arms highlight, lego_unit.c's original-shape
 * function (the port's entry, which also takes the engine context, stays in
 * lego_port.cpp for native callers). */
void lego_panel_selection_highlight_toggle(int on); /* 00435aa0, lego_unit.c */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_LEGO_H */
