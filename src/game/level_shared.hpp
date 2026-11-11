// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private to src/game/level.cpp and the units relocated out of it (ADR 0006
 * decision 4; tools/relocate_unit.py). Statics both sides use have
 * external linkage through the declarations below. Include it only from
 * those files: it is not a shared header. */
#ifndef LRR_GAME_LEVEL_SHARED_HPP
#define LRR_GAME_LEVEL_SHARED_HPP

/* --- dependency and terrain grid table fallback (see level.hpp) -----------
 * DAT 0x4b8bc8..0x4c71fc is a 300-cell, 0x31-dword-stride region addressed
 * by y + x * 15. For each cell, the block at dword 32 + cell * 0x31 holds
 * the selector, then sixteen field-flag words. Parallel counts start at
 * cell * 0x31 and pointer words at cell * 0x31 + 16. The active-level
 * DependencyGridConfigLoad path stores native pointer/count pairs in its
 * LevelState sidecar; this zeroed region is the no-level fallback. */
#define GRID_REC_REGION_DWORDS 14712 /* (0x4c71fc - 0x4b8bc8) / 4 */

#define GRID_REC_STRIDE        0x31  /* dwords per record block */

extern uint32_t s_grid_rec[GRID_REC_REGION_DWORDS];

#endif /* LRR_GAME_LEVEL_SHARED_HPP */
