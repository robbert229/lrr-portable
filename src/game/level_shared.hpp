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

/* --- cell timestamp lock table (see level.hpp) --------------------------
 * Corpus regions: the (x,y) pair slots (stride 8) end at the per-entry
 * active flags (stride 4), the stamps live in their own stride-4 region,
 * and the scan bound is the first address past the flag region — all
 * 1000 entries. */
#define LEVEL_TS_COUNT 1000

typedef struct CellCoordinateTimestamp {
    int32_t x;
    int32_t y;
} CellCoordinateTimestamp;

/* --- erosion timing band (2026-10-25) -------------------------------------
 * These module statics mirror the reference timing, stage, and lock tables;
 * EmergeMap and the live ticker now own their initialization and updates. */
#define EROSION_MARK_COUNT 2000   /* DAT_004cfbb4 */

#define EROSION_TICKET_COUNT LEVEL_TS_COUNT /* DAT_004d6914 */

struct LavaErosionPendingCell {
    int x;
    int y;
};
extern LevelState* s_lava_erosion_level_context;
void cell_init_pass(LevelState* s, float cell_x, uint32_t cell_y, ...);
LevelState* terrain_current_level_resolve(void);
extern CellCoordinateTimestamp s_ts_coordinates[LEVEL_TS_COUNT];
extern int32_t s_ts_stamps[LEVEL_TS_COUNT];
extern uint32_t s_ts_active[LEVEL_TS_COUNT];
extern int32_t s_ts_now;
extern float s_erosion_decay_scale;
extern float s_erosion_delay_scale;
extern LavaErosionPendingCell s_lava_erosion_pending_cells[EROSION_MARK_COUNT];
extern uint32_t s_lava_erosion_pending_count;
extern LavaErosionPendingCell s_lava_erosion_stage_coordinates[ EROSION_MARK_COUNT];
extern uint32_t s_erosion_marks[EROSION_MARK_COUNT];
extern float s_lava_erosion_stage_timers[EROSION_MARK_COUNT];
extern float s_erosion_trigger_accumulator;
extern int s_erosion_triggered_this_tick;
extern unsigned int s_lava_erosion_mutation_threshold;
extern LavaErosionEdgeApplyHook s_lava_erosion_edge_apply;
extern LavaErosionCellActionHook s_lava_erosion_work_clear;
extern LavaErosionCellActionHook s_lava_erosion_cell_change;
extern LavaErosionSoundHook s_lava_erosion_sound;
extern LavaErosionEventHook s_lava_erosion_event;
extern void* s_lava_erosion_hook_context;

/* --- terrain choice resource select (2026-10-25) ---------------------------
 * The first record word selects one of four independently allocated arrays;
 * the second word indexes that array. Kinds 6/7 select scalar resources.
 * An installer exposes array contents because the encyclopedia config
 * loader has no production owner in the portable startup path. */
typedef struct TerrainChoiceWindow {
    uint32_t* values;
    uint32_t count;
} TerrainChoiceWindow;
extern TerrainChoiceWindow s_choice_windows[4];
extern uint32_t s_choice_scalar[2];
extern uint32_t s_choice_active;
extern uint32_t s_choice_flags;
extern uintptr_t s_choice_record;

#endif /* LRR_GAME_LEVEL_SHARED_HPP */
