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
struct LevelState;
struct TerrainMap;
struct TerrainMapVector3;
struct InfoPoint2I;
/* legoGlobs stand-ins defined in lego_unit.c (GAME-021). */
extern void* legoGlobs_contCrystal;                 /* 00556374 */
extern void* legoGlobs_contOresTable;               /* 0055637c */
extern char** legoGlobs_vehicleName;                /* 005561e8 */
extern char** legoGlobs_miniFigureName;             /* 005561ec */
extern char** legoGlobs_rockMonsterName;            /* 005561f0 */
extern char** legoGlobs_buildingName;               /* 005561f4 */
/* legoGlobs' object-name tables, defined in entity.cpp (C linkage from the
 * pre-declarations before entity_for_legoobject.hpp). */
extern const char* s_object_name_special[9];        /* 0055618c */
extern const char* s_object_the_name_special[9];    /* 005561b0 */
extern const char** s_object_name_kind1;            /* 00556254 */
extern const char** s_object_name_kind2;            /* 00556258 */
extern const char** s_object_name_kind3;            /* 0055625c */
extern const char** s_object_name_kind4;            /* 00556260 */
extern const char** s_object_name_kind10;           /* 00556264 */
extern const char** s_object_the_name_kind1;        /* 00556268 */
extern const char** s_object_the_name_kind2;        /* 0055626c */
extern const char** s_object_the_name_kind3;        /* 00556270 */
extern const char** s_object_the_name_kind4;        /* 00556274 */
extern const char** s_object_the_name_kind10;       /* 00556278 */
/* GAME-021 Lego.c (lego.cpp, level.hpp, entity_api.h). */
int entity_type_pair_lookup(const char* name, int* type_out, int* row_out,
                            int* zero_out);                     /* 0042e8b0 */
int terrain_cell_byte_state_classify_current(const int xy[2],
    uint32_t* out_a, uint32_t* out_b, uint32_t* out_c,
    uint32_t* out_d);                                           /* 0042c5b0 */
struct LevelState* level_current_owner_get(void);               /* 00431560 */
/* 00431570 takes no argument; adapter at the end of lego.cpp. */
void* level_current_terrain_get_original(void);                 /* 00431570 */
/* GAME-019 Info.c (info_unit.h). */
void info_send(int type, char* text, void* object,
               struct InfoPoint2I* blockPos);                   /* 00419b80 */
/* GAME-023 parts 2-3 (legoobject.cpp, entity.hpp). */
void entity_transform_position_get_outputs(const struct EntitySlot* entity,
                                           float* out_x,
                                           float* out_y);       /* 00442260 */
/* Map3D (terrain_map.h). */
float terrain_context_scale_get(const void* context);           /* 00450890 */
int terrain_map_block_vertices_get(struct TerrainMap* map, unsigned int bx,
                                   unsigned int by,
                                   struct TerrainMapVector3* positions); /* 004500d0 */
/* Message.c (message.h). */
void cmd_produce(int op, const void* a1, int a2, const int* pair); /* 00452060 */
/* Stats.c (entity.hpp). */
uint32_t entity_stats_word_get(const struct EntitySlot* e);    /* 00469e60 */
uint32_t entity_state_flags_get(const struct EntitySlot* e);   /* 00469e80 */
/* Text.c (frontend.hpp). */
void advisor_text_animation_update(int slot, int force, int selected); /* 0046ac60 */
/* Maths.c (maths.h). */
float random_float_range(float low, float high);                /* 004798b0 */

/* ---- group F ---- */

/* ---- group G ---- */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_LEGOOBJECT_UNIT_IMPORTS_H */
