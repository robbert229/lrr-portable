// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what lego uses from level.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_LEVEL_FOR_LEGO_HPP
#define LRR_GAME_LEVEL_FOR_LEGO_HPP

typedef struct CellTransientEffectPort {
    LevelState* level;
    void* record;
    void* emitter;
    struct CellTransientEffectPort* next;
} CellTransientEffectPort;

struct TerrainActivityBuildRecordModel {
    LevelState* level;
    int x;
    int y;
    int enabled;
    uint8_t scene_flags_0c;
};

#define LEVEL_CELL_REFRESH_TRACK_CAP 100

#define LEVEL_CELL_VISITED_TRACK_CAP 100

/* SurfaceTypeDescriptionsConfigLoad owns the configured surface-name/value
 * tables consumed by the surface-type query helpers below. */
#define SURFACE_TYPE_SLOTS 18

extern TerrainCellOverlayHeightFillHook s_terrain_cell_overlay_height_fill;
extern void* s_terrain_cell_overlay_height_fill_context;
extern EngineState s_level_field_grid_state;
extern LevelState* s_level_field_grid_owner;
void terrain_cell_refresh_dispatch_level(LevelState* lvl, const int cell_xy[2], int mode);
void map_cell_tracking_activity_state_update(GameContext* game, LevelState* level);
extern CellTransientEffectPort* s_cell_transient_effect_ports;
CellTransientEffectPort* cell_transient_effect_port_find( const LevelState* level, const void* record);
void* cell_transient_effect_get(LevelState* level, void* record);
extern TerrainActivityBuildRecordModel s_terrain_activity_build_records[256];
const char* cfg_resolve_path(BlockFile* cfg, const char* level, const char* key, char* out, size_t out_sz);
int cfg_typed(BlockFile* cfg, const char* level, const char* key, int dflt);
const uint8_t* cell_record_unchecked_const(const LevelState* s, int cx, int cy);
uint32_t cell_flag_word_unchecked(const LevelState* s, int cx, int cy);
extern int32_t s_cell_refresh_track[LEVEL_CELL_REFRESH_TRACK_CAP][2];
extern int s_cell_refresh_track_count;
extern int32_t s_cell_previous_track[LEVEL_CELL_REFRESH_TRACK_CAP][2];
extern int s_cell_previous_track_count;
extern int32_t s_cell_visited_track[LEVEL_CELL_VISITED_TRACK_CAP][2];
extern int s_cell_visited_track_count;
extern TerrainNeighborRenderUpdateHook s_terrain_neighbor_render_hook;
extern void* s_terrain_neighbor_hook_context;
extern TerrainNeighborActionHook s_terrain_neighbor_build_action;
extern TerrainNeighborActionHook s_terrain_neighbor_clear_action;
extern void* s_terrain_neighbor_action_context;
extern GridCellEligibilityHooks s_grid_cell_eligibility_hooks;
extern const char* s_surface_type_names[SURFACE_TYPE_SLOTS];
extern uint32_t s_surface_type_values[SURFACE_TYPE_SLOTS];
extern TerrainCellLinkedEntryRemoveHook s_terrain_cell_linked_entry_remove_hook;
extern void* s_terrain_cell_linked_entry_remove_context;
void terrain_cell_state_effect_spawn(LevelState* level, const int cell_xy[2]);
extern const int s_cardinal_deltas[4][2];
uint32_t route_cell_flags(const LevelState* s, int x, int y);

#endif /* LRR_GAME_LEVEL_FOR_LEGO_HPP */
