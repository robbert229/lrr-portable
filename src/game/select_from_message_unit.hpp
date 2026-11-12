// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what select.cpp uses from message_unit.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by select.cpp only. */
#ifndef LRR_GAME_SELECT_FROM_MESSAGE_UNIT_HPP
#define LRR_GAME_SELECT_FROM_MESSAGE_UNIT_HPP

extern uint32_t s_selection_active_count;
extern EntitySlot* s_selection_active_records[SELECTION_MAX];

#endif /* LRR_GAME_SELECT_FROM_MESSAGE_UNIT_HPP */
