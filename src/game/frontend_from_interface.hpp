// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what frontend.cpp uses from interface.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by frontend.cpp only. */
#ifndef LRR_GAME_FRONTEND_FROM_INTERFACE_HPP
#define LRR_GAME_FRONTEND_FROM_INTERFACE_HPP

#include "frontend_for_interface.hpp"

namespace unported_ledger_stubs {
int simulation_pointer_action_dispatch_impl(int x, int y, int active, int suppress_initial, int count_action);
} // namespace unported_ledger_stubs
extern int s_back_button_width;
extern int s_back_button_height;
extern TextureSurface* s_back_button_highlight;
extern TextureSurface* s_back_button_pressed;
extern uint32_t s_sel_c0c;
extern uint32_t s_sel_c10;
extern float s_sel_c14;
extern float s_sel_c18;
extern float s_reg_e0;
extern float s_reg_e4;
extern float s_reg_e8;
extern float s_reg_ec;
extern float s_reg_f0;
extern float s_obj_track_word;
extern float s_inter_timer;
extern uint32_t s_inter_flags;
extern uint32_t s_status_banner_word;
extern float s_status_blink_timer;
extern uint32_t s_pointer_action_type;
extern uint32_t s_pointer_action_kind;
extern uint32_t s_pointer_action_entry;
extern uint32_t s_map_store_pair_x;
extern uint32_t s_map_store_pair_y;
extern uint32_t s_map_store_last_x;
extern uint32_t s_map_store_last_y;
extern uint32_t s_map_store_kind;
extern uint32_t s_cell_perm[CELL_PERMISSION_ENTRIES];
extern uint32_t s_interface_button_surfaces[2];
extern uint32_t s_cursor_tile_first;
extern uint32_t s_cursor_tile_second;

#endif /* LRR_GAME_FRONTEND_FROM_INTERFACE_HPP */
