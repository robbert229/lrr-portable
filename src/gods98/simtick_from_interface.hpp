// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what simtick.cpp uses from interface.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by simtick.cpp only. */
#ifndef LRR_GODS98_SIMTICK_FROM_INTERFACE_HPP
#define LRR_GODS98_SIMTICK_FROM_INTERFACE_HPP

extern uint32_t s_mute_word;
extern int s_mute_engaged;

#endif /* LRR_GODS98_SIMTICK_FROM_INTERFACE_HPP */
