// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what files uses from level.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_LEVEL_FOR_FILES_HPP
#define LRR_GAME_LEVEL_FOR_FILES_HPP

extern const char s_map_open_binary_mode[];

#endif /* LRR_GAME_LEVEL_FOR_FILES_HPP */
