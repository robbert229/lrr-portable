// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what frontend.cpp uses from helpwindow.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by frontend.cpp only. */
#ifndef LRR_GAME_FRONTEND_FROM_HELPWINDOW_HPP
#define LRR_GAME_FRONTEND_FROM_HELPWINDOW_HPP

#include "frontend_for_helpwindow.hpp"

extern uint32_t s_sel_state;
extern int s_sel_hit_index;
extern uint32_t s_sel_handles_a[SEL_HANDLE_ENTRIES * SEL_HANDLE_STRIDE];
extern uint32_t s_sel_handles_b[SEL_HANDLE_ENTRIES * SEL_HANDLE_STRIDE];
extern uint32_t s_sel_handle_extra;
extern int s_band_mode;
extern uintptr_t s_help_text_area_handle;
extern const char* s_help_info_strings[3][15 * 16];
extern float s_help_window_feedback_position;
extern float s_help_window_feedback_limit;
extern uint32_t s_sel_mode_state;

#endif /* LRR_GAME_FRONTEND_FROM_HELPWINDOW_HPP */
