// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-023 LegoObject.c part 1 (issue #575): the C declarations of the
 * functions and globals of other modules that legoobject_unit.c calls or
 * reads.
 *
 * Every entry has C linkage. Where the owning module has a C++ header, its
 * declaration there carries extern "C" (in place, or as a pre-declaration
 * before a frozen <l>_for_<u>.hpp / <l>_from_<u>.hpp include) and
 * src/game/test_c_boundaries.cpp includes both, so a mismatch fails to
 * compile. They are kept here, not in the owners' C boundary headers, so
 * the other C units' symbol numbering does not change (finding 538); they
 * move to the owners' headers when the owners are converted. Where a
 * callee's contract differs from the original's, the port's contract is
 * used and noted. */
#ifndef LEGORRR_GAME_LEGOOBJECT_UNIT_IMPORTS_H
#define LEGORRR_GAME_LEGOOBJECT_UNIT_IMPORTS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct EntitySlot;
struct EntityAuxiliaryReleaseRecord;

/* ---- LegoObject.c's own state (objectGlobs, 004de790-004eadd0) --------
 * The port defines these in legoobject.cpp, each annotated at the field's
 * address, with C linkage. Parts 2 and 3 of the conversion still read
 * them there. */
extern void* s_pool_chunks[32];                     /* 004de790 listSet */
extern void* s_pool_record_head;                    /* 004de810 freeList */
extern int s_terrain_sfx_grid[300];                 /* 004de814 */
extern const char* s_activity[79];                  /* 004decc4 activityName */
extern uint32_t s_pool_chunk_count;                 /* 004dee10 listCount */
extern uint32_t s_entity_state_flags;               /* 004dee14 flags */
extern uint32_t s_activity_resource_slots[11];      /* 004dee18 */
extern uint32_t s_grid_count_second[0x12c0];        /* 004dee44 */
extern uint32_t s_task_counter_nonzero;             /* 004e2684 */
extern uint32_t s_grid_count_first[0x12c0];         /* 004e3944 */
extern uint32_t s_task_counter_zero;                /* 004e7184 */
extern int s_predug_coordinates[20][2];             /* 004e844c */
extern int s_type11_cell_coordinates[10][2];        /* 004e84ec */
extern unsigned int s_predug_coordinate_count;      /* 004e853c */
extern unsigned int s_type11_cell_coordinate_count; /* 004e8540 */
extern uint32_t s_render_config_snapshot[6];        /* 004ea7ac */
extern struct EntitySlot* s_idle100[100];           /* 004eabcc */
extern int s_idle100_n;                             /* 004ead5c */
extern const char* s_ability_type_names[6];         /* 004ead60 */
extern uint32_t s_global_animation_state;           /* 004eadc4 */
extern struct EntityAuxiliaryReleaseRecord* s_entity_auxiliary_release_records; /* 004eadd0 */

/* ---- group A ---- */

/* ---- group B ---- */

/* ---- group C ---- */

/* ---- group D ---- */

/* ---- group E ---- */

/* ---- group F ---- */

struct BlockRec;
struct TerrainMap;

/* Lego_Object passed to a port entry that takes EntitySlot. */
#define LEGOOBJ_SLOT(o) ((struct EntitySlot*)(o))

/* legoGlobs.toolName (005561fc, ai.cpp's s_tool_type_names) and, right
 * after it in the original's Lego_Globs, langToolName (00556228). */
#define LEGO_TOOL_TYPE_COUNT 11
extern const char* s_tool_type_names[8];                     /* ai.cpp, 005561fc */
#define legoGlobs_toolName (s_tool_type_names)
#define legoGlobs_langToolName ((char**)(s_tool_type_names + LEGO_TOOL_TYPE_COUNT))

/* G98 Config.c (config.h). */
const char* string_path_join(const char* root, ...);                         /* 00478f50 */
char* block_get_str_copy(struct BlockRec* root, const char* path);           /* 00479050 */
/* GAME-021 (lego_unit.c). */
void global_object_field98_clear_current(void);                              /* 0041f8c0 */
/* Lego_GetMap (00431570) takes no argument; the port's entry (level.hpp)
 * takes two it ignores. A cdecl call without them is the original's. */
void* level_current_terrain_get(int cell_x, int cell_y);                     /* 00431570 */
#define Lego_GetMap() ((struct TerrainMap*)((void* (*)(void))level_current_terrain_get)())
uint32_t terrain_cell_flag_8_bounded_current(uint32_t cx, uint32_t cy);      /* 00432b50 */
int terrain_cell_route_and_occupancy_check_current(
    const struct EntitySlot* entity, int from_x, int from_y,
    int to_x, int to_y, int strict);                                         /* 00431da0 */
/* GAME-026 Map3D (terrain_map.h). */
int terrain_map_world_to_block(struct TerrainMap* map, float x, float y,
                               int* bx, int* by);                            /* 0044f6d0 */
/* GAME-043 Stats.c (entity.hpp, entity_from_entity_stats.hpp). */
uint32_t stats_instance_word_at_ec(const void* config_block);                /* 00469bb0 */
uint32_t entity_stats_word_get(const struct EntitySlot* e);                  /* 00469e60 */
uint32_t entity_capability_flags_get(const struct EntitySlot* e);            /* 00469ea0 */
int entity_building_index_set(struct EntitySlot* entity, uint32_t state);    /* 00469c10 */
/* GAME-047 Vehicle.c (record.hpp): Vehicle_GetUpgradeLevel. */
uint32_t entity_type_mask_get(const void* record);                           /* 0046cf80 */
/* Vehicle_SetUpgradeLevel (0046cf90). The port's entry takes a record list
 * and a channel state instead; GAME-021's thin adapter in vehicle_model.cpp. */
void scene_record_indexed_entries_rebuild_original(void* vehicleModel, int upgradeLevel);
/* GAME-045 Text.c (frontend.hpp). */
void advisor_text_animation_update(int slot, int force, int selected);      /* 0046ac60 */
/* LegoObject.c parts 2/3, still port models in legoobject.cpp. Where the
 * port's parameter types differ (float arrays for the route's block
 * coordinates, a float count), the unit passes the original's values
 * through casts. */
void entity_transform_position_get_outputs(const struct EntitySlot* entity,
                                           float* out_x, float* out_y);      /* 00442260 */
int entity_route_setup_and_marker_associate(struct EntitySlot* entity,
                                            unsigned int count,
                                            const float* x_values,
                                            const float* y_values,
                                            const float* final_marker_position); /* 00441700 */
typedef int (*TerrainPathStopHook)(void* level, const uint32_t xy[2],
                                   void* user);
int terrain_neighbor_path_check_current(void* actor, uint32_t start_x,
    uint32_t start_y, uint32_t goal_x, uint32_t goal_y, int** out_x,
    int** out_y, float* out_count, TerrainPathStopHook stop, void* user);     /* 004410f0 */
int entity_flags_mask107c33de_or_1c00203_any_test(const struct EntitySlot* e); /* 0043baf0 */
/* 004424f0 LegoObject_GetBlockPos: the GAME-013 port-tail adapter. */
void legoobject_block_position_get(struct EntitySlot* entity, int* bx, int* by);

/* ---- group G ---- */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_LEGOOBJECT_UNIT_IMPORTS_H */
