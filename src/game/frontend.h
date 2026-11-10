// SPDX-License-Identifier: LGPL-3.0-or-later
/* Front-end functions that units rebuilt as C call.
 *
 * C boundary header (ADR 0005): C declarations, with C linkage, of the
 * front-end functions that units rebuilt as C call. The C++ code keeps
 * using game/frontend.hpp (and, for the two tool-tip
 * entries, frontend.cpp itself), whose matching declarations carry extern "C";
 * src/game/test_c_boundaries.cpp includes both and fails to compile if they
 * disagree. Shared C++ headers do not include this file: every include and
 * declaration a header adds shifts MSVC's symbol numbering, and with it the
 * code generated for unrelated functions (finding 506). */
#ifndef LEGORRR_GAME_FRONTEND_H
#define LEGORRR_GAME_FRONTEND_H

#ifdef __cplusplus
extern "C" {
#endif

void position_tracker_box_draw(const float bounds[4]); /* FUN_0041cd30 */
void advisor_text_animation_update(int slot, int force, int selected);
                                                       /* FUN_0046ac60 */

/* ToolTip_SetContent (FUN_0046b520): format a tool tip's text, or load its
 * image when the text starts with '@', and arm the record. Port adapter:
 * the text path only (see frontend.cpp). */
void tooltip_set_content(int index, const char* format, ...);

/* ToolTip_Activate (FUN_0046b770): request an update of tool tip `index`. */
void timed_effect_update_request_set_corpus(int index);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_FRONTEND_H */
