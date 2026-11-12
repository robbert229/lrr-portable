// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what aitask uses from level.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_LEVEL_FOR_AITASK_HPP
#define LRR_GAME_LEVEL_FOR_AITASK_HPP

/* --- coordinate-callback dispatchers (see level.hpp) -------------------------
 * The target walks EntityPoolCallbackSearch with LAB_00402bb0, whose body is
 * absent from the recovered corpus, then resets the terrain surface type.
 * Preserve the known traversal and ordering while leaving the callback's
 * effects to an explicit runtime binding. */
typedef struct TerrainCellRefreshCallbackPayload {
    const int* cell_xy;
    int mode;
} TerrainCellRefreshCallbackPayload;

struct CoordinateEntityDispatchPayload {
    const void* coordinate_pair;
    int extra;
    CoordinateEntityDispatchHook callback;
};

int terrain_cell_refresh_callback_adapter(EntitySlot* entity, void* payload);
extern CoordinateEntityDispatchHook s_coordinate_direct_callback;
extern CoordinateEntityDispatchHook s_coordinate_mode_zero_callback;
extern CoordinateEntityDispatchHook s_coordinate_mode_nonzero_callback;
int coordinate_entity_dispatch_adapter(EntitySlot* entity, void* payload);

#endif /* LRR_GAME_LEVEL_FOR_AITASK_HPP */
