// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what lego uses from util.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_UTIL_FOR_LEGO_HPP
#define LRR_GAME_UTIL_FOR_LEGO_HPP

extern void (*s_resource_entry_release_hook)(void*);
extern void (*s_resource_array_release_hook)(void*);

#endif /* LRR_GAME_UTIL_FOR_LEGO_HPP */
