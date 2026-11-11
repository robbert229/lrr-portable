// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what tile_variants uses from util.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_UTIL_FOR_TILE_VARIANTS_HPP
#define LRR_GAME_UTIL_FOR_TILE_VARIANTS_HPP

namespace unported_ledger_stubs {
extern CellVisualChannelReleaseHook s_cell_visual_release_hook;
extern void* s_cell_visual_release_context;
} // namespace unported_ledger_stubs

#endif /* LRR_GAME_UTIL_FOR_TILE_VARIANTS_HPP */
