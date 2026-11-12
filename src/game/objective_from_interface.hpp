// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what objective.cpp uses from interface.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by objective.cpp only. */
#ifndef LRR_GAME_OBJECTIVE_FROM_INTERFACE_HPP
#define LRR_GAME_OBJECTIVE_FROM_INTERFACE_HPP

void advisor_animation_position_and_request_update_internal( int position_mode, int input_a, int input_b, const float* position, const TextureSurface* image, int typed);

#endif /* LRR_GAME_OBJECTIVE_FROM_INTERFACE_HPP */
