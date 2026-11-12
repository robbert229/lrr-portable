// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what interface uses from effects.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_EFFECTS_FOR_INTERFACE_HPP
#define LRR_GAME_EFFECTS_FOR_INTERFACE_HPP

extern EventTimerActionHook s_event_timer_action_hook;
extern void* s_event_timer_action_context;

#endif /* LRR_GAME_EFFECTS_FOR_INTERFACE_HPP */
