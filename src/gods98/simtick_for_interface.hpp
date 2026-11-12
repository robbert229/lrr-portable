// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what interface uses from simtick.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_SIMTICK_FOR_INTERFACE_HPP
#define LRR_GODS98_SIMTICK_FOR_INTERFACE_HPP

extern int (*s_mute_hook)(uint32_t id, int vol);

#endif /* LRR_GODS98_SIMTICK_FOR_INTERFACE_HPP */
