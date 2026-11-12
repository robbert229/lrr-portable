// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what interface uses from objective.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_OBJECTIVE_FOR_INTERFACE_HPP
#define LRR_GAME_OBJECTIVE_FOR_INTERFACE_HPP

extern AdvisorAnimationPositionUpdateState s_advisor_position_update_state;
extern AdvisorPositionUpdateValueResolveFn s_advisor_position_value_resolver;
extern void* s_advisor_position_value_resolver_user;

#endif /* LRR_GAME_OBJECTIVE_FOR_INTERFACE_HPP */
