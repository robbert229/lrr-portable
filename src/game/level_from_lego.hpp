// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what level.cpp uses from lego.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by level.cpp only. */
#ifndef LRR_GAME_LEVEL_FROM_LEGO_HPP
#define LRR_GAME_LEVEL_FROM_LEGO_HPP

int level_cell_flag8_check_target( const LevelState* unused, int cx, int cy);
float engine_float_field260_get_current_target(void);
uint32_t global_state_field84_get_current_target(void);
uint8_t terrain_neighbor_coordinate_check_current_target( int x, int y, uint8_t* kind, int mode);
extern uint32_t s_neighbor_pick_rotation;

#endif /* LRR_GAME_LEVEL_FROM_LEGO_HPP */
