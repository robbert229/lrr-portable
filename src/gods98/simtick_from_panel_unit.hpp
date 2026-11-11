// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what simtick.cpp uses from panel_unit.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by simtick.cpp only. */
#ifndef LRR_GODS98_SIMTICK_FROM_PANEL_UNIT_HPP
#define LRR_GODS98_SIMTICK_FROM_PANEL_UNIT_HPP

extern "C" uint32_t simtick_timer_word;

#endif /* LRR_GODS98_SIMTICK_FROM_PANEL_UNIT_HPP */
