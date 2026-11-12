// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what weapons uses from simtick.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_SIMTICK_FOR_WEAPONS_HPP
#define LRR_GODS98_SIMTICK_FOR_WEAPONS_HPP

extern const void** s_sfx_entries;
extern int s_sfx_n;
extern int s_sfx_rels;
extern SimtickSfxBankHooks s_sfx_bank_hooks;

#endif /* LRR_GODS98_SIMTICK_FOR_WEAPONS_HPP */
