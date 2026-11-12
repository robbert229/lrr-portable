// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what legoobject uses from level.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_LEVEL_FOR_LEGOOBJECT_HPP
#define LRR_GAME_LEVEL_FOR_LEGOOBJECT_HPP

/* --- grid-record count tables (see level.hpp) ------------------------------
 * The corpus's two adjacent tables (DAT pairs 0x4e3944/0x4dee44) as two
 * zero-initialized module statics: no ported code feeds them. The first
 * table is at 0x4e3944; the second, read as the difference's minuend, is at
 * 0x4dee44. */
#define GRID_COUNT_TABLE_DWORDS 0x12c0 /* 300 cells * 16 slots */

/* TerrainObjectTypeSfxAtCellGet (FUN_0044ad80) — see level.hpp. */
#define TERRAIN_SFX_GRID_CELLS 300

extern DockDefinitionCellGetHook s_dock_definition_cell_get_hook;
extern void* s_dock_definition_cell_get_context;
int64_t grid_count_index(int row, int column, int slot);

#endif /* LRR_GAME_LEVEL_FOR_LEGOOBJECT_HPP */
