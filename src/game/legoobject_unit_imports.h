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
struct LevelState;
struct InfoPoint2I;
struct TerrainMap;
/* legoGlobs (Lego.c): currLevel is terrain_grid.cpp's port global; the
 * upgrade costs are stand-ins in lego_unit.c. */
extern struct LevelState* g_active_level_retail;    /* 005560d4 currLevel */
extern int legoGlobs_BuildingUpgradeCostOre;        /* 00556f98 */
extern int legoGlobs_BuildingUpgradeCostStuds;      /* 00556f9c */
/* Lego.c (lego_unit.c). 0041f8e0 is an empty function in this build; the
 * original's caller compares its (undefined) return value. */
void noop_callback(void);                           /* 0041f8e0 */
void global_object_field98_add(int amount);         /* 0041f8a0 */
int lego_upgrade_resource_check(int ore);           /* 0041f900 */
void lego_upgrade_resource_debit(int ore, int amount); /* 0041f9e0 */
/* 00431570 takes no argument; the port's entry takes two ignored cells
 * (adapter at the end of lego.cpp). */
void* level_current_terrain_get_original(void);     /* 00431570 */
/* Info.c (info_unit.c). */
void info_send(int type, char* text, void* object,
               struct InfoPoint2I* blockPos);       /* 00419b80 */
/* Map3D.c (terrain_map.c). */
int terrain_map_block_to_world(struct TerrainMap* map, unsigned int bx,
                               unsigned int by, float* xOut,
                               float* yOut);        /* 0044f640 */
/* Building.c (building_model.cpp): Building_SetUpgradeLevel; the port's
 * entry takes the object for the building model. */
void entity_payload_state_set(struct EntitySlot* e, int state); /* 004089a0 */
/* Stats.c (entity_stats.cpp). */
uint32_t entity_state_flags_get(const struct EntitySlot* e); /* 00469e80 */
/* 00469c10: the port's entry is declared only in a frozen header with C++
 * linkage; C entry at the end of entity_stats.cpp. */
int legoobject_gd_building_index_set_adapter(struct EntitySlot* entity,
                                             uint32_t state); /* 00469c10 */
uint32_t building_upgrade_admission_check(int type, int row); /* 00469f40 */
uint32_t entity_mapped_indexed_record_word_at_fc_get(int group, int index,
                                                     int slot); /* 0046a0c0 */
/* Vehicle.c (vehicle_model.cpp). */
uint32_t record_flag_bit_test(const void* record, uint32_t bit_index,
                              int reject_masked);   /* 0046cf40 */
/* Container.c (container_unit.cpp). */
int ddiscene_object_position_get(void* object, void* alternate_frame,
                                 float position[3]); /* 004754c0 */
/* Effects (rock_fall_styles.cpp). */
int lava_effect_select_and_place(int slot, const float* position,
                                 const float* rotation); /* 0040cce0 */
/* HelpWindow.c (helpwindow.cpp, namespace lego). */
void building_upgrade_state_changed_dispatch(int type, int row, int column,
                                             int state); /* 00418830 */
/* LegoObject.c parts 2-3 (legoobject.cpp, legoobject.h). */
void entity_transform_position_get_outputs(const struct EntitySlot* entity,
                                           float* out_x,
                                           float* out_y); /* 00442260 */
void entity_world_position_set(struct EntitySlot* entity, float x, float y,
                               float heading, int update_heading); /* 004428a0 */
/* 004424f0 LegoObject_GetBlockPos: adapter in the GAME-013 port tail
 * (legoobject.h). */
void legoobject_block_position_get(struct EntitySlot* entity, int* bx,
                                   int* by);
void* entity_type_component_get(const void* entity_record); /* 0044a200 */


/* ---- group E ---- */

/* ---- group F ---- */

/* ---- group G ---- */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_LEGOOBJECT_UNIT_IMPORTS_H */
