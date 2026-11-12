// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what aitask uses from simtick.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_SIMTICK_FOR_AITASK_HPP
#define LRR_GODS98_SIMTICK_FOR_AITASK_HPP

extern SimtickBobbinActionHook s_bobbin_clear_delay_hook;

#endif /* LRR_GODS98_SIMTICK_FOR_AITASK_HPP */
