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
/* The lava-erosion unit's own definition must be the first declaration it
 * sees, after its 25.0f constant: MSVC 5.0 orders the memory operands of a
 * float product by declaration (finding 538), and the original multiplies
 * by this word before the constant (0x0040e9c0). */
#if !defined(LRR_LAVA_EROSION_UNIT)
extern float s_erosion_delay_scale;
#endif
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

#define PATH_RECORD_WINDOW 0xd4 /* the corpus's 53-dword window           */

typedef struct PathRecordPayloadPort {
    const void* record;
    void* payload;
    uint32_t count;
    struct PathRecordPayloadPort* next;
} PathRecordPayloadPort;

typedef struct PathRecordEntityPort {
    const void* record;
    uint32_t count;
    struct EntitySlot* entities[40];
    struct PathRecordEntityPort* next;
} PathRecordEntityPort;

namespace unported_ledger_stubs {
int ledger_stub_00409470_NeighborCellEntitySearchRecurse( const int cell_xy[2], int resource_amount);
} // namespace unported_ledger_stubs
TerrainSurfaceGrid* level_surface_grid_get(const LevelState* level);
extern uint32_t s_path_record_count;
extern void* s_path_record_last;
extern PathRecordPayloadPort* s_path_record_payload_ports;
extern PathRecordEntityPort* s_path_record_entity_ports;
namespace unported_ledger_stubs {
void placement_cell_search_expand(const int cell_xy[2]);
} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {
void entity_cell_path_effects_apply_level(EntitySlot* entity, LevelState* level);
} // namespace unported_ledger_stubs

#define GRID_REC_COLS          15    /* 20 rows by 15 columns */

void terrain_cell_record_get_target( int x, int y, int field, uint32_t* value_a, uint32_t* value_b);
uint32_t cell_flag4_is_set_target(int x, int y, int field);
void cell_flag100_set_target(int x, int y, int field);
uint32_t cell_flag100_is_set_target(int x, int y, int field);

#define TERRAIN_BLOCK_STRIDE 0x28

#define LEVEL_DIRTY_QUEUE_CAP 2500

typedef struct TerrainIndexedMeshObject {
    void* mesh_record;
} TerrainIndexedMeshObject;

typedef struct TerrainState {
    uint32_t dim[4];      /* [0]=w-1 [1]=h-1 [2]=w [3]=h */
    float    blocksize;
    float    terrain_value; /* TerrainCreate's fourth source parameter */
    float    world_off_x; /* -(w-1)*blocksize*0.5 */
    float    world_off_y; /*  (h-1)*blocksize*0.5 */
    uint32_t dirty_flags; /* TerrainCreate +0x73d0 sidecar word */
    uint32_t detail_mesh_token; /* retail +0x24 mode-2 indexed mesh object */
    uint8_t* blocks;      /* w*h*0x28 physical block table */
    uint8_t* variant_window; /* cell variant relation tables + header */
    float*   cell_vertices; /* four XYZ vertices per terrain quad */
    uint32_t* cell_detail_flags; /* indexed mesh +0x6c bit-0 sidecar */
    struct TerrainIndexedMeshRecord {
        uint32_t block_count;
        uint32_t reserved;
        void* blocks;
    } indexed_mesh;
    TerrainIndexedMeshObject detail_mesh_object;
    uint8_t* indexed_mesh_blocks; /* 0x70-byte indexed mesh entries */
    float* indexed_mesh_vertices; /* four 0x20-byte vertices per quad */
    /* GraphicsOverlayResources' separate quad list (0x552180/0x552198).
     * It owns derived output; cell_vertices remains the mutable base mesh. */
    TerrainCellQuadEmitGrid derived_mesh;
    uint32_t derived_mesh_dirty;
    float* derived_vertices; /* separate ordered retail X,Z,height quads */
    void* roof_scene;        /* genuine pooled native frame owner */
    void* roof_root;
    TerrainSurfaceGrid* detail_surface; /* physical-quad clear/restore owner */
    float    minimum_y;
    int      width, height;
    LevelState* owner_level; /* live current-level SceneRender quad owner */
    uint16_t dirty_queue_x[LEVEL_DIRTY_QUEUE_CAP];
    uint16_t dirty_queue_y[LEVEL_DIRTY_QUEUE_CAP];
    uint32_t dirty_queue_count;
    void* scene_context; /* portable sidecar for the input scene root */
} TerrainState;

extern LevelTerrainMaterialTransitionMeshHook s_material_transition_mesh_hook;
extern void* s_material_transition_mesh_context;
void terrain_indexed_mesh_cell_sync(TerrainState* terrain, size_t quad_index);
void terrain_create_cell_surface_vectors_rebuild(TerrainState* terrain, uint32_t quad_index, uint32_t record_index, int flipped);
int level_coord_pair_within_bounds_target( const uint32_t* dimensions, uint32_t cx, uint32_t cy);
extern void* s_cell_list_object;
extern int s_cell_list_scale;
extern uint32_t s_cell_list_flags;

uint8_t* voxel_at(LevelState* s, int x, int y);

#endif /* LRR_GAME_LEVEL_SHARED_HPP */
